/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
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


#ifndef INC_TASK_H

/*
 * 【中文导读】task.h —— 任务 API 声明（B级导读）
 * 全部任务 API 在此；每个函数前有官方详细英文文档（Usage/参数/返回值），当手册查。
 */
#define INC_TASK_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include task.h"
#endif

#include "list.h"

/* *INDENT-OFF* 关闭自动缩进 */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* 恢复自动缩进 */

/*
 * -----------------------------------------------------------
 * 宏与定义
 * ----------------------------------------------------------
 */

/*
 * 如果 tskKERNEL_VERSION_NUMBER 以 + 结尾，则表示这是编号版本发布之后
 * 仍在开发中的版本。
 *
 * tskKERNEL_VERSION_MAJOR、tskKERNEL_VERSION_MINOR、tskKERNEL_VERSION_BUILD
 * 的值反映最近一次发布的版本号。
 */
#define tskKERNEL_VERSION_NUMBER                      "V11.1.0+"
#define tskKERNEL_VERSION_MAJOR                       11
#define tskKERNEL_VERSION_MINOR                       1
#define tskKERNEL_VERSION_BUILD                       0

/*
 * 通过 MemoryRegion_t 结构体的 ulParameters 传入的 MPU 区域参数。
 */
#define tskMPU_REGION_READ_ONLY                       ( 1U << 0U )
#define tskMPU_REGION_READ_WRITE                      ( 1U << 1U )
#define tskMPU_REGION_EXECUTE_NEVER                   ( 1U << 2U )
#define tskMPU_REGION_NORMAL_MEMORY                   ( 1U << 3U )
#define tskMPU_REGION_DEVICE_MEMORY                   ( 1U << 4U )
#if defined( portARMV8M_MINOR_VERSION ) && ( portARMV8M_MINOR_VERSION >= 1 )
    #define tskMPU_REGION_PRIVILEGED_EXECUTE_NEVER    ( 1U << 5U )
#endif /* portARMV8M_MINOR_VERSION >= 1（ARMv8-M 次版本号大于等于 1） */
#define tskMPU_REGION_NON_SHAREABLE                   ( 1U << 6U )
#define tskMPU_REGION_OUTER_SHAREABLE                 ( 1U << 7U )
#define tskMPU_REGION_INNER_SHAREABLE                 ( 1U << 8U )

/*
 * 存储在 MPU 设置中的 MPU 区域权限，
 * 用于授权访问请求。
 */
#define tskMPU_READ_PERMISSION                        ( 1U << 0U )
#define tskMPU_WRITE_PERMISSION                       ( 1U << 1U )

/*
 * 任务通知功能过去每个任务只有一个通知值。现在每个任务拥有一个通知值数组，
 * 数组大小由 configTASK_NOTIFICATION_ARRAY_ENTRIES 决定。为保持向后兼容，
 * 所有使用原任务通知接口的代码默认使用数组中的第一个索引。
 */
#define tskDEFAULT_INDEX_TO_NOTIFY                    ( 0 )

/**
 * task. h
 *
 * 任务引用所使用的类型。例如，调用 xTaskCreate 会通过指针参数返回一个
 * TaskHandle_t 变量，该变量随后可作为 vTaskDelete 的参数用于删除任务。
 *
 * \defgroup TaskHandle_t TaskHandle_t
 * \ingroup Tasks
 */
struct tskTaskControlBlock; /* 沿用旧的命名约定，以免破坏能够识别内核的调试器。 */
typedef struct tskTaskControlBlock         * TaskHandle_t;
typedef const struct tskTaskControlBlock   * ConstTaskHandle_t;

/*
 * 定义应用程序任务钩子函数必须遵循的原型。
 */
typedef BaseType_t (* TaskHookFunction_t)( void * arg );

/* eTaskGetState 返回的任务状态。 */
typedef enum
{
    eRunning = 0, /* 任务正在查询自身状态，因此必定处于运行态。 */
    eReady,       /* 被查询的任务位于就绪链表或待处理就绪链表中。 */
    eBlocked,     /* 被查询的任务处于阻塞态。 */
    eSuspended,   /* 被查询的任务处于挂起态，或处于带无限超时的阻塞态。 */
    eDeleted,     /* 被查询的任务已被删除，但其 TCB 尚未释放。 */
    eInvalid      /* 用作“无效状态”值。 */
} eTaskState;

/* 调用 vTaskNotify() 时可执行的操作。 */
typedef enum
{
    eNoAction = 0,            /* 通知任务，但不更新其通知值。 */
    eSetBits,                 /* 设置任务通知值中的位。 */
    eIncrement,               /* 递增任务的通知值。 */
    eSetValueWithOverwrite,   /* 将任务的通知值设置为指定值，即使任务尚未读取先前值。 */
    eSetValueWithoutOverwrite /* 仅在任务已读取先前值的情况下设置任务的通知值。 */
} eNotifyAction;

/*
 * 仅供内部使用。
 */
typedef struct xTIME_OUT
{
    BaseType_t xOverflowCount;
    TickType_t xTimeOnEntering;
} TimeOut_t;

/*
 * 定义使用 MPU 时分配给任务的内存区域。
 */
typedef struct xMEMORY_REGION
{
    void * pvBaseAddress;
    uint32_t ulLengthInBytes;
    uint32_t ulParameters;
} MemoryRegion_t;

/*
 * 创建受 MPU 保护的任务所需的参数。
 */
typedef struct xTASK_PARAMETERS
{
    TaskFunction_t pvTaskCode;
    const char * pcName;
    configSTACK_DEPTH_TYPE usStackDepth;
    void * pvParameters;
    UBaseType_t uxPriority;
    StackType_t * puxStackBuffer;
    MemoryRegion_t xRegions[ portNUM_CONFIGURABLE_REGIONS ];
    #if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
        StaticTask_t * const pxTaskBuffer;
    #endif
} TaskParameters_t;

/*
 * 与 uxTaskGetSystemState() 函数配合使用，返回系统中每个任务的状态。
 */
typedef struct xTASK_STATUS
{
    TaskHandle_t xHandle;                         /* 结构体中其余信息所对应任务的句柄。 */
    const char * pcTaskName;                      /* 指向任务名称的指针。如果任务在结构体填充后被删除，该值将无效！ */
    UBaseType_t xTaskNumber;                      /* 任务唯一的编号。注意，这不是可通过 vTaskSetTaskNumber() 和 uxTaskGetTaskNumber() 修改的任务编号，而是创建任务时自动分配的、TCB 特有的唯一标识符。 */
    eTaskState eCurrentState;                     /* 填充结构体时任务所处的状态。 */
    UBaseType_t uxCurrentPriority;                /* 填充结构体时任务运行所用的优先级（可能为继承的优先级）。 */
    UBaseType_t uxBasePriority;                   /* 若任务当前优先级是通过继承得到的（用于在获取互斥量时避免无界优先级翻转），则此值为任务将恢复到的优先级。仅当 FreeRTOSConfig.h 中 configUSE_MUTEXES 定义为 1 时有效。 */
    configRUN_TIME_COUNTER_TYPE ulRunTimeCounter; /* 按运行时统计时钟定义，截至当前分配给该任务的总运行时间。参见 https://www.FreeRTOS.org/rtos-run-time-stats.html。仅当 FreeRTOSConfig.h 中 configGENERATE_RUN_TIME_STATS 定义为 1 时有效。 */
    StackType_t * pxStackBase;                    /* 指向任务栈区域的最低地址。 */
    #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
        StackType_t * pxTopOfStack;               /* 指向任务栈区域的顶部地址。 */
        StackType_t * pxEndOfStack;               /* 指向任务栈区域的结束地址。 */
    #endif
    configSTACK_DEPTH_TYPE usStackHighWaterMark;  /* 自任务创建以来其栈空间的最小剩余量。该值越接近零，说明任务越接近栈溢出。 */
    #if ( ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 ) )
        UBaseType_t uxCoreAffinityMask;           /* 任务的核心亲和性掩码。 */
    #endif
} TaskStatus_t;

/*
 * uxTaskCallForEachTask() 使用的回调类型。回调每次接收一个任务句柄和状态，
 * 以及调用方提供的不透明上下文指针。若回调需要 TaskStatus_t，可调用 vTaskGetInfo()。
 */
typedef void (* TaskStatusCallbackFunction_t)( TaskHandle_t xTask,
                                               eTaskState eState,
                                               void * pvCallbackContext );

/* eTaskConfirmSleepModeStatus() 的可能返回值。 */
typedef enum
{
    eAbortSleep = 0, /* 自调用 portSUPPRESS_TICKS_AND_SLEEP() 以来，已有任务进入就绪态或挂起了上下文切换——中止进入睡眠模式。 */
    eStandardSleep   /* 进入睡眠模式，但睡眠时间不超过预期的空闲时间。 */
    #if ( INCLUDE_vTaskSuspend == 1 )
        ,
        eNoTasksWaitingTimeout /* 没有任务在等待超时，因此可以安全地进入只能由外部中断退出的睡眠模式。 */
    #endif /* INCLUDE_vTaskSuspend（挂起功能使能条件） */
} eSleepModeStatus;

/**
 * 定义空闲任务使用的优先级。不得修改该值。
 *
 * \ingroup TaskUtils
 */
#define tskIDLE_PRIORITY    ( ( UBaseType_t ) 0U )

/**
 * 定义对全部可用核心的亲和性。
 *
 * \ingroup TaskUtils
 */
#define tskNO_AFFINITY      ( ( UBaseType_t ) -1 )

/**
 * task. h
 *
 * 强制进行上下文切换的宏。
 *
 * \defgroup taskYIELD taskYIELD
 * \ingroup SchedulerControl
 */
#define taskYIELD()                          portYIELD()

/**
 * task. h
 *
 * 标记临界区代码段开始的宏。进入临界区后不会发生抢占式上下文切换。
 *
 * 注意：根据移植层的具体实现，这可能改变栈内容，
 * 因此必须谨慎使用！
 *
 * \defgroup taskENTER_CRITICAL taskENTER_CRITICAL
 * \ingroup SchedulerControl
 */
#define taskENTER_CRITICAL()                 portENTER_CRITICAL()
#if ( configNUMBER_OF_CORES == 1 )
    #define taskENTER_CRITICAL_FROM_ISR()    portSET_INTERRUPT_MASK_FROM_ISR()
#else
    #define taskENTER_CRITICAL_FROM_ISR()    portENTER_CRITICAL_FROM_ISR()
#endif

/**
 * task. h
 *
 * 标记临界区代码段结束的宏。进入临界区后不会发生抢占式上下文切换。
 *
 * 注意：根据移植层的具体实现，这可能改变栈内容，
 * 因此必须谨慎使用！
 *
 * \defgroup taskEXIT_CRITICAL taskEXIT_CRITICAL
 * \ingroup SchedulerControl
 */
#define taskEXIT_CRITICAL()                    portEXIT_CRITICAL()
#if ( configNUMBER_OF_CORES == 1 )
    #define taskEXIT_CRITICAL_FROM_ISR( x )    portCLEAR_INTERRUPT_MASK_FROM_ISR( x )
#else
    #define taskEXIT_CRITICAL_FROM_ISR( x )    portEXIT_CRITICAL_FROM_ISR( x )
#endif

/**
 * task. h
 *
 * 禁用所有可屏蔽中断的宏。
 *
 * \defgroup taskDISABLE_INTERRUPTS taskDISABLE_INTERRUPTS
 * \ingroup SchedulerControl
 */
#define taskDISABLE_INTERRUPTS()    portDISABLE_INTERRUPTS()

/**
 * task. h
 *
 * 使能微控制器中断的宏。
 *
 * \defgroup taskENABLE_INTERRUPTS taskENABLE_INTERRUPTS
 * \ingroup SchedulerControl
 */
#define taskENABLE_INTERRUPTS()     portENABLE_INTERRUPTS()

/*
 * xTaskGetSchedulerState() 返回的定义。taskSCHEDULER_SUSPENDED 为 0，
 * 以便在 configASSERT() 定义为常量时生成更优化的代码，
 * 因为该常量会用于 assert() 语句。
 */
#define taskSCHEDULER_SUSPENDED      ( ( BaseType_t ) 0 )
#define taskSCHEDULER_NOT_STARTED    ( ( BaseType_t ) 1 )
#define taskSCHEDULER_RUNNING        ( ( BaseType_t ) 2 )

/* 检查核心 ID 是否有效。 */
#define taskVALID_CORE_ID( xCoreID )    ( ( ( ( ( BaseType_t ) 0 <= ( xCoreID ) ) && ( ( xCoreID ) < ( BaseType_t ) configNUMBER_OF_CORES ) ) ) ? ( pdTRUE ) : ( pdFALSE ) )

/*
 * -----------------------------------------------------------
 * 任务创建 API
 * ----------------------------------------------------------
 */

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskCreate(
 *                            TaskFunction_t pxTaskCode,
 *                            const char * const pcName,
 *                            const configSTACK_DEPTH_TYPE uxStackDepth,
 *                            void *pvParameters,
 *                            UBaseType_t uxPriority,
 *                            TaskHandle_t *pxCreatedTask
 *                        );
 * @endcode
 *
 * 创建新任务，并将其加入就绪任务链表。
 *
 * 在 FreeRTOS 内部实现中，任务使用两块内存。第一块用于保存任务的数据结构，
 * 第二块由任务用作栈。若使用 xTaskCreate() 创建任务，则这两块内存都在
 * xTaskCreate() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xTaskCreateStatic() 创建任务，
 * 则应用程序编写者必须提供所需内存。因此，xTaskCreateStatic() 允许在不使用
 * 任何动态内存分配的情况下创建任务。
 *
 * 若需不使用任何动态内存分配的版本，请参见 xTaskCreateStatic()。
 *
 * xTaskCreate() 只能用于创建可不受限制地访问整个微控制器内存映射的任务。
 * 支持 MPU 的系统也可以使用 xTaskCreateRestricted() 创建受 MPU 约束的任务。
 *
 * @param pxTaskCode 指向任务入口函数的指针。任务函数必须实现为
 * 永不返回（即无限循环）。
 *
 * @param pcName 任务的描述性名称。主要用于方便调试。
 * 最大长度由 configMAX_TASK_NAME_LEN 定义，默认值为 16。
 *
 * @param uxStackDepth 任务栈的大小，以栈可容纳的变量个数表示，
 * 而不是字节数。例如，若栈宽度为 16 位且 uxStackDepth 定义为 100，
 * 则将为栈存储分配 200 字节。
 *
 * @param pvParameters 将作为所创建任务参数的指针。
 *
 * @param uxPriority 任务应运行的优先级。支持 MPU 的系统可以通过
 * 设置优先级参数的 portPRIVILEGE_BIT 位，选择以特权（系统）模式创建任务。
 * 例如，要以优先级 2 创建特权任务，uxPriority 参数应设置为
 * ( 2 | portPRIVILEGE_BIT )。
 *
 * @param pxCreatedTask 用于回传句柄，以便引用所创建的任务。
 *
 * @return 若任务创建成功并加入就绪链表，返回 pdPASS；
 * 否则返回 projdefs.h 文件中定义的错误码。
 *
 * 用法示例：
 * @code{c}
 * // 待创建的任务。
 * void vTaskCode( void * pvParameters )
 * {
 *   for( ;; )
 *   {
 *       // 任务代码写在这里。
 *   }
 * }
 *
 * // 创建任务的函数。
 * void vOtherFunction( void )
 * {
 * static uint8_t ucParameterToPass;
 * TaskHandle_t xHandle = NULL;
 *
 *   // 创建任务并保存句柄。注意，传入的参数 ucParameterToPass
 *   // 必须在任务整个生命期内有效，因此这里声明为 static。如果它只是
 *   // 一个自动栈变量，那么到新任务尝试访问它时，它可能已不存在，
 *   // 或者至少已被破坏。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
 *   configASSERT( xHandle );
 *
 *   // 使用句柄删除任务。
 *   if( xHandle != NULL )
 *   {
 *      vTaskDelete( xHandle );
 *   }
 * }
 * @endcode
 * \defgroup xTaskCreate xTaskCreate
 * \ingroup Tasks
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE uxStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif

#if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    BaseType_t xTaskCreateAffinitySet( TaskFunction_t pxTaskCode,
                                       const char * const pcName,
                                       const configSTACK_DEPTH_TYPE uxStackDepth,
                                       void * const pvParameters,
                                       UBaseType_t uxPriority,
                                       UBaseType_t uxCoreAffinityMask,
                                       TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
 *                               const char * const pcName,
 *                               const configSTACK_DEPTH_TYPE uxStackDepth,
 *                               void *pvParameters,
 *                               UBaseType_t uxPriority,
 *                               StackType_t *puxStackBuffer,
 *                               StaticTask_t *pxTaskBuffer );
 * @endcode
 *
 * 创建新任务，并将其加入就绪任务链表。
 *
 * 在 FreeRTOS 内部实现中，任务使用两块内存。第一块用于保存任务的数据结构，
 * 第二块由任务用作栈。若使用 xTaskCreate() 创建任务，则这两块内存都在
 * xTaskCreate() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xTaskCreateStatic() 创建任务，
 * 则应用程序编写者必须提供所需内存。因此，xTaskCreateStatic() 允许在不使用
 * 任何动态内存分配的情况下创建任务。
 *
 * @param pxTaskCode 指向任务入口函数的指针。任务函数必须实现为
 * 永不返回（即无限循环）。
 *
 * @param pcName 任务的描述性名称。主要用于方便调试。
 * 字符串的最大长度由 FreeRTOSConfig.h 中的 configMAX_TASK_NAME_LEN 定义。
 *
 * @param uxStackDepth 任务栈的大小，以栈可容纳的变量个数表示，
 * 而不是字节数。例如，若栈宽度为 32 位且 uxStackDepth 定义为 100，
 * 则将为栈存储分配 400 字节。
 *
 * @param pvParameters 将作为所创建任务参数的指针。
 *
 * @param uxPriority 任务将运行的优先级。
 *
 * @param puxStackBuffer 必须指向一个至少有 uxStackDepth 个元素的
 * StackType_t 数组——该数组随后用作任务的栈，从而无需动态分配栈。
 *
 * @param pxTaskBuffer 必须指向一个 StaticTask_t 类型的变量，
 * 该变量随后用于保存任务的数据结构，从而无需动态分配内存。
 *
 * @return 若 puxStackBuffer 和 pxTaskBuffer 均不为 NULL，则任务将被创建，
 * 并返回所创建任务的句柄。若 puxStackBuffer 或 pxTaskBuffer 任一为 NULL，
 * 则任务不会创建，返回 NULL。
 *
 * 用法示例：
 * @code{c}
 *
 *  // 所创建任务用作栈的缓冲区大小。
 *  // 注意：这是栈可容纳的字数，而不是字节数。
 *  // 例如，若每个栈项为 32 位，且此值设为 100，
 *  // 则将分配 400 字节（100 * 32 位）。
 * #define STACK_SIZE 200
 *
 *  // 用于保存所创建任务 TCB 的结构体。
 *  StaticTask_t xTaskBuffer;
 *
 *  // 所创建任务用作栈的缓冲区。注意，这是一个
 *  // StackType_t 变量数组。StackType_t 的大小取决于
 *  // RTOS 移植层。
 *  StackType_t xStack[ STACK_SIZE ];
 *
 *  // 实现所创建任务的函数。
 *  void vTaskCode( void * pvParameters )
 *  {
 *      // 预期参数值为 1，因为在调用 xTaskCreateStatic() 时
 *      // 向 pvParameters 传入了 1。
 *      configASSERT( ( uint32_t ) pvParameters == 1U );
 *
 *      for( ;; )
 *      {
 *          // 任务代码写在这里。
 *      }
 *  }
 *
 *  // 创建任务的函数。
 *  void vOtherFunction( void )
 *  {
 *      TaskHandle_t xHandle = NULL;
 *
 *      // 创建任务，不使用任何动态内存分配。
 *      xHandle = xTaskCreateStatic(
 *                    vTaskCode,       // 实现任务的函数。
 *                    "NAME",          // 任务的文本名称。
 *                    STACK_SIZE,      // 栈大小，以字为单位，而不是字节。
 *                    ( void * ) 1,    // 传入任务的参数。
 *                    tskIDLE_PRIORITY,// 创建任务时使用的优先级。
 *                    xStack,          // 用作任务栈的数组。
 *                    &xTaskBuffer );  // 用于保存任务数据结构的变量。
 *
 *      // puxStackBuffer 和 pxTaskBuffer 均不为 NULL，因此任务已
 *      // 创建成功，xHandle 将是该任务的句柄。使用该句柄
 *      // 挂起任务。
 *      vTaskSuspend( xHandle );
 *  }
 * @endcode
 * \defgroup xTaskCreateStatic xTaskCreateStatic
 * \ingroup Tasks
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                    const char * const pcName,
                                    const configSTACK_DEPTH_TYPE uxStackDepth,
                                    void * const pvParameters,
                                    UBaseType_t uxPriority,
                                    StackType_t * const puxStackBuffer,
                                    StaticTask_t * const pxTaskBuffer ) PRIVILEGED_FUNCTION;
