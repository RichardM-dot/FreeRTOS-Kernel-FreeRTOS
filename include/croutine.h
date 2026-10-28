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

#ifndef CO_ROUTINE_H
#define CO_ROUTINE_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include croutine.h"
#endif

#include "list.h"

/* *INDENT-OFF* 关闭自动缩进 */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* 恢复自动缩进 */

/*
 * 用于隐藏协程控制块的实现。
 * 不过，由于协程功能采用宏实现，
 * 控制块结构仍必须包含在头文件中。
 */
typedef void * CoRoutineHandle_t;

/* 定义协程函数必须遵循的函数原型。 */
typedef void (* crCOROUTINE_CODE)( CoRoutineHandle_t xHandle,
                                   UBaseType_t uxIndex );

typedef struct corCoRoutineControlBlock
{
    crCOROUTINE_CODE pxCoRoutineFunction;
    ListItem_t xGenericListItem; /**< 用于将 CRCB 放入就绪队列和阻塞队列的链表项。 */
    ListItem_t xEventListItem;   /**< 用于将 CRCB 放入事件链表的链表项。 */
    UBaseType_t uxPriority;      /**< 协程相对于其他协程的优先级。 */
    UBaseType_t uxIndex;         /**< 当多个协程使用同一协程函数时，用于区分这些协程。 */
    uint16_t uxState;            /**< 由协程实现内部使用。 */
} CRCB_t;                        /* 协程控制块。注意：其大小直到 uxPriority 为止必须与 TCB_t 完全一致。 */

/**
 * croutine. h
 * @code{c}
 * BaseType_t xCoRoutineCreate(
 *                               crCOROUTINE_CODE pxCoRoutineCode,
 *                               UBaseType_t uxPriority,
 *                               UBaseType_t uxIndex
 *                             );
 * @endcode
 *
 * 创建一个新的协程，并将其加入已就绪可运行的协程链表。
 *
 * @param pxCoRoutineCode 指向协程函数的指针。协程函数
 * 需要使用特殊语法——更多信息请参见 WEB 文档中的协程章节。
 *
 * @param uxPriority 该协程相对其他协程运行时的优先级。
 *
 * @param uxIndex 用于区分执行同一函数的不同协程。
 * 详情请参见下面的示例以及 WEB 文档中的协程章节。
 *
 * @return 若协程创建成功并加入就绪链表，则返回 pdPASS，
 * 否则返回 ProjDefs.h 中定义的错误码。
 *
 * 示例用法：
 * @code{c}
 * // 待创建的协程。
 * void vFlashCoRoutine( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 若协程变量需要在阻塞调用前后保持取值，则必须声明为 static。
 * // 对于 const 变量，可能无需这样做。
 * static const char cLedToFlash[ 2 ] = { 5, 6 };
 * static const TickType_t uxFlashRates[ 2 ] = { 200, 400 };
 *
 *   // 每个协程都必须以调用 crSTART() 开始；
 *   crSTART( xHandle );
 *
 *   for( ;; )
 *   {
 *       // 该协程仅延时固定时间，然后翻转
 *       // 一个 LED。使用此函数创建两个协程，因此
 *       // 通过 uxIndex 参数告诉协程要闪烁哪个
 *       // LED 以及延时多长时间。这里假设 xQueue 已
 *       // 创建。
 *       vParTestToggleLED( cLedToFlash[ uxIndex ] );
 *       crDELAY( xHandle, uxFlashRates[ uxIndex ] );
 *   }
 *
 *   // 每个协程都必须以调用 crEND() 结束；
 *   crEND();
 * }
 *
 * // 创建两个协程的函数。
 * void vOtherFunction( void )
 * {
 * uint8_t ucParameterToPass;
 * TaskHandle_t xHandle;
 *
 *   // 创建两个优先级为 0 的协程。第一个协程使用索引 0，
 *   // 因此（根据上面的代码）每 200 个时钟节拍翻转一次 LED 5。第二个
 *   // 协程使用索引 1，因此每 400 个时钟节拍翻转一次 LED 6。
 *   for( uxIndex = 0; uxIndex < 2; uxIndex++ )
 *   {
 *       xCoRoutineCreate( vFlashCoRoutine, 0, uxIndex );
 *   }
 * }
 * @endcode
 * \defgroup xCoRoutineCreate xCoRoutineCreate
 * \ingroup Tasks
 */
