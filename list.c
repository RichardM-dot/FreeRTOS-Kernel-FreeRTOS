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
 * ============================================================================
 * 【中文导读】list.c —— 内核链表（一切调度的数据结构基础，先读这个！）
 * ----------------------------------------------------------------------------
 * FreeRTOS 的就绪/延时/等待队列全是这文件里的双向环形链表（xList）。
 * 三个核心概念：链表根 List_t、节点 ListItem_t、节点"宿主"pvOwner(=TCB)。
 * 插入按 pxItemValue 排序（延时到期时间），O(1) 摘除——调度快的原因。
 * 只有 5 个函数共 200 行，是理解 tasks.c 的前置必修课。
 * ============================================================================
 */


#include <stdlib.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将全部 API 函数重定义为使用 MPU 包装器的版本。该宏只应在从应用文件包含 task.h 时定义。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "list.h"

/*
 * MPU 移植层要求上述头文件定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE，但本文件不能定义它，这样才能生成正确的特权/非特权链接与放置方式。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*
 * -----------------------------------------------------------
 * 公共链表 API（文档见 list.h）
 * ----------------------------------------------------------
 */

void vListInitialise( List_t * const pxList )
{
    traceENTER_vListInitialise( pxList );

    /*
     * 链表结构中包含一个用于标记链表末尾的链表项。初始化链表时，将链表末尾项作为唯一的链表项插入。
     */
    pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );

    listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( &( pxList->xListEnd ) );

    /*
     * 链表末尾项的值取链表中的最大可能值，以确保它始终位于链表末尾。
     */
    pxList->xListEnd.xItemValue = portMAX_DELAY;

    /*
     * 链表末尾项的前后指针都指向自身，这样就能判断链表是否为空。
     */
    pxList->xListEnd.pxNext = ( ListItem_t * ) &( pxList->xListEnd );
    pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );

    /* 当 xListEnd 是真正的 ListItem_t 时，初始化其剩余字段。 */
    #if ( configUSE_MINI_LIST_ITEM == 0 )
    {
        pxList->xListEnd.pvOwner = NULL;
        pxList->xListEnd.pxContainer = NULL;
        listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( &( pxList->xListEnd ) );
    }
    #endif

    pxList->uxNumberOfItems = ( UBaseType_t ) 0U;

    /*
     * 若 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 设为 1，则向链表写入已知校验值。
     */
    listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList );
    listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList );

    traceRETURN_vListInitialise();
}
/* ----------------------------------------------------------- */

void vListInitialiseItem( ListItem_t * const pxItem )
{
    traceENTER_vListInitialiseItem( pxItem );

    /* 确保该链表项不被记录为已挂在某个链表上。 */
    pxItem->pxContainer = NULL;

    /*
     * 若 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 设为 1，则向链表项写入已知校验值。
     */
    listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
    listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );

    traceRETURN_vListInitialiseItem();
}
/* ----------------------------------------------------------- */

void vListInsertEnd( List_t * const pxList,
                     ListItem_t * const pxNewListItem )
{
    ListItem_t * const pxIndex = pxList->pxIndex;

    traceENTER_vListInsertEnd( pxList, pxNewListItem );

    /*
     * 仅在同时定义了 configASSERT() 时生效；这些检查可捕获链表数据结构被内存越界改写的问题，但无法捕获因配置错误或 FreeRTOS 使用不当导致的数据错误。
     */
    listTEST_LIST_INTEGRITY( pxList );
    listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );

    /*
     * 将新链表项插入 pxList，但不进行排序，而是让新项成为 listGET_OWNER_OF_NEXT_ENTRY() 最后取出的项。
     */
    pxNewListItem->pxNext = pxIndex;
    pxNewListItem->pxPrevious = pxIndex->pxPrevious;

    /* 仅在判定覆盖（decision coverage）测试中使用。 */
    mtCOVERAGE_TEST_DELAY();

    pxIndex->pxPrevious->pxNext = pxNewListItem;
    pxIndex->pxPrevious = pxNewListItem;

    /* 记录该项当前所在的链表。 */
    pxNewListItem->pxContainer = pxList;

    ( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems + 1U );

    traceRETURN_vListInsertEnd();
}
/* ----------------------------------------------------------- */