#endif /* configSUPPORT_STATIC_ALLOCATION（静态分配支持） */

#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    TaskHandle_t xTaskCreateStaticAffinitySet( TaskFunction_t pxTaskCode,
                                               const char * const pcName,
                                               const configSTACK_DEPTH_TYPE uxStackDepth,
                                               void * const pvParameters,
                                               UBaseType_t uxPriority,
                                               StackType_t * const puxStackBuffer,
                                               StaticTask_t * const pxTaskBuffer,
                                               UBaseType_t uxCoreAffinityMask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskCreateRestricted( TaskParameters_t *pxTaskDefinition, TaskHandle_t *pxCreatedTask );
 * @endcode
 *
 * 仅当 configSUPPORT_DYNAMIC_ALLOCATION 设置为 1 时可用。
 *
 * xTaskCreateRestricted() 只应在包含 MPU 实现的系统中使用。
 *
 * 创建新任务，并将其加入就绪任务链表。函数参数定义分配给任务的内存区域
 * 及相关访问权限。
 *
 * MemoryRegion_t.ulParameters 中使用的参数宏与移植层相关。
 * 部分移植层（包括 Cortex-M3/4 MPU 移植层）使用下文列出的 portMPU_REGION_* 值。
 * ARMv8-M MPU 移植层（如 CM23、CM33、CM52、CM55、CM85 和 STAR_MC3）使用
 * 本头文件中定义的 tskMPU_REGION_* 值；移植层会将其转换为 MPU 寄存器设置。
 * 选择区域参数宏之前，请先查看所选移植层的头文件。
 *
 * 若需不使用任何动态内存分配的版本，请参见 xTaskCreateRestrictedStatic()。
 *
 * @param pxTaskDefinition 指向一个结构体的指针，该结构体为每个常规
 * xTaskCreate() 参数（参见 xTaskCreate() API 文档）包含一个成员，
 * 并额外包含可选的栈缓冲区和内存区域定义。
 *
 * @param pxCreatedTask 用于回传句柄，以便引用所创建的任务。
 *
 * @return 若任务创建成功并加入就绪链表，返回 pdPASS；
 * 否则返回 projdefs.h 文件中定义的错误码。
 *
 * 用法示例：
 * @code{c}
 * // 创建定义待创建任务的 TaskParameters_t 结构体。
 * static const TaskParameters_t xCheckTaskParameters =
 * {
 *  vATask,     // pvTaskCode —— 实现任务的函数。
 *  "ATask",    // pcName —— 仅用于辅助调试的任务文本名称。
 *  100,        // uxStackDepth —— 以字为单位定义的栈大小。
 *  NULL,       // pvParameters —— 作为函数参数传入任务函数。
 *  ( 1U | portPRIVILEGE_BIT ),// uxPriority —— 任务优先级；若任务应以特权态运行，则设置 portPRIVILEGE_BIT。
 *  cStackBuffer,// puxStackBuffer —— 用作任务栈的缓冲区。
 *
 *  // xRegions —— 最多分配三个独立的内存区域供任务访问，并赋予
 *  // 相应访问权限。不同处理器有不同的内存对齐要求——完整信息
 *  // 请参阅 FreeRTOS 文档。
 *  {
 *      // 基地址                       长度    参数
 *      { cReadWriteArray,              32,     portMPU_REGION_READ_WRITE },
 *      { cReadOnlyArray,               32,     portMPU_REGION_READ_ONLY },
 *      { cPrivilegedOnlyAccessArray,   128,    portMPU_REGION_PRIVILEGED_READ_WRITE }
 *  }
 * };
 *
 * int main( void )
 * {
 * TaskHandle_t xHandle;
 *
 *  // 根据上面定义的 const 结构体创建任务。请求了任务句柄
 *  // （第二个参数不为 NULL），但这里仅作演示，实际并未使用。
 *  xTaskCreateRestricted( &xRegTest1Parameters, &xHandle );
 *
 *  // 启动调度器。
 *  vTaskStartScheduler();
 *
 *  // 只有内存不足、无法创建空闲任务和/或定时器任务时才会执行到这里。
 *  for( ;; );
 * }
 * @endcode
 * \defgroup xTaskCreateRestricted xTaskCreateRestricted
 * \ingroup Tasks
 */
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
                                      TaskHandle_t * pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    BaseType_t xTaskCreateRestrictedAffinitySet( const TaskParameters_t * const pxTaskDefinition,
                                                 UBaseType_t uxCoreAffinityMask,
                                                 TaskHandle_t * pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskCreateRestrictedStatic( TaskParameters_t *pxTaskDefinition, TaskHandle_t *pxCreatedTask );
 * @endcode
 *
 * 仅当 configSUPPORT_STATIC_ALLOCATION 设置为 1 时可用。
 *
 * xTaskCreateRestrictedStatic() 只应在包含 MPU 实现的系统中使用。
 *
 * 在 FreeRTOS 内部实现中，任务使用两块内存。第一块用于保存任务的数据结构，
 * 第二块由任务用作栈。若使用 xTaskCreateRestricted() 创建任务，则栈由
 * 应用程序编写者提供，而保存任务数据结构所需的内存则在
 * xTaskCreateRestricted() 函数内部自动动态分配。若使用
 * xTaskCreateRestrictedStatic() 创建任务，则应用程序编写者还必须提供
 * 保存任务数据结构所需的内存。因此，xTaskCreateRestrictedStatic() 允许在
 * 不使用任何动态内存分配的情况下创建受内存保护的任务。
 *
 * MemoryRegion_t.ulParameters 中使用的参数宏与移植层相关。
 * 部分移植层（包括 Cortex-M3/4 MPU 移植层）使用下文列出的 portMPU_REGION_* 值。
 * ARMv8-M MPU 移植层（如 CM23、CM33、CM52、CM55、CM85 和 STAR_MC3）使用
 * 本头文件中定义的 tskMPU_REGION_* 值；移植层会将其转换为 MPU 寄存器设置。
 * 选择区域参数宏之前，请先查看所选移植层的头文件。
 *
 * @param pxTaskDefinition 指向一个结构体的指针，该结构体为每个常规
 * xTaskCreate() 参数（参见 xTaskCreate() API 文档）包含一个成员，
 * 并额外包含可选的栈缓冲区和内存区域定义。若 configSUPPORT_STATIC_ALLOCATION
 * 设置为 1，该结构体还包含一个附加成员，用于指向 StaticTask_t 类型的变量，
 * 该变量随后用于保存任务的数据结构。
 *
 * @param pxCreatedTask 用于回传句柄，以便引用所创建的任务。
 *
 * @return 若任务创建成功并加入就绪链表，返回 pdPASS；
 * 否则返回 projdefs.h 文件中定义的错误码。
 *
 * 用法示例：
 * @code{c}
 * // 创建定义待创建任务的 TaskParameters_t 结构体。
 * // 仅当 configSUPPORT_STATIC_ALLOCATION 设置为 1 时，
 * // 结构体中才包含 StaticTask_t 变量。可使用 PRIVILEGED_DATA 宏
 * // 强制将该变量放入 RTOS 内核的特权数据区。
 * static PRIVILEGED_DATA StaticTask_t xTaskBuffer;
 * static const TaskParameters_t xCheckTaskParameters =
 * {
 *  vATask,     // pvTaskCode —— 实现任务的函数。
 *  "ATask",    // pcName —— 仅用于辅助调试的任务文本名称。
 *  100,        // uxStackDepth —— 以字为单位定义的栈大小。
 *  NULL,       // pvParameters —— 作为函数参数传入任务函数。
 *  ( 1U | portPRIVILEGE_BIT ),// uxPriority —— 任务优先级；若任务应以特权态运行，则设置 portPRIVILEGE_BIT。
 *  cStackBuffer,// puxStackBuffer —— 用作任务栈的缓冲区。
 *
 *  // xRegions —— 最多分配三个独立的内存区域供任务访问，并赋予
 *  // 相应访问权限。不同处理器有不同的内存对齐要求——完整信息
 *  // 请参阅 FreeRTOS 文档。
 *  {
 *      // 基地址                       长度    参数
 *      { cReadWriteArray,              32,     portMPU_REGION_READ_WRITE },
 *      { cReadOnlyArray,               32,     portMPU_REGION_READ_ONLY },
 *      { cPrivilegedOnlyAccessArray,   128,    portMPU_REGION_PRIVILEGED_READ_WRITE }
 *  }
 *
 *  &xTaskBuffer; // 保存任务的数据结构。
 * };
 *
 * int main( void )
 * {
 * TaskHandle_t xHandle;
 *
 *  // 根据上面定义的 const 结构体创建任务。请求了任务句柄
 *  // （第二个参数不为 NULL），但这里仅作演示，实际并未使用。
 *  xTaskCreateRestrictedStatic( &xRegTest1Parameters, &xHandle );
 *
 *  // 启动调度器。
 *  vTaskStartScheduler();
 *
 *  // 只有内存不足、无法创建空闲任务和/或定时器任务时才会执行到这里。
 *  for( ;; );
 * }
 * @endcode
 * \defgroup xTaskCreateRestrictedStatic xTaskCreateRestrictedStatic
 * \ingroup Tasks
 */
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    BaseType_t xTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
                                            TaskHandle_t * pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    BaseType_t xTaskCreateRestrictedStaticAffinitySet( const TaskParameters_t * const pxTaskDefinition,
                                                       UBaseType_t uxCoreAffinityMask,
                                                       TaskHandle_t * pxCreatedTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * void vTaskAllocateMPURegions( TaskHandle_t xTask, const MemoryRegion_t * const pxRegions );
 * @endcode
 *
 * 受限任务通过调用 xTaskCreateRestricted() 创建时会被分配内存区域。
 * 这些区域可以使用 vTaskAllocateMPURegions() 重新定义。
 *
 * @param xTaskToModify 正在更新的任务句柄。
 *
 * @param[in] pxRegions 指向包含新内存区域定义的 MemoryRegion_t 结构体的指针。
 *
 * MemoryRegion_t.ulParameters 中使用的参数宏与移植层相关。
 * 部分移植层（包括 Cortex-M3/4 MPU 移植层）使用下文列出的 portMPU_REGION_* 值。
 * ARMv8-M MPU 移植层（如 CM23、CM33、CM52、CM55、CM85 和 STAR_MC3）使用
 * 本头文件中定义的 tskMPU_REGION_* 值；移植层会将其转换为 MPU 寄存器设置。
 * 选择区域参数宏之前，请先查看所选移植层的头文件。
 *
 * 用法示例：
 * @code{c}
 * // 定义一个 MemoryRegion_t 结构体数组，配置一个 MPU 区域，
 * // 允许从 ucOneKByte 数组起始处开始的 1024 字节进行读写访问。
 * // 最多可定义 3 个区域，其余两个未使用，因此设为 0。
 * static const MemoryRegion_t xAltRegions[ portNUM_CONFIGURABLE_REGIONS ] =
 * {
 *  // 基地址            长度        参数
 *  { ucOneKByte,       1024,       portMPU_REGION_READ_WRITE },
 *  { 0,                0,          0 },
 *  { 0,                0,          0 }
 * };
 *
 * void vATask( void *pvParameters )
 * {
 *  // 此任务在创建时被配置为可访问 MPU 配置所定义的某些内存区域。
 *  // 在某一时刻，需要将这些 MPU 区域替换为上面 const 结构体
 *  // xAltRegions 中定义的区域。为此可调用 vTaskAllocateMPURegions()。
 *  // 任务句柄传入 NULL，表示此函数应修改调用任务的 MPU 区域。
 *  vTaskAllocateMPURegions( NULL, xAltRegions );
 *
 *  // 现在任务可以继续执行，但从此刻起只能访问其栈和 ucOneKByte
 *  // 数组（除非在其他地方声明了其他静态定义或共享的区域）。
 * }
 * @endcode
 * \defgroup vTaskAllocateMPURegions vTaskAllocateMPURegions
 * \ingroup Tasks
 */
#if ( portUSING_MPU_WRAPPERS == 1 )
    void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify,
                                  const MemoryRegion_t * const pxRegions ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * void vTaskDelete( TaskHandle_t xTaskToDelete );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_vTaskDelete 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 将任务从 RTOS 实时内核的管理中移除。被删除的任务将从所有就绪、
 * 阻塞、挂起和事件链表中移除。
 *
 * 注意：空闲任务负责释放已删除任务所占用的、由内核分配的内存。
 * 因此，如果应用程序调用了 vTaskDelete()，务必确保空闲任务不会
 * 缺少微控制器处理时间。任务代码分配的内存不会自动释放，
 * 应在删除任务之前释放。
 *
 * 有关使用 vTaskDelete() 的示例代码，请参见演示应用程序文件 death.c。
 *
 * @param xTaskToDelete 要删除的任务句柄。传入 NULL 将删除调用任务。
 *
 * 用法示例：
 * @code{c}
 * void vOtherFunction( void )
 * {
 * TaskHandle_t xHandle;
 *
 *   // 创建任务并保存句柄。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *   // 使用句柄删除任务。
 *   vTaskDelete( xHandle );
 * }
 * @endcode
 * \defgroup vTaskDelete vTaskDelete
 * \ingroup Tasks
 */
void vTaskDelete( TaskHandle_t xTaskToDelete ) PRIVILEGED_FUNCTION;

/*
 * -----------------------------------------------------------
 * 任务控制 API
 * ----------------------------------------------------------
 */

/**
 * task. h
 * @code{c}
 * void vTaskDelay( const TickType_t xTicksToDelay );
 * @endcode
 *
 * 按指定的时钟节拍数延迟任务。任务实际保持阻塞的时间取决于时钟节拍频率。
 * 可以使用常量 portTICK_PERIOD_MS 将节拍频率换算为实际时间，
 * 分辨率为一个节拍周期。
 *
 * 要使此函数可用，必须将 INCLUDE_vTaskDelay 定义为 1。
 * 更多信息请参见配置章节。
 *
 *
 * vTaskDelay() 指定的是相对于调用 vTaskDelay() 时刻的解除阻塞时间。
 * 例如，指定 100 个节拍的阻塞时间，任务将在调用 vTaskDelay() 之后
 * 100 个节拍时解除阻塞。因此，vTaskDelay() 并不是控制周期性任务频率的
 * 好方法，因为代码的执行路径以及其他任务和中断活动，都会影响
 * vTaskDelay() 的调用频率，进而影响任务下次执行的时间。
 * 若需固定频率执行，请参见 xTaskDelayUntil()；该 API 通过指定
 * 调用任务应解除阻塞的绝对时间（而非相对时间）来实现固定频率。
 *
 * @param xTicksToDelay 调用任务应阻塞的时间，以节拍周期为单位。
 *
 * 用法示例：
 *
 * void vTaskFunction( void * pvParameters )
 * {
 * // 阻塞 500ms。
 * const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
 *
 *   for( ;; )
 *   {
 *       // 每 500ms 翻转一次 LED，每次翻转之间进入阻塞。
 *       vToggleLED();
 *       vTaskDelay( xDelay );
 *   }
 * }
 *
 * \defgroup vTaskDelay vTaskDelay
 * \ingroup TaskCtrl
 */
void vTaskDelay( const TickType_t xTicksToDelay ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * TickType_t xTaskPeriodicDelay( TickType_t *pxPreviousWakeTime, const TickType_t xTimeIncrement );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_xTaskDelayUntil 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 周期性任务延迟，用于确保恒定的执行频率。
 *
 * 此函数与 xTaskDelayUntil() 类似，但有以下几处重要区别：
 * - pxPreviousWakeTime 保存上一次的唤醒时间，因此不会无限漂移
 * - 若挂起任务，则恢复时 pxPreviousWakeTime 会立即
 *   补上所有被跳过的增量
 * - 返回添加到 pxPreviosWakeTime 的增量次数
 *
 * @param pxPreviousWakeTime 指向保存任务上次解除阻塞时间的变量的指针。
 * 首次使用前，该变量必须用当前时间初始化。此后该变量会自动更新。
 *
 * @param xTimeIncrement 周期时间。任务将在 *pxPreviousWakeTime + xTimeIncrement
 * 时刻解除阻塞。传入相同的 xTimeIncrement 参数值，将使任务以固定间隔执行。
 *
 * @return xTimeIncrement 被添加到 pxPreviousWakeTime 的次数。
 * 首次调用时，或自上次调用以来经过的节拍不足时，返回 0；正常情况返回 1；
 * 若由于某种原因跳过了某些周期（例如调用任务被挂起的时间超过
 * xTimeIncrement 个节拍），则返回值大于 1。
 *
 * \defgroup xTaskPeriodicDelay xTaskPeriodicDelay
 * \ingroup TaskCtrl
 */
TickType_t xTaskPeriodicDelay( TickType_t * const pxPreviousWakeTime,
                               const TickType_t xTimeIncrement ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskDelayUntil( TickType_t *pxPreviousWakeTime, const TickType_t xTimeIncrement );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_xTaskDelayUntil 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 将任务延迟到指定时间。此函数可供周期性任务使用，以确保恒定的执行频率。
 *
 * 此函数与 vTaskDelay() 有一个重要区别：vTaskDelay() 会使任务从调用
 * vTaskDelay() 的时刻起阻塞指定的节拍数。因此，仅使用 vTaskDelay() 很难
 * 产生固定的执行频率，因为任务开始执行到调用 vTaskDelay() 之间的时间
 * 可能并不固定［任务在两次调用之间可能走不同的代码路径，或者每次执行时
 * 被中断或抢占的次数不同］。
 *
 * vTaskDelay() 指定的是相对于函数调用时刻的唤醒时间，而 xTaskDelayUntil()
 * 指定的是任务希望解除阻塞的绝对（精确）时间。
 *
 * 可以使用宏 pdMS_TO_TICKS() 将以毫秒指定的时间换算为节拍数，
 * 分辨率为一个节拍周期。
 *
 * @param pxPreviousWakeTime 指向保存任务上次解除阻塞时间的变量的指针。
 * 首次使用前，该变量必须用当前时间初始化（参见下面的示例）。此后该变量
 * 会在 xTaskDelayUntil() 内部自动更新。
 *
 * @param xTimeIncrement 周期时间。任务将在 *pxPreviousWakeTime + xTimeIncrement
 * 时刻解除阻塞。以相同的 xTimeIncrement 参数值调用 xTaskDelayUntil，
 * 将使任务以固定的周期执行。
 *
 * @return 可用于检查任务是否实际被延迟的值。
 * 若任务被延迟则返回 pdTRUE，否则返回 pdFALSE。若下一个预期唤醒时间
 * 已过，则任务不会被延迟。这可防止周期性任务累积延迟，使其能够恢复
 * 正常的定时节奏。
 *
 * 用法示例：
 * @code{c}
 * // 每 10 个节拍执行一次操作。
 * void vTaskFunction( void * pvParameters )
 * {
 * TickType_t xLastWakeTime;
 * const TickType_t xFrequency = 10;
 * BaseType_t xWasDelayed;
 *
 *     // 用当前时间初始化 xLastWakeTime 变量。
 *     xLastWakeTime = xTaskGetTickCount ();
 *     for( ;; )
 *     {
 *         // 等待下一个周期。
 *         xWasDelayed = xTaskDelayUntil( &xLastWakeTime, xFrequency );
 *
 *         // 在此执行操作。若此处的代码耗时过长，可用 xWasDelayed
 *         // 的值判断是否错过了截止时间。
 *     }
 * }
 * @endcode
 * \defgroup xTaskDelayUntil xTaskDelayUntil
 * \ingroup TaskCtrl
 */
BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime,
                            const TickType_t xTimeIncrement ) PRIVILEGED_FUNCTION;

/*
 * vTaskDelayUntil() 是 xTaskDelayUntil() 的旧版本，不返回值。
 */
#define vTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement )                   \
    do {                                                                        \
        ( void ) xTaskDelayUntil( ( pxPreviousWakeTime ), ( xTimeIncrement ) ); \
    } while( 0 )


/**
 * task. h
 * @code{c}
 * BaseType_t xTaskAbortDelay( TaskHandle_t xTask );
 * @endcode
 *
 * 要使此函数可用，必须在 FreeRTOSConfig.h 中将 INCLUDE_xTaskAbortDelay 定义为 1。
 *
 * 任务在等待事件时会进入阻塞态。所等待的事件可以是时间事件（等待一段时间），
 * 例如调用 vTaskDelay() 时，也可以是对象上的事件，例如调用
 * xQueueReceive() 或 ulTaskNotifyTake() 时。若在调用 xTaskAbortDelay() 时
 * 传入处于阻塞态任务的句柄，则该任务将离开阻塞态，并从使其进入阻塞态的
 * 那个函数调用中返回。
 *
 * 此函数没有 'FromISR' 版本，因为中断需要知道任务阻塞在哪个对象上，
 * 才能确定应采取哪些操作。例如，若任务阻塞在队列上，中断处理程序
 * 还需要知道该队列是否已被锁定。
 *
 * @param xTask 要移出阻塞态的任务句柄。
 *
 * @return 若 xTask 引用的任务未处于阻塞态，则返回 pdFAIL；
 * 否则返回 pdPASS。
 *
 * \defgroup xTaskAbortDelay xTaskAbortDelay
 * \ingroup TaskCtrl
 */
#if ( INCLUDE_xTaskAbortDelay == 1 )
    BaseType_t xTaskAbortDelay( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_uxTaskPriorityGet 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 获取任意任务的优先级。
 *
 * @param xTask 要查询的任务句柄。传入 NULL 句柄将返回调用任务的优先级。
 *
 * @return xTask 的优先级。
 *
 * 用法示例：
 * @code{c}
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 *
 *   // 创建任务并保存句柄。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *   // ...
 *
 *   // 使用句柄获取所创建任务的优先级。
 *   // 创建时使用的是 tskIDLE_PRIORITY，但任务可能自行修改过。
 *   if( uxTaskPriorityGet( xHandle ) != tskIDLE_PRIORITY )
 *   {
 *       // 任务已修改其优先级。
 *   }
 *
 *   // ...
 *
 *   // 我们的优先级是否高于所创建的任务？
 *   if( uxTaskPriorityGet( xHandle ) < uxTaskPriorityGet( NULL ) )
 *   {
 *       // 我们的优先级（通过 NULL 句柄获取）更高。
 *   }
 * }
 * @endcode
 * \defgroup uxTaskPriorityGet uxTaskPriorityGet
 * \ingroup TaskCtrl
 */
UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask );
 * @endcode
 *
 * 可在 ISR 中使用的 uxTaskPriorityGet() 版本。
 */
UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * UBaseType_t uxTaskBasePriorityGet( const TaskHandle_t xTask );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_uxTaskPriorityGet 和 configUSE_MUTEXES 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 获取任意任务的基础优先级。
 *
 * @param xTask 要查询的任务句柄。传入 NULL 句柄将返回调用任务的基础优先级。
 *
 * @return xTask 的基础优先级。
 *
 * \defgroup uxTaskBasePriorityGet uxTaskBasePriorityGet
 * \ingroup TaskCtrl
 */
UBaseType_t uxTaskBasePriorityGet( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * UBaseType_t uxTaskBasePriorityGetFromISR( const TaskHandle_t xTask );
 * @endcode
 *
 * 可在 ISR 中使用的 uxTaskBasePriorityGet() 版本。
 */
UBaseType_t uxTaskBasePriorityGetFromISR( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * eTaskState eTaskGetState( TaskHandle_t xTask );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_eTaskGetState 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 获取任意任务的状态。状态由 eTaskState 枚举类型编码。
 *
 * @param xTask 要查询的任务句柄。
 *
 * @return 调用该函数时 xTask 的状态。注意，从调用该函数到调用任务
 * 测试其返回值之间，任务状态可能已经改变。
 */
#if ( ( INCLUDE_eTaskGetState == 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_xTaskAbortDelay == 1 ) )
    eTaskState eTaskGetState( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * void vTaskGetInfo( TaskHandle_t xTask, TaskStatus_t *pxTaskStatus, BaseType_t xGetFreeStackSpace, eTaskState eState );
 * @endcode
 *
 * 要使此函数可用，必须将 configUSE_TRACE_FACILITY 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 用任务的相关信息填充 TaskStatus_t 结构体。
 *
 * @param xTask 要查询的任务句柄。若 xTask 为 NULL，则返回调用任务的信息。
 *
 * @param pxTaskStatus 指向 TaskStatus_t 结构体的指针，该结构体将被填入
 * xTask 参数所传句柄引用任务的相关信息。
 *
 * @param xGetFreeStackSpace TaskStatus_t 结构体包含一个成员，用于报告
 * 被查询任务的栈高水位线。计算栈高水位线耗时相对较长，可能使系统
 * 暂时无响应——因此提供 xGetFreeStackSpace 参数，以便跳过高水位线检查。
 * 仅当 xGetFreeStackSpace 未设置为 pdFALSE 时，高水位线值才会写入
 * TaskStatus_t 结构体。
 *
 * @param eState TaskStatus_t 结构体包含一个成员，用于报告被查询任务的
 * 状态。获取任务状态并不像简单赋值那样快——因此提供 eState 参数，
 * 以便从 TaskStatus_t 结构体中省略状态信息。若要获取状态信息，
 * 请将 eState 设置为 eInvalid——否则，传入的 eState 值将作为任务状态
 * 报告到 TaskStatus_t 结构体中。
 *
 * 用法示例：
 * @code{c}
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 * TaskStatus_t xTaskDetails;
 *
 *  // 根据任务名称获取任务句柄。
 *  xHandle = xTaskGetHandle( "Task_Name" );
 *
 *  // 检查句柄不为 NULL。
 *  configASSERT( xHandle );
 *
 *  // 使用句柄获取任务的更多信息。
 *  vTaskGetInfo( xHandle,
 *                &xTaskDetails,
 *                pdTRUE, // 在 xTaskDetails 中包含高水位线。
 *                eInvalid ); // 在 xTaskDetails 中包含任务状态。
 * }
 * @endcode
 * \defgroup vTaskGetInfo vTaskGetInfo
 * \ingroup TaskCtrl
 */
#if ( configUSE_TRACE_FACILITY == 1 )
    void vTaskGetInfo( TaskHandle_t xTask,
                       TaskStatus_t * pxTaskStatus,
                       BaseType_t xGetFreeStackSpace,
                       eTaskState eState ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_vTaskPrioritySet 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 设置任意任务的优先级。
 *
 * 若所设置的优先级高于当前正在执行的任务，则函数返回前将发生上下文切换。
 *
 * @param xTask 要设置优先级的任务句柄。传入 NULL 句柄将设置调用任务的优先级。
 *
 * @param uxNewPriority 任务将被设置的优先级。
 *
 * 用法示例：
 * @code{c}
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 *
 *   // 创建任务并保存句柄。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *   // ...
 *
 *   // 使用句柄提高所创建任务的优先级。
 *   vTaskPrioritySet( xHandle, tskIDLE_PRIORITY + 1 );
 *
 *   // ...
 *
 *   // 使用 NULL 句柄将我们的优先级提高到相同的值。
 *   vTaskPrioritySet( NULL, tskIDLE_PRIORITY + 1 );
 * }
 * @endcode
 * \defgroup vTaskPrioritySet vTaskPrioritySet
 * \ingroup TaskCtrl
 */
void vTaskPrioritySet( TaskHandle_t xTask,
                       UBaseType_t uxNewPriority ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * void vTaskSuspend( TaskHandle_t xTaskToSuspend );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_vTaskSuspend 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 挂起任意任务。任务处于挂起态时，无论其优先级多高，
 * 都不会获得任何微控制器处理时间。
 *
 * 对 vTaskSuspend 的调用不是累积的——
 * 即对同一任务调用两次 vTaskSuspend()，仍然只需一次
 * vTaskResume() 调用即可使该挂起任务进入就绪态。
 *
 * @param xTaskToSuspend 要挂起的任务句柄。传入 NULL 句柄将挂起调用任务。
 *
 * 用法示例：
 * @code{c}
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 *
 *   // 创建任务并保存句柄。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *   // ...
 *
 *   // 使用句柄挂起所创建的任务。
 *   vTaskSuspend( xHandle );
 *
 *   // ...
 *
 *   // 在此期间，所创建的任务不会运行，除非
 *   // 另一个任务调用 vTaskResume( xHandle )。
 *
 *   //...
 *
 *
 *   // 挂起自身。
 *   vTaskSuspend( NULL );
 *
 *   // 除非另一个任务以我们的句柄为参数调用 vTaskResume，
 *   // 否则无法执行到这里。
 * }
 * @endcode
 * \defgroup vTaskSuspend vTaskSuspend
 * \ingroup TaskCtrl
 */
void vTaskSuspend( TaskHandle_t xTaskToSuspend ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * void vTaskResume( TaskHandle_t xTaskToResume );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_vTaskSuspend 定义为 1。
 * 更多信息请参见配置章节。
 *
 * 恢复被挂起的任务。
 *
 * 通过一次或多次调用 vTaskSuspend() 挂起的任务，
 * 只需一次 vTaskResume() 调用即可重新进入可运行状态。
 *
 * @param xTaskToResume 要恢复为就绪态的任务句柄。
 *
 * 用法示例：
 * @code{c}
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 *
 *   // 创建任务并保存句柄。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
 *
 *   // ...
 *
 *   // 使用句柄挂起所创建的任务。
 *   vTaskSuspend( xHandle );
 *
 *   // ...
 *
 *   // 在此期间，所创建的任务不会运行，除非
 *   // 另一个任务调用 vTaskResume( xHandle )。
 *
 *   //...
 *
 *
 *   // 由我们自己恢复被挂起的任务。
 *   vTaskResume( xHandle );
 *
 *   // 所创建的任务将根据其在系统中的优先级，
 *   // 再次获得微控制器处理时间。
 * }
 * @endcode
 * \defgroup vTaskResume vTaskResume
 * \ingroup TaskCtrl
 */
void vTaskResume( TaskHandle_t xTaskToResume ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * void xTaskResumeFromISR( TaskHandle_t xTaskToResume );
 * @endcode
 *
 * 要使此函数可用，必须将 INCLUDE_xTaskResumeFromISR 定义为 1。
 * 更多信息请参见配置章节。
 *
 * vTaskResume() 的一种实现，可在 ISR 内部调用。
 *
 * 通过一次或多次调用 vTaskSuspend() 挂起的任务，
 * 只需一次 xTaskResumeFromISR() 调用即可重新进入可运行状态。
 *
 * 若中断可能在任务被挂起之前到达，则不应使用 xTaskResumeFromISR()
 * 来实现任务与中断的同步——因为这可能导致中断丢失。
 * 使用信号量作为同步机制可以避免这种情况。
 *
 * @param xTaskToResume 要恢复为就绪态的任务句柄。
 *
 * @return 若恢复该任务应导致上下文切换，则返回 pdTRUE，
 * 否则返回 pdFALSE。ISR 用它来判断中断返回后是否需要上下文切换。
 *
 * \defgroup vTaskResumeFromISR vTaskResumeFromISR
 * \ingroup TaskCtrl
 */
BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume ) PRIVILEGED_FUNCTION;

#if ( configUSE_CORE_AFFINITY == 1 )

/**
 * @brief 设置任务的核心亲和性掩码。
 *
 * 该掩码设置任务可在哪些核心上运行。要使此函数可用，
 * 必须将 configUSE_CORE_AFFINITY 定义为 1。
 *
 * @param xTask 要设置核心亲和性掩码的任务句柄。
 * 传入 NULL 将设置调用任务的核心亲和性掩码。
 *
 * @param uxCoreAffinityMask 一个按位值，指示任务可在哪些核心上运行。
 * 核心编号从 0 到 configNUMBER_OF_CORES - 1。
 * 例如，要确保任务可在核心 0 和核心 1 上运行，
 * 应将 uxCoreAffinityMask 设置为 0x03。
 *
 * 用法示例：
 *
 * // 创建任务的函数。
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 * UBaseType_t uxCoreAffinityMask;
 *
 *      // 创建任务并保存句柄。
 *      xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &( xHandle ) );
 *
 *      // 定义核心亲和性掩码，使此任务只能在
 *      // 核心 0 和核心 2 上运行。
 *      uxCoreAffinityMask = ( ( 1 << 0 ) | ( 1 << 2 ) );
 *
 *      // 设置任务的核心亲和性掩码。
 *      vTaskCoreAffinitySet( xHandle, uxCoreAffinityMask );
 * }
 */
    void vTaskCoreAffinitySet( const TaskHandle_t xTask,
                               UBaseType_t uxCoreAffinityMask );
#endif

#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )

/**
 * @brief 获取任务的核心亲和性掩码。
 *
 * 要使此函数可用，必须将 configUSE_CORE_AFFINITY 定义为 1。
 *
 * @param xTask 要获取核心亲和性掩码的任务句柄。
 * 传入 NULL 将获取调用任务的核心亲和性掩码。
 *
 * @return 核心亲和性掩码，它是一个按位值，指示任务可在哪些核心上运行。
 * 核心编号从 0 到 configNUMBER_OF_CORES - 1。例如，若任务可在核心 0
 * 和核心 1 上运行，则核心亲和性掩码为 0x03。
 *
 * 用法示例：
 *
 * // 网络任务的句柄——它在其他地方被填充。
 * TaskHandle_t xNetworkingTaskHandle;
 *
 * void vAFunction( void )
 * {
 * TaskHandle_t xHandle;
 * UBaseType_t uxNetworkingCoreAffinityMask;
 *
 *     // 创建任务并保存句柄。
 *     xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &( xHandle ) );
 *
 *     // 获取网络任务的核心亲和性掩码。
 *     uxNetworkingCoreAffinityMask = vTaskCoreAffinityGet( xNetworkingTaskHandle );
 *
 *     // 以下仅为示例的假设场景。假设我们有 2 个核心——核心 0 和核心 1。
 *     // 我们希望将应用任务固定到与网络任务不同的核心上，
 *     // 以确保应用任务不会干扰网络。
 *     if( ( uxNetworkingCoreAffinityMask & ( 1 << 0 ) ) != 0 )
 *     {
 *         // 网络任务可在核心 0 上运行，将我们的任务固定到核心 1。
 *         vTaskCoreAffinitySet( xHandle, ( 1 << 1 ) );
 *     }
 *     else
 *     {
 *         // 否则，将我们的任务固定到核心 0。
 *         vTaskCoreAffinitySet( xHandle, ( 1 << 0 ) );
 *     }
 * }
 */
    UBaseType_t vTaskCoreAffinityGet( ConstTaskHandle_t xTask );
#endif

#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )

/**
 * @brief 禁止任务被抢占。
 *
 * @param xTask 要禁止抢占的任务句柄。传入 NULL
 * 将禁止调用任务被抢占。
 *
 * 用法示例：
 *
 * void vTaskCode( void *pvParameters )
 * {
 *     // 消除未使用参数的警告。
 *     ( void ) pvParameters;
 *
 *     for( ;; )
 *     {
 *         // ... 在此执行某些功能。
 *
 *         // 禁止此任务被抢占。
 *         vTaskPreemptionDisable( NULL );
 *
 *         // 任务在此段代码执行期间不会被抢占……
 *
 *         // ... 直到再次使能抢占。
 *         vTaskPreemptionEnable( NULL );
 *
 *         // 任务在此段代码执行期间可被抢占。
 *     }
 * }
 */
    void vTaskPreemptionDisable( const TaskHandle_t xTask );
#endif

#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )

/**
 * @brief 使能任务被抢占。
 *
 * @param xTask 要使能被抢占的任务句柄。传入 NULL
 * 将使能调用任务被抢占。
 *
 * 用法示例：
 *
 * void vTaskCode( void *pvParameters )
 * {
 *     // 消除未使用参数的警告。
 *     ( void ) pvParameters;
 *
 *     for( ;; )
 *     {
 *         // ... 在此执行某些功能。
 *
 *         // 禁止此任务被抢占。
 *         vTaskPreemptionDisable( NULL );
 *
 *         // 任务在此段代码执行期间不会被抢占……
 *
 *         // ... 直到再次使能抢占。
 *         vTaskPreemptionEnable( NULL );
 *
 *         // 任务在此段代码执行期间可被抢占。
 *     }
 * }
 */
    void vTaskPreemptionEnable( const TaskHandle_t xTask );
#endif

/*
 * -----------------------------------------------------------
 * 调度器控制
 * ----------------------------------------------------------
 */

/**
 * task. h
 * @code{c}
 * void vTaskStartScheduler( void );
 * @endcode
 *
 * 启动实时内核的节拍处理。调用后，内核将控制哪些任务执行以及何时执行。
 *
 * 有关创建任务并启动内核的示例，请参见演示应用程序文件 main.c。
 *
 * 用法示例：
 * @code{c}
 * void vAFunction( void )
 * {
 *   // 启动内核前至少创建一个任务。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
 *
 *   // 以抢占方式启动实时内核。
 *   vTaskStartScheduler ();
 *
 *   // 除非某个任务调用 vTaskEndScheduler()，否则不会执行到这里。
 * }
 * @endcode
 *
 * \defgroup vTaskStartScheduler vTaskStartScheduler
 * \ingroup SchedulerControl
 */
void vTaskStartScheduler( void ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * void vTaskEndScheduler( void );
 * @endcode
 *
 * 注意：撰写本文时，只有运行在 PC 上替代 DOS 的 x86 实模式移植层实现了此函数。
 *
 * 停止实时内核节拍。所有已创建的任务都将被自动删除，多任务
 * （抢占式或协作式）将停止。随后从调用 vTaskStartScheduler() 的位置
 * 恢复执行，就像 vTaskStartScheduler() 刚刚返回一样。
 *
 * 有关使用 vTaskEndScheduler() 的示例，请参见 demo/PC 目录中的
 * 演示应用程序文件 main.c。
 *
 * vTaskEndScheduler() 要求在移植层中定义一个退出函数（PC 移植层参见
 * port.c 中的 vPortEndScheduler()）。该函数执行与硬件相关的操作，
 * 例如停止内核节拍。
 *
 * vTaskEndScheduler() 将释放内核分配的所有资源——但不会释放
 * 应用程序任务分配的资源。
 *
 * 用法示例：
 * @code{c}
 * void vTaskCode( void * pvParameters )
 * {
 *   for( ;; )
 *   {
 *       // 任务代码写在这里。
 *
 *       // 在某一时刻我们想结束实时内核处理，
 *       // 因此调用……
 *       vTaskEndScheduler ();
 *   }
 * }
 *
 * void vAFunction( void )
 * {
 *   // 启动内核前至少创建一个任务。
 *   xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
 *
 *   // 以抢占方式启动实时内核。
 *   vTaskStartScheduler ();
 *
 *   // 只有 vTaskCode() 任务调用了 vTaskEndScheduler() 时才会执行到这里。
 *   // 执行到这里时，我们已回到单任务执行状态。
 * }
 * @endcode
 *
 * \defgroup vTaskEndScheduler vTaskEndScheduler
 * \ingroup SchedulerControl
 */
void vTaskEndScheduler( void ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * void vTaskSuspendAll( void );
 * @endcode
 *
 * 挂起调度器，但不禁止中断。调度器挂起期间不会发生上下文切换。
 *
 * 调用 vTaskSuspendAll() 后，调用任务将继续执行，
 * 不会被换出，直到调用 xTaskResumeAll()。
 *
 * 在调度器挂起期间，不得调用可能导致上下文切换的 API 函数
 * （例如 xTaskDelayUntil()、xQueueSend() 等）。
 *
 * 用法示例：
 * @code{c}
 * void vTask1( void * pvParameters )
 * {
 *   for( ;; )
 *   {
 *       // 任务代码写在这里。
 *
 *       // ...
 *
 *       // 在某一时刻，任务希望执行一个耗时较长的操作，
 *       // 在此期间不希望被换出。它不能使用
 *       // taskENTER_CRITICAL()/taskEXIT_CRITICAL()，因为该操作
 *       // 的持续时间可能导致中断被错过——包括节拍中断。
 *
 *       // 防止实时内核将任务换出。
 *       vTaskSuspendAll ();
 *
 *       // 在此执行操作。无需使用临界区，因为我们拥有
 *       // 全部微控制器处理时间。在此期间中断仍会正常工作，
 *       // 内核节拍计数也会继续维护。
 *
 *       // ...
 *
 *       // 操作完成。重新启动内核。
 *       xTaskResumeAll ();
 *   }
 * }
 * @endcode
 * \defgroup vTaskSuspendAll vTaskSuspendAll
 * \ingroup SchedulerControl
 */
void vTaskSuspendAll( void ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskResumeAll( void );
 * @endcode
 *
 * 在调用 vTaskSuspendAll() 挂起调度器后，恢复调度器活动。
 *
 * xTaskResumeAll() 仅恢复调度器。它不会恢复先前因调用 vTaskSuspend()
 * 而挂起的任务。
 *
 * @return 若恢复调度器导致了上下文切换，则返回 pdTRUE，
 *         否则返回 pdFALSE。
 *
 * 用法示例：
 * @code{c}
 * void vTask1( void * pvParameters )
 * {
 *   for( ;; )
 *   {
 *       // 任务代码写在这里。
 *
 *       // ...
 *
 *       // 在某一时刻，任务希望执行一个耗时较长的操作，
 *       // 在此期间不希望被换出。它不能使用
 *       // taskENTER_CRITICAL()/taskEXIT_CRITICAL()，因为该操作
 *       // 的持续时间可能导致中断被错过——包括节拍中断。
 *
 *       // 防止实时内核将任务换出。
 *       vTaskSuspendAll ();
 *
 *       // 在此执行操作。无需使用临界区，因为我们拥有
 *       // 全部微控制器处理时间。在此期间中断仍会正常工作，
 *       // 实时内核节拍计数也会继续维护。
 *
 *       // ...
 *
 *       // 操作完成。重新启动内核。我们希望强制进行
 *       // 上下文切换——但如果恢复调度器已经导致上下文切换，
 *       // 就没有必要再强制切换。
 *       if( !xTaskResumeAll () )
 *       {
 *            taskYIELD ();
 *       }
 *   }
 * }
 * @endcode
 * \defgroup xTaskResumeAll xTaskResumeAll
 * \ingroup SchedulerControl
 */
BaseType_t xTaskResumeAll( void ) PRIVILEGED_FUNCTION;

/*
 * -----------------------------------------------------------
 * 任务工具
 * ----------------------------------------------------------
 */

/**
 * task. h
 * @code{c}
 * TickType_t xTaskGetTickCount( void );
 * @endcode
 *
 * @return 自调用 vTaskStartScheduler 以来的节拍计数。
 *
 * \defgroup xTaskGetTickCount xTaskGetTickCount
 * \ingroup TaskUtils
 */
TickType_t xTaskGetTickCount( void ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * TickType_t xTaskGetTickCountFromISR( void );
 * @endcode
 *
 * @return 自调用 vTaskStartScheduler 以来的节拍计数。
 *
 * 这是 xTaskGetTickCount() 的一个版本，可以安全地从 ISR 中调用——
 * 前提是 TickType_t 为所用微控制器的自然字长，或者不支持中断嵌套，
 * 或者未使用中断嵌套。
 *
 * \defgroup xTaskGetTickCountFromISR xTaskGetTickCountFromISR
 * \ingroup TaskUtils
 */
TickType_t xTaskGetTickCountFromISR( void ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * uint16_t uxTaskGetNumberOfTasks( void );
 * @endcode
 *
 * @return 实时内核当前管理的任务数量。
 * 这包括所有就绪、阻塞和挂起的任务。已被删除但尚未由空闲任务
 * 释放的任务也会计入该数量。
 *
 * \defgroup uxTaskGetNumberOfTasks uxTaskGetNumberOfTasks
 * \ingroup TaskUtils
 */
UBaseType_t uxTaskGetNumberOfTasks( void ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * char *pcTaskGetName( TaskHandle_t xTaskToQuery );
 * @endcode
 *
 * @return 句柄 xTaskToQuery 所引用任务的文本（人类可读）名称。
 * 任务可以通过传入自身句柄，或将 xTaskToQuery 设置为 NULL，
 * 来查询自己的名称。
 *
 * \defgroup pcTaskGetName pcTaskGetName
 * \ingroup TaskUtils
 */
char * pcTaskGetName( TaskHandle_t xTaskToQuery ) PRIVILEGED_FUNCTION;

/**
 * task. h
 * @code{c}
 * TaskHandle_t xTaskGetHandle( const char *pcNameToQuery );
 * @endcode
 *
 * 注意：此函数执行时间相对较长，应尽量少用。
 *
 * @return 具有人类可读名称 pcNameToQuery 的任务句柄。
 * 若未找到匹配名称，则返回 NULL。要使 pcTaskGetHandle() 可用，
 * 必须在 FreeRTOSConfig.h 中将 INCLUDE_xTaskGetHandle 设置为 1。
 *
 * \defgroup pcTaskGetHandle pcTaskGetHandle
 * \ingroup TaskUtils
 */
#if ( INCLUDE_xTaskGetHandle == 1 )
    TaskHandle_t xTaskGetHandle( const char * pcNameToQuery ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskGetStaticBuffers( TaskHandle_t xTask,
 *                                   StackType_t ** ppuxStackBuffer,
 *                                   StaticTask_t ** ppxTaskBuffer );
 * @endcode
 *
 * 获取指向静态创建任务的数据结构缓冲区和栈缓冲区的指针。
 * 这些缓冲区与创建任务时提供的缓冲区相同。
 *
 * @param xTask 要获取缓冲区的任务。
 *
 * @param ppuxStackBuffer 用于返回指向任务栈缓冲区的指针。
 *
 * @param ppxTaskBuffer 用于返回指向任务数据结构缓冲区的指针。
 *
 * @return 若成功获取缓冲区，返回 pdTRUE；否则返回 pdFALSE。
 *
 * \defgroup xTaskGetStaticBuffers xTaskGetStaticBuffers
 * \ingroup TaskUtils
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xTaskGetStaticBuffers( TaskHandle_t xTask,
                                      StackType_t ** ppuxStackBuffer,
                                      StaticTask_t ** ppxTaskBuffer ) PRIVILEGED_FUNCTION;
#endif /* configSUPPORT_STATIC_ALLOCATION（静态分配支持） */

/**
 * task.h
 * @code{c}
 * UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask );
 * @endcode
 *
 * 要使此函数可用，必须在 FreeRTOSConfig.h 中将
 * INCLUDE_uxTaskGetStackHighWaterMark 设置为 1。
 *
 * 返回与 xTask 关联的栈的高水位线。即任务启动以来
 * 曾出现过的最小空闲栈空间（以字为单位，因此在 32 位机器上
 * 值为 1 表示 4 字节）。返回值越小，说明任务越接近栈溢出。
 *
 * uxTaskGetStackHighWaterMark() 与 uxTaskGetStackHighWaterMark2() 除返回类型外
 * 完全相同。使用 configSTACK_DEPTH_TYPE 可以让用户决定返回类型。
 * 这样既解决了 8 位类型上的数值溢出问题，又不会破坏
 * 期望 8 位返回类型的应用程序的向后兼容性。
 *
 * @param xTask 要检查栈的任务句柄。
 * 将 xTask 设置为 NULL 可检查调用任务的栈。
 *
 * @return 自 xTask 引用的任务创建以来，曾出现过的最小空闲栈空间
 * （以字为单位，即栈上的实际空间而非字节数）。
 */
#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
    UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task.h
 * @code{c}
 * configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask );
 * @endcode
 *
 * 要使此函数可用，必须在 FreeRTOSConfig.h 中将
 * INCLUDE_uxTaskGetStackHighWaterMark2 设置为 1。
 *
 * 返回与 xTask 关联的栈的高水位线。即任务启动以来
 * 曾出现过的最小空闲栈空间（以字为单位，因此在 32 位机器上
 * 值为 1 表示 4 字节）。返回值越小，说明任务越接近栈溢出。
 *
 * uxTaskGetStackHighWaterMark() 与 uxTaskGetStackHighWaterMark2() 除返回类型外
 * 完全相同。使用 configSTACK_DEPTH_TYPE 可以让用户决定返回类型。
 * 这样既解决了 8 位类型上的数值溢出问题，又不会破坏
 * 期望 8 位返回类型的应用程序的向后兼容性。
 *
 * @param xTask 要检查栈的任务句柄。
 * 将 xTask 设置为 NULL 可检查调用任务的栈。
 *
 * @return 自 xTask 引用的任务创建以来，曾出现过的最小空闲栈空间
 * （以字为单位，即栈上的实际空间而非字节数）。
 */
#if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )
    configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif

/*
 * 使用跟踪宏时，有时需要在 FreeRTOS.h 之前包含 task.h。
 * 此时 TaskHookFunction_t 尚未定义，
 * 因此下面两个原型会导致编译错误。解决方法很简单：
 * 除非 configUSE_APPLICATION_TASK_TAG 配置常量明确要求
 * 这两个原型，否则对它们的包含加以保护。
 */
#ifdef configUSE_APPLICATION_TASK_TAG
    #if configUSE_APPLICATION_TASK_TAG == 1

/**
 * task.h
 * @code{c}
 * void vTaskSetApplicationTaskTag( TaskHandle_t xTask, TaskHookFunction_t pxHookFunction );
 * @endcode
 *
 * 将 pxHookFunction 设置为任务 xTask 使用的任务钩子函数。
 * xTask 传入 NULL 时，效果是设置调用任务的钩子函数。
 */
        void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
                                         TaskHookFunction_t pxHookFunction ) PRIVILEGED_FUNCTION;

/**
 * task.h
 * @code{c}
 * void xTaskGetApplicationTaskTag( TaskHandle_t xTask );
 * @endcode
 *
 * 返回分配给任务 xTask 的 pxHookFunction 值。
 * 不要在中断服务程序中调用——请改为调用
 * xTaskGetApplicationTaskTagFromISR()。
 */
        TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

/**
 * task.h
 * @code{c}
 * void xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask );
 * @endcode
 *
 * 返回分配给任务 xTask 的 pxHookFunction 值。
 * 可从中断服务程序中调用。
 */
        TaskHookFunction_t xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
    #endif /* configUSE_APPLICATION_TASK_TAG ==1（应用任务标签功能使能条件） */
#endif /* ifdef configUSE_APPLICATION_TASK_TAG（应用任务标签条件编译） */

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )

/*
 * 每个任务包含一个指针数组，其大小由 FreeRTOSConfig.h 中的
 * configNUM_THREAD_LOCAL_STORAGE_POINTERS 设置决定。
 * 内核自身不使用这些指针，因此应用程序编写者可以将其用于任何目的。
 * 下面两个函数分别用于设置和查询指针。
 */
    void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet,
                                            BaseType_t xIndex,
                                            void * pvValue ) PRIVILEGED_FUNCTION;
    void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery,
                                               BaseType_t xIndex ) PRIVILEGED_FUNCTION;

#endif

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )

/**
 * task.h
 * @code{c}
 * void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName);
 * @endcode
 *
 * 当检测到任务发生栈溢出时，会调用应用程序栈溢出钩子函数。
 *
 * 有关栈溢出检测的详细信息，请参见：https://www.FreeRTOS.org/Stacks-and-stack-overflow-checking.html
 *
 * @param xTask 刚刚超出其栈边界的任务。
 * @param pcTaskName 包含违规任务名称的字符串。
 */
    /* MISRA 参考 8.6.1 [外部链接] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-86 */
    /* coverity[misra_c_2012_rule_8_6_violation] —— MISRA C:2012 规则 8.6 违规抑制 */
    void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char * pcTaskName );

#endif

#if ( configUSE_IDLE_HOOK == 1 )

/**
 * task.h
 * @code{c}
 * void vApplicationIdleHook( void );
 * @endcode
 *
 * 应用程序空闲钩子由空闲任务调用。
 * 这使应用设计者能够在无需额外任务开销的情况下添加后台功能。
 * 注意：vApplicationIdleHook() 在任何情况下都不得调用可能阻塞的函数。
 */
    /* MISRA 参考 8.6.1 [外部链接] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-86 */
    /* coverity[misra_c_2012_rule_8_6_violation] —— MISRA C:2012 规则 8.6 违规抑制 */
    void vApplicationIdleHook( void );

#endif


#if  ( configUSE_TICK_HOOK != 0 )

/**
 * task.h
 * @code{c}
 * void vApplicationTickHook( void );
 * @endcode
 *
 * 此钩子函数在系统节拍处理程序中、所有操作系统工作完成后被调用。
 */
    /* MISRA 参考 8.6.1 [外部链接] */
    /* 更多详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-86 */
    /* coverity[misra_c_2012_rule_8_6_violation] —— MISRA C:2012 规则 8.6 违规抑制 */
    void vApplicationTickHook( void );

#endif

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

/**
 * task.h
 * @code{c}
 * void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, configSTACK_DEPTH_TYPE * puxIdleTaskStackSize )
 * @endcode
 *
 * 此函数用于向 FreeRTOS 提供一块静态分配的内存，以保存空闲任务的 TCB。
 * 当 configSUPPORT_STATIC_ALLOCATION 设置时需要使用此函数。
 * 更多信息请参见：https://www.FreeRTOS.org/a00110.html#configSUPPORT_STATIC_ALLOCATION
 *
 * @param ppxIdleTaskTCBBuffer 指向静态分配的 TCB 缓冲区的句柄
 * @param ppxIdleTaskStackBuffer 指向空闲任务静态分配栈缓冲区的句柄
 * @param puxIdleTaskStackSize 指向所分配栈缓冲区可容纳元素数量的指针
 */
    void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                        StackType_t ** ppxIdleTaskStackBuffer,
                                        configSTACK_DEPTH_TYPE * puxIdleTaskStackSize );

/**
 * task.h
 * @code{c}
 * void vApplicationGetPassiveIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, configSTACK_DEPTH_TYPE * puxIdleTaskStackSize, BaseType_t xCoreID )
 * @endcode
 *
 * 此函数用于向 FreeRTOS 提供一块静态分配的内存，以保存空闲任务的 TCB。
 * 当 configSUPPORT_STATIC_ALLOCATION 设置时需要使用此函数。
 * 更多信息请参见：https://www.FreeRTOS.org/a00110.html#configSUPPORT_STATIC_ALLOCATION
 *
 * 在 FreeRTOS SMP 中，总共有 configNUMBER_OF_CORES 个空闲任务：
 *  1. 1 个活动空闲任务，负责所有日常维护工作。
 *  2. ( configNUMBER_OF_CORES - 1 ) 个被动空闲任务，不执行任何操作。
 * 创建这些空闲任务，是为了确保每个核心在没有其他任务可运行时都有空闲任务可运行。
 *
 * 调用 vApplicationGetPassiveIdleTaskMemory 时，传入的被动空闲任务索引依次为
 * 0、1 ... ( configNUMBER_OF_CORES - 2 )，以获取各被动空闲任务的内存。
 *
 * @param ppxIdleTaskTCBBuffer 指向静态分配的 TCB 缓冲区的句柄
 * @param ppxIdleTaskStackBuffer 指向空闲任务静态分配栈缓冲区的句柄
 * @param puxIdleTaskStackSize 指向所分配栈缓冲区可容纳元素数量的指针
 * @param xPassiveIdleTaskIndex 空闲任务缓冲区对应的被动空闲任务索引
 */
    #if ( configNUMBER_OF_CORES > 1 )
        void vApplicationGetPassiveIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                                   StackType_t ** ppxIdleTaskStackBuffer,
                                                   configSTACK_DEPTH_TYPE * puxIdleTaskStackSize,
                                                   BaseType_t xPassiveIdleTaskIndex );
    #endif /* #if ( configNUMBER_OF_CORES > 1 )（多核条件） */
#endif /* if ( configSUPPORT_STATIC_ALLOCATION == 1 )（静态分配支持条件） */

/**
 * task.h
 * @code{c}
 * BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask, void *pvParameter );
 * @endcode
 *
 * 调用与 xTask 关联的钩子函数。xTask 传入 NULL 时，
 * 效果是调用运行任务（即调用任务）的钩子函数。
 *
 * pvParameter 会传给钩子函数，由任务自行解释。
 * 返回值是用户注册的任务钩子函数所返回的值。
 */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
    BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask,
                                             void * pvParameter ) PRIVILEGED_FUNCTION;
#endif

/**
 * xTaskGetIdleTaskHandle() 仅在 FreeRTOSConfig.h 中
 * INCLUDE_xTaskGetIdleTaskHandle 设置为 1 时可用。
 *
 * 在单核 FreeRTOS 中，此函数仅返回空闲任务的句柄。
 * 在调度器启动之前调用 xTaskGetIdleTaskHandle() 是无效的。
 *
 * 在 FreeRTOS SMP 中，总共有 configNUMBER_OF_CORES 个空闲任务：
 *  1. 1 个活动空闲任务，负责所有日常维护工作。
 *  2. ( configNUMBER_OF_CORES - 1 ) 个被动空闲任务，不执行任何操作。
 * 创建这些空闲任务，是为了确保每个核心在没有其他任务可运行时都有空闲任务可运行。
 * 调用 xTaskGetIdleTaskHandle() 或 xTaskGetIdleTaskHandleForCore()，并将 xCoreID
 * 设置为 0，可获取活动空闲任务的句柄。调用 xTaskGetIdleTaskHandleForCore()，
 * 并将 xCoreID 设置为 1、2 ... ( configNUMBER_OF_CORES - 1 )，
 * 可获取各被动空闲任务的句柄。
 */
#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
    #if ( configNUMBER_OF_CORES == 1 )
        TaskHandle_t xTaskGetIdleTaskHandle( void ) PRIVILEGED_FUNCTION;
    #endif /* #if ( configNUMBER_OF_CORES == 1 )（单核条件） */

    TaskHandle_t xTaskGetIdleTaskHandleForCore( BaseType_t xCoreID ) PRIVILEGED_FUNCTION;
#endif /* #if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )（空闲任务句柄功能使能条件） */

/**
 * 要使 uxTaskGetSystemState() 可用，必须在 FreeRTOSConfig.h 中
 * 将 configUSE_TRACE_FACILITY 定义为 1。
 *
 * uxTaskGetSystemState() 为系统中的每个任务填充一个 TaskStatus_t 结构体。
 * TaskStatus_t 结构体包含任务句柄、任务名称、任务优先级、任务状态以及
 * 任务消耗的总运行时间等成员。完整成员列表请参见本文件中的
 * TaskStatus_t 结构体定义。
 *
 * 注意：此函数仅供调试使用，因为使用它会使调度器长时间保持挂起。
 *
 * @param pxTaskStatusArray 指向 TaskStatus_t 结构体数组的指针。
 * 对于受 RTOS 控制的每个任务，该数组必须至少包含一个 TaskStatus_t 结构体。
 * 受 RTOS 控制的任务数量可通过 uxTaskGetNumberOfTasks() API 函数确定。
 *
 * @param uxArraySize pxTaskStatusArray 参数所指向数组的大小。该大小以
 * 数组中的索引个数或数组包含的 TaskStatus_t 结构体个数表示，
 * 而不是数组的字节数。
 *
 * @param pulTotalRunTime 若 FreeRTOSConfig.h 中 configGENERATE_RUN_TIME_STATS
 * 设置为 1，则 uxTaskGetSystemState() 会将 *pulTotalRunTime 设置为目标启动以来
 * 的总运行时间（按运行时统计时钟定义，参见
 * https://www.FreeRTOS.org/rtos-run-time-stats.html）。
 * pulTotalRunTime 可设置为 NULL，以省略总运行时间信息。
 *
 * @return uxTaskGetSystemState() 填充的 TaskStatus_t 结构体数量。
 * 该值应等于 uxTaskGetNumberOfTasks() API 函数返回的数量；
 * 但如果 uxArraySize 参数传入的值过小，则返回值为零。
 *
 * 用法示例：
 * @code{c}
 *  // 此示例演示如何根据 uxTaskGetSystemState() 提供的原始数据
 *  // 生成人类可读的运行时间统计表。
 *  // 人类可读的表格写入 pcWriteBuffer。
 *  void vTaskGetRunTimeStats( char *pcWriteBuffer )
 *  {
 *  TaskStatus_t *pxTaskStatusArray;
 *  volatile UBaseType_t uxArraySize, x;
 *  configRUN_TIME_COUNTER_TYPE ulTotalRunTime, ulStatsAsPercentage;
 *
 *      // 确保写入缓冲区中不包含字符串。
 * pcWriteBuffer = 0x00;
 *
 *      // 对任务数量进行快照，以防此函数执行期间任务数量发生变化。
 *      uxArraySize = uxTaskGetNumberOfTasks();
 *
 *      // 为每个任务分配一个 TaskStatus_t 结构体。也可以在编译时
 *      // 静态分配一个数组。
 *      pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
 *
 *      if( pxTaskStatusArray != NULL )
 *      {
 *          // 生成每个任务的原始状态信息。
 *          uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
 *
 *          // 用于百分比计算。
 *          ulTotalRunTime /= 100U;
 *
 *          // 避免除零错误。
 *          if( ulTotalRunTime > 0 )
 *          {
 *              // 对 pxTaskStatusArray 数组中每个已填充的位置，
 *              // 将原始数据格式化为人类可读的 ASCII 数据。
 *              for( x = 0; x < uxArraySize; x++ )
 *              {
 *                  // 该任务消耗了总运行时间的百分之多少？
 *                  // 结果始终向下取整到最接近的整数。
 *                  // ulTotalRunTimeDiv100 已经除以 100。
 *                  ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;
 *
 *                  if( ulStatsAsPercentage > 0U )
 *                  {
 *                      sprintf( pcWriteBuffer, "%s\t\t%lu\t\t%lu%%\r\n", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage );
 *                  }
 *                  else
 *                  {
 *                      // 若此处百分比为零，说明该任务消耗的总运行时间
 *                      // 不足 1%。
 *                      sprintf( pcWriteBuffer, "%s\t\t%lu\t\t<1%%\r\n", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter );
 *                  }
 *
 *                  pcWriteBuffer += strlen( ( char * ) pcWriteBuffer );
 *              }
 *          }
 *
 *          // 不再需要该数组，释放其占用的内存。
 *          vPortFree( pxTaskStatusArray );
 *      }
 *  }
 *  @endcode
 */
#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                      const UBaseType_t uxArraySize,
                                      configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime ) PRIVILEGED_FUNCTION;