BaseType_t xCoRoutineCreate( crCOROUTINE_CODE pxCoRoutineCode,
                             UBaseType_t uxPriority,
                             UBaseType_t uxIndex );


/**
 * croutine. h
 * @code{c}
 * void vCoRoutineSchedule( void );
 * @endcode
 *
 * 运行协程。
 *
 * vCoRoutineSchedule() 执行能够运行的最高优先级协程。该协程会一直执行，
 * 直到它阻塞、让出 CPU 或被任务抢占。协程以协作方式执行，因此一个
 * 协程不能被另一个协程抢占，但可以被任务抢占。
 *
 * 若应用程序同时包含任务和协程，则
 * 应由空闲任务（在空闲任务钩子函数中）调用 vCoRoutineSchedule。
 *
 * 示例用法：
 * @code{c}
 * // 该空闲任务钩子函数每次被调用时都会调度一个协程。
 * // 空闲任务的其余部分将在两次协程调用之间执行。
 * void vApplicationIdleHook( void )
 * {
 *  vCoRoutineSchedule();
 * }
 *
 * // 或者，如果不需要空闲任务的其他部分执行，
 * // 则空闲任务钩子函数可以在无限循环中调用
 * // vCoRoutineSchedule()。
 * void vApplicationIdleHook( void )
 * {
 *  for( ;; )
 *  {
 *      vCoRoutineSchedule();
 *  }
 * }
 * @endcode
 * \defgroup vCoRoutineSchedule vCoRoutineSchedule
 * \ingroup Tasks
 */
void vCoRoutineSchedule( void );

/**
 * croutine. h
 * @code{c}
 * crSTART( CoRoutineHandle_t xHandle );
 * @endcode
 *
 * 此宏必须始终在协程函数的开头调用。
 *
 * 示例用法：
 * @code{c}
 * // 待创建的协程。
 * void vACoRoutine( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 若协程变量需要在阻塞调用前后保持取值，则必须声明为 static。
 * static int32_t ulAVariable;
 *
 *   // 每个协程都必须以调用 crSTART() 开始；
 *   crSTART( xHandle );
 *
 *   for( ;; )
 *   {
 *        // 协程功能代码放在此处。
 *   }
 *
 *   // 每个协程都必须以调用 crEND() 结束；
 *   crEND();
 * }
 * @endcode
 * \defgroup crSTART crSTART
 * \ingroup Tasks
 */
