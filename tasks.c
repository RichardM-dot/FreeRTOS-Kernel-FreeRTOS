/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
 * 【中文导读】tasks.c —— 任务管理与调度器（FreeRTOS 心脏，建议精读）
 * ----------------------------------------------------------------------------
 * 官方英文注释已极完善（每个 API 前有 Usage/param/return 说明），本导读
 * 帮你建立阅读地图。核心概念：
 *   - TCB（任务控制块）: 每任务一个，存栈顶指针/优先级/状态节点/名字
 *   - 任务栈: 任务的"私有内存"，上下文切换就是保存/恢复栈里的寄存器
 *   - 五状态模型: Running/Ready/Blocked/Suspended + Deleted
 *   - 调度规则: 位图找最高优先级就绪任务（configUSE_PORT_OPTIMISED...时
 *     用 CLZ 指令 O(1)）；同优先级时间片轮转（tick 中断触发切换）
 *
 * 阅读路线（按函数名搜索）：
 *   1. xTaskCreate / xTaskCreateStatic —— 任务怎么"出生"（分配栈+TCB）
 *   2. prvInitialiseNewTask            —— TCB 初始化细节
 *   3. vTaskStartScheduler             —— 调度器启动（会开 SysTick）
 *   4. xTaskIncrementTick              —— tick 中断里跑（时基与延时到期）
 *   5. vTaskSwitchContext              —— 选出下一个该跑的任务（PendSV 里调）
 *   6. prvAddTaskToReadyList / prvResetNextTaskUnblockTime —— 就绪链表操作
 *   7. eTaskConfirmSleepModeStatus / vTaskSuspendAll 等 —— 挂起调度器的临界区
 *
 * 重要机制：
 *   - uxSchedulerSuspended: 调度器挂起计数（vTaskSuspendAll/xTaskResumeAll）
 *   - ulTaskSwitchedIn/pxCurrentTCB: "当前正在跑谁"的唯一权威
 *   - 延时表 pxDelayedTaskList + pxOverflowDelayedTaskList 双链表防回绕
 * ============================================================================
 */

/* 标准头文件包含。 */
#include <stdlib.h>
#include <string.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将
 * 所有 API 函数重定义为使用 MPU 包装函数。仅当从应用程序文件
 * 包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* FreeRTOS 头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stack_macros.h"

/*
 * 默认定义仅适用于非 MPU 移植层。原因是不同体系结构的
 * 栈对齐要求各不相同。
 */
#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS != 0 ) )
    #error configKERNEL_PROVIDED_STATIC_MEMORY cannot be set to 1 when using an MPU port. The vApplicationGet*TaskMemory() functions must be provided manually.
#endif

/*
 * MPU 移植层要求上述头文件定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE，
 * 但本文件不定义，以便生成正确的特权与非特权链接和放置。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*
 * 将 configUSE_STATS_FORMATTING_FUNCTIONS 设为 2，可在不在此处包含
 * stdio.h 的情况下包含统计信息格式化函数。
 */
#if ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )

/*
 * 本文件末尾有两个可选函数，可将 uxTaskGetSystemState() 函数生成的
 * 原始数据转换为便于阅读的文本。注意这些格式化函数仅为方便使用而提供，
 * 不属于内核的一部分。
 */
    #include <stdio.h>
#endif /* 条件：configUSE_STATS_FORMATTING_FUNCTIONS == 1 ) */

#if ( configUSE_PREEMPTION == 0 )

/*
 * 若使用协作式调度器，则不应仅因唤醒了更高优先级的任务
 * 就执行让出。
 */
    #define taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB )
    #define taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB )
#else

    #if ( configNUMBER_OF_CORES == 1 )

/*
 * 该宏请求正在运行的任务 pxTCB 让出 CPU。在单核调度器中，
 * 运行中的任务始终运行在核 0 上，可使用 portYIELD_WITHIN_API()
 * 请求运行在核 0 上的任务让出。因此该宏中不使用 pxTCB。
 */
        #define taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB ) \
    do {                                                         \
        ( void ) ( pxTCB );                                      \
        portYIELD_WITHIN_API();                                  \
    } while( 0 )

        #define taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB ) \
    do {                                                        \
        if( pxCurrentTCB->uxPriority < ( pxTCB )->uxPriority )  \
        {                                                       \
            portYIELD_WITHIN_API();                             \
        }                                                       \
        else                                                    \
        {                                                       \
            mtCOVERAGE_TEST_MARKER();                           \
        }                                                       \
    } while( 0 )

    #else /* 条件：if ( configNUMBER_OF_CORES == 1 ) */

/* 让出该任务当前所在的核心。 */
        #define taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB )    prvYieldCore( ( pxTCB )->xTaskRunState )

/* 若某运行中任务的优先级低于此任务，则让该任务让出 CPU。 */
        #define taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB )     prvYieldForTask( pxTCB )

    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

#endif /* 条件：if ( configUSE_PREEMPTION == 0 ) */

/* 可赋给 TCB 的 ucNotifyState 成员的取值。 */
#define taskNOT_WAITING_NOTIFICATION              ( ( uint8_t ) 0 ) /* 必须为 0，因为它是初始化值。 */
#define taskWAITING_NOTIFICATION                  ( ( uint8_t ) 1 )
#define taskNOTIFICATION_RECEIVED                 ( ( uint8_t ) 2 )

/*
 * 创建任务时用于填充任务栈的值。该值仅用于检查任务的
 * 高水位线（high water mark）。
 */
#define tskSTACK_FILL_BYTE                        ( 0xa5U )

/* 用于记录任务栈和 TCB 分配方式的位标志。 */
#define tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB    ( ( uint8_t ) 0 )
#define tskSTATICALLY_ALLOCATED_STACK_ONLY        ( ( uint8_t ) 1 )
#define tskSTATICALLY_ALLOCATED_STACK_AND_TCB     ( ( uint8_t ) 2 )

/*
 * 若设置了以下任一项，则用已知值填充任务栈，以便确定高水位线。
 * 若以下各项均未设置，则不填充栈，从而避免对 memset 的不必要依赖。
 */
#if ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )
    #define tskSET_NEW_STACKS_TO_KNOWN_VALUE    1
#else
    #define tskSET_NEW_STACKS_TO_KNOWN_VALUE    0
#endif

/*
 * vListTask 用于指示任务所处状态的宏。
 */
#define tskRUNNING_CHAR      ( 'X' )
#define tskBLOCKED_CHAR      ( 'B' )
#define tskREADY_CHAR        ( 'R' )
#define tskDELETED_CHAR      ( 'D' )
#define tskSUSPENDED_CHAR    ( 'S' )

/*
 * 某些内核感知调试器要求调试器需要访问的数据具有全局作用域，
 * 而不是文件作用域。
 */
#ifdef portREMOVE_STATIC_QUALIFIER
    #define STATIC
#else
    #define STATIC    static
#endif

/*
 * 分配给空闲任务的名称。可在 FreeRTOSConfig.h 中定义
 * configIDLE_TASK_NAME 来覆盖此名称。
 */
#ifndef configIDLE_TASK_NAME
    #define configIDLE_TASK_NAME    "IDLE"
#endif

/* 为核心 ID 和空字符终止符预留空间。 */
#if ( configNUMBER_OF_CORES > 1 )
    /* 最多 9 个核的多核系统需要 1 个字符表示核心 ID，1 个字符表示空字符终止符。 */
    #if ( configMAX_TASK_NAME_LEN < 2U )
        #error Minimum required task name length is 2. Please increase configMAX_TASK_NAME_LEN.
    #endif
    #define taskRESERVED_TASK_NAME_LENGTH    2U

#else /* 条件：if ( configNUMBER_OF_CORES > 1 ) */
    /* 为空字符终止符预留空间。 */
    #if ( configMAX_TASK_NAME_LEN < 1U )
        #error Minimum required task name length is 1. Please increase configMAX_TASK_NAME_LEN.
    #endif
    #define taskRESERVED_TASK_NAME_LENGTH    1U
#endif /* 条件：if ( ( configNUMBER_OF_CORES > 1 ) */

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )

/*
 * 若 configUSE_PORT_OPTIMISED_TASK_SELECTION 为 0，则以通用方式执行任务选择，
 * 不针对任何特定微控制器体系结构进行优化。
 */

/*
 * uxTopReadyPriority 保存处于就绪态的最高优先级任务的优先级。
 */
    #define taskRECORD_READY_PRIORITY( uxPriority ) \
    do {                                            \
        if( ( uxPriority ) > uxTopReadyPriority )   \
        {                                           \
            uxTopReadyPriority = ( uxPriority );    \
        }                                           \
    } while( 0 ) /* taskRECORD_READY_PRIORITY 宏结束 */

/*-----------------------------------------------------------*/

    #if ( configNUMBER_OF_CORES == 1 )
        #define taskSELECT_HIGHEST_PRIORITY_TASK()                                       \
    do {                                                                                 \
        UBaseType_t uxTopPriority = uxTopReadyPriority;                                  \
                                                                                         \
        /* Find the highest priority queue that contains ready tasks. */                 \
        while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) != pdFALSE ) \
        {                                                                                \
            configASSERT( uxTopPriority );                                               \
            --uxTopPriority;                                                             \
        }                                                                                \
                                                                                         \
        /* listGET_OWNER_OF_NEXT_ENTRY indexes through the list, so the tasks of \
         * the  same priority get an equal share of the processor time. */                    \
        listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) ); \
        uxTopReadyPriority = uxTopPriority;                                                   \
    } while( 0 ) /* taskSELECT_HIGHEST_PRIORITY_TASK 宏结束 */
    #else /* 条件：if ( configNUMBER_OF_CORES == 1 ) */

        #define taskSELECT_HIGHEST_PRIORITY_TASK( xCoreID )    prvSelectHighestPriorityTask( xCoreID )

    #endif /* 条件：if ( configNUMBER_OF_CORES == 1 ) */

/*-----------------------------------------------------------*/

/*
 * 将 taskRESET_READY_PRIORITY() 和 portRESET_READY_PRIORITY() 定义为空，
 * 因为它们仅在使用移植层优化的任务选择方法时才需要。
 */
    #define taskRESET_READY_PRIORITY( uxPriority )
    #define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )

#else /* configUSE_PORT_OPTIMISED_TASK_SELECTION 配置项 */

/*
 * 若 configUSE_PORT_OPTIMISED_TASK_SELECTION 为 1，则根据所用
 * 微控制器体系结构定制任务选择方式。
 */

/* 提供移植层优化版本。调用移植层定义的宏。 */
    #define taskRECORD_READY_PRIORITY( uxPriority )    portRECORD_READY_PRIORITY( ( uxPriority ), uxTopReadyPriority )

/*-----------------------------------------------------------*/

    #define taskSELECT_HIGHEST_PRIORITY_TASK()                                                  \
    do {                                                                                        \
        UBaseType_t uxTopPriority;                                                              \
                                                                                                \
        /* Find the highest priority list that contains ready tasks. */                         \
        portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );                          \
        configASSERT( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ uxTopPriority ] ) ) > 0 ); \
        listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );   \
    } while( 0 )

/*-----------------------------------------------------------*/

/*
 * 提供移植层优化版本；仅当被重置的 TCB 正被就绪链表引用时才调用。
 * 若它由延时链表或挂起链表引用，则它不在就绪链表中。
 */
    #define taskRESET_READY_PRIORITY( uxPriority )                                                     \
    do {                                                                                               \
        if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 ) \
        {                                                                                              \
            portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );                        \
        }                                                                                              \
    } while( 0 )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 配置项 */

/*-----------------------------------------------------------*/

/*
 * 当时钟节拍计数溢出时，pxDelayedTaskList 与
 * pxOverflowDelayedTaskList 会互换。
 */
#define taskSWITCH_DELAYED_LISTS()                                                \
    do {                                                                          \
        List_t * pxTemp;                                                          \
                                                                                  \
        /* The delayed tasks list should be empty when the lists are switched. */ \
        configASSERT( ( listLIST_IS_EMPTY( pxDelayedTaskList ) ) );               \
                                                                                  \
        pxTemp = pxDelayedTaskList;                                               \
        pxDelayedTaskList = pxOverflowDelayedTaskList;                            \
        pxOverflowDelayedTaskList = pxTemp;                                       \
        xNumOfOverflows = ( BaseType_t ) ( xNumOfOverflows + 1 );                 \
        prvResetNextTaskUnblockTime();                                            \
    } while( 0 )

/*-----------------------------------------------------------*/

/*
 * 将 pxTCB 表示的任务放入其对应的就绪链表。
 * 任务被插入到链表末尾。
 */
#define prvAddTaskToReadyList( pxTCB )                                                                     \
    do {                                                                                                   \
        traceMOVED_TASK_TO_READY_STATE( pxTCB );                                                           \
        taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );                                                \
        listINSERT_END( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
        tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB );                                                      \
    } while( 0 )
/*-----------------------------------------------------------*/

/*
 * 多个函数接收可为 NULL 的 TaskHandle_t 参数，NULL 表示应使用当前
 * 正在执行任务的句柄替代该参数。此宏仅检查参数是否为 NULL，
 * 并返回相应 TCB 的指针。
 */
#define prvGetTCBFromHandle( pxHandle )    ( ( ( pxHandle ) == NULL ) ? pxCurrentTCB : ( pxHandle ) )

/*
 * 事件链表项的 item 值通常用于保存其所属任务的优先级（编码时允许按
 * 相反的优先级顺序保存）。但该值偶尔会被借用。当它被用于其他目的时，
 * 重要的是：即使任务优先级发生变化，也不能更新该值。以下位定义
 * 用于告知调度器不要修改该值——此时使用该值的模块负责在释放时
 * 确保其恢复为原值。
 */
#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    #define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( uint16_t ) 0x8000U )
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    #define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( uint32_t ) 0x80000000U )
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
    #define taskEVENT_LIST_ITEM_VALUE_IN_USE    ( ( uint64_t ) 0x8000000000000000U )
#endif

/* 表示任务未在任何核心上主动运行。 */
#define taskTASK_NOT_RUNNING           ( ( BaseType_t ) ( -1 ) )

/* 表示任务正在运行，但已被调度为让出 CPU。 */
#define taskTASK_SCHEDULED_TO_YIELD    ( ( BaseType_t ) ( -2 ) )

/* 若任务正在运行且未被调度为让出 CPU，则返回 pdTRUE。 */
#if ( configNUMBER_OF_CORES == 1 )
    #define taskTASK_IS_RUNNING( pxTCB )                          ( ( ( pxTCB ) == pxCurrentTCB ) ? ( pdTRUE ) : ( pdFALSE ) )
    #define taskTASK_IS_RUNNING_OR_SCHEDULED_TO_YIELD( pxTCB )    ( ( ( pxTCB ) == pxCurrentTCB ) ? ( pdTRUE ) : ( pdFALSE ) )
#else
    #define taskTASK_IS_RUNNING( pxTCB )                          ( ( ( ( pxTCB )->xTaskRunState >= ( BaseType_t ) 0 ) && ( ( pxTCB )->xTaskRunState < ( BaseType_t ) configNUMBER_OF_CORES ) ) ? ( pdTRUE ) : ( pdFALSE ) )
    #define taskTASK_IS_RUNNING_OR_SCHEDULED_TO_YIELD( pxTCB )    ( ( ( pxTCB )->xTaskRunState != taskTASK_NOT_RUNNING ) ? ( pdTRUE ) : ( pdFALSE ) )
#endif

/* 表示该任务为空闲任务。 */
#define taskATTRIBUTE_IS_IDLE    ( UBaseType_t ) ( 1U << 0U )

#if ( ( configNUMBER_OF_CORES > 1 ) && ( portCRITICAL_NESTING_IN_TCB == 1 ) )
    #define portGET_CRITICAL_NESTING_COUNT( xCoreID )          ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting )
    #define portSET_CRITICAL_NESTING_COUNT( xCoreID, x )       ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting = ( x ) )
    #define portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting++ )
    #define portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( pxCurrentTCBs[ ( xCoreID ) ]->uxCriticalNesting-- )
#endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( portCRITICAL_NESTING_IN_TCB == 1 ) ) */

#define taskBITS_PER_BYTE    ( ( size_t ) 8 )

#if ( configNUMBER_OF_CORES > 1 )

/*
 * 让出指定核心。必须从临界区调用，且 xCoreID 必须有效。
 * 单核系统中只有一个核心可让出，因此不需要该宏。
 */
    #define prvYieldCore( xCoreID )                                                          \
    do {                                                                                     \
        if( ( xCoreID ) == ( BaseType_t ) portGET_CORE_ID() )                                \
        {                                                                                    \
            /* Pending a yield for this core since it is in the critical section. */         \
            xYieldPendings[ ( xCoreID ) ] = pdTRUE;                                          \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
            /* Request other core to yield if it is not requested before. */                 \
            if( pxCurrentTCBs[ ( xCoreID ) ]->xTaskRunState != taskTASK_SCHEDULED_TO_YIELD ) \
            {                                                                                \
                portYIELD_CORE( xCoreID );                                                   \
                pxCurrentTCBs[ ( xCoreID ) ]->xTaskRunState = taskTASK_SCHEDULED_TO_YIELD;   \
            }                                                                                \
        }                                                                                    \
    } while( 0 )
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
/*-----------------------------------------------------------*/

/*
 * 任务控制块。为每个任务分配一个任务控制块（TCB），用于保存
 * 任务状态信息，包括指向任务上下文（任务的运行环境，含寄存器值）
 * 的指针。
 */
typedef struct tskTaskControlBlock       /* 沿用旧命名约定，以免破坏内核感知调试器。 */
{
    volatile StackType_t * pxTopOfStack; /**< 指向任务栈中最后压入项的位置。此成员必须是 TCB 结构体的第一个成员。 */

    #if ( portUSING_MPU_WRAPPERS == 1 )
        xMPU_SETTINGS xMPUSettings; /**< MPU 设置由移植层定义。此成员必须是 TCB 结构体的第二个成员。 */
    #endif

    #if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 )
        UBaseType_t uxCoreAffinityMask; /**< 用于将任务绑定到特定核心。UBaseType_t 的位数必须大于或等于 configNUMBER_OF_CORES。 */
    #endif

    ListItem_t xStateListItem;                  /**< 任务的状态由该状态链表项所引用的链表表示（就绪、阻塞、挂起）。 */
    ListItem_t xEventListItem;                  /**< 用于从事件链表中引用任务。 */
    UBaseType_t uxPriority;                     /**< 任务的优先级。0 为最低优先级。 */
    StackType_t * pxStack;                      /**< 指向栈的起始位置。 */
    #if ( configNUMBER_OF_CORES > 1 )
        volatile BaseType_t xTaskRunState;      /**< 若任务正在运行，用于标识其运行所在的核心；否则标识任务状态——未运行或让出中。 */
        UBaseType_t uxTaskAttributes;           /**< 任务属性——目前用于标识空闲任务。 */
    #endif
    char pcTaskName[ configMAX_TASK_NAME_LEN ]; /**< 创建任务时指定的描述性名称。仅用于方便调试。 */

    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
        BaseType_t xPreemptionDisable; /**< 用于阻止该任务被抢占。 */
    #endif

    #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
        StackType_t * pxEndOfStack; /**< 指向栈的最高有效地址。 */
    #endif

    #if ( portCRITICAL_NESTING_IN_TCB == 1 )
        UBaseType_t uxCriticalNesting; /**< 对于不在移植层维护自身计数的移植层，保存临界区嵌套深度。 */
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxTCBNumber;  /**< 保存每创建一个 TCB 就递增的数值。调试器可据此判断任务何时被删除后又重新创建。 */
        UBaseType_t uxTaskNumber; /**< 保存供第三方跟踪代码使用的数值。 */
    #endif

    #if ( configUSE_MUTEXES == 1 )
        UBaseType_t uxBasePriority; /**< 最近一次分配给该任务的优先级——供优先级继承机制使用。 */
        UBaseType_t uxMutexesHeld;
    #endif

    #if ( configUSE_APPLICATION_TASK_TAG == 1 )
        TaskHookFunction_t pxTaskTag;
    #endif

    #if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
        void * pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
    #endif

    #if ( configGENERATE_RUN_TIME_STATS == 1 )
        configRUN_TIME_COUNTER_TYPE ulRunTimeCounter; /**< 保存任务处于运行态所累计的时间。 */
    #endif

    #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        configTLS_BLOCK_TYPE xTLSBlock; /**< 用作该任务线程局部存储（TLS）块的内存块。 */
    #endif

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
        volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
        volatile uint8_t ucNotifyState[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
    #endif

    /*
     * 参见 FreeRTOS.h 中 tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE
     * 定义处的注释。
     */
    #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
        uint8_t ucStaticallyAllocated; /**< 若任务为静态分配则置为 pdTRUE，以确保不会尝试释放该内存。 */
    #endif

    #if ( INCLUDE_xTaskAbortDelay == 1 )
        uint8_t ucDelayAborted;
    #endif

    #if ( configUSE_POSIX_ERRNO == 1 )
        int iTaskErrno;
    #endif
} tskTCB;

/*
 * 上方保留旧的 tskTCB 名称，并在下方 typedef 为新的 TCB_t 名称，
 * 以便兼容旧版内核感知调试器。
 */
typedef tskTCB TCB_t;

#if ( configNUMBER_OF_CORES == 1 )
    /* MISRA 参考 8.4.1 [声明应当可见] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-84 */
    /* coverity[misra_c_2012_rule_8_4_violation] 抑制 Coverity 的 MISRA C-2012 规则 8.4 告警 */
    portDONT_DISCARD PRIVILEGED_DATA TCB_t * volatile pxCurrentTCB = NULL;
#else
    /* MISRA 参考 8.4.1 [声明应当可见] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-84 */
    /* coverity[misra_c_2012_rule_8_4_violation] 抑制 Coverity 的 MISRA C-2012 规则 8.4 告警 */
    portDONT_DISCARD PRIVILEGED_DATA TCB_t * volatile pxCurrentTCBs[ configNUMBER_OF_CORES ];
    #define pxCurrentTCB    xTaskGetCurrentTaskHandle()
#endif

/*
 * 就绪任务与阻塞任务的链表。 --------------------
 * xDelayedTaskList1 和 xDelayedTaskList2 可以移到函数作用域内，
 * 但这样做会破坏某些内核感知调试器以及依赖移除 static 限定符的调试器。
 */
PRIVILEGED_DATA STATIC List_t pxReadyTasksLists[ configMAX_PRIORITIES ]; /**< 按优先级组织的就绪任务。 */
PRIVILEGED_DATA STATIC List_t xDelayedTaskList1;                         /**< 延时任务。 */
PRIVILEGED_DATA STATIC List_t xDelayedTaskList2;                         /**< 延时任务（使用两个链表——一个用于延时已超过当前节拍计数的任务。 */
PRIVILEGED_DATA STATIC List_t * volatile pxDelayedTaskList;              /**< 指向当前使用的延时任务链表。 */
PRIVILEGED_DATA STATIC List_t * volatile pxOverflowDelayedTaskList;      /**< 指向当前用于保存延时超过当前节拍计数任务的延时任务链表。 */
PRIVILEGED_DATA STATIC List_t xPendingReadyList;                         /**< 在调度器挂起期间被置为就绪的任务。调度器恢复时它们会被移入就绪链表。 */

#if ( INCLUDE_vTaskDelete == 1 )

    PRIVILEGED_DATA STATIC List_t xTasksWaitingTermination; /**< 已删除但内存尚未释放的任务。 */
    PRIVILEGED_DATA STATIC volatile UBaseType_t uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;

#endif

#if ( INCLUDE_vTaskSuspend == 1 )

    PRIVILEGED_DATA STATIC List_t xSuspendedTaskList; /**< 当前处于挂起态的任务。 */

#endif

/*
 * 全局 POSIX errno。上下文切换时会修改其值，使其与当前运行任务的
 * errno 一致。
 */
#if ( configUSE_POSIX_ERRNO == 1 )
    int FreeRTOS_errno = 0;
#endif

/* 其他文件私有变量。 -------------------------------- */
PRIVILEGED_DATA STATIC volatile UBaseType_t uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
PRIVILEGED_DATA STATIC volatile TickType_t xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
PRIVILEGED_DATA STATIC volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;
PRIVILEGED_DATA STATIC volatile BaseType_t xSchedulerRunning = pdFALSE;
PRIVILEGED_DATA STATIC volatile TickType_t xPendedTicks = ( TickType_t ) 0U;
PRIVILEGED_DATA STATIC volatile BaseType_t xYieldPendings[ configNUMBER_OF_CORES ] = { pdFALSE };
PRIVILEGED_DATA STATIC volatile BaseType_t xNumOfOverflows = ( BaseType_t ) 0;
PRIVILEGED_DATA STATIC UBaseType_t uxTaskNumber = ( UBaseType_t ) 0U;
PRIVILEGED_DATA STATIC volatile TickType_t xNextTaskUnblockTime = ( TickType_t ) 0U; /* 在调度器启动前初始化为 portMAX_DELAY。 */
PRIVILEGED_DATA STATIC TaskHandle_t xIdleTaskHandles[ configNUMBER_OF_CORES ];       /**< 保存空闲任务的句柄。空闲任务在调度器启动时自动创建。 */

/*
 * 改进对 OpenOCD 的支持。内核通过优先级链表跟踪就绪任务。
 * 为跟踪远程线程状态，OpenOCD 使用 uxTopUsedPriority 确定需要
 * 从远程目标读回的优先级链表数量。
 */
STATIC const volatile UBaseType_t uxTopUsedPriority = configMAX_PRIORITIES - 1U;

/*
 * 调度器挂起期间，上下文切换保持挂起状态。此外，调度器挂起时，
 * 中断不得操作 TCB 的 xStateListItem，也不得操作 xStateListItem 可能
 * 引用的任何链表。若中断需要在调度器挂起期间解除任务阻塞，则应将
 * 任务的事件链表项移入 xPendingReadyList，待调度器恢复后由内核将任务
 * 从挂起就绪链表移入真正的就绪链表。挂起就绪链表本身只能从临界区访问。
 *
 * 对 uxSchedulerSuspended 的更新必须同时受任务锁和 ISR 锁保护，
 * 且不得在 ISR 中执行。读取操作只需其中任一锁保护，且可在 ISR 或任务中执行。
 */
PRIVILEGED_DATA STATIC volatile UBaseType_t uxSchedulerSuspended = ( UBaseType_t ) 0U;

#if ( configGENERATE_RUN_TIME_STATS == 1 )

/*
 * 不要将这些变量移到函数作用域内，否则需要移除 static 限定符的调试器
 * 将无法正常工作。
 */
PRIVILEGED_DATA STATIC configRUN_TIME_COUNTER_TYPE ulTaskSwitchedInTime[ configNUMBER_OF_CORES ] = { 0U };    /**< 保存任务上次切入时定时器/计数器的值。 */
PRIVILEGED_DATA STATIC volatile configRUN_TIME_COUNTER_TYPE ulTotalRunTime[ configNUMBER_OF_CORES ] = { 0U }; /**< 保存由运行时间计数器时钟定义的累计执行时间。 */

#endif

/*-----------------------------------------------------------*/

/* 文件私有函数。 -------------------------------- */

/*
 * 在调度器启动期间创建空闲任务。
 */
STATIC BaseType_t prvCreateIdleTasks( void );

#if ( configNUMBER_OF_CORES > 1 )

/*
 * 检查是否有其他任务在当前任务等待进入临界区期间将其移出了就绪链表，
 * 若是则让出 CPU。
 */
    STATIC void prvCheckForRunStateChange( void );
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

#if ( configNUMBER_OF_CORES > 1 )

/*
 * 让出一个核心（若不允许不同优先级同时运行，则让出多个核心），
 * 以便任务 pxTCB 得以运行。
 */
    STATIC void prvYieldForTask( const TCB_t * pxTCB );
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

#if ( configNUMBER_OF_CORES > 1 )

/*
 * 为指定核心选择可运行的最高优先级任务。
 */
    STATIC void prvSelectHighestPriorityTask( BaseType_t xCoreID );
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

/**
 * 若 xTask 引用的任务当前处于挂起态，则该工具函数返回 pdTRUE；
 * 若该任务处于其他任何状态，则返回 pdFALSE。
 */
#if ( INCLUDE_vTaskSuspend == 1 )

    STATIC BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

#endif /* 条件：INCLUDE_vTaskSuspend */

/*
 * 初始化调度器使用的所有链表的工具函数。创建第一个任务时
 * 自动调用。
 */
STATIC void prvInitialiseTaskLists( void ) PRIVILEGED_FUNCTION;

/*
 * 空闲任务，与所有任务一样实现为永不结束的循环。
 * 创建第一个用户任务时，空闲任务会被自动创建并加入就绪链表。
 *
 * 在 FreeRTOS SMP 中，还会创建 configNUMBER_OF_CORES - 1 个被动空闲任务，
 * 以确保每个核心在没有其他任务可运行时都有一个空闲任务可运行。
 *
 * 使用 portTASK_FUNCTION_PROTO() 宏以支持移植层/编译器特定的语言扩展。
 * 这些函数的等价原型为：
 *
 * void prvIdleTask( void *pvParameters );
 * void prvPassiveIdleTask( void *pvParameters );
 */
STATIC portTASK_FUNCTION_PROTO( prvIdleTask,
                                pvParameters ) PRIVILEGED_FUNCTION;
#if ( configNUMBER_OF_CORES > 1 )
    STATIC portTASK_FUNCTION_PROTO( prvPassiveIdleTask,
                                    pvParameters ) PRIVILEGED_FUNCTION;
#endif

/*
 * 释放调度器为保存 TCB 而分配的所有内存的工具函数，
 * 包括 TCB 指向的栈。
 *
 * 它不会释放任务自身分配的内存（即任务应用代码中调用
 * pvPortMalloc 分配的内存）。
 */
#if ( INCLUDE_vTaskDelete == 1 )

    STATIC void prvDeleteTCB( TCB_t * pxTCB ) PRIVILEGED_FUNCTION;

#endif

/*
 * 仅由空闲任务使用。检查待删除任务链表中是否有任务。
 * 若有，则清理该任务并删除其 TCB。
 */
STATIC void prvCheckTasksWaitingTermination( void ) PRIVILEGED_FUNCTION;

/*
 * 当前正在执行的任务进入阻塞态。将该任务加入当前延时任务链表
 * 或溢出延时任务链表。
 */
STATIC void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,
                                            const BaseType_t xCanBlockIndefinitely ) PRIVILEGED_FUNCTION;

/*
 * 在 pxList 中查找名称为 pcNameToQuery 的任务——若找到则返回其句柄，
 * 否则返回 NULL。
 */
#if ( INCLUDE_xTaskGetHandle == 1 )

    STATIC TCB_t * prvSearchForNameWithinSingleList( List_t * pxList,
                                                     const char pcNameToQuery[] ) PRIVILEGED_FUNCTION;

#endif

/*
 * 创建任务时，任务栈会被填充为已知值。此函数通过确定栈中仍保持
 * 原始预设值的部分大小，来计算任务栈的“高水位线”。
 */
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )

    STATIC configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte ) PRIVILEGED_FUNCTION;

#endif

/*
 * 返回从当前到内核下一次将任务从阻塞态转为运行态之前所经过的时间
 * （以时钟节拍为单位），或到节拍计数溢出之前的时间（取较早者）。
 *
 * 此条件编译应使用“不等于 0”而非“等于 1”的判断。这是为了确保当用户
 * 定义的低功耗模式实现要求将 configUSE_TICKLESS_IDLE 设为非 1 的值时，
 * 仍可调用 portSUPPRESS_TICKS_AND_SLEEP()。
 */
#if ( configUSE_TICKLESS_IDLE != 0 )

    STATIC TickType_t prvGetExpectedIdleTime( void ) PRIVILEGED_FUNCTION;

#endif

/*
 * 将 xNextTaskUnblockTime 设为下一个阻塞态任务
 * 退出阻塞态的时间。
 */
STATIC void prvResetNextTaskUnblockTime( void ) PRIVILEGED_FUNCTION;

#if ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 )

/*
 * 辅助函数，用于在打印人类可读的任务信息表格时
 * 用空格填充任务名称。
 */
    static char * prvWriteNameToBuffer( char * pcBuffer,
                                        const char * pcTaskName ) PRIVILEGED_FUNCTION;

#endif

/*
 * 在静态或动态分配 TCB 结构体之后调用，
 * 用于填充该结构体的成员。
 */
STATIC void prvInitialiseNewTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t * pxNewTCB,
                                  const MemoryRegion_t * const xRegions ) PRIVILEGED_FUNCTION;

/*
 * 在创建并初始化新任务之后调用，
 * 使该任务受调度器管理。
 */
STATIC void prvAddNewTaskToReadyList( TCB_t * pxNewTCB ) PRIVILEGED_FUNCTION;

/*
 * 使用静态缓冲区（TCB 和栈）创建任务。创建成功则返回
 * 任务句柄，否则返回 NULL。
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    STATIC TCB_t * prvCreateStaticTask( TaskFunction_t pxTaskCode,
                                        const char * const pcName,
                                        const configSTACK_DEPTH_TYPE uxStackDepth,
                                        void * const pvParameters,
                                        UBaseType_t uxPriority,
                                        StackType_t * const puxStackBuffer,
                                        StaticTask_t * const pxTaskBuffer,
                                        TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif /* 条件：#if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */

/*
 * 使用静态缓冲区（TCB 和栈）创建受限任务。创建成功则返回
 * 任务句柄，否则返回 NULL。
 */
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    STATIC TCB_t * prvCreateRestrictedStaticTask( const TaskParameters_t * const pxTaskDefinition,
                                                  TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif /* 条件：#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */

/*
 * 使用静态任务栈缓冲区和动态分配的 TCB 缓冲区创建受限任务。
 * 创建成功则返回任务句柄，否则返回 NULL。
 */
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    STATIC TCB_t * prvCreateRestrictedTask( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif /* 条件：#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */

/*
 * 使用动态分配的缓冲区（TCB 和栈）创建任务。创建成功则返回
 * 任务句柄，否则返回 NULL。
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    STATIC TCB_t * prvCreateTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif /* 条件：#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) */

/*
 * 仅当定义了用户可定义的宏 FREERTOS_TASKS_C_ADDITIONS_INIT() 时
 * 才应调用 freertos_tasks_c_additions_init()，因为该函数只调用这个宏。
 */
#ifdef FREERTOS_TASKS_C_ADDITIONS_INIT

    STATIC void freertos_tasks_c_additions_init( void ) PRIVILEGED_FUNCTION;

#endif

#if ( configUSE_PASSIVE_IDLE_HOOK == 1 )
    extern void vApplicationPassiveIdleHook( void );
#endif /* 条件：#if ( configUSE_PASSIVE_IDLE_HOOK == 1 ) */

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

/*
 * 将 snprintf 的返回值转换为实际写入的字符数。可能的情况如下：
 *
 * 1. 提供给 snprintf 的缓冲区足够容纳生成的字符串。此时返回值为实际
 *    写入的字符数，不包括终止空字符。
 * 2. 提供给 snprintf 的缓冲区不足以容纳生成的字符串。此时返回值为
 *    假若缓冲区足够大时本应写入的字符数，不包括终止空字符。
 * 3. 编码错误。此时返回值为负数。
 *
 * 由上述情况 1 和 2 可知 ==> 仅当返回值非负且小于传入的缓冲区长度时，
 * 字符串才已完整写入。
 */
    STATIC size_t prvSnprintfReturnValueToCharsWritten( int iSnprintfReturnValue,
                                                        size_t n );

#endif /* 条件：#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) ) */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )
    STATIC void prvCheckForRunStateChange( void )
    {
        UBaseType_t uxPrevCriticalNesting;
        const TCB_t * pxThisTCB;
        BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();

        /* 只能从任务内部调用。 */
        portASSERT_IF_IN_ISR();

        /*
         * 调用此函数时始终已禁用中断，
         * 因此这样做是安全的。
         */
        pxThisTCB = pxCurrentTCBs[ xCoreID ];

        while( pxThisTCB->xTaskRunState == taskTASK_SCHEDULED_TO_YIELD )
        {
            /*
             * 只有在刚进入临界区或刚挂起调度器，且其他任务请求本任务
             * 让出 CPU 时才会执行到这里。
             *
             * 这稍显复杂，因为需要保存并恢复挂起计数和临界区嵌套计数，
             * 还要释放并重新获取正确的锁。若在重新获取锁期间状态再次变化，
             * 则需重新执行上述过程。
             */
            uxPrevCriticalNesting = portGET_CRITICAL_NESTING_COUNT( xCoreID );

            if( uxPrevCriticalNesting > 0U )
            {
                portSET_CRITICAL_NESTING_COUNT( xCoreID, 0U );
                portRELEASE_ISR_LOCK( xCoreID );
            }
            else
            {
                /*
                 * 调度器已挂起。仅当任务未被请求让出 CPU 时才更新
                 * uxSchedulerSuspended。
                 */
                mtCOVERAGE_TEST_MARKER();
            }

            portRELEASE_TASK_LOCK( xCoreID );
            portMEMORY_BARRIER();
            configASSERT( pxThisTCB->xTaskRunState == taskTASK_SCHEDULED_TO_YIELD );

            portENABLE_INTERRUPTS();

            /*
             * 使能中断应会使该核心立即处理挂起的中断并让出。处理完挂起中断后，
             * 任务需要在此循环中重新评估其运行状态，因为其他核心可能已请求
             * 该任务让出，从而可能改变其运行状态。
             */

            portDISABLE_INTERRUPTS();

            xCoreID = ( BaseType_t ) portGET_CORE_ID();
            portGET_TASK_LOCK( xCoreID );
            portGET_ISR_LOCK( xCoreID );

            portSET_CRITICAL_NESTING_COUNT( xCoreID, uxPrevCriticalNesting );

            if( uxPrevCriticalNesting == 0U )
            {
                portRELEASE_ISR_LOCK( xCoreID );
            }
        }
    }
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )
    STATIC void prvYieldForTask( const TCB_t * pxTCB )
    {
        BaseType_t xLowestPriorityToPreempt;
        BaseType_t xCurrentCoreTaskPriority;
        BaseType_t xLowestPriorityCore = ( BaseType_t ) -1;
        BaseType_t xCoreID;
        const BaseType_t xCurrentCoreID = ( BaseType_t ) portGET_CORE_ID();

        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            BaseType_t xYieldCount = 0;
        #endif /* 条件：#if ( configRUN_MULTIPLE_PRIORITIES == 0 ) */

        /* 必须从临界区调用。 */
        configASSERT( portGET_CRITICAL_NESTING_COUNT( xCurrentCoreID ) > 0U );

        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )

            /*
             * 若该任务的优先级低于当前就绪任务的优先级，
             * 则任何任务都不应为它让出 CPU。
             */
            if( pxTCB->uxPriority >= uxTopReadyPriority )
        #else
            /* 对于已在运行的任务，无需让出。 */
            if( taskTASK_IS_RUNNING( pxTCB ) == pdFALSE )
        #endif
        {
            xLowestPriorityToPreempt = ( BaseType_t ) pxTCB->uxPriority;

            /*
             * 若 pxTCB 的优先级为 0，则 xLowestPriorityToPreempt 会递减为 -1。
             * 这没有问题，因为下面会给系统空闲任务赋予 -1 的优先级。
             */
            --xLowestPriorityToPreempt;

            for( xCoreID = ( BaseType_t ) 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
            {
                xCurrentCoreTaskPriority = ( BaseType_t ) pxCurrentTCBs[ xCoreID ]->uxPriority;

                /* 此处将系统空闲任务的优先级设为 tskIDLE_PRIORITY - 1。 */
                if( ( pxCurrentTCBs[ xCoreID ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                {
                    xCurrentCoreTaskPriority = ( BaseType_t ) ( xCurrentCoreTaskPriority - 1 );
                }

                if( ( taskTASK_IS_RUNNING( pxCurrentTCBs[ xCoreID ] ) != pdFALSE ) && ( xYieldPendings[ xCoreID ] == pdFALSE ) )
                {
                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                        if( taskTASK_IS_RUNNING( pxTCB ) == pdFALSE )
                    #endif
                    {
                        if( xCurrentCoreTaskPriority <= xLowestPriorityToPreempt )
                        {
                            #if ( configUSE_CORE_AFFINITY == 1 )
                                if( ( pxTCB->uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                            #endif
                            {
                                #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                                    if( pxCurrentTCBs[ xCoreID ]->xPreemptionDisable == pdFALSE )
                                #endif
                                {
                                    xLowestPriorityToPreempt = xCurrentCoreTaskPriority;
                                    xLowestPriorityCore = xCoreID;
                                }
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }

                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                    {
                        /*
                         * 让所有优先级低于待运行任务的、当前正在运行的非空闲任务
                         * 让出 CPU。
                         */
                        if( ( xCurrentCoreTaskPriority > ( ( BaseType_t ) tskIDLE_PRIORITY - 1 ) ) &&
                            ( xCurrentCoreTaskPriority < ( BaseType_t ) pxTCB->uxPriority ) )
                        {
                            prvYieldCore( xCoreID );
                            xYieldCount++;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #endif /* 条件：#if ( configRUN_MULTIPLE_PRIORITIES == 0 ) */
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }

            #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                if( ( xYieldCount == 0 ) && ( xLowestPriorityCore >= 0 ) )
            #else /* 条件：#if ( configRUN_MULTIPLE_PRIORITIES == 0 ) */
                if( xLowestPriorityCore >= 0 )
            #endif /* 条件：#if ( configRUN_MULTIPLE_PRIORITIES == 0 ) */
            {
                prvYieldCore( xLowestPriorityCore );
            }

            #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                /* 确认调用核心始终向更高优先级的任务让出。 */
                if( ( ( pxCurrentTCBs[ xCurrentCoreID ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) == 0U ) &&
                    ( pxTCB->uxPriority > pxCurrentTCBs[ xCurrentCoreID ]->uxPriority ) )
                {
                    configASSERT( ( xYieldPendings[ xCurrentCoreID ] == pdTRUE ) ||
                                  ( taskTASK_IS_RUNNING( pxCurrentTCBs[ xCurrentCoreID ] ) == pdFALSE ) );
                }
            #endif
        }
    }
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )
    STATIC void prvSelectHighestPriorityTask( BaseType_t xCoreID )
    {
        UBaseType_t uxCurrentPriority = uxTopReadyPriority;
        BaseType_t xTaskScheduled = pdFALSE;
        BaseType_t xDecrementTopPriority = pdTRUE;
        TCB_t * pxTCB = NULL;

        #if ( configUSE_CORE_AFFINITY == 1 )
            const TCB_t * pxPreviousTCB = NULL;
        #endif
        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            BaseType_t xPriorityDropped = pdFALSE;
        #endif

        /* 应在调度器运行时调用此函数。 */
        configASSERT( xSchedulerRunning == pdTRUE );

        /*
         * 创建新任务时，正在运行的相同优先级任务会让出 CPU 以运行新任务。
         * 运行中的任务让出自身时仍位于就绪链表中。由于新任务总是加到就绪
         * 链表末尾，该运行中的任务会在新任务之前被选中。另一个问题是，让出
         * 自身后该运行任务在就绪链表中的位置不变，因此可能比其他等待更久的
         * 任务更早被选中。
         *
         * 为解决这些问题，在就绪链表中查找任务之前，应先将正在运行的任务
         * 放到就绪链表末尾。
         */
        if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxCurrentTCBs[ xCoreID ]->uxPriority ] ),
                                     &pxCurrentTCBs[ xCoreID ]->xStateListItem ) == pdTRUE )
        {
            ( void ) uxListRemove( &pxCurrentTCBs[ xCoreID ]->xStateListItem );
            vListInsertEnd( &( pxReadyTasksLists[ pxCurrentTCBs[ xCoreID ]->uxPriority ] ),
                            &pxCurrentTCBs[ xCoreID ]->xStateListItem );
        }

        while( xTaskScheduled == pdFALSE )
        {
            #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
            {
                if( uxCurrentPriority < uxTopReadyPriority )
                {
                    /*
                     * 除空闲任务外，不能调度优先级低于另一核心上当前运行任务
                     * 优先级的任何任务。
                     */
                    uxCurrentPriority = tskIDLE_PRIORITY;
                }
            }
            #endif

            if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxCurrentPriority ] ) ) == pdFALSE )
            {
                const List_t * const pxReadyList = &( pxReadyTasksLists[ uxCurrentPriority ] );
                const ListItem_t * pxEndMarker = listGET_END_MARKER( pxReadyList );
                ListItem_t * pxIterator;

                /*
                 * uxCurrentPriority 对应的就绪任务链表非空，因此 uxTopReadyPriority
                 * 不得再继续递减。
                 */
                xDecrementTopPriority = pdFALSE;

                for( pxIterator = listGET_HEAD_ENTRY( pxReadyList ); pxIterator != pxEndMarker; pxIterator = listGET_NEXT( pxIterator ) )
                {
                    /* MISRA 参考 11.5.3 [void 指针赋值] */
                    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                    /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                    pxTCB = ( TCB_t * ) listGET_LIST_ITEM_OWNER( pxIterator );

                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                    {
                        /*
                         * 当由于同一时间只允许一个优先级运行而回退到空闲优先级时，
                         * 应只调度真正的空闲任务，而不是处于空闲优先级的用户任务。
                         */
                        if( uxCurrentPriority < uxTopReadyPriority )
                        {
                            if( ( pxTCB->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) == 0U )
                            {
                                continue;
                            }
                        }
                    }
                    #endif /* 条件：#if ( configRUN_MULTIPLE_PRIORITIES == 0 ) */

                    if( pxTCB->xTaskRunState == taskTASK_NOT_RUNNING )
                    {
                        #if ( configUSE_CORE_AFFINITY == 1 )
                            if( ( pxTCB->uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                        #endif
                        {
                            /* 若该任务未在任何核心上执行，则将其切入。 */
                            pxCurrentTCBs[ xCoreID ]->xTaskRunState = taskTASK_NOT_RUNNING;
                            #if ( configUSE_CORE_AFFINITY == 1 )
                                pxPreviousTCB = pxCurrentTCBs[ xCoreID ];
                            #endif
                            pxTCB->xTaskRunState = xCoreID;
                            pxCurrentTCBs[ xCoreID ] = pxTCB;
                            xTaskScheduled = pdTRUE;
                        }
                    }
                    else if( pxTCB == pxCurrentTCBs[ xCoreID ] )
                    {
                        configASSERT( ( pxTCB->xTaskRunState == xCoreID ) || ( pxTCB->xTaskRunState == taskTASK_SCHEDULED_TO_YIELD ) );

                        #if ( configUSE_CORE_AFFINITY == 1 )
                            if( ( pxTCB->uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                        #endif
                        {
                            /* 该任务已在此核心上运行，将其标记为已调度。 */
                            pxTCB->xTaskRunState = xCoreID;
                            xTaskScheduled = pdTRUE;
                        }
                    }
                    else
                    {
                        /* 该任务正在 xCoreID 以外的核心上运行。 */
                        mtCOVERAGE_TEST_MARKER();
                    }

                    if( xTaskScheduled != pdFALSE )
                    {
                        /* 已选择在该核心上运行的任务。 */
                        break;
                    }
                }
            }
            else
            {
                if( xDecrementTopPriority != pdFALSE )
                {
                    uxTopReadyPriority--;
                    #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
                    {
                        xPriorityDropped = pdTRUE;
                    }
                    #endif
                }
            }

            /*
             * 调度器启动时会创建 configNUMBER_OF_CORES 个空闲任务。
             * 当 uxCurrentPriority 为 tskIDLE_PRIORITY 时，调度器应能选出一个任务运行。
             * uxCurrentPriority 永远不会递减到低于 tskIDLE_PRIORITY 的值。
             */
            if( uxCurrentPriority > tskIDLE_PRIORITY )
            {
                uxCurrentPriority--;
            }
            else
            {
                /*
                 * 当未创建空闲任务时会调用此函数。
                 * 跳出循环以防止 uxCurrentPriority 越界。
                 */
                break;
            }
        }

        #if ( configRUN_MULTIPLE_PRIORITIES == 0 )
        {
            if( xTaskScheduled == pdTRUE )
            {
                if( xPriorityDropped != pdFALSE )
                {
                    /*
                     * 可能有多个就绪任务因有更高优先级任务在运行而无法运行。
                     * 既然最后一个高优先级任务已不再运行，应确保所有其他空闲任务
                     * 都让出 CPU。
                     */
                    BaseType_t x;

                    for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configNUMBER_OF_CORES; x++ )
                    {
                        if( ( pxCurrentTCBs[ x ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                        {
                            prvYieldCore( x );
                        }
                    }
                }
            }
        }
        #endif /* 条件：#if ( configRUN_MULTIPLE_PRIORITIES == 0 ) */

        #if ( configUSE_CORE_AFFINITY == 1 )
        {
            if( xTaskScheduled == pdTRUE )
            {
                if( ( pxPreviousTCB != NULL ) && ( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxPreviousTCB->uxPriority ] ), &( pxPreviousTCB->xStateListItem ) ) != pdFALSE ) )
                {
                    /*
                     * 一个就绪任务刚被从此核心逐出。
                     * 检查它能否被调度到其他核心上。
                     */
                    UBaseType_t uxCoreMap = pxPreviousTCB->uxCoreAffinityMask;
                    BaseType_t xLowestPriority = ( BaseType_t ) pxPreviousTCB->uxPriority;
                    BaseType_t xLowestPriorityCore = -1;
                    BaseType_t x;

                    if( ( pxPreviousTCB->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                    {
                        xLowestPriority = xLowestPriority - 1;
                    }

                    if( ( uxCoreMap & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) != 0U )
                    {
                        /*
                         * pxPreviousTCB 已从此核心移除，且此核心未从其核心亲和性掩码中排除。
                         *
                         * pxPreviousTCB 被新的更高优先级任务 pxCurrentTCBs[ xCoreID ] 抢占。
                         * 为 pxPreviousTCB 搜索新核心时，无需查看允许 pxCurrentTCBs[ xCoreID ]
                         * 运行的核心。原因是——当多个核心都可用于接纳一个任务时，
                         * 会抢占运行最低优先级任务的核心。由于此核心（即 xCoreID）是为
                         * pxCurrentTCBs[ xCoreID ] 而被抢占的，这意味着 pxCurrentTCBs[ xCoreID ]
                         * 可运行的所有其他核心上，正在运行的任务优先级都不低于 pxPreviousTCB 的
                         * 优先级。因此，能够为 pxPreviousTCB 被抢占的核心，只能是那些不允许
                         * pxCurrentTCBs[ xCoreID ] 运行的核心（当然，pxPreviousTCB 允许在这些核心上运行）。
                         *
                         * 这是一项优化，可减少为让 pxPreviousTCB 运行而需要搜索的核心数量。
                         */
                        uxCoreMap &= ~( pxCurrentTCBs[ xCoreID ]->uxCoreAffinityMask );
                    }
                    else
                    {
                        /*
                         * pxPreviousTCB 的核心亲和性掩码已改变，不再允许在此核心上运行。
                         * 在 pxPreviousTCB 新的核心亲和性掩码中搜索所有核心，
                         * 以找到可运行的核心。
                         */
                    }

                    uxCoreMap &= ( ( 1U << configNUMBER_OF_CORES ) - 1U );

                    for( x = ( ( BaseType_t ) configNUMBER_OF_CORES - 1 ); x >= ( BaseType_t ) 0; x-- )
                    {
                        UBaseType_t uxCore = ( UBaseType_t ) x;
                        BaseType_t xTaskPriority;

                        if( ( uxCoreMap & ( ( UBaseType_t ) 1U << uxCore ) ) != 0U )
                        {
                            xTaskPriority = ( BaseType_t ) pxCurrentTCBs[ uxCore ]->uxPriority;

                            if( ( pxCurrentTCBs[ uxCore ]->uxTaskAttributes & taskATTRIBUTE_IS_IDLE ) != 0U )
                            {
                                xTaskPriority = xTaskPriority - ( BaseType_t ) 1;
                            }

                            uxCoreMap &= ~( ( UBaseType_t ) 1U << uxCore );

                            if( ( xTaskPriority < xLowestPriority ) &&
                                ( taskTASK_IS_RUNNING( pxCurrentTCBs[ uxCore ] ) != pdFALSE ) &&
                                ( xYieldPendings[ uxCore ] == pdFALSE ) )
                            {
                                #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                                    if( pxCurrentTCBs[ uxCore ]->xPreemptionDisable == pdFALSE )
                                #endif
                                {
                                    xLowestPriority = xTaskPriority;
                                    xLowestPriorityCore = ( BaseType_t ) uxCore;
                                }
                            }
                        }
                    }

                    if( xLowestPriorityCore >= 0 )
                    {
                        prvYieldCore( xLowestPriorityCore );
                    }
                }
            }
        }
        #endif /* 条件：#if ( configUSE_CORE_AFFINITY == 1 ) */
    }

#endif /* 条件：( configNUMBER_OF_CORES > 1 ) */

/*-----------------------------------------------------------*/

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

    STATIC TCB_t * prvCreateStaticTask( TaskFunction_t pxTaskCode,
                                        const char * const pcName,
                                        const configSTACK_DEPTH_TYPE uxStackDepth,
                                        void * const pvParameters,
                                        UBaseType_t uxPriority,
                                        StackType_t * const puxStackBuffer,
                                        StaticTask_t * const pxTaskBuffer,
                                        TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;

        configASSERT( puxStackBuffer != NULL );
        configASSERT( pxTaskBuffer != NULL );

        #if ( configASSERT_DEFINED == 1 )
        {
            /*
             * 进行完整性检查：用于声明 StaticTask_t 类型变量的结构体大小
             * 应等于真实任务结构体的大小。
             */
            volatile size_t xSize = sizeof( StaticTask_t );
            configASSERT( xSize == sizeof( TCB_t ) );
            ( void ) xSize; /* 在未使用 configASSERT() 时防止出现未使用变量警告。 */
        }
        #endif /* 条件：configASSERT_DEFINED */

        if( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
        {
            /*
             * 任务的 TCB 和栈所用内存已传入此函数——直接使用它们。
             */
            /* MISRA 参考 11.3.1 [非对齐访问] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
            /* coverity[misra_c_2012_rule_11_3_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.3 告警 */
            pxNewTCB = ( TCB_t * ) pxTaskBuffer;
            ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );
            pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;

            #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
            {
                /*
                 * 任务可以静态或动态创建，因此在此记录该任务为静态创建，
                 * 以备后续删除任务时使用。
                 */
                pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
            }
            #endif /* 条件：tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

            prvInitialiseNewTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
        }
        else
        {
            pxNewTCB = NULL;
        }

        return pxNewTCB;
    }
/*-----------------------------------------------------------*/

    TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                    const char * const pcName,
                                    const configSTACK_DEPTH_TYPE uxStackDepth,
                                    void * const pvParameters,
                                    UBaseType_t uxPriority,
                                    StackType_t * const puxStackBuffer,
                                    StaticTask_t * const pxTaskBuffer )
    {
        TaskHandle_t xReturn = NULL;
        TCB_t * pxNewTCB;

        traceENTER_xTaskCreateStatic( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer );

        pxNewTCB = prvCreateStaticTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, &xReturn );

        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif

            prvAddNewTaskToReadyList( pxNewTCB );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTaskCreateStatic( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        TaskHandle_t xTaskCreateStaticAffinitySet( TaskFunction_t pxTaskCode,
                                                   const char * const pcName,
                                                   const configSTACK_DEPTH_TYPE uxStackDepth,
                                                   void * const pvParameters,
                                                   UBaseType_t uxPriority,
                                                   StackType_t * const puxStackBuffer,
                                                   StaticTask_t * const pxTaskBuffer,
                                                   UBaseType_t uxCoreAffinityMask )
        {
            TaskHandle_t xReturn = NULL;
            TCB_t * pxNewTCB;

            traceENTER_xTaskCreateStaticAffinitySet( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, uxCoreAffinityMask );

            pxNewTCB = prvCreateStaticTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, &xReturn );

            if( pxNewTCB != NULL )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;

                prvAddNewTaskToReadyList( pxNewTCB );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            traceRETURN_xTaskCreateStaticAffinitySet( xReturn );

            return xReturn;
        }
    #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */

#endif /* 条件：SUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    STATIC TCB_t * prvCreateRestrictedStaticTask( const TaskParameters_t * const pxTaskDefinition,
                                                  TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;

        configASSERT( pxTaskDefinition->puxStackBuffer != NULL );
        configASSERT( pxTaskDefinition->pxTaskBuffer != NULL );

        if( ( pxTaskDefinition->puxStackBuffer != NULL ) && ( pxTaskDefinition->pxTaskBuffer != NULL ) )
        {
            /*
             * 为 TCB 分配空间。内存来源取决于移植层 malloc 函数的实现
             * 以及是否使用静态分配。
             */
            pxNewTCB = ( TCB_t * ) pxTaskDefinition->pxTaskBuffer;
            ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );

            /* 将栈位置保存到 TCB 中。 */
            pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;

            #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
            {
                /*
                 * 任务可以静态或动态创建，因此在此记录该任务为静态创建，
                 * 以备后续删除任务时使用。
                 */
                pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
            }
            #endif /* 条件：tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

            prvInitialiseNewTask( pxTaskDefinition->pvTaskCode,
                                  pxTaskDefinition->pcName,
                                  pxTaskDefinition->usStackDepth,
                                  pxTaskDefinition->pvParameters,
                                  pxTaskDefinition->uxPriority,
                                  pxCreatedTask, pxNewTCB,
                                  pxTaskDefinition->xRegions );
        }
        else
        {
            pxNewTCB = NULL;
        }

        return pxNewTCB;
    }
/*-----------------------------------------------------------*/

    BaseType_t xTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        BaseType_t xReturn;

        traceENTER_xTaskCreateRestrictedStatic( pxTaskDefinition, pxCreatedTask );

        configASSERT( pxTaskDefinition != NULL );

        pxNewTCB = prvCreateRestrictedStaticTask( pxTaskDefinition, pxCreatedTask );

        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif

            prvAddNewTaskToReadyList( pxNewTCB );
            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }

        traceRETURN_xTaskCreateRestrictedStatic( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        BaseType_t xTaskCreateRestrictedStaticAffinitySet( const TaskParameters_t * const pxTaskDefinition,
                                                           UBaseType_t uxCoreAffinityMask,
                                                           TaskHandle_t * pxCreatedTask )
        {
            TCB_t * pxNewTCB;
            BaseType_t xReturn;

            traceENTER_xTaskCreateRestrictedStaticAffinitySet( pxTaskDefinition, uxCoreAffinityMask, pxCreatedTask );

            configASSERT( pxTaskDefinition != NULL );

            pxNewTCB = prvCreateRestrictedStaticTask( pxTaskDefinition, pxCreatedTask );

            if( pxNewTCB != NULL )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;

                prvAddNewTaskToReadyList( pxNewTCB );
                xReturn = pdPASS;
            }
            else
            {
                xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
            }

            traceRETURN_xTaskCreateRestrictedStaticAffinitySet( xReturn );

            return xReturn;
        }
    #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */

#endif /* 条件：( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
/*-----------------------------------------------------------*/

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    STATIC TCB_t * prvCreateRestrictedTask( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;

        configASSERT( pxTaskDefinition->puxStackBuffer );

        if( pxTaskDefinition->puxStackBuffer != NULL )
        {
            /* MISRA 参考 11.5.1 [malloc 内存赋值] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
            pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

            if( pxNewTCB != NULL )
            {
                ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );

                /* 将栈位置保存到 TCB 中。 */
                pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;

                #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
                {
                    /*
                     * 任务可以静态或动态创建，因此在此记录该任务使用了静态分配的栈，
                     * 以备后续删除任务时使用。TCB 为动态分配。
                     */
                    pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_ONLY;
                }
                #endif /* 条件：tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

                prvInitialiseNewTask( pxTaskDefinition->pvTaskCode,
                                      pxTaskDefinition->pcName,
                                      pxTaskDefinition->usStackDepth,
                                      pxTaskDefinition->pvParameters,
                                      pxTaskDefinition->uxPriority,
                                      pxCreatedTask, pxNewTCB,
                                      pxTaskDefinition->xRegions );
            }
        }
        else
        {
            pxNewTCB = NULL;
        }

        return pxNewTCB;
    }
/*-----------------------------------------------------------*/

    BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
                                      TaskHandle_t * pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        BaseType_t xReturn;

        traceENTER_xTaskCreateRestricted( pxTaskDefinition, pxCreatedTask );

        pxNewTCB = prvCreateRestrictedTask( pxTaskDefinition, pxCreatedTask );

        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */

            prvAddNewTaskToReadyList( pxNewTCB );

            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }

        traceRETURN_xTaskCreateRestricted( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        BaseType_t xTaskCreateRestrictedAffinitySet( const TaskParameters_t * const pxTaskDefinition,
                                                     UBaseType_t uxCoreAffinityMask,
                                                     TaskHandle_t * pxCreatedTask )
        {
            TCB_t * pxNewTCB;
            BaseType_t xReturn;

            traceENTER_xTaskCreateRestrictedAffinitySet( pxTaskDefinition, uxCoreAffinityMask, pxCreatedTask );

            pxNewTCB = prvCreateRestrictedTask( pxTaskDefinition, pxCreatedTask );

            if( pxNewTCB != NULL )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;

                prvAddNewTaskToReadyList( pxNewTCB );

                xReturn = pdPASS;
            }
            else
            {
                xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
            }

            traceRETURN_xTaskCreateRestrictedAffinitySet( xReturn );

            return xReturn;
        }
    #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */


#endif /* 条件：portUSING_MPU_WRAPPERS */
/*-----------------------------------------------------------*/

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    STATIC TCB_t * prvCreateTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;

        /*
         * 若栈向下增长，则先分配栈再分配 TCB，以免栈增长时覆盖 TCB。
         * 反之，若栈向上增长，则先分配 TCB 再分配栈。
         */
        #if ( portSTACK_GROWTH > 0 )
        {
            /*
             * 为 TCB 分配空间。内存来源取决于移植层 malloc 函数的实现
             * 以及是否使用静态分配。
             */
            /* MISRA 参考 11.5.1 [malloc 内存赋值] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
            pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

            if( pxNewTCB != NULL )
            {
                ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );

                /*
                 * 为正在创建的任务分配栈空间。栈内存的基址保存在 TCB 中，
                 * 以便后续需要时删除该任务。
                 */
                /* MISRA 参考 11.5.1 [malloc 内存赋值] */
                /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                pxNewTCB->pxStack = ( StackType_t * ) pvPortMallocStack( ( ( ( size_t ) uxStackDepth ) * sizeof( StackType_t ) ) );

                if( pxNewTCB->pxStack == NULL )
                {
                    /* 无法分配栈。删除已分配的 TCB。 */
                    vPortFree( pxNewTCB );
                    pxNewTCB = NULL;
                }
            }
        }
        #else /* 条件：portSTACK_GROWTH */
        {
            StackType_t * pxStack;

            /* 为正在创建的任务分配栈空间。 */
            /* MISRA 参考 11.5.1 [malloc 内存赋值] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
            pxStack = ( StackType_t * ) pvPortMallocStack( ( ( ( size_t ) uxStackDepth ) * sizeof( StackType_t ) ) );

            if( pxStack != NULL )
            {
                /* 为 TCB 分配空间。 */
                /* MISRA 参考 11.5.1 [malloc 内存赋值] */
                /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

                if( pxNewTCB != NULL )
                {
                    ( void ) memset( ( void * ) pxNewTCB, 0x00, sizeof( TCB_t ) );

                    /* 将栈位置保存到 TCB 中。 */
                    pxNewTCB->pxStack = pxStack;
                }
                else
                {
                    /*
                     * 由于 TCB 未能创建，该栈无法使用。
                     * 将其释放。
                     */
                    vPortFreeStack( pxStack );
                }
            }
            else
            {
                pxNewTCB = NULL;
            }
        }
        #endif /* 条件：portSTACK_GROWTH */

        if( pxNewTCB != NULL )
        {
            #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
            {
                /*
                 * 任务可以静态或动态创建，因此在此记录该任务为动态创建，
                 * 以备后续删除任务时使用。
                 */
                pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;
            }
            #endif /* 条件：tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

            prvInitialiseNewTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
        }

        return pxNewTCB;
    }
/*-----------------------------------------------------------*/

    BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE uxStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask )
    {
        TCB_t * pxNewTCB;
        BaseType_t xReturn;

        traceENTER_xTaskCreate( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );

        pxNewTCB = prvCreateTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );

        if( pxNewTCB != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = configTASK_DEFAULT_CORE_AFFINITY;
            }
            #endif

            prvAddNewTaskToReadyList( pxNewTCB );
            xReturn = pdPASS;
        }
        else
        {
            xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        }

        traceRETURN_xTaskCreate( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
        BaseType_t xTaskCreateAffinitySet( TaskFunction_t pxTaskCode,
                                           const char * const pcName,
                                           const configSTACK_DEPTH_TYPE uxStackDepth,
                                           void * const pvParameters,
                                           UBaseType_t uxPriority,
                                           UBaseType_t uxCoreAffinityMask,
                                           TaskHandle_t * const pxCreatedTask )
        {
            TCB_t * pxNewTCB;
            BaseType_t xReturn;

            traceENTER_xTaskCreateAffinitySet( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, uxCoreAffinityMask, pxCreatedTask );

            pxNewTCB = prvCreateTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );

            if( pxNewTCB != NULL )
            {
                /* 在调度该任务之前设置其核心亲和性。 */
                pxNewTCB->uxCoreAffinityMask = uxCoreAffinityMask;

                prvAddNewTaskToReadyList( pxNewTCB );
                xReturn = pdPASS;
            }
            else
            {
                xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
            }

            traceRETURN_xTaskCreateAffinitySet( xReturn );

            return xReturn;
        }
    #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */

#endif /* 条件：configSUPPORT_DYNAMIC_ALLOCATION */
/*-----------------------------------------------------------*/