/**
 * 对每个任务，以任务句柄、状态以及提供的上下文调用 pxCallbackFunction。
 *
 * 注意：此函数仅供调试使用，因为它会使调度器长时间保持挂起。
 * 回调在调度器挂起期间运行，因此必须快速返回，且不得执行阻塞操作。
 *
 * 注意：此 API 仅限特权态使用（它会从特权上下文调用用户回调）。
 *
 * @param pxCallbackFunction 对每个任务调用一次的回调（传入任务句柄、
 * 状态和 pvCallbackContext）。
 *
 * @param pvCallbackContext 调用方提供的不透明上下文，原样传给每次回调调用。
 *
 * @param pulTotalRunTime 若 FreeRTOSConfig.h 中 configGENERATE_RUN_TIME_STATS
 * 设置为 1，则 *pulTotalRunTime 被设置为自启动以来的总运行时间。
 * pulTotalRunTime 可设置为 NULL，以省略总运行时间信息。
 *
 * @return 提供给回调的任务数量。
 */
    UBaseType_t uxTaskCallForEachTask( TaskStatusCallbackFunction_t pxCallbackFunction,
                                       void * pvCallbackContext,
                                       configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime ) PRIVILEGED_FUNCTION;

#endif /* if ( configUSE_TRACE_FACILITY == 1 )（跟踪功能使能条件） */