#define crSTART( pxCRCB )                            \
    switch( ( ( CRCB_t * ) ( pxCRCB ) )->uxState ) { \
        case 0:

/**
 * croutine. h
 * @code{c}
 * crEND();
 * @endcode
 *
 * 此宏必须始终在协程函数的末尾调用。
 *
 * 示例用法：
 * @code{c}
 * // 待创建的协程。
 * void vACoRoutine( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 若协程变量需要在阻塞调用前后保持取值，则必须声明为 static。
 * static int32_t ulAVariable;
 *
 *   // 每个协程都必须以调用 crSTART() 开始；
 *   crSTART( xHandle );
 *
 *   for( ;; )
 *   {
 *        // 协程功能代码放在此处。
 *   }
 *
 *   // 每个协程都必须以调用 crEND() 结束；
 *   crEND();
 * }
 * @endcode
 * \defgroup crSTART crSTART
 * \ingroup Tasks
 */

/* *INDENT-OFF* 关闭自动缩进 */
#define crEND()    }
/* *INDENT-ON* 恢复自动缩进 */

/*
 * 这些宏仅供协程实现内部使用。
 * 应用程序编写者不应直接使用这些宏。
 */
#define crSET_STATE0( xHandle )                                       \
    ( ( CRCB_t * ) ( xHandle ) )->uxState = ( __LINE__ * 2 ); return; \
    case ( __LINE__ * 2 ):
#define crSET_STATE1( xHandle )                                               \
    ( ( CRCB_t * ) ( xHandle ) )->uxState = ( ( __LINE__ * 2 ) + 1 ); return; \
    case ( ( __LINE__ * 2 ) + 1 ):

/**
 * croutine. h
 * @code{c}
 * crDELAY( CoRoutineHandle_t xHandle, TickType_t xTicksToDelay );
 * @endcode
 *
 * 将协程延时固定的一段时间。
 *
 * crDELAY 只能从协程函数本身调用——不能
 * 从协程函数所调用的函数内部调用。这是因为
 * 协程不维护自己的栈。
 *
 * @param xHandle 要延时的协程的句柄。这是协程函数的
 * xHandle 参数。
 *
 * @param xTickToDelay 协程应延时的时钟节拍数。
 * 该值对应的实际时间由 configTICK_RATE_HZ（在 FreeRTOSConfig.h 中设置）
 * 定义。可使用常量 portTICK_PERIOD_MS 将时钟节拍转换为毫秒。
 *
 * 示例用法：
 * @code{c}
 * // 待创建的协程。
 * void vACoRoutine( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 若协程变量需要在阻塞调用前后保持取值，则必须声明为 static。
 * // 对于 const 变量，可能无需这样做。
 * // 需要延时 200ms。
 * static const xTickType xDelayTime = 200 / portTICK_PERIOD_MS;
 *
 *   // 每个协程都必须以调用 crSTART() 开始；
 *   crSTART( xHandle );
 *
 *   for( ;; )
 *   {
 *      // 延时 200ms。
 *      crDELAY( xHandle, xDelayTime );
 *
 *      // 在此处执行某些操作。
 *   }
 *
 *   // 每个协程都必须以调用 crEND() 结束；
 *   crEND();
 * }
 * @endcode
 * \defgroup crDELAY crDELAY
 * \ingroup Tasks
 */
#define crDELAY( xHandle, xTicksToDelay )                          \
    do {                                                           \
        if( ( xTicksToDelay ) > 0 )                                \
        {                                                          \
            vCoRoutineAddToDelayedList( ( xTicksToDelay ), NULL ); \
        }                                                          \
        crSET_STATE0( ( xHandle ) );                               \
    } while( 0 )

/**
 * @code{c}
 * crQUEUE_SEND(
 *                CoRoutineHandle_t xHandle,
 *                QueueHandle_t pxQueue,
 *                void *pvItemToQueue,
 *                TickType_t xTicksToWait,
 *                BaseType_t *pxResult
 *           )
 * @endcode
 *
 * 宏 crQUEUE_SEND() 和 crQUEUE_RECEIVE() 相当于任务所使用的
 * xQueueSend() 和 xQueueReceive() 函数的协程版本。
 *
 * crQUEUE_SEND 和 crQUEUE_RECEIVE 只能从协程中使用，而
 * xQueueSend() 和 xQueueReceive() 只能从任务中使用。
 *
 * crQUEUE_SEND 只能从协程函数本身调用——不能
 * 从协程函数所调用的函数内部调用。这是因为
 * 协程不维护自己的栈。
 *
 * 关于在任务与协程之间以及中断服务程序（ISR）与协程之间传递数据的信息，
 * 请参见 WEB 文档中的协程章节。
 *
 * @param xHandle 调用协程的句柄。这是协程函数的 xHandle
 * 参数。
 *
 * @param pxQueue 数据将要投递到的队列句柄。
 * 该句柄是在使用 xQueueCreate() API 函数创建队列时作为返回值获得的。
 *
 * @param pvItemToQueue 指向要投递到队列的数据的指针。
 * 每个入队项包含的字节数在创建队列时指定。系统会从 pvItemToQueue
 * 复制这么多个字节到队列本身。
 *
 * @param xTickToDelay 若队列空间不能立即可用，协程为等待队列空间
 * 变为可用而应阻塞的时钟节拍数。该值对应的实际时间由
 * configTICK_RATE_HZ（在 FreeRTOSConfig.h 中设置）定义。可使用常量
 * portTICK_PERIOD_MS 将时钟节拍转换为毫秒（参见下面的示例）。
 *
 * @param pxResult 若数据成功投递到队列，则 pxResult 所指向的变量将被设为 pdPASS；
 * 否则将被设为 ProjDefs.h 中定义的错误码。
 *
 * 示例用法：
 * @code{c}
 * // 协程函数：阻塞固定时间，然后向队列投递一个数字。
 * static void prvCoRoutineFlashTask( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 若协程变量需要在阻塞调用前后保持取值，则必须声明为 static。
 * static BaseType_t xNumberToPost = 0;
 * static BaseType_t xResult;
 *
 *  // 协程必须以调用 crSTART() 开始。
 *  crSTART( xHandle );
 *
 *  for( ;; )
 *  {
 *      // 这里假设队列已经创建。
 *      crQUEUE_SEND( xHandle, xCoRoutineQueue, &xNumberToPost, NO_DELAY, &xResult );
 *
 *      if( xResult != pdPASS )
 *      {
 *          // 消息未投递成功！
 *      }
 *
 *      // 递增要投递到队列的数字。
 *      xNumberToPost++;
 *
 *      // 延时 100 个时钟节拍。
 *      crDELAY( xHandle, 100 );
 *  }
 *
 *  // 协程必须以调用 crEND() 结束。
 *  crEND();
 * }
 * @endcode
 * \defgroup crQUEUE_SEND crQUEUE_SEND
 * \ingroup Tasks
 */
#define crQUEUE_SEND( xHandle, pxQueue, pvItemToQueue, xTicksToWait, pxResult )           \
    do {                                                                                  \
        *( pxResult ) = xQueueCRSend( ( pxQueue ), ( pvItemToQueue ), ( xTicksToWait ) ); \
        if( *( pxResult ) == errQUEUE_BLOCKED )                                           \
        {                                                                                 \
            crSET_STATE0( ( xHandle ) );                                                  \
            *pxResult = xQueueCRSend( ( pxQueue ), ( pvItemToQueue ), 0 );                \
        }                                                                                 \
        if( *pxResult == errQUEUE_YIELD )                                                 \
        {                                                                                 \
            crSET_STATE1( ( xHandle ) );                                                  \
            *pxResult = pdPASS;                                                           \
        }                                                                                 \
    } while( 0 )

/**
 * croutine. h
 * @code{c}
 * crQUEUE_RECEIVE(
 *                   CoRoutineHandle_t xHandle,
 *                   QueueHandle_t pxQueue,
 *                   void *pvBuffer,
 *                   TickType_t xTicksToWait,
 *                   BaseType_t *pxResult
 *               )
 * @endcode
 *
 * 宏 crQUEUE_SEND() 和 crQUEUE_RECEIVE() 相当于任务所使用的
 * xQueueSend() 和 xQueueReceive() 函数的协程版本。
 *
 * crQUEUE_SEND 和 crQUEUE_RECEIVE 只能从协程中使用，而
 * xQueueSend() 和 xQueueReceive() 只能从任务中使用。
 *
 * crQUEUE_RECEIVE 只能从协程函数本身调用——不能
 * 从协程函数所调用的函数内部调用。这是因为
 * 协程不维护自己的栈。
 *
 * 关于在任务与协程之间以及 ISR 与协程之间传递数据的信息，
 * 请参见 WEB 文档中的协程章节。
 *
 * @param xHandle 调用协程的句柄。这是协程函数的 xHandle
 * 参数。
 *
 * @param pxQueue 数据将从中接收的队列句柄。
 * 该句柄是在使用 xQueueCreate() API 函数创建队列时作为返回值获得的。
 *
 * @param pvBuffer 接收到的数据项要复制到的缓冲区。
 * 每个入队项包含的字节数在创建队列时指定。系统会复制
 * 这么多个字节到 pvBuffer。
 *
 * @param xTickToDelay 若数据不能立即可用，协程为等待队列中的数据
 * 变为可用而应阻塞的时钟节拍数。该值对应的实际时间由
 * configTICK_RATE_HZ（在 FreeRTOSConfig.h 中设置）定义。可使用常量
 * portTICK_PERIOD_MS 将时钟节拍转换为毫秒（参见
 * crQUEUE_SEND 示例）。
 *
 * @param pxResult 若成功从队列获取数据，则 pxResult 所指向的变量将被设为 pdPASS；
 * 否则将被设为 ProjDefs.h 中定义的错误码。
 *
 * 示例用法：
 * @code{c}
 * // 协程从队列接收要闪烁的 LED 编号。它在队列上阻塞，
 * // 直到接收到该编号。
 * static void prvCoRoutineFlashWorkTask( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 若协程变量需要在阻塞调用前后保持取值，则必须声明为 static。
 * static BaseType_t xResult;
 * static UBaseType_t uxLEDToFlash;
 *
 *  // 所有协程都必须以调用 crSTART() 开始。
 *  crSTART( xHandle );
 *
 *  for( ;; )
 *  {
 *      // 等待队列中的数据变为可用。
 *      crQUEUE_RECEIVE( xHandle, xCoRoutineQueue, &uxLEDToFlash, portMAX_DELAY, &xResult );
 *
 *      if( xResult == pdPASS )
 *      {
 *          // 已收到要闪烁的 LED 编号——闪烁它！
 *          vParTestToggleLED( uxLEDToFlash );
 *      }
 *  }
 *
 *  crEND();
 * }
 * @endcode
 * \defgroup crQUEUE_RECEIVE crQUEUE_RECEIVE
 * \ingroup Tasks
 */
#define crQUEUE_RECEIVE( xHandle, pxQueue, pvBuffer, xTicksToWait, pxResult )           \
    do {                                                                                \
        *( pxResult ) = xQueueCRReceive( ( pxQueue ), ( pvBuffer ), ( xTicksToWait ) ); \
        if( *( pxResult ) == errQUEUE_BLOCKED )                                         \
        {                                                                               \
            crSET_STATE0( ( xHandle ) );                                                \
            *( pxResult ) = xQueueCRReceive( ( pxQueue ), ( pvBuffer ), 0 );            \
        }                                                                               \
        if( *( pxResult ) == errQUEUE_YIELD )                                           \
        {                                                                               \
            crSET_STATE1( ( xHandle ) );                                                \
            *( pxResult ) = pdPASS;                                                     \
        }                                                                               \
    } while( 0 )

/**
 * croutine. h
 * @code{c}
 * crQUEUE_SEND_FROM_ISR(
 *                          QueueHandle_t pxQueue,
 *                          void *pvItemToQueue,
 *                          BaseType_t xCoRoutinePreviouslyWoken
 *                     )
 * @endcode
 *
 * 宏 crQUEUE_SEND_FROM_ISR() 和 crQUEUE_RECEIVE_FROM_ISR() 相当于任务所使用的
 * xQueueSendFromISR() 和 xQueueReceiveFromISR() 函数的协程版本。
 *
 * crQUEUE_SEND_FROM_ISR() 和 crQUEUE_RECEIVE_FROM_ISR() 只能用于
 * 在协程与 ISR 之间传递数据，而 xQueueSendFromISR() 和
 * xQueueReceiveFromISR() 只能用于在任务与 ISR 之间传递数据。
 *
 * crQUEUE_SEND_FROM_ISR 只能从 ISR 调用，用于向协程内使用的队列
 * 发送数据。
 *
 * 关于在任务与协程之间以及 ISR 与协程之间传递数据的信息，
 * 请参见 WEB 文档中的协程章节。
 *
 * @param xQueue 数据项将要投递到的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的数据项的指针。
 * 队列可容纳数据项的大小在创建队列时已经定义，因此会从 pvItemToQueue
 * 复制这么多字节到队列存储区。
 *
 * @param xCoRoutinePreviouslyWoken 包含此参数是为了让 ISR 能够在一次中断中
 * 多次向同一队列投递数据。第一次调用应始终传入 pdFALSE。
 * 后续调用应传入上一次调用返回的值。
 *
 * @return 若投递到队列的操作唤醒了某个协程，则返回 pdTRUE。ISR 使用该返回值
 * 判断在退出 ISR 后是否可能需要进行上下文切换。
 *
 * 示例用法：
 * @code{c}
 * // 一个在队列上阻塞、等待接收字符的协程。
 * static void vReceivingCoRoutine( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * char cRxedChar;
 * BaseType_t xResult;
 *
 *   // 所有协程都必须以调用 crSTART() 开始。
 *   crSTART( xHandle );
 *
 *   for( ;; )
 *   {
 *       // 等待队列中的数据变为可用。这里假设
 *       // 队列 xCommsRxQueue 已经创建！
 *       crQUEUE_RECEIVE( xHandle, xCommsRxQueue, &uxLEDToFlash, portMAX_DELAY, &xResult );
 *
 *       // 是否接收到字符？
 *       if( xResult == pdPASS )
 *       {
 *           // 在此处处理该字符。
 *       }
 *   }
 *
 *   // 所有协程都必须以调用 crEND() 结束。
 *   crEND();
 * }
 *
 * // 一个使用队列将串口接收到的字符发送给
 * // 协程的 ISR。
 * void vUART_ISR( void )
 * {
 * char cRxedChar;
 * BaseType_t xCRWokenByPost = pdFALSE;
 *
 *   // 循环读取字符，直到 UART 中没有剩余字符。
 *   while( UART_RX_REG_NOT_EMPTY() )
 *   {
 *       // 从 UART 获取字符。
 *       cRxedChar = UART_RX_REG;
 *
 *       // 将字符投递到队列。xCRWokenByPost 在
 *       // 第一次循环时为 pdFALSE。若投递操作导致某个协程
 *       // 被唤醒（解除阻塞），则 xCRWokenByPost 将被设为 pdTRUE。
 *       // 这样就能确保：即使有多个协程阻塞在队列上，
 *       // 无论向队列投递多少字符，此 ISR 也只唤醒其中一个。
 *       xCRWokenByPost = crQUEUE_SEND_FROM_ISR( xCommsRxQueue, &cRxedChar, xCRWokenByPost );
 *   }
 * }
 * @endcode
 * \defgroup crQUEUE_SEND_FROM_ISR crQUEUE_SEND_FROM_ISR
 * \ingroup Tasks
 */
#define crQUEUE_SEND_FROM_ISR( pxQueue, pvItemToQueue, xCoRoutinePreviouslyWoken ) \
    xQueueCRSendFromISR( ( pxQueue ), ( pvItemToQueue ), ( xCoRoutinePreviouslyWoken ) )


/**
 * croutine. h
 * @code{c}
 * crQUEUE_SEND_FROM_ISR(
 *                          QueueHandle_t pxQueue,
 *                          void *pvBuffer,
 *                          BaseType_t * pxCoRoutineWoken
 *                     )
 * @endcode
 *
 * 宏 crQUEUE_SEND_FROM_ISR() 和 crQUEUE_RECEIVE_FROM_ISR() 相当于任务所使用的
 * xQueueSendFromISR() 和 xQueueReceiveFromISR() 函数的协程版本。
 *
 * crQUEUE_SEND_FROM_ISR() 和 crQUEUE_RECEIVE_FROM_ISR() 只能用于
 * 在协程与 ISR 之间传递数据，而 xQueueSendFromISR() 和
 * xQueueReceiveFromISR() 只能用于在任务与 ISR 之间传递数据。
 *
 * crQUEUE_RECEIVE_FROM_ISR 只能从 ISR 调用，用于从协程内使用的队列
 * （由协程投递数据的队列）接收数据。
 *
 * 关于在任务与协程之间以及 ISR 与协程之间传递数据的信息，
 * 请参见 WEB 文档中的协程章节。
 *
 * @param xQueue 数据项将要投递到的队列句柄。
 *
 * @param pvBuffer 指向接收数据项所要放入的缓冲区的指针。
 * 队列可容纳数据项的大小在创建队列时已经定义，因此会从队列
 * 复制这么多字节到 pvBuffer。
 *
 * @param pxCoRoutineWoken 协程可能阻塞等待队列空间变为可用。
 * 若 crQUEUE_RECEIVE_FROM_ISR 导致这样的协程解除阻塞，
 * *pxCoRoutineWoken 将被设为 pdTRUE；否则 *pxCoRoutineWoken 保持不变。
 *
 * @return 若成功从队列接收到一个数据项则返回 pdTRUE，否则
 * 返回 pdFALSE。
 *
 * 示例用法：
 * @code{c}
 * // 一个先向队列投递字符、然后阻塞固定时间的协程。
 * // 每次字符都会递增。
 * static void vSendingCoRoutine( CoRoutineHandle_t xHandle, UBaseType_t uxIndex )
 * {
 * // 该协程阻塞期间 cChar 需要保持其值，因此
 * // 必须声明为 static。
 * static char cCharToTx = 'a';
 * BaseType_t xResult;
 *
 *   // 所有协程都必须以调用 crSTART() 开始。
 *   crSTART( xHandle );
 *
 *   for( ;; )
 *   {
 *       // 将下一个字符发送到队列。
 *       crQUEUE_SEND( xHandle, xCoRoutineQueue, &cCharToTx, NO_DELAY, &xResult );
 *
 *       if( xResult == pdPASS )
 *       {
 *           // 字符已成功投递到队列。
 *       }
 *       else
 *       {
 *          // 无法将字符投递到队列。
 *       }
 *
 *       // 使能 UART 发送中断，以便在这个
 *       // 假想的 UART 中产生中断。该中断将从队列
 *       // 获取字符并发送。
 *       ENABLE_RX_INTERRUPT();
 *
 *       // 递增到下一个字符，然后阻塞固定时间。
 *       // cCharToTx 在延时期间会保持其值，因为它
 *       // 声明为 static。
 *       cCharToTx++;
 *       if( cCharToTx > 'x' )
 *       {
 *          cCharToTx = 'a';
 *       }
 *       crDELAY( 100 );
 *   }
 *
 *   // 所有协程都必须以调用 crEND() 结束。
 *   crEND();
 * }
 *
 * // 一个使用队列接收字符并通过 UART 发送的 ISR。
 * void vUART_ISR( void )
 * {
 * char cCharToTx;
 * BaseType_t xCRWokenByPost = pdFALSE;
 *
 *   while( UART_TX_REG_EMPTY() )
 *   {
 *       // 队列中是否有等待发送的字符？
 *       // 若某个协程被投递操作唤醒，xCRWokenByPost
 *       // 会自动设为 pdTRUE——从而确保无论
 *       // 该循环执行多少次，都只唤醒一个协程。
 *       if( crQUEUE_RECEIVE_FROM_ISR( pxQueue, &cCharToTx, &xCRWokenByPost ) )
 *       {
 *           SEND_CHARACTER( cCharToTx );
 *       }
 *   }
 * }
 * @endcode
 * \defgroup crQUEUE_RECEIVE_FROM_ISR crQUEUE_RECEIVE_FROM_ISR
 * \ingroup Tasks
 */
#define crQUEUE_RECEIVE_FROM_ISR( pxQueue, pvBuffer, pxCoRoutineWoken ) \
    xQueueCRReceiveFromISR( ( pxQueue ), ( pvBuffer ), ( pxCoRoutineWoken ) )

/*
 * 此函数仅供协程宏内部使用。
 * 由于协程实现采用宏形式，该函数原型必须
 * 出现在此处。应用程序编写者不应使用此函数。
 *
 * 将当前协程从其就绪链表中移除，并放入
 * 相应的延时链表。
 */
void vCoRoutineAddToDelayedList( TickType_t xTicksToDelay,
                                 List_t * pxEventList );

/*
 * 此函数仅供队列实现内部使用。
 * 应用程序编写者不应使用此函数。
 *
 * 从事件链表中移除最高优先级的协程，并将其放入
 * 待处理就绪链表。
 */
BaseType_t xCoRoutineRemoveFromEventList( const List_t * pxEventList );


/*
 * 此函数复位协程模块的内部状态。应用程序必须在
 * 重新启动调度器之前调用它。
 */
void vCoRoutineResetState( void ) PRIVILEGED_FUNCTION;

/* *INDENT-OFF* 关闭自动缩进 */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* 恢复自动缩进 */

#endif /* CO_ROUTINE_H 头文件包含保护 */
