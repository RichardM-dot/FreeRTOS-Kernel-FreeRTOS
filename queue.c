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
 * 【中文导读】queue.c —— 队列（任务间通信的根基）
 * ----------------------------------------------------------------------------
 * FreeRTOS 的信号量/互斥量/计数信号量全部是"特殊用法"的队列实现的
 * （semphr.h 只是宏），所以读懂 queue.c = 读懂全部 IPC。
 * 核心结构 Queue_t: 环形存储区 + 两个事件链表（等发送的/等接收的任务）。
 *
 * 阅读路线：
 *   1. xQueueGenericCreate —— 队列内存布局（头部+存储区一次分配）
 *   2. xQueueGenericSend   —— 入队：拷贝数据、唤醒等接收任务、满则阻塞/覆盖
 *   3. xQueueReceive       —— 出队：对称逻辑
 *   4. xQueueGenericSendFromISR / xQueueReceiveFromISR —— 中断版（不阻塞）
 *   5. prvCopyDataToQueue  —— 队列/互斥量行为分叉的地方（mutex 有优先级继承）
 * 关键点：全流程用 critical section / 调度锁保护，读时对照"谁在临界区里"。
 * ============================================================================
 */

#include <stdlib.h>
#include <string.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 把全部 API 函数重定义为使用 MPU 包装函数。该宏只应在应用程序文件包含 task.h 时定义。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#if ( configUSE_CO_ROUTINES == 1 )
    #include "croutine.h"
#endif

/*
 * MPU 移植层要求上述头文件中定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE，但本文件不能保留该定义，以便生成正确的特权态与非特权态链接和放置。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE


/* cRxLock 和 cTxLock 结构体成员使用的常量。 */
#define queueUNLOCKED             ( ( int8_t ) -1 )
#define queueLOCKED_UNMODIFIED    ( ( int8_t ) 0 )
#define queueINT8_MAX             ( ( int8_t ) 127 )

/*
 * 当 Queue_t 结构体用于表示基础队列时，其 pcHead 和 pcTail 成员用作指向队列存储区的指针。当 Queue_t 结构体用于表示互斥量时，不需要 pcHead 和 pcTail 指针，此时 pcHead 被置为 NULL，表示该结构体改为保存互斥量持有者的指针（如果有）。为保持代码可读性，为 pcHead 及相应结构体成员映射了替代名称。QueuePointers_t 与 SemaphoreData_t 类型组成联合体，因为二者的用途互斥，取决于队列的具体用途。
 */
#define uxQueueType               pcHead
#define queueQUEUE_IS_MUTEX       NULL

typedef struct QueuePointers
{
    int8_t * pcTail;     /**< 指向队列存储区末尾的字节。分配时比存储队列项所需多出一个字节，该指针用作边界标记。 */
    int8_t * pcReadFrom; /**< 当结构体用作队列时，指向最近一次读取队列项的位置。 */
} QueuePointers_t;

typedef struct SemaphoreData
{
    TaskHandle_t xMutexHolder;        /**< 持有互斥量的任务句柄。 */
    UBaseType_t uxRecursiveCallCount; /**< 当结构体用作互斥量时，记录递归互斥量被递归获取（take）的次数。 */
} SemaphoreData_t;

/*
 * 信号量并不实际存储或复制数据，因此其消息项大小为 0。
 */
#define queueSEMAPHORE_QUEUE_ITEM_LENGTH    ( ( UBaseType_t ) 0 )
#define queueMUTEX_GIVE_BLOCK_TIME          ( ( TickType_t ) 0U )

#if ( configUSE_PREEMPTION == 0 )

/*
 * 如果使用协作式调度器，则不应仅因为唤醒了更高优先级的任务就执行让出 CPU 操作。
 */
    #define queueYIELD_IF_USING_PREEMPTION()
#else
    #if ( configNUMBER_OF_CORES == 1 )
        #define queueYIELD_IF_USING_PREEMPTION()    portYIELD_WITHIN_API()
    #else /* 对应 #if ( configNUMBER_OF_CORES == 1 ) */
        #define queueYIELD_IF_USING_PREEMPTION()    vTaskYieldWithinAPI()
    #endif /* 对应 #if ( configNUMBER_OF_CORES == 1 ) */
#endif

/*
 * 调度器所用队列的定义。队列项按值复制入队，而不是按引用入队。原因参见以下链接：https://www.FreeRTOS.org/Embedded-RTOS-Queues.html
 */
typedef struct QueueDefinition /* 沿用旧命名约定，以避免破坏内核感知调试器。 */
{
    int8_t * pcHead;           /**< 指向队列存储区起始位置。 */
    int8_t * pcWriteTo;        /**< 指向存储区中下一个空闲位置。 */

    union
    {
        QueuePointers_t xQueue;     /**< 仅当该结构体用作队列时需要的数据。 */
        SemaphoreData_t xSemaphore; /**< 仅当该结构体用作信号量时需要的数据。 */
    } u;

    List_t xTasksWaitingToSend;             /**< 因等待向该队列发送消息而阻塞的任务链表，按优先级顺序存放。 */
    List_t xTasksWaitingToReceive;          /**< 因等待从该队列读取消息而阻塞的任务链表，按优先级顺序存放。 */

    volatile UBaseType_t uxMessagesWaiting; /**< 队列中当前的消息项数量。 */
    UBaseType_t uxLength;                   /**< 队列长度，以可容纳的消息项数量（而非字节数）定义。 */
    UBaseType_t uxItemSize;                 /**< 队列中每个消息项的大小。 */

    volatile int8_t cRxLock;                /**< 记录队列被锁定期间从队列接收（移除）的消息项数量。队列未锁定时置为 queueUNLOCKED。 */
    volatile int8_t cTxLock;                /**< 记录队列被锁定期间向队列发送（添加）的消息项数量。队列未锁定时置为 queueUNLOCKED。 */

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
        uint8_t ucStaticallyAllocated; /**< 若队列所用内存为静态分配，则置为 pdTRUE，以确保不会尝试释放该内存。 */
    #endif

    #if ( configUSE_QUEUE_SETS == 1 )
        struct QueueDefinition * pxQueueSetContainer;
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxQueueNumber;
        uint8_t ucQueueType;
    #endif
} xQUEUE;

/*
 * 保留旧的 xQUEUE 名称，并在下方用 typedef 关联到新名称 Queue_t，以便兼容旧版内核感知调试器。
 */
typedef xQUEUE Queue_t;

/*-----------------------------------------------------------*/

/*
 * 队列注册表只是供内核感知调试器定位队列结构的一种手段，没有其他用途，因此是可选组件。
 */
#if ( configQUEUE_REGISTRY_SIZE > 0 )

/*
 * 存储在队列注册表数组中的类型。它允许为每个队列指定名称，使内核感知调试更加友好。
 */
    typedef struct QUEUE_REGISTRY_ITEM
    {
        const char * pcQueueName;
        QueueHandle_t xHandle;
    } xQueueRegistryItem;

/*
 * 保留旧的 xQueueRegistryItem 名称，并在下方用 typedef 关联到新名称 QueueRegistryItem_t，以便兼容旧版内核感知调试器。
 */
    typedef xQueueRegistryItem QueueRegistryItem_t;

/*
 * 队列注册表只是由 QueueRegistryItem_t 结构体组成的数组。结构体的 pcQueueName 成员为 NULL 表示该数组位置空闲。
 */

/* MISRA 参考 8.4.2 [声明应可见] */
/* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-84 */
/* coverity[misra_c_2012_rule_8_4_violation]（Coverity 抑制标记） */
    PRIVILEGED_DATA QueueRegistryItem_t xQueueRegistry[ configQUEUE_REGISTRY_SIZE ];

#endif /* configQUEUE_REGISTRY_SIZE 配置项 */

/*
 * 解锁由 prvLockQueue 调用锁定的队列。锁定队列不会阻止 ISR 向队列添加或移除消息项，但会阻止 ISR 从队列事件链表中移除任务。若 ISR 发现队列已锁定，则改为递增相应的队列锁定计数，以表示可能有任务需要解除阻塞。队列解锁时会检查这些锁定计数，并采取相应操作。
 */
static void prvUnlockQueue( Queue_t * const pxQueue ) PRIVILEGED_FUNCTION;

/*
 * 使用临界区判断队列中是否有数据。
 *
 * @return 若队列不包含任何消息项则返回 pdTRUE，否则返回 pdFALSE。
 */
static BaseType_t prvIsQueueEmpty( const Queue_t * pxQueue ) PRIVILEGED_FUNCTION;

/*
 * 使用临界区判断队列中是否有空闲空间。
 *
 * @return 若没有空闲空间则返回 pdTRUE，否则返回 pdFALSE；
 */
static BaseType_t prvIsQueueFull( const Queue_t * pxQueue ) PRIVILEGED_FUNCTION;

/*
 * 将消息项复制到队列中，位置可以是队列前端或后端。
 */
static BaseType_t prvCopyDataToQueue( Queue_t * const pxQueue,
                                      const void * pvItemToQueue,
                                      const BaseType_t xPosition ) PRIVILEGED_FUNCTION;

/*
 * 将消息项从队列中复制出来。
 */
static void prvCopyDataFromQueue( Queue_t * const pxQueue,
                                  void * const pvBuffer ) PRIVILEGED_FUNCTION;

#if ( configUSE_QUEUE_SETS == 1 )

/*
 * 检查队列是否为某个队列集的成员，若是，则通知该队列集此队列中已有数据。
 */
    static BaseType_t prvNotifyQueueSetContainer( const Queue_t * const pxQueue ) PRIVILEGED_FUNCTION;
#endif

/*
 * 在静态或动态分配 Queue_t 结构体之后调用，用于填充该结构体的成员。
 */
static void prvInitialiseNewQueue( const UBaseType_t uxQueueLength,
                                   const UBaseType_t uxItemSize,
                                   uint8_t * pucQueueStorage,
                                   const uint8_t ucQueueType,
                                   Queue_t * pxNewQueue ) PRIVILEGED_FUNCTION;

/*
 * 互斥量是一种特殊类型的队列。创建互斥量时，先创建队列，然后调用 prvInitialiseMutex() 将队列配置为互斥量。
 */
#if ( configUSE_MUTEXES == 1 )
    static void prvInitialiseMutex( Queue_t * pxNewQueue ) PRIVILEGED_FUNCTION;
#endif

#if ( configUSE_MUTEXES == 1 )

/*
 * 若等待互斥量的任务使互斥量持有者继承了优先级，但等待任务随后超时，则持有者应取消该优先级继承——但最多只降低到其他同样等待该互斥量的任务中的最高优先级。本函数返回该优先级。
 */
    static UBaseType_t prvGetHighestPriorityOfWaitToReceiveList( const Queue_t * const pxQueue ) PRIVILEGED_FUNCTION;
#endif
/*-----------------------------------------------------------*/

/*
 * 用于将队列标记为已锁定的宏。锁定队列可阻止 ISR 访问队列事件链表。
 */
#define prvLockQueue( pxQueue )                            \
    taskENTER_CRITICAL();                                  \
    {                                                      \
        if( ( pxQueue )->cRxLock == queueUNLOCKED )        \
        {                                                  \
            ( pxQueue )->cRxLock = queueLOCKED_UNMODIFIED; \
        }                                                  \
        if( ( pxQueue )->cTxLock == queueUNLOCKED )        \
        {                                                  \
            ( pxQueue )->cTxLock = queueLOCKED_UNMODIFIED; \
        }                                                  \
    }                                                      \
    taskEXIT_CRITICAL()

/*
 * 用于递增队列数据结构中 cTxLock 成员的宏。该值以系统中的任务数为上限，因为解除阻塞的任务数不可能超过系统中的任务数。
 */
#define prvIncrementQueueTxLock( pxQueue, cTxLock )                           \
    do {                                                                      \
        const UBaseType_t uxNumberOfTasks = uxTaskGetNumberOfTasks();         \
        if( ( UBaseType_t ) ( cTxLock ) < uxNumberOfTasks )                   \
        {                                                                     \
            configASSERT( ( cTxLock ) != queueINT8_MAX );                     \
            ( pxQueue )->cTxLock = ( int8_t ) ( ( cTxLock ) + ( int8_t ) 1 ); \
        }                                                                     \
    } while( 0 )

/*
 * 用于递增队列数据结构中 cRxLock 成员的宏。该值以系统中的任务数为上限，因为解除阻塞的任务数不可能超过系统中的任务数。
 */
