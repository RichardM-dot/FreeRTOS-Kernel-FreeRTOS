/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * 【中文导读】croutine.c —— 协程（co-routine，现已少用）
 * ----------------------------------------------------------------------------
 * 比 task 更省内存的"伪并发"：共享栈、显式 yield 让出执行权。
 * 小 RAM 设备的历史方案，现代项目一律用 task，本文件了解即可。
 * ============================================================================
 */

#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

/* 如果未使用协程，可删除整个文件。 */
#if ( configUSE_CO_ROUTINES != 0 )

/*
 * 某些内核感知调试器要求所查看的数据具有全局作用域，
 * 而非文件作用域。
 */
    #ifdef portREMOVE_STATIC_QUALIFIER
        #define STATIC
    #else
        #define STATIC    static
    #endif


/* 就绪与阻塞协程使用的链表。 -------------------- */
    STATIC List_t pxReadyCoRoutineLists[ configMAX_CO_ROUTINE_PRIORITIES ]; /**< 按优先级排列的就绪协程。 */
    STATIC List_t xDelayedCoRoutineList1;                                   /**< 延时中的协程。 */
    STATIC List_t xDelayedCoRoutineList2;                                   /**< 延时中的协程（使用两个链表：其中一个用于保存延时已溢出当前时钟节拍计数的协程）。 */
    STATIC List_t * pxDelayedCoRoutineList = NULL;                          /**< 指向当前正在使用的延时协程链表。 */
    STATIC List_t * pxOverflowDelayedCoRoutineList = NULL;                  /**< 指向当前正在使用的延时协程链表，其中保存延时已溢出当前时钟节拍计数的协程。 */
    STATIC List_t xPendingReadyCoRoutineList;                               /**< 保存由外部事件置为就绪的协程。这些协程不能直接加入就绪链表，因为中断无法访问就绪链表。 */

/* 其他文件私有变量。 -------------------------------- */
    CRCB_t * pxCurrentCoRoutine = NULL;
    STATIC UBaseType_t uxTopCoRoutineReadyPriority = ( UBaseType_t ) 0U;
    STATIC TickType_t xCoRoutineTickCount = ( TickType_t ) 0U;
    STATIC TickType_t xLastTickCount = ( TickType_t ) 0U;
    STATIC TickType_t xPassedTicks = ( TickType_t ) 0U;

/* 协程创建时的初始状态。 */
    #define corINITIAL_STATE    ( 0 )

/*
 * 将 pxCRCB 所代表的协程放入与其优先级对应的就绪队列，
 * 插入位置为链表末尾。
 *
 * 本宏会访问协程就绪链表，因此不得在中断服务程序（ISR）中使用。
 */
    #define prvAddCoRoutineToReadyQueue( pxCRCB )                                                                               \
    do {                                                                                                                        \
        if( ( pxCRCB )->uxPriority > uxTopCoRoutineReadyPriority )                                                              \
        {                                                                                                                       \
            uxTopCoRoutineReadyPriority = ( pxCRCB )->uxPriority;                                                               \
        }                                                                                                                       \
        vListInsertEnd( ( List_t * ) &( pxReadyCoRoutineLists[ ( pxCRCB )->uxPriority ] ), &( ( pxCRCB )->xGenericListItem ) ); \
    } while( 0 )

/*
 * 用于初始化调度器所使用的全部链表。创建第一个协程时，
 * 本函数会被自动调用。
 */
    STATIC void prvInitialiseCoRoutineLists( void );

/*
 * 由中断置为就绪的协程不能直接放入就绪链表（二者之间没有互斥保护）。
 * 这些协程先被放入待处理就绪链表，以便随后由协程调度器将其移入
 * 就绪链表。
 */
    STATIC void prvCheckPendingReadyList( void );

/*
 * 本函数检查当前处于延时状态的协程链表，判断是否有协程需要唤醒。
 *
 * 协程按唤醒时间的先后顺序存放在队列中，因此一旦发现某个协程的
 * 定时器尚未到期，就无需继续向后遍历链表。
 */
    STATIC void prvCheckDelayedList( void );