/**
 * task. h
 * @code{c}
 * void vTaskListTasks( char *pcWriteBuffer, size_t uxBufferLength );
 * @endcode
 *
 * 要使此函数可用，必须将 configUSE_TRACE_FACILITY 和
 * configUSE_STATS_FORMATTING_FUNCTIONS 都定义为 1。更多信息请参见
 * FreeRTOS.org 网站的配置章节。
 *
 * 注意 1：此函数在执行期间会禁止中断。它不用于正常的应用程序运行时，
 * 而是一种调试辅助手段。
 *
 * 列出所有当前任务，以及它们当前的状态和栈使用高水位线。
 *
 * 任务状态报告为运行（'X'）、阻塞（'B'）、就绪（'R'）、已删除（'D'）
 * 或挂起（'S'）。
 *
 * 请注意：
 *
 * 此函数仅为方便使用而提供，许多演示应用程序都在使用它。
 * 不要将其视为调度器的一部分。
 *
 * vTaskListTasks() 调用 uxTaskGetSystemState()，然后将 uxTaskGetSystemState()
 * 的部分输出格式化为人类可读的表格，按以下格式显示任务信息：
 * 任务名称、任务状态、任务优先级、任务栈高水位线、任务编号。
 *
 * 以下是一个输出示例：
 * Task A       X       2           67           2
 * Task B       R       1           67           3
 * IDLE         R       0           67           5
 * Tmr Svc      B       6           137          6
 *
 * 栈使用量以栈顶还能容纳的未使用 StackType_t 字数表示，
 * 而不是字节数。
 *
 * vTaskListTasks() 依赖 snprintf() C 库函数，这可能会增大代码体积、
 * 占用大量栈空间，并在不同平台上产生不同结果。许多 FreeRTOS/Demo
 * 子目录中提供了一个名为 printf-stdarg.c 的第三方替代实现，它小巧且
 * 功能有限（注意：printf-stdarg.c 并未提供完整的 snprintf() 实现！）。
 *
 * 建议生产系统直接调用 uxTaskGetSystemState() 获取原始统计数据，
 * 而不是通过调用 vTaskListTasks() 间接获取。
 *
 * @param pcWriteBuffer 用于以 ASCII 形式写入上述详细信息的缓冲区。
 * 假定该缓冲区足够大，能够容纳生成的报告。每个任务约 40 字节
 * 应该足够。
 *
 * @param uxBufferLength pcWriteBuffer 的长度。
 *
 * \defgroup vTaskListTasks vTaskListTasks
 * \ingroup TaskUtils
 */
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    void vTaskListTasks( char * pcWriteBuffer,
                         size_t uxBufferLength ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * void vTaskList( char *pcWriteBuffer );
 * @endcode
 *
 * 要使此函数可用，必须将 configUSE_TRACE_FACILITY 和
 * configUSE_STATS_FORMATTING_FUNCTIONS 都定义为 1。更多信息请参见
 * FreeRTOS.org 网站的配置章节。
 *
 * 警告：此函数假定 pcWriteBuffer 的长度为 configSTATS_BUFFER_MAX_LENGTH。
 * 保留此函数仅是为了向后兼容。建议新应用程序使用
 * vTaskListTasks，并显式提供 pcWriteBuffer 的长度。
 *
 * 注意 1：此函数在执行期间会禁止中断。它不用于正常的应用程序运行时，
 * 而是一种调试辅助手段。
 *
 * 列出所有当前任务，以及它们当前的状态和栈使用高水位线。
 *
 * 任务状态报告为运行（'X'）、阻塞（'B'）、就绪（'R'）、已删除（'D'）
 * 或挂起（'S'）。
 *
 * 请注意：
 *
 * 此函数仅为方便使用而提供，许多演示应用程序都在使用它。
 * 不要将其视为调度器的一部分。
 *
 * vTaskList() 调用 uxTaskGetSystemState()，然后将 uxTaskGetSystemState()
 * 的部分输出格式化为人类可读的表格，按以下格式显示任务信息：
 * 任务名称、任务状态、任务优先级、任务栈高水位线、任务编号。
 *
 * 以下是一个输出示例：
 * Task A       X       2           67           2
 * Task B       R       1           67           3
 * IDLE         R       0           67           5
 * Tmr Svc      B       6           137          6
 *
 * 栈使用量以栈顶还能容纳的未使用 StackType_t 字数表示，
 * 而不是字节数。
 *
 * vTaskList() 依赖 snprintf() C 库函数，这可能会增大代码体积、
 * 占用大量栈空间，并在不同平台上产生不同结果。许多 FreeRTOS/Demo
 * 子目录中提供了一个名为 printf-stdarg.c 的第三方替代实现，它小巧且
 * 功能有限（注意：printf-stdarg.c 并未提供完整的 snprintf() 实现！）。
 *
 * 建议生产系统直接调用 uxTaskGetSystemState() 获取原始统计数据，
 * 而不是通过调用 vTaskList() 间接获取。
 *
 * @param pcWriteBuffer 用于以 ASCII 形式写入上述详细信息的缓冲区。
 * 假定该缓冲区足够大，能够容纳生成的报告。每个任务约 40 字节
 * 应该足够。
 *
 * \defgroup vTaskList vTaskList
 * \ingroup TaskUtils
 */
#define vTaskList( pcWriteBuffer )    vTaskListTasks( ( pcWriteBuffer ), configSTATS_BUFFER_MAX_LENGTH )

/**
 * task. h
 * @code{c}
 * void vTaskGetRunTimeStatistics( char *pcWriteBuffer, size_t uxBufferLength );
 * @endcode
 *
 * 要使此函数可用，必须将 configGENERATE_RUN_TIME_STATS 和
 * configUSE_STATS_FORMATTING_FUNCTIONS 都定义为 1。应用程序还必须
 * 提供 portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 和
 * portGET_RUN_TIME_COUNTER_VALUE() 的定义，分别用于配置外设定时器/计数器
 * 并返回该定时器的当前计数值。计数器的频率应至少为节拍频率的 10 倍。
 *
 * 注意 1：此函数在执行期间会禁止中断。它不用于正常的应用程序运行时，
 * 而是一种调试辅助手段。
 *
 * 将 configGENERATE_RUN_TIME_STATS 设置为 1 后，系统会为每个任务
 * 保存累计的总执行时间。累计时间值的分辨率取决于
 * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 宏所配置定时器的频率。
 * 调用 vTaskGetRunTimeStatistics() 会将每个任务的总执行时间写入缓冲区，
 * 既以绝对计数值表示，也以占系统总执行时间的百分比表示。
 *
 * 注意 2：
 *
 * 此函数仅为方便使用而提供，许多演示应用程序都在使用它。
 * 不要将其视为调度器的一部分。
 *
 * vTaskGetRunTimeStatistics() 调用 uxTaskGetSystemState()，然后将
 * uxTaskGetSystemState() 的部分输出格式化为人类可读的表格，
 * 以绝对值和百分比两种形式显示每个任务在运行态所花费的时间。
 *
 * vTaskGetRunTimeStatistics() 依赖 snprintf() C 库函数，这可能会增大
 * 代码体积、占用大量栈空间，并在不同平台上产生不同结果。
 * 许多 FreeRTOS/Demo 子目录中提供了一个名为 printf-stdarg.c 的
 * 第三方替代实现，它小巧且功能有限（注意：printf-stdarg.c 并未提供
 * 完整的 snprintf() 实现！）。
 *
 * 建议生产系统直接调用 uxTaskGetSystemState() 获取原始统计数据，
 * 而不是通过调用 vTaskGetRunTimeStatistics() 间接获取。
 *
 * @param pcWriteBuffer 用于以 ASCII 形式写入执行时间的缓冲区。
 * 假定该缓冲区足够大，能够容纳生成的报告。每个任务约 40 字节
 * 应该足够。
 *
 * @param uxBufferLength pcWriteBuffer 的长度。
 *
 * \defgroup vTaskGetRunTimeStatistics vTaskGetRunTimeStatistics
 * \ingroup TaskUtils
 */
#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configUSE_TRACE_FACILITY == 1 ) )
    void vTaskGetRunTimeStatistics( char * pcWriteBuffer,
                                    size_t uxBufferLength ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * void vTaskGetRunTimeStats( char *pcWriteBuffer );
 * @endcode
 *
 * 要使此函数可用，必须将 configGENERATE_RUN_TIME_STATS 和
 * configUSE_STATS_FORMATTING_FUNCTIONS 都定义为 1。应用程序还必须
 * 提供 portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 和
 * portGET_RUN_TIME_COUNTER_VALUE() 的定义，分别用于配置外设定时器/计数器
 * 并返回该定时器的当前计数值。计数器的频率应至少为节拍频率的 10 倍。
 *
 * 警告：此函数假定 pcWriteBuffer 的长度为 configSTATS_BUFFER_MAX_LENGTH。
 * 保留此函数仅是为了向后兼容。建议新应用程序使用
 * vTaskGetRunTimeStatistics，并显式提供 pcWriteBuffer 的长度。
 *
 * 注意 1：此函数在执行期间会禁止中断。它不用于正常的应用程序运行时，
 * 而是一种调试辅助手段。
 *
 * 将 configGENERATE_RUN_TIME_STATS 设置为 1 后，系统会为每个任务
 * 保存累计的总执行时间。累计时间值的分辨率取决于
 * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 宏所配置定时器的频率。
 * 调用 vTaskGetRunTimeStats() 会将每个任务的总执行时间写入缓冲区，
 * 既以绝对计数值表示，也以占系统总执行时间的百分比表示。
 *
 * 注意 2：
 *
 * 此函数仅为方便使用而提供，许多演示应用程序都在使用它。
 * 不要将其视为调度器的一部分。
 *
 * vTaskGetRunTimeStats() 调用 uxTaskGetSystemState()，然后将
 * uxTaskGetSystemState() 的部分输出格式化为人类可读的表格，
 * 以绝对值和百分比两种形式显示每个任务在运行态所花费的时间。
 *
 * vTaskGetRunTimeStats() 依赖 snprintf() C 库函数，这可能会增大
 * 代码体积、占用大量栈空间，并在不同平台上产生不同结果。
 * 许多 FreeRTOS/Demo 子目录中提供了一个名为 printf-stdarg.c 的
 * 第三方替代实现，它小巧且功能有限（注意：printf-stdarg.c 并未提供
 * 完整的 snprintf() 实现！）。
 *
 * 建议生产系统直接调用 uxTaskGetSystemState() 获取原始统计数据，
 * 而不是通过调用 vTaskGetRunTimeStats() 间接获取。
 *
 * @param pcWriteBuffer 用于以 ASCII 形式写入执行时间的缓冲区。
 * 假定该缓冲区足够大，能够容纳生成的报告。每个任务约 40 字节
 * 应该足够。
 *
 * \defgroup vTaskGetRunTimeStats vTaskGetRunTimeStats
 * \ingroup TaskUtils
 */
#define vTaskGetRunTimeStats( pcWriteBuffer )    vTaskGetRunTimeStatistics( ( pcWriteBuffer ), configSTATS_BUFFER_MAX_LENGTH )

/**
 * task. h
 * @code{c}
 * configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimeCounter( const TaskHandle_t xTask );
 * configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimePercent( const TaskHandle_t xTask );
 * @endcode
 *
 * 要使这些函数可用，必须将 configGENERATE_RUN_TIME_STATS 定义为 1。
 * 应用程序还必须提供 portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 和
 * portGET_RUN_TIME_COUNTER_VALUE() 的定义，分别用于配置外设定时器/计数器
 * 并返回该定时器的当前计数值。计数器的频率应至少为节拍频率的 10 倍。
 *
 * 将 configGENERATE_RUN_TIME_STATS 设置为 1 后，系统会为每个任务
 * 保存累计的总执行时间。累计时间值的分辨率取决于
 * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 宏所配置定时器的频率。
 * uxTaskGetSystemState() 和 vTaskGetRunTimeStats() 会将每个任务的总执行时间
 * 写入缓冲区，而 ulTaskGetRunTimeCounter() 仅返回单个任务的总执行时间，
 * ulTaskGetRunTimePercent() 仅返回单个任务所占用的 CPU 时间百分比。
 *
 * @return 给定任务的总运行时间，或给定任务消耗的总运行时间百分比。
 * 这是任务实际执行的时间。时间单位取决于
 * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 和
 * portGET_RUN_TIME_COUNTER_VALUE() 宏所配置的频率。
 *
 * \defgroup ulTaskGetRunTimeCounter ulTaskGetRunTimeCounter
 * \ingroup TaskUtils
 */
#if ( configGENERATE_RUN_TIME_STATS == 1 )
    configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimeCounter( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
    configRUN_TIME_COUNTER_TYPE ulTaskGetRunTimePercent( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounter( void );
 * configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimePercent( void );
 * @endcode
 *
 * 要使这些函数可用，必须将 configGENERATE_RUN_TIME_STATS 定义为 1。
 * 应用程序还必须提供 portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 和
 * portGET_RUN_TIME_COUNTER_VALUE() 的定义，分别用于配置外设定时器/计数器
 * 并返回该定时器的当前计数值。计数器的频率应至少为节拍频率的 10 倍。
 *
 * 将 configGENERATE_RUN_TIME_STATS 设置为 1 后，系统会为每个任务
 * 保存累计的总执行时间。累计时间值的分辨率取决于
 * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 宏所配置定时器的频率。
 * uxTaskGetSystemState() 和 vTaskGetRunTimeStats() 会将每个任务的总执行时间
 * 写入缓冲区，而 ulTaskGetIdleRunTimeCounter() 仅返回空闲任务的总执行时间，
 * ulTaskGetIdleRunTimePercent() 仅返回空闲任务所占用的 CPU 时间百分比。
 *
 * 注意，只有在没有其他任务以空闲优先级执行、未使用无节拍空闲模式，
 * 且 configIDLE_SHOULD_YIELD 设置为 0 时，空闲时间量才能很好地
 * 衡量系统中的富余时间。
 *
 * @return 空闲任务的总运行时间，或空闲任务消耗的总运行时间百分比。
 * 这是空闲任务实际执行的时间。时间单位取决于
 * portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 和
 * portGET_RUN_TIME_COUNTER_VALUE() 宏所配置的频率。
 *
 * \defgroup ulTaskGetIdleRunTimeCounter ulTaskGetIdleRunTimeCounter
 * \ingroup TaskUtils
 */
#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )
    configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounter( void ) PRIVILEGED_FUNCTION;
    configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimePercent( void ) PRIVILEGED_FUNCTION;
#endif

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyIndexed( TaskHandle_t xTaskToNotify, UBaseType_t uxIndexToNotify, uint32_t ulValue, eNotifyAction eAction );
 * BaseType_t xTaskNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction );
 * @endcode
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使这些函数可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 向任务发送直接任务通知，可选择附带数值和操作。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 事件可以通过中间对象发送给任务。此类对象的例子包括队列、信号量、
 * 互斥量和事件组。任务通知则是一种无需此类中间对象、
 * 直接向任务发送事件的方法。
 *
 * 发送给任务的通知可以选择执行某种操作，例如更新、覆盖或递增任务的
 * 某个通知值。因此，任务通知既可用于向任务发送数据，也可用作
 * 轻量、快速的二值信号量或计数信号量。
 *
 * 任务可以使用 xTaskNotifyWaitIndexed() 或 ulTaskNotifyTakeIndexed()
 * ［可选地］阻塞等待通知挂起。任务处于阻塞态期间不消耗任何 CPU 时间。
 *
 * 发送给任务的通知将保持挂起状态，直到任务调用
 * xTaskNotifyWaitIndexed() 或 ulTaskNotifyTakeIndexed()（或其非索引版本）
 * 将其清除。若通知到达时任务已经处于阻塞态等待通知，则任务将自动
 * 离开阻塞态（解除阻塞），并清除该通知。
 *
 * **注意** 数组中的每个通知独立运作——任务一次只能阻塞在数组中的一个
 * 通知上，且不会被发送到其他数组索引的通知解除阻塞。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * xTaskNotify() 是原始 API 函数，它始终作用于数组索引 0 处的通知值，
 * 从而保持向后兼容。调用 xTaskNotify() 等价于调用
 * xTaskNotifyIndexed()，并将 uxIndexToNotify 参数设置为 0。
 *
 * @param xTaskToNotify 被通知任务的句柄。任务句柄可由创建任务时使用的
 * xTaskCreate() API 函数返回，当前运行任务的句柄可通过调用
 * xTaskGetCurrentTaskHandle() 获取。
 *
 * @param uxIndexToNotify 目标任务通知值数组中的索引，通知将发送到该索引。
 * uxIndexToNotify 必须小于 configTASK_NOTIFICATION_ARRAY_ENTRIES。
 * xTaskNotify() 没有此参数，始终将通知发送到索引 0。
 *
 * @param ulValue 可随通知发送的数据。数据如何使用取决于
 * eAction 参数的值。
 *
 * @param eAction 指定通知如何更新任务的通知值（如果需要更新）。
 * eAction 的有效值如下：
 *
 * eSetBits -
 * 目标通知值与 ulValue 按位或。此时 xTaskNotifyIndexed() 始终返回 pdPASS。
 *
 * eIncrement -
 * 目标通知值递增。不使用 ulValue，此时 xTaskNotifyIndexed() 始终返回 pdPASS。
 *
 * eSetValueWithOverwrite -
 * 目标通知值被设置为 ulValue 的值，即使被通知任务尚未处理同一数组索引处
 * 先前的通知（该任务在该索引处已有挂起的通知）。
 * 此时 xTaskNotifyIndexed() 始终返回 pdPASS。
 *
 * eSetValueWithoutOverwrite -
 * 若被通知任务在同一数组索引处没有已挂起的通知，则目标通知值被设置为
 * ulValue，xTaskNotifyIndexed() 将返回 pdPASS。若被通知任务在同一数组索引处
 * 已有挂起的通知，则不执行任何操作并返回 pdFAIL。
 *
 * eNoAction -
 * 任务在指定数组索引处收到通知，但该索引处的通知值不更新。
 * 不使用 ulValue，此时 xTaskNotifyIndexed() 始终返回 pdPASS。
 *
 * @param pulPreviousNotificationValue 可用于传出目标任务在通知函数
 * 修改任何位之前的通知值。
 *
 * @return 取决于 eAction 的值。参见 eAction 参数的说明。
 *
 * \defgroup xTaskNotifyIndexed xTaskNotifyIndexed
 * \ingroup TaskNotifications
 */
BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                               UBaseType_t uxIndexToNotify,
                               uint32_t ulValue,
                               eNotifyAction eAction,
                               uint32_t * pulPreviousNotificationValue ) PRIVILEGED_FUNCTION;
#define xTaskNotify( xTaskToNotify, ulValue, eAction ) \
    xTaskGenericNotify( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( ulValue ), ( eAction ), NULL )
#define xTaskNotifyIndexed( xTaskToNotify, uxIndexToNotify, ulValue, eAction ) \
    xTaskGenericNotify( ( xTaskToNotify ), ( uxIndexToNotify ), ( ulValue ), ( eAction ), NULL )

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyAndQueryIndexed( TaskHandle_t xTaskToNotify, UBaseType_t uxIndexToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotifyValue );
 * BaseType_t xTaskNotifyAndQuery( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotifyValue );
 * @endcode
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * xTaskNotifyAndQueryIndexed() 执行与 xTaskNotifyIndexed() 相同的操作，
 * 区别在于它还会通过附加的 pulPreviousNotifyValue 参数返回目标任务的
 * 先前通知值（该值为函数调用时的通知值，而不是函数返回时的通知值）。
 *
 * xTaskNotifyAndQuery() 执行与 xTaskNotify() 相同的操作，
 * 区别在于它还会通过附加的 pulPreviousNotifyValue 参数返回目标任务的
 * 先前通知值（该值为函数调用时的通知值，而不是函数返回时的通知值）。
 *
 * \defgroup xTaskNotifyAndQueryIndexed xTaskNotifyAndQueryIndexed
 * \ingroup TaskNotifications
 */
#define xTaskNotifyAndQuery( xTaskToNotify, ulValue, eAction, pulPreviousNotifyValue ) \
    xTaskGenericNotify( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( ulValue ), ( eAction ), ( pulPreviousNotifyValue ) )
#define xTaskNotifyAndQueryIndexed( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotifyValue ) \
    xTaskGenericNotify( ( xTaskToNotify ), ( uxIndexToNotify ), ( ulValue ), ( eAction ), ( pulPreviousNotifyValue ) )

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyIndexedFromISR( TaskHandle_t xTaskToNotify, UBaseType_t uxIndexToNotify, uint32_t ulValue, eNotifyAction eAction, BaseType_t *pxHigherPriorityTaskWoken );
 * BaseType_t xTaskNotifyFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使这些函数可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * xTaskNotifyIndexed() 的一个版本，可在中断服务程序（ISR）中使用。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 事件可以通过中间对象发送给任务。此类对象的例子包括队列、信号量、
 * 互斥量和事件组。任务通知则是一种无需此类中间对象、
 * 直接向任务发送事件的方法。
 *
 * 发送给任务的通知可以选择执行某种操作，例如更新、覆盖或递增任务的
 * 某个通知值。因此，任务通知既可用于向任务发送数据，也可用作
 * 轻量、快速的二值信号量或计数信号量。
 *
 * 任务可以使用 xTaskNotifyWaitIndexed()［可选地］阻塞等待通知挂起，
 * 或使用 ulTaskNotifyTakeIndexed()［可选地］阻塞等待通知值变为非零。
 * 任务处于阻塞态期间不消耗任何 CPU 时间。
 *
 * 发送给任务的通知将保持挂起状态，直到任务调用
 * xTaskNotifyWaitIndexed() 或 ulTaskNotifyTakeIndexed()（或其非索引版本）
 * 将其清除。若通知到达时任务已经处于阻塞态等待通知，则任务将自动
 * 离开阻塞态（解除阻塞），并清除该通知。
 *
 * **注意** 数组中的每个通知独立运作——任务一次只能阻塞在数组中的一个
 * 通知上，且不会被发送到其他数组索引的通知解除阻塞。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * xTaskNotifyFromISR() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 xTaskNotifyFromISR() 等价于调用
 * xTaskNotifyIndexedFromISR()，并将 uxIndexToNotify 参数设置为 0。
 *
 * @param uxIndexToNotify 目标任务通知值数组中的索引，通知将发送到该索引。
 * uxIndexToNotify 必须小于 configTASK_NOTIFICATION_ARRAY_ENTRIES。
 * xTaskNotifyFromISR() 没有此参数，始终将通知发送到索引 0。
 *
 * @param xTaskToNotify 被通知任务的句柄。任务句柄可由创建任务时使用的
 * xTaskCreate() API 函数返回，当前运行任务的句柄可通过调用
 * xTaskGetCurrentTaskHandle() 获取。
 *
 * @param ulValue 可随通知发送的数据。数据如何使用取决于
 * eAction 参数的值。
 *
 * @param eAction 指定通知如何更新任务的通知值（如果需要更新）。
 * eAction 的有效值如下：
 *
 * eSetBits -
 * 任务的通知值与 ulValue 按位或。此时 xTaskNotify() 始终返回 pdPASS。
 *
 * eIncrement -
 * 任务的通知值递增。不使用 ulValue，此时 xTaskNotify() 始终返回 pdPASS。
 *
 * eSetValueWithOverwrite -
 * 任务的通知值被设置为 ulValue 的值，即使被通知任务尚未处理先前的通知
 * （该任务已有挂起的通知）。此时 xTaskNotify() 始终返回 pdPASS。
 *
 * eSetValueWithoutOverwrite -
 * 若被通知任务没有已挂起的通知，则任务的通知值被设置为 ulValue，
 * xTaskNotify() 将返回 pdPASS。若被通知任务已有挂起的通知，
 * 则不执行任何操作并返回 pdFAIL。
 *
 * eNoAction -
 * 任务收到通知，但其通知值不更新。不使用 ulValue，
 * 此时 xTaskNotify() 始终返回 pdPASS。
 *
 * @param pulPreviousNotificationValue 可用于传出目标任务在通知函数
 * 修改任何位之前的通知值。
 *
 * @param pxHigherPriorityTaskWoken 若发送通知导致被通知任务离开阻塞态，
 * 且解除阻塞任务的优先级高于当前运行任务，则 xTaskNotifyFromISR()
 * 会将 *pxHigherPriorityTaskWoken 设置为 pdTRUE。若 xTaskNotifyFromISR()
 * 将该值设置为 pdTRUE，则应在退出中断前请求上下文切换。
 * 如何从 ISR 请求上下文切换取决于移植层——请参见所用移植层的文档页面。
 *
 * @return 取决于 eAction 的值。参见 eAction 参数的说明。
 *
 * \defgroup xTaskNotifyIndexedFromISR xTaskNotifyIndexedFromISR
 * \ingroup TaskNotifications
 */
BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                      UBaseType_t uxIndexToNotify,
                                      uint32_t ulValue,
                                      eNotifyAction eAction,
                                      uint32_t * pulPreviousNotificationValue,
                                      BaseType_t * pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
#define xTaskNotifyFromISR( xTaskToNotify, ulValue, eAction, pxHigherPriorityTaskWoken ) \
    xTaskGenericNotifyFromISR( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( ulValue ), ( eAction ), NULL, ( pxHigherPriorityTaskWoken ) )
#define xTaskNotifyIndexedFromISR( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pxHigherPriorityTaskWoken ) \
    xTaskGenericNotifyFromISR( ( xTaskToNotify ), ( uxIndexToNotify ), ( ulValue ), ( eAction ), NULL, ( pxHigherPriorityTaskWoken ) )

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyAndQueryIndexedFromISR( TaskHandle_t xTaskToNotify, UBaseType_t uxIndexToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue, BaseType_t *pxHigherPriorityTaskWoken );
 * BaseType_t xTaskNotifyAndQueryFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * xTaskNotifyAndQueryIndexedFromISR() 执行与 xTaskNotifyIndexedFromISR()
 * 相同的操作，区别在于它还会通过附加的 pulPreviousNotifyValue 参数
 * 返回目标任务的先前通知值（该值为函数调用时的通知值，
 * 而不是函数返回时的通知值）。
 *
 * xTaskNotifyAndQueryFromISR() 执行与 xTaskNotifyFromISR() 相同的操作，
 * 区别在于它还会通过附加的 pulPreviousNotifyValue 参数返回目标任务的
 * 先前通知值（该值为函数调用时的通知值，而不是函数返回时的通知值）。
 *
 * \defgroup xTaskNotifyAndQueryIndexedFromISR xTaskNotifyAndQueryIndexedFromISR
 * \ingroup TaskNotifications
 */
#define xTaskNotifyAndQueryIndexedFromISR( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken ) \
    xTaskGenericNotifyFromISR( ( xTaskToNotify ), ( uxIndexToNotify ), ( ulValue ), ( eAction ), ( pulPreviousNotificationValue ), ( pxHigherPriorityTaskWoken ) )
#define xTaskNotifyAndQueryFromISR( xTaskToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken ) \
    xTaskGenericNotifyFromISR( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( ulValue ), ( eAction ), ( pulPreviousNotificationValue ), ( pxHigherPriorityTaskWoken ) )

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyWaitIndexed( UBaseType_t uxIndexToWaitOn, uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait );
 *
 * BaseType_t xTaskNotifyWait( uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait );
 * @endcode
 *
 * 等待直接任务通知数组中指定索引处的通知挂起。
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使此函数可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 事件可以通过中间对象发送给任务。此类对象的例子包括队列、信号量、
 * 互斥量和事件组。任务通知则是一种无需此类中间对象、
 * 直接向任务发送事件的方法。
 *
 * 发送给任务的通知可以选择执行某种操作，例如更新、覆盖或递增任务的
 * 某个通知值。因此，任务通知既可用于向任务发送数据，也可用作
 * 轻量、快速的二值信号量或计数信号量。
 *
 * 发送给任务的通知将保持挂起状态，直到任务调用
 * xTaskNotifyWaitIndexed() 或 ulTaskNotifyTakeIndexed()（或其非索引版本）
 * 将其清除。若通知到达时任务已经处于阻塞态等待通知，则任务将自动
 * 离开阻塞态（解除阻塞），并清除该通知。
 *
 * 任务可以使用 xTaskNotifyWaitIndexed()［可选地］阻塞等待通知挂起，
 * 或使用 ulTaskNotifyTakeIndexed()［可选地］阻塞等待通知值变为非零。
 * 任务处于阻塞态期间不消耗任何 CPU 时间。
 *
 * **注意** 数组中的每个通知独立运作——任务一次只能阻塞在数组中的一个
 * 通知上，且不会被发送到其他数组索引的通知解除阻塞。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * xTaskNotifyWait() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 xTaskNotifyWait() 等价于调用
 * xTaskNotifyWaitIndexed()，并将 uxIndexToWaitOn 参数设置为 0。
 *
 * @param uxIndexToWaitOn 调用任务通知值数组中的索引，调用任务将在该索引处
 * 等待接收到通知。uxIndexToWaitOn 必须小于
 * configTASK_NOTIFICATION_ARRAY_ENTRIES。xTaskNotifyWait() 没有此参数，
 * 始终在索引 0 处等待通知。
 *
 * @param ulBitsToClearOnEntry 在 ulBitsToClearOnEntry 值中置位的位，
 * 将在调用任务检查是否有通知挂起之前，从调用任务的通知值中清除；
 * 若没有通知挂起，则可选地进入阻塞。将 ulBitsToClearOnEntry 设置为
 * ULONG_MAX（若包含 limits.h）或 0xffffffffU（若未包含 limits.h），
 * 其效果是将任务的通知值重置为 0。将 ulBitsToClearOnEntry 设置为 0，
 * 则任务的通知值保持不变。
 *
 * @param ulBitsToClearOnExit 若调用任务退出 xTaskNotifyWait() 函数之前
 * 有通知挂起或已收到通知，则任务的通知值（参见 xTaskNotify() API 函数）
 * 将通过 pulNotificationValue 参数传出。随后，ulBitsToClearOnExit 中置位的
 * 任何位将从任务的通知值中清除（注意，*pulNotificationValue 在清除任何位
 * 之前设置）。将 ulBitsToClearOnExit 设置为 ULONG_MAX（若包含 limits.h）或
 * 0xffffffffUL（若未包含 limits.h），其效果是在函数退出前将任务的通知值
 * 重置为 0。将 ulBitsToClearOnExit 设置为 0，则函数退出时任务的通知值
 * 保持不变（此时通过 pulNotificationValue 传出的值将与任务的通知值一致）。
 *
 * @param pulNotificationValue 用于将任务的通知值传出函数。注意，传出值
 * 不会受到 ulBitsToClearOnExit 非零所导致的位清除的影响。
 *
 * @param xTicksToWait 若调用 xTaskNotifyWait() 时尚无通知挂起，则任务应在
 * 阻塞态等待接收通知的最长时间。任务处于阻塞态期间不消耗任何处理时间。
 * 该时间以内核节拍为单位指定，可使用宏 pdMS_TO_TICKS( value_in_ms )
 * 将毫秒时间转换为节拍时间。
 *
 * @return 若收到通知（包括调用 xTaskNotifyWait 时已经挂起的通知），
 * 则返回 pdPASS；否则返回 pdFAIL。
 *
 * \defgroup xTaskNotifyWaitIndexed xTaskNotifyWaitIndexed
 * \ingroup TaskNotifications
 */
BaseType_t xTaskGenericNotifyWait( UBaseType_t uxIndexToWaitOn,
                                   uint32_t ulBitsToClearOnEntry,
                                   uint32_t ulBitsToClearOnExit,
                                   uint32_t * pulNotificationValue,
                                   TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
#define xTaskNotifyWait( ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait ) \
    xTaskGenericNotifyWait( tskDEFAULT_INDEX_TO_NOTIFY, ( ulBitsToClearOnEntry ), ( ulBitsToClearOnExit ), ( pulNotificationValue ), ( xTicksToWait ) )
#define xTaskNotifyWaitIndexed( uxIndexToWaitOn, ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait ) \
    xTaskGenericNotifyWait( ( uxIndexToWaitOn ), ( ulBitsToClearOnEntry ), ( ulBitsToClearOnExit ), ( pulNotificationValue ), ( xTicksToWait ) )

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyGiveIndexed( TaskHandle_t xTaskToNotify, UBaseType_t uxIndexToNotify );
 * BaseType_t xTaskNotifyGive( TaskHandle_t xTaskToNotify );
 * @endcode
 *
 * 以类似于释放计数信号量的方式，向目标任务通知数组的特定索引
 * 发送直接任务通知。
 *
 * 更多详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使这些宏可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 事件可以通过中间对象发送给任务。此类对象的例子包括队列、信号量、
 * 互斥量和事件组。任务通知则是一种无需此类中间对象、
 * 直接向任务发送事件的方法。
 *
 * 发送给任务的通知可以选择执行某种操作，例如更新、覆盖或递增任务的
 * 某个通知值。因此，任务通知既可用于向任务发送数据，也可用作
 * 轻量、快速的二值信号量或计数信号量。
 *
 * xTaskNotifyGiveIndexed() 是一个辅助宏，适用于将任务通知用作
 * 轻量且更快的二值信号量或计数信号量替代品的场景。实际 FreeRTOS
 * 信号量通过 xSemaphoreGive() API 函数释放，而改用任务通知的等效操作
 * 是 xTaskNotifyGiveIndexed()。
 *
 * 当任务通知被用作二值信号量或计数信号量的替代品时，被通知任务应使用
 * ulTaskNotifyTakeIndexed() API 函数等待通知，而不是使用
 * xTaskNotifyWaitIndexed() API 函数。
 *
 * **注意** 数组中的每个通知独立运作——任务一次只能阻塞在数组中的一个
 * 通知上，且不会被发送到其他数组索引的通知解除阻塞。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * xTaskNotifyGive() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 xTaskNotifyGive() 等价于调用
 * xTaskNotifyGiveIndexed()，并将 uxIndexToNotify 参数设置为 0。
 *
 * @param xTaskToNotify 被通知任务的句柄。任务句柄可由创建任务时使用的
 * xTaskCreate() API 函数返回，当前运行任务的句柄可通过调用
 * xTaskGetCurrentTaskHandle() 获取。
 *
 * @param uxIndexToNotify 目标任务通知值数组中的索引，通知将发送到该索引。
 * uxIndexToNotify 必须小于 configTASK_NOTIFICATION_ARRAY_ENTRIES。
 * xTaskNotifyGive() 没有此参数，始终将通知发送到索引 0。
 *
 * @return xTaskNotifyGive() 是一个宏，它调用 xTaskNotify() 并将
 * eAction 参数设置为 eIncrement——因此始终返回 pdPASS。
 *
 * \defgroup xTaskNotifyGiveIndexed xTaskNotifyGiveIndexed
 * \ingroup TaskNotifications
 */
#define xTaskNotifyGive( xTaskToNotify ) \
    xTaskGenericNotify( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( 0 ), eIncrement, NULL )
#define xTaskNotifyGiveIndexed( xTaskToNotify, uxIndexToNotify ) \
    xTaskGenericNotify( ( xTaskToNotify ), ( uxIndexToNotify ), ( 0 ), eIncrement, NULL )

/**
 * task. h
 * @code{c}
 * void vTaskNotifyGiveIndexedFromISR( TaskHandle_t xTaskHandle, UBaseType_t uxIndexToNotify, BaseType_t *pxHigherPriorityTaskWoken );
 * void vTaskNotifyGiveFromISR( TaskHandle_t xTaskHandle, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * xTaskNotifyGiveIndexed() 的一个版本，可在中断服务程序（ISR）中调用。
 *
 * 更多详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使此宏可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 事件可以通过中间对象发送给任务。此类对象的例子包括队列、信号量、
 * 互斥量和事件组。任务通知则是一种无需此类中间对象、
 * 直接向任务发送事件的方法。
 *
 * 发送给任务的通知可以选择执行某种操作，例如更新、覆盖或递增任务的
 * 某个通知值。因此，任务通知既可用于向任务发送数据，也可用作
 * 轻量、快速的二值信号量或计数信号量。
 *
 * vTaskNotifyGiveIndexedFromISR() 适用于将任务通知用作轻量且更快的
 * 二值信号量或计数信号量替代品的场景。实际 FreeRTOS 信号量通过
 * xSemaphoreGiveFromISR() API 函数从 ISR 中释放，而改用任务通知的
 * 等效操作是 vTaskNotifyGiveIndexedFromISR()。
 *
 * 当任务通知被用作二值信号量或计数信号量的替代品时，被通知任务应使用
 * ulTaskNotifyTakeIndexed() API 函数等待通知，而不是使用
 * xTaskNotifyWaitIndexed() API 函数。
 *
 * **注意** 数组中的每个通知独立运作——任务一次只能阻塞在数组中的一个
 * 通知上，且不会被发送到其他数组索引的通知解除阻塞。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * xTaskNotifyFromISR() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 xTaskNotifyGiveFromISR() 等价于调用
 * xTaskNotifyGiveIndexedFromISR()，并将 uxIndexToNotify 参数设置为 0。
 *
 * @param xTaskToNotify 被通知任务的句柄。任务句柄可由创建任务时使用的
 * xTaskCreate() API 函数返回，当前运行任务的句柄可通过调用
 * xTaskGetCurrentTaskHandle() 获取。
 *
 * @param uxIndexToNotify 目标任务通知值数组中的索引，通知将发送到该索引。
 * uxIndexToNotify 必须小于 configTASK_NOTIFICATION_ARRAY_ENTRIES。
 * xTaskNotifyGiveFromISR() 没有此参数，始终将通知发送到索引 0。
 *
 * @param pxHigherPriorityTaskWoken 若发送通知导致被通知任务离开阻塞态，
 * 且解除阻塞任务的优先级高于当前运行任务，则 vTaskNotifyGiveFromISR()
 * 会将 *pxHigherPriorityTaskWoken 设置为 pdTRUE。
 * 若 vTaskNotifyGiveFromISR() 将该值设置为 pdTRUE，则应在退出中断前
 * 请求上下文切换。如何从 ISR 请求上下文切换取决于移植层——
 * 请参见所用移植层的文档页面。
 *
 * \defgroup vTaskNotifyGiveIndexedFromISR vTaskNotifyGiveIndexedFromISR
 * \ingroup TaskNotifications
 */
void vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                    UBaseType_t uxIndexToNotify,
                                    BaseType_t * pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
#define vTaskNotifyGiveFromISR( xTaskToNotify, pxHigherPriorityTaskWoken ) \
    vTaskGenericNotifyGiveFromISR( ( xTaskToNotify ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( pxHigherPriorityTaskWoken ) )
#define vTaskNotifyGiveIndexedFromISR( xTaskToNotify, uxIndexToNotify, pxHigherPriorityTaskWoken ) \
    vTaskGenericNotifyGiveFromISR( ( xTaskToNotify ), ( uxIndexToNotify ), ( pxHigherPriorityTaskWoken ) )

/**
 * task. h
 * @code{c}
 * uint32_t ulTaskNotifyTakeIndexed( UBaseType_t uxIndexToWaitOn, BaseType_t xClearCountOnExit, TickType_t xTicksToWait );
 *
 * uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait );
 * @endcode
 *
 * 以类似于获取计数信号量的方式，等待调用任务通知数组中特定索引处的
 * 直接任务通知。
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使此函数可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 事件可以通过中间对象发送给任务。此类对象的例子包括队列、信号量、
 * 互斥量和事件组。任务通知则是一种无需此类中间对象、
 * 直接向任务发送事件的方法。
 *
 * 发送给任务的通知可以选择执行某种操作，例如更新、覆盖或递增任务的
 * 某个通知值。因此，任务通知既可用于向任务发送数据，也可用作
 * 轻量、快速的二值信号量或计数信号量。
 *
 * ulTaskNotifyTakeIndexed() 适用于将任务通知用作更快、更轻量的
 * 二值信号量或计数信号量替代品的场景。实际 FreeRTOS 信号量通过
 * xSemaphoreTake() API 函数获取，而改用任务通知的等效操作是
 * ulTaskNotifyTakeIndexed()。
 *
 * 当任务将其通知值用作二值信号量或计数信号量时，其他任务应使用
 * xTaskNotifyGiveIndexed() 宏，或使用 xTaskNotifyIndex() 函数并将
 * eAction 参数设置为 eIncrement，向该任务发送通知。
 *
 * ulTaskNotifyTakeIndexed() 可以在退出时将 uxIndexToWaitOn 参数所指定
 * 数组索引处的任务通知值清零，此时通知值的作用类似于二值信号量；
 * 也可以在退出时递减通知值，此时通知值的作用类似于计数信号量。
 *
 * 任务可以使用 ulTaskNotifyTakeIndexed()［可选地］阻塞等待通知。
 * 任务处于阻塞态期间不消耗任何 CPU 时间。
 *
 * xTaskNotifyWaitIndexed() 在有通知挂起时返回，而
 * ulTaskNotifyTakeIndexed() 在任务的通知值非零时返回。
 *
 * **注意** 数组中的每个通知独立运作——任务一次只能阻塞在数组中的一个
 * 通知上，且不会被发送到其他数组索引的通知解除阻塞。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * ulTaskNotifyTake() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 ulTaskNotifyTake() 等价于调用
 * ulTaskNotifyTakeIndexed()，并将 uxIndexToWaitOn 参数设置为 0。
 *
 * @param uxIndexToWaitOn 调用任务通知值数组中的索引，调用任务将在该索引处
 * 等待通知值变为非零。uxIndexToWaitOn 必须小于
 * configTASK_NOTIFICATION_ARRAY_ENTRIES。xTaskNotifyTake() 没有此参数，
 * 始终在索引 0 处等待通知。
 *
 * @param xClearCountOnExit 若 xClearCountOnExit 为 pdFALSE，则函数退出时
 * 任务的通知值递减。这样通知值的作用类似于计数信号量。
 * 若 xClearCountOnExit 不为 pdFALSE，则函数退出时任务的通知值清零。
 * 这样通知值的作用类似于二值信号量。
 *
 * @param xTicksToWait 若调用 ulTaskNotifyTake() 时计数值尚未大于零，
 * 则任务应在阻塞态等待其通知值大于零的最长时间。
 * 任务处于阻塞态期间不消耗任何处理时间。该时间以内核节拍为单位指定，
 * 可使用宏 pdMS_TO_TICKS( value_in_ms ) 将毫秒时间转换为节拍时间。
 *
 * @return 任务通知值被清零或递减之前（参见 xClearCountOnExit 参数）的
 * 通知计数值。
 *
 * \defgroup ulTaskNotifyTakeIndexed ulTaskNotifyTakeIndexed
 * \ingroup TaskNotifications
 */
uint32_t ulTaskGenericNotifyTake( UBaseType_t uxIndexToWaitOn,
                                  BaseType_t xClearCountOnExit,
                                  TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
#define ulTaskNotifyTake( xClearCountOnExit, xTicksToWait ) \
    ulTaskGenericNotifyTake( ( tskDEFAULT_INDEX_TO_NOTIFY ), ( xClearCountOnExit ), ( xTicksToWait ) )
#define ulTaskNotifyTakeIndexed( uxIndexToWaitOn, xClearCountOnExit, xTicksToWait ) \
    ulTaskGenericNotifyTake( ( uxIndexToWaitOn ), ( xClearCountOnExit ), ( xTicksToWait ) )

/**
 * task. h
 * @code{c}
 * BaseType_t xTaskNotifyStateClearIndexed( TaskHandle_t xTask, UBaseType_t uxIndexToClear );
 *
 * BaseType_t xTaskNotifyStateClear( TaskHandle_t xTask );
 * @endcode
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使这些函数可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * 若向通知数组中的某个索引发送了通知，则称该索引处的通知处于“挂起”状态，
 * 直到接收任务读取它或显式清除它。xTaskNotifyStateClearIndexed()
 * 就是用于在不读取通知值的情况下清除挂起通知的函数。
 * 同一数组索引处的通知值不会被修改。将 xTask 设置为 NULL 可清除
 * 调用任务的通知状态。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * xTaskNotifyStateClear() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 xTaskNotifyStateClear() 等价于调用
 * xTaskNotifyStateClearIndexed()，并将 uxIndexToNotify 参数设置为 0。
 *
 * @param xTask 要清除通知状态的 RTOS 任务句柄。将 xTask 设置为 NULL
 * 可清除调用任务的通知状态。要获取任务句柄，可使用 xTaskCreate() 创建任务
 * 并利用 pxCreatedTask 参数，或使用 xTaskCreateStatic() 创建任务并保存
 * 其返回值，也可在调用 xTaskGetHandle() 时使用任务名称。
 *
 * @param uxIndexToClear 目标任务通知值数组中要操作的索引。例如，
 * 将 uxIndexToClear 设置为 1 将清除数组中索引 1 处通知的状态。
 * uxIndexToClear 必须小于 configTASK_NOTIFICATION_ARRAY_ENTRIES。
 * ulTaskNotifyStateClear() 没有此参数，始终操作索引 0 处的通知。
 *
 * @return 若任务的通知状态被设置为 eNotWaitingNotification，则返回 pdTRUE；
 * 否则返回 pdFALSE。
 *
 * \defgroup xTaskNotifyStateClearIndexed xTaskNotifyStateClearIndexed
 * \ingroup TaskNotifications
 */
BaseType_t xTaskGenericNotifyStateClear( TaskHandle_t xTask,
                                         UBaseType_t uxIndexToClear ) PRIVILEGED_FUNCTION;
#define xTaskNotifyStateClear( xTask ) \
    xTaskGenericNotifyStateClear( ( xTask ), ( tskDEFAULT_INDEX_TO_NOTIFY ) )
#define xTaskNotifyStateClearIndexed( xTask, uxIndexToClear ) \
    xTaskGenericNotifyStateClear( ( xTask ), ( uxIndexToClear ) )

/**
 * task. h
 * @code{c}
 * uint32_t ulTaskNotifyValueClearIndexed( TaskHandle_t xTask, UBaseType_t uxIndexToClear, uint32_t ulBitsToClear );
 *
 * uint32_t ulTaskNotifyValueClear( TaskHandle_t xTask, uint32_t ulBitsToClear );
 * @endcode
 *
 * 详情请参见 https://www.FreeRTOS.org/RTOS-task-notifications.html。
 *
 * 要使这些函数可用，configUSE_TASK_NOTIFICATIONS 必须未定义或定义为 1。
 *
 * 每个任务都有一个私有的“通知值”（或称“通知”）数组，
 * 其中每个通知值都是 32 位无符号整数（uint32_t）。常量
 * configTASK_NOTIFICATION_ARRAY_ENTRIES 设置数组中的索引数量，
 * （为保持向后兼容）若未定义则默认为 1。
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个通知值。
 *
 * ulTaskNotifyValueClearIndexed() 清除由 xTask 引用任务的通知值中、
 * 数组索引 uxIndexToClear 处由 ulBitsToClear 位掩码指定的位。
 *
 * 向后兼容信息：
 * 在 FreeRTOS V10.4.0 之前，每个任务只有一个“通知值”，
 * 所有任务通知 API 函数都作用于该值。用通知值数组替换单一通知值
 * 后，需要一组新的 API 函数来访问数组中的特定通知。
 * ulTaskNotifyValueClear() 是原始 API 函数，它始终作用于数组索引 0 处的
 * 通知值，从而保持向后兼容。调用 ulTaskNotifyValueClear() 等价于调用
 * ulTaskNotifyValueClearIndexed()，并将 uxIndexToClear 参数设置为 0。
 *
 * @param xTask 要清除其某个通知值中位的 RTOS 任务句柄。将 xTask 设置为 NULL
 * 可清除调用任务通知值中的位。要获取任务句柄，可使用 xTaskCreate() 创建任务
 * 并利用 pxCreatedTask 参数，或使用 xTaskCreateStatic() 创建任务并保存
 * 其返回值，也可在调用 xTaskGetHandle() 时使用任务名称。
 *
 * @param uxIndexToClear 目标任务通知值数组中要清除位的索引。
 * uxIndexToClear 必须小于 configTASK_NOTIFICATION_ARRAY_ENTRIES。
 * ulTaskNotifyValueClear() 没有此参数，始终清除索引 0 处通知值中的位。
 *
 * @param ulBitsToClear 要清除的位掩码，作用于 xTask 的通知值。
 * 将某位置 1 可清除任务通知值中对应的位。将 ulBitsToClear 设置为 0xffffffff
 * （32 位架构上为 UINT_MAX）可将通知值清零。将 ulBitsToClear 设置为 0
 * 可查询任务的通知值而不清除任何位。
 *
 *
 * @return 目标任务通知值在清除 ulBitsToClear 指定的位之前的值。
 * \defgroup ulTaskNotifyValueClear ulTaskNotifyValueClear
 * \ingroup TaskNotifications
 */
uint32_t ulTaskGenericNotifyValueClear( TaskHandle_t xTask,
                                        UBaseType_t uxIndexToClear,
                                        uint32_t ulBitsToClear ) PRIVILEGED_FUNCTION;
#define ulTaskNotifyValueClear( xTask, ulBitsToClear ) \
    ulTaskGenericNotifyValueClear( ( xTask ), ( tskDEFAULT_INDEX_TO_NOTIFY ), ( ulBitsToClear ) )
#define ulTaskNotifyValueClearIndexed( xTask, uxIndexToClear, ulBitsToClear ) \
    ulTaskGenericNotifyValueClear( ( xTask ), ( uxIndexToClear ), ( ulBitsToClear ) )

/**
 * task.h
 * @code{c}
 * void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut );
 * @endcode
 *
 * 捕获当前时间，供以后与 xTaskCheckForTimeOut() 配合使用。
 *
 * @param pxTimeOut 指向超时对象的指针，当前时间将被捕获到该对象中。
 * 捕获的时间包括节拍计数，以及自系统首次启动以来节拍计数溢出的次数。
 * \defgroup vTaskSetTimeOutState vTaskSetTimeOutState
 * \ingroup TaskCtrl
 */
void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut ) PRIVILEGED_FUNCTION;

/**
 * task.h
 * @code{c}
 * BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut, TickType_t * const pxTicksToWait );
 * @endcode
 *
 * 判断自调用 vTaskSetTimeOutState() 捕获时间以来，是否已经过
 * pxTicksToWait 个节拍。捕获的时间包括节拍计数以及节拍计数溢出的次数。
 *
 * @param pxTimeOut 先前使用 vTaskSetTimeOutState 捕获的时间状态。
 * 若超时尚未发生，则将其更新为当前时间状态。
 * @param pxTicksToWait 要检查超时的节拍数，即若自 pxTimeOut 上次更新
 * （由 vTaskSetTimeOutState() 或 xTaskCheckForTimeOut() 更新）以来
 * 已经过 pxTicksToWait 个节拍，则说明已超时。若超时尚未发生，
 * 则将 pxTicksToWait 更新为剩余的节拍数。
 *
 * @return 若已超时，返回 pdTRUE。否则返回 pdFALSE，
 * 并将 pxTicksToWait 更新为剩余的节拍数。
 *
 * @see https://www.FreeRTOS.org/xTaskCheckForTimeOut.html
 *
 * 用法示例：
 * @code{c}
 *  // 驱动程序库函数，用于从由 UART 中断填充的接收缓冲区中
 *  // 接收 uxWantedBytes 个字节。若接收缓冲区中的字节数不足，
 *  // 则任务进入阻塞态，直到收到已向缓冲区放入更多数据的通知。
 *  // 若数据仍然不足，则任务重新进入阻塞态，并使用
 *  // xTaskCheckForTimeOut() 重新计算阻塞时间，以确保在阻塞态
 *  // 花费的总时间不超过 MAX_TIME_TO_WAIT。此过程持续进行，
 *  // 直到缓冲区至少包含 uxWantedBytes 个字节，或
 *  // 在阻塞态花费的总时间达到 MAX_TIME_TO_WAIT——此时任务
 *  // 读取当前可用的字节数，最多不超过 uxWantedBytes 个。
 *
 *  size_t xUART_Receive( uint8_t *pucBuffer, size_t uxWantedBytes )
 *  {
 *  size_t uxReceived = 0;
 *  TickType_t xTicksToWait = MAX_TIME_TO_WAIT;
 *  TimeOut_t xTimeOut;
 *
 *      // 初始化 xTimeOut。它会记录进入此函数的时间。
 *      vTaskSetTimeOutState( &xTimeOut );
 *
 *      // 循环，直到缓冲区包含所需数量的字节，或发生超时。
 *      while( UART_bytes_in_rx_buffer( pxUARTInstance ) < uxWantedBytes )
 *      {
 *          // 缓冲区中的数据不足，因此此任务将进入阻塞态。
 *          // 调整 xTicksToWait，以计入此前在此函数内已在阻塞态
 *          // 花费的时间，确保在阻塞态花费的总时间
 *          // 不超过 MAX_TIME_TO_WAIT。
 *          if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) != pdFALSE )
 *          {
 *              // 在获得所需数量的字节之前已超时，
 *              // 退出循环。
 *              break;
 *          }
 *
 *          // 最多等待 xTicksToWait 个节拍，以收到接收中断
 *          // 已向缓冲区放入更多数据的通知。
 *          ulTaskNotifyTake( pdTRUE, xTicksToWait );
 *      }
 *
 *      // 尝试从接收缓冲区读取 uxWantedBytes 个字节到 pucBuffer。
 *      // 返回实际读取的字节数（可能少于 uxWantedBytes）。
 *      uxReceived = UART_read_from_receive_buffer( pxUARTInstance,
 *                                                  pucBuffer,
 *                                                  uxWantedBytes );
 *
 *      return uxReceived;
 *  }
 * @endcode
 * \defgroup xTaskCheckForTimeOut xTaskCheckForTimeOut
 * \ingroup TaskCtrl
 */
BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut,
                                 TickType_t * const pxTicksToWait ) PRIVILEGED_FUNCTION;

/**
 * task.h
 * @code{c}
 * BaseType_t xTaskCatchUpTicks( TickType_t xTicksToCatchUp );
 * @endcode
 *
 * 当应用程序代码长时间禁止中断，导致节拍中断被错过时，
 * 此函数用于修正节拍计数值。
 *
 * 此函数与 vTaskStepTick() 类似，但不同之处在于：
 * xTaskCatchUpTicks() 可能将节拍计数向前推进越过某个任务本应
 * 离开阻塞态的时间点。这意味着在推进节拍计数时，
 * 可能需要将任务从阻塞态移出。
 *
 * @param xTicksToCatchUp 因中断被禁止而错过的节拍中断数量。
 * 该值不会自动计算，必须由应用程序编写者计算。
 *
 * @return 若向前推进节拍计数导致任务离开阻塞态并执行了上下文切换，
 * 则返回 pdTRUE；否则返回 pdFALSE。
 *
 * \defgroup xTaskCatchUpTicks xTaskCatchUpTicks
 * \ingroup TaskCtrl
 */
BaseType_t xTaskCatchUpTicks( TickType_t xTicksToCatchUp ) PRIVILEGED_FUNCTION;

/**
 * task.h
 * @code{c}
 * void vTaskResetState( void );
 * @endcode
 *
 * 此函数重置任务的内部状态。应用程序必须在重新启动调度器之前调用它。
 *
 * \defgroup vTaskResetState vTaskResetState
 * \ingroup SchedulerControl
 */
void vTaskResetState( void ) PRIVILEGED_FUNCTION;


/*
 * -----------------------------------------------------------
 * 可供移植使用的调度器内部接口
 * ----------------------------------------------------------
 */

#if ( configNUMBER_OF_CORES == 1 )
    #define taskYIELD_WITHIN_API()    portYIELD_WITHIN_API()
#else /* #if ( configNUMBER_OF_CORES == 1 )（单核条件） */
    #define taskYIELD_WITHIN_API()    vTaskYieldWithinAPI()
#endif /* #if ( configNUMBER_OF_CORES == 1 )（单核条件） */

/*
 * 应用程序代码不得使用此函数。它仅用于实现调度器移植，
 * 是调度器专用接口。
 *
 * 此函数由实时内核节拍调用（抢占式或协作式），它将递增节拍计数，
 * 并检查是否有因有限时间阻塞而需要从阻塞链表移出、放入就绪链表的任务。
 * 若返回非零值，则需要上下文切换，原因可能是：
 *   + 某任务因超时到期而从阻塞链表移出，或
 *   + 正在使用时间片轮转，且存在与当前运行任务优先级相同的任务。
 */
BaseType_t xTaskIncrementTick( void ) PRIVILEGED_FUNCTION;

/*
 * 应用程序代码不得使用此函数。它是调度器专用接口。
 *
 * 调用此函数时必须禁止中断。
 *
 * 从就绪链表中移除调用任务，并将其同时放入等待特定事件的任务链表
 * 和延时任务链表。若事件发生（且没有更高优先级任务等待同一事件）
 * 或延时时间到期，任务将从这两个链表中移除并重新放回就绪链表。
 *
 * “无序”版本用 xItemValue 值替换事件链表项的值，
 * 并将链表项插入链表末尾。
 *
 * “有序”版本使用现有事件链表项的值（即所属任务的优先级），
 * 按任务优先级顺序将链表项插入事件链表。
 *
 * @param pxEventList 包含因等待事件发生而阻塞的任务的链表。
 *
 * @param xItemValue 当事件链表不按任务优先级排序时，
 * 用于事件链表项的项值。
 *
 * @param xTicksToWait 任务等待事件发生的最长时间。
 * 该时间以内核节拍为单位指定，可使用常量 portTICK_PERIOD_MS
 * 将内核节拍转换为实际时间。
 */
void vTaskPlaceOnEventList( List_t * const pxEventList,
                            const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
void vTaskPlaceOnUnorderedEventList( List_t * pxEventList,
                                     const TickType_t xItemValue,
                                     const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

/*
 * 应用程序代码不得使用此函数。它是调度器专用接口。
 *
 * 调用此函数时必须禁止中断。
 *
 * 此函数执行的功能与 vTaskPlaceOnEventList() 几乎相同。
 * 区别在于此函数不允许任务无限期阻塞，而 vTaskPlaceOnEventList() 允许。
 */
void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
                                      TickType_t xTicksToWait,
                                      const BaseType_t xWaitIndefinitely ) PRIVILEGED_FUNCTION;

/*
 * 应用程序代码不得使用此函数。它是调度器专用接口。
 *
 * 调用此函数时必须禁止中断。
 *
 * 从指定事件链表和阻塞任务链表中移除任务，并将其放入就绪队列。
 *
 * 当事件发生使任务解除阻塞，或阻塞超时时间到期时，
 * 会调用 xTaskRemoveFromEventList()/vTaskRemoveFromUnorderedEventList()。
 *
 * 当事件链表按任务优先级排序时使用 xTaskRemoveFromEventList()。
 * 它从事件链表头部移除链表项，因为该链表项对应事件链表上所有任务中
 * 优先级最高的所属任务。当事件链表不按顺序排列，且事件链表项保存的
 * 不是所属任务优先级时，使用 vTaskRemoveFromUnorderedEventList()。
 * 此时，事件链表项的值会更新为 xItemValue 参数传入的值。
 *
 * @return 若被移除任务的优先级高于调用任务，则返回 pdTRUE；否则返回 pdFALSE。
 */
BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList ) PRIVILEGED_FUNCTION;
void vTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem,
                                        const TickType_t xItemValue ) PRIVILEGED_FUNCTION;

/*
 * 应用程序代码不得使用此函数。它仅用于实现调度器移植，
 * 是调度器专用接口。
 *
 * 将当前 TCB 的指针设置为最高优先级的就绪运行任务的 TCB。
 */
#if ( configNUMBER_OF_CORES == 1 )
    portDONT_DISCARD void vTaskSwitchContext( void ) PRIVILEGED_FUNCTION;
#else
    portDONT_DISCARD void vTaskSwitchContext( BaseType_t xCoreID ) PRIVILEGED_FUNCTION;
#endif

/*
 * 这些函数不得在应用程序代码中使用。它们由事件位模块使用。
 */
TickType_t uxTaskResetEventItemValue( void ) PRIVILEGED_FUNCTION;

/*
 * 返回调用任务的句柄。
 */
TaskHandle_t xTaskGetCurrentTaskHandle( void ) PRIVILEGED_FUNCTION;

/*
 * 返回指定核心上运行任务的句柄。
 */
TaskHandle_t xTaskGetCurrentTaskHandleForCore( BaseType_t xCoreID ) PRIVILEGED_FUNCTION;

/*
 * 队列实现使用的快捷方式，用于避免不必要的 taskYIELD() 调用。
 */
void vTaskMissedYield( void ) PRIVILEGED_FUNCTION;

/*
 * 返回调度器状态，为 taskSCHEDULER_RUNNING、taskSCHEDULER_NOT_STARTED 或 taskSCHEDULER_SUSPENDED。
 */
BaseType_t xTaskGetSchedulerState( void ) PRIVILEGED_FUNCTION;

/*
 * 若互斥量持有者的优先级低于调用任务，则将互斥量持有者的优先级提升到调用任务的优先级。
 */
BaseType_t xTaskPriorityInherit( TaskHandle_t const pxMutexHolder ) PRIVILEGED_FUNCTION;

/*
 * 在任务持有信号量期间继承了更高优先级的情况下，将任务优先级恢复为其应有的优先级。
 */
BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder ) PRIVILEGED_FUNCTION;

/*
 * 若一个更高优先级任务尝试获取互斥量，导致较低优先级任务继承了
 * 该更高优先级任务的优先级——但该更高优先级任务随后在未获取到互斥量的
 * 情况下超时，则较低优先级任务将再次放弃该优先级——但只会降低到
 * 仍在等待该互斥量的最高优先级任务的水平（若有多个任务在等待该互斥量）。
 */
void vTaskPriorityDisinheritAfterTimeout( TaskHandle_t const pxMutexHolder,
                                          UBaseType_t uxHighestPriorityWaitingTask ) PRIVILEGED_FUNCTION;

/*
 * 获取分配给 xTask 参数所引用任务的 uxTaskNumber。
 */
#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif

/*
 * 将 xTask 参数所引用任务的 uxTaskNumber 设置为 uxHandle。
 */
#if ( configUSE_TRACE_FACILITY == 1 )
    void vTaskSetTaskNumber( TaskHandle_t xTask,
                             const UBaseType_t uxHandle ) PRIVILEGED_FUNCTION;
#endif

/*
 * 仅当 configUSE_TICKLESS_IDLE 设置为 1 时可用。
 * 若使用无节拍模式，或实现了低功耗模式，则空闲期间不会执行节拍中断。
 * 在这种情况下，调度器维护的节拍计数值需要通过向前跳过等于空闲期的
 * 时间，来与实际执行时间保持一致。
 */
#if ( configUSE_TICKLESS_IDLE != 0 )
    void vTaskStepTick( TickType_t xTicksToJump ) PRIVILEGED_FUNCTION;
#endif

/*
 * 仅当 configUSE_TICKLESS_IDLE 设置为 1 时可用。
 * 提供此函数以便在 portSUPPRESS_TICKS_AND_SLEEP() 中使用，
 * 使移植层特定的睡眠函数能够判断是否可以继续进入睡眠，
 * 以及如果可以继续，是否可以无限期睡眠。
 *
 * 此函数是必要的，因为 portSUPPRESS_TICKS_AND_SLEEP() 仅在调度器挂起时
 * 被调用，而不是在临界区内调用。因此，在 portSUPPRESS_TICKS_AND_SLEEP()
 * 与实际进入低功耗模式之间，中断可能请求上下文切换。
 * 应在停止定时器之后、进入睡眠模式之前的短临界区内调用
 * eTaskConfirmSleepModeStatus()，以确保可以安全进入睡眠模式。
 */
#if ( configUSE_TICKLESS_IDLE != 0 )
    eSleepModeStatus eTaskConfirmSleepModeStatus( void ) PRIVILEGED_FUNCTION;
#endif

/*
 * 仅供内部使用。获取互斥量时递增互斥量持有计数，并返回已获取该互斥量的任务句柄。
 */
TaskHandle_t pvTaskIncrementMutexHeldCount( void ) PRIVILEGED_FUNCTION;

/*
 * 仅供内部使用。与 vTaskSetTimeOutState() 相同，但不使用临界区。
 */
void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut ) PRIVILEGED_FUNCTION;