void vListInsert( List_t * const pxList,
                  ListItem_t * const pxNewListItem )
{
    ListItem_t * pxIterator;
    const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    traceENTER_vListInsert( pxList, pxNewListItem );

    /*
     * 仅在同时定义了 configASSERT() 时生效；这些检查可捕获链表数据结构被内存越界改写的问题，但无法捕获因配置错误或 FreeRTOS 使用不当导致的数据错误。
     */
    listTEST_LIST_INTEGRITY( pxList );
    listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );

    /*
     * 将新链表项按 xItemValue 顺序插入链表。
     *
     * 若链表中已存在相同 item 值的项，则新项应排在其后。这可确保存放在就绪链表中的 TCB（其 xItemValue 值均相同）能够公平分享 CPU。但是，若 xItemValue 与尾部标记项的值相同，下面的遍历循环将无法结束，因此需要先检查该值，必要时对算法稍作调整。
     */
    if( xValueOfInsertion == portMAX_DELAY )
    {
        pxIterator = pxList->xListEnd.pxPrevious;
    }
    else
    {
        /*
         * *** 注意 ***********************************************************
         *  若应用程序在此处崩溃，常见原因如下所列。更多提示请参阅
         *  https://www.freertos.org/Why-FreeRTOS/FAQs ，并确保定义了 configASSERT()！
         *  https://www.FreeRTOS.org/a00110.html#configASSERT
         *
         *   1) 栈溢出 ——
         *      参见 https://www.FreeRTOS.org/Stacks-and-stack-overflow-checking.html
         *   2) 中断优先级设置错误，尤其是在 Cortex-M 器件上：数值较大的优先级值
         *      实际表示较低的中断优先级，这可能与直觉相反。参见
         *      https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html 以及
         *      https://www.FreeRTOS.org/a00110.html 中 configMAX_SYSCALL_INTERRUPT_PRIORITY 的定义
         *   3) 在临界区内或调度器挂起时调用 API 函数，或在中断中调用
         *      不以 "FromISR" 结尾的 API 函数。
         *   4) 在队列或信号量尚未初始化、或调度器尚未启动前就使用它们
         *      （是否在 vTaskStartScheduler() 调用之前就有中断在触发？）。
         *   5) 若 FreeRTOS 移植层支持中断嵌套，请确保 tick 中断的优先级
         *      等于或低于 configMAX_SYSCALL_INTERRUPT_PRIORITY。
         * ********************************************************************
         */

        for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd ); pxIterator->pxNext->xItemValue <= xValueOfInsertion; pxIterator = pxIterator->pxNext )
        {
            /*
             * 此处无需执行任何操作，仅继续遍历到目标插入位置。
             * 若代码卡在这里，请参阅上方的注意事项。
             */
        }
    }

    pxNewListItem->pxNext = pxIterator->pxNext;
    pxNewListItem->pxNext->pxPrevious = pxNewListItem;
    pxNewListItem->pxPrevious = pxIterator;
    pxIterator->pxNext = pxNewListItem;

    /*
     * 记录该项当前所在的链表，以便之后快速移除该项。
     */
    pxNewListItem->pxContainer = pxList;

    ( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems + 1U );

    traceRETURN_vListInsert();
}
/* ----------------------------------------------------------- */


UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
{
    /*
     * 链表项自身知道它属于哪个链表，因此可从链表项反查所属链表。
     */
    List_t * const pxList = pxItemToRemove->pxContainer;

    traceENTER_uxListRemove( pxItemToRemove );

    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
    pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

    /* 仅在判定覆盖测试中使用。 */
    mtCOVERAGE_TEST_DELAY();

    /* 确保索引始终指向一个有效项。 */
    if( pxList->pxIndex == pxItemToRemove )
    {
        pxList->pxIndex = pxItemToRemove->pxPrevious;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    pxItemToRemove->pxContainer = NULL;
    ( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems - 1U );

    traceRETURN_uxListRemove( pxList->uxNumberOfItems );

    return pxList->uxNumberOfItems;
}
/* ----------------------------------------------------------- */