/*-----------------------------------------------------------*/

    BaseType_t xCoRoutineCreate( crCOROUTINE_CODE pxCoRoutineCode,
                                 UBaseType_t uxPriority,
                                 UBaseType_t uxIndex )
    {
        BaseType_t xReturn;
        CRCB_t * pxCoRoutine;

        traceENTER_xCoRoutineCreate( pxCoRoutineCode, uxPriority, uxIndex );

        /* 分配用于存放协程控制块（CRCB）的内存。 */
        /* MISRA 参考 11.5.1 [Malloc 内存分配] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation]（MISRA C:2012 规则 11.5 违规的 Coverity 抑制标记） */
        pxCoRoutine = ( CRCB_t * ) pvPortMalloc( sizeof( CRCB_t ) );

        if( pxCoRoutine )
        {
            /*
             * 如果 pxCurrentCoRoutine 为 NULL，则说明这是创建的第一个协程，
             * 需要对协程数据结构进行初始化。
             */
            if( pxCurrentCoRoutine == NULL )
            {
                pxCurrentCoRoutine = pxCoRoutine;
                prvInitialiseCoRoutineLists();
            }

            /* 检查优先级是否在允许范围内。 */
            if( uxPriority >= configMAX_CO_ROUTINE_PRIORITIES )
            {
                uxPriority = configMAX_CO_ROUTINE_PRIORITIES - 1;
            }

            /* 使用函数参数填充协程控制块。 */
            pxCoRoutine->uxState = corINITIAL_STATE;
            pxCoRoutine->uxPriority = uxPriority;
            pxCoRoutine->uxIndex = uxIndex;
            pxCoRoutine->pxCoRoutineFunction = pxCoRoutineCode;

            /* 初始化协程控制块的其他所有参数。 */
            vListInitialiseItem( &( pxCoRoutine->xGenericListItem ) );
            vListInitialiseItem( &( pxCoRoutine->xEventListItem ) );

            /*
             * 将协程控制块设置为 ListItem_t 的反向链接。
             * 这样便能从链表中的通用列表项
             * 找回其所属的 CRCB。
             */
            listSET_LIST_ITEM_OWNER( &( pxCoRoutine->xGenericListItem ), pxCoRoutine );
            listSET_LIST_ITEM_OWNER( &( pxCoRoutine->xEventListItem ), pxCoRoutine );

            /* 事件链表始终按优先级排序。 */
            listSET_LIST_ITEM_VALUE( &( pxCoRoutine->xEventListItem ), ( ( TickType_t ) configMAX_CO_ROUTINE_PRIORITIES - ( TickType_t ) uxPriority ) );

            /*
             * 协程初始化完成后，便可将其加入
             * 对应优先级的就绪链表。
             */
            prvAddCoRoutineToReadyQueue( pxCoRoutine );

            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }

        traceRETURN_xCoRoutineCreate( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    void vCoRoutineAddToDelayedList( TickType_t xTicksToDelay,
                                     List_t * pxEventList )
    {
        TickType_t xTimeToWake;

        traceENTER_vCoRoutineAddToDelayedList( xTicksToDelay, pxEventList );

        /*
         * 计算唤醒时间——该值可能溢出，但
         * 不会造成问题。
         */
        xTimeToWake = xCoRoutineTickCount + xTicksToDelay;

        /*
         * 必须先将自己从就绪链表中移除，再加入阻塞链表，
         * 因为这两个链表使用的是同一个列表项。
         */
        ( void ) uxListRemove( ( ListItem_t * ) &( pxCurrentCoRoutine->xGenericListItem ) );

        /* 该列表项将按唤醒时间顺序插入。 */
        listSET_LIST_ITEM_VALUE( &( pxCurrentCoRoutine->xGenericListItem ), xTimeToWake );

        if( xTimeToWake < xCoRoutineTickCount )
        {
            /*
             * 唤醒时间已溢出，将该列表项放入
             * 溢出链表。
             */
            vListInsert( ( List_t * ) pxOverflowDelayedCoRoutineList, ( ListItem_t * ) &( pxCurrentCoRoutine->xGenericListItem ) );
        }
        else
        {
            /*
             * 唤醒时间未溢出，因此使用
             * 当前的阻塞链表。
             */
            vListInsert( ( List_t * ) pxDelayedCoRoutineList, ( ListItem_t * ) &( pxCurrentCoRoutine->xGenericListItem ) );
        }

        if( pxEventList )
        {
            /*
             * 同时将协程加入事件链表。若执行此操作，
             * 则调用本函数时必须关闭中断。
             */
            vListInsert( pxEventList, &( pxCurrentCoRoutine->xEventListItem ) );
        }

        traceRETURN_vCoRoutineAddToDelayedList();
    }
/*-----------------------------------------------------------*/

    STATIC void prvCheckPendingReadyList( void )
    {
        /*
         * 是否有协程等待被移入就绪链表？这些协程是由中断服务程序（ISR）
         * 置为就绪的。ISR 本身无法访问就绪链表。
         */
        while( listLIST_IS_EMPTY( &xPendingReadyCoRoutineList ) == pdFALSE )
        {
            CRCB_t * pxUnblockedCRCB;

            /* 待处理就绪链表可由 ISR 访问。 */
            portDISABLE_INTERRUPTS();
            {
                pxUnblockedCRCB = ( CRCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &xPendingReadyCoRoutineList ) );
                ( void ) uxListRemove( &( pxUnblockedCRCB->xEventListItem ) );
            }
            portENABLE_INTERRUPTS();

            ( void ) uxListRemove( &( pxUnblockedCRCB->xGenericListItem ) );
            prvAddCoRoutineToReadyQueue( pxUnblockedCRCB );
        }
    }
/*-----------------------------------------------------------*/

    STATIC void prvCheckDelayedList( void )
    {
        CRCB_t * pxCRCB;

        xPassedTicks = xTaskGetTickCount() - xLastTickCount;

        while( xPassedTicks )
        {
            xCoRoutineTickCount++;
            xPassedTicks--;

            /* 如果时钟节拍计数已溢出，则需要交换延时链表。 */
            if( xCoRoutineTickCount == 0 )
            {
                List_t * pxTemp;

                /*
                 * 时钟节拍计数已溢出，因此需要交换延时链表。如果此时
                 * pxDelayedCoRoutineList 中仍有列表项，则说明存在错误！
                 */
                pxTemp = pxDelayedCoRoutineList;
                pxDelayedCoRoutineList = pxOverflowDelayedCoRoutineList;
                pxOverflowDelayedCoRoutineList = pxTemp;
            }

            /* 检查本次时钟节拍是否使某个超时到期。 */
            while( listLIST_IS_EMPTY( pxDelayedCoRoutineList ) == pdFALSE )
            {
                pxCRCB = ( CRCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedCoRoutineList );

                if( xCoRoutineTickCount < listGET_LIST_ITEM_VALUE( &( pxCRCB->xGenericListItem ) ) )
                {
                    /* 超时尚未到期。 */
                    break;
                }

                portDISABLE_INTERRUPTS();
                {
                    /*
                     * 事件可能恰好在此临界区之前发生。若如此，
                     * 通用列表项已被移至待处理就绪链表，
                     * 下面一行仍然有效。同时 pvContainer 参数
                     * 已被置为 NULL，因此后续几行也同样有效。
                     */
                    ( void ) uxListRemove( &( pxCRCB->xGenericListItem ) );

                    /* 该协程是否同时还在等待某个事件？ */
                    if( pxCRCB->xEventListItem.pxContainer )
                    {
                        ( void ) uxListRemove( &( pxCRCB->xEventListItem ) );
                    }
                }
                portENABLE_INTERRUPTS();

                prvAddCoRoutineToReadyQueue( pxCRCB );
            }
        }

        xLastTickCount = xCoRoutineTickCount;
    }
/*-----------------------------------------------------------*/

    void vCoRoutineSchedule( void )
    {
        traceENTER_vCoRoutineSchedule();

        /*
         * 只有在 prvInitialiseCoRoutineLists() 被调用之后才能运行协程。
         * 创建协程时，prvInitialiseCoRoutineLists() 会被自动调用。
         */
        if( pxDelayedCoRoutineList != NULL )
        {
            /* 检查是否有因事件而就绪的协程需要移入就绪链表。 */
            prvCheckPendingReadyList();

            /* 检查是否有延时协程已超时。 */
            prvCheckDelayedList();

            /* 查找包含就绪协程的最高优先级队列。 */
            while( listLIST_IS_EMPTY( &( pxReadyCoRoutineLists[ uxTopCoRoutineReadyPriority ] ) ) )
            {
                if( uxTopCoRoutineReadyPriority == 0 )
                {
                    /* 没有更多需要检查的协程。 */
                    return;
                }

                --uxTopCoRoutineReadyPriority;
            }

            /*
             * listGET_OWNER_OF_NEXT_ENTRY 会遍历链表，因此同一优先级的
             * 协程可均等分享处理器时间。
             */
            listGET_OWNER_OF_NEXT_ENTRY( pxCurrentCoRoutine, &( pxReadyCoRoutineLists[ uxTopCoRoutineReadyPriority ] ) );

            /* 调用协程。 */
            ( pxCurrentCoRoutine->pxCoRoutineFunction )( pxCurrentCoRoutine, pxCurrentCoRoutine->uxIndex );
        }

        traceRETURN_vCoRoutineSchedule();
    }
/*-----------------------------------------------------------*/

    STATIC void prvInitialiseCoRoutineLists( void )
    {
        UBaseType_t uxPriority;

        for( uxPriority = 0; uxPriority < configMAX_CO_ROUTINE_PRIORITIES; uxPriority++ )
        {
            vListInitialise( ( List_t * ) &( pxReadyCoRoutineLists[ uxPriority ] ) );
        }

        vListInitialise( ( List_t * ) &xDelayedCoRoutineList1 );
        vListInitialise( ( List_t * ) &xDelayedCoRoutineList2 );
        vListInitialise( ( List_t * ) &xPendingReadyCoRoutineList );

        /*
         * 初始时，pxDelayedCoRoutineList 指向链表 list1，
         * pxOverflowDelayedCoRoutineList 指向链表 list2。
         */
        pxDelayedCoRoutineList = &xDelayedCoRoutineList1;
        pxOverflowDelayedCoRoutineList = &xDelayedCoRoutineList2;
    }
/*-----------------------------------------------------------*/

    BaseType_t xCoRoutineRemoveFromEventList( const List_t * pxEventList )
    {
        CRCB_t * pxUnblockedCRCB;
        BaseType_t xReturn;

        traceENTER_xCoRoutineRemoveFromEventList( pxEventList );

        /*
         * 本函数在中断上下文中被调用，只能访问事件链表和待处理就绪链表。
         * 本函数假定调用者已确认 pxEventList 非空。
         */
        pxUnblockedCRCB = ( CRCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
        ( void ) uxListRemove( &( pxUnblockedCRCB->xEventListItem ) );
        vListInsertEnd( ( List_t * ) &( xPendingReadyCoRoutineList ), &( pxUnblockedCRCB->xEventListItem ) );

        if( pxUnblockedCRCB->uxPriority >= pxCurrentCoRoutine->uxPriority )
        {
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }

        traceRETURN_xCoRoutineRemoveFromEventList( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。这些状态通常在启动时初始化。
 * 应用程序必须在重启调度器之前调用本函数。
 */
    void vCoRoutineResetState( void )
    {
        /* 就绪与阻塞协程使用的链表。 */
        pxDelayedCoRoutineList = NULL;
        pxOverflowDelayedCoRoutineList = NULL;

        /* 其他文件私有变量。 */
        pxCurrentCoRoutine = NULL;
        uxTopCoRoutineReadyPriority = ( UBaseType_t ) 0U;
        xCoRoutineTickCount = ( TickType_t ) 0U;
        xLastTickCount = ( TickType_t ) 0U;
        xPassedTicks = ( TickType_t ) 0U;
    }
/*-----------------------------------------------------------*/

#endif /* configUSE_CO_ROUTINES != 0（已启用协程） */
