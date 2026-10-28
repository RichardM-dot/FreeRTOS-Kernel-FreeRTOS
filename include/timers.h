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


#ifndef TIMERS_H
#define TIMERS_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include timers.h"
#endif

#include "task.h"


/* *INDENT-OFF* —— 暂停自动缩进（uncrustify 指令） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* —— 恢复自动缩进（uncrustify 指令） */

/*
 * -----------------------------------------------------------
 * 宏与定义
 * ----------------------------------------------------------
 */

/*
 * 可在定时器命令队列上发送/接收的命令 ID。这些命令只能通过下文定义的公共
 * 软件定时器 API 宏使用。从中断发送的命令必须使用最大的编号，因为
 * tmrFIRST_FROM_ISR_COMMAND 用于判断应调用队列发送函数的任务版本还是中断版本。
 */
#define tmrCOMMAND_EXECUTE_CALLBACK_FROM_ISR    ( ( BaseType_t ) -2 )
#define tmrCOMMAND_EXECUTE_CALLBACK             ( ( BaseType_t ) -1 )
#define tmrCOMMAND_START_DONT_TRACE             ( ( BaseType_t ) 0 )
#define tmrCOMMAND_START                        ( ( BaseType_t ) 1 )
#define tmrCOMMAND_RESET                        ( ( BaseType_t ) 2 )
#define tmrCOMMAND_STOP                         ( ( BaseType_t ) 3 )
#define tmrCOMMAND_CHANGE_PERIOD                ( ( BaseType_t ) 4 )
#define tmrCOMMAND_DELETE                       ( ( BaseType_t ) 5 )

#define tmrFIRST_FROM_ISR_COMMAND               ( ( BaseType_t ) 6 )
#define tmrCOMMAND_START_FROM_ISR               ( ( BaseType_t ) 6 )
#define tmrCOMMAND_RESET_FROM_ISR               ( ( BaseType_t ) 7 )
#define tmrCOMMAND_STOP_FROM_ISR                ( ( BaseType_t ) 8 )
#define tmrCOMMAND_CHANGE_PERIOD_FROM_ISR       ( ( BaseType_t ) 9 )


/**
 * 软件定时器的引用类型。例如，调用 xTimerCreate() 会返回一个 TimerHandle_t
 * 变量，随后可在调用其他软件定时器 API 函数（如 xTimerStart()、xTimerReset()
 * 等）时使用该变量引用相应的定时器。
 */
struct tmrTimerControl; /* 沿用旧命名约定，以免破坏内核感知调试器。 */
typedef struct tmrTimerControl * TimerHandle_t;

/*
 * 定义定时器回调函数必须遵循的原型。
 */
typedef void (* TimerCallbackFunction_t)( TimerHandle_t xTimer );

/*
 * 定义与 xTimerPendFunctionCallFromISR() 函数配合使用的函数必须遵循的原型。
 */
typedef void (* PendedFunction_t)( void * arg1,
                                   uint32_t arg2 );