STATIC void prvInitialiseNewTask( TaskFunction_t pxTaskCode,
                                  const char * const pcName,
                                  const configSTACK_DEPTH_TYPE uxStackDepth,
                                  void * const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t * const pxCreatedTask,
                                  TCB_t * pxNewTCB,
                                  const MemoryRegion_t * const xRegions )
{
    StackType_t * pxTopOfStack;
    UBaseType_t x;

    #if ( portUSING_MPU_WRAPPERS == 1 )
        /* 是否以特权模式创建该任务？ */
        BaseType_t xRunPrivileged;

        if( ( uxPriority & portPRIVILEGE_BIT ) != 0U )
        {
            xRunPrivileged = pdTRUE;
        }
        else
        {
            xRunPrivileged = pdFALSE;
        }
        uxPriority &= ~portPRIVILEGE_BIT;
    #endif /* 条件：portUSING_MPU_WRAPPERS == 1 */

    /* 若不需要，则避免依赖 memset()。 */
    #if ( tskSET_NEW_STACKS_TO_KNOWN_VALUE == 1 )
    {
        /* 用已知值填充栈以辅助调试。 */
        ( void ) memset( pxNewTCB->pxStack, ( int ) tskSTACK_FILL_BYTE, ( size_t ) uxStackDepth * sizeof( StackType_t ) );
    }
    #endif /* 条件：tskSET_NEW_STACKS_TO_KNOWN_VALUE */

    /*
     * 计算栈顶地址。这取决于栈是从高地址向低地址增长（如 80x86）
     * 还是相反。portSTACK_GROWTH 用于按移植层的要求使结果为正或负。
     */
    #if ( portSTACK_GROWTH < 0 )
    {
        pxTopOfStack = &( pxNewTCB->pxStack[ uxStackDepth - ( configSTACK_DEPTH_TYPE ) 1 ] );
        pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

        /* 检查计算出的栈顶对齐是否正确。 */
        configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0U ) );

        #if ( configRECORD_STACK_HIGH_ADDRESS == 1 )
        {
            /*
             * 同时记录栈的高地址，以便辅助调试。
             */
            pxNewTCB->pxEndOfStack = pxTopOfStack;
        }
        #endif /* 条件：configRECORD_STACK_HIGH_ADDRESS */
    }
    #else /* 条件：portSTACK_GROWTH */
    {
        pxTopOfStack = pxNewTCB->pxStack;
        pxTopOfStack = ( StackType_t * ) ( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) + portBYTE_ALIGNMENT_MASK ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

        /* 检查计算出的栈顶对齐是否正确。 */
        configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0U ) );

        /*
         * 若执行栈检查，则需要栈空间的另一端地址。
         */
        pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( uxStackDepth - ( configSTACK_DEPTH_TYPE ) 1 );
    }
    #endif /* 条件：portSTACK_GROWTH */

    /* 将任务名称保存到 TCB 中。 */
    if( pcName != NULL )
    {
        for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
        {
            pxNewTCB->pcTaskName[ x ] = pcName[ x ];

            /*
             * 若字符串短于 configMAX_TASK_NAME_LEN 个字符，则不要复制整个
             * configMAX_TASK_NAME_LEN 长度，以防字符串之后的内存不可访问（可能性极小）。
             */
            if( pcName[ x ] == ( char ) 0x00 )
            {
                break;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        /*
         * 若字符串长度大于或等于 configMAX_TASK_NAME_LEN，
         * 确保名称字符串被终止。
         */
        pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1U ] = '\0';
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* 该值用作数组下标，因此必须确保其不会过大。 */
    configASSERT( uxPriority < configMAX_PRIORITIES );

    if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
        uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    pxNewTCB->uxPriority = uxPriority;
    #if ( configUSE_MUTEXES == 1 )
    {
        pxNewTCB->uxBasePriority = uxPriority;
    }
    #endif /* 条件：configUSE_MUTEXES */

    vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
    vListInitialiseItem( &( pxNewTCB->xEventListItem ) );

    /*
     * 将 pxNewTCB 设置为从 ListItem_t 反向链接的对象。这样便可从链表中的
     * 通用链表项找回所属的 TCB。
     */
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );

    /* 事件链表始终按优先级顺序排列。 */
    listSET_LIST_ITEM_VALUE( &( pxNewTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriority );
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xEventListItem ), pxNewTCB );

    #if ( portUSING_MPU_WRAPPERS == 1 )
    {
        vPortStoreTaskMPUSettings( &( pxNewTCB->xMPUSettings ), xRegions, pxNewTCB->pxStack, uxStackDepth );
    }
    #else
    {
        /* 避免编译器产生未引用参数警告。 */
        ( void ) xRegions;
    }
    #endif

    #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
    {
        /* 为任务的 TLS 块分配并初始化内存。 */
        configINIT_TLS_BLOCK( pxNewTCB->xTLSBlock, pxTopOfStack );
    }
    #endif

    /*
     * 初始化 TCB 栈，使其看起来如同任务已在运行但被调度器中断。
     * 返回地址被设为任务函数的起始地址。栈初始化完成后，
     * 更新栈顶变量。
     */
    #if ( portUSING_MPU_WRAPPERS == 1 )
    {
        /*
         * 若移植层具备栈溢出检测能力，
         * 还应将栈结束地址传给栈初始化函数。
         */
        #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        {
            #if ( portSTACK_GROWTH < 0 )
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxStack, pxTaskCode, pvParameters, xRunPrivileged, &( pxNewTCB->xMPUSettings ) );
            }
            #else /* 条件：portSTACK_GROWTH */
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxEndOfStack, pxTaskCode, pvParameters, xRunPrivileged, &( pxNewTCB->xMPUSettings ) );
            }
            #endif /* 条件：portSTACK_GROWTH */
        }
        #else /* 条件：portHAS_STACK_OVERFLOW_CHECKING */
        {
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged, &( pxNewTCB->xMPUSettings ) );
        }
        #endif /* 条件：portHAS_STACK_OVERFLOW_CHECKING */
    }
    #else /* 条件：portUSING_MPU_WRAPPERS */
    {
        /*
         * 若移植层具备栈溢出检测能力，
         * 还应将栈结束地址传给栈初始化函数。
         */
        #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        {
            #if ( portSTACK_GROWTH < 0 )
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxStack, pxTaskCode, pvParameters );
            }
            #else /* 条件：portSTACK_GROWTH */
            {
                pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxEndOfStack, pxTaskCode, pvParameters );
            }
            #endif /* 条件：portSTACK_GROWTH */
        }
        #else /* 条件：portHAS_STACK_OVERFLOW_CHECKING */
        {
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );
        }
        #endif /* 条件：portHAS_STACK_OVERFLOW_CHECKING */

        #if ( portSTACK_GROWTH < 0 )
        {
            configASSERT( ( ( portPOINTER_SIZE_TYPE ) ( pxTopOfStack - pxNewTCB->pxTopOfStack ) ) < ( ( portPOINTER_SIZE_TYPE ) uxStackDepth ) );
        }
        #else /* 条件：portSTACK_GROWTH */
        {
            configASSERT( ( ( portPOINTER_SIZE_TYPE ) ( pxNewTCB->pxTopOfStack - pxTopOfStack ) ) < ( ( portPOINTER_SIZE_TYPE ) uxStackDepth ) );
        }
        #endif /* 条件：portSTACK_GROWTH */
    }
    #endif /* 条件：portUSING_MPU_WRAPPERS */

    /* 初始化任务状态和任务属性。 */
    #if ( configNUMBER_OF_CORES > 1 )
    {
        pxNewTCB->xTaskRunState = taskTASK_NOT_RUNNING;

        /* 这是空闲任务吗？ */
        if( ( ( TaskFunction_t ) pxTaskCode == ( TaskFunction_t ) ( &prvIdleTask ) ) || ( ( TaskFunction_t ) pxTaskCode == ( TaskFunction_t ) ( &prvPassiveIdleTask ) ) )
        {
            pxNewTCB->uxTaskAttributes |= taskATTRIBUTE_IS_IDLE;
        }
    }
    #endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

    if( pxCreatedTask != NULL )
    {
        /*
         * 以匿名方式传出句柄。该句柄可用于修改已创建任务的优先级、
         * 删除已创建任务等。
         */
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES == 1 )

    STATIC void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
    {
        /*
         * 确保链表更新期间中断不会访问任务链表。
         */
        taskENTER_CRITICAL();
        {
            uxCurrentNumberOfTasks = ( UBaseType_t ) ( uxCurrentNumberOfTasks + 1U );

            if( pxCurrentTCB == NULL )
            {
                /*
                 * 没有其他任务，或所有其他任务都处于挂起态——
                 * 将此任务设为当前任务。
                 */
                pxCurrentTCB = pxNewTCB;

                if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
                {
                    /*
                     * 这是创建的第一个任务，因此执行所需的初步初始化。
                     * 若此调用失败将无法恢复，但会报告该失败。
                     */
                    prvInitialiseTaskLists();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /*
                 * 若调度器尚未运行，且该任务是截至目前创建的最高优先级任务，
                 * 则将其设为当前任务。
                 */
                if( xSchedulerRunning == pdFALSE )
                {
                    if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
                    {
                        pxCurrentTCB = pxNewTCB;
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

            uxTaskNumber++;

            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                /* 在 TCB 中增加一个仅供跟踪使用的计数器。 */
                pxNewTCB->uxTCBNumber = uxTaskNumber;
            }
            #endif /* 条件：configUSE_TRACE_FACILITY */
            traceTASK_CREATE( pxNewTCB );

            prvAddTaskToReadyList( pxNewTCB );

            portSETUP_TCB( pxNewTCB );
        }
        taskEXIT_CRITICAL();

        if( xSchedulerRunning != pdFALSE )
        {
            /*
             * 若所创建任务的优先级高于当前任务，
             * 则它应立即运行。
             */
            taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxNewTCB );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }

#else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

    STATIC void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
    {
        /*
         * 确保链表更新期间中断不会访问任务链表。
         */
        taskENTER_CRITICAL();
        {
            uxCurrentNumberOfTasks++;

            if( xSchedulerRunning == pdFALSE )
            {
                if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
                {
                    /*
                     * 这是创建的第一个任务，因此执行所需的初步初始化。
                     * 若此调用失败将无法恢复，但会报告该失败。
                     */
                    prvInitialiseTaskLists();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                /*
                 * SMP 调度器运行之前，所有核心都以空闲任务启动。
                 * 空闲任务在 prvCreateIdleTasks() 中创建时被分配到各核心。
                 */
            }

            uxTaskNumber++;

            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                /* 在 TCB 中增加一个仅供跟踪使用的计数器。 */
                pxNewTCB->uxTCBNumber = uxTaskNumber;
            }
            #endif /* 条件：configUSE_TRACE_FACILITY */
            traceTASK_CREATE( pxNewTCB );

            prvAddTaskToReadyList( pxNewTCB );

            portSETUP_TCB( pxNewTCB );

            if( xSchedulerRunning != pdFALSE )
            {
                /*
                 * 若所创建任务的优先级高于另一当前正在运行的任务，且已启用抢占，
                 * 则它应立即运行。
                 */
                taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxNewTCB );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();
    }

#endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

    STATIC size_t prvSnprintfReturnValueToCharsWritten( int iSnprintfReturnValue,
                                                        size_t n )
    {
        size_t uxCharsWritten;

        if( iSnprintfReturnValue < 0 )
        {
            /*
             * 编码错误——返回 0 表示未向缓冲区写入任何内容。
             */
            uxCharsWritten = 0;
        }
        else if( iSnprintfReturnValue >= ( int ) n )
        {
            /*
             * 这是提供的缓冲区不足以容纳生成字符串的情况。
             * 返回实际写入的字符数，不计终止空字符。
             */
            uxCharsWritten = n - 1U;
        }
        else
        {
            /* 完整字符串已写入缓冲区。 */
            uxCharsWritten = ( size_t ) iSnprintfReturnValue;
        }

        return uxCharsWritten;
    }

#endif /* 条件：#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) ) */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelete == 1 )

    void vTaskDelete( TaskHandle_t xTaskToDelete )
    {
        TCB_t * pxTCB;
        BaseType_t xDeleteTCBInIdleTask = pdFALSE;
        BaseType_t xTaskIsRunningOrYielding;

        traceENTER_vTaskDelete( xTaskToDelete );

        taskENTER_CRITICAL();
        {
            /*
             * 若此处传入 NULL，则表示正在删除调用任务自身。
             */
            pxTCB = prvGetTCBFromHandle( xTaskToDelete );
            configASSERT( pxTCB != NULL );

            /* 将任务从就绪/延时链表中移除。 */
            if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
            {
                taskRESET_READY_PRIORITY( pxTCB->uxPriority );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /* 该任务是否还在等待某个事件？ */
            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
            {
                ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /*
             * 同时递增 uxTaskNumber，以便内核感知调试器检测到任务链表需要重新生成。
             * 此操作在 portPRE_TASK_DELETE_HOOK() 之前执行，因为在 Windows 移植层中
             * 该宏不会返回。
             */
            uxTaskNumber++;

            /*
             * 使用临时变量作为逻辑运算符之前读取 volatile 变量的独立顺序点，
             * 以确保符合 MISRA C 2012 规则 13.5。
             */
            xTaskIsRunningOrYielding = taskTASK_IS_RUNNING_OR_SCHEDULED_TO_YIELD( pxTCB );

            /*
             * 若任务正在运行（或正在让出），必须将其加入终止链表，以便空闲任务
             * 在它不再运行时将其删除。
             */
            if( ( xSchedulerRunning != pdFALSE ) && ( xTaskIsRunningOrYielding != pdFALSE ) )
            {
                /*
                 * 正在删除一个正在运行或已被调度让出的任务。当该任务仍在核心上运行时，
                 * 删除无法完成，因为需要上下文切换到其他任务。将任务放入终止链表。
                 * 空闲任务会检查终止链表，并释放调度器为该被删除任务的 TCB 和栈
                 * 分配的内存。
                 */
                vListInsertEnd( &xTasksWaitingTermination, &( pxTCB->xStateListItem ) );

                /*
                 * 递增 ucTasksDeleted 变量，以便空闲任务知道有任务已被删除，
                 * 从而应检查 xTasksWaitingTermination 链表。
                 */
                ++uxDeletedTasksWaitingCleanUp;

                /*
                 * 在 portPRE_TASK_DELETE_HOOK() 之前调用删除钩子函数，
                 * 因为在 Win32 移植层中 portPRE_TASK_DELETE_HOOK() 不会返回。
                 */
                traceTASK_DELETE( pxTCB );

                /* 在空闲任务中删除任务 TCB。 */
                xDeleteTCBInIdleTask = pdTRUE;

                /*
                 * 删除前钩子主要用于 Windows 模拟器，在其中执行 Windows 特定的
                 * 清理操作；此后无法再从此任务让出——因此使用 xYieldPending
                 * 锁存需要上下文切换的请求。
                 */
                #if ( configNUMBER_OF_CORES == 1 )
                    portPRE_TASK_DELETE_HOOK( pxTCB, &( xYieldPendings[ 0 ] ) );
                #else
                    portPRE_TASK_DELETE_HOOK( pxTCB, &( xYieldPendings[ pxTCB->xTaskRunState ] ) );
                #endif

                /*
                 * 在 SMP 情况下，被删除的任务可能正在另一个核心上运行。必须在退出
                 * 临界区之前将该任务逐出，以确保它无法执行将其重新放回就绪/状态/
                 * 事件链表的操作，从而使删除操作失效。一旦被逐出，该任务将不再位于
                 * 就绪链表中，因此永远不会再被调度。
                 */
                #if ( configNUMBER_OF_CORES > 1 )
                {
                    if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                    {
                        if( pxTCB->xTaskRunState == ( BaseType_t ) portGET_CORE_ID() )
                        {
                            configASSERT( uxSchedulerSuspended == 0 );
                            taskYIELD_WITHIN_API();
                        }
                        else
                        {
                            prvYieldCore( pxTCB->xTaskRunState );
                        }
                    }
                }
                #endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
            }
            else
            {
                --uxCurrentNumberOfTasks;
                traceTASK_DELETE( pxTCB );

                /*
                 * 重置下一个预期的解除阻塞时间，以防它引用的是刚被删除的任务。
                 */
                prvResetNextTaskUnblockTime();
            }
        }
        taskEXIT_CRITICAL();

        /*
         * 若任务不是删除自身，则在临界区之外调用 prvDeleteTCB。
         * 若任务删除自身，则 prvDeleteTCB 由空闲任务调用的
         * prvCheckTasksWaitingTermination 调用。
         */
        if( xDeleteTCBInIdleTask != pdTRUE )
        {
            prvDeleteTCB( pxTCB );
        }

        /*
         * 若刚被删除的是当前正在运行的任务，则强制重新调度。
         */
        #if ( configNUMBER_OF_CORES == 1 )
        {
            if( xSchedulerRunning != pdFALSE )
            {
                if( pxTCB == pxCurrentTCB )
                {
                    configASSERT( uxSchedulerSuspended == 0 );
                    taskYIELD_WITHIN_API();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

        traceRETURN_vTaskDelete();
    }

#endif /* 条件：INCLUDE_vTaskDelete */
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskDelayUntil == 1 )

    TickType_t xTaskPeriodicDelay( TickType_t * const pxPreviousWakeTime,
                                   const TickType_t xTimeIncrement )
    {
        TickType_t xIncrements, xTicksIncrements, xTicksToWait;

        traceENTER_xTaskPeriodicDelay( pxPreviousWakeTime, xTimeIncrement );

        configASSERT( pxPreviousWakeTime );
        configASSERT( ( xTimeIncrement > 0U ) );

        vTaskSuspendAll();
        {
            /* 只要所有值类型相同，这种处理在溢出时也能正确工作。 */
            const TickType_t xTicksElapsed = xTickCount - *pxPreviousWakeTime;

            configASSERT( uxSchedulerSuspended == 1U );

            /*
             * 需要追赶的节拍增量次数：若经过的节拍不足则为 0，
             * 常见情况下为 1，若任务未及时恢复则可能大于 1。
             */
            xIncrements = xTicksElapsed / xTimeIncrement;
            xTicksIncrements = xIncrements * xTimeIncrement;

            /* 更新上次唤醒时间 */
            *pxPreviousWakeTime += xTicksIncrements;

            /* 距下次唤醒的节拍数 */
            xTicksToWait = xTimeIncrement - ( xTicksElapsed - xTicksIncrements );

            prvAddCurrentTaskToDelayedList( xTicksToWait, pdFALSE );
        }

        /*
         * 若 xTaskResumeAll 尚未强制重新调度，则强制重新调度；
         * 此时本任务可能已进入睡眠。
         */
        if( xTaskResumeAll() == pdFALSE )
        {
            taskYIELD_WITHIN_API();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTaskPeriodicDelay( xIncrements );

        return xIncrements;
    }


    BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime,
                                const TickType_t xTimeIncrement )
    {
        TickType_t xTimeToWake;
        BaseType_t xAlreadyYielded, xShouldDelay = pdFALSE;

        traceENTER_xTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement );

        configASSERT( pxPreviousWakeTime );
        configASSERT( ( xTimeIncrement > 0U ) );

        vTaskSuspendAll();
        {
            /*
             * 小幅优化。此代码块内节拍计数不会变化。
             */
            const TickType_t xConstTickCount = xTickCount;

            configASSERT( uxSchedulerSuspended == 1U );

            /* 计算任务期望唤醒的节拍时间。 */
            xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

            if( xConstTickCount < *pxPreviousWakeTime )
            {
                /*
                 * 自上次调用此函数以来节拍计数已溢出。此时，只有当唤醒时间也溢出
                 * 且唤醒时间大于节拍计数时才应真正延时。在这种情况下，
                 * 就如同两个时间都未溢出一样。
                 */
                if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xConstTickCount ) )
                {
                    xShouldDelay = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                /*
                 * 节拍时间未溢出。此时若唤醒时间已溢出，和/或节拍时间
                 * 小于唤醒时间，则执行延时。
                 */
                if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xConstTickCount ) )
                {
                    xShouldDelay = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }

            /* 更新唤醒时间，供下次调用使用。 */
            *pxPreviousWakeTime = xTimeToWake;

            if( xShouldDelay != pdFALSE )
            {
                traceTASK_DELAY_UNTIL( xTimeToWake );

                /*
                 * prvAddCurrentTaskToDelayedList() 需要的是阻塞时间而非唤醒时间，
                 * 因此减去当前节拍计数。
                 */
                prvAddCurrentTaskToDelayedList( xTimeToWake - xConstTickCount, pdFALSE );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        xAlreadyYielded = xTaskResumeAll();

        /*
         * 若 xTaskResumeAll 尚未强制重新调度，则强制重新调度；
         * 此时本任务可能已进入睡眠。
         */
        if( xAlreadyYielded == pdFALSE )
        {
            taskYIELD_WITHIN_API();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTaskDelayUntil( xShouldDelay );

        return xShouldDelay;
    }

#endif /* 条件：INCLUDE_xTaskDelayUntil */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelay == 1 )

    void vTaskDelay( const TickType_t xTicksToDelay )
    {
        BaseType_t xAlreadyYielded = pdFALSE;

        traceENTER_vTaskDelay( xTicksToDelay );

        /* 延时时间为 0 只会强制重新调度。 */
        if( xTicksToDelay > ( TickType_t ) 0U )
        {
            vTaskSuspendAll();
            {
                configASSERT( uxSchedulerSuspended == 1U );

                traceTASK_DELAY();

                /*
                 * 在调度器挂起期间从事件链表中移除的任务，在调度器恢复之前
                 * 既不会被放入就绪链表，也不会从阻塞链表中移除。
                 *
                 * 当前正在执行的任务不可能位于事件链表中。
                 */
                prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );
            }
            xAlreadyYielded = xTaskResumeAll();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /*
         * 若 xTaskResumeAll 尚未强制重新调度，则强制重新调度；
         * 此时本任务可能已进入睡眠。
         */
        if( xAlreadyYielded == pdFALSE )
        {
            taskYIELD_WITHIN_API();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_vTaskDelay();
    }

#endif /* 条件：INCLUDE_vTaskDelay */
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_eTaskGetState == 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_xTaskAbortDelay == 1 ) )

    eTaskState eTaskGetState( TaskHandle_t xTask )
    {
        eTaskState eReturn;
        List_t const * pxStateList;
        List_t const * pxEventList;
        List_t const * pxDelayedList;
        List_t const * pxOverflowedDelayedList;
        const TCB_t * const pxTCB = xTask;

        traceENTER_eTaskGetState( xTask );

        configASSERT( pxTCB != NULL );

        #if ( configNUMBER_OF_CORES == 1 )
            if( pxTCB == pxCurrentTCB )
            {
                /* 调用此函数的任务正在查询自身状态。 */
                eReturn = eRunning;
            }
            else
        #endif
        {
            taskENTER_CRITICAL();
            {
                pxStateList = listLIST_ITEM_CONTAINER( &( pxTCB->xStateListItem ) );
                pxEventList = listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) );
                pxDelayedList = pxDelayedTaskList;
                pxOverflowedDelayedList = pxOverflowDelayedTaskList;
            }
            taskEXIT_CRITICAL();

            if( pxEventList == &xPendingReadyList )
            {
                /*
                 * 任务已被放入挂起就绪链表，因此无论其状态链表项当前位于
                 * 哪个链表，其状态均为 eReady。
                 */
                eReturn = eReady;
            }
            else if( ( pxStateList == pxDelayedList ) || ( pxStateList == pxOverflowedDelayedList ) )
            {
                /*
                 * 被查询的任务由某个阻塞链表引用。
                 */
                eReturn = eBlocked;
            }

            #if ( INCLUDE_vTaskSuspend == 1 )
                else if( pxStateList == &xSuspendedTaskList )
                {
                    /*
                     * 被查询的任务由挂起链表引用。
                     * 它是真正挂起，还是无限期阻塞？
                     */
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL )
                    {
                        #if ( configUSE_TASK_NOTIFICATIONS == 1 )
                        {
                            BaseType_t x;

                            /*
                             * 该任务未出现在任何 RTOS 对象的事件链表项中，但若它正在等待
                             * 任务通知而非等待对象，则仍可能处于阻塞态。若也不是，则处于挂起态。
                             */
                            eReturn = eSuspended;

                            for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                            {
                                if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                                {
                                    eReturn = eBlocked;
                                    break;
                                }
                            }
                        }
                        #else /* 条件：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
                        {
                            eReturn = eSuspended;
                        }
                        #endif /* 条件：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
                    }
                    else
                    {
                        eReturn = eBlocked;
                    }
                }
            #endif /* 条件：if ( INCLUDE_vTaskSuspend == 1 ) */

            #if ( INCLUDE_vTaskDelete == 1 )
                else if( ( pxStateList == &xTasksWaitingTermination ) || ( pxStateList == NULL ) )
                {
                    /*
                     * 被查询的任务由已删除任务链表引用，或者根本未被任何链表引用。
                     */
                    eReturn = eDeleted;
                }
            #endif

            else
            {
                #if ( configNUMBER_OF_CORES == 1 )
                {
                    /*
                     * 若任务不处于任何其他状态，则它必定处于就绪态
                     * （包括挂起就绪态）。
                     */
                    eReturn = eReady;
                }
                #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                {
                    if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                    {
                        /* 它是否正在某个核心上主动运行？ */
                        eReturn = eRunning;
                    }
                    else
                    {
                        /*
                         * 若任务不处于任何其他状态，则它必定处于就绪态
                         * （包括挂起就绪态）。
                         */
                        eReturn = eReady;
                    }
                }
                #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
            }
        }

        traceRETURN_eTaskGetState( eReturn );

        return eReturn;
    }

#endif /* 条件：INCLUDE_eTaskGetState */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

    UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;

        traceENTER_uxTaskPriorityGet( xTask );

        portBASE_TYPE_ENTER_CRITICAL();
        {
            /*
             * 若此处传入 NULL，则查询的是调用 uxTaskPriorityGet() 的任务的优先级。
             */
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            uxReturn = pxTCB->uxPriority;
        }
        portBASE_TYPE_EXIT_CRITICAL();

        traceRETURN_uxTaskPriorityGet( uxReturn );

        return uxReturn;
    }

#endif /* 条件：INCLUDE_uxTaskPriorityGet */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

    UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;
        UBaseType_t uxSavedInterruptStatus;

        traceENTER_uxTaskPriorityGetFromISR( xTask );

        /*
         * 支持中断嵌套的 RTOS 移植层具有“最高系统调用（或最高 API 调用）
         * 中断优先级”的概念。高于该优先级的中断保持永久使能，即使 RTOS 内核
         * 处于临界区，但这些中断不能调用任何 FreeRTOS API 函数。
         * 若在 FreeRTOSConfig.h 中定义了 configASSERT()，则当中断的优先级
         * 高于配置的最高系统调用优先级时，从该中断调用 FreeRTOS API 函数
         * 会导致 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 触发断言失败。
         * 只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于或（逻辑上）
         * 低于最高系统调用中断优先级的中断中调用。FreeRTOS 维护一套独立的
         * 中断安全 API，以确保中断入口尽可能快速、简单。以下链接提供更多信息
         * （尽管针对 Cortex-M）：
         * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

        /* MISRA 参考 4.7.1 [应检查返回值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
        /* coverity[misra_c_2012_directive_4_7_violation] 抑制 Coverity 的 MISRA C-2012 指令 4.7 告警 */
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            /*
             * 若此处传入 NULL，则查询的是调用任务的优先级。
             */
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            uxReturn = pxTCB->uxPriority;
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

        traceRETURN_uxTaskPriorityGetFromISR( uxReturn );

        return uxReturn;
    }

#endif /* 条件：INCLUDE_uxTaskPriorityGet */
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) )

    UBaseType_t uxTaskBasePriorityGet( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;

        traceENTER_uxTaskBasePriorityGet( xTask );

        portBASE_TYPE_ENTER_CRITICAL();
        {
            /*
             * 若此处传入 NULL，则查询的是调用 uxTaskBasePriorityGet() 的任务的
             * 基本优先级。
             */
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            uxReturn = pxTCB->uxBasePriority;
        }
        portBASE_TYPE_EXIT_CRITICAL();

        traceRETURN_uxTaskBasePriorityGet( uxReturn );

        return uxReturn;
    }

#endif /* 条件：#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) )

    UBaseType_t uxTaskBasePriorityGetFromISR( const TaskHandle_t xTask )
    {
        TCB_t const * pxTCB;
        UBaseType_t uxReturn;
        UBaseType_t uxSavedInterruptStatus;

        traceENTER_uxTaskBasePriorityGetFromISR( xTask );

        /*
         * 支持中断嵌套的 RTOS 移植层具有“最高系统调用（或最高 API 调用）
         * 中断优先级”的概念。高于该优先级的中断保持永久使能，即使 RTOS 内核
         * 处于临界区，但这些中断不能调用任何 FreeRTOS API 函数。
         * 若在 FreeRTOSConfig.h 中定义了 configASSERT()，则当中断的优先级
         * 高于配置的最高系统调用优先级时，从该中断调用 FreeRTOS API 函数
         * 会导致 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 触发断言失败。
         * 只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于或（逻辑上）
         * 低于最高系统调用中断优先级的中断中调用。FreeRTOS 维护一套独立的
         * 中断安全 API，以确保中断入口尽可能快速、简单。以下链接提供更多信息
         * （尽管针对 Cortex-M）：
         * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

        /* MISRA 参考 4.7.1 [应检查返回值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
        /* coverity[misra_c_2012_directive_4_7_violation] 抑制 Coverity 的 MISRA C-2012 指令 4.7 告警 */
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            /*
             * 若此处传入 NULL，则查询的是调用任务的基本优先级。
             */
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            uxReturn = pxTCB->uxBasePriority;
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

        traceRETURN_uxTaskBasePriorityGetFromISR( uxReturn );

        return uxReturn;
    }

#endif /* 条件：#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskPrioritySet == 1 )

    void vTaskPrioritySet( TaskHandle_t xTask,
                           UBaseType_t uxNewPriority )
    {
        TCB_t * pxTCB;
        UBaseType_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
        BaseType_t xYieldRequired = pdFALSE;

        #if ( configNUMBER_OF_CORES > 1 )
            BaseType_t xYieldForTask = pdFALSE;
        #endif

        traceENTER_vTaskPrioritySet( xTask, uxNewPriority );

        configASSERT( uxNewPriority < configMAX_PRIORITIES );

        /* 确保新优先级有效。 */
        if( uxNewPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
        {
            uxNewPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        taskENTER_CRITICAL();
        {
            /*
             * 若此处传入 NULL，则修改的是调用任务的优先级。
             */
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );

            #if ( configUSE_MUTEXES == 1 )
            {
                uxCurrentBasePriority = pxTCB->uxBasePriority;
            }
            #else
            {
                uxCurrentBasePriority = pxTCB->uxPriority;
            }
            #endif

            if( uxCurrentBasePriority != uxNewPriority )
            {
                /*
                 * 优先级变更可能已使某任务就绪，且其优先级高于正在运行的任务。
                 */
                if( uxNewPriority > uxCurrentBasePriority )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        if( pxTCB != pxCurrentTCB )
                        {
                            /*
                             * 正在提高非当前运行任务的优先级。
                             * 提高后的优先级是否高于正在运行的任务？
                             */
                            if( uxNewPriority > pxCurrentTCB->uxPriority )
                            {
                                xYieldRequired = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        else
                        {
                            /*
                             * 正在提高运行任务的优先级，但该运行任务必定已是可运行的
                             * 最高优先级任务，因此无需让出。
                             */
                        }
                    }
                    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                    {
                        /*
                         * 正在提高某任务的优先级，因此稍后让出 CPU 给该任务。
                         */
                        xYieldForTask = pdTRUE;
                    }
                    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                }
                else if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                {
                    /*
                     * 降低运行任务的优先级意味着现在可能有另一个更高优先级的任务
                     * 已就绪可执行。
                     */
                    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                        if( pxTCB->xPreemptionDisable == pdFALSE )
                    #endif
                    {
                        xYieldRequired = pdTRUE;
                    }
                }
                else
                {
                    /*
                     * 降低任何其他任务的优先级无需让出，因为运行任务的优先级
                     * 必定高于被修改任务的新优先级。
                     */
                }

                /*
                 * 在修改 uxPriority 成员之前，记住该任务可能被引用的就绪链表，
                 * 以便 taskRESET_READY_PRIORITY() 宏正确工作。
                 */
                uxPriorityUsedOnEntry = pxTCB->uxPriority;

                #if ( configUSE_MUTEXES == 1 )
                {
                    /*
                     * 仅当任务当前未使用继承优先级，或新优先级大于继承优先级时，
                     * 才更改正在使用的优先级。
                     */
                    if( ( pxTCB->uxBasePriority == pxTCB->uxPriority ) || ( uxNewPriority > pxTCB->uxPriority ) )
                    {
                        pxTCB->uxPriority = uxNewPriority;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* 无论如何都设置基本优先级。 */
                    pxTCB->uxBasePriority = uxNewPriority;
                }
                #else /* 条件：if ( configUSE_MUTEXES == 1 ) */
                {
                    pxTCB->uxPriority = uxNewPriority;
                }
                #endif /* 条件：if ( configUSE_MUTEXES == 1 ) */

                /*
                 * 仅当事件链表项的值未被用于其他目的时才重置该值。
                 */
                if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
                {
                    listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxNewPriority ) );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                /*
                 * 若任务位于阻塞链表或挂起链表中，只需修改其优先级变量即可。
                 * 但若任务位于就绪链表中，则需先将其移除，再放入与其新优先级
                 * 对应的链表。
                 */
                if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxPriorityUsedOnEntry ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
                {
                    /*
                     * 任务当前位于其就绪链表中——在加入新的就绪链表之前先将其移除。
                     * 由于处于临界区，即使调度器已挂起也可以执行此操作。
                     */
                    if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                    {
                        /*
                         * 已知任务位于其就绪链表中，因此无需再次检查，
                         * 可直接调用移植层重置宏。
                         */
                        portRESET_READY_PRIORITY( uxPriorityUsedOnEntry, uxTopReadyPriority );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    prvAddTaskToReadyList( pxTCB );
                }
                else
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    #else
                    {
                        /*
                         * 可能由于正在提高优先级，xYieldForTask 已被置为 pdTRUE。
                         * 但由于该任务不在就绪链表中，实际上无需为它让出。
                         */
                        xYieldForTask = pdFALSE;
                    }
                    #endif
                }

                if( xYieldRequired != pdFALSE )
                {
                    /* 运行任务的优先级被降低。请求该任务让出 CPU。 */
                    taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxTCB );
                }
                else
                {
                    #if ( configNUMBER_OF_CORES > 1 )
                        if( xYieldForTask != pdFALSE )
                        {
                            /*
                             * 正在提高该任务的优先级。若有运行中任务的优先级低于此任务，
                             * 则该运行任务应为此任务让出 CPU。
                             */
                            taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB );
                        }
                        else
                    #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }

                /*
                 * 在未使用移植层优化的任务选择时，
                 * 消除编译器关于未使用变量的警告。
                 */
                ( void ) uxPriorityUsedOnEntry;
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTaskPrioritySet();
    }

#endif /* 条件：INCLUDE_vTaskPrioritySet */
/*-----------------------------------------------------------*/

#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    void vTaskCoreAffinitySet( const TaskHandle_t xTask,
                               UBaseType_t uxCoreAffinityMask )
    {
        TCB_t * pxTCB;
        BaseType_t xCoreID;

        traceENTER_vTaskCoreAffinitySet( xTask, uxCoreAffinityMask );

        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            pxTCB->uxCoreAffinityMask = uxCoreAffinityMask;

            if( xSchedulerRunning != pdFALSE )
            {
                if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                {
                    xCoreID = ( BaseType_t ) pxTCB->xTaskRunState;

                    /*
                     * 若该任务无法再在其原先运行的核心上运行，则请求该核心让出。
                     */
                    if( ( uxCoreAffinityMask & ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID ) ) == 0U )
                    {
                        prvYieldCore( xCoreID );
                    }
                }
                else
                {
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        /*
                         * 当就绪任务能够运行时，SMP 调度器会请求某个核心让出。可能在请求的
                         * 核心选中该就绪任务之前，其核心亲和性已被改变。此时，由于核心亲和性
                         * 约束，原先请求的核心可能不会选中该任务，SMP 调度器必须选择新的核心
                         * 为该任务让出。
                         */
                        prvYieldForTask( xTask );
                    }
                    #else /* 条件：#if( configUSE_PREEMPTION == 1 ) */
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    #endif /* 条件：#if( configUSE_PREEMPTION == 1 ) */
                }
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTaskCoreAffinitySet();
    }
#endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    UBaseType_t vTaskCoreAffinityGet( ConstTaskHandle_t xTask )
    {
        const TCB_t * pxTCB;
        UBaseType_t uxCoreAffinityMask;

        traceENTER_vTaskCoreAffinityGet( xTask );

        portBASE_TYPE_ENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            uxCoreAffinityMask = pxTCB->uxCoreAffinityMask;
        }
        portBASE_TYPE_EXIT_CRITICAL();

        traceRETURN_vTaskCoreAffinityGet( uxCoreAffinityMask );

        return uxCoreAffinityMask;
    }
#endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */

/*-----------------------------------------------------------*/

#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )

    void vTaskPreemptionDisable( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;

        traceENTER_vTaskPreemptionDisable( xTask );

        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            pxTCB->xPreemptionDisable = pdTRUE;
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTaskPreemptionDisable();
    }

#endif /* 条件：#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 ) */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )

    void vTaskPreemptionEnable( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        BaseType_t xCoreID;

        traceENTER_vTaskPreemptionEnable( xTask );

        taskENTER_CRITICAL();
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            pxTCB->xPreemptionDisable = pdFALSE;

            if( xSchedulerRunning != pdFALSE )
            {
                if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                {
                    xCoreID = ( BaseType_t ) pxTCB->xTaskRunState;
                    prvYieldCore( xCoreID );
                }
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTaskPreemptionEnable();
    }

#endif /* 条件：#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 ) */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

    void vTaskSuspend( TaskHandle_t xTaskToSuspend )
    {
        TCB_t * pxTCB;

        traceENTER_vTaskSuspend( xTaskToSuspend );

        taskENTER_CRITICAL();
        {
            /*
             * 若此处传入 NULL，则挂起的是正在运行的任务。
             */
            pxTCB = prvGetTCBFromHandle( xTaskToSuspend );
            configASSERT( pxTCB != NULL );

            traceTASK_SUSPEND( pxTCB );

            /*
             * 将任务从就绪/延时链表中移除，并放入挂起链表。
             */
            if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
            {
                taskRESET_READY_PRIORITY( pxTCB->uxPriority );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /* 该任务是否还在等待某个事件？ */
            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
            {
                ( void ) uxListRemove( &( pxTCB->xEventListItem ) );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            vListInsertEnd( &xSuspendedTaskList, &( pxTCB->xStateListItem ) );

            #if ( configUSE_TASK_NOTIFICATIONS == 1 )
            {
                BaseType_t x;

                for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                {
                    if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                    {
                        /*
                         * 该任务原本阻塞等待任务通知，但现在已被挂起，因此未收到通知。
                         */
                        pxTCB->ucNotifyState[ x ] = taskNOT_WAITING_NOTIFICATION;
                    }
                }
            }
            #endif /* 条件：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */

            /*
             * 在 SMP 情况下，被挂起的任务可能正在另一个核心上运行。必须在退出
             * 临界区之前将该任务逐出，以确保它无法执行将其重新放回就绪/状态/
             * 事件链表的操作，从而使挂起操作失效。一旦被逐出，该任务在恢复之前
             * 不会再被调度，因为它将不再位于就绪链表中。
             */
            #if ( configNUMBER_OF_CORES > 1 )
            {
                if( xSchedulerRunning != pdFALSE )
                {
                    /*
                     * 重置下一个预期的解除阻塞时间，以防它引用的是当前已进入挂起态的
                     * 任务。
                     */
                    prvResetNextTaskUnblockTime();

                    if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                    {
                        if( pxTCB->xTaskRunState == ( BaseType_t ) portGET_CORE_ID() )
                        {
                            /* 当前任务刚被挂起。 */
                            configASSERT( uxSchedulerSuspended == 0 );
                            vTaskYieldWithinAPI();
                        }
                        else
                        {
                            prvYieldCore( pxTCB->xTaskRunState );
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
            #endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
        }
        taskEXIT_CRITICAL();

        #if ( configNUMBER_OF_CORES == 1 )
        {
            UBaseType_t uxCurrentListLength;

            if( xSchedulerRunning != pdFALSE )
            {
                /*
                 * 重置下一个预期的解除阻塞时间，以防它引用的是当前已进入挂起态的
                 * 任务。
                 */
                taskENTER_CRITICAL();
                {
                    prvResetNextTaskUnblockTime();
                }
                taskEXIT_CRITICAL();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            if( pxTCB == pxCurrentTCB )
            {
                if( xSchedulerRunning != pdFALSE )
                {
                    /* 当前任务刚被挂起。 */
                    configASSERT( uxSchedulerSuspended == 0 );
                    portYIELD_WITHIN_API();
                }
                else
                {
                    /*
                     * 调度器尚未运行，但 pxCurrentTCB 所指向的任务刚被挂起，
                     * 必须调整 pxCurrentTCB 使其指向其他任务。
                     */

                    /*
                     * 使用临时变量作为比较之前读取 volatile 变量的独立顺序点，
                     * 以确保符合 MISRA C 2012 规则 13.2。
                     */
                    uxCurrentListLength = listCURRENT_LIST_LENGTH( &xSuspendedTaskList );

                    if( uxCurrentListLength == uxCurrentNumberOfTasks )
                    {
                        /*
                         * 没有其他就绪任务，因此将 pxCurrentTCB 设回 NULL，这样下次
                         * 创建任务时，无论其相对优先级如何，pxCurrentTCB 都会指向该任务。
                         */
                        pxCurrentTCB = NULL;
                    }
                    else
                    {
                        vTaskSwitchContext();
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

        traceRETURN_vTaskSuspend();
    }

#endif /* 条件：INCLUDE_vTaskSuspend */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

    STATIC BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask )
    {
        BaseType_t xReturn = pdFALSE;
        const TCB_t * const pxTCB = xTask;

        /*
         * 会访问 xPendingReadyList，因此必须从临界区调用。
         */

        /* 检查调用任务是否被挂起没有意义。 */
        configASSERT( xTask );

        /* 被恢复的任务确实位于挂起链表中吗？ */
        if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList, &( pxTCB->xStateListItem ) ) != pdFALSE )
        {
            /* 该任务是否已在 ISR 中被恢复？ */
            if( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) == pdFALSE )
            {
                /*
                 * 它位于挂起链表中是因为处于挂起态，
                 * 还是因为无超时阻塞？
                 */
                if( listIS_CONTAINED_WITHIN( NULL, &( pxTCB->xEventListItem ) ) != pdFALSE )
                {
                    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
                    {
                        BaseType_t x;

                        /*
                         * 该任务未出现在任何 RTOS 对象的事件链表项中，但若它正在等待
                         * 任务通知而非等待对象，则仍可能处于阻塞态。若也不是，则处于挂起态。
                         */
                        xReturn = pdTRUE;

                        for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                        {
                            if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                            {
                                xReturn = pdFALSE;
                                break;
                            }
                        }
                    }
                    #else /* 条件：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
                    {
                        xReturn = pdTRUE;
                    }
                    #endif /* 条件：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
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

        return xReturn;
    }

#endif /* 条件：INCLUDE_vTaskSuspend */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

    void vTaskResume( TaskHandle_t xTaskToResume )
    {
        TCB_t * const pxTCB = xTaskToResume;

        traceENTER_vTaskResume( xTaskToResume );

        /* 恢复调用任务自身没有意义。 */
        configASSERT( xTaskToResume );

        #if ( configNUMBER_OF_CORES == 1 )

            /*
             * 该参数不能为 NULL，因为不可能恢复当前正在执行的任务。
             */
            if( ( pxTCB != pxCurrentTCB ) && ( pxTCB != NULL ) )
        #else

            /*
             * 该参数不能为 NULL，因为不可能恢复当前正在执行的任务。也不可能恢复
             * 正在另一个核心上主动运行的任务，但在此处检查其运行状态并不安全。
             * 因此进入临界区，检查该任务是否确实处于挂起态。
             */
            if( pxTCB != NULL )
        #endif
        {
            taskENTER_CRITICAL();
            {
                if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
                {
                    traceTASK_RESUME( pxTCB );

                    /*
                     * 即使调度器已挂起也可以访问就绪链表，因为此操作位于临界区内。
                     */
                    ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );

                    /*
                     * 此次让出可能不会使刚恢复的任务立即运行，但会使链表处于正确状态，
                     * 以便下次让出时生效。
                     */
                    taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            taskEXIT_CRITICAL();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_vTaskResume();
    }

#endif /* 条件：INCLUDE_vTaskSuspend */

/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

    BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume )
    {
        BaseType_t xYieldRequired = pdFALSE;
        TCB_t * const pxTCB = xTaskToResume;
        UBaseType_t uxSavedInterruptStatus;

        traceENTER_xTaskResumeFromISR( xTaskToResume );

        configASSERT( xTaskToResume );

        /*
         * 支持中断嵌套的 RTOS 移植层具有“最高系统调用（或最高 API 调用）
         * 中断优先级”的概念。高于该优先级的中断保持永久使能，即使 RTOS 内核
         * 处于临界区，但这些中断不能调用任何 FreeRTOS API 函数。
         * 若在 FreeRTOSConfig.h 中定义了 configASSERT()，则当中断的优先级
         * 高于配置的最高系统调用优先级时，从该中断调用 FreeRTOS API 函数
         * 会导致 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 触发断言失败。
         * 只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于或（逻辑上）
         * 低于最高系统调用中断优先级的中断中调用。FreeRTOS 维护一套独立的
         * 中断安全 API，以确保中断入口尽可能快速、简单。以下链接提供更多信息
         * （尽管针对 Cortex-M）：
         * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

        /* MISRA 参考 4.7.1 [应检查返回值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
        /* coverity[misra_c_2012_directive_4_7_violation] 抑制 Coverity 的 MISRA C-2012 指令 4.7 告警 */
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        {
            if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
            {
                traceTASK_RESUME_FROM_ISR( pxTCB );

                /* 检查是否可以访问就绪链表。 */
                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        /*
                         * 可以访问就绪链表，因此直接将任务从挂起链表移入就绪链表。
                         */
                        if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                        {
                            xYieldRequired = pdTRUE;

                            /*
                             * 标记存在待处理的让出请求，以防用户未使用返回值通过移植层特定的
                             * portYIELD_FROM_ISR() 从 ISR 发起上下文切换。
                             */
                            xYieldPendings[ 0 ] = pdTRUE;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

                    ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );
                }
                else
                {
                    /*
                     * 无法访问延时链表或就绪链表，因此任务暂存于挂起就绪链表中，
                     * 直到调度器恢复。
                     */
                    vListInsertEnd( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
                }

                #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PREEMPTION == 1 ) )
                {
                    prvYieldForTask( pxTCB );

                    if( xYieldPendings[ portGET_CORE_ID() ] != pdFALSE )
                    {
                        xYieldRequired = pdTRUE;
                    }
                }
                #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PREEMPTION == 1 ) ) */
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

        traceRETURN_xTaskResumeFromISR( xYieldRequired );

        return xYieldRequired;
    }

#endif /* 条件：( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) ) */
/*-----------------------------------------------------------*/

STATIC BaseType_t prvCreateIdleTasks( void )
{
    BaseType_t xReturn = pdPASS;
    BaseType_t xCoreID;
    char cIdleName[ configMAX_TASK_NAME_LEN ] = { 0 };
    TaskFunction_t pxIdleTaskFunction = NULL;
    UBaseType_t xIdleTaskNameIndex;

    /* MISRA 参考 14.3.1 [与配置相关的恒成立条件] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-143 */
    /* coverity[misra_c_2012_rule_14_3_violation] 抑制 Coverity 的 MISRA C-2012 规则 14.3 告警 */
    for( xIdleTaskNameIndex = 0U; xIdleTaskNameIndex < ( configMAX_TASK_NAME_LEN - taskRESERVED_TASK_NAME_LENGTH ); xIdleTaskNameIndex++ )
    {
        /* MISRA 参考 18.1.1 [与配置相关的边界检查] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-181 */
        /* coverity[misra_c_2012_rule_18_1_violation] 抑制 Coverity 的 MISRA C-2012 规则 18.1 告警 */
        cIdleName[ xIdleTaskNameIndex ] = configIDLE_TASK_NAME[ xIdleTaskNameIndex ];

        if( cIdleName[ xIdleTaskNameIndex ] == ( char ) 0x00 )
        {
            break;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }

    /* 确保空字符终止。 */
    cIdleName[ xIdleTaskNameIndex ] = '\0';

    /* 以最低优先级添加每个空闲任务。 */
    for( xCoreID = ( BaseType_t ) 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
    {
        #if ( configNUMBER_OF_CORES == 1 )
        {
            pxIdleTaskFunction = &prvIdleTask;
        }
        #else /* 条件：#if (  configNUMBER_OF_CORES == 1 ) */
        {
            /*
             * 在 FreeRTOS SMP 中，还会创建 configNUMBER_OF_CORES - 1 个被动空闲任务，
             * 以确保每个核心在没有其他任务可运行时都有一个空闲任务可运行。
             */
            if( xCoreID == 0 )
            {
                pxIdleTaskFunction = &prvIdleTask;
            }
            else
            {
                pxIdleTaskFunction = &prvPassiveIdleTask;
            }
        }
        #endif /* 条件：#if (  configNUMBER_OF_CORES == 1 ) */

        /*
         * 为空闲任务名称添加后缀以区分各空闲任务。
         * 单核 FreeRTOS 中只有一个空闲任务，因此不需要此函数。
         */
        #if ( configNUMBER_OF_CORES > 1 )
        {
            /*
             * 将空闲任务编号追加到名称末尾。
             *
             * 注意：空闲任务名称索引仅支持单字符核心 ID（0-9）。若核心 ID 超过 9，
             * 空闲任务名称将包含不正确的 ASCII 字符。这是可以接受的，
             * 因为任务名称主要用于调试。
             */
            cIdleName[ xIdleTaskNameIndex ] = ( char ) ( xCoreID + '0' );
            cIdleName[ xIdleTaskNameIndex + 1U ] = '\0';
        }
        #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */

        #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
        {
            StaticTask_t * pxIdleTaskTCBBuffer = NULL;
            StackType_t * pxIdleTaskStackBuffer = NULL;
            configSTACK_DEPTH_TYPE uxIdleTaskStackSize;

            /*
             * 使用用户提供的内存创建空闲任务——先获取该内存的地址，
             * 再创建空闲任务。
             */
            #if ( configNUMBER_OF_CORES == 1 )
            {
                vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &uxIdleTaskStackSize );
            }
            #else
            {
                if( xCoreID == 0 )
                {
                    vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &uxIdleTaskStackSize );
                }
                else
                {
                    vApplicationGetPassiveIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &uxIdleTaskStackSize, ( BaseType_t ) ( xCoreID - 1 ) );
                }
            }
            #endif /* 条件：if ( configNUMBER_OF_CORES == 1 ) */
            xIdleTaskHandles[ xCoreID ] = xTaskCreateStatic( pxIdleTaskFunction,
                                                             cIdleName,
                                                             uxIdleTaskStackSize,
                                                             ( void * ) NULL,
                                                             portPRIVILEGE_BIT, /* 实际效果等同于 ( tskIDLE_PRIORITY | portPRIVILEGE_BIT )，但 tskIDLE_PRIORITY 为 0。 */
                                                             pxIdleTaskStackBuffer,
                                                             pxIdleTaskTCBBuffer );

            if( xIdleTaskHandles[ xCoreID ] != NULL )
            {
                xReturn = pdPASS;
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        #else /* 条件：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
        {
            /* 正在使用动态分配的内存创建空闲任务。 */
            xReturn = xTaskCreate( pxIdleTaskFunction,
                                   cIdleName,
                                   configMINIMAL_STACK_SIZE,
                                   ( void * ) NULL,
                                   portPRIVILEGE_BIT, /* 实际效果等同于 ( tskIDLE_PRIORITY | portPRIVILEGE_BIT )，但 tskIDLE_PRIORITY 为 0。 */
                                   &xIdleTaskHandles[ xCoreID ] );
        }
        #endif /* 条件：configSUPPORT_STATIC_ALLOCATION */

        /* 若任一空闲任务创建失败，则跳出循环。 */
        if( xReturn != pdPASS )
        {
            break;
        }
        else
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                mtCOVERAGE_TEST_MARKER();
            }
            #else
            {
                /* 在 SMP 调度器运行之前，将空闲任务分配到各核心。 */
                xIdleTaskHandles[ xCoreID ]->xTaskRunState = xCoreID;
                pxCurrentTCBs[ xCoreID ] = xIdleTaskHandles[ xCoreID ];
                #if ( ( configIDLE_AFFINITY == 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
                {
                    xIdleTaskHandles[ xCoreID ]->uxCoreAffinityMask = ( ( UBaseType_t ) 1U << ( UBaseType_t ) xCoreID );
                }
                #endif /* 条件：#if ( ( configIDLE_AFFINITY == 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */
            }
            #endif /* 条件：if ( configNUMBER_OF_CORES == 1 ) */
        }
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

void vTaskStartScheduler( void )
{
    BaseType_t xReturn;

    traceENTER_vTaskStartScheduler();

    #if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 )
    {
        /*
         * 完整性检查：UBaseType_t 的位数必须大于或等于
         * confNUMBER_OF_CORES。
         */
        configASSERT( ( sizeof( UBaseType_t ) * taskBITS_PER_BYTE ) >= configNUMBER_OF_CORES );
    }
    #endif /* 条件：#if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) */

    xReturn = prvCreateIdleTasks();

    #if ( configUSE_TIMERS == 1 )
    {
        if( xReturn == pdPASS )
        {
            xReturn = xTimerCreateTimerTask();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif /* 条件：configUSE_TIMERS */

    if( xReturn == pdPASS )
    {
        /*
         * 仅当定义了用户可定义的宏 FREERTOS_TASKS_C_ADDITIONS_INIT() 时
         * 才应调用 freertos_tasks_c_additions_init()，因为该函数只调用这个宏。
         */
        #ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
        {
            freertos_tasks_c_additions_init();
        }
        #endif

        /*
         * 此处关闭中断，以确保在调用 xPortStartScheduler() 之前或期间
         * 不会发生时钟节拍。已创建任务的栈中包含一个状态字，其中中断处于
         * 开启状态，因此第一个任务开始运行时中断会自动重新使能。
         */
        portDISABLE_INTERRUPTS();

        #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        {
            /*
             * 将 C 运行库的 TLS 块切换为指向将首先运行的任务的 TLS 块。
             */
            configSET_TLS_BLOCK( pxCurrentTCB->xTLSBlock );
        }
        #endif

        xNextTaskUnblockTime = portMAX_DELAY;
        xSchedulerRunning = pdTRUE;
        xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;

        /*
         * 若定义了 configGENERATE_RUN_TIME_STATS，则必须定义以下宏，以配置
         * 用于生成运行时间计数器时基的定时器/计数器。注意：若
         * configGENERATE_RUN_TIME_STATS 设为 0 且以下代码行编译失败，
         * 请确认 FreeRTOSConfig.h 文件中未定义
         * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()。
         */
        portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();

        traceTASK_SWITCHED_IN();

        traceSTARTING_SCHEDULER( xIdleTaskHandles );

        /*
         * 时钟节拍的设置与硬件相关，因此位于移植接口中。
         */

        /*
         * xPortStartScheduler 的返回值不需要使用，因此采用 void 数据类型。
         */
        ( void ) xPortStartScheduler();

        /*
         * 大多数情况下，xPortStartScheduler() 不会返回。若返回 pdTRUE，则表示
         * 没有足够的堆内存创建空闲任务或定时器任务。若返回 pdFALSE，则表示
         * 应用程序调用了 xTaskEndScheduler()。大多数移植层不实现
         * xTaskEndScheduler()，因为没有可返回的上下文。
         */
    }
    else
    {
        /*
         * 仅当内核无法启动（因为 FreeRTOS 堆不足以创建空闲任务或定时器任务）
         * 时才会执行到此处。
         */
        configASSERT( xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY );
    }

    /*
     * 当 INCLUDE_xTaskGetIdleTaskHandle 设为 0（即 xIdleTaskHandles 不再在
     * 其他地方使用）时，防止编译器告警。
     */
    ( void ) xIdleTaskHandles;

    /*
     * OpenOCD 使用 uxTopUsedPriority 进行线程调试。防止 uxTopUsedPriority
     * 因内核不再使用而被优化掉。
     */
    ( void ) uxTopUsedPriority;

    traceRETURN_vTaskStartScheduler();
}
/*-----------------------------------------------------------*/

void vTaskEndScheduler( void )
{
    traceENTER_vTaskEndScheduler();

    #if ( INCLUDE_vTaskDelete == 1 )
    {
        BaseType_t xCoreID;

        #if ( configUSE_TIMERS == 1 )
        {
            /* 删除内核创建的定时器任务。 */
            vTaskDelete( xTimerGetTimerDaemonTaskHandle() );
        }
        #endif /* 条件：#if ( configUSE_TIMERS == 1 ) */

        /* 删除内核创建的空闲任务。 */
        for( xCoreID = 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
        {
            vTaskDelete( xIdleTaskHandles[ xCoreID ] );
        }

        /*
         * 空闲任务负责回收 xTasksWaitingTermination 链表中任务的资源。
         * 由于空闲任务现已被删除且不再运行，需要回收
         * xTasksWaitingTermination 链表中所有任务的资源。
         */
        prvCheckTasksWaitingTermination();
    }
    #endif /* 条件：#if ( INCLUDE_vTaskDelete == 1 ) */

    /*
     * 停止调度器中断并调用移植层调度器结束例程，以便必要时恢复
     * 原始 ISR。移植层必须确保中断使能位保持正确状态。
     */
    portDISABLE_INTERRUPTS();
    xSchedulerRunning = pdFALSE;

    /*
     * 此函数必须从任务中调用，调度器停止后由应用程序负责删除该任务。
     */
    vPortEndScheduler();

    traceRETURN_vTaskEndScheduler();
}
/*----------------------------------------------------------*/

void vTaskSuspendAll( void )
{
    traceENTER_vTaskSuspendAll();

    #if ( configNUMBER_OF_CORES == 1 )
    {
        /*
         * 无需临界区，因为该变量类型为 BaseType_t。每个任务维护自己的上下文，
         * 且当该变量非零时不会发生上下文切换。因此，只要从寄存器写回内存的
         * 操作是原子的，就不会有问题。
         *
         * 考虑以下场景，初始时 uxSchedulerSuspended 为零。
         *
         * 1. 将 uxSchedulerSuspended 加载到寄存器。
         * 2. 此时上下文切换使另一任务运行，另一任务使用同一变量。由于原任务
         *    尚未更新该变量，另一任务会看到该变量为零。最终原任务再次运行。
         *    **这只会发生在 uxSchedulerSuspended 再次为零的时候**。当原任务再次
         *    运行时，CPU 寄存器内容恢复到其被切换出去时的状态——因此它读入
         *    寄存器的值仍与 uxSchedulerSuspended 变量的值一致。
         *
         * 3. 递增寄存器。
         * 4. 将寄存器存入 uxSchedulerSuspended。恢复到 uxSchedulerSuspended 的值
         *    将是正确的 1，即使该变量在此期间被其他任务使用过。
         */

        /*
         * portSOFTWARE_BARRIER() 仅实现于仿真/模拟移植层中，
         * 这些移植层不具备实时行为。
         */
        portSOFTWARE_BARRIER();

        /*
         * 若 uxSchedulerSuspended 非零，则调度器处于挂起状态。
         * 使用递增操作以允许 vTaskSuspendAll() 调用嵌套。
         */
        uxSchedulerSuspended = ( UBaseType_t ) ( uxSchedulerSuspended + 1U );

        /*
         * 为移植层和优化编译器强制顺序，否则它们可能将上述递增操作
         * 放到其他位置。
         */
        portMEMORY_BARRIER();
    }
    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
    {
        UBaseType_t ulState;
        BaseType_t xCoreID;

        /* 只能从任务内部调用。 */
        portASSERT_IF_IN_ISR();

        if( xSchedulerRunning != pdFALSE )
        {
            /*
             * 对 uxSchedulerSuspended 的写操作必须同时受任务锁和 ISR 锁保护。
             * 必须在获取锁之前禁用中断，以防此任务在递增 uxSchedulerSuspended
             * 之前被中断并发生上下文切换。在释放 ISR 锁并递增
             * uxSchedulerSuspended 之后重新使能中断是安全的，因为此时已会阻止
             * 上下文切换。
             */
            ulState = portSET_INTERRUPT_MASK();

            xCoreID = ( BaseType_t ) portGET_CORE_ID();

            /* 绝不能从临界区内部调用。 */
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0 );

            /*
             * portSOFTWARE_BARRIER() 仅实现于仿真/模拟移植层中，
             * 这些移植层不具备实时行为。
             */
            portSOFTWARE_BARRIER();

            portGET_TASK_LOCK( xCoreID );

            /*
             * uxSchedulerSuspended 在 prvCheckForRunStateChange 之后递增。
             * 目的是防止在 fromISR API 读取该变量时修改它。
             */
            if( uxSchedulerSuspended == 0U )
            {
                prvCheckForRunStateChange();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /*
             * 再次查询 coreID，因为 prvCheckForRunStateChange 可能已使任务被调度到
             * 不同核心。prvCheckForRunStateChange 中会获取该核心对应的正确任务锁。
             */
            xCoreID = ( BaseType_t ) portGET_CORE_ID();

            portGET_ISR_LOCK( xCoreID );

            /*
             * 若 uxSchedulerSuspended 非零，则调度器处于挂起状态。
             * 使用递增操作以允许 vTaskSuspendAll() 调用嵌套。
             */
            ++uxSchedulerSuspended;
            portRELEASE_ISR_LOCK( xCoreID );

            portCLEAR_INTERRUPT_MASK( ulState );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

    traceRETURN_vTaskSuspendAll();
}

/*----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE != 0 )

    STATIC TickType_t prvGetExpectedIdleTime( void )
    {
        TickType_t xReturn;
        BaseType_t xHigherPriorityReadyTasks = pdFALSE;

        /*
         * xHigherPriorityReadyTasks 处理 configUSE_PREEMPTION 为 0 的情况：
         * 即使空闲任务正在运行，也可能存在优先级高于空闲任务的就绪态任务。
         */
        #if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
        {
            if( uxTopReadyPriority > tskIDLE_PRIORITY )
            {
                xHigherPriorityReadyTasks = pdTRUE;
            }
        }
        #else
        {
            const UBaseType_t uxLeastSignificantBit = ( UBaseType_t ) 0x01;

            /*
             * 使用移植层优化的任务选择时，uxTopReadyPriority 变量用作位图。
             * 若除最低位以外的其他位被置位，则存在优先级高于空闲优先级的就绪态任务。
             * 这处理了使用协作式调度器的情况。
             */
            if( uxTopReadyPriority > uxLeastSignificantBit )
            {
                xHigherPriorityReadyTasks = pdTRUE;
            }
        }
        #endif /* 条件：if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 ) */

        if( pxCurrentTCB->uxPriority > tskIDLE_PRIORITY )
        {
            xReturn = 0;
        }
        else if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > 1U )
        {
            /*
             * 还有其他空闲优先级的任务处于就绪态。若使用时间片轮转，
             * 则必须处理下一个时钟节拍中断。
             */
            xReturn = 0;
        }
        else if( xHigherPriorityReadyTasks != pdFALSE )
        {
            /*
             * 存在优先级高于空闲优先级的就绪态任务。
             * 只有 configUSE_PREEMPTION 为 0 时才会执行到此路径。
             */
            xReturn = 0;
        }
        else
        {
            xReturn = xNextTaskUnblockTime;
            xReturn -= xTickCount;
        }

        return xReturn;
    }

#endif /* 条件：configUSE_TICKLESS_IDLE */
/*----------------------------------------------------------*/

BaseType_t xTaskResumeAll( void )
{
    TCB_t * pxTCB = NULL;
    BaseType_t xAlreadyYielded = pdFALSE;

    traceENTER_xTaskResumeAll();

    #if ( configNUMBER_OF_CORES > 1 )
        if( xSchedulerRunning != pdFALSE )
    #endif
    {
        /*
         * 调度器挂起期间，ISR 可能已将某任务从事件链表中移除。若是这样，
         * 被移除的任务会被加入 xPendingReadyList。调度器恢复后，可安全地将
         * 所有挂起就绪任务从该链表移入各自对应的就绪链表。
         */
        taskENTER_CRITICAL();
        {
            const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();

            /*
             * 若 uxSchedulerSuspended 为零，则此函数与之前的 vTaskSuspendAll()
             * 调用不匹配。
             */
            configASSERT( uxSchedulerSuspended != 0U );

            uxSchedulerSuspended = ( UBaseType_t ) ( uxSchedulerSuspended - 1U );
            portRELEASE_TASK_LOCK( xCoreID );

            if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
            {
                if( uxCurrentNumberOfTasks > ( UBaseType_t ) 0U )
                {
                    /*
                     * 将挂起就绪链表中的所有就绪任务移入各自对应的就绪链表。
                     */
                    while( listLIST_IS_EMPTY( &xPendingReadyList ) == pdFALSE )
                    {
                        /* MISRA 参考 11.5.3 [void 指针赋值] */
                        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                        /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                        pxTCB = listGET_OWNER_OF_HEAD_ENTRY( ( &xPendingReadyList ) );
                        listREMOVE_ITEM( &( pxTCB->xEventListItem ) );
                        portMEMORY_BARRIER();
                        listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                        prvAddTaskToReadyList( pxTCB );

                        #if ( configNUMBER_OF_CORES == 1 )
                        {
                            /*
                             * 若被移动任务的优先级高于当前任务，则必须执行让出。
                             */
                            if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                            {
                                xYieldPendings[ xCoreID ] = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                        {
                            /*
                             * 在任务被加入 xPendingReadyList 的那一刻，所有相应任务都会让出。
                             * 若当前核心已让出，则已调用 vTaskSwitchContext()，它会把当前核心的
                             * xYieldPendings 置为 pdTRUE。
                             */
                        }
                        #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                    }

                    if( pxTCB != NULL )
                    {
                        /*
                         * 调度器挂起期间有任务被解除阻塞，这可能使下一个解除阻塞时间
                         * 未能重新计算；若如此，现在重新计算。这主要对低功耗无节拍实现
                         * 很重要，可避免不必要地退出低功耗状态。
                         */
                        prvResetNextTaskUnblockTime();
                    }

                    /*
                     * 若调度器挂起期间发生了时钟节拍，则现在应处理这些节拍。这确保
                     * 节拍计数不会漂移，且延时任务在正确的时间恢复。
                     *
                     * 从此处的任何核心调用 xTaskIncrementTick 都应是安全的，因为处于
                     * 临界区，且 xTaskIncrementTick 自身也在临界区内保护自己。
                     * 从任何核心挂起调度器都会使 xTaskIncrementTick 递增 uxPendedCounts。
                     */
                    {
                        TickType_t xPendedCounts = xPendedTicks; /* 非 volatile 副本。 */

                        if( xPendedCounts > ( TickType_t ) 0U )
                        {
                            do
                            {
                                if( xTaskIncrementTick() != pdFALSE )
                                {
                                    /*
                                     * 其他核心在 xTaskIncrementTick() 内部被中断。
                                     */
                                    xYieldPendings[ xCoreID ] = pdTRUE;
                                }
                                else
                                {
                                    mtCOVERAGE_TEST_MARKER();
                                }

                                --xPendedCounts;
                            } while( xPendedCounts > ( TickType_t ) 0U );

                            xPendedTicks = 0;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }

                    if( xYieldPendings[ xCoreID ] != pdFALSE )
                    {
                        #if ( configUSE_PREEMPTION != 0 )
                        {
                            xAlreadyYielded = pdTRUE;
                        }
                        #endif /* 条件：#if ( configUSE_PREEMPTION != 0 ) */

                        #if ( configNUMBER_OF_CORES == 1 )
                        {
                            taskYIELD_TASK_CORE_IF_USING_PREEMPTION( pxCurrentTCB );
                        }
                        #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();
    }

    traceRETURN_xTaskResumeAll( xAlreadyYielded );

    return xAlreadyYielded;
}
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount( void )
{
    TickType_t xTicks;

    traceENTER_xTaskGetTickCount();

    /* 在 16 位处理器上运行时需要临界区。 */
    portTICK_TYPE_ENTER_CRITICAL();
    {
        xTicks = xTickCount;
    }
    portTICK_TYPE_EXIT_CRITICAL();

    traceRETURN_xTaskGetTickCount( xTicks );

    return xTicks;
}
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCountFromISR( void )
{
    TickType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;

    traceENTER_xTaskGetTickCountFromISR();

    /*
     * 支持中断嵌套的 RTOS 移植层具有“最高系统调用（或最高 API 调用）
     * 中断优先级”的概念。高于该优先级的中断保持永久使能，即使 RTOS 内核
     * 处于临界区，但这些中断不能调用任何 FreeRTOS API 函数。
     * 若在 FreeRTOSConfig.h 中定义了 configASSERT()，则当中断的优先级
     * 高于配置的最高系统调用优先级时，从该中断调用 FreeRTOS API 函数
     * 会导致 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 触发断言失败。
     * 只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于或（逻辑上）
     * 低于最高系统调用中断优先级的中断中调用。FreeRTOS 维护一套独立的
     * 中断安全 API，以确保中断入口尽可能快速、简单。以下链接提供更多信息
     * （尽管针对 Cortex-M）：https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
     */
    portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uxSavedInterruptStatus = portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR();
    {
        xReturn = xTickCount;
    }
    portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xTaskGetTickCountFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t uxTaskGetNumberOfTasks( void )
{
    traceENTER_uxTaskGetNumberOfTasks();

    /*
     * 无需临界区，因为这些变量的类型为 BaseType_t。
     */
    traceRETURN_uxTaskGetNumberOfTasks( uxCurrentNumberOfTasks );

    return uxCurrentNumberOfTasks;
}
/*-----------------------------------------------------------*/

char * pcTaskGetName( TaskHandle_t xTaskToQuery )
{
    TCB_t * pxTCB;

    traceENTER_pcTaskGetName( xTaskToQuery );

    /*
     * 若此处传入 NULL，则查询的是调用任务的名称。
     */
    pxTCB = prvGetTCBFromHandle( xTaskToQuery );
    configASSERT( pxTCB != NULL );

    traceRETURN_pcTaskGetName( &( pxTCB->pcTaskName[ 0 ] ) );

    return &( pxTCB->pcTaskName[ 0 ] );
}
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetHandle == 1 )
    STATIC TCB_t * prvSearchForNameWithinSingleList( List_t * pxList,
                                                     const char pcNameToQuery[] )
    {
        TCB_t * pxReturn = NULL;
        TCB_t * pxTCB = NULL;
        UBaseType_t x;
        char cNextChar;
        BaseType_t xBreakLoop;
        const ListItem_t * pxEndMarker = listGET_END_MARKER( pxList );
        ListItem_t * pxIterator;

        /* 调用此函数时调度器处于挂起状态。 */

        if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
        {
            for( pxIterator = listGET_HEAD_ENTRY( pxList ); pxIterator != pxEndMarker; pxIterator = listGET_NEXT( pxIterator ) )
            {
                /* MISRA 参考 11.5.3 [void 指针赋值] */
                /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                pxTCB = listGET_LIST_ITEM_OWNER( pxIterator );

                /*
                 * 逐个检查名称中的字符，以确定匹配或不匹配。
                 */
                xBreakLoop = pdFALSE;

                for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
                {
                    cNextChar = pxTCB->pcTaskName[ x ];

                    if( cNextChar != pcNameToQuery[ x ] )
                    {
                        /* 字符不匹配。 */
                        xBreakLoop = pdTRUE;
                    }
                    else if( cNextChar == ( char ) 0x00 )
                    {
                        /*
                         * 两个字符串均已终止，因此必定已找到匹配。
                         */
                        pxReturn = pxTCB;
                        xBreakLoop = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    if( xBreakLoop != pdFALSE )
                    {
                        break;
                    }
                }

                if( pxReturn != NULL )
                {
                    /* 已找到句柄。 */
                    break;
                }
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        return pxReturn;
    }

#endif /* 条件：INCLUDE_xTaskGetHandle */
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetHandle == 1 )

    TaskHandle_t xTaskGetHandle( const char * pcNameToQuery )
    {
        UBaseType_t uxQueue = configMAX_PRIORITIES;
        TCB_t * pxTCB;

        traceENTER_xTaskGetHandle( pcNameToQuery );

        /* 任务名称将被截断为 configMAX_TASK_NAME_LEN - 1 字节。 */
        configASSERT( strlen( pcNameToQuery ) < configMAX_TASK_NAME_LEN );

        vTaskSuspendAll();
        {
            /* 搜索就绪链表。 */
            do
            {
                uxQueue--;
                pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) &( pxReadyTasksLists[ uxQueue ] ), pcNameToQuery );

                if( pxTCB != NULL )
                {
                    /* 已找到句柄。 */
                    break;
                }
            } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );

            /* 搜索延时链表。 */
            if( pxTCB == NULL )
            {
                pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxDelayedTaskList, pcNameToQuery );
            }

            if( pxTCB == NULL )
            {
                pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxOverflowDelayedTaskList, pcNameToQuery );
            }

            #if ( INCLUDE_vTaskSuspend == 1 )
            {
                if( pxTCB == NULL )
                {
                    /* 搜索挂起链表。 */
                    pxTCB = prvSearchForNameWithinSingleList( &xSuspendedTaskList, pcNameToQuery );
                }
            }
            #endif

            #if ( INCLUDE_vTaskDelete == 1 )
            {
                if( pxTCB == NULL )
                {
                    /* 搜索已删除任务链表。 */
                    pxTCB = prvSearchForNameWithinSingleList( &xTasksWaitingTermination, pcNameToQuery );
                }
            }
            #endif
        }
        ( void ) xTaskResumeAll();

        traceRETURN_xTaskGetHandle( pxTCB );

        return pxTCB;
    }

#endif /* 条件：INCLUDE_xTaskGetHandle */
/*-----------------------------------------------------------*/

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

    BaseType_t xTaskGetStaticBuffers( TaskHandle_t xTask,
                                      StackType_t ** ppuxStackBuffer,
                                      StaticTask_t ** ppxTaskBuffer )
    {
        BaseType_t xReturn;
        TCB_t * pxTCB;

        traceENTER_xTaskGetStaticBuffers( xTask, ppuxStackBuffer, ppxTaskBuffer );

        configASSERT( ppuxStackBuffer != NULL );
        configASSERT( ppxTaskBuffer != NULL );

        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE == 1 )
        {
            if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB )
            {
                *ppuxStackBuffer = pxTCB->pxStack;
                /* MISRA 参考 11.3.1 [非对齐访问] */
                /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
                /* coverity[misra_c_2012_rule_11_3_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.3 告警 */
                *ppxTaskBuffer = ( StaticTask_t * ) pxTCB;
                xReturn = pdTRUE;
            }
            else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
            {
                *ppuxStackBuffer = pxTCB->pxStack;
                *ppxTaskBuffer = NULL;
                xReturn = pdTRUE;
            }
            else
            {
                xReturn = pdFALSE;
            }
        }
        #else /* 条件：tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE == 1 */
        {
            *ppuxStackBuffer = pxTCB->pxStack;
            *ppxTaskBuffer = ( StaticTask_t * ) pxTCB;
            xReturn = pdTRUE;
        }
        #endif /* 条件：tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE == 1 */

        traceRETURN_xTaskGetStaticBuffers( xReturn );

        return xReturn;
    }

#endif /* 条件：configSUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    STATIC UBaseType_t prvForEachTaskInList( List_t * pxList,
                                             eTaskState eState,
                                             TaskStatusCallbackFunction_t pxCallbackFunction,
                                             void * pvCallbackContext );

/* uxTaskGetSystemState 回调上下文：当前写入 TaskStatusArray 的位置 */
    typedef struct xTASK_STATUS_ARRAY_WRITER_CONTEXT
    {
        TaskStatus_t * pxTaskStatusArray;
        UBaseType_t uxIndex;
    } TaskStatusArrayWriterContext_t;

/* uxTaskGetSystemState 的回调：将一个任务的状态写入 TaskStatusArray */
    STATIC void prvTaskStatusArrayWriter( TaskHandle_t xTask,
                                          eTaskState eState,
                                          void * pvCallbackContext )
    {
        TaskStatusArrayWriterContext_t * pxContext = ( TaskStatusArrayWriterContext_t * ) pvCallbackContext;

        vTaskGetInfo( xTask, &( pxContext->pxTaskStatusArray[ pxContext->uxIndex++ ] ), pdTRUE, eState );
    }

    STATIC void prvGetTotalRunTime( configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime )
    {
        if( pulTotalRunTime != NULL )
        {
            #if ( configGENERATE_RUN_TIME_STATS == 1 )
                #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                    portALT_GET_RUN_TIME_COUNTER_VALUE( ( *pulTotalRunTime ) );
                #else
                    *pulTotalRunTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
                #endif
            #else
                *pulTotalRunTime = 0;
            #endif /* 条件：if ( configGENERATE_RUN_TIME_STATS == 1 ) */
        }
    }

/* 对每个任务调用提供的回调函数（并传入提供的上下文）。 */
/* 调用者在使用此函数前后必须挂起调度器。 */
    STATIC UBaseType_t prvCallForEachTask( TaskStatusCallbackFunction_t pxCallbackFunction,
                                           void * pvCallbackContext )
    {
        UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;

        /* 遍历处于就绪态的每个任务。 */
        do
        {
            uxQueue--;
            uxTask = ( UBaseType_t ) ( uxTask + prvForEachTaskInList( &( pxReadyTasksLists[ uxQueue ] ), eReady, pxCallbackFunction, pvCallbackContext ) );
        } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );

        /* 遍历处于阻塞态的每个任务。 */
        uxTask = ( UBaseType_t ) ( uxTask + prvForEachTaskInList( ( List_t * ) pxDelayedTaskList, eBlocked, pxCallbackFunction, pvCallbackContext ) );
        uxTask = ( UBaseType_t ) ( uxTask + prvForEachTaskInList( ( List_t * ) pxOverflowDelayedTaskList, eBlocked, pxCallbackFunction, pvCallbackContext ) );

        #if ( INCLUDE_vTaskDelete == 1 )
        {
            /* 遍历每个已删除但尚未清理的任务。 */
            uxTask = ( UBaseType_t ) ( uxTask + prvForEachTaskInList( &xTasksWaitingTermination, eDeleted, pxCallbackFunction, pvCallbackContext ) );
        }
        #endif

        #if ( INCLUDE_vTaskSuspend == 1 )
        {
            /* 遍历处于挂起态的每个任务。 */
            uxTask = ( UBaseType_t ) ( uxTask + prvForEachTaskInList( &xSuspendedTaskList, eSuspended, pxCallbackFunction, pvCallbackContext ) );
        }
        #endif

        return uxTask;
    }

    UBaseType_t uxTaskCallForEachTask( TaskStatusCallbackFunction_t pxCallbackFunction,
                                       void * pvCallbackContext,
                                       configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime )
    {
        UBaseType_t uxTask;

        configASSERT( pxCallbackFunction != NULL );

        if( pxCallbackFunction == NULL )
        {
            return 0;
        }

        vTaskSuspendAll();
        {
            uxTask = prvCallForEachTask( pxCallbackFunction, pvCallbackContext );
            prvGetTotalRunTime( pulTotalRunTime );
        }
        ( void ) xTaskResumeAll();

        return uxTask;
    }

    UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                      const UBaseType_t uxArraySize,
                                      configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime )
    {
        UBaseType_t uxTask = 0;

        traceENTER_uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, pulTotalRunTime );

        vTaskSuspendAll();
        {
            /* 数组空间是否足以容纳系统中的每个任务？ */
            if( uxArraySize >= uxCurrentNumberOfTasks )
            {
                TaskStatusArrayWriterContext_t xContext;
                xContext.pxTaskStatusArray = pxTaskStatusArray;
                xContext.uxIndex = 0;
                uxTask = prvCallForEachTask( prvTaskStatusArrayWriter, &xContext );
                prvGetTotalRunTime( pulTotalRunTime );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        ( void ) xTaskResumeAll();

        traceRETURN_uxTaskGetSystemState( uxTask );

        return uxTask;
    }

#endif /* 条件：configUSE_TRACE_FACILITY */
/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )

    #if ( configNUMBER_OF_CORES == 1 )
        TaskHandle_t xTaskGetIdleTaskHandle( void )
        {
            traceENTER_xTaskGetIdleTaskHandle();

            /*
             * 若在调度器启动之前调用 xTaskGetIdleTaskHandle()，
             * 则 xIdleTaskHandles 为 NULL。
             */
            configASSERT( ( xIdleTaskHandles[ 0 ] != NULL ) );

            traceRETURN_xTaskGetIdleTaskHandle( xIdleTaskHandles[ 0 ] );

            return xIdleTaskHandles[ 0 ];
        }
    #endif /* 条件：if ( configNUMBER_OF_CORES == 1 ) */

    TaskHandle_t xTaskGetIdleTaskHandleForCore( BaseType_t xCoreID )
    {
        traceENTER_xTaskGetIdleTaskHandleForCore( xCoreID );

        /* 确保核心 ID 有效。 */
        configASSERT( taskVALID_CORE_ID( xCoreID ) == pdTRUE );

        /*
         * 若在调度器启动之前调用 xTaskGetIdleTaskHandle()，
         * 则 xIdleTaskHandles 为 NULL。
         */
        configASSERT( ( xIdleTaskHandles[ xCoreID ] != NULL ) );

        traceRETURN_xTaskGetIdleTaskHandleForCore( xIdleTaskHandles[ xCoreID ] );

        return xIdleTaskHandles[ xCoreID ];
    }

#endif /* 条件：INCLUDE_xTaskGetIdleTaskHandle */
/*----------------------------------------------------------*/

/*
 * 此条件编译应使用“不等于 0”而非“等于 1”的判断。这是为了确保
 * 当用户定义的低功耗模式实现要求将 configUSE_TICKLESS_IDLE 设为
 * 非 1 的值时，vTaskStepTick() 仍可用。
 */
#if ( configUSE_TICKLESS_IDLE != 0 )

    void vTaskStepTick( TickType_t xTicksToJump )
    {
        TickType_t xUpdatedTickCount;

        traceENTER_vTaskStepTick( xTicksToJump );

        /*
         * 在时钟节拍被抑制一段时间后校正节拍计数值。注意这*不会*为每个
         * 步进的节拍调用节拍钩子函数。
         */
        xUpdatedTickCount = xTickCount + xTicksToJump;
        configASSERT( xUpdatedTickCount <= xNextTaskUnblockTime );

        if( xUpdatedTickCount == xNextTaskUnblockTime )
        {
            /*
             * 安排在调度器恢复时，xTaskIncrementTick() 中的 xTickCount 达到
             * xNextTaskUnblockTime。这可确保所有延时任务在正确的时间恢复。
             */
            configASSERT( uxSchedulerSuspended != ( UBaseType_t ) 0U );
            configASSERT( xTicksToJump != ( TickType_t ) 0 );

            /* 防止时钟节拍中断同时修改 xPendedTicks。 */
            taskENTER_CRITICAL();
            {
                xPendedTicks++;
            }
            taskEXIT_CRITICAL();
            xTicksToJump--;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        xTickCount += xTicksToJump;

        traceINCREASE_TICK_COUNT( xTicksToJump );
        traceRETURN_vTaskStepTick();
    }

#endif /* 条件：configUSE_TICKLESS_IDLE */
/*----------------------------------------------------------*/

BaseType_t xTaskCatchUpTicks( TickType_t xTicksToCatchUp )
{
    BaseType_t xYieldOccurred;

    traceENTER_xTaskCatchUpTicks( xTicksToCatchUp );

    /*
     * 不得在调度器挂起时调用，因为实现依赖 xTaskResumeAll()
     * 将 xPendedTicks 递减到 0。
     */
    configASSERT( uxSchedulerSuspended == ( UBaseType_t ) 0U );

    /*
     * 使用 xPendedTicks 模拟调度器挂起期间发生的 xTicksToCatchUp 个节拍，
     * 使这些节拍在 xTaskResumeAll() 中执行。
     */
    vTaskSuspendAll();

    /* 防止时钟节拍中断同时修改 xPendedTicks。 */
    taskENTER_CRITICAL();
    {
        xPendedTicks += xTicksToCatchUp;
    }
    taskEXIT_CRITICAL();
    xYieldOccurred = xTaskResumeAll();

    traceRETURN_xTaskCatchUpTicks( xYieldOccurred );

    return xYieldOccurred;
}
/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskAbortDelay == 1 )

    BaseType_t xTaskAbortDelay( TaskHandle_t xTask )
    {
        TCB_t * pxTCB = xTask;
        BaseType_t xReturn;

        traceENTER_xTaskAbortDelay( xTask );

        configASSERT( pxTCB != NULL );

        vTaskSuspendAll();
        {
            /*
             * 只有任务确实处于阻塞态时，才能将其提前从阻塞态移除。
             */
            if( eTaskGetState( xTask ) == eBlocked )
            {
                xReturn = pdPASS;

                /*
                 * 从阻塞链表中移除对该任务的引用。由于调度器已挂起，
                 * 中断不会操作 xStateListItem。
                 */
                ( void ) uxListRemove( &( pxTCB->xStateListItem ) );

                /*
                 * 该任务是否还在等待某个事件？若是，也将其从事件链表中移除。
                 * 即使调度器已挂起，中断仍可能操作事件链表项，因此使用临界区。
                 */
                taskENTER_CRITICAL();
                {
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                    {
                        ( void ) uxListRemove( &( pxTCB->xEventListItem ) );

                        /*
                         * 这使任务知道它已被强制从阻塞态移除，因此不应重新评估
                         * 其阻塞时间并再次阻塞。
                         */
                        pxTCB->ucDelayAborted = ( uint8_t ) pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                taskEXIT_CRITICAL();

                /* 将解除阻塞的任务放入相应的就绪链表。 */
                prvAddTaskToReadyList( pxTCB );

                /*
                 * 若关闭抢占，解除任务阻塞不会导致立即上下文切换。
                 */
                #if ( configUSE_PREEMPTION == 1 )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                    {
                        /*
                         * 抢占已开启，但仅当被解除阻塞任务的优先级高于当前正在执行的
                         * 任务时才应执行上下文切换。
                         */
                        if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                        {
                            /*
                             * 将让出请求挂起，待调度器恢复时执行。
                             */
                            xYieldPendings[ 0 ] = pdTRUE;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                    {
                        taskENTER_CRITICAL();
                        {
                            prvYieldForTask( pxTCB );
                        }
                        taskEXIT_CRITICAL();
                    }
                    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                }
                #endif /* 条件：#if ( configUSE_PREEMPTION == 1 ) */
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        ( void ) xTaskResumeAll();

        traceRETURN_xTaskAbortDelay( xReturn );

        return xReturn;
    }

#endif /* 条件：INCLUDE_xTaskAbortDelay */
/*----------------------------------------------------------*/

BaseType_t xTaskIncrementTick( void )
{
    TCB_t * pxTCB;
    TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;

    traceENTER_xTaskIncrementTick();

    /*
     * 每次发生时钟节拍中断时由移植层调用。递增节拍计数，
     * 然后检查新的节拍值是否会使某些任务解除阻塞。
     */
    traceTASK_INCREMENT_TICK( xTickCount );

    /*
     * 每个内核定时器事件都应递增节拍。核 0 负责递增节拍；若调度器已挂起，
     * 则负责递增挂起节拍。若挂起节拍大于零，则调用 xTaskResumeAll 的核心
     * 负责递增节拍。
     */
    if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
    {
        /*
         * 小幅优化。此代码块内节拍计数不会变化。
         */
        const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;

        /*
         * 递增 RTOS 节拍；若计数回绕到 0，则交换延时链表与溢出延时链表。
         */
        xTickCount = xConstTickCount;

        if( xConstTickCount == ( TickType_t ) 0U )
        {
            taskSWITCH_DELAYED_LISTS();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /*
         * 检查此节拍是否使某个超时到期。任务按唤醒时间顺序存储在队列中——
         * 这意味着一旦找到阻塞时间尚未到期的任务，就无需继续查看链表后面的项。
         */
        if( xConstTickCount >= xNextTaskUnblockTime )
        {
            for( ; ; )
            {
                if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
                {
                    /*
                     * 延时链表为空。将 xNextTaskUnblockTime 设为最大可能值，使
                     * if( xTickCount >= xNextTaskUnblockTime ) 判断在下次循环时极不可能成立。
                     */
                    xNextTaskUnblockTime = portMAX_DELAY;
                    break;
                }
                else
                {
                    /*
                     * 延时链表非空，获取延时链表头项的 item 值。这是延时链表头任务
                     * 必须从阻塞态移除的时间。
                     */
                    /* MISRA 参考 11.5.3 [void 指针赋值] */
                    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                    /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                    pxTCB = listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
                    xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

                    if( xConstTickCount < xItemValue )
                    {
                        /*
                         * 尚未到解除此项阻塞的时间，但该项的值是阻塞链表头任务必须从阻塞态
                         * 移除的时间——因此将该 item 值记录到 xNextTaskUnblockTime。
                         */
                        xNextTaskUnblockTime = xItemValue;
                        break;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* 现在应将此项从阻塞态移除。 */
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );

                    /*
                     * 该任务是否还在等待某个事件？若是，将其从事件链表中移除。
                     */
                    if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                    {
                        listREMOVE_ITEM( &( pxTCB->xEventListItem ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /*
                     * 将解除阻塞的任务放入相应的就绪链表。
                     */
                    prvAddTaskToReadyList( pxTCB );

                    /*
                     * 若关闭抢占，解除任务阻塞不会导致立即上下文切换。
                     */
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        #if ( configNUMBER_OF_CORES == 1 )
                        {
                            /*
                             * 抢占已开启，但仅当被解除阻塞任务的优先级高于当前正在执行的
                             * 任务时才应执行上下文切换。相同优先级任务共享处理器时间的情况
                             * （同时开启抢占和时间片轮转时发生）在下方处理。
                             */
                            if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                            {
                                xSwitchRequired = pdTRUE;
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }
                        }
                        #else /* 条件：#if( configNUMBER_OF_CORES == 1 ) */
                        {
                            prvYieldForTask( pxTCB );
                        }
                        #endif /* 条件：#if( configNUMBER_OF_CORES == 1 ) */
                    }
                    #endif /* 条件：#if ( configUSE_PREEMPTION == 1 ) */
                }
            }
        }

        /*
         * 若抢占已开启且应用编写者未显式关闭时间片轮转，与当前运行任务
         * 优先级相同的任务将共享处理器时间（时间片）。
         */
        #if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > 1U )
                {
                    xSwitchRequired = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
            {
                BaseType_t xCoreID;

                for( xCoreID = 0; xCoreID < ( ( BaseType_t ) configNUMBER_OF_CORES ); xCoreID++ )
                {
                    if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCBs[ xCoreID ]->uxPriority ] ) ) > 1U )
                    {
                        xYieldPendings[ xCoreID ] = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
            }
            #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
        }
        #endif /* 条件：#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) */

        #if ( configUSE_TICK_HOOK == 1 )
        {
            /*
             * 防止在挂起节拍计数被递减（调度器解锁）时调用节拍钩子函数。
             */
            if( xPendedTicks == ( TickType_t ) 0 )
            {
                vApplicationTickHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif /* 条件：configUSE_TICK_HOOK */

        #if ( configUSE_PREEMPTION == 1 )
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                /* 单核情况下核心 ID 始终为 0。 */
                if( xYieldPendings[ 0 ] != pdFALSE )
                {
                    xSwitchRequired = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
            {
                BaseType_t xCoreID, xCurrentCoreID;
                xCurrentCoreID = ( BaseType_t ) portGET_CORE_ID();

                for( xCoreID = 0; xCoreID < ( BaseType_t ) configNUMBER_OF_CORES; xCoreID++ )
                {
                    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
                        if( pxCurrentTCBs[ xCoreID ]->xPreemptionDisable == pdFALSE )
                    #endif
                    {
                        if( xYieldPendings[ xCoreID ] != pdFALSE )
                        {
                            if( xCoreID == xCurrentCoreID )
                            {
                                xSwitchRequired = pdTRUE;
                            }
                            else
                            {
                                prvYieldCore( xCoreID );
                            }
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }
                    }
                }
            }
            #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
        }
        #endif /* 条件：#if ( configUSE_PREEMPTION == 1 ) */
    }
    else
    {
        xPendedTicks += 1U;

        /*
         * 即使调度器已锁定，节拍钩子函数仍会按固定间隔调用。
         */
        #if ( configUSE_TICK_HOOK == 1 )
        {
            vApplicationTickHook();
        }
        #endif
    }

    traceRETURN_xTaskIncrementTick( xSwitchRequired );

    return xSwitchRequired;
}
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

    void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
                                     TaskHookFunction_t pxHookFunction )
    {
        TCB_t * xTCB;

        traceENTER_vTaskSetApplicationTaskTag( xTask, pxHookFunction );

        /*
         * 若 xTask 为 NULL，则设置的是调用任务的任务钩子。
         */
        if( xTask == NULL )
        {
            xTCB = ( TCB_t * ) pxCurrentTCB;
        }
        else
        {
            xTCB = xTask;
        }

        /*
         * 将钩子函数保存到 TCB 中。需要临界区，因为该值可能从中断中访问。
         */
        taskENTER_CRITICAL();
        {
            xTCB->pxTaskTag = pxHookFunction;
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTaskSetApplicationTaskTag();
    }

#endif /* 条件：configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

    TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        TaskHookFunction_t xReturn;

        traceENTER_xTaskGetApplicationTaskTag( xTask );

        /* 若 xTask 为 NULL，则获取调用任务的钩子。 */
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        /*
         * 将钩子函数保存到 TCB 中。需要临界区，因为该值可能从中断中访问。
         */
        taskENTER_CRITICAL();
        {
            xReturn = pxTCB->pxTaskTag;
        }
        taskEXIT_CRITICAL();

        traceRETURN_xTaskGetApplicationTaskTag( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

    TaskHookFunction_t xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        TaskHookFunction_t xReturn;
        UBaseType_t uxSavedInterruptStatus;

        traceENTER_xTaskGetApplicationTaskTagFromISR( xTask );

        /* 若 xTask 为 NULL，则获取调用任务的钩子。 */
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        /*
         * 将钩子函数保存到 TCB 中。需要临界区，因为该值可能从中断中访问。
         */
        /* MISRA 参考 4.7.1 [应检查返回值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
        /* coverity[misra_c_2012_directive_4_7_violation] 抑制 Coverity 的 MISRA C-2012 指令 4.7 告警 */
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        {
            xReturn = pxTCB->pxTaskTag;
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

        traceRETURN_xTaskGetApplicationTaskTagFromISR( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

    BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask,
                                             void * pvParameter )
    {
        TCB_t * xTCB;
        BaseType_t xReturn;

        traceENTER_xTaskCallApplicationTaskHook( xTask, pvParameter );

        /* 若 xTask 为 NULL，则调用的是本任务自己的任务钩子。 */
        if( xTask == NULL )
        {
            xTCB = pxCurrentTCB;
        }
        else
        {
            xTCB = xTask;
        }

        if( xTCB->pxTaskTag != NULL )
        {
            xReturn = xTCB->pxTaskTag( pvParameter );
        }
        else
        {
            xReturn = pdFAIL;
        }

        traceRETURN_xTaskCallApplicationTaskHook( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES == 1 )
    void vTaskSwitchContext( void )
    {
        traceENTER_vTaskSwitchContext();

        if( uxSchedulerSuspended != ( UBaseType_t ) 0U )
        {
            /*
             * 调度器当前处于挂起状态——不允许上下文切换。
             */
            xYieldPendings[ 0 ] = pdTRUE;
        }
        else
        {
            xYieldPendings[ 0 ] = pdFALSE;
            traceTASK_SWITCHED_OUT();

            #if ( configGENERATE_RUN_TIME_STATS == 1 )
            {
                #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                    portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime[ 0 ] );
                #else
                    ulTotalRunTime[ 0 ] = portGET_RUN_TIME_COUNTER_VALUE();
                #endif

                /*
                 * 将任务已运行的时间累加到目前的累计时间中。任务开始运行的时间
                 * 保存在 ulTaskSwitchedInTime 中。注意此处没有溢出保护，因此计数值
                 * 仅在定时器溢出前有效。防止负值的保护是为了应对可疑的运行时间
                 * 统计计数器实现——这些实现由应用程序提供，而非内核。
                 */
                if( ulTotalRunTime[ 0 ] > ulTaskSwitchedInTime[ 0 ] )
                {
                    pxCurrentTCB->ulRunTimeCounter += ( ulTotalRunTime[ 0 ] - ulTaskSwitchedInTime[ 0 ] );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                ulTaskSwitchedInTime[ 0 ] = ulTotalRunTime[ 0 ];
            }
            #endif /* 条件：configGENERATE_RUN_TIME_STATS */

            /* 若已配置，则检查栈溢出。 */
            taskCHECK_FOR_STACK_OVERFLOW();

            /* 在当前运行任务被切出之前，保存其 errno。 */
            #if ( configUSE_POSIX_ERRNO == 1 )
            {
                pxCurrentTCB->iTaskErrno = FreeRTOS_errno;
            }
            #endif

            /*
             * 使用通用 C 代码或移植层优化的汇编代码选择要运行的新任务。
             */
            /* MISRA 参考 11.5.3 [void 指针赋值] */
            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
            taskSELECT_HIGHEST_PRIORITY_TASK();
            traceTASK_SWITCHED_IN();

            /*
             * 用于在任务切换后立即注入移植层特定行为的宏，例如设置栈末端
             * 观察点或重新配置 MPU。
             */
            portTASK_SWITCH_HOOK( pxCurrentTCB );

            /* 新任务切入后，更新全局 errno。 */
            #if ( configUSE_POSIX_ERRNO == 1 )
            {
                FreeRTOS_errno = pxCurrentTCB->iTaskErrno;
            }
            #endif

            #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
            {
                /*
                 * 将 C 运行库的 TLS 块切换为指向此任务专用的 TLS 块。
                 */
                configSET_TLS_BLOCK( pxCurrentTCB->xTLSBlock );
            }
            #endif
        }

        traceRETURN_vTaskSwitchContext();
    }
#else /* 条件：if ( configNUMBER_OF_CORES == 1 ) */
    void vTaskSwitchContext( BaseType_t xCoreID )
    {
        traceENTER_vTaskSwitchContext();

        /*
         * 获取两种锁：
         * - ISR 锁保护就绪链表，避免其他 ISR 和任务同时访问。
         * - 同时获取任务锁以在此暂停，以防另一核心已挂起调度器。若另一核心
         *   挂起了调度器，我们不想只是设置 xYieldPending 然后继续。只有当前
         *   核心挂起了调度器时才应这样做。
         */

        portGET_TASK_LOCK( xCoreID ); /* 必须始终先获取任务锁。 */
        portGET_ISR_LOCK( xCoreID );
        {
            /*
             * 绝不能从临界区内部调用 vTaskSwitchContext()。这对单核 FreeRTOS
             * 不一定成立，但对本 SMP 移植层成立。
             */
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0 );

            if( uxSchedulerSuspended != ( UBaseType_t ) 0U )
            {
                /*
                 * 调度器当前处于挂起状态——不允许上下文切换。
                 */
                xYieldPendings[ xCoreID ] = pdTRUE;
            }
            else
            {
                xYieldPendings[ xCoreID ] = pdFALSE;
                traceTASK_SWITCHED_OUT();

                #if ( configGENERATE_RUN_TIME_STATS == 1 )
                {
                    #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                        portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime[ xCoreID ] );
                    #else
                        ulTotalRunTime[ xCoreID ] = portGET_RUN_TIME_COUNTER_VALUE();
                    #endif

                    /*
                     * 将任务已运行的时间累加到目前的累计时间中。任务开始运行的时间
                     * 保存在 ulTaskSwitchedInTime 中。注意此处没有溢出保护，因此计数值
                     * 仅在定时器溢出前有效。防止负值的保护是为了应对可疑的运行时间
                     * 统计计数器实现——这些实现由应用程序提供，而非内核。
                     */
                    if( ulTotalRunTime[ xCoreID ] > ulTaskSwitchedInTime[ xCoreID ] )
                    {
                        pxCurrentTCBs[ xCoreID ]->ulRunTimeCounter += ( ulTotalRunTime[ xCoreID ] - ulTaskSwitchedInTime[ xCoreID ] );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    ulTaskSwitchedInTime[ xCoreID ] = ulTotalRunTime[ xCoreID ];
                }
                #endif /* 条件：configGENERATE_RUN_TIME_STATS */

                /* 若已配置，则检查栈溢出。 */
                taskCHECK_FOR_STACK_OVERFLOW();

                /* 在当前运行任务被切出之前，保存其 errno。 */
                #if ( configUSE_POSIX_ERRNO == 1 )
                {
                    pxCurrentTCBs[ xCoreID ]->iTaskErrno = FreeRTOS_errno;
                }
                #endif

                /* 选择要运行的新任务。 */
                taskSELECT_HIGHEST_PRIORITY_TASK( xCoreID );
                traceTASK_SWITCHED_IN();

                /*
                 * 用于在任务切换后立即注入移植层特定行为的宏，例如设置栈末端
                 * 观察点或重新配置 MPU。
                 */
                portTASK_SWITCH_HOOK( pxCurrentTCBs[ portGET_CORE_ID() ] );

                /* 新任务切入后，更新全局 errno。 */
                #if ( configUSE_POSIX_ERRNO == 1 )
                {
                    FreeRTOS_errno = pxCurrentTCBs[ xCoreID ]->iTaskErrno;
                }
                #endif

                #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
                {
                    /*
                     * 将 C 运行库的 TLS 块切换为指向此任务专用的 TLS 块。
                     */
                    configSET_TLS_BLOCK( pxCurrentTCBs[ xCoreID ]->xTLSBlock );
                }
                #endif
            }
        }
        portRELEASE_ISR_LOCK( xCoreID );
        portRELEASE_TASK_LOCK( xCoreID );

        traceRETURN_vTaskSwitchContext();
    }
#endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */
/*-----------------------------------------------------------*/

void vTaskPlaceOnEventList( List_t * const pxEventList,
                            const TickType_t xTicksToWait )
{
    traceENTER_vTaskPlaceOnEventList( pxEventList, xTicksToWait );

    configASSERT( pxEventList );

    /*
     * 调用此函数时必须已挂起调度器，
     * 且正在访问的队列已被锁定。
     */

    /*
     * 将 TCB 的事件链表项放入相应的事件链表。
     * 该项按优先级顺序插入链表，因此最高优先级的任务将首先被事件唤醒。
     *
     * 注意：链表按 ListItem_t.xItemValue 升序排序。
     * 通常，TCB 的 ListItem_t 成员的 xItemValue 为：
     *     xItemValue = ( configMAX_PRIORITIES - uxPriority )
     * 因此，事件链表按优先级降序排序。
     *
     * 包含该事件链表的队列已被锁定，防止中断同时访问。
     */
    vListInsert( pxEventList, &( pxCurrentTCB->xEventListItem ) );

    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );

    traceRETURN_vTaskPlaceOnEventList();
}
/*-----------------------------------------------------------*/

void vTaskPlaceOnUnorderedEventList( List_t * pxEventList,
                                     const TickType_t xItemValue,
                                     const TickType_t xTicksToWait )
{
    traceENTER_vTaskPlaceOnUnorderedEventList( pxEventList, xItemValue, xTicksToWait );

    configASSERT( pxEventList );

    /*
     * 调用此函数时必须已挂起调度器。
     * 它供事件组实现使用。
     */
    configASSERT( uxSchedulerSuspended != ( UBaseType_t ) 0U );

    /*
     * 将 item 值存入事件链表项。此处访问事件链表项是安全的，因为中断
     * 不会访问不处于阻塞态任务的事件链表项。
     */
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ), xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );

    /*
     * 将 TCB 的事件链表项放到相应事件链表的末尾。此处访问事件链表是
     * 安全的，因为这是事件组实现的一部分——中断不会直接访问事件组
     * （而是通过向任务级挂起函数调用来间接访问）。
     */
    listINSERT_END( pxEventList, &( pxCurrentTCB->xEventListItem ) );

    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );

    traceRETURN_vTaskPlaceOnUnorderedEventList();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TIMERS == 1 )

    void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
                                          TickType_t xTicksToWait,
                                          const BaseType_t xWaitIndefinitely )
    {
        traceENTER_vTaskPlaceOnEventListRestricted( pxEventList, xTicksToWait, xWaitIndefinitely );

        configASSERT( pxEventList );

        /*
         * 应用程序代码不应调用此函数，因此其名称中带有“Restricted”。
         * 它不属于公共 API，专供内核代码使用，并有特殊调用要求——
         * 调用时应已挂起调度器。
         */


        /*
         * 将 TCB 的事件链表项放入相应的事件链表。此情况下假定它是唯一
         * 将在此事件链表上等待的任务，因此可用更快的 vListInsertEnd()
         * 代替 vListInsert。
         */
        listINSERT_END( pxEventList, &( pxCurrentTCB->xEventListItem ) );

        /*
         * 若任务应无限期阻塞，则将阻塞时间设为
         * prvAddCurrentTaskToDelayedList() 函数内可识别为无限期延时的值。
         */
        if( xWaitIndefinitely != pdFALSE )
        {
            xTicksToWait = portMAX_DELAY;
        }

        traceTASK_DELAY_UNTIL( ( xTickCount + xTicksToWait ) );
        prvAddCurrentTaskToDelayedList( xTicksToWait, xWaitIndefinitely );

        traceRETURN_vTaskPlaceOnEventListRestricted();
    }

#endif /* 条件：configUSE_TIMERS */
/*-----------------------------------------------------------*/

BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList )
{
    TCB_t * pxUnblockedTCB;
    BaseType_t xReturn;

    traceENTER_xTaskRemoveFromEventList( pxEventList );

    /*
     * 此函数必须从临界区调用。
     * 也可从 ISR 内的临界区调用。
     */

    /*
     * 事件链表按优先级顺序排序，因此可移除链表中的第一项，因为它必定是
     * 最高优先级。将 TCB 从延时链表中移除，并加入就绪链表。
     *
     * 若某个事件针对的是已锁定的队列，则此函数永远不会被调用——而是
     * 改为修改队列的锁计数。这意味着此处对事件链表的独占访问是有保证的。
     *
     * 此函数假定已检查并确保 pxEventList 非空。
     */
    /* MISRA 参考 11.5.3 [void 指针赋值] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
    /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
    pxUnblockedTCB = listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
    configASSERT( pxUnblockedTCB );
    listREMOVE_ITEM( &( pxUnblockedTCB->xEventListItem ) );

    if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
    {
        listREMOVE_ITEM( &( pxUnblockedTCB->xStateListItem ) );
        prvAddTaskToReadyList( pxUnblockedTCB );

        #if ( configUSE_TICKLESS_IDLE != 0 )
        {
            /*
             * 若任务因内核对象而阻塞，则 xNextTaskUnblockTime 可能被设为该阻塞
             * 任务的超时时间。若任务因超时以外的原因被解除阻塞，
             * xNextTaskUnblockTime 通常保持不变，因为当节拍计数等于
             * xNextTaskUnblockTime 时它会自动重置为新值。但若使用无节拍空闲，
             * 尽早进入睡眠模式可能更重要——因此在此重置 xNextTaskUnblockTime，
             * 确保它尽可能早地更新。
             */
            prvResetNextTaskUnblockTime();
        }
        #endif
    }
    else
    {
        /*
         * 无法访问延时链表和就绪链表，因此将此任务暂存为挂起状态，
         * 直到调度器恢复。
         */
        listINSERT_END( &( xPendingReadyList ), &( pxUnblockedTCB->xEventListItem ) );
    }

    #if ( configNUMBER_OF_CORES == 1 )
    {
        if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
        {
            /*
             * 若从事件链表中移除的任务优先级高于调用任务，则返回 true。
             * 这使调用任务知道是否应立即强制上下文切换。
             */
            xReturn = pdTRUE;

            /*
             * 标记存在待处理的让出请求，以防用户未使用 ISR 安全 FreeRTOS 函数的
             * "xHigherPriorityTaskWoken" 参数。
             */
            xYieldPendings[ 0 ] = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
    {
        xReturn = pdFALSE;

        #if ( configUSE_PREEMPTION == 1 )
        {
            prvYieldForTask( pxUnblockedTCB );

            if( xYieldPendings[ portGET_CORE_ID() ] != pdFALSE )
            {
                xReturn = pdTRUE;
            }
        }
        #endif /* 条件：#if ( configUSE_PREEMPTION == 1 ) */
    }
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

    traceRETURN_xTaskRemoveFromEventList( xReturn );
    return xReturn;
}
/*-----------------------------------------------------------*/

void vTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem,
                                        const TickType_t xItemValue )
{
    TCB_t * pxUnblockedTCB;

    traceENTER_vTaskRemoveFromUnorderedEventList( pxEventListItem, xItemValue );

    /*
     * 调用此函数时必须已挂起调度器。
     * 它供事件标志实现使用。
     */
    configASSERT( uxSchedulerSuspended != ( UBaseType_t ) 0U );

    /* 将新的 item 值存入事件链表。 */
    listSET_LIST_ITEM_VALUE( pxEventListItem, xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );

    /*
     * 从事件标志中移除事件链表。中断不会访问事件标志。
     */
    /* MISRA 参考 11.5.3 [void 指针赋值] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
    /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
    pxUnblockedTCB = listGET_LIST_ITEM_OWNER( pxEventListItem );
    configASSERT( pxUnblockedTCB );
    listREMOVE_ITEM( pxEventListItem );

    #if ( configUSE_TICKLESS_IDLE != 0 )
    {
        /*
         * 若任务因内核对象而阻塞，则 xNextTaskUnblockTime 可能被设为该阻塞
         * 任务的超时时间。若任务因超时以外的原因被解除阻塞，
         * xNextTaskUnblockTime 通常保持不变，因为当节拍计数等于
         * xNextTaskUnblockTime 时它会自动重置为新值。但若使用无节拍空闲，
         * 尽早进入睡眠模式可能更重要——因此在此重置 xNextTaskUnblockTime，
         * 确保它尽可能早地更新。
         */
        prvResetNextTaskUnblockTime();
    }
    #endif

    /*
     * 将任务从延时链表中移除并加入就绪链表。调度器已挂起，
     * 因此中断不会访问就绪链表。
     */
    listREMOVE_ITEM( &( pxUnblockedTCB->xStateListItem ) );
    prvAddTaskToReadyList( pxUnblockedTCB );

    #if ( configNUMBER_OF_CORES == 1 )
    {
        if( pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority )
        {
            /*
             * 被解除阻塞任务的优先级高于调用任务，因此需要上下文切换。
             * 调用此函数时调度器已挂起，因此设置 xYieldPending，使调度器
             * 恢复（取消挂起）后立即发生上下文切换。
             */
            xYieldPendings[ 0 ] = pdTRUE;
        }
    }
    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
    {
        #if ( configUSE_PREEMPTION == 1 )
        {
            taskENTER_CRITICAL();
            {
                prvYieldForTask( pxUnblockedTCB );
            }
            taskEXIT_CRITICAL();
        }
        #endif
    }
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

    traceRETURN_vTaskRemoveFromUnorderedEventList();
}
/*-----------------------------------------------------------*/

void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut )
{
    traceENTER_vTaskSetTimeOutState( pxTimeOut );

    configASSERT( pxTimeOut );
    taskENTER_CRITICAL();
    {
        pxTimeOut->xOverflowCount = xNumOfOverflows;
        pxTimeOut->xTimeOnEntering = xTickCount;
    }
    taskEXIT_CRITICAL();

    traceRETURN_vTaskSetTimeOutState();
}
/*-----------------------------------------------------------*/

void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut )
{
    traceENTER_vTaskInternalSetTimeOutState( pxTimeOut );

    /* 仅供内部使用，因为它不使用临界区。 */
    pxTimeOut->xOverflowCount = xNumOfOverflows;
    pxTimeOut->xTimeOnEntering = xTickCount;

    traceRETURN_vTaskInternalSetTimeOutState();
}
/*-----------------------------------------------------------*/

BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut,
                                 TickType_t * const pxTicksToWait )
{
    BaseType_t xReturn;

    traceENTER_xTaskCheckForTimeOut( pxTimeOut, pxTicksToWait );

    configASSERT( pxTimeOut );
    configASSERT( pxTicksToWait );

    taskENTER_CRITICAL();
    {
        /* 小幅优化。此代码块内节拍计数不会变化。 */
        const TickType_t xConstTickCount = xTickCount;
        const TickType_t xElapsedTime = xConstTickCount - pxTimeOut->xTimeOnEntering;

        #if ( INCLUDE_xTaskAbortDelay == 1 )
            if( pxCurrentTCB->ucDelayAborted != ( uint8_t ) pdFALSE )
            {
                /*
                 * 延时已被中止，这与超时不同，但结果相同。
                 */
                pxCurrentTCB->ucDelayAborted = ( uint8_t ) pdFALSE;
                xReturn = pdTRUE;
            }
            else
        #endif

        #if ( INCLUDE_vTaskSuspend == 1 )
            if( *pxTicksToWait == portMAX_DELAY )
            {
                /*
                 * 若 INCLUDE_vTaskSuspend 设为 1 且指定的阻塞时间为最大阻塞时间，
                 * 则任务应无限期阻塞，因此永不超时。
                 */
                xReturn = pdFALSE;
            }
            else
        #endif

        if( ( xNumOfOverflows != pxTimeOut->xOverflowCount ) && ( xConstTickCount >= pxTimeOut->xTimeOnEntering ) )
        {
            /*
             * 节拍计数大于调用 vTaskSetTimeout() 时的时间，但自调用
             * vTaskSetTimeOut() 以来也已发生溢出。它必定已完整回绕并再次越过该值。
             * 这是自调用 vTaskSetTimeout() 之后发生的。
             */
            xReturn = pdTRUE;
            *pxTicksToWait = ( TickType_t ) 0;
        }
        else if( xElapsedTime < *pxTicksToWait )
        {
            /* 不是真正的超时。调整剩余时间参数。 */
            *pxTicksToWait -= xElapsedTime;
            vTaskInternalSetTimeOutState( pxTimeOut );
            xReturn = pdFALSE;
        }
        else
        {
            *pxTicksToWait = ( TickType_t ) 0;
            xReturn = pdTRUE;
        }
    }
    taskEXIT_CRITICAL();

    traceRETURN_xTaskCheckForTimeOut( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

void vTaskMissedYield( void )
{
    traceENTER_vTaskMissedYield();

    /* 必须从临界区内部调用。 */
    xYieldPendings[ portGET_CORE_ID() ] = pdTRUE;

    traceRETURN_vTaskMissedYield();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask )
    {
        UBaseType_t uxReturn;
        TCB_t const * pxTCB;

        traceENTER_uxTaskGetTaskNumber( xTask );

        if( xTask != NULL )
        {
            pxTCB = xTask;
            uxReturn = pxTCB->uxTaskNumber;
        }
        else
        {
            uxReturn = 0U;
        }

        traceRETURN_uxTaskGetTaskNumber( uxReturn );

        return uxReturn;
    }

#endif /* 条件：configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    void vTaskSetTaskNumber( TaskHandle_t xTask,
                             const UBaseType_t uxHandle )
    {
        TCB_t * pxTCB;

        traceENTER_vTaskSetTaskNumber( xTask, uxHandle );

        if( xTask != NULL )
        {
            pxTCB = xTask;
            pxTCB->uxTaskNumber = uxHandle;
        }

        traceRETURN_vTaskSetTaskNumber();
    }

#endif /* 条件：configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

/*
 * -----------------------------------------------------------
 * 被动空闲任务。
 * ----------------------------------------------------------
 *
 * 被动空闲任务用于 SMP 系统中的所有附加核心。必须只有 1 个活动空闲
 * 任务，其余为被动空闲任务。
 *
 * 使用 portTASK_FUNCTION() 宏以支持移植层/编译器特定的语言扩展。
 * 此函数的等价原型为：
 *
 * void prvPassiveIdleTask( void *pvParameters );
 */

#if ( configNUMBER_OF_CORES > 1 )
    STATIC portTASK_FUNCTION( prvPassiveIdleTask,
                              pvParameters )
    {
        ( void ) pvParameters;

        taskYIELD();

        for( ; configCONTROL_INFINITE_LOOP(); )
        {
            #if ( configUSE_PREEMPTION == 0 )
            {
                /*
                 * 若未使用抢占，则持续强制任务切换，以检查是否有其他任务变为可运行。
                 * 若使用抢占，则无需这样做，因为任何变为可运行的任务都会自动获得
                 * 处理器。
                 */
                taskYIELD();
            }
            #endif /* 条件：configUSE_PREEMPTION */

            #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
            {
                /*
                 * 使用抢占时，相同优先级的任务将进行时间片轮转。若有共享空闲优先级
                 * 的任务已就绪可运行，则空闲任务应在时间片结束前让出。
                 *
                 * 此处不需要临界区，因为只是读取链表，偶尔读到不正确的值也无妨。
                 * 若空闲优先级对应的就绪链表中的任务数比空闲任务数（等于配置的核心数）
                 * 多一个，则说明有空闲任务以外的任务已就绪可执行。
                 */
                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) configNUMBER_OF_CORES )
                {
                    taskYIELD();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #endif /* 条件：( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) ) */

            #if ( configUSE_PASSIVE_IDLE_HOOK == 1 )
            {
                /*
                 * 在空闲任务内部调用用户定义的函数。这使应用设计者能够添加后台
                 * 功能，而无需额外任务的开销。
                 *
                 * 此钩子用于管理核心活动，例如在核心空闲时将其禁用。
                 *
                 * 注意：vApplicationPassiveIdleHook() 在任何情况下都不得调用可能阻塞
                 * 的函数。
                 */
                vApplicationPassiveIdleHook();
            }
            #endif /* 条件：configUSE_PASSIVE_IDLE_HOOK */
        }
    }
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

/*
 * -----------------------------------------------------------
 * 空闲任务。
 * ----------------------------------------------------------
 *
 * 使用 portTASK_FUNCTION() 宏以支持移植层/编译器特定的语言扩展。
 * 此函数的等价原型为：
 *
 * void prvIdleTask( void *pvParameters );
 */

STATIC portTASK_FUNCTION( prvIdleTask,
                          pvParameters )
{
    /* 消除警告。 */
    ( void ) pvParameters;

    /**
     * 这是 RTOS 空闲任务——在调度器启动时自动创建。
     */

    /*
     * 以防具有安全上下文的任务删除自身，此时空闲任务负责删除
     * 该任务的安全上下文（若有）。
     */
    portALLOCATE_SECURE_CONTEXT( configMINIMAL_SECURE_STACK_SIZE );

    #if ( configNUMBER_OF_CORES > 1 )
    {
        /*
         * 在 SMP 中，所有核心都从空闲任务启动。此次初始让出使应用程序任务
         * 得以启动。
         */
        taskYIELD();
    }
    #endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

    for( ; configCONTROL_INFINITE_LOOP(); )
    {
        /*
         * 检查是否有任务删除了自身——若有，则由空闲任务负责释放
         * 被删除任务的 TCB 和栈。
         */
        prvCheckTasksWaitingTermination();

        #if ( configUSE_PREEMPTION == 0 )
        {
            /*
             * 若未使用抢占，则持续强制任务切换，以检查是否有其他任务变为可运行。
             * 若使用抢占，则无需这样做，因为任何变为可运行的任务都会自动获得
             * 处理器。
             */
            taskYIELD();
        }
        #endif /* 条件：configUSE_PREEMPTION */

        #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
        {
            /*
             * 使用抢占时，相同优先级的任务将进行时间片轮转。若有共享空闲优先级
             * 的任务已就绪可运行，则空闲任务应在时间片结束前让出。
             *
             * 此处不需要临界区，因为只是读取链表，偶尔读到不正确的值也无妨。
             * 若空闲优先级对应的就绪链表中的任务数比空闲任务数（等于配置的核心数）
             * 多一个，则说明有空闲任务以外的任务已就绪可执行。
             */
            if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) configNUMBER_OF_CORES )
            {
                taskYIELD();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif /* 条件：( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) ) */

        #if ( configUSE_IDLE_HOOK == 1 )
        {
            /* 在空闲任务内部调用用户定义的函数。 */
            vApplicationIdleHook();
        }
        #endif /* 条件：configUSE_IDLE_HOOK */

        /*
         * 此条件编译应使用“不等于 0”而非“等于 1”的判断。这是为了确保
         * 当用户定义的低功耗模式实现要求将 configUSE_TICKLESS_IDLE 设为
         * 非 1 的值时，仍会调用 portSUPPRESS_TICKS_AND_SLEEP()。
         */
        #if ( configUSE_TICKLESS_IDLE != 0 )
        {
            TickType_t xExpectedIdleTime;

            /*
             * 不宜在空闲任务的每次迭代中都先挂起再恢复调度器。因此，先在
             * 未挂起调度器的情况下对预期空闲时间进行初步测试。此处的结果
             * 不一定有效。
             */
            xExpectedIdleTime = prvGetExpectedIdleTime();

            if( xExpectedIdleTime >= ( TickType_t ) configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
            {
                vTaskSuspendAll();
                {
                    /*
                     * 现在调度器已挂起，可以再次采样预期空闲时间，
                     * 且此次其值可以使用。
                     */
                    configASSERT( xNextTaskUnblockTime >= xTickCount );
                    xExpectedIdleTime = prvGetExpectedIdleTime();

                    /*
                     * 若应用程序不希望调用 portSUPPRESS_TICKS_AND_SLEEP()，
                     * 可定义以下宏将 xExpectedIdleTime 设为 0。
                     */
                    configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING( xExpectedIdleTime );

                    if( xExpectedIdleTime >= ( TickType_t ) configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
                    {
                        traceLOW_POWER_IDLE_BEGIN();
                        portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime );
                        traceLOW_POWER_IDLE_END();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif /* 条件：configUSE_TICKLESS_IDLE */

        #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PASSIVE_IDLE_HOOK == 1 ) )
        {
            /*
             * 在空闲任务内部调用用户定义的函数。这使应用设计者能够添加后台
             * 功能，而无需额外任务的开销。
             *
             * 此钩子用于管理核心活动，例如在核心空闲时将其禁用。
             *
             * 注意：vApplicationPassiveIdleHook() 在任何情况下都不得调用可能阻塞
             * 的函数。
             */
            vApplicationPassiveIdleHook();
        }
        #endif /* 条件：#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PASSIVE_IDLE_HOOK == 1 ) ) */
    }
}
/*-----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE != 0 )

    eSleepModeStatus eTaskConfirmSleepModeStatus( void )
    {
        #if ( INCLUDE_vTaskSuspend == 1 )
            /* 除应用程序任务外，还存在空闲任务。 */
            const UBaseType_t uxNonApplicationTasks = configNUMBER_OF_CORES;
        #endif /* 条件：INCLUDE_vTaskSuspend */

        eSleepModeStatus eReturn = eStandardSleep;

        traceENTER_eTaskConfirmSleepModeStatus();

        /* 此函数必须从临界区调用。 */

        if( listCURRENT_LIST_LENGTH( &xPendingReadyList ) != 0U )
        {
            /* 调度器挂起期间有任务被置为就绪。 */
            eReturn = eAbortSleep;
        }
        else if( xYieldPendings[ portGET_CORE_ID() ] != pdFALSE )
        {
            /* 调度器挂起期间有让出请求被挂起。 */
            eReturn = eAbortSleep;
        }
        else if( xPendedTicks != 0U )
        {
            /*
             * 时钟节拍中断已经发生，但因调度器挂起而保持挂起状态。
             */
            eReturn = eAbortSleep;
        }

        #if ( INCLUDE_vTaskSuspend == 1 )
            else if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) == ( uxCurrentNumberOfTasks - uxNonApplicationTasks ) )
            {
                /*
                 * 若所有任务都在挂起链表中（这可能意味着它们具有无限期阻塞时间，
                 * 而非真正挂起），则可以安全地关闭所有时钟，仅等待外部中断。
                 */
                eReturn = eNoTasksWaitingTimeout;
            }
        #endif /* 条件：INCLUDE_vTaskSuspend */
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_eTaskConfirmSleepModeStatus( eReturn );

        return eReturn;
    }

#endif /* 条件：configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

    void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet,
                                            BaseType_t xIndex,
                                            void * pvValue )
    {
        TCB_t * pxTCB;

        traceENTER_vTaskSetThreadLocalStoragePointer( xTaskToSet, xIndex, pvValue );

        if( ( xIndex >= 0 ) &&
            ( xIndex < ( BaseType_t ) configNUM_THREAD_LOCAL_STORAGE_POINTERS ) )
        {
            pxTCB = prvGetTCBFromHandle( xTaskToSet );
            configASSERT( pxTCB != NULL );
            pxTCB->pvThreadLocalStoragePointers[ xIndex ] = pvValue;
        }

        traceRETURN_vTaskSetThreadLocalStoragePointer();
    }

#endif /* 条件：configNUM_THREAD_LOCAL_STORAGE_POINTERS */
/*-----------------------------------------------------------*/

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

    void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery,
                                               BaseType_t xIndex )
    {
        void * pvReturn = NULL;
        TCB_t * pxTCB;

        traceENTER_pvTaskGetThreadLocalStoragePointer( xTaskToQuery, xIndex );

        if( ( xIndex >= 0 ) &&
            ( xIndex < ( BaseType_t ) configNUM_THREAD_LOCAL_STORAGE_POINTERS ) )
        {
            pxTCB = prvGetTCBFromHandle( xTaskToQuery );
            configASSERT( pxTCB != NULL );

            pvReturn = pxTCB->pvThreadLocalStoragePointers[ xIndex ];
        }
        else
        {
            pvReturn = NULL;
        }

        traceRETURN_pvTaskGetThreadLocalStoragePointer( pvReturn );

        return pvReturn;
    }

#endif /* 条件：configNUM_THREAD_LOCAL_STORAGE_POINTERS */
/*-----------------------------------------------------------*/

#if ( portUSING_MPU_WRAPPERS == 1 )

    void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify,
                                  const MemoryRegion_t * const pxRegions )
    {
        TCB_t * pxTCB;

        traceENTER_vTaskAllocateMPURegions( xTaskToModify, pxRegions );

        /*
         * 若此处传入 NULL，则修改的是调用任务的 MPU 设置。
         */
        pxTCB = prvGetTCBFromHandle( xTaskToModify );
        configASSERT( pxTCB != NULL );

        vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), pxRegions, NULL, 0 );

        traceRETURN_vTaskAllocateMPURegions();
    }

#endif /* 条件：portUSING_MPU_WRAPPERS */
/*-----------------------------------------------------------*/

STATIC void prvInitialiseTaskLists( void )
{
    UBaseType_t uxPriority;

    for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }

    vListInitialise( &xDelayedTaskList1 );
    vListInitialise( &xDelayedTaskList2 );
    vListInitialise( &xPendingReadyList );

    #if ( INCLUDE_vTaskDelete == 1 )
    {
        vListInitialise( &xTasksWaitingTermination );
    }
    #endif /* 条件：INCLUDE_vTaskDelete */

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        vListInitialise( &xSuspendedTaskList );
    }
    #endif /* 条件：INCLUDE_vTaskSuspend */

    /*
     * 初始时 pxDelayedTaskList 使用 list1，
     * pxOverflowDelayedTaskList 使用 list2。
     */
    pxDelayedTaskList = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;
}
/*-----------------------------------------------------------*/

STATIC void prvCheckTasksWaitingTermination( void )
{
    /** 此函数由 RTOS 空闲任务调用 */

    #if ( INCLUDE_vTaskDelete == 1 )
    {
        TCB_t * pxTCB;

        /*
         * uxDeletedTasksWaitingCleanUp 用于避免在空闲任务中过于频繁地调用
         * taskENTER_CRITICAL()。
         */
        while( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
        {
            #if ( configNUMBER_OF_CORES == 1 )
            {
                taskENTER_CRITICAL();
                {
                    {
                        /* MISRA 参考 11.5.3 [void 指针赋值] */
                        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                        /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                        pxTCB = listGET_OWNER_OF_HEAD_ENTRY( ( &xTasksWaitingTermination ) );
                        ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                        --uxCurrentNumberOfTasks;
                        --uxDeletedTasksWaitingCleanUp;
                    }
                }
                taskEXIT_CRITICAL();

                prvDeleteTCB( pxTCB );
            }
            #else /* 条件：#if( configNUMBER_OF_CORES == 1 ) */
            {
                pxTCB = NULL;

                taskENTER_CRITICAL();
                {
                    /*
                     * 在 SMP 中，可能有多个空闲任务同时运行，需要检查在我们等待进入
                     * 临界区期间其他空闲任务是否已完成清理。
                     */
                    if( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
                    {
                        /* MISRA 参考 11.5.3 [void 指针赋值] */
                        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                        /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                        pxTCB = listGET_OWNER_OF_HEAD_ENTRY( ( &xTasksWaitingTermination ) );

                        if( pxTCB->xTaskRunState == taskTASK_NOT_RUNNING )
                        {
                            ( void ) uxListRemove( &( pxTCB->xStateListItem ) );
                            --uxCurrentNumberOfTasks;
                            --uxDeletedTasksWaitingCleanUp;
                        }
                        else
                        {
                            /*
                             * 待删除的 TCB 尚未被调度器切出，因此提前退出此循环，
                             * 下次再试。
                             */
                            taskEXIT_CRITICAL();
                            break;
                        }
                    }
                }
                taskEXIT_CRITICAL();

                if( pxTCB != NULL )
                {
                    prvDeleteTCB( pxTCB );
                }
            }
            #endif /* 条件：#if( configNUMBER_OF_CORES == 1 ) */
        }
    }
    #endif /* 条件：INCLUDE_vTaskDelete */
}
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    void vTaskGetInfo( TaskHandle_t xTask,
                       TaskStatus_t * pxTaskStatus,
                       BaseType_t xGetFreeStackSpace,
                       eTaskState eState )
    {
        TCB_t * pxTCB;

        traceENTER_vTaskGetInfo( xTask, pxTaskStatus, xGetFreeStackSpace, eState );

        /* 若 xTask 为 NULL，则获取调用任务的状态。 */
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        pxTaskStatus->xHandle = pxTCB;
        pxTaskStatus->pcTaskName = ( const char * ) &( pxTCB->pcTaskName[ 0 ] );
        pxTaskStatus->uxCurrentPriority = pxTCB->uxPriority;
        pxTaskStatus->pxStackBase = pxTCB->pxStack;
        #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
            pxTaskStatus->pxTopOfStack = ( StackType_t * ) pxTCB->pxTopOfStack;
            pxTaskStatus->pxEndOfStack = pxTCB->pxEndOfStack;
        #endif
        pxTaskStatus->xTaskNumber = pxTCB->uxTCBNumber;

        #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) )
        {
            pxTaskStatus->uxCoreAffinityMask = pxTCB->uxCoreAffinityMask;
        }
        #endif

        #if ( configUSE_MUTEXES == 1 )
        {
            pxTaskStatus->uxBasePriority = pxTCB->uxBasePriority;
        }
        #else
        {
            pxTaskStatus->uxBasePriority = 0;
        }
        #endif

        #if ( configGENERATE_RUN_TIME_STATS == 1 )
        {
            pxTaskStatus->ulRunTimeCounter = ulTaskGetRunTimeCounter( xTask );
        }
        #else
        {
            pxTaskStatus->ulRunTimeCounter = ( configRUN_TIME_COUNTER_TYPE ) 0;
        }
        #endif

        /*
         * 获取任务状态稍显繁琐，因此仅当传入此函数的 eState 值为 eInvalid
         * 时才执行；否则直接将状态设为传入的值。
         */
        if( eState != eInvalid )
        {
            if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
            {
                pxTaskStatus->eCurrentState = eRunning;
            }
            else
            {
                pxTaskStatus->eCurrentState = eState;

                #if ( INCLUDE_vTaskSuspend == 1 )
                {
                    /*
                     * 若任务位于挂起链表中，则它可能实际上只是无限期阻塞——
                     * 因此严格来说应报告为处于阻塞态。
                     */
                    if( eState == eSuspended )
                    {
                        vTaskSuspendAll();
                        {
                            if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
                            {
                                pxTaskStatus->eCurrentState = eBlocked;
                            }
                            else
                            {
                                #if ( configUSE_TASK_NOTIFICATIONS == 1 )
                                {
                                    BaseType_t x;

                                    /*
                                     * 该任务未出现在任何 RTOS 对象的事件链表项中，但若它正在等待
                                     * 任务通知而非等待对象，则仍可能处于阻塞态。若也不是，则处于挂起态。
                                     */
                                    for( x = ( BaseType_t ) 0; x < ( BaseType_t ) configTASK_NOTIFICATION_ARRAY_ENTRIES; x++ )
                                    {
                                        if( pxTCB->ucNotifyState[ x ] == taskWAITING_NOTIFICATION )
                                        {
                                            pxTaskStatus->eCurrentState = eBlocked;
                                            break;
                                        }
                                    }
                                }
                                #endif /* 条件：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
                            }
                        }
                        ( void ) xTaskResumeAll();
                    }
                }
                #endif /* 条件：INCLUDE_vTaskSuspend */

                /*
                 * 任务可以同时位于挂起就绪链表和其他状态链表中。无论任务位于
                 * 哪个状态链表，这些任务都处于就绪态。
                 */
                taskENTER_CRITICAL();
                {
                    if( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) != pdFALSE )
                    {
                        pxTaskStatus->eCurrentState = eReady;
                    }
                }
                taskEXIT_CRITICAL();
            }
        }
        else
        {
            pxTaskStatus->eCurrentState = eTaskGetState( pxTCB );
        }

        /*
         * 获取栈空间需要一些时间，因此提供 xGetFreeStackSpace 参数
         * 以允许跳过该操作。
         */
        if( xGetFreeStackSpace != pdFALSE )
        {
            #if ( portSTACK_GROWTH > 0 )
            {
                pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxEndOfStack );
            }
            #else
            {
                pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxStack );
            }
            #endif
        }
        else
        {
            pxTaskStatus->usStackHighWaterMark = 0;
        }

        traceRETURN_vTaskGetInfo();
    }

#endif /* 条件：configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

    STATIC UBaseType_t prvForEachTaskInList( List_t * pxList,
                                             eTaskState eState,
                                             TaskStatusCallbackFunction_t pxCallbackFunction,
                                             void * pvCallbackContext )
    {
        UBaseType_t uxTask = 0;
        const ListItem_t * pxEndMarker = listGET_END_MARKER( pxList );
        ListItem_t * pxIterator;
        TCB_t * pxTCB = NULL;

        if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
        {
            /* 将 pxList 引用的每个任务句柄交给回调函数。 */
            for( pxIterator = listGET_HEAD_ENTRY( pxList ); pxIterator != pxEndMarker; pxIterator = listGET_NEXT( pxIterator ) )
            {
                /* MISRA 参考 11.5.3 [void 指针赋值] */
                /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
                pxTCB = listGET_LIST_ITEM_OWNER( pxIterator );

                pxCallbackFunction( ( TaskHandle_t ) pxTCB, eState, pvCallbackContext );
                uxTask++;
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        return uxTask;
    }

#endif /* 条件：configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )

    STATIC configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte )
    {
        configSTACK_DEPTH_TYPE uxCount = 0U;

        while( *pucStackByte == ( uint8_t ) tskSTACK_FILL_BYTE )
        {
            pucStackByte -= portSTACK_GROWTH;
            uxCount++;
        }

        uxCount /= ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t );

        return uxCount;
    }

#endif /* 条件：( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )

/*
 * uxTaskGetStackHighWaterMark() 与 uxTaskGetStackHighWaterMark2() 除返回
 * 类型外完全相同。使用 configSTACK_DEPTH_TYPE 允许用户确定返回类型。
 * 它解决了 8 位类型上值溢出的问题，同时不破坏期望 8 位返回类型的
 * 应用程序的向后兼容性。
 */
    configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        uint8_t * pucEndOfStack;
        configSTACK_DEPTH_TYPE uxReturn;

        traceENTER_uxTaskGetStackHighWaterMark2( xTask );

        /*
         * uxTaskGetStackHighWaterMark() 与 uxTaskGetStackHighWaterMark2() 除返回
         * 类型外完全相同。使用 configSTACK_DEPTH_TYPE 允许用户确定返回类型。
         * 它解决了 8 位类型上值溢出的问题，同时不破坏期望 8 位返回类型的
         * 应用程序的向后兼容性。
         */

        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        #if portSTACK_GROWTH < 0
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
        }
        #else
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
        }
        #endif

        uxReturn = prvTaskCheckFreeStackSpace( pucEndOfStack );

        traceRETURN_uxTaskGetStackHighWaterMark2( uxReturn );

        return uxReturn;
    }

#endif /* 条件：INCLUDE_uxTaskGetStackHighWaterMark2 */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

    UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        uint8_t * pucEndOfStack;
        UBaseType_t uxReturn;

        traceENTER_uxTaskGetStackHighWaterMark( xTask );

        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        #if portSTACK_GROWTH < 0
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
        }
        #else
        {
            pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
        }
        #endif

        uxReturn = ( UBaseType_t ) prvTaskCheckFreeStackSpace( pucEndOfStack );

        traceRETURN_uxTaskGetStackHighWaterMark( uxReturn );

        return uxReturn;
    }

#endif /* 条件：INCLUDE_uxTaskGetStackHighWaterMark */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelete == 1 )

    STATIC void prvDeleteTCB( TCB_t * pxTCB )
    {
        /*
         * 此调用专为 TriCore 移植层所需。它必须位于 vPortFree() 调用之前。
         * 该调用也供希望静态分配和清理 RAM 的移植层/示例使用。
         */
        portCLEAN_UP_TCB( pxTCB );

        #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        {
            /* 释放为任务 TLS 块分配的内存。 */
            configDEINIT_TLS_BLOCK( pxTCB->xTLSBlock );
        }
        #endif

        #if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) && ( portUSING_MPU_WRAPPERS == 0 ) )
        {
            /*
             * 该任务只能是通过动态方式分配的——同时释放栈和 TCB。
             */
            vPortFreeStack( pxTCB->pxStack );
            vPortFree( pxTCB );
        }
        #elif ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
        {
            /*
             * 任务可能是静态或动态分配的，因此在释放内存之前先检查
             * 哪些部分是静态分配的。
             */
            if( pxTCB->ucStaticallyAllocated == tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB )
            {
                /*
                 * 栈和 TCB 均为动态分配，因此两者都必须释放。
                 */
                vPortFreeStack( pxTCB->pxStack );
                vPortFree( pxTCB );
            }
            else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
            {
                /*
                 * 仅栈为静态分配，因此只需释放 TCB 这一块内存。
                 */
                vPortFree( pxTCB );
            }
            else
            {
                /*
                 * 栈和 TCB 都不是动态分配的，因此无需释放任何内存。
                 */
                configASSERT( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB );
                mtCOVERAGE_TEST_MARKER();
            }
        }
        #endif /* 条件：configSUPPORT_DYNAMIC_ALLOCATION */
    }

#endif /* 条件：INCLUDE_vTaskDelete */
/*-----------------------------------------------------------*/

STATIC void prvResetNextTaskUnblockTime( void )
{
    if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
    {
        /*
         * 新的当前延时链表为空。将 xNextTaskUnblockTime 设为最大可能值，使
         * if( xTickCount >= xNextTaskUnblockTime ) 判断在延时链表中有项之前
         * 极不可能成立。
         */
        xNextTaskUnblockTime = portMAX_DELAY;
    }
    else
    {
        /*
         * 新的当前延时链表非空，获取延时链表头项的 item 值。这是延时链表
         * 头任务应从阻塞态移除的时间。
         */
        xNextTaskUnblockTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxDelayedTaskList );
    }
}
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_RECURSIVE_MUTEXES == 1 ) ) || ( configNUMBER_OF_CORES > 1 )

    #if ( configNUMBER_OF_CORES == 1 )
        TaskHandle_t xTaskGetCurrentTaskHandle( void )
        {
            TaskHandle_t xReturn;

            traceENTER_xTaskGetCurrentTaskHandle();

            /*
             * 无需临界区，因为此函数不是从中断中调用，且对于任何单个执行线程，
             * 当前 TCB 始终相同。
             */
            xReturn = pxCurrentTCB;

            traceRETURN_xTaskGetCurrentTaskHandle( xReturn );

            return xReturn;
        }
    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
        TaskHandle_t xTaskGetCurrentTaskHandle( void )
        {
            TaskHandle_t xReturn;
            UBaseType_t uxSavedInterruptStatus;

            traceENTER_xTaskGetCurrentTaskHandle();

            uxSavedInterruptStatus = portSET_INTERRUPT_MASK();
            {
                xReturn = pxCurrentTCBs[ portGET_CORE_ID() ];
            }
            portCLEAR_INTERRUPT_MASK( uxSavedInterruptStatus );

            traceRETURN_xTaskGetCurrentTaskHandle( xReturn );

            return xReturn;
        }
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

    TaskHandle_t xTaskGetCurrentTaskHandleForCore( BaseType_t xCoreID )
    {
        TaskHandle_t xReturn = NULL;

        traceENTER_xTaskGetCurrentTaskHandleForCore( xCoreID );

        if( taskVALID_CORE_ID( xCoreID ) != pdFALSE )
        {
            #if ( configNUMBER_OF_CORES == 1 )
                xReturn = pxCurrentTCB;
            #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                xReturn = pxCurrentTCBs[ xCoreID ];
            #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
        }

        traceRETURN_xTaskGetCurrentTaskHandleForCore( xReturn );

        return xReturn;
    }

#endif /* 条件：( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_RECURSIVE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )

    BaseType_t xTaskGetSchedulerState( void )
    {
        BaseType_t xReturn;

        traceENTER_xTaskGetSchedulerState();

        if( xSchedulerRunning == pdFALSE )
        {
            xReturn = taskSCHEDULER_NOT_STARTED;
        }
        else
        {
            #if ( configNUMBER_OF_CORES > 1 )
                taskENTER_CRITICAL();
            #endif
            {
                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    xReturn = taskSCHEDULER_RUNNING;
                }
                else
                {
                    xReturn = taskSCHEDULER_SUSPENDED;
                }
            }
            #if ( configNUMBER_OF_CORES > 1 )
                taskEXIT_CRITICAL();
            #endif
        }

        traceRETURN_xTaskGetSchedulerState( xReturn );

        return xReturn;
    }

#endif /* 条件：( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

    BaseType_t xTaskPriorityInherit( TaskHandle_t const pxMutexHolder )
    {
        TCB_t * const pxMutexHolderTCB = pxMutexHolder;
        BaseType_t xReturn = pdFALSE;

        traceENTER_xTaskPriorityInherit( pxMutexHolder );

        /*
         * 若互斥量被中断获取，则互斥量持有者为 NULL。
         * 此情况下不应用优先级继承。
         */
        if( pxMutexHolder != NULL )
        {
            /*
             * 若互斥量持有者的优先级低于试图获取该互斥量的任务，则持有者
             * 将临时继承该试图获取互斥量任务的优先级。
             */
            if( pxMutexHolderTCB->uxPriority < pxCurrentTCB->uxPriority )
            {
                /*
                 * 根据新优先级调整互斥量持有者的状态。仅当事件链表项的值未被用于
                 * 其他目的时才重置该值。
                 */
                if( ( listGET_LIST_ITEM_VALUE( &( pxMutexHolderTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
                {
                    listSET_LIST_ITEM_VALUE( &( pxMutexHolderTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

                /*
                 * 若被修改的任务处于就绪态，则需要将其移入新的链表。
                 */
                if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxMutexHolderTCB->uxPriority ] ), &( pxMutexHolderTCB->xStateListItem ) ) != pdFALSE )
                {
                    if( uxListRemove( &( pxMutexHolderTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                    {
                        /*
                         * 已知任务位于其就绪链表中，因此无需再次检查，
                         * 可直接调用移植层重置宏。
                         */
                        portRESET_READY_PRIORITY( pxMutexHolderTCB->uxPriority, uxTopReadyPriority );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* 在移入新链表之前继承优先级。 */
                    pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
                    prvAddTaskToReadyList( pxMutexHolderTCB );
                    #if ( configNUMBER_OF_CORES > 1 )
                    {
                        /*
                         * 该任务的优先级被提高。若它未在运行，则为它让出 CPU。
                         */
                        if( taskTASK_IS_RUNNING( pxMutexHolderTCB ) != pdTRUE )
                        {
                            prvYieldForTask( pxMutexHolderTCB );
                        }
                    }
                    #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */
                }
                else
                {
                    /* 仅继承优先级。 */
                    pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
                }

                traceTASK_PRIORITY_INHERIT( pxMutexHolderTCB, pxCurrentTCB->uxPriority );

                /* 已发生优先级继承。 */
                xReturn = pdTRUE;
            }
            else
            {
                if( pxMutexHolderTCB->uxBasePriority < pxCurrentTCB->uxPriority )
                {
                    /*
                     * 互斥量持有者的基本优先级低于试图获取该互斥量的任务的优先级，
                     * 但互斥量持有者的当前优先级不低于试图获取该互斥量任务的优先级。
                     * 因此，互斥量持有者必定已继承了某个优先级；若并非如此，
                     * 则会发生继承。
                     */
                    xReturn = pdTRUE;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTaskPriorityInherit( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

    BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder )
    {
        TCB_t * const pxTCB = pxMutexHolder;
        BaseType_t xReturn = pdFALSE;

        traceENTER_xTaskPriorityDisinherit( pxMutexHolder );

        if( pxMutexHolder != NULL )
        {
            /*
             * 任务只有持有互斥量时才能具有继承优先级。若互斥量由任务持有，
             * 则不能从中断中释放；若由持有任务释放，则该任务必定是运行态任务。
             */
            configASSERT( pxTCB == pxCurrentTCB );
            configASSERT( pxTCB->uxMutexesHeld );
            ( pxTCB->uxMutexesHeld )--;

            /*
             * 互斥量持有者是否继承了其他任务的优先级？
             */
            if( pxTCB->uxPriority != pxTCB->uxBasePriority )
            {
                /* 仅当未持有其他互斥量时才取消继承。 */
                if( pxTCB->uxMutexesHeld == ( UBaseType_t ) 0 )
                {
                    /*
                     * 任务只有持有互斥量时才能具有继承优先级。若互斥量由任务持有，
                     * 则不能从中断中释放；若由持有任务释放，则该任务必定是运行态任务。
                     * 将持有任务从就绪链表中移除。
                     */
                    if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                    {
                        portRESET_READY_PRIORITY( pxTCB->uxPriority, uxTopReadyPriority );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /*
                     * 在将任务加入新的就绪链表之前取消优先级继承。
                     */
                    traceTASK_PRIORITY_DISINHERIT( pxTCB, pxTCB->uxBasePriority );
                    pxTCB->uxPriority = pxTCB->uxBasePriority;

                    /*
                     * 重置事件链表项的值。若此任务正在运行，该值不可能被用于其他目的；
                     * 且它必定正在运行才能归还互斥量。
                     */
                    listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxTCB->uxPriority );
                    prvAddTaskToReadyList( pxTCB );
                    #if ( configNUMBER_OF_CORES > 1 )
                    {
                        /*
                         * 该任务的优先级被降低。让出该任务正在运行的核心。
                         */
                        if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                        {
                            prvYieldCore( pxTCB->xTaskRunState );
                        }
                    }
                    #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */

                    /*
                     * 返回 true 表示需要上下文切换。实际仅在某一边界情况下才需要：
                     * 任务持有多个互斥量，且互斥量的释放顺序与获取顺序不同。
                     * 若归还第一个互斥量时未发生上下文切换（即使有任务在等待它），
                     * 则归还最后一个互斥量时应发生上下文切换，无论是否有任务在等待。
                     */
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
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTaskPriorityDisinherit( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

    void vTaskPriorityDisinheritAfterTimeout( TaskHandle_t const pxMutexHolder,
                                              UBaseType_t uxHighestPriorityWaitingTask )
    {
        TCB_t * const pxTCB = pxMutexHolder;
        UBaseType_t uxPriorityUsedOnEntry, uxPriorityToUse;
        const UBaseType_t uxOnlyOneMutexHeld = ( UBaseType_t ) 1;

        traceENTER_vTaskPriorityDisinheritAfterTimeout( pxMutexHolder, uxHighestPriorityWaitingTask );

        if( pxMutexHolder != NULL )
        {
            /*
             * 若 pxMutexHolder 非 NULL，则持有者必须至少持有一个互斥量。
             */
            configASSERT( pxTCB->uxMutexesHeld );

            /*
             * 确定应将互斥量持有任务的优先级设为多少。该值取持有任务的
             * 基本优先级与等待获取该互斥量的最高优先级任务的优先级中的较大者。
             */
            if( pxTCB->uxBasePriority < uxHighestPriorityWaitingTask )
            {
                uxPriorityToUse = uxHighestPriorityWaitingTask;
            }
            else
            {
                uxPriorityToUse = pxTCB->uxBasePriority;
            }

            /* 是否需要更改优先级？ */
            if( pxTCB->uxPriority != uxPriorityToUse )
            {
                /*
                 * 仅当未持有其他互斥量时才取消继承。这是优先级继承实现中的
                 * 简化处理。若持有互斥量的任务还持有其他互斥量，则优先级继承
                 * 可能是由其他互斥量引起的。
                 */
                if( pxTCB->uxMutexesHeld == uxOnlyOneMutexHeld )
                {
                    /*
                     * 若任务因已持有其试图获取的互斥量而超时，则它不可能继承了自己的
                     * 优先级。
                     */
                    configASSERT( pxTCB != pxCurrentTCB );

                    /*
                     * 取消优先级继承，并记住此前的优先级，以便确定目标任务的状态。
                     */
                    traceTASK_PRIORITY_DISINHERIT( pxTCB, uxPriorityToUse );
                    uxPriorityUsedOnEntry = pxTCB->uxPriority;
                    pxTCB->uxPriority = uxPriorityToUse;

                    /*
                     * 仅当事件链表项的值未被用于其他目的时才重置该值。
                     */
                    if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == ( ( TickType_t ) 0U ) )
                    {
                        listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriorityToUse );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /*
                     * 若运行任务不是持有互斥量的任务，则持有互斥量的任务可能处于就绪态、
                     * 阻塞态或挂起态。仅当任务处于就绪态时才将其从当前状态链表中移除，
                     * 因为任务优先级将要改变，而每个优先级对应一个就绪链表。
                     */
                    if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxPriorityUsedOnEntry ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
                    {
                        if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
                        {
                            /*
                             * 已知任务位于其就绪链表中，因此无需再次检查，
                             * 可直接调用移植层重置宏。
                             */
                            portRESET_READY_PRIORITY( uxPriorityUsedOnEntry, uxTopReadyPriority );
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }

                        prvAddTaskToReadyList( pxTCB );
                        #if ( configNUMBER_OF_CORES > 1 )
                        {
                            /*
                             * 该任务的优先级被降低。让出该任务正在运行的核心。
                             */
                            if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
                            {
                                prvYieldCore( pxTCB->xTaskRunState );
                            }
                        }
                        #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */
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

        traceRETURN_vTaskPriorityDisinheritAfterTimeout();
    }

#endif /* 条件：configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

/*
 * 若不在临界区中，则立即让出。否则将 xYieldPendings 置为 true，
 * 等到退出临界区后再让出。
 */
    void vTaskYieldWithinAPI( void )
    {
        UBaseType_t ulState;

        traceENTER_vTaskYieldWithinAPI();

        ulState = portSET_INTERRUPT_MASK();
        {
            const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();

            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
            {
                portYIELD();
            }
            else
            {
                xYieldPendings[ xCoreID ] = pdTRUE;
            }
        }
        portCLEAR_INTERRUPT_MASK( ulState );

        traceRETURN_vTaskYieldWithinAPI();
    }
#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

/*-----------------------------------------------------------*/

#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) && ( configNUMBER_OF_CORES == 1 ) )

    void vTaskEnterCritical( void )
    {
        traceENTER_vTaskEnterCritical();

        portDISABLE_INTERRUPTS();

        if( xSchedulerRunning != pdFALSE )
        {
            ( pxCurrentTCB->uxCriticalNesting )++;

            /*
             * 这不是进入临界区函数的中断安全版本，因此若在中断上下文中调用
             * 则触发 assert()。只有以 "FromISR" 结尾的 API 函数才能在中断中使用。
             * 仅当临界区嵌套计数为 1 时才断言，以防 assert 函数本身也使用临界区
             * 而导致递归调用。
             */
            if( pxCurrentTCB->uxCriticalNesting == 1U )
            {
                portASSERT_IF_IN_ISR();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_vTaskEnterCritical();
    }

#endif /* 条件：#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) && ( configNUMBER_OF_CORES == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    void vTaskEnterCritical( void )
    {
        traceENTER_vTaskEnterCritical();

        portDISABLE_INTERRUPTS();
        {
            const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();

            if( xSchedulerRunning != pdFALSE )
            {
                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
                {
                    portGET_TASK_LOCK( xCoreID );
                    portGET_ISR_LOCK( xCoreID );
                }

                portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID );

                /*
                 * 这不是进入临界区函数的中断安全版本，因此若在中断上下文中调用
                 * 则触发 assert()。只有以 "FromISR" 结尾的 API 函数才能在中断中使用。
                 * 仅当临界区嵌套计数为 1 时才断言，以防 assert 函数本身也使用临界区
                 * 而导致递归调用。
                 */
                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 1U )
                {
                    portASSERT_IF_IN_ISR();

                    if( uxSchedulerSuspended == 0U )
                    {
                        /*
                         * 唯一可能出问题的情况是：在上下文切换之前调用此函数，而在
                         * pxCurrentTCB 改变之后才调用 vTaskExitCritical()。因此不应在
                         * vTaskSwitchContext() 内使用此函数。
                         */
                        prvCheckForRunStateChange();
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        traceRETURN_vTaskEnterCritical();
    }

#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    UBaseType_t vTaskEnterCriticalFromISR( void )
    {
        UBaseType_t uxSavedInterruptStatus = 0;
        const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();

        traceENTER_vTaskEnterCriticalFromISR();

        if( xSchedulerRunning != pdFALSE )
        {
            uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
            {
                portGET_ISR_LOCK( xCoreID );
            }

            portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_vTaskEnterCriticalFromISR( uxSavedInterruptStatus );

        return uxSavedInterruptStatus;
    }

#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
/*-----------------------------------------------------------*/

#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) && ( configNUMBER_OF_CORES == 1 ) )

    void vTaskExitCritical( void )
    {
        traceENTER_vTaskExitCritical();

        if( xSchedulerRunning != pdFALSE )
        {
            /*
             * 若 pxCurrentTCB->uxCriticalNesting 为零，则此函数与之前的
             * vTaskEnterCritical() 调用不匹配。
             */
            configASSERT( pxCurrentTCB->uxCriticalNesting > 0U );

            /*
             * 不应在 ISR 中调用此函数。从 ISR 退出临界区请使用
             * vTaskExitCriticalFromISR。
             */
            portASSERT_IF_IN_ISR();

            if( pxCurrentTCB->uxCriticalNesting > 0U )
            {
                ( pxCurrentTCB->uxCriticalNesting )--;

                if( pxCurrentTCB->uxCriticalNesting == 0U )
                {
                    portENABLE_INTERRUPTS();
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

        traceRETURN_vTaskExitCritical();
    }

#endif /* 条件：#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) && ( configNUMBER_OF_CORES == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    void vTaskExitCritical( void )
    {
        const BaseType_t xCoreID = ( BaseType_t ) portGET_CORE_ID();

        traceENTER_vTaskExitCritical();

        if( xSchedulerRunning != pdFALSE )
        {
            /*
             * 若临界区嵌套计数为零，则此函数与之前的 vTaskEnterCritical()
             * 调用不匹配。
             */
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U );

            /*
             * 不应在 ISR 中调用此函数。从 ISR 退出临界区请使用
             * vTaskExitCriticalFromISR。
             */
            portASSERT_IF_IN_ISR();

            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U )
            {
                portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID );

                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
                {
                    BaseType_t xYieldCurrentTask;

                    /* 在临界区内获取 xYieldPending 状态。 */
                    xYieldCurrentTask = xYieldPendings[ xCoreID ];

                    portRELEASE_ISR_LOCK( xCoreID );
                    portRELEASE_TASK_LOCK( xCoreID );
                    portENABLE_INTERRUPTS();

                    /*
                     * 任务在临界区内让出时只会将 xYieldPending 置为 true。因此现在已
                     * 退出临界区，检查 xYieldPending 是否为 true，若为 true 则让出。
                     */
                    if( xYieldCurrentTask != pdFALSE )
                    {
                        portYIELD();
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

        traceRETURN_vTaskExitCritical();
    }

#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    void vTaskExitCriticalFromISR( UBaseType_t uxSavedInterruptStatus )
    {
        BaseType_t xCoreID;

        traceENTER_vTaskExitCriticalFromISR( uxSavedInterruptStatus );

        if( xSchedulerRunning != pdFALSE )
        {
            xCoreID = ( BaseType_t ) portGET_CORE_ID();

            /*
             * 若临界区嵌套计数为零，则此函数与之前的 vTaskEnterCritical()
             * 调用不匹配。
             */
            configASSERT( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U );

            if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) > 0U )
            {
                portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID );

                if( portGET_CRITICAL_NESTING_COUNT( xCoreID ) == 0U )
                {
                    portRELEASE_ISR_LOCK( xCoreID );
                    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
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

        traceRETURN_vTaskExitCriticalFromISR();
    }

#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */
/*-----------------------------------------------------------*/

#if ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 )

    STATIC char * prvWriteNameToBuffer( char * pcBuffer,
                                        const char * pcTaskName )
    {
        size_t x;

        /* 首先复制整个字符串。 */
        ( void ) strcpy( pcBuffer, pcTaskName );

        /*
         * 在字符串末尾用空格填充，以确保打印时各列对齐。
         */
        for( x = strlen( pcBuffer ); x < ( size_t ) ( ( size_t ) configMAX_TASK_NAME_LEN - 1U ); x++ )
        {
            pcBuffer[ x ] = ' ';
        }

        /* 终止字符串。 */
        pcBuffer[ x ] = ( char ) 0x00;

        /* 返回新的字符串末尾位置。 */
        return &( pcBuffer[ x ] );
    }

#endif /* 条件：( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) */
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

    void vTaskListTasks( char * pcWriteBuffer,
                         size_t uxBufferLength )
    {
        TaskStatus_t * pxTaskStatusArray;
        size_t uxConsumedBufferLength = 0;
        size_t uxCharsWrittenBySnprintf;
        int iSnprintfReturnValue;
        BaseType_t xOutputBufferFull = pdFALSE;
        UBaseType_t uxArraySize, x;
        char cStatus;

        traceENTER_vTaskListTasks( pcWriteBuffer, uxBufferLength );

        /*
         * 请注意：
         *
         * 此函数仅为方便使用而提供，许多示例应用程序都使用它。
         * 不要将其视为调度器的一部分。
         *
         * vTaskListTasks() 会调用 uxTaskGetSystemState()，然后将其部分输出
         * 格式化为人类可读的表格，显示任务名称、状态、优先级、栈使用量和任务编号。
         * 栈使用量表示为栈顶剩余未使用的 StackType_t 字数量，而非字节数。
         *
         * vTaskListTasks() 依赖 C 库函数 snprintf()，这可能会增大代码体积、
         * 占用大量栈，并在不同平台上产生不同结果。许多 FreeRTOS/Demo 子目录中
         * 提供了名为 printf-stdarg.c 的替代方案，它是第三方实现，体积小、功能有限
         * （注意 printf-stdarg.c 并未提供完整的 snprintf() 实现！）。
         *
         * 建议生产系统直接调用 uxTaskGetSystemState() 获取原始统计数据，
         * 而不是通过调用 vTaskListTasks() 间接获取。
         */


        /* 确保写入缓冲区不包含字符串。 */
        *pcWriteBuffer = ( char ) 0x00;

        /*
         * 对任务数量进行快照，以防此函数执行期间数量发生变化。
         */
        uxArraySize = uxCurrentNumberOfTasks;

        /*
         * 为每个任务分配一个数组索引。注意！若 configSUPPORT_DYNAMIC_ALLOCATION
         * 设为 0，则 pvPortMalloc() 等价于 NULL。
         */
        /* MISRA 参考 11.5.1 [malloc 内存赋值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
        pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

        if( pxTaskStatusArray != NULL )
        {
            /* 生成（二进制）数据。 */
            uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );

            /* 根据二进制数据创建人类可读的表格。 */
            for( x = 0; x < uxArraySize; x++ )
            {
                switch( pxTaskStatusArray[ x ].eCurrentState )
                {
                    case eRunning:
                        cStatus = tskRUNNING_CHAR;
                        break;

                    case eReady:
                        cStatus = tskREADY_CHAR;
                        break;

                    case eBlocked:
                        cStatus = tskBLOCKED_CHAR;
                        break;

                    case eSuspended:
                        cStatus = tskSUSPENDED_CHAR;
                        break;

                    case eDeleted:
                        cStatus = tskDELETED_CHAR;
                        break;

                    case eInvalid: /* 继续执行下一分支。 */
                    default:       /*
                     * 不应执行到此处，但包含此分支以防止静态检查错误。
                     */
                        cStatus = ( char ) 0x00;
                        break;
                }

                /* 缓冲区是否有足够空间容纳任务名称？ */
                if( ( uxConsumedBufferLength + configMAX_TASK_NAME_LEN ) <= uxBufferLength )
                {
                    /*
                     * 将任务名称写入字符串，并用空格填充，以便更容易以表格形式打印。
                     */
                    pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );
                    /* 不计终止空字符。 */
                    uxConsumedBufferLength = uxConsumedBufferLength + ( configMAX_TASK_NAME_LEN - 1U );

                    /*
                     * 缓冲区是否还有剩余空间？减 1 是因为 snprintf 会写入终止空字符。
                     * 因此实际上是在检查缓冲区是否有空间至少写入一个非空字符。
                     */
                    if( uxConsumedBufferLength < ( uxBufferLength - 1U ) )
                    {
                        /* 写入字符串的其余部分。 */
                        #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) )
                            /* MISRA 参考 21.6.1 [snprintf 的实用用法] */
                            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-216 */
                            /* coverity[misra_c_2012_rule_21_6_violation] 抑制 Coverity 的 MISRA C-2012 规则 21.6 告警 */
                            iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                             uxBufferLength - uxConsumedBufferLength,
                                                             "\t%c\t%u\t%u\t%u\t0x%x\r\n",
                                                             cStatus,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].uxCurrentPriority,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].usStackHighWaterMark,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].xTaskNumber,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].uxCoreAffinityMask );
                        #else /* 条件：( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) ) */
                            /* MISRA 参考 21.6.1 [snprintf 的实用用法] */
                            /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-216 */
                            /* coverity[misra_c_2012_rule_21_6_violation] 抑制 Coverity 的 MISRA C-2012 规则 21.6 告警 */
                            iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                             uxBufferLength - uxConsumedBufferLength,
                                                             "\t%c\t%u\t%u\t%u\r\n",
                                                             cStatus,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].uxCurrentPriority,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].usStackHighWaterMark,
                                                             ( unsigned int ) pxTaskStatusArray[ x ].xTaskNumber );
                        #endif /* 条件：( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) ) */
                        uxCharsWrittenBySnprintf = prvSnprintfReturnValueToCharsWritten( iSnprintfReturnValue, uxBufferLength - uxConsumedBufferLength );

                        uxConsumedBufferLength += uxCharsWrittenBySnprintf;
                        pcWriteBuffer += uxCharsWrittenBySnprintf;
                    }
                    else
                    {
                        xOutputBufferFull = pdTRUE;
                    }
                }
                else
                {
                    xOutputBufferFull = pdTRUE;
                }

                if( xOutputBufferFull == pdTRUE )
                {
                    break;
                }
            }

            /*
             * 再次释放该数组。注意！若 configSUPPORT_DYNAMIC_ALLOCATION 为 0，
             * 则 vPortFree() 会被 #define 为空。
             */
            vPortFree( pxTaskStatusArray );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_vTaskListTasks();
    }

#endif /* 条件：( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) ) */
/*----------------------------------------------------------*/

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configUSE_TRACE_FACILITY == 1 ) )

    void vTaskGetRunTimeStatistics( char * pcWriteBuffer,
                                    size_t uxBufferLength )
    {
        TaskStatus_t * pxTaskStatusArray;
        size_t uxConsumedBufferLength = 0;
        size_t uxCharsWrittenBySnprintf;
        int iSnprintfReturnValue;
        BaseType_t xOutputBufferFull = pdFALSE;
        UBaseType_t uxArraySize, x;
        configRUN_TIME_COUNTER_TYPE ulTotalTime = 0;
        configRUN_TIME_COUNTER_TYPE ulStatsAsPercentage;

        traceENTER_vTaskGetRunTimeStatistics( pcWriteBuffer, uxBufferLength );

        /*
         * 请注意：
         *
         * 此函数仅为方便使用而提供，许多示例应用程序都使用它。
         * 不要将其视为调度器的一部分。
         *
         * vTaskGetRunTimeStatistics() 会调用 uxTaskGetSystemState()，然后将其部分
         * 输出格式化为人类可读的表格，显示每个任务处于运行态的时间，
         * 包括绝对值和百分比。
         *
         * vTaskGetRunTimeStatistics() 依赖 C 库函数 snprintf()，这可能会增大代码
         * 体积、占用大量栈，并在不同平台上产生不同结果。许多 FreeRTOS/Demo
         * 子目录中提供了名为 printf-stdarg.c 的替代方案，它是第三方实现，
         * 体积小、功能有限（注意 printf-stdarg.c 并未提供完整的 snprintf() 实现！）。
         *
         * 建议生产系统直接调用 uxTaskGetSystemState() 获取原始统计数据，
         * 而不是通过调用 vTaskGetRunTimeStatistics() 间接获取。
         */

        /* 确保写入缓冲区不包含字符串。 */
        *pcWriteBuffer = ( char ) 0x00;

        /*
         * 对任务数量进行快照，以防此函数执行期间数量发生变化。
         */
        uxArraySize = uxCurrentNumberOfTasks;

        /*
         * 为每个任务分配一个数组索引。注意！若 configSUPPORT_DYNAMIC_ALLOCATION
         * 设为 0，则 pvPortMalloc() 等价于 NULL。
         */
        /* MISRA 参考 11.5.1 [malloc 内存赋值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation] 抑制 Coverity 的 MISRA C-2012 规则 11.5 告警 */
        pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

        if( pxTaskStatusArray != NULL )
        {
            /* 生成（二进制）数据。 */
            uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );

            /* 用于百分比计算。 */
            ulTotalTime /= ( ( configRUN_TIME_COUNTER_TYPE ) 100U );

            /* 避免除零错误。 */
            if( ulTotalTime > 0U )
            {
                /* 根据二进制数据创建人类可读的表格。 */
                for( x = 0; x < uxArraySize; x++ )
                {
                    /*
                     * 该任务占用了总运行时间的百分之多少？该值始终向下取整到最接近的
                     * 整数。ulTotalRunTime 已预先除以 100。
                     */
                    ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalTime;

                    /* 缓冲区是否有足够空间容纳任务名称？ */
                    if( ( uxConsumedBufferLength + configMAX_TASK_NAME_LEN ) <= uxBufferLength )
                    {
                        /*
                         * 将任务名称写入字符串，并用空格填充，以便更容易以表格形式打印。
                         */
                        pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );
                        /* 不计终止空字符。 */
                        uxConsumedBufferLength = uxConsumedBufferLength + ( configMAX_TASK_NAME_LEN - 1U );

                        /*
                         * 缓冲区是否还有剩余空间？减 1 是因为 snprintf 会写入终止空字符。
                         * 因此实际上是在检查缓冲区是否有空间至少写入一个非空字符。
                         */
                        if( uxConsumedBufferLength < ( uxBufferLength - 1U ) )
                        {
                            if( ulStatsAsPercentage > 0U )
                            {
                                #ifdef portLU_PRINTF_SPECIFIER_REQUIRED
                                {
                                    /* MISRA 参考 21.6.1 [snprintf 的实用用法] */
                                    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-216 */
                                    /* coverity[misra_c_2012_rule_21_6_violation] 抑制 Coverity 的 MISRA C-2012 规则 21.6 告警 */
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%lu\t\t%lu%%\r\n",
                                                                     pxTaskStatusArray[ x ].ulRunTimeCounter,
                                                                     ulStatsAsPercentage );
                                }
                                #else /* 条件：ifdef portLU_PRINTF_SPECIFIER_REQUIRED */
                                {
                                    /*
                                     * sizeof( int ) == sizeof( long )，因此可使用更小的
                                     * printf() 库。
                                     */
                                    /* MISRA 参考 21.6.1 [snprintf 的实用用法] */
                                    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-216 */
                                    /* coverity[misra_c_2012_rule_21_6_violation] 抑制 Coverity 的 MISRA C-2012 规则 21.6 告警 */
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%u\t\t%u%%\r\n",
                                                                     ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter,
                                                                     ( unsigned int ) ulStatsAsPercentage );
                                }
                                #endif /* 条件：ifdef portLU_PRINTF_SPECIFIER_REQUIRED */
                            }
                            else
                            {
                                /*
                                 * 若此处百分比为零，则该任务占用总运行时间不足 1%。
                                 */
                                #ifdef portLU_PRINTF_SPECIFIER_REQUIRED
                                {
                                    /* MISRA 参考 21.6.1 [snprintf 的实用用法] */
                                    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-216 */
                                    /* coverity[misra_c_2012_rule_21_6_violation] 抑制 Coverity 的 MISRA C-2012 规则 21.6 告警 */
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%lu\t\t<1%%\r\n",
                                                                     pxTaskStatusArray[ x ].ulRunTimeCounter );
                                }
                                #else
                                {
                                    /*
                                     * sizeof( int ) == sizeof( long )，因此可使用更小的
                                     * printf() 库。
                                     */
                                    /* MISRA 参考 21.6.1 [snprintf 的实用用法] */
                                    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-216 */
                                    /* coverity[misra_c_2012_rule_21_6_violation] 抑制 Coverity 的 MISRA C-2012 规则 21.6 告警 */
                                    iSnprintfReturnValue = snprintf( pcWriteBuffer,
                                                                     uxBufferLength - uxConsumedBufferLength,
                                                                     "\t%u\t\t<1%%\r\n",
                                                                     ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter );
                                }
                                #endif /* 条件：ifdef portLU_PRINTF_SPECIFIER_REQUIRED */
                            }

                            uxCharsWrittenBySnprintf = prvSnprintfReturnValueToCharsWritten( iSnprintfReturnValue, uxBufferLength - uxConsumedBufferLength );
                            uxConsumedBufferLength += uxCharsWrittenBySnprintf;
                            pcWriteBuffer += uxCharsWrittenBySnprintf;
                        }
                        else
                        {
                            xOutputBufferFull = pdTRUE;
                        }
                    }
                    else
                    {
                        xOutputBufferFull = pdTRUE;
                    }

                    if( xOutputBufferFull == pdTRUE )
                    {
                        break;
                    }
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /*
             * 再次释放该数组。注意！若 configSUPPORT_DYNAMIC_ALLOCATION 为 0，
             * 则 vPortFree() 会被 #define 为空。
             */
            vPortFree( pxTaskStatusArray );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_vTaskGetRunTimeStatistics();
    }

#endif /* 条件：( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) ) */
/*-----------------------------------------------------------*/

TickType_t uxTaskResetEventItemValue( void )
{
    TickType_t uxReturn;

    traceENTER_uxTaskResetEventItemValue();

    uxReturn = listGET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ) );

    /*
     * 将事件链表项重置为其正常值——以便可用于队列和信号量。
     */
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority ) );

    traceRETURN_uxTaskResetEventItemValue( uxReturn );

    return uxReturn;
}
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

    TaskHandle_t pvTaskIncrementMutexHeldCount( void )
    {
        TCB_t * pxTCB;

        traceENTER_pvTaskIncrementMutexHeldCount();

        pxTCB = pxCurrentTCB;

        /*
         * 若在创建任何任务之前调用 xSemaphoreCreateMutex()，
         * 则 pxCurrentTCB 将为 NULL。
         */
        if( pxTCB != NULL )
        {
            ( pxTCB->uxMutexesHeld )++;
        }

        traceRETURN_pvTaskIncrementMutexHeldCount( pxTCB );

        return pxTCB;
    }

#endif /* 条件：configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    uint32_t ulTaskGenericNotifyTake( UBaseType_t uxIndexToWaitOn,
                                      BaseType_t xClearCountOnExit,
                                      TickType_t xTicksToWait )
    {
        uint32_t ulReturn;
        BaseType_t xAlreadyYielded, xShouldBlock = pdFALSE;

        traceENTER_ulTaskGenericNotifyTake( uxIndexToWaitOn, xClearCountOnExit, xTicksToWait );

        configASSERT( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES );

        /*
         * 若通知计数为零，且愿意等待任务通知，则阻塞该任务并等待。
         */
        if( ( pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] == 0U ) && ( xTicksToWait > ( TickType_t ) 0 ) )
        {
            /*
             * 此处挂起调度器，因为 prvAddCurrentTaskToDelayedList 是非确定性操作。
             */
            vTaskSuspendAll();
            {
                /*
                 * 必须进入临界区，以原子方式检查是否已发生通知，并设置标志表示正在
                 * 等待任务通知。若不这样做，从 ISR 发送的通知将会丢失。
                 */
                taskENTER_CRITICAL();
                {
                    /* 仅当通知计数尚未非零时才阻塞。 */
                    if( pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] == 0U )
                    {
                        /* 将此任务标记为正在等待任务通知。 */
                        pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskWAITING_NOTIFICATION;

                        /* 安排等待任务通知。 */
                        xShouldBlock = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                taskEXIT_CRITICAL();

                /*
                 * 现在已退出临界区，但调度器仍处于挂起状态，因此可以安全地执行
                 * 非确定性操作，例如 prvAddCurrentTaskToDelayedList。
                 */
                if( xShouldBlock == pdTRUE )
                {
                    traceTASK_NOTIFY_TAKE_BLOCK( uxIndexToWaitOn );
                    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            xAlreadyYielded = xTaskResumeAll();

            /* 若 xTaskResumeAll 尚未强制重新调度，则强制重新调度。 */
            if( ( xShouldBlock == pdTRUE ) && ( xAlreadyYielded == pdFALSE ) )
            {
                taskYIELD_WITHIN_API();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        taskENTER_CRITICAL();
        {
            traceTASK_NOTIFY_TAKE( uxIndexToWaitOn );
            ulReturn = pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ];

            if( ulReturn != 0U )
            {
                if( xClearCountOnExit != pdFALSE )
                {
                    pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] = ( uint32_t ) 0U;
                }
                else
                {
                    pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] = ulReturn - ( uint32_t ) 1;
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskNOT_WAITING_NOTIFICATION;
        }
        taskEXIT_CRITICAL();

        traceRETURN_ulTaskGenericNotifyTake( ulReturn );

        return ulReturn;
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    BaseType_t xTaskGenericNotifyWait( UBaseType_t uxIndexToWaitOn,
                                       uint32_t ulBitsToClearOnEntry,
                                       uint32_t ulBitsToClearOnExit,
                                       uint32_t * pulNotificationValue,
                                       TickType_t xTicksToWait )
    {
        BaseType_t xReturn, xAlreadyYielded, xShouldBlock = pdFALSE;

        traceENTER_xTaskGenericNotifyWait( uxIndexToWaitOn, ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait );

        configASSERT( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES );

        /*
         * 若任务尚未收到通知，且愿意等待，则阻塞该任务并等待。
         */
        if( ( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED ) && ( xTicksToWait > ( TickType_t ) 0 ) )
        {
            /*
             * 此处挂起调度器，因为 prvAddCurrentTaskToDelayedList 是非确定性操作。
             */
            vTaskSuspendAll();
            {
                /*
                 * 必须进入临界区，以原子方式检查和更新任务通知值。若不这样做，
                 * 来自 ISR 的通知将会丢失。
                 */
                taskENTER_CRITICAL();
                {
                    /* 仅当尚无待处理通知时才阻塞。 */
                    if( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED )
                    {
                        /*
                         * 清除任务通知值中的位，因为通知任务或中断可能已设置这些位。
                         * 这可用于将值清零。
                         */
                        pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] &= ~ulBitsToClearOnEntry;

                        /* 将此任务标记为正在等待任务通知。 */
                        pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskWAITING_NOTIFICATION;

                        /* 安排等待任务通知。 */
                        xShouldBlock = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                taskEXIT_CRITICAL();

                /*
                 * 现在已退出临界区，但调度器仍处于挂起状态，因此可以安全地执行
                 * 非确定性操作，例如 prvAddCurrentTaskToDelayedList。
                 */
                if( xShouldBlock == pdTRUE )
                {
                    traceTASK_NOTIFY_WAIT_BLOCK( uxIndexToWaitOn );
                    prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            xAlreadyYielded = xTaskResumeAll();

            /* 若 xTaskResumeAll 尚未强制重新调度，则强制重新调度。 */
            if( ( xShouldBlock == pdTRUE ) && ( xAlreadyYielded == pdFALSE ) )
            {
                taskYIELD_WITHIN_API();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        taskENTER_CRITICAL();
        {
            traceTASK_NOTIFY_WAIT( uxIndexToWaitOn );

            if( pulNotificationValue != NULL )
            {
                /*
                 * 输出当前通知值，该值可能已改变，也可能未改变。
                 */
                *pulNotificationValue = pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ];
            }

            /*
             * 若 ucNotifyState 被置位，则要么任务从未进入阻塞态（因为已有待处理
             * 通知），要么任务因通知而解除阻塞。否则任务因超时而解除阻塞。
             */
            if( pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] != taskNOTIFICATION_RECEIVED )
            {
                /* 未收到通知。 */
                xReturn = pdFALSE;
            }
            else
            {
                /*
                 * 已有待处理通知，或在任务等待期间收到了通知。
                 */
                pxCurrentTCB->ulNotifiedValue[ uxIndexToWaitOn ] &= ~ulBitsToClearOnExit;
                xReturn = pdTRUE;
            }

            pxCurrentTCB->ucNotifyState[ uxIndexToWaitOn ] = taskNOT_WAITING_NOTIFICATION;
        }
        taskEXIT_CRITICAL();

        traceRETURN_xTaskGenericNotifyWait( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                                   UBaseType_t uxIndexToNotify,
                                   uint32_t ulValue,
                                   eNotifyAction eAction,
                                   uint32_t * pulPreviousNotificationValue )
    {
        TCB_t * pxTCB;
        BaseType_t xReturn = pdPASS;
        uint8_t ucOriginalNotifyState;

        traceENTER_xTaskGenericNotify( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue );

        configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );
        configASSERT( xTaskToNotify );
        pxTCB = xTaskToNotify;

        taskENTER_CRITICAL();
        {
            if( pulPreviousNotificationValue != NULL )
            {
                *pulPreviousNotificationValue = pxTCB->ulNotifiedValue[ uxIndexToNotify ];
            }

            ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];

            pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;

            switch( eAction )
            {
                case eSetBits:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] |= ulValue;
                    break;

                case eIncrement:
                    ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;
                    break;

                case eSetValueWithOverwrite:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    break;

                case eSetValueWithoutOverwrite:

                    if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
                    {
                        pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    }
                    else
                    {
                        /* 无法将值写入该任务。 */
                        xReturn = pdFAIL;
                    }

                    break;

                case eNoAction:

                    /*
                     * 正在通知该任务，但不更新其通知值。
                     */
                    break;

                default:

                    /*
                     * 若所有枚举都已处理，则不应执行到此处。通过测试一个编译器无法
                     * 假定为常量的值来人为触发断言。
                     */
                    configASSERT( xTickCount == ( TickType_t ) 0 );

                    break;
            }

            traceTASK_NOTIFY( uxIndexToNotify );

            /*
             * 若任务处于阻塞态且专门在等待任务通知，则现在将其解除阻塞。
             */
            if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
            {
                listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                prvAddTaskToReadyList( pxTCB );

                /* 该任务不应位于事件链表中。 */
                configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

                #if ( configUSE_TICKLESS_IDLE != 0 )
                {
                    /*
                     * 若任务因等待任务通知而阻塞，则 xNextTaskUnblockTime 可能被设为该
                     * 阻塞任务的超时时间。若任务因超时以外的原因被解除阻塞，
                     * xNextTaskUnblockTime 通常保持不变，因为当节拍计数等于
                     * xNextTaskUnblockTime 时它会自动重置为新值。但若使用无节拍空闲，
                     * 尽早进入睡眠模式可能更重要——因此在此重置 xNextTaskUnblockTime，
                     * 确保它尽可能早地更新。
                     */
                    prvResetNextTaskUnblockTime();
                }
                #endif

                /*
                 * 检查被通知任务的优先级是否高于当前正在执行的任务。
                 */
                taskYIELD_ANY_CORE_IF_USING_PREEMPTION( pxTCB );
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_xTaskGenericNotify( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                          UBaseType_t uxIndexToNotify,
                                          uint32_t ulValue,
                                          eNotifyAction eAction,
                                          uint32_t * pulPreviousNotificationValue,
                                          BaseType_t * pxHigherPriorityTaskWoken )
    {
        TCB_t * pxTCB;
        uint8_t ucOriginalNotifyState;
        BaseType_t xReturn = pdPASS;
        UBaseType_t uxSavedInterruptStatus;

        traceENTER_xTaskGenericNotifyFromISR( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken );

        configASSERT( xTaskToNotify );
        configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );

        /*
         * 支持中断嵌套的 RTOS 移植层具有“最高系统调用（或最高 API 调用）
         * 中断优先级”的概念。高于该优先级的中断保持永久使能，即使 RTOS 内核
         * 处于临界区，但这些中断不能调用任何 FreeRTOS API 函数。
         * 若在 FreeRTOSConfig.h 中定义了 configASSERT()，则当中断的优先级
         * 高于配置的最高系统调用优先级时，从该中断调用 FreeRTOS API 函数
         * 会导致 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 触发断言失败。
         * 只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于或（逻辑上）
         * 低于最高系统调用中断优先级的中断中调用。FreeRTOS 维护一套独立的
         * 中断安全 API，以确保中断入口尽可能快速、简单。以下链接提供更多信息
         * （尽管针对 Cortex-M）：
         * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

        pxTCB = xTaskToNotify;

        /* MISRA 参考 4.7.1 [应检查返回值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
        /* coverity[misra_c_2012_directive_4_7_violation] 抑制 Coverity 的 MISRA C-2012 指令 4.7 告警 */
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            if( pulPreviousNotificationValue != NULL )
            {
                *pulPreviousNotificationValue = pxTCB->ulNotifiedValue[ uxIndexToNotify ];
            }

            ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
            pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;

            switch( eAction )
            {
                case eSetBits:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] |= ulValue;
                    break;

                case eIncrement:
                    ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;
                    break;

                case eSetValueWithOverwrite:
                    pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    break;

                case eSetValueWithoutOverwrite:

                    if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
                    {
                        pxTCB->ulNotifiedValue[ uxIndexToNotify ] = ulValue;
                    }
                    else
                    {
                        /* 无法将值写入该任务。 */
                        xReturn = pdFAIL;
                    }

                    break;

                case eNoAction:

                    /*
                     * 正在通知该任务，但不更新其通知值。
                     */
                    break;

                default:

                    /*
                     * 若所有枚举都已处理，则不应执行到此处。通过测试一个编译器无法
                     * 假定为常量的值来人为触发断言。
                     */
                    configASSERT( xTickCount == ( TickType_t ) 0 );
                    break;
            }

            traceTASK_NOTIFY_FROM_ISR( uxIndexToNotify );

            /*
             * 若任务处于阻塞态且专门在等待任务通知，则现在将其解除阻塞。
             */
            if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
            {
                /* 该任务不应位于事件链表中。 */
                configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );

                    #if ( configUSE_TICKLESS_IDLE != 0 )
                    {
                        /*
                         * 若任务因等待任务通知而阻塞，则 xNextTaskUnblockTime 可能被设为该
                         * 阻塞任务的超时时间。若任务因超时以外的原因被解除阻塞，
                         * xNextTaskUnblockTime 通常保持不变，因为当节拍计数等于
                         * xNextTaskUnblockTime 时它会自动重置为新值。但若使用无节拍空闲，
                         * 尽早进入睡眠模式可能更重要——因此在此重置 xNextTaskUnblockTime，
                         * 确保它尽可能早地更新。
                         */
                        prvResetNextTaskUnblockTime();
                    }
                    #endif
                }
                else
                {
                    /*
                     * 无法访问延时链表和就绪链表，因此将此任务暂存为挂起状态，
                     * 直到调度器恢复。
                     */
                    listINSERT_END( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
                }

                #if ( configNUMBER_OF_CORES == 1 )
                {
                    if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                    {
                        /*
                         * 被通知任务的优先级高于当前正在执行的任务，因此需要让出。
                         */
                        if( pxHigherPriorityTaskWoken != NULL )
                        {
                            *pxHigherPriorityTaskWoken = pdTRUE;
                        }

                        /*
                         * 标记存在待处理的让出请求，以防用户未使用 ISR 安全 FreeRTOS 函数的
                         * "xHigherPriorityTaskWoken" 参数。
                         */
                        xYieldPendings[ 0 ] = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                {
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        prvYieldForTask( pxTCB );

                        if( xYieldPendings[ portGET_CORE_ID() ] == pdTRUE )
                        {
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                        }
                    }
                    #endif /* 条件：if ( configUSE_PREEMPTION == 1 ) */
                }
                #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
            }
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

        traceRETURN_xTaskGenericNotifyFromISR( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    void vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                        UBaseType_t uxIndexToNotify,
                                        BaseType_t * pxHigherPriorityTaskWoken )
    {
        TCB_t * pxTCB;
        uint8_t ucOriginalNotifyState;
        UBaseType_t uxSavedInterruptStatus;

        traceENTER_vTaskGenericNotifyGiveFromISR( xTaskToNotify, uxIndexToNotify, pxHigherPriorityTaskWoken );

        configASSERT( xTaskToNotify );
        configASSERT( uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES );

        /*
         * 支持中断嵌套的 RTOS 移植层具有“最高系统调用（或最高 API 调用）
         * 中断优先级”的概念。高于该优先级的中断保持永久使能，即使 RTOS 内核
         * 处于临界区，但这些中断不能调用任何 FreeRTOS API 函数。
         * 若在 FreeRTOSConfig.h 中定义了 configASSERT()，则当中断的优先级
         * 高于配置的最高系统调用优先级时，从该中断调用 FreeRTOS API 函数
         * 会导致 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 触发断言失败。
         * 只有以 FromISR 结尾的 FreeRTOS 函数才能从优先级等于或（逻辑上）
         * 低于最高系统调用中断优先级的中断中调用。FreeRTOS 维护一套独立的
         * 中断安全 API，以确保中断入口尽可能快速、简单。以下链接提供更多信息
         * （尽管针对 Cortex-M）：
         * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

        pxTCB = xTaskToNotify;

        /* MISRA 参考 4.7.1 [应检查返回值] */
        /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
        /* coverity[misra_c_2012_directive_4_7_violation] 抑制 Coverity 的 MISRA C-2012 指令 4.7 告警 */
        uxSavedInterruptStatus = ( UBaseType_t ) taskENTER_CRITICAL_FROM_ISR();
        {
            ucOriginalNotifyState = pxTCB->ucNotifyState[ uxIndexToNotify ];
            pxTCB->ucNotifyState[ uxIndexToNotify ] = taskNOTIFICATION_RECEIVED;

            /*
             * “释放（Give）”等价于递增计数信号量的计数。
             */
            ( pxTCB->ulNotifiedValue[ uxIndexToNotify ] )++;

            traceTASK_NOTIFY_GIVE_FROM_ISR( uxIndexToNotify );

            /*
             * 若任务处于阻塞态且专门在等待任务通知，则现在将其解除阻塞。
             */
            if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
            {
                /* 该任务不应位于事件链表中。 */
                configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

                if( uxSchedulerSuspended == ( UBaseType_t ) 0U )
                {
                    listREMOVE_ITEM( &( pxTCB->xStateListItem ) );
                    prvAddTaskToReadyList( pxTCB );

                    #if ( configUSE_TICKLESS_IDLE != 0 )
                    {
                        /*
                         * 若任务因等待任务通知而阻塞，则 xNextTaskUnblockTime 可能被设为该
                         * 阻塞任务的超时时间。若任务因超时以外的原因被解除阻塞，
                         * xNextTaskUnblockTime 通常保持不变，因为当节拍计数等于
                         * xNextTaskUnblockTime 时它会自动重置为新值。但若使用无节拍空闲，
                         * 尽早进入睡眠模式可能更重要——因此在此重置 xNextTaskUnblockTime，
                         * 确保它尽可能早地更新。
                         */
                        prvResetNextTaskUnblockTime();
                    }
                    #endif
                }
                else
                {
                    /*
                     * 无法访问延时链表和就绪链表，因此将此任务暂存为挂起状态，
                     * 直到调度器恢复。
                     */
                    listINSERT_END( &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
                }

                #if ( configNUMBER_OF_CORES == 1 )
                {
                    if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
                    {
                        /*
                         * 被通知任务的优先级高于当前正在执行的任务，因此需要让出。
                         */
                        if( pxHigherPriorityTaskWoken != NULL )
                        {
                            *pxHigherPriorityTaskWoken = pdTRUE;
                        }

                        /*
                         * 标记存在待处理的让出请求，以防用户未使用 ISR 安全 FreeRTOS 函数中的
                         * "xHigherPriorityTaskWoken" 参数。
                         */
                        xYieldPendings[ 0 ] = pdTRUE;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
                {
                    #if ( configUSE_PREEMPTION == 1 )
                    {
                        prvYieldForTask( pxTCB );

                        if( xYieldPendings[ portGET_CORE_ID() ] == pdTRUE )
                        {
                            if( pxHigherPriorityTaskWoken != NULL )
                            {
                                *pxHigherPriorityTaskWoken = pdTRUE;
                            }
                        }
                    }
                    #endif /* 条件：#if ( configUSE_PREEMPTION == 1 ) */
                }
                #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
            }
        }
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

        traceRETURN_vTaskGenericNotifyGiveFromISR();
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    BaseType_t xTaskGenericNotifyStateClear( TaskHandle_t xTask,
                                             UBaseType_t uxIndexToClear )
    {
        TCB_t * pxTCB;
        BaseType_t xReturn;

        traceENTER_xTaskGenericNotifyStateClear( xTask, uxIndexToClear );

        configASSERT( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES );

        /*
         * 若此处传入 NULL，则清除的是调用任务的通知状态。
         */
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        taskENTER_CRITICAL();
        {
            if( pxTCB->ucNotifyState[ uxIndexToClear ] == taskNOTIFICATION_RECEIVED )
            {
                pxTCB->ucNotifyState[ uxIndexToClear ] = taskNOT_WAITING_NOTIFICATION;
                xReturn = pdPASS;
            }
            else
            {
                xReturn = pdFAIL;
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_xTaskGenericNotifyStateClear( xReturn );

        return xReturn;
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

    uint32_t ulTaskGenericNotifyValueClear( TaskHandle_t xTask,
                                            UBaseType_t uxIndexToClear,
                                            uint32_t ulBitsToClear )
    {
        TCB_t * pxTCB;
        uint32_t ulReturn;

        traceENTER_ulTaskGenericNotifyValueClear( xTask, uxIndexToClear, ulBitsToClear );

        configASSERT( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES );

        /*
         * 若此处传入 NULL，则清除的是调用任务的通知状态。
         */
        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        taskENTER_CRITICAL();
        {
            /*
             * 返回清除这些位之前的通知值，然后清除位掩码。
             */
            ulReturn = pxTCB->ulNotifiedValue[ uxIndexToClear ];
            pxTCB->ulNotifiedValue[ uxIndexToClear ] &= ~ulBitsToClear;
        }
        taskEXIT_CRITICAL();

        traceRETURN_ulTaskGenericNotifyValueClear( ulReturn );

        return ulReturn;
    }

#endif /* 条件：configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if ( configGENERATE_RUN_TIME_STATS == 1 )

    configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimeCounter( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        configRUN_TIME_COUNTER_TYPE ulTotalTime = 0, ulTimeSinceLastSwitchedIn = 0, ulTaskRunTime = 0;

        traceENTER_ulTaskGetRunTimeCounter( xTask );

        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        taskENTER_CRITICAL();
        {
            if( taskTASK_IS_RUNNING( pxTCB ) == pdTRUE )
            {
                #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                    portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
                #else
                    ulTotalTime = portGET_RUN_TIME_COUNTER_VALUE();
                #endif

                #if ( configNUMBER_OF_CORES == 1 )
                    ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ 0 ];
                #else
                    ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ pxTCB->xTaskRunState ];
                #endif
            }

            ulTaskRunTime = pxTCB->ulRunTimeCounter + ulTimeSinceLastSwitchedIn;
        }
        taskEXIT_CRITICAL();

        traceRETURN_ulTaskGetRunTimeCounter( ulTaskRunTime );

        return ulTaskRunTime;
    }

#endif /* 条件：if ( configGENERATE_RUN_TIME_STATS == 1 ) */
/*-----------------------------------------------------------*/

#if ( configGENERATE_RUN_TIME_STATS == 1 )

    configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimePercent( const TaskHandle_t xTask )
    {
        TCB_t * pxTCB;
        configRUN_TIME_COUNTER_TYPE ulTotalTime, ulReturn, ulTaskRunTime;

        traceENTER_ulTaskGetRunTimePercent( xTask );

        ulTaskRunTime = ulTaskGetRunTimeCounter( xTask );

        #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
            portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
        #else
            ulTotalTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
        #endif

        /* 用于百分比计算。 */
        ulTotalTime /= ( configRUN_TIME_COUNTER_TYPE ) 100;

        /* 避免除零错误。 */
        if( ulTotalTime > ( configRUN_TIME_COUNTER_TYPE ) 0 )
        {
            pxTCB = prvGetTCBFromHandle( xTask );
            configASSERT( pxTCB != NULL );

            ulReturn = ulTaskRunTime / ulTotalTime;
        }
        else
        {
            ulReturn = 0;
        }

        traceRETURN_ulTaskGetRunTimePercent( ulReturn );

        return ulReturn;
    }

#endif /* 条件：if ( configGENERATE_RUN_TIME_STATS == 1 ) */
/*-----------------------------------------------------------*/

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )

    configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounter( void )
    {
        configRUN_TIME_COUNTER_TYPE ulTotalTime = 0, ulTimeSinceLastSwitchedIn = 0, ulIdleTaskRunTime = 0;
        BaseType_t i;

        traceENTER_ulTaskGetIdleRunTimeCounter();

        taskENTER_CRITICAL();
        {
            #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
            #else
                ulTotalTime = portGET_RUN_TIME_COUNTER_VALUE();
            #endif

            for( i = 0; i < ( BaseType_t ) configNUMBER_OF_CORES; i++ )
            {
                if( taskTASK_IS_RUNNING( xIdleTaskHandles[ i ] ) == pdTRUE )
                {
                    #if ( configNUMBER_OF_CORES == 1 )
                        ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ 0 ];
                    #else
                        ulTimeSinceLastSwitchedIn = ulTotalTime - ulTaskSwitchedInTime[ xIdleTaskHandles[ i ]->xTaskRunState ];
                    #endif
                }
                else
                {
                    ulTimeSinceLastSwitchedIn = 0;
                }

                ulIdleTaskRunTime += ( xIdleTaskHandles[ i ]->ulRunTimeCounter + ulTimeSinceLastSwitchedIn );
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_ulTaskGetIdleRunTimeCounter( ulIdleTaskRunTime );

        return ulIdleTaskRunTime;
    }

#endif /* 条件：if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )

    configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimePercent( void )
    {
        configRUN_TIME_COUNTER_TYPE ulTotalTime, ulReturn;
        configRUN_TIME_COUNTER_TYPE ulRunTimeCounter = 0;

        traceENTER_ulTaskGetIdleRunTimePercent();

        #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
            portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalTime );
        #else
            ulTotalTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
        #endif

        ulTotalTime *= configNUMBER_OF_CORES;

        /* 用于百分比计算。 */
        ulTotalTime /= ( configRUN_TIME_COUNTER_TYPE ) 100;

        /* 避免除零错误。 */
        if( ulTotalTime > ( configRUN_TIME_COUNTER_TYPE ) 0 )
        {
            ulRunTimeCounter = ulTaskGetIdleRunTimeCounter();
            ulReturn = ulRunTimeCounter / ulTotalTime;
        }
        else
        {
            ulReturn = 0;
        }

        traceRETURN_ulTaskGetIdleRunTimePercent( ulReturn );

        return ulReturn;
    }

#endif /* 条件：if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) ) */
/*-----------------------------------------------------------*/

STATIC void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,
                                            const BaseType_t xCanBlockIndefinitely )
{
    TickType_t xTimeToWake;
    const TickType_t xConstTickCount = xTickCount;
    List_t * const pxDelayedList = pxDelayedTaskList;
    List_t * const pxOverflowDelayedList = pxOverflowDelayedTaskList;

    #if ( INCLUDE_xTaskAbortDelay == 1 )
    {
        /*
         * 即将进入延时链表，因此确保将 ucDelayAborted 标志重置为 pdFALSE，
         * 以便在任务退出阻塞态时能检测到它已被置为 pdTRUE。
         */
        pxCurrentTCB->ucDelayAborted = ( uint8_t ) pdFALSE;
    }
    #endif

    /*
     * 在将任务加入阻塞链表之前将其从就绪链表中移除，因为两个链表
     * 使用同一个链表项。
     */
    if( uxListRemove( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
    {
        /*
         * 当前任务必定位于就绪链表中，因此无需检查，
         * 可直接调用移植层重置宏。
         */
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        if( ( xTicksToWait == portMAX_DELAY ) && ( xCanBlockIndefinitely != pdFALSE ) )
        {
            /*
             * 将任务加入挂起任务链表而非延时任务链表，以确保它不会被定时事件
             * 唤醒。它将无限期阻塞。
             */
            listINSERT_END( &xSuspendedTaskList, &( pxCurrentTCB->xStateListItem ) );
        }
        else
        {
            /*
             * 计算若事件未发生则任务应被唤醒的时间。该时间可能溢出，但无关紧要，
             * 内核会正确处理。
             */
            xTimeToWake = xConstTickCount + xTicksToWait;

            /* 链表项将按唤醒时间顺序插入。 */
            listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

            if( xTimeToWake < xConstTickCount )
            {
                /*
                 * 唤醒时间已溢出。将此链表项放入溢出链表。
                 */
                traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST();
                vListInsert( pxOverflowDelayedList, &( pxCurrentTCB->xStateListItem ) );
            }
            else
            {
                /*
                 * 唤醒时间未溢出，因此使用当前阻塞链表。
                 */
                traceMOVED_TASK_TO_DELAYED_LIST();
                vListInsert( pxDelayedList, &( pxCurrentTCB->xStateListItem ) );

                /*
                 * 若进入阻塞态的任务被放在阻塞任务链表头部，则 xNextTaskUnblockTime
                 * 也需要更新。
                 */
                if( xTimeToWake < xNextTaskUnblockTime )
                {
                    xNextTaskUnblockTime = xTimeToWake;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
    }
    #else /* 条件：INCLUDE_vTaskSuspend */
    {
        /*
         * 计算若事件未发生则任务应被唤醒的时间。该时间可能溢出，但无关紧要，
         * 内核会正确处理。
         */
        xTimeToWake = xConstTickCount + xTicksToWait;

        /* 链表项将按唤醒时间顺序插入。 */
        listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

        if( xTimeToWake < xConstTickCount )
        {
            traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST();
            /* 唤醒时间已溢出。将此链表项放入溢出链表。 */
            vListInsert( pxOverflowDelayedList, &( pxCurrentTCB->xStateListItem ) );
        }
        else
        {
            traceMOVED_TASK_TO_DELAYED_LIST();
            /* 唤醒时间未溢出，因此使用当前阻塞链表。 */
            vListInsert( pxDelayedList, &( pxCurrentTCB->xStateListItem ) );

            /*
             * 若进入阻塞态的任务被放在阻塞任务链表头部，则 xNextTaskUnblockTime
             * 也需要更新。
             */
            if( xTimeToWake < xNextTaskUnblockTime )
            {
                xNextTaskUnblockTime = xTimeToWake;
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }

        /* 当 INCLUDE_vTaskSuspend 不为 1 时避免编译器告警。 */
        ( void ) xCanBlockIndefinitely;
    }
    #endif /* 条件：INCLUDE_vTaskSuspend */
}
/*-----------------------------------------------------------*/

#if ( portUSING_MPU_WRAPPERS == 1 )

    xMPU_SETTINGS * xTaskGetMPUSettings( TaskHandle_t xTask )
    {
        TCB_t * pxTCB;

        traceENTER_xTaskGetMPUSettings( xTask );

        pxTCB = prvGetTCBFromHandle( xTask );
        configASSERT( pxTCB != NULL );

        traceRETURN_xTaskGetMPUSettings( &( pxTCB->xMPUSettings ) );

        return &( pxTCB->xMPUSettings );
    }

#endif /* 条件：portUSING_MPU_WRAPPERS */
/*-----------------------------------------------------------*/

/*
 * 此后的代码允许将附加代码插入本源文件，尤其是在需要访问文件作用域
 * 函数和数据时（例如执行模块测试时）。
 */

#ifdef FREERTOS_MODULE_TEST
    #include "tasks_test_access_functions.h"
#endif


#if ( configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1 )

    #include "freertos_tasks_c_additions.h"

    #ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
        STATIC void freertos_tasks_c_additions_init( void )
        {
            FREERTOS_TASKS_C_ADDITIONS_INIT();
        }
    #endif

#endif /* 条件：if ( configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1 ) */
/*-----------------------------------------------------------*/

#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS == 0 ) )

/*
 * 这是内核提供的 vApplicationGetIdleTaskMemory() 实现，用于提供空闲任务
 * 所使用的内存。当 configKERNEL_PROVIDED_STATIC_MEMORY 设为 1 时使用。
 * 应用程序可通过将 configKERNEL_PROVIDED_STATIC_MEMORY 设为 0 或保持
 * 未定义，来提供自己的 vApplicationGetIdleTaskMemory 实现。
 */
    void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                        StackType_t ** ppxIdleTaskStackBuffer,
                                        configSTACK_DEPTH_TYPE * puxIdleTaskStackSize )
    {
        static StaticTask_t xIdleTaskTCB;
        static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

        *ppxIdleTaskTCBBuffer = &( xIdleTaskTCB );
        *ppxIdleTaskStackBuffer = &( uxIdleTaskStack[ 0 ] );
        *puxIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    }

    #if ( configNUMBER_OF_CORES > 1 )

        void vApplicationGetPassiveIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                                   StackType_t ** ppxIdleTaskStackBuffer,
                                                   configSTACK_DEPTH_TYPE * puxIdleTaskStackSize,
                                                   BaseType_t xPassiveIdleTaskIndex )
        {
            static StaticTask_t xIdleTaskTCBs[ configNUMBER_OF_CORES - 1 ];
            static StackType_t uxIdleTaskStacks[ configNUMBER_OF_CORES - 1 ][ configMINIMAL_STACK_SIZE ];

            *ppxIdleTaskTCBBuffer = &( xIdleTaskTCBs[ xPassiveIdleTaskIndex ] );
            *ppxIdleTaskStackBuffer = &( uxIdleTaskStacks[ xPassiveIdleTaskIndex ][ 0 ] );
            *puxIdleTaskStackSize = configMINIMAL_STACK_SIZE;
        }

    #endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

#endif /* 条件：#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS == 0 ) ) */
/*-----------------------------------------------------------*/

#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS == 0 ) && ( configUSE_TIMERS == 1 ) )

/*
 * 这是内核提供的 vApplicationGetTimerTaskMemory() 实现，用于提供定时器
 * 服务任务所使用的内存。当 configKERNEL_PROVIDED_STATIC_MEMORY 设为 1 时
 * 使用。应用程序可通过将 configKERNEL_PROVIDED_STATIC_MEMORY 设为 0
 * 或保持未定义，来提供自己的 vApplicationGetTimerTaskMemory 实现。
 */
    void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                         StackType_t ** ppxTimerTaskStackBuffer,
                                         configSTACK_DEPTH_TYPE * puxTimerTaskStackSize )
    {
        static StaticTask_t xTimerTaskTCB;
        static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

        *ppxTimerTaskTCBBuffer = &( xTimerTaskTCB );
        *ppxTimerTaskStackBuffer = &( uxTimerTaskStack[ 0 ] );
        *puxTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
    }

#endif /* 条件：#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configKERNEL_PROVIDED_STATIC_MEMORY == 1 ) && ( portUSING_MPU_WRAPPERS == 0 ) && ( configUSE_TIMERS == 1 ) ) */
/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。该状态通常在启动时初始化。
 * 应用程序必须在重新启动调度器之前调用此函数。
 */
void vTaskResetState( void )
{
    BaseType_t xCoreID;

    /* 任务控制块。 */
    #if ( configNUMBER_OF_CORES == 1 )
    {
        pxCurrentTCB = NULL;
    }
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

    #if ( INCLUDE_vTaskDelete == 1 )
    {
        uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;
    }
    #endif /* 条件：#if ( INCLUDE_vTaskDelete == 1 ) */

    #if ( configUSE_POSIX_ERRNO == 1 )
    {
        FreeRTOS_errno = 0;
    }
    #endif /* 条件：#if ( configUSE_POSIX_ERRNO == 1 ) */

    /* 其他文件私有变量。 */
    uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
    xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
    uxTopReadyPriority = tskIDLE_PRIORITY;
    xSchedulerRunning = pdFALSE;
    xPendedTicks = ( TickType_t ) 0U;

    for( xCoreID = 0; xCoreID < configNUMBER_OF_CORES; xCoreID++ )
    {
        xYieldPendings[ xCoreID ] = pdFALSE;
    }

    xNumOfOverflows = ( BaseType_t ) 0;
    uxTaskNumber = ( UBaseType_t ) 0U;
    xNextTaskUnblockTime = ( TickType_t ) 0U;

    uxSchedulerSuspended = ( UBaseType_t ) 0U;

    #if ( configGENERATE_RUN_TIME_STATS == 1 )
    {
        for( xCoreID = 0; xCoreID < configNUMBER_OF_CORES; xCoreID++ )
        {
            ulTaskSwitchedInTime[ xCoreID ] = 0U;
            ulTotalRunTime[ xCoreID ] = 0U;
        }
    }
    #endif /* 条件：#if ( configGENERATE_RUN_TIME_STATS == 1 ) */
}
/*-----------------------------------------------------------*/