#define prvIncrementQueueRxLock( pxQueue, cRxLock )                           \
    do {                                                                      \
        const UBaseType_t uxNumberOfTasks = uxTaskGetNumberOfTasks();         \
        if( ( UBaseType_t ) ( cRxLock ) < uxNumberOfTasks )                   \
        {                                                                     \
            configASSERT( ( cRxLock ) != queueINT8_MAX );                     \
            ( pxQueue )->cRxLock = ( int8_t ) ( ( cRxLock ) + ( int8_t ) 1 ); \
        }                                                                     \
    } while( 0 )
/*-----------------------------------------------------------*/

BaseType_t xQueueGenericReset( QueueHandle_t xQueue,
                               BaseType_t xNewQueue )
{
    BaseType_t xReturn = pdPASS;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueGenericReset( xQueue, xNewQueue );

    configASSERT( pxQueue );

    if( ( pxQueue != NULL ) &&
        ( pxQueue->uxLength >= 1U ) &&
        /* 检查乘法是否溢出。 */
        ( ( SIZE_MAX / pxQueue->uxLength ) >= pxQueue->uxItemSize ) )
    {
        taskENTER_CRITICAL();
        {
            pxQueue->u.xQueue.pcTail = pxQueue->pcHead + ( pxQueue->uxLength * pxQueue->uxItemSize );
            pxQueue->uxMessagesWaiting = ( UBaseType_t ) 0U;
            pxQueue->pcWriteTo = pxQueue->pcHead;
            pxQueue->u.xQueue.pcReadFrom = pxQueue->pcHead + ( ( pxQueue->uxLength - 1U ) * pxQueue->uxItemSize );
            pxQueue->cRxLock = queueUNLOCKED;
            pxQueue->cTxLock = queueUNLOCKED;

            if( xNewQueue == pdFALSE )
            {
                /*
                 * 如果有任务因等待从队列读取而阻塞，这些任务将保持阻塞，因为本函数退出后队列仍为空。如果有任务因等待向队列写入而阻塞，则应解除其中一个任务的阻塞，因为本函数退出后就可以向队列写入。
                 */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /* 确保事件队列以正确的状态初始化。 */
                vListInitialise( &( pxQueue->xTasksWaitingToSend ) );
                vListInitialise( &( pxQueue->xTasksWaitingToReceive ) );
            }
        }
        taskEXIT_CRITICAL();
    }
    else
    {
        xReturn = pdFAIL;
    }

    configASSERT( xReturn != pdFAIL );

    /*
     * 返回值仅为了与之前版本的调用语义保持一致。
     */
    traceRETURN_xQueueGenericReset( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

    QueueHandle_t xQueueGenericCreateStatic( const UBaseType_t uxQueueLength,
                                             const UBaseType_t uxItemSize,
                                             uint8_t * pucQueueStorage,
                                             StaticQueue_t * pxStaticQueue,
                                             const uint8_t ucQueueType )
    {
        Queue_t * pxNewQueue = NULL;

        traceENTER_xQueueGenericCreateStatic( uxQueueLength, uxItemSize, pucQueueStorage, pxStaticQueue, ucQueueType );

        /*
         * 必须提供 StaticQueue_t 结构体和队列存储区。
         */
        configASSERT( pxStaticQueue );

        if( ( uxQueueLength > ( UBaseType_t ) 0 ) &&
            ( pxStaticQueue != NULL ) &&

            /*
             * 当消息项大小不为 0 时应提供队列存储区，当消息项大小为 0 时不应提供。
             */
            ( !( ( pucQueueStorage != NULL ) && ( uxItemSize == 0U ) ) ) &&
            ( !( ( pucQueueStorage == NULL ) && ( uxItemSize != 0U ) ) ) )
        {
            #if ( configASSERT_DEFINED == 1 )
            {
                /*
                 * 检查用于声明 StaticQueue_t 或 StaticSemaphore_t 类型变量的结构体大小是否等于真实队列和信号量结构体的大小。
                 */
                volatile size_t xSize = sizeof( StaticQueue_t );

                /* 该断言在单元测试中无法进行分支覆盖 */
                configASSERT( xSize == sizeof( Queue_t ) ); /* LCOV_EXCL_BR_LINE 分支覆盖排除标记 */
                ( void ) xSize;                             /* 在未定义 configASSERT() 时防止出现未使用变量警告。 */
            }
            #endif /* configASSERT_DEFINED 配置项 */

            /*
             * 传入了静态分配队列的地址，直接使用它。同时传入了静态分配存储区的地址，但该地址已经设置完毕。
             */
            /* MISRA 参考 11.3.1 [非对齐访问] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
            /* coverity[misra_c_2012_rule_11_3_violation]（Coverity 抑制标记） */
            pxNewQueue = ( Queue_t * ) pxStaticQueue;

            #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
            {
                /*
                 * 队列既可以静态分配也可以动态分配，因此记录此队列是静态分配的，以备之后删除该队列时使用。
                 */
                pxNewQueue->ucStaticallyAllocated = pdTRUE;
            }
            #endif /* configSUPPORT_DYNAMIC_ALLOCATION 配置项 */

            prvInitialiseNewQueue( uxQueueLength, uxItemSize, pucQueueStorage, ucQueueType, pxNewQueue );
        }
        else
        {
            configASSERT( pxNewQueue );
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xQueueGenericCreateStatic( pxNewQueue );

        return pxNewQueue;
    }

#endif /* configSUPPORT_STATIC_ALLOCATION 配置项 */
/*-----------------------------------------------------------*/

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

    BaseType_t xQueueGenericGetStaticBuffers( QueueHandle_t xQueue,
                                              uint8_t ** ppucQueueStorage,
                                              StaticQueue_t ** ppxStaticQueue )
    {
        BaseType_t xReturn;
        Queue_t * const pxQueue = xQueue;

        traceENTER_xQueueGenericGetStaticBuffers( xQueue, ppucQueueStorage, ppxStaticQueue );

        configASSERT( pxQueue );
        configASSERT( ppxStaticQueue );

        #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
        {
            /* 检查该队列是否为静态分配。 */
            if( pxQueue->ucStaticallyAllocated == ( uint8_t ) pdTRUE )
            {
                if( ppucQueueStorage != NULL )
                {
                    *ppucQueueStorage = ( uint8_t * ) pxQueue->pcHead;
                }

                /* MISRA 参考 11.3.1 [非对齐访问] */
                /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
                /* coverity[misra_c_2012_rule_11_3_violation]（Coverity 抑制标记） */
                *ppxStaticQueue = ( StaticQueue_t * ) pxQueue;
                xReturn = pdTRUE;
            }
            else
            {
                xReturn = pdFALSE;
            }
        }
        #else /* configSUPPORT_DYNAMIC_ALLOCATION 配置项 */
        {
            /* 该队列必定是静态分配的。 */
            if( ppucQueueStorage != NULL )
            {
                *ppucQueueStorage = ( uint8_t * ) pxQueue->pcHead;
            }

            *ppxStaticQueue = ( StaticQueue_t * ) pxQueue;
            xReturn = pdTRUE;
        }
        #endif /* configSUPPORT_DYNAMIC_ALLOCATION 配置项 */

        traceRETURN_xQueueGenericGetStaticBuffers( xReturn );

        return xReturn;
    }

#endif /* configSUPPORT_STATIC_ALLOCATION 配置项 */
/*-----------------------------------------------------------*/

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

    QueueHandle_t xQueueGenericCreate( const UBaseType_t uxQueueLength,
                                       const UBaseType_t uxItemSize,
                                       const uint8_t ucQueueType )
    {
        Queue_t * pxNewQueue = NULL;
        size_t xQueueSizeInBytes;
        uint8_t * pucQueueStorage;

        traceENTER_xQueueGenericCreate( uxQueueLength, uxItemSize, ucQueueType );

        if( ( uxQueueLength > ( UBaseType_t ) 0 ) &&
            /* 检查乘法是否溢出。 */
            ( ( SIZE_MAX / uxQueueLength ) >= uxItemSize ) &&
            /* 检查加法是否溢出。 */
            /* MISRA 参考 14.3.1 [依赖配置的不变量] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-143。 */
            /* coverity[misra_c_2012_rule_14_3_violation]（Coverity 抑制标记） */
            ( ( SIZE_MAX - sizeof( Queue_t ) ) >= ( size_t ) ( ( size_t ) uxQueueLength * ( size_t ) uxItemSize ) ) )
        {
            /*
             * 分配足够的空间以容纳队列任意时刻可能存在的最大消息项数。当队列用作信号量时，uxItemSize 可以为 0。
             */
            xQueueSizeInBytes = ( size_t ) ( ( size_t ) uxQueueLength * ( size_t ) uxItemSize );

            /* MISRA 参考 11.5.1 [malloc 内存赋值] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation]（Coverity 抑制标记） */
            pxNewQueue = ( Queue_t * ) pvPortMalloc( sizeof( Queue_t ) + xQueueSizeInBytes );

            if( pxNewQueue != NULL )
            {
                /*
                 * 跳过队列结构体，定位到队列存储区的位置。
                 */
                pucQueueStorage = ( uint8_t * ) pxNewQueue;
                pucQueueStorage += sizeof( Queue_t );

                #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
                {
                    /*
                     * 队列既可以静态创建也可以动态创建，因此记录此队列是动态创建的，以备之后删除时使用。
                     */
                    pxNewQueue->ucStaticallyAllocated = pdFALSE;
                }
                #endif /* configSUPPORT_STATIC_ALLOCATION 配置项 */

                prvInitialiseNewQueue( uxQueueLength, uxItemSize, pucQueueStorage, ucQueueType, pxNewQueue );
            }
            else
            {
                traceQUEUE_CREATE_FAILED( ucQueueType );
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            configASSERT( pxNewQueue );
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xQueueGenericCreate( pxNewQueue );

        return pxNewQueue;
    }

#endif /* configSUPPORT_DYNAMIC_ALLOCATION 配置项 */
/*-----------------------------------------------------------*/

static void prvInitialiseNewQueue( const UBaseType_t uxQueueLength,
                                   const UBaseType_t uxItemSize,
                                   uint8_t * pucQueueStorage,
                                   const uint8_t ucQueueType,
                                   Queue_t * pxNewQueue )
{
    /*
     * 在未将 configUSE_TRACE_FACILITY 设为 1 时，消除未使用参数引起的编译器警告。
     */
    ( void ) ucQueueType;

    if( uxItemSize == ( UBaseType_t ) 0 )
    {
        /*
         * 队列存储区未分配 RAM，但 pcHead 不能置为 NULL，因为 NULL 被用作表示队列为互斥量的标志。因此将 pcHead 指向队列自身，作为一个已知位于内存映射范围内的合法值。
         */
        pxNewQueue->pcHead = ( int8_t * ) pxNewQueue;
    }
    else
    {
        /* 将头部指针设置为队列存储区的起始位置。 */
        pxNewQueue->pcHead = ( int8_t * ) pucQueueStorage;
    }

    /*
     * 按队列类型定义处的说明初始化队列成员。
     */
    pxNewQueue->uxLength = uxQueueLength;
    pxNewQueue->uxItemSize = uxItemSize;
    ( void ) xQueueGenericReset( pxNewQueue, pdTRUE );

    #if ( configUSE_TRACE_FACILITY == 1 )
    {
        pxNewQueue->ucQueueType = ucQueueType;
    }
    #endif /* configUSE_TRACE_FACILITY 配置项 */

    #if ( configUSE_QUEUE_SETS == 1 )
    {
        pxNewQueue->pxQueueSetContainer = NULL;
    }
    #endif /* configUSE_QUEUE_SETS 配置项 */

    traceQUEUE_CREATE( pxNewQueue );
}
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

    static void prvInitialiseMutex( Queue_t * pxNewQueue )
    {
        if( pxNewQueue != NULL )
        {
            /*
             * 队列创建函数会为通用队列正确设置队列结构体的所有成员，但本函数创建的是互斥量。这里覆盖那些需要以不同方式设置的成员——尤其是实现优先级继承所需的信息。
             */
            pxNewQueue->u.xSemaphore.xMutexHolder = NULL;
            pxNewQueue->uxQueueType = queueQUEUE_IS_MUTEX;

            /* 以应对递归互斥量的情况。 */
            pxNewQueue->u.xSemaphore.uxRecursiveCallCount = 0;

            traceCREATE_MUTEX( pxNewQueue );

            /* 以信号量的预期状态作为初始状态。 */
            ( void ) xQueueGenericSend( pxNewQueue, NULL, ( TickType_t ) 0U, queueSEND_TO_BACK );
        }
        else
        {
            traceCREATE_MUTEX_FAILED();
        }
    }

#endif /* configUSE_MUTEXES 配置项 */
/*-----------------------------------------------------------*/

#if ( ( configUSE_MUTEXES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

    QueueHandle_t xQueueCreateMutex( const uint8_t ucQueueType )
    {
        QueueHandle_t xNewQueue;
        const UBaseType_t uxMutexLength = ( UBaseType_t ) 1, uxMutexSize = ( UBaseType_t ) 0;

        traceENTER_xQueueCreateMutex( ucQueueType );

        xNewQueue = xQueueGenericCreate( uxMutexLength, uxMutexSize, ucQueueType );
        prvInitialiseMutex( ( Queue_t * ) xNewQueue );

        traceRETURN_xQueueCreateMutex( xNewQueue );

        return xNewQueue;
    }

#endif /* configUSE_MUTEXES 配置项 */
/*-----------------------------------------------------------*/

#if ( ( configUSE_MUTEXES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

    QueueHandle_t xQueueCreateMutexStatic( const uint8_t ucQueueType,
                                           StaticQueue_t * pxStaticQueue )
    {
        QueueHandle_t xNewQueue;
        const UBaseType_t uxMutexLength = ( UBaseType_t ) 1, uxMutexSize = ( UBaseType_t ) 0;

        traceENTER_xQueueCreateMutexStatic( ucQueueType, pxStaticQueue );

        /*
         * 当 configUSE_TRACE_FACILITY 不等于 1 时，防止出现未使用参数的编译器警告。
         */
        ( void ) ucQueueType;

        xNewQueue = xQueueGenericCreateStatic( uxMutexLength, uxMutexSize, NULL, pxStaticQueue, ucQueueType );
        prvInitialiseMutex( ( Queue_t * ) xNewQueue );

        traceRETURN_xQueueCreateMutexStatic( xNewQueue );

        return xNewQueue;
    }

#endif /* configUSE_MUTEXES 配置项 */
/*-----------------------------------------------------------*/

#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )

    TaskHandle_t xQueueGetMutexHolder( QueueHandle_t xSemaphore )
    {
        TaskHandle_t pxReturn;
        Queue_t * const pxSemaphore = ( Queue_t * ) xSemaphore;

        traceENTER_xQueueGetMutexHolder( xSemaphore );

        configASSERT( xSemaphore );

        /*
         * 本函数由 xSemaphoreGetMutexHolder() 调用，不应直接调用。注意：用它可以很好地判断调用任务是否为互斥量持有者，但不适合用来确定互斥量持有者的身份，因为在随后的临界区退出与函数返回之间，持有者可能发生变化。
         */
        taskENTER_CRITICAL();
        {
            if( pxSemaphore->uxQueueType == queueQUEUE_IS_MUTEX )
            {
                pxReturn = pxSemaphore->u.xSemaphore.xMutexHolder;
            }
            else
            {
                pxReturn = NULL;
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_xQueueGetMutexHolder( pxReturn );

        return pxReturn;
    }

#endif /* 对应 if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )

    TaskHandle_t xQueueGetMutexHolderFromISR( QueueHandle_t xSemaphore )
    {
        TaskHandle_t pxReturn;

        traceENTER_xQueueGetMutexHolderFromISR( xSemaphore );

        configASSERT( xSemaphore );

        /*
         * 互斥量不能在中断服务程序中使用，因此互斥量持有者在 ISR 中不会变化，此处不需要临界区。
         */
        if( ( ( Queue_t * ) xSemaphore )->uxQueueType == queueQUEUE_IS_MUTEX )
        {
            pxReturn = ( ( Queue_t * ) xSemaphore )->u.xSemaphore.xMutexHolder;
        }
        else
        {
            pxReturn = NULL;
        }

        traceRETURN_xQueueGetMutexHolderFromISR( pxReturn );

        return pxReturn;
    }

#endif /* 对应 if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( configUSE_RECURSIVE_MUTEXES == 1 )

    BaseType_t xQueueGiveMutexRecursive( QueueHandle_t xMutex )
    {
        BaseType_t xReturn;
        Queue_t * const pxMutex = ( Queue_t * ) xMutex;

        traceENTER_xQueueGiveMutexRecursive( xMutex );

        configASSERT( pxMutex );

        /*
         * 如果当前任务持有该互斥量，则 xMutexHolder 不会在本任务之外发生变化。如果当前任务未持有该互斥量，则 pxMutexHolder 不可能恰好等于本任务的句柄；而这是唯一需要判断的条件，因此即使另一个任务同时访问 pxMutexHolder 也没有关系。所以测试 pxMutexHolder 变量不需要互斥保护。
         */
        if( pxMutex->u.xSemaphore.xMutexHolder == xTaskGetCurrentTaskHandle() )
        {
            traceGIVE_MUTEX_RECURSIVE( pxMutex );

            /*
             * 如果 xMutexHolder 等于任务句柄，则 uxRecursiveCallCount 不可能为 0，因此无需做下溢检查。此外，uxRecursiveCallCount 只由互斥量持有者修改，而持有者只能有一个，因此修改该成员不需要互斥保护。
             */
            ( pxMutex->u.xSemaphore.uxRecursiveCallCount )--;

            /* 递归调用计数是否已递减到 0？ */
            if( pxMutex->u.xSemaphore.uxRecursiveCallCount == ( UBaseType_t ) 0 )
            {
                /*
                 * 归还互斥量。这会自动解除任何其他等待访问该互斥量的任务的阻塞。
                 */
                ( void ) xQueueGenericSend( pxMutex, NULL, queueMUTEX_GIVE_BLOCK_TIME, queueSEND_TO_BACK );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            xReturn = pdPASS;
        }
        else
        {
            /*
             * 调用任务不是持有者，因此不能归还该互斥量。
             */
            xReturn = pdFAIL;

            traceGIVE_MUTEX_RECURSIVE_FAILED( pxMutex );
        }

        traceRETURN_xQueueGiveMutexRecursive( xReturn );

        return xReturn;
    }

#endif /* configUSE_RECURSIVE_MUTEXES 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_RECURSIVE_MUTEXES == 1 )

    BaseType_t xQueueTakeMutexRecursive( QueueHandle_t xMutex,
                                         TickType_t xTicksToWait )
    {
        BaseType_t xReturn;
        Queue_t * const pxMutex = ( Queue_t * ) xMutex;

        traceENTER_xQueueTakeMutexRecursive( xMutex, xTicksToWait );

        configASSERT( pxMutex );

        /*
         * 关于互斥保护的说明，与 xQueueGiveMutexRecursive() 中的注释相同。
         */

        traceTAKE_MUTEX_RECURSIVE( pxMutex );

        if( pxMutex->u.xSemaphore.xMutexHolder == xTaskGetCurrentTaskHandle() )
        {
            ( pxMutex->u.xSemaphore.uxRecursiveCallCount )++;

            /* 检查是否发生溢出。 */
            configASSERT( pxMutex->u.xSemaphore.uxRecursiveCallCount );

            xReturn = pdPASS;
        }
        else
        {
            xReturn = xQueueSemaphoreTake( pxMutex, xTicksToWait );

            /*
             * 只有在成功获得互斥量时才会返回 pdPASS。调用任务在到达此处之前可能已进入阻塞状态。
             */
            if( xReturn != pdFAIL )
            {
                ( pxMutex->u.xSemaphore.uxRecursiveCallCount )++;

                /* 检查是否发生溢出。 */
                configASSERT( pxMutex->u.xSemaphore.uxRecursiveCallCount );
            }
            else
            {
                traceTAKE_MUTEX_RECURSIVE_FAILED( pxMutex );
            }
        }

        traceRETURN_xQueueTakeMutexRecursive( xReturn );

        return xReturn;
    }

#endif /* configUSE_RECURSIVE_MUTEXES 配置项 */
/*-----------------------------------------------------------*/

#if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

    QueueHandle_t xQueueCreateCountingSemaphoreStatic( const UBaseType_t uxMaxCount,
                                                       const UBaseType_t uxInitialCount,
                                                       StaticQueue_t * pxStaticQueue )
    {
        QueueHandle_t xHandle = NULL;

        traceENTER_xQueueCreateCountingSemaphoreStatic( uxMaxCount, uxInitialCount, pxStaticQueue );

        if( ( uxMaxCount != 0U ) &&
            ( uxInitialCount <= uxMaxCount ) )
        {
            xHandle = xQueueGenericCreateStatic( uxMaxCount, queueSEMAPHORE_QUEUE_ITEM_LENGTH, NULL, pxStaticQueue, queueQUEUE_TYPE_COUNTING_SEMAPHORE );

            if( xHandle != NULL )
            {
                ( ( Queue_t * ) xHandle )->uxMessagesWaiting = uxInitialCount;

                traceCREATE_COUNTING_SEMAPHORE();
            }
            else
            {
                traceCREATE_COUNTING_SEMAPHORE_FAILED();
            }
        }
        else
        {
            configASSERT( xHandle );
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xQueueCreateCountingSemaphoreStatic( xHandle );

        return xHandle;
    }

#endif /* 对应 ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

    QueueHandle_t xQueueCreateCountingSemaphore( const UBaseType_t uxMaxCount,
                                                 const UBaseType_t uxInitialCount )
    {
        QueueHandle_t xHandle = NULL;

        traceENTER_xQueueCreateCountingSemaphore( uxMaxCount, uxInitialCount );

        if( ( uxMaxCount != 0U ) &&
            ( uxInitialCount <= uxMaxCount ) )
        {
            xHandle = xQueueGenericCreate( uxMaxCount, queueSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_COUNTING_SEMAPHORE );

            if( xHandle != NULL )
            {
                ( ( Queue_t * ) xHandle )->uxMessagesWaiting = uxInitialCount;

                traceCREATE_COUNTING_SEMAPHORE();
            }
            else
            {
                traceCREATE_COUNTING_SEMAPHORE_FAILED();
            }
        }
        else
        {
            configASSERT( xHandle );
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xQueueCreateCountingSemaphore( xHandle );

        return xHandle;
    }

#endif /* 对应 ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

BaseType_t xQueueGenericSend( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              TickType_t xTicksToWait,
                              const BaseType_t xCopyPosition )
{
    BaseType_t xEntryTimeSet = pdFALSE, xYieldRequired;
    TimeOut_t xTimeOut;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, xCopyPosition );

    configASSERT( pxQueue );
    configASSERT( !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ) );
    configASSERT( !( ( xCopyPosition == queueOVERWRITE ) && ( pxQueue->uxLength != 1 ) ) );
    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    {
        configASSERT( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0 ) ) );
    }
    #endif

    for( ; ; )
    {
        taskENTER_CRITICAL();
        {
            /*
             * 此时队列中是否有空闲空间？正在运行的任务必定是希望访问该队列的最高优先级任务。如果要覆盖队列头部的消息项，则队列是否已满无关紧要。
             */
            if( ( pxQueue->uxMessagesWaiting < pxQueue->uxLength ) || ( xCopyPosition == queueOVERWRITE ) )
            {
                traceQUEUE_SEND( pxQueue );

                #if ( configUSE_QUEUE_SETS == 1 )
                {
                    const UBaseType_t uxPreviousMessagesWaiting = pxQueue->uxMessagesWaiting;

                    xYieldRequired = prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );

                    if( pxQueue->pxQueueSetContainer != NULL )
                    {
                        if( ( xCopyPosition == queueOVERWRITE ) && ( uxPreviousMessagesWaiting != ( UBaseType_t ) 0 ) )
                        {
                            /*
                             * 不要通知队列集，因为队列中的现有消息项被覆盖，队列中的消息项数量没有变化。
                             */
                            mtCOVERAGE_TEST_MARKER();
                        }
                        else if( prvNotifyQueueSetContainer( pxQueue ) != pdFALSE )
                        {
                            /*
                             * 该队列是某个队列集的成员，向队列集发送消息导致更高优先级的任务解除阻塞，需要进行上下文切换。
                             */
                            queueYIELD_IF_USING_PREEMPTION();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        /*
                         * 如果有任务在等待该队列中的数据到达，则立即解除其阻塞。
                         */
                        if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                        {
                            if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                            {
                                /*
                                 * 被解除阻塞的任务优先级高于当前任务，因此立即让出 CPU。在临界区内执行此操作没有问题——内核会处理相关事宜。
                                 */
                                queueYIELD_IF_USING_PREEMPTION();
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else if( xYieldRequired != pdFALSE )
                        {
                            /*
                             * 此路径是一种特殊情况，仅当任务持有多个互斥量，且归还互斥量的顺序与获取顺序不同时才会执行。
                             */
                            queueYIELD_IF_USING_PREEMPTION();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                }
                #else /* configUSE_QUEUE_SETS 配置项 */
                {
                    xYieldRequired = prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );

                    /*
                     * 如果有任务在等待该队列中的数据到达，则立即解除其阻塞。
                     */
                    if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                    {
                        if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                        {
                            /*
                             * 被解除阻塞的任务优先级高于当前任务，因此立即让出 CPU。在临界区内执行此操作没有问题——内核会处理相关事宜。
                             */
                            queueYIELD_IF_USING_PREEMPTION();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else if( xYieldRequired != pdFALSE )
                    {
                        /*
                         * 此路径是一种特殊情况，仅当任务持有多个互斥量，且归还互斥量的顺序与获取顺序不同时才会执行。
                         */
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #endif /* configUSE_QUEUE_SETS 配置项 */

                taskEXIT_CRITICAL();

                traceRETURN_xQueueGenericSend( pdPASS );

                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( TickType_t ) 0 )
                {
                    /*
                     * 队列已满且未指定阻塞时间（或阻塞时间已到期），因此立即退出。
                     */
                    taskEXIT_CRITICAL();

                    /*
                     * 在退出函数之前恢复到原来的特权级别。
                     */
                    traceQUEUE_SEND_FAILED( pxQueue );
                    traceRETURN_xQueueGenericSend( errQUEUE_FULL );

                    return errQUEUE_FULL;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    /*
                     * 队列已满且指定了阻塞时间，因此配置超时结构体。
                     */
                    vTaskInternalSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
                else
                {
                    /* 进入时间已经设置。 */
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        taskEXIT_CRITICAL();

        /*
         * 临界区已退出，现在中断和其他任务可以向队列发送数据或从队列接收数据。
         */

        vTaskSuspendAll();
        prvLockQueue( pxQueue );

        /* 更新超时状态，检查是否已经超时。 */
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            if( prvIsQueueFull( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_SEND( pxQueue );
                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToSend ), xTicksToWait );

                /*
                 * 解锁队列意味着队列事件可以影响事件链表。此时发生的中断可能会再次把本任务从事件链表中移除——但由于调度器仍处于挂起状态，任务会进入挂起就绪链表，而不是实际的就绪链表。
                 */
                prvUnlockQueue( pxQueue );

                /*
                 * 恢复调度器会把任务从挂起就绪链表移入就绪链表——因此本任务在让出 CPU 之前就可能已经位于就绪链表中，此时除非挂起就绪链表中还有更高优先级的任务，否则让出 CPU 不会引起上下文切换。
                 */
                if( xTaskResumeAll() == pdFALSE )
                {
                    taskYIELD_WITHIN_API();
                }
            }
            else
            {
                /* 重试。 */
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            /* 超时已到期。 */
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();

            traceQUEUE_SEND_FAILED( pxQueue );
            traceRETURN_xQueueGenericSend( errQUEUE_FULL );

            return errQUEUE_FULL;
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t xQueueGenericSendFromISR( QueueHandle_t xQueue,
                                     const void * const pvItemToQueue,
                                     BaseType_t * const pxHigherPriorityTaskWoken,
                                     const BaseType_t xCopyPosition )
{
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueGenericSendFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken, xCopyPosition );

    configASSERT( ( pxQueue != NULL ) && !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ) );
    configASSERT( ( pxQueue != NULL ) && !( ( xCopyPosition == queueOVERWRITE ) && ( pxQueue->uxLength != 1 ) ) );

    /*
     * 支持中断嵌套的 RTOS 移植层具有最大系统调用（或最大 API 调用）中断优先级的概念。高于最大系统调用优先级的中断始终保持使能，即使 RTOS 内核处于临界区也是如此，但这些中断不能调用任何 FreeRTOS API 函数。如果在 FreeRTOSConfig.h 中定义了 configASSERT()，那么从优先级高于所配置最大系统调用优先级的中断中调用 FreeRTOS API 函数时，portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 将触发断言失败。只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于（逻辑上）或低于最大系统调用中断优先级的中断中调用。FreeRTOS 提供独立的、中断安全的 API，以确保中断入口尽可能快速和简单。更多信息（尽管主要针对 Cortex-M）参见以下链接：https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
     */
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    /*
     * 与 xQueueGenericSend 类似，但队列没有空闲空间时不阻塞。同时不直接唤醒因等待队列读取而阻塞的任务，而是返回一个标志，表示是否需要上下文切换（即本次发送是否唤醒了优先级高于当前任务的任务）。
     */
    /* MISRA 参考 4.7.1 [应检查返回值] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 抑制标记） */
    uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
    {
        if( ( pxQueue->uxMessagesWaiting < pxQueue->uxLength ) || ( xCopyPosition == queueOVERWRITE ) )
        {
            const int8_t cTxLock = pxQueue->cTxLock;
            const UBaseType_t uxPreviousMessagesWaiting = pxQueue->uxMessagesWaiting;

            traceQUEUE_SEND_FROM_ISR( pxQueue );

            /*
             * 信号量使用 xQueueGiveFromISR()，因此 pxQueue 不会是信号量或互斥量。这意味着 prvCopyDataToQueue() 不会导致任务取消优先级继承，所以即使取消继承函数在访问就绪链表前不检查调度器是否挂起，也可以在此处调用 prvCopyDataToQueue()。
             */
            ( void ) prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );

            /*
             * 队列锁定时不会修改事件链表，具体修改将在之后解锁队列时完成。
             */
            if( cTxLock == queueUNLOCKED )
            {
                #if ( configUSE_QUEUE_SETS == 1 )
                {
                    if( pxQueue->pxQueueSetContainer != NULL )
                    {
                        if( ( xCopyPosition == queueOVERWRITE ) && ( uxPreviousMessagesWaiting != ( UBaseType_t ) 0 ) )
                        {
                            /*
                             * 不要通知队列集，因为队列中的现有消息项被覆盖，队列中的消息项数量没有变化。
                             */
                            mtCOVERAGE_TEST_MARKER();
                        }
                        else if( prvNotifyQueueSetContainer( pxQueue ) != pdFALSE )
                        {
                            /*
                             * 该队列是某个队列集的成员，向队列集发送消息导致更高优先级的任务解除阻塞，需要进行上下文切换。
                             */
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                        {
                            if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                            {
                                /*
                                 * 等待的任务优先级更高，因此记录需要进行上下文切换。
                                 */
                                if( pxHigherPriorityTaskWoken != NULL )
                                {
                                    *pxHigherPriorityTaskWoken = pdTRUE;
                                }
                                else
                                {
                                    mtCOVERAGE_TEST_MARKER();
                                }
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                }
                #else /* configUSE_QUEUE_SETS 配置项 */
                {
                    if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                    {
                        if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                        {
                            /*
                             * 等待的任务优先级更高，因此记录需要进行上下文切换。
                             */
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* 此路径中未使用。 */
                    ( void ) uxPreviousMessagesWaiting;
                }
                #endif /* configUSE_QUEUE_SETS 配置项 */
            }
            else
            {
                /*
                 * 递增锁定计数，使解锁队列的任务知道在队列锁定期间有数据被发送。
                 */
                prvIncrementQueueTxLock( pxQueue, cTxLock );
            }

            xReturn = pdPASS;
        }
        else
        {
            traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue );
            xReturn = errQUEUE_FULL;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xQueueGenericSendFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xQueueGiveFromISR( QueueHandle_t xQueue,
                              BaseType_t * const pxHigherPriorityTaskWoken )
{
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueGiveFromISR( xQueue, pxHigherPriorityTaskWoken );

    /*
     * 与 xQueueGenericSendFromISR() 类似，但用于消息项大小为 0 的信号量。不直接唤醒因等待队列读取而阻塞的任务，而是返回一个标志，表示是否需要上下文切换（即本次发送是否唤醒了优先级高于当前任务的任务）。
     */

    /*
     * 如果消息项大小不为 0，应使用 xQueueGenericSendFromISR() 而不是 xQueueGiveFromISR()。
     */
    configASSERT( ( pxQueue != NULL ) && ( pxQueue->uxItemSize == 0 ) );

    /*
     * 通常不会在中断中归还互斥量，尤其是有互斥量持有者时，因为优先级继承只对任务有意义，对中断没有任何意义。
     */
    configASSERT( ( pxQueue != NULL ) && !( ( pxQueue->uxQueueType == queueQUEUE_IS_MUTEX ) && ( pxQueue->u.xSemaphore.xMutexHolder != NULL ) ) );

    /*
     * 支持中断嵌套的 RTOS 移植层具有最大系统调用（或最大 API 调用）中断优先级的概念。高于最大系统调用优先级的中断始终保持使能，即使 RTOS 内核处于临界区也是如此，但这些中断不能调用任何 FreeRTOS API 函数。如果在 FreeRTOSConfig.h 中定义了 configASSERT()，那么从优先级高于所配置最大系统调用优先级的中断中调用 FreeRTOS API 函数时，portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 将触发断言失败。只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于（逻辑上）或低于最大系统调用中断优先级的中断中调用。FreeRTOS 提供独立的、中断安全的 API，以确保中断入口尽可能快速和简单。更多信息（尽管主要针对 Cortex-M）参见以下链接：https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
     */
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    /* MISRA 参考 4.7.1 [应检查返回值] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 抑制标记） */
    uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
    {
        const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

        /*
         * 当队列用于实现信号量时，实际上不会有数据通过队列传递，但检查队列是否仍有空闲空间依然是有效的。
         */
        if( uxMessagesWaiting < pxQueue->uxLength )
        {
            const int8_t cTxLock = pxQueue->cTxLock;

            traceQUEUE_SEND_FROM_ISR( pxQueue );

            /*
             * 任务只有在作为互斥量持有者时才可能具有继承的优先级——而如果存在互斥量持有者，就不能从 ISR 中归还该互斥量。由于这是该函数的中断版本，可以假定不存在互斥量持有者，也无需判断是否需要取消优先级继承。只需增加可用消息（信号量）的计数即可。
             */
            pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxMessagesWaiting + ( UBaseType_t ) 1 );

            /*
             * 队列锁定时不会修改事件链表，具体修改将在之后解锁队列时完成。
             */
            if( cTxLock == queueUNLOCKED )
            {
                #if ( configUSE_QUEUE_SETS == 1 )
                {
                    if( pxQueue->pxQueueSetContainer != NULL )
                    {
                        if( prvNotifyQueueSetContainer( pxQueue ) != pdFALSE )
                        {
                            /*
                             * 该信号量是某个队列集的成员，向队列集发送消息导致更高优先级的任务解除阻塞，需要进行上下文切换。
                             */
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                        {
                            if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                            {
                                /*
                                 * 等待的任务优先级更高，因此记录需要进行上下文切换。
                                 */
                                if( pxHigherPriorityTaskWoken != NULL )
                                {
                                    *pxHigherPriorityTaskWoken = pdTRUE;
                                }
                                else
                                {
                                    mtCOVERAGE_TEST_MARKER();
                                }
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                }
                #else /* configUSE_QUEUE_SETS 配置项 */
                {
                    if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                    {
                        if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                        {
                            /*
                             * 等待的任务优先级更高，因此记录需要进行上下文切换。
                             */
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #endif /* configUSE_QUEUE_SETS 配置项 */
            }
            else
            {
                /*
                 * 递增锁定计数，使解锁队列的任务知道在队列锁定期间有数据被发送。
                 */
                prvIncrementQueueTxLock( pxQueue, cTxLock );
            }

            xReturn = pdPASS;
        }
        else
        {
            traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue );
            xReturn = errQUEUE_FULL;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xQueueGiveFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xQueueReceive( QueueHandle_t xQueue,
                          void * const pvBuffer,
                          TickType_t xTicksToWait )
{
    BaseType_t xEntryTimeSet = pdFALSE;
    TimeOut_t xTimeOut;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueReceive( xQueue, pvBuffer, xTicksToWait );

    /* 检查指针不为 NULL。 */
    configASSERT( ( pxQueue ) );

    /*
     * 接收数据的缓冲区只有在数据大小为 0（即不向缓冲区复制任何数据）时才能为 NULL。
     */
    configASSERT( !( ( ( pvBuffer ) == NULL ) && ( ( pxQueue )->uxItemSize != ( UBaseType_t ) 0U ) ) );

    /* 调度器挂起时不能阻塞。 */
    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    {
        configASSERT( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0 ) ) );
    }
    #endif

    for( ; ; )
    {
        taskENTER_CRITICAL();
        {
            const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

            /*
             * 此时队列中是否有数据？正在运行的任务必定是希望访问该队列的最高优先级任务。
             */
            if( uxMessagesWaiting > ( UBaseType_t ) 0 )
            {
                /* 有数据可用，移除一个消息项。 */
                prvCopyDataFromQueue( pxQueue, pvBuffer );
                traceQUEUE_RECEIVE( pxQueue );
                pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxMessagesWaiting - ( UBaseType_t ) 1 );

                /*
                 * 队列现在有空闲空间，是否有任务在等待向队列发送消息？如果有，则解除优先级最高的等待任务的阻塞。
                 */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                taskEXIT_CRITICAL();

                traceRETURN_xQueueReceive( pdPASS );

                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( TickType_t ) 0 )
                {
                    /*
                     * 队列为空且未指定阻塞时间（或阻塞时间已到期），因此立即退出。
                     */
                    taskEXIT_CRITICAL();

                    traceQUEUE_RECEIVE_FAILED( pxQueue );
                    traceRETURN_xQueueReceive( errQUEUE_EMPTY );

                    return errQUEUE_EMPTY;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    /*
                     * 队列为空且指定了阻塞时间，因此配置超时结构体。
                     */
                    vTaskInternalSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
                else
                {
                    /* 进入时间已经设置。 */
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        taskEXIT_CRITICAL();

        /*
         * 临界区已退出，现在中断和其他任务可以向队列发送数据或从队列接收数据。
         */

        vTaskSuspendAll();
        prvLockQueue( pxQueue );

        /* 更新超时状态，检查是否已经超时。 */
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            /*
             * 超时尚未到期。如果队列仍为空，则将任务放入等待从队列接收消息的任务链表。
             */
            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue );
                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToReceive ), xTicksToWait );
                prvUnlockQueue( pxQueue );

                if( xTaskResumeAll() == pdFALSE )
                {
                    taskYIELD_WITHIN_API();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /*
                 * 队列中又有数据了。返回循环开头，尝试读取数据。
                 */
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            /*
             * 已超时。如果队列中没有数据则退出，否则返回循环开头尝试读取数据。
             */
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();

            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceQUEUE_RECEIVE_FAILED( pxQueue );
                traceRETURN_xQueueReceive( errQUEUE_EMPTY );

                return errQUEUE_EMPTY;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t xQueueSemaphoreTake( QueueHandle_t xQueue,
                                TickType_t xTicksToWait )
{
    BaseType_t xEntryTimeSet = pdFALSE;
    TimeOut_t xTimeOut;
    Queue_t * const pxQueue = xQueue;

    #if ( configUSE_MUTEXES == 1 )
        BaseType_t xInheritanceOccurred = pdFALSE;
    #endif

    traceENTER_xQueueSemaphoreTake( xQueue, xTicksToWait );

    /* 检查队列指针不为 NULL。 */
    configASSERT( ( pxQueue ) );

    /*
     * 检查这确实是信号量，此时消息项大小应为 0。
     */
    configASSERT( pxQueue->uxItemSize == 0 );

    /* 调度器挂起时不能阻塞。 */
    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    {
        configASSERT( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0 ) ) );
    }
    #endif

    for( ; ; )
    {
        taskENTER_CRITICAL();
        {
            /*
             * 信号量是消息项大小为 0 的队列，队列中的消息数量就是信号量的计数值。
             */
            const UBaseType_t uxSemaphoreCount = pxQueue->uxMessagesWaiting;

            /*
             * 此时队列中是否有数据？正在运行的任务必定是希望访问该队列的最高优先级任务。
             */
            if( uxSemaphoreCount > ( UBaseType_t ) 0 )
            {
                traceQUEUE_RECEIVE( pxQueue );

                /*
                 * 信号量是数据大小为 0 的队列，等待的消息数量就是信号量的计数。这里减少该计数。
                 */
                pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxSemaphoreCount - ( UBaseType_t ) 1 );

                #if ( configUSE_MUTEXES == 1 )
                {
                    if( pxQueue->uxQueueType == queueQUEUE_IS_MUTEX )
                    {
                        /*
                         * 记录实现优先级继承所需的信息，以备需要时使用。
                         */
                        pxQueue->u.xSemaphore.xMutexHolder = pvTaskIncrementMutexHeldCount();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #endif /* configUSE_MUTEXES 配置项 */

                /*
                 * 检查是否有其他任务因等待归还该信号量而阻塞，如果有，则解除其中优先级最高的任务的阻塞。
                 */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                taskEXIT_CRITICAL();

                traceRETURN_xQueueSemaphoreTake( pdPASS );

                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( TickType_t ) 0 )
                {
                    /*
                     * 信号量计数为 0 且未指定阻塞时间（或阻塞时间已到期），因此立即退出。
                     */
                    taskEXIT_CRITICAL();

                    traceQUEUE_RECEIVE_FAILED( pxQueue );
                    traceRETURN_xQueueSemaphoreTake( errQUEUE_EMPTY );

                    return errQUEUE_EMPTY;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    /*
                     * 信号量计数为 0 且指定了阻塞时间，因此配置超时结构体，准备进入阻塞状态。
                     */
                    vTaskInternalSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
                else
                {
                    /* 进入时间已经设置。 */
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        taskEXIT_CRITICAL();

        /*
         * 临界区已退出，现在中断和其他任务可以归还或获取该信号量。
         */

        vTaskSuspendAll();
        prvLockQueue( pxQueue );

        /* 更新超时状态，检查是否已经超时。 */
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            /*
             * 已指定阻塞时间且尚未到期。如果信号量计数为 0，则进入阻塞状态，等待信号量可用。由于信号量是用队列实现的，队列为空等价于信号量计数为 0。
             */
            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue );

                #if ( configUSE_MUTEXES == 1 )
                {
                    if( pxQueue->uxQueueType == queueQUEUE_IS_MUTEX )
                    {
                        taskENTER_CRITICAL();
                        {
                            xInheritanceOccurred = xTaskPriorityInherit( pxQueue->u.xSemaphore.xMutexHolder );
                        }
                        taskEXIT_CRITICAL();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #endif /* 对应 if ( configUSE_MUTEXES == 1 ) */

                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToReceive ), xTicksToWait );
                prvUnlockQueue( pxQueue );

                if( xTaskResumeAll() == pdFALSE )
                {
                    taskYIELD_WITHIN_API();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /*
                 * 没有超时且信号量计数不为 0，因此尝试再次获取信号量。
                 */
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            /* 已超时。 */
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();

            /*
             * 如果信号量计数为 0，则因超时已到期而立即退出。否则返回循环开头，尝试获取已知可用的信号量。由于信号量是用队列实现的，队列为空等价于信号量计数为 0。
             */
            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                #if ( configUSE_MUTEXES == 1 )
                {
                    /*
                     * 只有在 pxQueue->uxQueueType == queueQUEUE_IS_MUTEX 时 xInheritanceOccurred 才可能被置位，因此无需再次测试队列类型来确认它确实是互斥量。
                     */
                    if( xInheritanceOccurred != pdFALSE )
                    {
                        taskENTER_CRITICAL();
                        {
                            UBaseType_t uxHighestWaitingPriority;

                            /*
                             * 本任务因等待该互斥量而阻塞，导致另一个任务继承了本任务的优先级。现在本任务已超时，应再次取消该优先级继承，但最多只降低到同样等待该互斥量的下一个最高优先级任务的优先级。
                             */
                            uxHighestWaitingPriority = prvGetHighestPriorityOfWaitToReceiveList( pxQueue );

                            /*
                             * vTaskPriorityDisinheritAfterTimeout 使用 uxHighestWaitingPriority 参数作为 pxReadyTasksLists 的索引，把互斥量持有者按其新优先级加入就绪链表。Coverity 认为这可能导致越界访问，但事实并非如此，因为 prvGetHighestPriorityOfWaitToReceiveList 返回的 uxHighestWaitingPriority 上限为 ( configMAX_PRIORITIES - 1 )。
                             */
                            /* coverity[overrun]（Coverity 抑制标记） */
                            vTaskPriorityDisinheritAfterTimeout( pxQueue->u.xSemaphore.xMutexHolder, uxHighestWaitingPriority );
                        }
                        taskEXIT_CRITICAL();
                    }
                }
                #endif /* configUSE_MUTEXES 配置项 */

                traceQUEUE_RECEIVE_FAILED( pxQueue );
                traceRETURN_xQueueSemaphoreTake( errQUEUE_EMPTY );

                return errQUEUE_EMPTY;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t xQueuePeek( QueueHandle_t xQueue,
                       void * const pvBuffer,
                       TickType_t xTicksToWait )
{
    BaseType_t xEntryTimeSet = pdFALSE;
    TimeOut_t xTimeOut;
    int8_t * pcOriginalReadPosition;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueuePeek( xQueue, pvBuffer, xTicksToWait );

    /*
     * 接收数据的缓冲区只有在数据大小为 0（即不向缓冲区复制任何数据）时才能为 NULL。
     */
    configASSERT( ( pxQueue != NULL ) && !( ( ( pvBuffer ) == NULL ) && ( ( pxQueue )->uxItemSize != ( UBaseType_t ) 0U ) ) );

    /* 调度器挂起时不能阻塞。 */
    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    {
        configASSERT( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0 ) ) );
    }
    #endif

    for( ; ; )
    {
        taskENTER_CRITICAL();
        {
            const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

            /*
             * 此时队列中是否有数据？正在运行的任务必定是希望访问该队列的最高优先级任务。
             */
            if( uxMessagesWaiting > ( UBaseType_t ) 0 )
            {
                /*
                 * 记住读取位置，以便从队列读出数据后可以复位，因为本函数只是窥视（peek）数据，并不移除数据。
                 */
                pcOriginalReadPosition = pxQueue->u.xQueue.pcReadFrom;

                prvCopyDataFromQueue( pxQueue, pvBuffer );
                traceQUEUE_PEEK( pxQueue );

                /* 数据不会被移除，因此复位读取指针。 */
                pxQueue->u.xQueue.pcReadFrom = pcOriginalReadPosition;

                /*
                 * 数据仍留在队列中，因此检查是否还有其他任务在等待该数据。
                 */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                    {
                        /* 等待的任务优先级高于本任务。 */
                        queueYIELD_IF_USING_PREEMPTION();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                taskEXIT_CRITICAL();

                traceRETURN_xQueuePeek( pdPASS );

                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( TickType_t ) 0 )
                {
                    /*
                     * 队列为空且未指定阻塞时间（或阻塞时间已到期），因此立即退出。
                     */
                    taskEXIT_CRITICAL();

                    traceQUEUE_PEEK_FAILED( pxQueue );
                    traceRETURN_xQueuePeek( errQUEUE_EMPTY );

                    return errQUEUE_EMPTY;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    /*
                     * 队列为空且指定了阻塞时间，因此配置超时结构体，准备进入阻塞状态。
                     */
                    vTaskInternalSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
                else
                {
                    /* 进入时间已经设置。 */
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        taskEXIT_CRITICAL();

        /*
         * 临界区已退出，现在中断和其他任务可以向队列发送数据或从队列接收数据。
         */

        vTaskSuspendAll();
        prvLockQueue( pxQueue );

        /* 更新超时状态，检查是否已经超时。 */
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            /*
             * 超时尚未到期，检查此时队列中是否有数据；如果没有，则进入阻塞状态等待数据。
             */
            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_PEEK( pxQueue );
                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToReceive ), xTicksToWait );
                prvUnlockQueue( pxQueue );

                if( xTaskResumeAll() == pdFALSE )
                {
                    taskYIELD_WITHIN_API();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /*
                 * 此时队列中有数据，因此不要进入阻塞状态，而是返回循环开头尝试获取数据。
                 */
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            /*
             * 超时已到期。如果队列中仍无数据则退出，否则返回循环开头再次尝试读取数据。
             */
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();

            if( prvIsQueueEmpty( pxQueue ) != pdFALSE )
            {
                traceQUEUE_PEEK_FAILED( pxQueue );
                traceRETURN_xQueuePeek( errQUEUE_EMPTY );

                return errQUEUE_EMPTY;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t xQueueReceiveFromISR( QueueHandle_t xQueue,
                                 void * const pvBuffer,
                                 BaseType_t * const pxHigherPriorityTaskWoken )
{
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueReceiveFromISR( xQueue, pvBuffer, pxHigherPriorityTaskWoken );

    configASSERT( pxQueue );
    configASSERT( !( ( pvBuffer == NULL ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ) );

    /*
     * 支持中断嵌套的 RTOS 移植层具有最大系统调用（或最大 API 调用）中断优先级的概念。高于最大系统调用优先级的中断始终保持使能，即使 RTOS 内核处于临界区也是如此，但这些中断不能调用任何 FreeRTOS API 函数。如果在 FreeRTOSConfig.h 中定义了 configASSERT()，那么从优先级高于所配置最大系统调用优先级的中断中调用 FreeRTOS API 函数时，portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 将触发断言失败。只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于（逻辑上）或低于最大系统调用中断优先级的中断中调用。FreeRTOS 提供独立的、中断安全的 API，以确保中断入口尽可能快速和简单。更多信息（尽管主要针对 Cortex-M）参见以下链接：https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
     */
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    /* MISRA 参考 4.7.1 [应检查返回值] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 抑制标记） */
    uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
    {
        const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

        /* ISR 中不能阻塞，因此检查是否有数据可用。 */
        if( uxMessagesWaiting > ( UBaseType_t ) 0 )
        {
            const int8_t cRxLock = pxQueue->cRxLock;

            traceQUEUE_RECEIVE_FROM_ISR( pxQueue );

            prvCopyDataFromQueue( pxQueue, pvBuffer );
            pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxMessagesWaiting - ( UBaseType_t ) 1 );

            /*
             * 如果队列已锁定，则不会修改事件链表。此时改为更新锁定计数，以便解锁队列的任务知道在队列锁定期间有 ISR 移除了数据。
             */
            if( cRxLock == queueUNLOCKED )
            {
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        /*
                         * 等待的任务优先级高于当前任务，因此强制进行上下文切换。
                         */
                        if( pxHigherPriorityTaskWoken != NULL )
                        {
                            *pxHigherPriorityTaskWoken = pdTRUE;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /*
                 * 递增锁定计数，使解锁队列的任务知道在队列锁定期间有数据被移除。
                 */
                prvIncrementQueueRxLock( pxQueue, cRxLock );
            }

            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
            traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue );
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xQueueReceiveFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xQueuePeekFromISR( QueueHandle_t xQueue,
                              void * const pvBuffer )
{
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;
    int8_t * pcOriginalReadPosition;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueuePeekFromISR( xQueue, pvBuffer );

    configASSERT( ( pxQueue != NULL ) && !( ( pvBuffer == NULL ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ) );
    configASSERT( ( pxQueue != NULL ) && ( pxQueue->uxItemSize != 0 ) ); /* 不能对信号量执行窥视（peek）操作。 */

    /*
     * 支持中断嵌套的 RTOS 移植层具有最大系统调用（或最大 API 调用）中断优先级的概念。高于最大系统调用优先级的中断始终保持使能，即使 RTOS 内核处于临界区也是如此，但这些中断不能调用任何 FreeRTOS API 函数。如果在 FreeRTOSConfig.h 中定义了 configASSERT()，那么从优先级高于所配置最大系统调用优先级的中断中调用 FreeRTOS API 函数时，portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 将触发断言失败。只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于（逻辑上）或低于最大系统调用中断优先级的中断中调用。FreeRTOS 提供独立的、中断安全的 API，以确保中断入口尽可能快速和简单。更多信息（尽管主要针对 Cortex-M）参见以下链接：https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
     */
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    /* MISRA 参考 4.7.1 [应检查返回值] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 抑制标记） */
    uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
    {
        /* ISR 中不能阻塞，因此检查是否有数据可用。 */
        if( pxQueue->uxMessagesWaiting > ( UBaseType_t ) 0 )
        {
            traceQUEUE_PEEK_FROM_ISR( pxQueue );

            /*
             * 记住读取位置以便复位，因为实际上没有从队列中移除任何数据。
             */
            pcOriginalReadPosition = pxQueue->u.xQueue.pcReadFrom;
            prvCopyDataFromQueue( pxQueue, pvBuffer );
            pxQueue->u.xQueue.pcReadFrom = pcOriginalReadPosition;

            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
            traceQUEUE_PEEK_FROM_ISR_FAILED( pxQueue );
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xQueuePeekFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue )
{
    UBaseType_t uxReturn;

    traceENTER_uxQueueMessagesWaiting( xQueue );

    configASSERT( xQueue );

    portBASE_TYPE_ENTER_CRITICAL();
    {
        uxReturn = ( ( Queue_t * ) xQueue )->uxMessagesWaiting;
    }
    portBASE_TYPE_EXIT_CRITICAL();

    traceRETURN_uxQueueMessagesWaiting( uxReturn );

    return uxReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t uxQueueSpacesAvailable( const QueueHandle_t xQueue )
{
    UBaseType_t uxReturn;
    Queue_t * const pxQueue = xQueue;

    traceENTER_uxQueueSpacesAvailable( xQueue );

    configASSERT( pxQueue );

    portBASE_TYPE_ENTER_CRITICAL();
    {
        uxReturn = ( UBaseType_t ) ( pxQueue->uxLength - pxQueue->uxMessagesWaiting );
    }
    portBASE_TYPE_EXIT_CRITICAL();

    traceRETURN_uxQueueSpacesAvailable( uxReturn );

    return uxReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t uxQueueMessagesWaitingFromISR( const QueueHandle_t xQueue )
{
    UBaseType_t uxReturn;
    Queue_t * const pxQueue = xQueue;

    traceENTER_uxQueueMessagesWaitingFromISR( xQueue );

    configASSERT( pxQueue );
    uxReturn = pxQueue->uxMessagesWaiting;

    traceRETURN_uxQueueMessagesWaitingFromISR( uxReturn );

    return uxReturn;
}
/*-----------------------------------------------------------*/

void vQueueDelete( QueueHandle_t xQueue )
{
    Queue_t * const pxQueue = xQueue;

    traceENTER_vQueueDelete( xQueue );

    configASSERT( pxQueue );
    configASSERT( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) );
    configASSERT( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) );
    traceQUEUE_DELETE( pxQueue );

    #if ( configQUEUE_REGISTRY_SIZE > 0 )
    {
        vQueueUnregisterQueue( pxQueue );
    }
    #endif

    #if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) )
    {
        /*
         * 该队列只可能是动态分配的——将其释放。
         */
        vPortFree( pxQueue );
    }
    #elif ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    {
        /*
         * 该队列可能是静态或动态分配的，因此在尝试释放内存之前先进行检查。
         */
        if( pxQueue->ucStaticallyAllocated == ( uint8_t ) pdFALSE )
        {
            vPortFree( pxQueue );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #else /* 对应 if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) ) */
    {
        /*
         * 该队列必定是静态分配的，因此不会被删除。这里避免未使用参数引起的编译器警告。
         */
        ( void ) pxQueue;
    }
    #endif /* configSUPPORT_DYNAMIC_ALLOCATION 配置项 */

    traceRETURN_vQueueDelete();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    UBaseType_t uxQueueGetQueueNumber( QueueHandle_t xQueue )
    {
        traceENTER_uxQueueGetQueueNumber( xQueue );

        traceRETURN_uxQueueGetQueueNumber( ( ( Queue_t * ) xQueue )->uxQueueNumber );

        return ( ( Queue_t * ) xQueue )->uxQueueNumber;
    }

#endif /* configUSE_TRACE_FACILITY 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    void vQueueSetQueueNumber( QueueHandle_t xQueue,
                               UBaseType_t uxQueueNumber )
    {
        traceENTER_vQueueSetQueueNumber( xQueue, uxQueueNumber );

        ( ( Queue_t * ) xQueue )->uxQueueNumber = uxQueueNumber;

        traceRETURN_vQueueSetQueueNumber();
    }

#endif /* configUSE_TRACE_FACILITY 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    uint8_t ucQueueGetQueueType( QueueHandle_t xQueue )
    {
        traceENTER_ucQueueGetQueueType( xQueue );

        traceRETURN_ucQueueGetQueueType( ( ( Queue_t * ) xQueue )->ucQueueType );

        return ( ( Queue_t * ) xQueue )->ucQueueType;
    }

#endif /* configUSE_TRACE_FACILITY 配置项 */
/*-----------------------------------------------------------*/

UBaseType_t uxQueueGetQueueItemSize( QueueHandle_t xQueue ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    traceENTER_uxQueueGetQueueItemSize( xQueue );

    traceRETURN_uxQueueGetQueueItemSize( ( ( Queue_t * ) xQueue )->uxItemSize );

    return ( ( Queue_t * ) xQueue )->uxItemSize;
}
/*-----------------------------------------------------------*/

UBaseType_t uxQueueGetQueueLength( QueueHandle_t xQueue ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    traceENTER_uxQueueGetQueueLength( xQueue );

    traceRETURN_uxQueueGetQueueLength( ( ( Queue_t * ) xQueue )->uxLength );

    return ( ( Queue_t * ) xQueue )->uxLength;
}
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

    static UBaseType_t prvGetHighestPriorityOfWaitToReceiveList( const Queue_t * const pxQueue )
    {
        UBaseType_t uxHighestPriorityOfWaitingTasks;

        /*
         * 若等待互斥量的任务使互斥量持有者继承了优先级，但等待任务随后超时，则持有者应取消该优先级继承——但最多只降低到其他同样等待该互斥量的任务中的最高优先级。为此，返回正在等待该互斥量的最高优先级任务的优先级。
         */
        if( listCURRENT_LIST_LENGTH( &( pxQueue->xTasksWaitingToReceive ) ) > 0U )
        {
            uxHighestPriorityOfWaitingTasks = ( UBaseType_t ) ( ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) listGET_ITEM_VALUE_OF_HEAD_ENTRY( &( pxQueue->xTasksWaitingToReceive ) ) );
        }
        else
        {
            uxHighestPriorityOfWaitingTasks = tskIDLE_PRIORITY;
        }

        return uxHighestPriorityOfWaitingTasks;
    }

#endif /* configUSE_MUTEXES 配置项 */
/*-----------------------------------------------------------*/

static BaseType_t prvCopyDataToQueue( Queue_t * const pxQueue,
                                      const void * pvItemToQueue,
                                      const BaseType_t xPosition )
{
    BaseType_t xReturn = pdFALSE;
    UBaseType_t uxMessagesWaiting;

    /* 本函数在临界区内调用。 */

    uxMessagesWaiting = pxQueue->uxMessagesWaiting;

    if( pxQueue->uxItemSize == ( UBaseType_t ) 0 )
    {
        #if ( configUSE_MUTEXES == 1 )
        {
            if( pxQueue->uxQueueType == queueQUEUE_IS_MUTEX )
            {
                /* 该互斥量不再被持有。 */
                xReturn = xTaskPriorityDisinherit( pxQueue->u.xSemaphore.xMutexHolder );
                pxQueue->u.xSemaphore.xMutexHolder = NULL;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif /* configUSE_MUTEXES 配置项 */
    }
    else if( xPosition == queueSEND_TO_BACK )
    {
        ( void ) memcpy( ( void * ) pxQueue->pcWriteTo, pvItemToQueue, ( size_t ) pxQueue->uxItemSize );
        pxQueue->pcWriteTo += pxQueue->uxItemSize;

        if( pxQueue->pcWriteTo >= pxQueue->u.xQueue.pcTail )
        {
            pxQueue->pcWriteTo = pxQueue->pcHead;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        ( void ) memcpy( ( void * ) pxQueue->u.xQueue.pcReadFrom, pvItemToQueue, ( size_t ) pxQueue->uxItemSize );
        pxQueue->u.xQueue.pcReadFrom -= pxQueue->uxItemSize;

        if( pxQueue->u.xQueue.pcReadFrom < pxQueue->pcHead )
        {
            pxQueue->u.xQueue.pcReadFrom = ( pxQueue->u.xQueue.pcTail - pxQueue->uxItemSize );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        if( xPosition == queueOVERWRITE )
        {
            if( uxMessagesWaiting > ( UBaseType_t ) 0 )
            {
                /*
                 * 这里不是添加消息项而是覆盖，因此从记录的队列消息项数量中减一，以便下面再次加一时记录的消息项数量仍然正确。
                 */
                --uxMessagesWaiting;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }

    pxQueue->uxMessagesWaiting = ( UBaseType_t ) ( uxMessagesWaiting + ( UBaseType_t ) 1 );

    return xReturn;
}
/*-----------------------------------------------------------*/

static void prvCopyDataFromQueue( Queue_t * const pxQueue,
                                  void * const pvBuffer )
{
    if( pxQueue->uxItemSize != ( UBaseType_t ) 0 )
    {
        pxQueue->u.xQueue.pcReadFrom += pxQueue->uxItemSize;

        if( pxQueue->u.xQueue.pcReadFrom >= pxQueue->u.xQueue.pcTail )
        {
            pxQueue->u.xQueue.pcReadFrom = pxQueue->pcHead;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        ( void ) memcpy( ( void * ) pvBuffer, ( void * ) pxQueue->u.xQueue.pcReadFrom, ( size_t ) pxQueue->uxItemSize );
    }
}
/*-----------------------------------------------------------*/

static void prvUnlockQueue( Queue_t * const pxQueue )
{
    /* 本函数必须在调度器挂起状态下调用。 */

    /*
     * 锁定计数包含队列被锁定期间额外放入或移除的数据项数量。队列锁定时可以添加或移除消息项，但不能更新事件链表。
     */
    taskENTER_CRITICAL();
    {
        int8_t cTxLock = pxQueue->cTxLock;

        /* 检查队列锁定期间是否有数据被添加。 */
        while( cTxLock > queueLOCKED_UNMODIFIED )
        {
            /*
             * 队列锁定期间有数据被发送。是否有任务因等待数据可用而阻塞？
             */
            #if ( configUSE_QUEUE_SETS == 1 )
            {
                if( pxQueue->pxQueueSetContainer != NULL )
                {
                    if( prvNotifyQueueSetContainer( pxQueue ) != pdFALSE )
                    {
                        /*
                         * 该队列是某个队列集的成员，向队列集发送消息导致更高优先级的任务解除阻塞，需要进行上下文切换。
                         */
                        vTaskMissedYield();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    /*
                     * 由于调度器仍处于挂起状态，从事件链表中移除的任务会被加入挂起就绪链表。
                     */
                    if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                    {
                        if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                        {
                            /*
                             * 等待的任务优先级更高，因此记录需要进行上下文切换。
                             */
                            vTaskMissedYield();
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            #else /* configUSE_QUEUE_SETS 配置项 */
            {
                /*
                 * 由于调度器仍处于挂起状态，从事件链表中移除的任务会被加入挂起就绪链表。
                 */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                    {
                        /*
                         * 等待的任务优先级更高，因此记录需要进行上下文切换。
                         */
                        vTaskMissedYield();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    break;
                }
            }
            #endif /* configUSE_QUEUE_SETS 配置项 */

            --cTxLock;
        }

        pxQueue->cTxLock = queueUNLOCKED;
    }
    taskEXIT_CRITICAL();

    /* 对 Rx 锁定计数执行相同操作。 */
    taskENTER_CRITICAL();
    {
        int8_t cRxLock = pxQueue->cRxLock;

        while( cRxLock > queueLOCKED_UNMODIFIED )
        {
            if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
            {
                if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                {
                    vTaskMissedYield();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                --cRxLock;
            }
            else
            {
                break;
            }
        }

        pxQueue->cRxLock = queueUNLOCKED;
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

static BaseType_t prvIsQueueEmpty( const Queue_t * pxQueue )
{
    BaseType_t xReturn;

    taskENTER_CRITICAL();
    {
        if( pxQueue->uxMessagesWaiting == ( UBaseType_t ) 0 )
        {
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xQueueIsQueueEmptyFromISR( const QueueHandle_t xQueue )
{
    BaseType_t xReturn;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueIsQueueEmptyFromISR( xQueue );

    configASSERT( pxQueue );

    if( pxQueue->uxMessagesWaiting == ( UBaseType_t ) 0 )
    {
        xReturn = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }

    traceRETURN_xQueueIsQueueEmptyFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvIsQueueFull( const Queue_t * pxQueue )
{
    BaseType_t xReturn;

    taskENTER_CRITICAL();
    {
        if( pxQueue->uxMessagesWaiting == pxQueue->uxLength )
        {
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xQueueIsQueueFullFromISR( const QueueHandle_t xQueue )
{
    BaseType_t xReturn;
    Queue_t * const pxQueue = xQueue;

    traceENTER_xQueueIsQueueFullFromISR( xQueue );

    configASSERT( pxQueue );

    if( pxQueue->uxMessagesWaiting == pxQueue->uxLength )
    {
        xReturn = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }

    traceRETURN_xQueueIsQueueFullFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

#if ( configUSE_CO_ROUTINES == 1 )

    BaseType_t xQueueCRSend( QueueHandle_t xQueue,
                             const void * pvItemToQueue,
                             TickType_t xTicksToWait )
    {
        BaseType_t xReturn;
        Queue_t * const pxQueue = xQueue;

        traceENTER_xQueueCRSend( xQueue, pvItemToQueue, xTicksToWait );

        /*
         * 如果队列已满，可能需要阻塞。这里的临界区用于防止中断在检查队列是否已满与在队列上阻塞之间从队列中移除数据。
         */
        portDISABLE_INTERRUPTS();
        {
            if( prvIsQueueFull( pxQueue ) != pdFALSE )
            {
                /*
                 * 队列已满——是需要阻塞，还是直接放弃发送并退出？
                 */
                if( xTicksToWait > ( TickType_t ) 0 )
                {
                    /*
                     * 由于本函数由协程调用，不能直接阻塞，而是返回一个表示需要阻塞的值。
                     */
                    vCoRoutineAddToDelayedList( xTicksToWait, &( pxQueue->xTasksWaitingToSend ) );
                    portENABLE_INTERRUPTS();
                    return errQUEUE_BLOCKED;
                }
                else
                {
                    portENABLE_INTERRUPTS();
                    return errQUEUE_FULL;
                }
            }
        }
        portENABLE_INTERRUPTS();

        portDISABLE_INTERRUPTS();
        {
            if( pxQueue->uxMessagesWaiting < pxQueue->uxLength )
            {
                /* 队列中有空闲空间，将数据复制到队列中。 */
                prvCopyDataToQueue( pxQueue, pvItemToQueue, queueSEND_TO_BACK );
                xReturn = pdPASS;

                /* 是否有协程在等待数据可用？ */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                {
                    /*
                     * 在这种情况下，由于处于临界区内，可以直接把协程放入就绪链表。但这里采用了与中断事件相同的挂起就绪链表机制。
                     */
                    if( xCoRoutineRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                    {
                        /*
                         * 等待的协程优先级更高，因此记录可能需要让出 CPU。
                         */
                        xReturn = errQUEUE_YIELD;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                xReturn = errQUEUE_FULL;
            }
        }
        portENABLE_INTERRUPTS();

        traceRETURN_xQueueCRSend( xReturn );

        return xReturn;
    }

#endif /* configUSE_CO_ROUTINES 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_CO_ROUTINES == 1 )

    BaseType_t xQueueCRReceive( QueueHandle_t xQueue,
                                void * pvBuffer,
                                TickType_t xTicksToWait )
    {
        BaseType_t xReturn;
        Queue_t * const pxQueue = xQueue;

        traceENTER_xQueueCRReceive( xQueue, pvBuffer, xTicksToWait );

        /*
         * 如果队列已空，可能需要阻塞。这里的临界区用于防止中断在检查队列是否为空与在队列上阻塞之间向队列添加数据。
         */
        portDISABLE_INTERRUPTS();
        {
            if( pxQueue->uxMessagesWaiting == ( UBaseType_t ) 0 )
            {
                /*
                 * 队列中没有消息，是需要阻塞，还是直接空手退出？
                 */
                if( xTicksToWait > ( TickType_t ) 0 )
                {
                    /*
                     * 由于本函数由协程调用，不能直接阻塞，而是返回一个表示需要阻塞的值。
                     */
                    vCoRoutineAddToDelayedList( xTicksToWait, &( pxQueue->xTasksWaitingToReceive ) );
                    portENABLE_INTERRUPTS();
                    return errQUEUE_BLOCKED;
                }
                else
                {
                    portENABLE_INTERRUPTS();
                    return errQUEUE_FULL;
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        portENABLE_INTERRUPTS();

        portDISABLE_INTERRUPTS();
        {
            if( pxQueue->uxMessagesWaiting > ( UBaseType_t ) 0 )
            {
                /* 队列中有数据可用。 */
                pxQueue->u.xQueue.pcReadFrom += pxQueue->uxItemSize;

                if( pxQueue->u.xQueue.pcReadFrom >= pxQueue->u.xQueue.pcTail )
                {
                    pxQueue->u.xQueue.pcReadFrom = pxQueue->pcHead;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                --( pxQueue->uxMessagesWaiting );
                ( void ) memcpy( ( void * ) pvBuffer, ( void * ) pxQueue->u.xQueue.pcReadFrom, ( unsigned ) pxQueue->uxItemSize );

                xReturn = pdPASS;

                /* 是否有协程在等待空间可用？ */
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    /*
                     * 在这种情况下，由于处于临界区内，可以直接把协程放入就绪链表。但这里采用了与中断事件相同的挂起就绪链表机制。
                     */
                    if( xCoRoutineRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        xReturn = errQUEUE_YIELD;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        portENABLE_INTERRUPTS();

        traceRETURN_xQueueCRReceive( xReturn );

        return xReturn;
    }

#endif /* configUSE_CO_ROUTINES 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_CO_ROUTINES == 1 )

    BaseType_t xQueueCRSendFromISR( QueueHandle_t xQueue,
                                    const void * pvItemToQueue,
                                    BaseType_t xCoRoutinePreviouslyWoken )
    {
        Queue_t * const pxQueue = xQueue;

        traceENTER_xQueueCRSendFromISR( xQueue, pvItemToQueue, xCoRoutinePreviouslyWoken );

        /*
         * ISR 中不能阻塞，因此如果队列没有空闲空间，就直接退出而不做任何操作。
         */
        if( pxQueue->uxMessagesWaiting < pxQueue->uxLength )
        {
            prvCopyDataToQueue( pxQueue, pvItemToQueue, queueSEND_TO_BACK );

            /*
             * 每次 ISR 只希望唤醒一个协程，因此检查是否已经有协程被唤醒。
             */
            if( xCoRoutinePreviouslyWoken == pdFALSE )
            {
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                {
                    if( xCoRoutineRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
                    {
                        return pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xQueueCRSendFromISR( xCoRoutinePreviouslyWoken );

        return xCoRoutinePreviouslyWoken;
    }

#endif /* configUSE_CO_ROUTINES 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_CO_ROUTINES == 1 )

    BaseType_t xQueueCRReceiveFromISR( QueueHandle_t xQueue,
                                       void * pvBuffer,
                                       BaseType_t * pxCoRoutineWoken )
    {
        BaseType_t xReturn;
        Queue_t * const pxQueue = xQueue;

        traceENTER_xQueueCRReceiveFromISR( xQueue, pvBuffer, pxCoRoutineWoken );

        /*
         * ISR 中不能阻塞，因此检查是否有数据可用。如果没有，就直接退出而不做任何操作。
         */
        if( pxQueue->uxMessagesWaiting > ( UBaseType_t ) 0 )
        {
            /* 从队列中复制数据。 */
            pxQueue->u.xQueue.pcReadFrom += pxQueue->uxItemSize;

            if( pxQueue->u.xQueue.pcReadFrom >= pxQueue->u.xQueue.pcTail )
            {
                pxQueue->u.xQueue.pcReadFrom = pxQueue->pcHead;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            --( pxQueue->uxMessagesWaiting );
            ( void ) memcpy( ( void * ) pvBuffer, ( void * ) pxQueue->u.xQueue.pcReadFrom, ( unsigned ) pxQueue->uxItemSize );

            if( ( *pxCoRoutineWoken ) == pdFALSE )
            {
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
                {
                    if( xCoRoutineRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
                    {
                        *pxCoRoutineWoken = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
        }

        traceRETURN_xQueueCRReceiveFromISR( xReturn );

        return xReturn;
    }

#endif /* configUSE_CO_ROUTINES 配置项 */
/*-----------------------------------------------------------*/

#if ( configQUEUE_REGISTRY_SIZE > 0 )

    void vQueueAddToRegistry( QueueHandle_t xQueue,
                              const char * pcQueueName )
    {
        UBaseType_t ux;
        QueueRegistryItem_t * pxEntryToWrite = NULL;

        traceENTER_vQueueAddToRegistry( xQueue, pcQueueName );

        configASSERT( xQueue );

        if( pcQueueName != NULL )
        {
            /*
             * 检查注册表中是否有空闲位置。名称为 NULL 表示该位置空闲。
             */
            for( ux = ( UBaseType_t ) 0U; ux < ( UBaseType_t ) configQUEUE_REGISTRY_SIZE; ux++ )
            {
                /* 如果该队列已在注册表中，则替换现有条目。 */
                if( xQueue == xQueueRegistry[ ux ].xHandle )
                {
                    pxEntryToWrite = &( xQueueRegistry[ ux ] );
                    break;
                }
                /* 否则，存储到下一个空闲位置 */
                else if( ( pxEntryToWrite == NULL ) && ( xQueueRegistry[ ux ].pcQueueName == NULL ) )
                {
                    pxEntryToWrite = &( xQueueRegistry[ ux ] );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }

        if( pxEntryToWrite != NULL )
        {
            /* 存储该队列的信息。 */
            pxEntryToWrite->pcQueueName = pcQueueName;
            pxEntryToWrite->xHandle = xQueue;

            traceQUEUE_REGISTRY_ADD( xQueue, pcQueueName );
        }

        traceRETURN_vQueueAddToRegistry();
    }

#endif /* configQUEUE_REGISTRY_SIZE 配置项 */
/*-----------------------------------------------------------*/

#if ( configQUEUE_REGISTRY_SIZE > 0 )

    const char * pcQueueGetName( QueueHandle_t xQueue )
    {
        UBaseType_t ux;
        const char * pcReturn = NULL;

        traceENTER_pcQueueGetName( xQueue );

        configASSERT( xQueue );

        /*
         * 注意：这里没有任何保护措施来防止在搜索注册表期间其他任务添加或移除条目。
         */

        for( ux = ( UBaseType_t ) 0U; ux < ( UBaseType_t ) configQUEUE_REGISTRY_SIZE; ux++ )
        {
            if( xQueueRegistry[ ux ].xHandle == xQueue )
            {
                pcReturn = xQueueRegistry[ ux ].pcQueueName;
                break;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        traceRETURN_pcQueueGetName( pcReturn );

        return pcReturn;
    }

#endif /* configQUEUE_REGISTRY_SIZE 配置项 */
/*-----------------------------------------------------------*/

#if ( configQUEUE_REGISTRY_SIZE > 0 )

    void vQueueUnregisterQueue( QueueHandle_t xQueue )
    {
        UBaseType_t ux;

        traceENTER_vQueueUnregisterQueue( xQueue );

        configASSERT( xQueue );

        /*
         * 检查正在注销的队列句柄是否确实在注册表中。
         */
        for( ux = ( UBaseType_t ) 0U; ux < ( UBaseType_t ) configQUEUE_REGISTRY_SIZE; ux++ )
        {
            if( xQueueRegistry[ ux ].xHandle == xQueue )
            {
                /* 将名称置为 NULL，表示该位置再次空闲。 */
                xQueueRegistry[ ux ].pcQueueName = NULL;

                /*
                 * 将句柄置为 NULL，以确保同一队列句柄在添加、移除、再次添加后不会在注册表中出现两次。
                 */
                xQueueRegistry[ ux ].xHandle = ( QueueHandle_t ) 0;
                break;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        traceRETURN_vQueueUnregisterQueue();
    }

#endif /* configQUEUE_REGISTRY_SIZE 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_TIMERS == 1 )

    void vQueueWaitForMessageRestricted( QueueHandle_t xQueue,
                                         TickType_t xTicksToWait,
                                         const BaseType_t xWaitIndefinitely )
    {
        Queue_t * const pxQueue = xQueue;

        traceENTER_vQueueWaitForMessageRestricted( xQueue, xTicksToWait, xWaitIndefinitely );

        /*
         * 本函数不应由应用代码调用，因此名称中带有“Restricted”（受限）。它不属于公共 API，专供内核代码使用，并且有特殊的调用要求。它可能导致对某个最多只可能包含一个节点的链表调用 vListInsert()，因此该链表操作很快；但即便如此，也应在调度器锁定（而不是临界区）状态下调用。
         */

        /*
         * 仅当队列中没有消息时才执行操作。本函数不会真正使任务阻塞，只是将其放入阻塞链表。在调度器解锁之前任务不会阻塞——解锁时将执行一次让出 CPU 操作。如果在队列锁定期间有消息项加入队列，而调用任务又在队列上阻塞，那么队列解锁时调用任务会立即解除阻塞。
         */
        prvLockQueue( pxQueue );

        if( pxQueue->uxMessagesWaiting == ( UBaseType_t ) 0U )
        {
            /* 队列中没有任何消息，按指定时长阻塞。 */
            vTaskPlaceOnEventListRestricted( &( pxQueue->xTasksWaitingToReceive ), xTicksToWait, xWaitIndefinitely );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        prvUnlockQueue( pxQueue );

        traceRETURN_vQueueWaitForMessageRestricted();
    }

#endif /* configUSE_TIMERS 配置项 */
/*-----------------------------------------------------------*/

#if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

    QueueSetHandle_t xQueueCreateSet( const UBaseType_t uxEventQueueLength )
    {
        QueueSetHandle_t pxQueue;

        traceENTER_xQueueCreateSet( uxEventQueueLength );

        pxQueue = xQueueGenericCreate( uxEventQueueLength, ( UBaseType_t ) sizeof( Queue_t * ), queueQUEUE_TYPE_SET );

        traceRETURN_xQueueCreateSet( pxQueue );

        return pxQueue;
    }

#endif /* 对应 #if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

    QueueSetHandle_t xQueueCreateSetStatic( const UBaseType_t uxEventQueueLength,
                                            uint8_t * pucQueueStorage,
                                            StaticQueue_t * pxStaticQueue )
    {
        QueueSetHandle_t pxQueue;

        traceENTER_xQueueCreateSetStatic( uxEventQueueLength );

        pxQueue = xQueueGenericCreateStatic( uxEventQueueLength, ( UBaseType_t ) sizeof( Queue_t * ), pucQueueStorage, pxStaticQueue, queueQUEUE_TYPE_SET );

        traceRETURN_xQueueCreateSetStatic( pxQueue );

        return pxQueue;
    }

#endif /* 对应 #if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )

    BaseType_t xQueueAddToSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                               QueueSetHandle_t xQueueSet )
    {
        BaseType_t xReturn;

        traceENTER_xQueueAddToSet( xQueueOrSemaphore, xQueueSet );

        taskENTER_CRITICAL();
        {
            if( ( ( Queue_t * ) xQueueSet )->uxItemSize != ( UBaseType_t ) sizeof( Queue_t * ) )
            {
                /*
                 * 作为队列集传入的对象不是队列集。队列集的消息项大小始终为 sizeof( Queue_t * )。拒绝任何其他对象，以防止类型混淆，避免 prvNotifyQueueSetContainer() 之后从栈上的单个指针复制 uxItemSize 个字节。
                 */
                xReturn = pdFAIL;
            }
            else if( ( ( Queue_t * ) xQueueOrSemaphore )->pxQueueSetContainer != NULL )
            {
                /* 不能将队列/信号量添加到多个队列集。 */
                xReturn = pdFAIL;
            }
            else if( ( ( Queue_t * ) xQueueOrSemaphore )->uxMessagesWaiting != ( UBaseType_t ) 0 )
            {
                /*
                 * 如果队列/信号量中已有消息项，则不能将其添加到队列集。
                 */
                xReturn = pdFAIL;
            }
            else
            {
                ( ( Queue_t * ) xQueueOrSemaphore )->pxQueueSetContainer = xQueueSet;
                xReturn = pdPASS;
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_xQueueAddToSet( xReturn );

        return xReturn;
    }

#endif /* configUSE_QUEUE_SETS 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )

    BaseType_t xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                                    QueueSetHandle_t xQueueSet )
    {
        BaseType_t xReturn;
        Queue_t * const pxQueueOrSemaphore = ( Queue_t * ) xQueueOrSemaphore;

        traceENTER_xQueueRemoveFromSet( xQueueOrSemaphore, xQueueSet );

        if( pxQueueOrSemaphore->pxQueueSetContainer != xQueueSet )
        {
            /* 该队列不是该队列集的成员。 */
            xReturn = pdFAIL;
        }
        else if( pxQueueOrSemaphore->uxMessagesWaiting != ( UBaseType_t ) 0 )
        {
            /*
             * 当队列非空时将其从队列集中移除是危险的，因为队列集仍会保留该队列的挂起事件。
             */
            xReturn = pdFAIL;
        }
        else
        {
            taskENTER_CRITICAL();
            {
                /* 该队列不再包含在队列集中。 */
                pxQueueOrSemaphore->pxQueueSetContainer = NULL;
            }
            taskEXIT_CRITICAL();
            xReturn = pdPASS;
        }

        traceRETURN_xQueueRemoveFromSet( xReturn );

        return xReturn;
    }

#endif /* configUSE_QUEUE_SETS 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )

    QueueSetMemberHandle_t xQueueSelectFromSet( QueueSetHandle_t xQueueSet,
                                                TickType_t const xTicksToWait )
    {
        QueueSetMemberHandle_t xReturn = NULL;

        traceENTER_xQueueSelectFromSet( xQueueSet, xTicksToWait );

        ( void ) xQueueReceive( ( QueueHandle_t ) xQueueSet, &xReturn, xTicksToWait );

        traceRETURN_xQueueSelectFromSet( xReturn );

        return xReturn;
    }

#endif /* configUSE_QUEUE_SETS 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )

    QueueSetMemberHandle_t xQueueSelectFromSetFromISR( QueueSetHandle_t xQueueSet )
    {
        QueueSetMemberHandle_t xReturn = NULL;

        traceENTER_xQueueSelectFromSetFromISR( xQueueSet );

        ( void ) xQueueReceiveFromISR( ( QueueHandle_t ) xQueueSet, &xReturn, NULL );

        traceRETURN_xQueueSelectFromSetFromISR( xReturn );

        return xReturn;
    }

#endif /* configUSE_QUEUE_SETS 配置项 */
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )

    static BaseType_t prvNotifyQueueSetContainer( const Queue_t * const pxQueue )
    {
        Queue_t * pxQueueSetContainer = pxQueue->pxQueueSetContainer;
        BaseType_t xReturn = pdFALSE;

        /* 本函数必须在临界区内调用。 */

        /*
         * 下面这行在单元测试中不可达，因为每次调用 prvNotifyQueueSetContainer 之前都会检查 pxQueueSetContainer != NULL
         */
        configASSERT( pxQueueSetContainer ); /* LCOV_EXCL_BR_LINE 分支覆盖排除标记 */
        configASSERT( pxQueueSetContainer->uxMessagesWaiting < pxQueueSetContainer->uxLength );

        /* 调用者已确保 pxQueue->pxQueueSetContainer 非空。 */
        /* coverity[dereference]（Coverity 抑制标记） */
        if( pxQueueSetContainer->uxMessagesWaiting < pxQueueSetContainer->uxLength )
        {
            const int8_t cTxLock = pxQueueSetContainer->cTxLock;

            traceQUEUE_SET_SEND( pxQueueSetContainer );

            /* 复制的是包含数据的队列句柄。 */
            xReturn = prvCopyDataToQueue( pxQueueSetContainer, &pxQueue, queueSEND_TO_BACK );

            if( cTxLock == queueUNLOCKED )
            {
                if( listLIST_IS_EMPTY( &( pxQueueSetContainer->xTasksWaitingToReceive ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueueSetContainer->xTasksWaitingToReceive ) ) != pdFALSE )
                    {
                        /* 等待的任务优先级更高。 */
                        xReturn = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                prvIncrementQueueTxLock( pxQueueSetContainer, cTxLock );
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        return xReturn;
    }

#endif /* configUSE_QUEUE_SETS 配置项 */