/**
 * TimerHandle_t xTimerCreate(  const char * const pcTimerName,
 *                              TickType_t xTimerPeriodInTicks,
 *                              BaseType_t xAutoReload,
 *                              void * pvTimerID,
 *                              TimerCallbackFunction_t pxCallbackFunction );
 *
 * 创建一个新的软件定时器实例，并返回用于引用该定时器的句柄。
 *
 * 在 FreeRTOS 内部实现中，软件定时器使用一块内存来存放定时器数据结构。若使用
 * xTimerCreate() 创建软件定时器，所需内存会在 xTimerCreate() 函数内部自动动态
 * 分配（参见 https://www.FreeRTOS.org/a00111.html）。若使用 xTimerCreateStatic()
 * 创建软件定时器，则必须由应用编写者提供该软件定时器将要使用的内存。因此，
 * xTimerCreateStatic() 允许在不使用任何动态内存分配的情况下创建软件定时器。
 *
 * 定时器创建后处于休眠状态。xTimerStart()、xTimerReset()、xTimerStartFromISR()、
 * xTimerResetFromISR()、xTimerChangePeriod() 和 xTimerChangePeriodFromISR() API
 * 函数均可用于将定时器转换为活动状态。
 *
 * @param pcTimerName 分配给定时器的文本名称。该名称纯粹用于辅助调试。内核本身
 * 只通过句柄引用定时器，从不使用其名称。
 *
 * @param xTimerPeriodInTicks 定时器周期。时间以 tick 周期为单位定义，因此可使用
 * 常量 portTICK_PERIOD_MS 将以毫秒指定的时间转换为 tick 数。例如，若定时器必须在
 * 100 个 tick 后到期，则 xTimerPeriodInTicks 应设为 100。另一种情况，若定时器必须在
 * 500ms 后到期，则在 configTICK_RATE_HZ 小于或等于 1000 的前提下，可将 xPeriod 设为
 * ( 500 / portTICK_PERIOD_MS )。定时器周期必须大于 0。
 *
 * @param xAutoReload 若 xAutoReload 设为 pdTRUE，则定时器将按 xTimerPeriodInTicks
 * 参数设定的频率反复到期。若 xAutoReload 设为 pdFALSE，则定时器为单次定时器，
 * 到期后进入休眠状态。
 *
 * @param pvTimerID 分配给所创建定时器的标识符。通常用于在同一个回调函数被分配给
 * 多个定时器时，在定时器回调函数中识别是哪个定时器到期。
 *
 * @param pxCallbackFunction 定时器到期时要调用的函数。回调函数必须采用
 * TimerCallbackFunction_t 定义的原型，即 "void vCallbackFunction( TimerHandle_t xTimer );"。
 *
 * @return 若定时器创建成功，则返回新创建定时器的句柄。若由于剩余 FreeRTOS 堆不足
 * 以分配定时器结构而无法创建定时器，则返回 NULL。
 *
 * 用法示例：
 * @verbatim
 * #define NUM_TIMERS 5
 *
 * // 用于保存所创建定时器句柄的数组。
 * TimerHandle_t xTimers[ NUM_TIMERS ];
 *
 * // 用于记录每个定时器到期次数的计数数组。
 * int32_t lExpireCounters[ NUM_TIMERS ] = { 0 };
 *
 * // 定义一个供多个定时器实例共用的回调函数。
 * // 该回调函数只统计相应定时器的到期次数，并在定时器到期 10 次后将其停止。
 * void vTimerCallback( TimerHandle_t pxTimer )
 * {
 * int32_t lArrayIndex;
 * const int32_t xMaxExpiryCountBeforeStopping = 10;
 *
 *     // 可选：若 pxTimer 参数为 NULL 则做相应处理。
 *     configASSERT( pxTimer );
 *
 *     // 是哪个定时器到期了？
 *     lArrayIndex = ( int32_t ) pvTimerGetTimerID( pxTimer );
 *
 *     // 递增 pxTimer 的到期次数。
 *     lExpireCounters[ lArrayIndex ] += 1;
 *
 *     // 若定时器已到期 10 次，则停止其运行。
 *     if( lExpireCounters[ lArrayIndex ] == xMaxExpiryCountBeforeStopping )
 *     {
 *         // 在定时器回调函数中调用定时器 API 函数时不要使用阻塞时间，
 *         // 否则可能导致死锁！
 *         xTimerStop( pxTimer, 0 );
 *     }
 * }
 *
 * void main( void )
 * {
 * int32_t x;
 *
 *     // 创建并启动若干定时器。在调度器启动之前启动定时器，
 *     // 意味着调度器启动后这些定时器会立即开始运行。
 *     for( x = 0; x < NUM_TIMERS; x++ )
 *     {
 *         xTimers[ x ] = xTimerCreate(    "Timer",             // 仅为文本名称，内核不使用。
 *                                         ( 100 * ( x + 1 ) ), // 定时器周期（以 tick 为单位）。
 *                                         pdTRUE,              // 定时器到期后将自动重载。
 *                                         ( void * ) x,        // 为每个定时器分配等于其数组下标的唯一 ID。
 *                                         vTimerCallback       // 每个定时器到期时都调用同一个回调函数。
 *                                     );
 *
 *         if( xTimers[ x ] == NULL )
 *         {
 *             // 定时器创建失败。
 *         }
 *         else
 *         {
 *             // 启动定时器。此处未指定阻塞时间；即使指定了也会被忽略，
 *             // 因为调度器尚未启动。
 *             if( xTimerStart( xTimers[ x ], 0 ) != pdPASS )
 *             {
 *                 // 定时器未能进入活动状态。
 *             }
 *         }
 *     }
 *
 *     // ...
 *     // 在此创建任务。
 *     // ...
 *
 *     // 启动调度器后，这些定时器将开始运行，因为它们已进入活动状态。
 *     vTaskStartScheduler();
 *
 *     // 不应执行到此处。
 *     for( ;; );
 * }
 * @endverbatim
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    TimerHandle_t xTimerCreate( const char * const pcTimerName,
                                const TickType_t xTimerPeriodInTicks,
                                const BaseType_t xAutoReload,
                                void * const pvTimerID,
                                TimerCallbackFunction_t pxCallbackFunction ) PRIVILEGED_FUNCTION;
#endif

/**
 * TimerHandle_t xTimerCreateStatic(const char * const pcTimerName,
 *                                  TickType_t xTimerPeriodInTicks,
 *                                  BaseType_t xAutoReload,
 *                                  void * pvTimerID,
 *                                  TimerCallbackFunction_t pxCallbackFunction,
 *                                  StaticTimer_t *pxTimerBuffer );
 *
 * 创建一个新的软件定时器实例，并返回用于引用该定时器的句柄。
 *
 * 在 FreeRTOS 内部实现中，软件定时器使用一块内存来存放定时器数据结构。若使用
 * xTimerCreate() 创建软件定时器，所需内存会在 xTimerCreate() 函数内部自动动态
 * 分配（参见 https://www.FreeRTOS.org/a00111.html）。若使用 xTimerCreateStatic()
 * 创建软件定时器，则必须由应用编写者提供该软件定时器将要使用的内存。因此，
 * xTimerCreateStatic() 允许在不使用任何动态内存分配的情况下创建软件定时器。
 *
 * 定时器创建后处于休眠状态。xTimerStart()、xTimerReset()、xTimerStartFromISR()、
 * xTimerResetFromISR()、xTimerChangePeriod() 和 xTimerChangePeriodFromISR() API
 * 函数均可用于将定时器转换为活动状态。
 *
 * @param pcTimerName 分配给定时器的文本名称。该名称纯粹用于辅助调试。内核本身
 * 只通过句柄引用定时器，从不使用其名称。
 *
 * @param xTimerPeriodInTicks 定时器周期。时间以 tick 周期为单位定义，因此可使用
 * 常量 portTICK_PERIOD_MS 将以毫秒指定的时间转换为 tick 数。例如，若定时器必须在
 * 100 个 tick 后到期，则 xTimerPeriodInTicks 应设为 100。另一种情况，若定时器必须在
 * 500ms 后到期，则在 configTICK_RATE_HZ 小于或等于 1000 的前提下，可将 xPeriod 设为
 * ( 500 / portTICK_PERIOD_MS )。定时器周期必须大于 0。
 *
 * @param xAutoReload 若 xAutoReload 设为 pdTRUE，则定时器将按 xTimerPeriodInTicks
 * 参数设定的频率反复到期。若 xAutoReload 设为 pdFALSE，则定时器为单次定时器，
 * 到期后进入休眠状态。
 *
 * @param pvTimerID 分配给所创建定时器的标识符。通常用于在同一个回调函数被分配给
 * 多个定时器时，在定时器回调函数中识别是哪个定时器到期。
 *
 * @param pxCallbackFunction 定时器到期时要调用的函数。回调函数必须采用
 * TimerCallbackFunction_t 定义的原型，即 "void vCallbackFunction( TimerHandle_t xTimer );"。
 *
 * @param pxTimerBuffer 必须指向一个 StaticTimer_t 类型的变量，该变量随后用于存放
 * 软件定时器的数据结构，从而无需动态分配内存。
 *
 * @return 若定时器创建成功，则返回所创建定时器的句柄。若 pxTimerBuffer 为 NULL，
 * 则返回 NULL。
 *
 * 用法示例：
 * @verbatim
 *
 * // 用于存放软件定时器数据结构的缓冲区。
 * static StaticTimer_t xTimerBuffer;
 *
 * // 将由软件定时器回调函数递增的变量。
 * UBaseType_t uxVariableToIncrement = 0;
 *
 * // 一个软件定时器回调函数：当软件定时器创建时传入的变量会被该函数递增。
 * // 第 5 次递增后，回调函数将停止该软件定时器。
 * static void prvTimerCallback( TimerHandle_t xExpiredTimer )
 * {
 * UBaseType_t *puxVariableToIncrement;
 * BaseType_t xReturned;
 *
 *     // 从定时器 ID 获取待递增变量的地址。
 *     puxVariableToIncrement = ( UBaseType_t * ) pvTimerGetTimerID( xExpiredTimer );
 *
 *     // 递增变量，以表明定时器回调已执行。
 *     ( *puxVariableToIncrement )++;
 *
 *     // 若该回调已执行所需次数，则停止定时器。
 *     if( *puxVariableToIncrement == 5 )
 *     {
 *         // 此处由定时器回调调用，因此绝不能阻塞。
 *         xTimerStop( xExpiredTimer, staticDONT_BLOCK );
 *     }
 * }
 *
 *
 * void main( void )
 * {
 *     // 创建软件定时器。xTimerCreateStatic() 比普通 xTimerCreate() API 函数多一个
 *     // 参数，该参数是指向 StaticTimer_t 结构的指针，用于存放软件定时器结构。
 *     // 若该参数传入 NULL，则会动态分配该结构，如同调用 xTimerCreate() 一样。
 *     xTimer = xTimerCreateStatic( "T1",             // 任务的文本名称。仅用于辅助调试，FreeRTOS 不使用。
 *                                  xTimerPeriod,     // 定时器周期（以 tick 为单位）。
 *                                  pdTRUE,           // 这是自动重载定时器。
 *                                  ( void * ) &uxVariableToIncrement,    // 由软件定时器回调函数递增的变量
 *                                  prvTimerCallback, // 定时器到期时要执行的函数。
 *                                  &xTimerBuffer );  // 用于存放软件定时器结构的缓冲区。
 *
 *     // 调度器尚未启动，因此不使用阻塞时间。
 *     xReturned = xTimerStart( xTimer, 0 );
 *
 *     // ...
 *     // 在此创建任务。
 *     // ...
 *
 *     // 启动调度器后，这些定时器将开始运行，因为它们已进入活动状态。
 *     vTaskStartScheduler();
 *
 *     // 不应执行到此处。
 *     for( ;; );
 * }
 * @endverbatim
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    TimerHandle_t xTimerCreateStatic( const char * const pcTimerName,
                                      const TickType_t xTimerPeriodInTicks,
                                      const BaseType_t xAutoReload,
                                      void * const pvTimerID,
                                      TimerCallbackFunction_t pxCallbackFunction,
                                      StaticTimer_t * pxTimerBuffer ) PRIVILEGED_FUNCTION;
#endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */

/**
 * void *pvTimerGetTimerID( TimerHandle_t xTimer );
 *
 * 返回分配给定时器的 ID。
 *
 * 创建定时器时，通过调用 xTimerCreate() 的 pvTimerID 参数为定时器分配 ID；
 * 也可以通过调用 vTimerSetTimerID() API 函数分配。
 *
 * 若同一个回调函数被分配给多个定时器，则可将定时器 ID 用作定时器专用
 * （定时器本地）存储。
 *
 * @param xTimer 被查询的定时器。
 *
 * @return 分配给被查询定时器的 ID。
 *
 * 用法示例：
 *
 * 参见 xTimerCreate() API 函数的用法示例场景。
 */
void * pvTimerGetTimerID( const TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * void vTimerSetTimerID( TimerHandle_t xTimer, void *pvNewID );
 *
 * 设置分配给定时器的 ID。
 *
 * 创建定时器时，通过调用 xTimerCreate() 的 pvTimerID 参数为定时器分配 ID。
 *
 * 若同一个回调函数被分配给多个定时器，则可将定时器 ID 用作定时器专用
 * （定时器本地）存储。
 *
 * @param xTimer 被更新的定时器。
 *
 * @param pvNewID 要分配给定时器的 ID。
 *
 * 用法示例：
 *
 * 参见 xTimerCreate() API 函数的用法示例场景。
 */
void vTimerSetTimerID( TimerHandle_t xTimer,
                       void * pvNewID ) PRIVILEGED_FUNCTION;

/**
 * BaseType_t xTimerIsTimerActive( TimerHandle_t xTimer );
 *
 * 查询定时器处于活动状态还是休眠状态。
 *
 * 定时器处于休眠状态的条件为：
 *     1) 已创建但尚未启动，或
 *     2) 是已到期的单次定时器且尚未重新启动。
 *
 * 定时器创建后处于休眠状态。xTimerStart()、xTimerReset()、xTimerStartFromISR()、
 * xTimerResetFromISR()、xTimerChangePeriod() 和 xTimerChangePeriodFromISR() API
 * 函数均可用于将定时器转换为活动状态。
 *
 * @param xTimer 被查询的定时器。
 *
 * @return 若定时器处于休眠状态，则返回 pdFALSE；若定时器处于活动状态，则返回
 * 非 pdFALSE 的值。
 *
 * 用法示例：
 * @verbatim
 * // 此函数假定 xTimer 已经创建。
 * void vAFunction( TimerHandle_t xTimer )
 * {
 *     if( xTimerIsTimerActive( xTimer ) != pdFALSE ) // 或更简单等价的写法 "if( xTimerIsTimerActive( xTimer ) )"
 *     {
 *         // xTimer 处于活动状态，执行相应操作。
 *     }
 *     else
 *     {
 *         // xTimer 未处于活动状态，执行其他操作。
 *     }
 * }
 * @endverbatim
 */
BaseType_t xTimerIsTimerActive( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * TaskHandle_t xTimerGetTimerDaemonTaskHandle( void );
 *
 * 直接返回定时器服务任务（守护任务）的句柄。在调度器启动之前调用
 * xTimerGetTimerDaemonTaskHandle() 是无效的。
 */
TaskHandle_t xTimerGetTimerDaemonTaskHandle( void ) PRIVILEGED_FUNCTION;

/**
 * BaseType_t xTimerStart( TimerHandle_t xTimer, TickType_t xTicksToWait );
 *
 * 定时器功能由定时器服务任务（守护任务）提供。许多公共 FreeRTOS 定时器 API
 * 函数通过名为定时器命令队列的队列向定时器服务任务发送命令。定时器命令队列是
 * 内核私有的，应用代码不能直接访问。定时器命令队列的长度由配置常量
 * configTIMER_QUEUE_LENGTH 设置。
 *
 * xTimerStart() 用于启动先前通过 xTimerCreate() API 函数创建的定时器。若定时器
 * 已经启动并处于活动状态，则 xTimerStart() 的功能与 xTimerReset() API 函数等效。
 *
 * 启动定时器可确保定时器处于活动状态。若在此期间定时器未被停止、删除或复位，
 * 则与定时器关联的回调函数将在调用 xTimerStart() 之后 'n' 个 tick 时被调用，
 * 其中 'n' 是定时器定义的周期。
 *
 * 在调度器启动之前调用 xTimerStart() 是有效的，但此时定时器要到调度器启动后才会
 * 真正启动，且定时器的到期时间相对于调度器启动时刻，而非相对于调用
 * xTimerStart() 的时刻。
 *
 * 要使 xTimerStart() 可用，配置常量 configUSE_TIMERS 必须设置为 1。
 *
 * @param xTimer 被启动/重新启动的定时器句柄。
 *
 * @param xTicksToWait 指定调用任务应保持在阻塞态等待的时间（以 tick 为单位），
 * 用于等待启动命令成功发送到定时器命令队列——前提是调用 xTimerStart() 时队列
 * 已满。若在调度器启动之前调用 xTimerStart()，则忽略 xTicksToWait。
 *
 * @return 若经过 xTicksToWait 个 tick 后启动命令仍无法发送到定时器命令队列，
 * 则返回 pdFAIL。若命令成功发送到定时器命令队列，则返回 pdPASS。命令的实际处理
 * 时机取决于定时器服务任务（守护任务）相对于系统中其他任务的优先级，但定时器的
 * 到期时间相对于实际调用 xTimerStart() 的时刻。定时器服务任务（守护任务）的
 * 优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 *
 * 参见 xTimerCreate() API 函数的用法示例场景。
 */
#define xTimerStart( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START, ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )

/**
 * BaseType_t xTimerStop( TimerHandle_t xTimer, TickType_t xTicksToWait );
 *
 * 定时器功能由定时器服务任务（守护任务）提供。许多公共 FreeRTOS 定时器 API
 * 函数通过名为定时器命令队列的队列向定时器服务任务发送命令。定时器命令队列是
 * 内核私有的，应用代码不能直接访问。定时器命令队列的长度由配置常量
 * configTIMER_QUEUE_LENGTH 设置。
 *
 * xTimerStop() 用于停止先前通过 xTimerStart()、xTimerReset()、
 * xTimerStartFromISR()、xTimerResetFromISR()、xTimerChangePeriod() 或
 * xTimerChangePeriodFromISR() API 函数启动的定时器。
 *
 * 停止定时器可确保定时器不处于活动状态。
 *
 * 要使 xTimerStop() 可用，配置常量 configUSE_TIMERS 必须设置为 1。
 *
 * @param xTimer 被停止的定时器句柄。
 *
 * @param xTicksToWait 指定调用任务应保持在阻塞态等待的时间（以 tick 为单位），
 * 用于等待停止命令成功发送到定时器命令队列——前提是调用 xTimerStop() 时队列
 * 已满。若在调度器启动之前调用 xTimerStop()，则忽略 xTicksToWait。
 *
 * @return 若经过 xTicksToWait 个 tick 后停止命令仍无法发送到定时器命令队列，
 * 则返回 pdFAIL。若命令成功发送到定时器命令队列，则返回 pdPASS。命令的实际处理
 * 时机取决于定时器服务任务（守护任务）相对于系统中其他任务的优先级。定时器
 * 服务任务（守护任务）的优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 *
 * 参见 xTimerCreate() API 函数的用法示例场景。
 */
#define xTimerStop( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_STOP, 0U, NULL, ( xTicksToWait ) )

/**
 * BaseType_t xTimerChangePeriod(   TimerHandle_t xTimer,
 *                                  TickType_t xNewPeriod,
 *                                  TickType_t xTicksToWait );
 *
 * 定时器功能由定时器服务任务（守护任务）提供。许多公共 FreeRTOS 定时器 API
 * 函数通过名为定时器命令队列的队列向定时器服务任务发送命令。定时器命令队列是
 * 内核私有的，应用代码不能直接访问。定时器命令队列的长度由配置常量
 * configTIMER_QUEUE_LENGTH 设置。
 *
 * xTimerChangePeriod() 用于更改先前通过 xTimerCreate() API 函数创建的定时器的
 * 周期。
 *
 * xTimerChangePeriod() 可用于更改活动状态或休眠状态定时器的周期。
 *
 * 要使 xTimerChangePeriod() 可用，配置常量 configUSE_TIMERS 必须设置为 1。
 *
 * @param xTimer 要更改周期的定时器句柄。
 *
 * @param xNewPeriod xTimer 的新周期。定时器周期以 tick 周期为单位指定，因此可
 * 使用常量 portTICK_PERIOD_MS 将以毫秒指定的时间转换为 tick 数。例如，若定时器
 * 必须在 100 个 tick 后到期，则 xNewPeriod 应设为 100。另一种情况，若定时器必须
 * 在 500ms 后到期，则在 configTICK_RATE_HZ 小于或等于 1000 的前提下，可将
 * xNewPeriod 设为 ( 500 / portTICK_PERIOD_MS )。
 *
 * @param xTicksToWait 指定调用任务应保持在阻塞态等待的时间（以 tick 为单位），
 * 用于等待更改周期命令成功发送到定时器命令队列——前提是调用
 * xTimerChangePeriod() 时队列已满。若在调度器启动之前调用 xTimerChangePeriod()，
 * 则忽略 xTicksToWait。
 *
 * @return 若经过 xTicksToWait 个 tick 后更改周期命令仍无法发送到定时器命令队列，
 * 则返回 pdFAIL。若命令成功发送到定时器命令队列，则返回 pdPASS。命令的实际处理
 * 时机取决于定时器服务任务（守护任务）相对于系统中其他任务的优先级。定时器
 * 服务任务（守护任务）的优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 * @verbatim
 * // 此函数假定 xTimer 已经创建。若调用时 xTimer 引用的定时器已处于活动状态，
 * // 则删除该定时器。若调用时 xTimer 引用的定时器未处于活动状态，则将定时器
 * // 周期设为 500ms 并启动该定时器。
 * void vAFunction( TimerHandle_t xTimer )
 * {
 *     if( xTimerIsTimerActive( xTimer ) != pdFALSE ) // 或更简单等价的写法 "if( xTimerIsTimerActive( xTimer ) )"
 *     {
 *         // xTimer 已处于活动状态——将其删除。
 *         xTimerDelete( xTimer );
 *     }
 *     else
 *     {
 *         // xTimer 未处于活动状态，将其周期改为 500ms。这也会使定时器启动。
 *         // 若更改周期命令不能立即发送到定时器命令队列，则最多阻塞 100 个 tick。
 *         if( xTimerChangePeriod( xTimer, 500 / portTICK_PERIOD_MS, 100 ) == pdPASS )
 *         {
 *             // 命令发送成功。
 *         }
 *         else
 *         {
 *             // 即使等待 100 个 tick 后命令仍无法发送。请在此采取适当措施。
 *         }
 *     }
 * }
 * @endverbatim
 */
#define xTimerChangePeriod( xTimer, xNewPeriod, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_CHANGE_PERIOD, ( xNewPeriod ), NULL, ( xTicksToWait ) )

/**
 * BaseType_t xTimerDelete( TimerHandle_t xTimer, TickType_t xTicksToWait );
 *
 * 定时器功能由定时器服务任务（守护任务）提供。许多公共 FreeRTOS 定时器 API
 * 函数通过名为定时器命令队列的队列向定时器服务任务发送命令。定时器命令队列是
 * 内核私有的，应用代码不能直接访问。定时器命令队列的长度由配置常量
 * configTIMER_QUEUE_LENGTH 设置。
 *
 * xTimerDelete() 用于删除先前通过 xTimerCreate() API 函数创建的定时器。
 *
 * 要使 xTimerDelete() 可用，配置常量 configUSE_TIMERS 必须设置为 1。
 *
 * @param xTimer 被删除的定时器句柄。
 *
 * @param xTicksToWait 指定调用任务应保持在阻塞态等待的时间（以 tick 为单位），
 * 用于等待删除命令成功发送到定时器命令队列——前提是调用 xTimerDelete() 时队列
 * 已满。若在调度器启动之前调用 xTimerDelete()，则忽略 xTicksToWait。
 *
 * @return 若经过 xTicksToWait 个 tick 后删除命令仍无法发送到定时器命令队列，
 * 则返回 pdFAIL。若命令成功发送到定时器命令队列，则返回 pdPASS。命令的实际处理
 * 时机取决于定时器服务任务（守护任务）相对于系统中其他任务的优先级。定时器
 * 服务任务（守护任务）的优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 *
 * 参见 xTimerChangePeriod() API 函数的用法示例场景。
 */
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )
    BaseType_t xTimerDelete( TimerHandle_t xTimer,
                             TickType_t xTicksToWait );
#else
    #define xTimerDelete( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_DELETE, 0U, NULL, ( xTicksToWait ) )
#endif

/**
 * BaseType_t xTimerReset( TimerHandle_t xTimer, TickType_t xTicksToWait );
 *
 * 定时器功能由定时器服务任务（守护任务）提供。许多公共 FreeRTOS 定时器 API
 * 函数通过名为定时器命令队列的队列向定时器服务任务发送命令。定时器命令队列是
 * 内核私有的，应用代码不能直接访问。定时器命令队列的长度由配置常量
 * configTIMER_QUEUE_LENGTH 设置。
 *
 * xTimerReset() 用于重新启动先前通过 xTimerCreate() API 函数创建的定时器。若
 * 定时器已经启动并处于活动状态，则 xTimerReset() 会使定时器重新计算其到期时间，
 * 使其相对于调用 xTimerReset() 的时刻。若定时器处于休眠状态，则 xTimerReset()
 * 的功能与 xTimerStart() API 函数等效。
 *
 * 复位定时器可确保定时器处于活动状态。若在此期间定时器未被停止、删除或复位，
 * 则与定时器关联的回调函数将在调用 xTimerReset() 之后 'n' 个 tick 时被调用，
 * 其中 'n' 是定时器定义的周期。
 *
 * 在调度器启动之前调用 xTimerReset() 是有效的，但此时定时器要到调度器启动后才会
 * 真正启动，且定时器的到期时间相对于调度器启动时刻，而非相对于调用
 * xTimerReset() 的时刻。
 *
 * 要使 xTimerReset() 可用，配置常量 configUSE_TIMERS 必须设置为 1。
 *
 * @param xTimer 被复位/启动/重新启动的定时器句柄。
 *
 * @param xTicksToWait 指定调用任务应保持在阻塞态等待的时间（以 tick 为单位），
 * 用于等待复位命令成功发送到定时器命令队列——前提是调用 xTimerReset() 时队列
 * 已满。若在调度器启动之前调用 xTimerReset()，则忽略 xTicksToWait。
 *
 * @return 若经过 xTicksToWait 个 tick 后复位命令仍无法发送到定时器命令队列，
 * 则返回 pdFAIL。若命令成功发送到定时器命令队列，则返回 pdPASS。命令的实际处理
 * 时机取决于定时器服务任务（守护任务）相对于系统中其他任务的优先级，但定时器的
 * 到期时间相对于实际调用 xTimerStart() 的时刻。定时器服务任务（守护任务）的
 * 优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 * @verbatim
 * // 按下按键时点亮 LCD 背光。若 5 秒内没有按键按下，则关闭 LCD 背光。
 * // 在此场景中，定时器为单次定时器。
 *
 * TimerHandle_t xBacklightTimer = NULL;
 *
 * // 分配给该单次定时器的回调函数。在此场景中不使用该参数。
 * void vBacklightTimerCallback( TimerHandle_t pxTimer )
 * {
 *     // 定时器已到期，因此距上次按键必定已过去 5 秒。关闭 LCD 背光。
 *     vSetBacklightState( BACKLIGHT_OFF );
 * }
 *
 * // 按键事件处理函数。
 * void vKeyPressEventHandler( char cKey )
 * {
 *     // 复位负责在按键无操作 5 秒后关闭背光的定时器。若命令不能立即发送，
 *     // 则最多等待 10 个 tick 以等待其成功发送。
 *     if( xTimerReset( xBacklightTimer, 10 ) == pdPASS )
 *     {
 *        // 点亮 LCD 背光。若 5 秒内无按键操作，vBacklightTimerCallback
 *        // 将在之后将其关闭。
 *        vSetBacklightState( BACKLIGHT_ON );
 *      }
 *     else
 *     {
 *         // 复位命令未成功执行。请在此采取适当措施。
 *     }
 *
 *     // 在此执行按键的其余处理。
 * }
 *
 * void main( void )
 * {
 *
 *     // 创建并启动单次定时器，用于在 5 秒内无按键按下时关闭背光。
 *     xBacklightTimer = xTimerCreate( "BacklightTimer",           // 仅为文本名称，内核不使用。
 *                                     pdMS_TO_TICKS( 5000 ),      // 定时器周期（以 tick 为单位）。
 *                                     pdFALSE,                    // 该定时器为单次定时器。
 *                                     0,                          // 回调不使用该 ID，因此可取任意值。
 *                                     vBacklightTimerCallback     // 关闭 LCD 背光的回调函数。
 *                                    );
 *
 *     if( xBacklightTimer == NULL )
 *     {
 *         // 定时器创建失败。
 *     }
 *     else
 *     {
 *         // 启动定时器。此处未指定阻塞时间；即使指定了也会被忽略，
 *         // 因为调度器尚未启动。
 *         if( xTimerStart( xBacklightTimer, 0 ) != pdPASS )
 *         {
 *             // 定时器未能进入活动状态。
 *         }
 *     }
 *
 *     // ...
 *     // 在此创建任务。
 *     // ...
 *
 *     // 启动调度器后，定时器将开始运行，因为它已进入活动状态。
 *     vTaskStartScheduler();
 *
 *     // 不应执行到此处。
 *     for( ;; );
 * }
 * @endverbatim
 */
#define xTimerReset( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_RESET, ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )

/**
 * BaseType_t xTimerStartFromISR(   TimerHandle_t xTimer,
 *                                  BaseType_t *pxHigherPriorityTaskWoken );
 *
 * xTimerStart() 的中断服务程序版本。
 *
 * @param xTimer 被启动/重新启动的定时器句柄。
 *
 * @param pxHigherPriorityTaskWoken 定时器服务任务（守护任务）大部分时间处于阻塞态，
 * 等待定时器命令队列上的消息到达。调用 xTimerStartFromISR() 会向定时器命令队列
 * 写入一条消息，因此有可能使定时器服务任务（守护任务）离开阻塞态。若调用
 * xTimerStartFromISR() 使定时器服务任务（守护任务）离开阻塞态，且该定时器服务
 * 任务（守护任务）的优先级等于或高于当前正在执行的任务（被中断的任务），则在
 * xTimerStartFromISR() 函数内部会将 *pxHigherPriorityTaskWoken 设为 pdTRUE。
 * 若 xTimerStartFromISR() 将该值设为 pdTRUE，则应在中断退出之前执行上下文切换。
 *
 * @return 若启动命令无法发送到定时器命令队列，则返回 pdFAIL。若命令成功发送到
 * 定时器命令队列，则返回 pdPASS。命令的实际处理时机取决于定时器服务任务
 * （守护任务）相对于系统中其他任务的优先级，但定时器的到期时间相对于实际调用
 * xTimerStartFromISR() 的时刻。定时器服务任务（守护任务）的优先级由配置常量
 * configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 * @verbatim
 * // 此场景假定 xBacklightTimer 已经创建。按下按键时点亮 LCD 背光。若 5 秒内
 * // 没有按键按下，则关闭 LCD 背光。在此场景中，定时器为单次定时器；与
 * // xTimerReset() 函数的示例不同，按键事件处理函数是中断服务程序。
 *
 * // 分配给该单次定时器的回调函数。在此场景中不使用该参数。
 * void vBacklightTimerCallback( TimerHandle_t pxTimer )
 * {
 *     // 定时器已到期，因此距上次按键必定已过去 5 秒。关闭 LCD 背光。
 *     vSetBacklightState( BACKLIGHT_OFF );
 * }
 *
 * // 按键中断服务程序。
 * void vKeyPressEventInterruptHandler( void )
 * {
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 *
 *     // 确保 LCD 背光已点亮，然后重新启动负责在按键无操作 5 秒后关闭
 *     // 背光的定时器。这是中断服务程序，因此只能调用以 "FromISR" 结尾的
 *     // FreeRTOS API 函数。
 *     vSetBacklightState( BACKLIGHT_ON );
 *
 *     // 此处可调用 xTimerStartFromISR() 或 xTimerResetFromISR()，
 *     // 因为两者都会使定时器重新计算其到期时间。
 *     // xHigherPriorityTaskWoken 在声明时（在本函数中）已初始化为 pdFALSE。
 *     if( xTimerStartFromISR( xBacklightTimer, &xHigherPriorityTaskWoken ) != pdPASS )
 *     {
 *         // 启动命令未成功执行。请在此采取适当措施。
 *     }
 *
 *     // 在此执行按键的其余处理。
 *
 *     // 若 xHigherPriorityTaskWoken 等于 pdTRUE，则应执行上下文切换。
 *     // 在 ISR 内部执行上下文切换所需的语法因移植层和编译器而异。
 *     // 请查看所用移植层的示例，以确定实际所需的语法。
 *     if( xHigherPriorityTaskWoken != pdFALSE )
 *     {
 *         // 在此调用中断安全的让出函数（实际函数取决于所用的 FreeRTOS 移植层）。
 *     }
 * }
 * @endverbatim
 */
#define xTimerStartFromISR( xTimer, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START_FROM_ISR, ( xTaskGetTickCountFromISR() ), ( pxHigherPriorityTaskWoken ), 0U )

/**
 * BaseType_t xTimerStopFromISR(    TimerHandle_t xTimer,
 *                                  BaseType_t *pxHigherPriorityTaskWoken );
 *
 * xTimerStop() 的中断服务程序版本。
 *
 * @param xTimer 被停止的定时器句柄。
 *
 * @param pxHigherPriorityTaskWoken 定时器服务任务（守护任务）大部分时间处于阻塞态，
 * 等待定时器命令队列上的消息到达。调用 xTimerStopFromISR() 会向定时器命令队列
 * 写入一条消息，因此有可能使定时器服务任务（守护任务）离开阻塞态。若调用
 * xTimerStopFromISR() 使定时器服务任务（守护任务）离开阻塞态，且该定时器服务
 * 任务（守护任务）的优先级等于或高于当前正在执行的任务（被中断的任务），则在
 * xTimerStopFromISR() 函数内部会将 *pxHigherPriorityTaskWoken 设为 pdTRUE。
 * 若 xTimerStopFromISR() 将该值设为 pdTRUE，则应在中断退出之前执行上下文切换。
 *
 * @return 若停止命令无法发送到定时器命令队列，则返回 pdFAIL。若命令成功发送到
 * 定时器命令队列，则返回 pdPASS。命令的实际处理时机取决于定时器服务任务
 * （守护任务）相对于系统中其他任务的优先级。定时器服务任务（守护任务）的
 * 优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 * @verbatim
 * // 此场景假定 xTimer 已经创建并已启动。中断发生时，应直接停止该定时器。
 *
 * // 用于停止定时器的中断服务程序。
 * void vAnExampleInterruptServiceRoutine( void )
 * {
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 *
 *     // 中断已发生——直接停止定时器。
 *     // xHigherPriorityTaskWoken 在其定义处（本函数内）已设为 pdFALSE。
 *     // 由于这是中断服务程序，只能使用以 "FromISR" 结尾的 FreeRTOS API 函数。
 *     if( xTimerStopFromISR( xTimer, &xHigherPriorityTaskWoken ) != pdPASS )
 *     {
 *         // 停止命令未成功执行。请在此采取适当措施。
 *     }
 *
 *     // 若 xHigherPriorityTaskWoken 等于 pdTRUE，则应执行上下文切换。
 *     // 在 ISR 内部执行上下文切换所需的语法因移植层和编译器而异。
 *     // 请查看所用移植层的示例，以确定实际所需的语法。
 *     if( xHigherPriorityTaskWoken != pdFALSE )
 *     {
 *         // 在此调用中断安全的让出函数（实际函数取决于所用的 FreeRTOS 移植层）。
 *     }
 * }
 * @endverbatim
 */
#define xTimerStopFromISR( xTimer, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_STOP_FROM_ISR, 0, ( pxHigherPriorityTaskWoken ), 0U )

/**
 * BaseType_t xTimerChangePeriodFromISR( TimerHandle_t xTimer,
 *                                       TickType_t xNewPeriod,
 *                                       BaseType_t *pxHigherPriorityTaskWoken );
 *
 * xTimerChangePeriod() 的中断服务程序版本。
 *
 * @param xTimer 要更改周期的定时器句柄。
 *
 * @param xNewPeriod xTimer 的新周期。定时器周期以 tick 周期为单位指定，因此可
 * 使用常量 portTICK_PERIOD_MS 将以毫秒指定的时间转换为 tick 数。例如，若定时器
 * 必须在 100 个 tick 后到期，则 xNewPeriod 应设为 100。另一种情况，若定时器必须
 * 在 500ms 后到期，则在 configTICK_RATE_HZ 小于或等于 1000 的前提下，可将
 * xNewPeriod 设为 ( 500 / portTICK_PERIOD_MS )。
 *
 * @param pxHigherPriorityTaskWoken 定时器服务任务（守护任务）大部分时间处于阻塞态，
 * 等待定时器命令队列上的消息到达。调用 xTimerChangePeriodFromISR() 会向定时器
 * 命令队列写入一条消息，因此有可能使定时器服务任务（守护任务）离开阻塞态。若
 * 调用 xTimerChangePeriodFromISR() 使定时器服务任务（守护任务）离开阻塞态，且
 * 该定时器服务任务（守护任务）的优先级等于或高于当前正在执行的任务（被中断的
 * 任务），则在 xTimerChangePeriodFromISR() 函数内部会将 *pxHigherPriorityTaskWoken
 * 设为 pdTRUE。若 xTimerChangePeriodFromISR() 将该值设为 pdTRUE，则应在中断退出
 * 之前执行上下文切换。
 *
 * @return 若更改定时器周期的命令无法发送到定时器命令队列，则返回 pdFAIL。若命令
 * 成功发送到定时器命令队列，则返回 pdPASS。命令的实际处理时机取决于定时器服务
 * 任务（守护任务）相对于系统中其他任务的优先级。定时器服务任务（守护任务）的
 * 优先级由配置常量 configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 * @verbatim
 * // 此场景假定 xTimer 已经创建并已启动。中断发生时，应将 xTimer 的周期改为
 * // 500ms。
 *
 * // 用于更改 xTimer 周期的中断服务程序。
 * void vAnExampleInterruptServiceRoutine( void )
 * {
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 *
 *     // 中断已发生——将 xTimer 的周期改为 500ms。
 *     // xHigherPriorityTaskWoken 在其定义处（本函数内）已设为 pdFALSE。
 *     // 由于这是中断服务程序，只能使用以 "FromISR" 结尾的 FreeRTOS API 函数。
 *     if( xTimerChangePeriodFromISR( xTimer, &xHigherPriorityTaskWoken ) != pdPASS )
 *     {
 *         // 更改定时器周期的命令未成功执行。请在此采取适当措施。
 *     }
 *
 *     // 若 xHigherPriorityTaskWoken 等于 pdTRUE，则应执行上下文切换。
 *     // 在 ISR 内部执行上下文切换所需的语法因移植层和编译器而异。
 *     // 请查看所用移植层的示例，以确定实际所需的语法。
 *     if( xHigherPriorityTaskWoken != pdFALSE )
 *     {
 *         // 在此调用中断安全的让出函数（实际函数取决于所用的 FreeRTOS 移植层）。
 *     }
 * }
 * @endverbatim
 */
#define xTimerChangePeriodFromISR( xTimer, xNewPeriod, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_CHANGE_PERIOD_FROM_ISR, ( xNewPeriod ), ( pxHigherPriorityTaskWoken ), 0U )

/**
 * BaseType_t xTimerResetFromISR(   TimerHandle_t xTimer,
 *                                  BaseType_t *pxHigherPriorityTaskWoken );
 *
 * xTimerReset() 的中断服务程序版本。
 *
 * @param xTimer 要启动、复位或重新启动的定时器句柄。
 *
 * @param pxHigherPriorityTaskWoken 定时器服务任务（守护任务）大部分时间处于阻塞态，
 * 等待定时器命令队列上的消息到达。调用 xTimerResetFromISR() 会向定时器命令队列
 * 写入一条消息，因此有可能使定时器服务任务（守护任务）离开阻塞态。若调用
 * xTimerResetFromISR() 使定时器服务任务（守护任务）离开阻塞态，且该定时器服务
 * 任务（守护任务）的优先级等于或高于当前正在执行的任务（被中断的任务），则在
 * xTimerResetFromISR() 函数内部会将 *pxHigherPriorityTaskWoken 设为 pdTRUE。
 * 若 xTimerResetFromISR() 将该值设为 pdTRUE，则应在中断退出之前执行上下文切换。
 *
 * @return 若复位命令无法发送到定时器命令队列，则返回 pdFAIL。若命令成功发送到
 * 定时器命令队列，则返回 pdPASS。命令的实际处理时机取决于定时器服务任务
 * （守护任务）相对于系统中其他任务的优先级，但定时器的到期时间相对于实际调用
 * xTimerResetFromISR() 的时刻。定时器服务任务（守护任务）的优先级由配置常量
 * configTIMER_TASK_PRIORITY 设置。
 *
 * 用法示例：
 * @verbatim
 * // 此场景假定 xBacklightTimer 已经创建。按下按键时点亮 LCD 背光。若 5 秒内
 * // 没有按键按下，则关闭 LCD 背光。在此场景中，定时器为单次定时器；与
 * // xTimerReset() 函数的示例不同，按键事件处理函数是中断服务程序。
 *
 * // 分配给该单次定时器的回调函数。在此场景中不使用该参数。
 * void vBacklightTimerCallback( TimerHandle_t pxTimer )
 * {
 *     // 定时器已到期，因此距上次按键必定已过去 5 秒。关闭 LCD 背光。
 *     vSetBacklightState( BACKLIGHT_OFF );
 * }
 *
 * // 按键中断服务程序。
 * void vKeyPressEventInterruptHandler( void )
 * {
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 *
 *     // 确保 LCD 背光已点亮，然后复位负责在按键无操作 5 秒后关闭背光的
 *     // 定时器。这是中断服务程序，因此只能调用以 "FromISR" 结尾的
 *     // FreeRTOS API 函数。
 *     vSetBacklightState( BACKLIGHT_ON );
 *
 *     // 此处可调用 xTimerStartFromISR() 或 xTimerResetFromISR()，
 *     // 因为两者都会使定时器重新计算其到期时间。
 *     // xHigherPriorityTaskWoken 在声明时（在本函数中）已初始化为 pdFALSE。
 *     if( xTimerResetFromISR( xBacklightTimer, &xHigherPriorityTaskWoken ) != pdPASS )
 *     {
 *         // 复位命令未成功执行。请在此采取适当措施。
 *     }
 *
 *     // 在此执行按键的其余处理。
 *
 *     // 若 xHigherPriorityTaskWoken 等于 pdTRUE，则应执行上下文切换。
 *     // 在 ISR 内部执行上下文切换所需的语法因移植层和编译器而异。
 *     // 请查看所用移植层的示例，以确定实际所需的语法。
 *     if( xHigherPriorityTaskWoken != pdFALSE )
 *     {
 *         // 在此调用中断安全的让出函数（实际函数取决于所用的 FreeRTOS 移植层）。
 *     }
 * }
 * @endverbatim
 */
#define xTimerResetFromISR( xTimer, pxHigherPriorityTaskWoken ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_RESET_FROM_ISR, ( xTaskGetTickCountFromISR() ), ( pxHigherPriorityTaskWoken ), 0U )


/**
 * BaseType_t xTimerPendFunctionCallFromISR( PendedFunction_t xFunctionToPend,
 *                                          void *pvParameter1,
 *                                          uint32_t ulParameter2,
 *                                          BaseType_t *pxHigherPriorityTaskWoken );
 *
 *
 * 由应用中断服务程序调用，用于将函数的执行推迟到 RTOS 守护任务（即定时器服务
 * 任务，因此该函数在 timers.c 中实现并以 'Timer' 为前缀）。
 *
 * 理想情况下，中断服务程序（ISR）应尽可能短小，但有时 ISR 要么需要处理大量工作，
 * 要么需要执行非确定性的处理。在这些情况下，可使用
 * xTimerPendFunctionCallFromISR() 将函数的处理推迟到 RTOS 守护任务。
 *
 * 该机制允许中断直接返回到随后将执行挂起回调函数的任务。这样，回调函数可以在
 * 时间上与中断连续执行——就如同回调函数本身在中断中执行一样。
 *
 * @param xFunctionToPend 要从定时器服务任务（守护任务）执行的函数。该函数必须
 * 符合 PendedFunction_t 原型。
 *
 * @param pvParameter1 回调函数第一个参数的值。该参数为 void * 类型，以便用于
 * 传递任意类型。例如，可将 unsigned long 转换为 void *，也可让 void * 指向
 * 一个结构体。
 *
 * @param ulParameter2 回调函数第二个参数的值。
 *
 * @param pxHigherPriorityTaskWoken 如上所述，调用此函数会向定时器守护任务发送
 * 一条消息。若定时器守护任务的优先级（在 FreeRTOSConfig.h 中通过
 * configTIMER_TASK_PRIORITY 设置）高于当前正在运行的任务（被中断的任务）的
 * 优先级，则在 xTimerPendFunctionCallFromISR() 内部会将 *pxHigherPriorityTaskWoken
 * 设为 pdTRUE，表示应在中断退出之前请求上下文切换。因此，*pxHigherPriorityTaskWoken
 * 必须初始化为 pdFALSE。参见下面的示例代码。
 *
 * @return 若消息成功发送到定时器守护任务，则返回 pdPASS；否则返回 pdFALSE。
 *
 * 用法示例：
 * @verbatim
 *
 *  // 将在守护任务上下文中执行的回调函数。
 *  // 注意，所有回调函数都必须使用此相同的原型。
 *  void vProcessInterface( void *pvParameter1, uint32_t ulParameter2 )
 *  {
 *      BaseType_t xInterfaceToService;
 *
 *      // 需要处理的接口通过第二个参数传入。此场景中不使用第一个参数。
 *      xInterfaceToService = ( BaseType_t ) ulParameter2;
 *
 *      // ...在此执行处理...
 *  }
 *
 *  // 从多个接口接收数据包的中断服务程序
 *  void vAnISR( void )
 *  {
 *      BaseType_t xInterfaceToService, xHigherPriorityTaskWoken;
 *
 *      // 查询硬件，确定需要处理哪个接口。
 *      xInterfaceToService = prvCheckInterfaces();
 *
 *      // 实际处理将推迟到任务中执行。请求执行 vProcessInterface() 回调函数，
 *      // 并传入需要处理的接口编号。需要处理的接口通过第二个参数传入。
 *      // 此场景中不使用第一个参数。
 *      xHigherPriorityTaskWoken = pdFALSE;
 *      xTimerPendFunctionCallFromISR( vProcessInterface, NULL, ( uint32_t ) xInterfaceToService, &xHigherPriorityTaskWoken );
 *
 *      // 若 xHigherPriorityTaskWoken 现在被设为 pdTRUE，则应请求上下文切换。
 *      // 所用宏与移植层相关，为 portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()
 *      // 之一——请参阅所用移植层的文档页面。
 *      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *
 *  }
 * @endverbatim
 */
#if ( INCLUDE_xTimerPendFunctionCall == 1 )
    BaseType_t xTimerPendFunctionCallFromISR( PendedFunction_t xFunctionToPend,
                                              void * pvParameter1,
                                              uint32_t ulParameter2,
                                              BaseType_t * pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
#endif

/**
 * BaseType_t xTimerPendFunctionCall( PendedFunction_t xFunctionToPend,
 *                                    void *pvParameter1,
 *                                    uint32_t ulParameter2,
 *                                    TickType_t xTicksToWait );
 *
 *
 * 用于将函数的执行推迟到 RTOS 守护任务（即定时器服务任务，因此该函数在 timers.c
 * 中实现并以 'Timer' 为前缀）。
 *
 * @param xFunctionToPend 要从定时器服务任务（守护任务）执行的函数。该函数必须
 * 符合 PendedFunction_t 原型。
 *
 * @param pvParameter1 回调函数第一个参数的值。该参数为 void * 类型，以便用于
 * 传递任意类型。例如，可将 unsigned long 转换为 void *，也可让 void * 指向
 * 一个结构体。
 *
 * @param ulParameter2 回调函数第二个参数的值。
 *
 * @param xTicksToWait 调用此函数会通过队列向定时器守护任务发送一条消息。若发现
 * 定时器队列已满，则 xTicksToWait 是调用任务为等待该队列出现可用空间而应保持在
 * 阻塞态的时间（因此不消耗任何处理时间）。
 *
 * @return 若消息成功发送到定时器守护任务，则返回 pdPASS；否则返回 pdFALSE。
 */
#if ( INCLUDE_xTimerPendFunctionCall == 1 )
    BaseType_t xTimerPendFunctionCall( PendedFunction_t xFunctionToPend,
                                       void * pvParameter1,
                                       uint32_t ulParameter2,
                                       TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
#endif

/**
 * const char * const pcTimerGetName( TimerHandle_t xTimer );
 *
 * 返回创建定时器时为该定时器分配的名称。
 *
 * @param xTimer 被查询的定时器句柄。
 *
 * @return 分配给 xTimer 参数所指定定时器的名称。
 */
const char * pcTimerGetName( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * void vTimerSetReloadMode( TimerHandle_t xTimer, const BaseType_t xAutoReload );
 *
 * 将定时器更新为自动重载定时器或单次定时器：自动重载定时器每次到期后会自动
 * 复位；单次定时器只到期一次，除非手动重新启动。
 *
 * @param xTimer 被更新的定时器句柄。
 *
 * @param xAutoReload 若 xAutoReload 设为 pdTRUE，则定时器将按定时器周期设定的
 * 频率反复到期（参见 xTimerCreate() API 函数的 xTimerPeriodInTicks 参数）。若
 * xAutoReload 设为 pdFALSE，则定时器为单次定时器，到期后进入休眠状态。
 */
void vTimerSetReloadMode( TimerHandle_t xTimer,
                          const BaseType_t xAutoReload ) PRIVILEGED_FUNCTION;

/**
 * BaseType_t xTimerGetReloadMode( TimerHandle_t xTimer );
 *
 * 查询定时器，以确定它是自动重载定时器还是单次定时器：自动重载定时器每次到期后
 * 会自动复位；单次定时器只到期一次，除非手动重新启动。
 *
 * @param xTimer 被查询的定时器句柄。
 *
 * @return 若定时器为自动重载定时器，则返回 pdTRUE；否则返回 pdFALSE。
 */
BaseType_t xTimerGetReloadMode( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * UBaseType_t uxTimerGetReloadMode( TimerHandle_t xTimer );
 *
 * 查询定时器，以确定它是自动重载定时器还是单次定时器：自动重载定时器每次到期后
 * 会自动复位；单次定时器只到期一次，除非手动重新启动。
 *
 * @param xTimer 被查询的定时器句柄。
 *
 * @return 若定时器为自动重载定时器，则返回 pdTRUE；否则返回 pdFALSE。
 */
UBaseType_t uxTimerGetReloadMode( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * TickType_t xTimerGetPeriod( TimerHandle_t xTimer );
 *
 * 返回定时器的周期。
 *
 * @param xTimer 被查询的定时器句柄。
 *
 * @return 定时器的周期，以 tick 为单位。
 */
TickType_t xTimerGetPeriod( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * TickType_t xTimerGetExpiryTime( TimerHandle_t xTimer );
 *
 * 返回定时器将要到期的时刻（以 tick 为单位）。若该值小于当前 tick 计数，
 * 则说明到期时间相对当前时间已发生溢出。
 *
 * @param xTimer 被查询的定时器句柄。
 *
 * @return 若定时器正在运行，则返回定时器下次到期的时刻（以 tick 为单位）。
 * 若定时器未运行，则返回值未定义。
 */
TickType_t xTimerGetExpiryTime( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

/**
 * BaseType_t xTimerGetStaticBuffer( TimerHandle_t xTimer,
 *                                   StaticTimer_t ** ppxTimerBuffer );
 *
 * 获取指向静态创建定时器数据结构缓冲区的指针。该缓冲区与创建定时器时提供的
 * 缓冲区相同。
 *
 * @param xTimer 要获取其缓冲区的定时器。
 *
 * @param ppxTaskBuffer 用于返回指向定时器数据结构缓冲区的指针。
 *
 * @return 若成功获取缓冲区，则返回 pdTRUE；否则返回 pdFALSE。
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xTimerGetStaticBuffer( TimerHandle_t xTimer,
                                      StaticTimer_t ** ppxTimerBuffer ) PRIVILEGED_FUNCTION;
#endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */

/*
 * 本部分之后的函数不属于公共 API，仅供内核内部使用。
 */
BaseType_t xTimerCreateTimerTask( void ) PRIVILEGED_FUNCTION;

/*
 * 将 xTimerGenericCommand 拆分为两个子函数并改为宏，可消除从 ISR 调用时的
 * 递归路径。这主要是针对 XCore XCC 移植层：当未拆分时，该移植层会检测到
 * 递归路径并在编译期间报错。
 */
BaseType_t xTimerGenericCommandFromTask( TimerHandle_t xTimer,
                                         const BaseType_t xCommandID,
                                         const TickType_t xOptionalValue,
                                         BaseType_t * const pxHigherPriorityTaskWoken,
                                         const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

BaseType_t xTimerGenericCommandFromISR( TimerHandle_t xTimer,
                                        const BaseType_t xCommandID,
                                        const TickType_t xOptionalValue,
                                        BaseType_t * const pxHigherPriorityTaskWoken,
                                        const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

#define xTimerGenericCommand( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait )         \
    ( ( xCommandID ) < tmrFIRST_FROM_ISR_COMMAND ?                                                                  \
      xTimerGenericCommandFromTask( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait ) : \
      xTimerGenericCommandFromISR( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait ) )
#if ( configUSE_TRACE_FACILITY == 1 )
    void vTimerSetTimerNumber( TimerHandle_t xTimer,
                               UBaseType_t uxTimerNumber ) PRIVILEGED_FUNCTION;
    UBaseType_t uxTimerGetTimerNumber( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;
#endif

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

/**
 * task.h
 * @code{c}
 * void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer, StackType_t ** ppxTimerTaskStackBuffer, configSTACK_DEPTH_TYPE * puxTimerTaskStackSize )
 * @endcode
 *
 * 此函数用于向 FreeRTOS 提供一块静态分配的内存，以存放定时器任务的 TCB。当
 * configSUPPORT_STATIC_ALLOCATION 被设置时需要此函数。更多信息参见 URI：
 * https://www.FreeRTOS.org/a00110.html#configSUPPORT_STATIC_ALLOCATION
 *
 * @param ppxTimerTaskTCBBuffer   指向静态分配的 TCB 缓冲区的句柄
 * @param ppxTimerTaskStackBuffer 指向为定时器任务静态分配的栈缓冲区的句柄
 * @param puxTimerTaskStackSize   指向所分配栈缓冲区可容纳元素数量的指针
 */
    void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                         StackType_t ** ppxTimerTaskStackBuffer,
                                         configSTACK_DEPTH_TYPE * puxTimerTaskStackSize );

#endif

#if ( configUSE_DAEMON_TASK_STARTUP_HOOK != 0 )

/**
 *  timers.h
 * @code{c}
 * void vApplicationDaemonTaskStartupHook( void );
 * @endcode
 *
 * 该钩子函数在定时器任务开始运行时由定时器任务调用一次。
 */
    /* MISRA 参考 8.6.1 [外部链接] */
    /* 更多细节参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-86 */
    /* coverity[misra_c_2012_rule_8_6_violation] —— MISRA C:2012 规则 8.6 违规抑制 */
    void vApplicationDaemonTaskStartupHook( void );

#endif

/*
 * 此函数重置定时器模块的内部状态。应用必须在重新启动调度器之前调用它。
 */
void vTimerResetState( void ) PRIVILEGED_FUNCTION;

/* *INDENT-OFF* —— 暂停自动缩进（uncrustify 指令） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* —— 恢复自动缩进（uncrustify 指令） */
#endif /* 头文件保护宏 TIMERS_H */