/*
 * 仅供内部使用。在单核 FreeRTOS 中与 portYIELD_WITHIN_API() 相同。对于 SMP，移植层不定义此函数。
 */
#if ( configNUMBER_OF_CORES > 1 )
    void vTaskYieldWithinAPI( void );
#endif

/*
 * 此函数仅用于实现调度器移植，且仅在 portCRITICAL_NESTING_IN_TCB 设置为 1
 * 或 configNUMBER_OF_CORES 大于 1 时可用。若移植层希望在单核 FreeRTOS 中
 * 将临界区嵌套计数保存在 TCB 中，则可在 portENTER_CRITICAL 的实现中
 * 使用此函数。若移植层运行的是多核 FreeRTOS，则应在 portENTER_CRITICAL
 * 的实现中使用此函数。
 */
#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) || ( configNUMBER_OF_CORES > 1 ) )
    void vTaskEnterCritical( void );
#endif

/*
 * 此函数仅用于实现调度器移植，且仅在 portCRITICAL_NESTING_IN_TCB 设置为 1
 * 或 configNUMBER_OF_CORES 大于 1 时可用。若移植层希望在单核 FreeRTOS 中
 * 将临界区嵌套计数保存在 TCB 中，则可在 portEXIT_CRITICAL 的实现中
 * 使用此函数。若移植层运行的是多核 FreeRTOS，则应在 portEXIT_CRITICAL
 * 的实现中使用此函数。
 */
#if ( ( portCRITICAL_NESTING_IN_TCB == 1 ) || ( configNUMBER_OF_CORES > 1 ) )
    void vTaskExitCritical( void );
#endif

/*
 * 此函数仅用于实现调度器移植，且仅在 configNUMBER_OF_CORES 大于 1 时可用。
 * 若移植层运行的是多核 FreeRTOS，则应在 portENTER_CRITICAL_FROM_ISR
 * 的实现中使用此函数。
 */
#if ( configNUMBER_OF_CORES > 1 )
    UBaseType_t vTaskEnterCriticalFromISR( void );
#endif

/*
 * 此函数仅用于实现调度器移植，且仅在 configNUMBER_OF_CORES 大于 1 时可用。
 * 若移植层运行的是多核 FreeRTOS，则应在 portEXIT_CRITICAL_FROM_ISR
 * 的实现中使用此函数。
 */
#if ( configNUMBER_OF_CORES > 1 )
    void vTaskExitCriticalFromISR( UBaseType_t uxSavedInterruptStatus );
#endif

#if ( portUSING_MPU_WRAPPERS == 1 )

/*
 * 仅供内部使用。获取与任务关联的 MPU 设置。
 */
    xMPU_SETTINGS * xTaskGetMPUSettings( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

#endif /* portUSING_MPU_WRAPPERS（MPU 封装使能条件） */


#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

/*
 * 仅供内部使用。授予/撤销任务对内核对象的访问权限。
 */
    void vGrantAccessToKernelObject( TaskHandle_t xExternalTaskHandle,
                                     int32_t lExternalKernelObjectHandle ) PRIVILEGED_FUNCTION;
    void vRevokeAccessToKernelObject( TaskHandle_t xExternalTaskHandle,
                                      int32_t lExternalKernelObjectHandle ) PRIVILEGED_FUNCTION;

/*
 * 仅供内部使用。授予/撤销任务对内核对象的访问权限。
 */
    void vPortGrantAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                         int32_t lInternalIndexOfKernelObject ) PRIVILEGED_FUNCTION;
    void vPortRevokeAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                          int32_t lInternalIndexOfKernelObject ) PRIVILEGED_FUNCTION;

#endif /* #if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )（MPU 访问控制条件） */

/* *INDENT-OFF* 关闭自动缩进 */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* 恢复自动缩进 */
#endif /* INC_TASK_H（头文件包含保护） */
