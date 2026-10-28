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
 * 【中文导读】timers.c —— 软件定时器
 * ----------------------------------------------------------------------------
 * 软件定时器不是硬件定时器中断！所有回调都运行在"定时器服务任务"
 * （prvTimerTask）里，命令（启动/停止/改周期）经"定时器命令队列"发给它。
 * 含义：回调不是中断上下文，可以调非 FromISR API，但绝不能在回调里阻塞。
 * 阅读路线：xTimerCreate → xTimerStart(发命令) → prvTimerTask(服务任务) →
 * prvSampleTimeNow/prvInsertTimerInActiveList（按到期时间排的双向链表）。
 * ============================================================================
 */

/* 标准头文件包含。 */
#include <stdlib.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可阻止 task.h 将所有 API 函数
 * 重新定义为使用 MPU 包装器。仅当从应用文件包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 0 )
    #error configUSE_TIMERS must be set to 1 to make the xTimerPendFunctionCall() function available.
#endif

/*
 * MPU 移植层要求上述头文件定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE，
 * 但本文件不能保留该定义，以便生成正确的特权与非特权链接和放置属性。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE


/*
 * 如果应用未配置软件定时器功能，则整个源文件都会被跳过。该 #if 在本文件
 * 最末尾处闭合。若要启用软件定时器功能，请确保在 FreeRTOSConfig.h 中
 * 将 configUSE_TIMERS 设置为 1。
 */
#if ( configUSE_TIMERS == 1 )

/* 杂项定义。 */
    #define tmrNO_DELAY                    ( ( TickType_t ) 0U )
    #define tmrMAX_TIME_BEFORE_OVERFLOW    ( ( TickType_t ) -1 )

/*
 * 分配给定时器服务任务的名称。可通过在 FreeRTOSConfig.h 中定义
 * configTIMER_SERVICE_TASK_NAME 覆盖。
 */
    #ifndef configTIMER_SERVICE_TASK_NAME
        #define configTIMER_SERVICE_TASK_NAME    "Tmr Svc"
    #endif

    #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )

/*
 * SMP 系统上分配给定时器服务任务的内核亲和性。可通过在 FreeRTOSConfig.h 中
 * 定义 configTIMER_SERVICE_TASK_CORE_AFFINITY 覆盖。
 */
        #ifndef configTIMER_SERVICE_TASK_CORE_AFFINITY
            #define configTIMER_SERVICE_TASK_CORE_AFFINITY    tskNO_AFFINITY
        #endif
    #endif /* 多核且启用核亲和性： #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */

/* 定时器结构体 ucStatus 成员使用的位定义。 */
    #define tmrSTATUS_IS_ACTIVE                  ( 0x01U )
    #define tmrSTATUS_IS_STATICALLY_ALLOCATED    ( 0x02U )
    #define tmrSTATUS_IS_AUTORELOAD              ( 0x04U )

/* 定时器结构体自身的定义。 */
    typedef struct tmrTimerControl                                               /* 沿用旧命名约定，以免破坏内核感知调试器。 */
    {
        const char * pcTimerName;                                                /**< 文本名称。内核不使用该字段，仅用于方便调试。 */
        ListItem_t xTimerListItem;                                               /**< 标准链表项，所有内核功能均使用它进行事件管理。 */
        TickType_t xTimerPeriodInTicks;                                          /**< 定时器的到期周期，即多久到期一次。 */
        void * pvTimerID;                                                        /**< 用于标识定时器的 ID。当多个定时器共用同一个回调函数时，可据此区分定时器。 */
        portTIMER_CALLBACK_ATTRIBUTE TimerCallbackFunction_t pxCallbackFunction; /**< 定时器到期时将被调用的函数。 */
        #if ( configUSE_TRACE_FACILITY == 1 )
            UBaseType_t uxTimerNumber;                                           /**< 由 FreeRTOS+Trace 等跟踪工具分配的 ID。 */
        #endif
        uint8_t ucStatus;                                                        /**< 保存标志位，指示定时器是否为静态分配，以及当前是否处于活动状态。 */
    } xTIMER;

/*
 * 上面保留旧的 xTIMER 名称，再通过 typedef 映射到下面的新名称 Timer_t，
 * 以便兼容旧版内核感知调试器。
 */
    typedef xTIMER Timer_t;

/*
 * 可在定时器队列上发送和接收的消息定义。可入队的消息有两类：操作软件定时器的消息，
 * 以及请求执行非定时器相关回调的消息。这两类消息分别定义在两个独立的结构体中：
 * xTimerParametersType 与 xCallbackParametersType。
 */
    typedef struct tmrTimerParameters
    {
        TickType_t xMessageValue; /**< 部分命令使用的可选值，例如修改定时器周期时使用。 */
        Timer_t * pxTimer;        /**< 命令将要作用的定时器。 */
    } TimerParameter_t;


    typedef struct tmrCallbackParameters
    {
        portTIMER_CALLBACK_ATTRIBUTE
        PendedFunction_t pxCallbackFunction; /* << 要执行的回调函数。 */
        void * pvParameter1;                 /* << 作为回调函数第一个参数传入的值。 */
        uint32_t ulParameter2;               /* << 作为回调函数第二个参数传入的值。 */
    } CallbackParameters_t;

/*
 * 包含两种消息类型的结构体，同时含有一个标识符，
 * 用于判断当前哪种消息类型有效。
 */
    typedef struct tmrTimerQueueMessage
    {
        BaseType_t xMessageID; /**< 发送给定时器服务任务的命令。 */
        union
        {
            TimerParameter_t xTimerParameters;

            /*
             * 如果不会用到 xCallbackParameters，就不要包含它，因为这会增大结构体
             * （进而增大定时器队列）的体积。
             */
            #if ( INCLUDE_xTimerPendFunctionCall == 1 )
                CallbackParameters_t xCallbackParameters;
            #endif /* 配置项 INCLUDE_xTimerPendFunctionCall */
        } u;
    } DaemonTaskMessage_t;

/*
 * 存储活动定时器的链表。定时器按到期时间排序，最近到期的定时器位于链表头部。
 * 仅允许定时器服务任务访问这些链表。
 * xActiveTimerList1 和 xActiveTimerList2 原本可以放在函数作用域内，但这样会
 * 破坏某些内核感知调试器，以及依赖移除 static 限定符的调试器。
 */
    PRIVILEGED_DATA static List_t xActiveTimerList1;
    PRIVILEGED_DATA static List_t xActiveTimerList2;
    PRIVILEGED_DATA static List_t * pxCurrentTimerList;
    PRIVILEGED_DATA static List_t * pxOverflowTimerList;

/* 用于向定时器服务任务发送命令的队列。 */
    PRIVILEGED_DATA static QueueHandle_t xTimerQueue = NULL;
    PRIVILEGED_DATA static TaskHandle_t xTimerTaskHandle = NULL;

/*-----------------------------------------------------------*/

/*
 * 如果定时器服务任务所用的基础设施尚未初始化，则对其进行初始化。
 */
    static void prvCheckForValidListAndQueue( void ) PRIVILEGED_FUNCTION;

/*
 * 定时器服务任务（守护任务）。定时器功能由该任务控制。其他任务通过
 * xTimerQueue 队列与定时器服务任务通信。
 */
    static portTASK_FUNCTION_PROTO( prvTimerTask, pvParameters ) PRIVILEGED_FUNCTION;

/*
 * 由定时器服务任务调用，用于解析并处理其从定时器队列上收到的命令。
 */
    static void prvProcessReceivedCommands( void ) PRIVILEGED_FUNCTION;

/*
 * 根据到期时间是否引起定时器计数器溢出，将定时器插入 xActiveTimerList1
 * 或 xActiveTimerList2。
 */
    static BaseType_t prvInsertTimerInActiveList( Timer_t * const pxTimer,
                                                  const TickType_t xNextExpiryTime,
                                                  const TickType_t xTimeNow,
                                                  const TickType_t xCommandTime ) PRIVILEGED_FUNCTION;

/*
 * 重新装载指定的自动重载定时器。如果重载出现积压，则清除积压，
 * 并为每次额外重载调用一次回调函数。该函数返回时，下一次到期时间将晚于 xTimeNow。
 */
    static void prvReloadTimer( Timer_t * const pxTimer,
                                TickType_t xExpiredTime,
                                const TickType_t xTimeNow ) PRIVILEGED_FUNCTION;

/*
 * 活动定时器已到达其到期时间。若为自动重载定时器则重新装载，然后调用其回调函数。
 */
    static void prvProcessExpiredTimer( const TickType_t xNextExpireTime,
                                        const TickType_t xTimeNow ) PRIVILEGED_FUNCTION;

/*
 * 节拍计数已溢出。在确认当前定时器链表不再引用任何定时器后，切换定时器链表。
 */
    static void prvSwitchTimerLists( void ) PRIVILEGED_FUNCTION;

/*
 * 获取当前节拍计数；如果自上次调用 prvSampleTimeNow() 以来发生了节拍计数溢出，
 * 则将 *pxTimerListsWereSwitched 置为 pdTRUE。
 */
    static TickType_t prvSampleTimeNow( BaseType_t * const pxTimerListsWereSwitched ) PRIVILEGED_FUNCTION;

/*
 * 如果定时器链表中存在活动定时器，则返回最先到期定时器的到期时间，
 * 并将 *pxListWasEmpty 置为 false。如果定时器链表中没有任何定时器，
 * 则返回 0，并将 *pxListWasEmpty 置为 pdTRUE。
 */
    static TickType_t prvGetNextExpireTime( BaseType_t * const pxListWasEmpty ) PRIVILEGED_FUNCTION;

/*
 * 如果有定时器到期，则处理该定时器。否则阻塞定时器服务任务，
 * 直到有定时器到期或收到命令。
 */
    static void prvProcessTimerOrBlockTask( const TickType_t xNextExpireTime,
                                            BaseType_t xListWasEmpty ) PRIVILEGED_FUNCTION;

/*
 * 在静态或动态分配 Timer_t 结构体之后调用，用于填充该结构体的成员。
 */
    static void prvInitialiseNewTimer( const char * const pcTimerName,
                                       const TickType_t xTimerPeriodInTicks,
                                       const BaseType_t xAutoReload,
                                       void * const pvTimerID,
                                       TimerCallbackFunction_t pxCallbackFunction,
                                       Timer_t * pxNewTimer ) PRIVILEGED_FUNCTION;
/*-----------------------------------------------------------*/

    BaseType_t xTimerCreateTimerTask( void )
    {
        BaseType_t xReturn = pdFAIL;

        traceENTER_xTimerCreateTimerTask();

        /*
         * 如果 configUSE_TIMERS 设置为 1，则在调度器启动时调用本函数。
         * 检查定时器服务任务所需的基础设施是否已创建/初始化。如果定时器已经创建，
         * 则初始化工作早已完成。
         */
        prvCheckForValidListAndQueue();

        if( xTimerQueue != NULL )
        {
            #if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
            {
                #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
                {
                    StaticTask_t * pxTimerTaskTCBBuffer = NULL;
                    StackType_t * pxTimerTaskStackBuffer = NULL;
                    configSTACK_DEPTH_TYPE uxTimerTaskStackSize;

                    vApplicationGetTimerTaskMemory( &pxTimerTaskTCBBuffer, &pxTimerTaskStackBuffer, &uxTimerTaskStackSize );
                    xTimerTaskHandle = xTaskCreateStaticAffinitySet( &prvTimerTask,
                                                                     configTIMER_SERVICE_TASK_NAME,
                                                                     uxTimerTaskStackSize,
                                                                     NULL,
                                                                     ( ( UBaseType_t ) configTIMER_TASK_PRIORITY ) | portPRIVILEGE_BIT,
                                                                     pxTimerTaskStackBuffer,
                                                                     pxTimerTaskTCBBuffer,
                                                                     configTIMER_SERVICE_TASK_CORE_AFFINITY );

                    if( xTimerTaskHandle != NULL )
                    {
                        xReturn = pdPASS;
                    }
                }
                #else /* 条件不成立：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
                {
                    xReturn = xTaskCreateAffinitySet( &prvTimerTask,
                                                      configTIMER_SERVICE_TASK_NAME,
                                                      configTIMER_TASK_STACK_DEPTH,
                                                      NULL,
                                                      ( ( UBaseType_t ) configTIMER_TASK_PRIORITY ) | portPRIVILEGE_BIT,
                                                      configTIMER_SERVICE_TASK_CORE_AFFINITY,
                                                      &xTimerTaskHandle );
                }
                #endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */
            }
            #else /* 条件不成立：多核且启用核亲和性 ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */
            {
                #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
                {
                    StaticTask_t * pxTimerTaskTCBBuffer = NULL;
                    StackType_t * pxTimerTaskStackBuffer = NULL;
                    configSTACK_DEPTH_TYPE uxTimerTaskStackSize;

                    vApplicationGetTimerTaskMemory( &pxTimerTaskTCBBuffer, &pxTimerTaskStackBuffer, &uxTimerTaskStackSize );
                    xTimerTaskHandle = xTaskCreateStatic( &prvTimerTask,
                                                          configTIMER_SERVICE_TASK_NAME,
                                                          uxTimerTaskStackSize,
                                                          NULL,
                                                          ( ( UBaseType_t ) configTIMER_TASK_PRIORITY ) | portPRIVILEGE_BIT,
                                                          pxTimerTaskStackBuffer,
                                                          pxTimerTaskTCBBuffer );

                    if( xTimerTaskHandle != NULL )
                    {
                        xReturn = pdPASS;
                    }
                }
                #else /* 条件不成立：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
                {
                    xReturn = xTaskCreate( &prvTimerTask,
                                           configTIMER_SERVICE_TASK_NAME,
                                           configTIMER_TASK_STACK_DEPTH,
                                           NULL,
                                           ( ( UBaseType_t ) configTIMER_TASK_PRIORITY ) | portPRIVILEGE_BIT,
                                           &xTimerTaskHandle );
                }
                #endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */
            }
            #endif /* 条件不成立：多核且启用核亲和性 ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) ) */
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        configASSERT( xReturn );

        traceRETURN_xTimerCreateTimerTask( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

        TimerHandle_t xTimerCreate( const char * const pcTimerName,
                                    const TickType_t xTimerPeriodInTicks,
                                    const BaseType_t xAutoReload,
                                    void * const pvTimerID,
                                    TimerCallbackFunction_t pxCallbackFunction )
        {
            Timer_t * pxNewTimer;

            traceENTER_xTimerCreate( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction );

            /* MISRA 参考 11.5.1 [malloc 内存赋值] */
            /* 详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation] —— MISRA C:2012 规则 11.5 违规抑制 */
            pxNewTimer = ( Timer_t * ) pvPortMalloc( sizeof( Timer_t ) );

            if( pxNewTimer != NULL )
            {
                /*
                 * 此时状态值仍为零，因为该定时器不是静态创建的，且尚未启动。
                 * 自动重载位可能在 prvInitialiseNewTimer 中设置。
                 */
                pxNewTimer->ucStatus = 0x00;
                prvInitialiseNewTimer( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction, pxNewTimer );
            }

            traceRETURN_xTimerCreate( pxNewTimer );

            return pxNewTimer;
        }

    #endif /* 配置项 configSUPPORT_DYNAMIC_ALLOCATION */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

        TimerHandle_t xTimerCreateStatic( const char * const pcTimerName,
                                          const TickType_t xTimerPeriodInTicks,
                                          const BaseType_t xAutoReload,
                                          void * const pvTimerID,
                                          TimerCallbackFunction_t pxCallbackFunction,
                                          StaticTimer_t * pxTimerBuffer )
        {
            Timer_t * pxNewTimer;

            traceENTER_xTimerCreateStatic( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction, pxTimerBuffer );

            #if ( configASSERT_DEFINED == 1 )
            {
                /*
                 * 健全性检查：用于声明 StaticTimer_t 类型变量的结构体大小，
                 * 是否等于实际定时器结构体的大小。
                 */
                volatile size_t xSize = sizeof( StaticTimer_t );
                configASSERT( xSize == sizeof( Timer_t ) );
                ( void ) xSize; /* 在未定义 configASSERT() 时防止出现未使用变量警告。 */
            }
            #endif /* 配置项 configASSERT_DEFINED */

            /* 必须提供指向 StaticTimer_t 结构体的指针，直接使用该指针。 */
            configASSERT( pxTimerBuffer );
            /* MISRA 参考 11.3.1 [非对齐访问] */
            /* 详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
            /* coverity[misra_c_2012_rule_11_3_violation] —— MISRA C:2012 规则 11.3 违规抑制 */
            pxNewTimer = ( Timer_t * ) pxTimerBuffer;

            if( pxNewTimer != NULL )
            {
                /*
                 * 定时器可以静态或动态创建，因此在此标记该定时器为静态创建，以备后续删除时使用。
                 * 自动重载位可能在 prvInitialiseNewTimer() 中设置。
                 */
                pxNewTimer->ucStatus = ( uint8_t ) tmrSTATUS_IS_STATICALLY_ALLOCATED;

                prvInitialiseNewTimer( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction, pxNewTimer );
            }

            traceRETURN_xTimerCreateStatic( pxNewTimer );

            return pxNewTimer;
        }

    #endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

    static void prvInitialiseNewTimer( const char * const pcTimerName,
                                       const TickType_t xTimerPeriodInTicks,
                                       const BaseType_t xAutoReload,
                                       void * const pvTimerID,
                                       TimerCallbackFunction_t pxCallbackFunction,
                                       Timer_t * pxNewTimer )
    {
        /* 0 不是 xTimerPeriodInTicks 的有效值。 */
        configASSERT( ( xTimerPeriodInTicks > 0 ) );

        /*
         * 确保定时器服务任务所需的基础设施已创建/初始化。
         */
        prvCheckForValidListAndQueue();

        /*
         * 使用函数参数初始化定时器结构体的成员。
         */
        pxNewTimer->pcTimerName = pcTimerName;
        pxNewTimer->xTimerPeriodInTicks = xTimerPeriodInTicks;
        pxNewTimer->pvTimerID = pvTimerID;
        pxNewTimer->pxCallbackFunction = pxCallbackFunction;
        vListInitialiseItem( &( pxNewTimer->xTimerListItem ) );

        if( xAutoReload != pdFALSE )
        {
            pxNewTimer->ucStatus |= ( uint8_t ) tmrSTATUS_IS_AUTORELOAD;
        }

        traceTIMER_CREATE( pxNewTimer );
    }
/*-----------------------------------------------------------*/

    BaseType_t xTimerGenericCommandFromTask( TimerHandle_t xTimer,
                                             const BaseType_t xCommandID,
                                             const TickType_t xOptionalValue,
                                             BaseType_t * const pxHigherPriorityTaskWoken,
                                             const TickType_t xTicksToWait )
    {
        BaseType_t xReturn = pdFAIL;
        DaemonTaskMessage_t xMessage;

        ( void ) pxHigherPriorityTaskWoken;

        traceENTER_xTimerGenericCommandFromTask( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait );

        /*
         * 向定时器服务任务发送消息，以对指定的定时器执行特定操作。
         */
        if( ( xTimerQueue != NULL ) && ( xTimer != NULL ) )
        {
            /* 向定时器服务任务发送命令，以启动 xTimer 定时器。 */
            xMessage.xMessageID = xCommandID;
            xMessage.u.xTimerParameters.xMessageValue = xOptionalValue;
            xMessage.u.xTimerParameters.pxTimer = xTimer;

            /*
             * 同时施加下界和上界限制，确保此处只接受由任务发出的有效命令。
             */
            configASSERT( ( xCommandID >= tmrCOMMAND_START_DONT_TRACE ) && ( xCommandID < tmrFIRST_FROM_ISR_COMMAND ) );

            if( ( xCommandID >= tmrCOMMAND_START_DONT_TRACE ) && ( xCommandID < tmrFIRST_FROM_ISR_COMMAND ) )
            {
                if( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING )
                {
                    xReturn = xQueueSendToBack( xTimerQueue, &xMessage, xTicksToWait );
                }
                else
                {
                    xReturn = xQueueSendToBack( xTimerQueue, &xMessage, tmrNO_DELAY );
                }
            }

            traceTIMER_COMMAND_SEND( xTimer, xCommandID, xOptionalValue, xReturn );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTimerGenericCommandFromTask( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    BaseType_t xTimerGenericCommandFromISR( TimerHandle_t xTimer,
                                            const BaseType_t xCommandID,
                                            const TickType_t xOptionalValue,
                                            BaseType_t * const pxHigherPriorityTaskWoken,
                                            const TickType_t xTicksToWait )
    {
        BaseType_t xReturn = pdFAIL;
        DaemonTaskMessage_t xMessage;

        ( void ) xTicksToWait;

        traceENTER_xTimerGenericCommandFromISR( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait );

        /*
         * 向定时器服务任务发送消息，以对指定的定时器执行特定操作。
         */
        if( ( xTimerQueue != NULL ) && ( xTimer != NULL ) )
        {
            /* 向定时器服务任务发送命令，以启动 xTimer 定时器。 */
            xMessage.xMessageID = xCommandID;
            xMessage.u.xTimerParameters.xMessageValue = xOptionalValue;
            xMessage.u.xTimerParameters.pxTimer = xTimer;

            configASSERT( xCommandID >= tmrFIRST_FROM_ISR_COMMAND );

            if( xCommandID >= tmrFIRST_FROM_ISR_COMMAND )
            {
                xReturn = xQueueSendToBackFromISR( xTimerQueue, &xMessage, pxHigherPriorityTaskWoken );
            }

            traceTIMER_COMMAND_SEND( xTimer, xCommandID, xOptionalValue, xReturn );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceRETURN_xTimerGenericCommandFromISR( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    TaskHandle_t xTimerGetTimerDaemonTaskHandle( void )
    {
        traceENTER_xTimerGetTimerDaemonTaskHandle();

        /*
         * 如果在调度器启动之前调用 xTimerGetTimerDaemonTaskHandle()，
         * 则 xTimerTaskHandle 为 NULL。
         */
        configASSERT( ( xTimerTaskHandle != NULL ) );

        traceRETURN_xTimerGetTimerDaemonTaskHandle( xTimerTaskHandle );

        return xTimerTaskHandle;
    }
/*-----------------------------------------------------------*/

    TickType_t xTimerGetPeriod( TimerHandle_t xTimer )
    {
        Timer_t * pxTimer = xTimer;

        traceENTER_xTimerGetPeriod( xTimer );

        configASSERT( xTimer );

        traceRETURN_xTimerGetPeriod( pxTimer->xTimerPeriodInTicks );

        return pxTimer->xTimerPeriodInTicks;
    }
/*-----------------------------------------------------------*/

    void vTimerSetReloadMode( TimerHandle_t xTimer,
                              const BaseType_t xAutoReload )
    {
        Timer_t * pxTimer = xTimer;

        traceENTER_vTimerSetReloadMode( xTimer, xAutoReload );

        configASSERT( xTimer );
        taskENTER_CRITICAL();
        {
            if( xAutoReload != pdFALSE )
            {
                pxTimer->ucStatus |= ( uint8_t ) tmrSTATUS_IS_AUTORELOAD;
            }
            else
            {
                pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_AUTORELOAD );
            }
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTimerSetReloadMode();
    }
/*-----------------------------------------------------------*/

    BaseType_t xTimerGetReloadMode( TimerHandle_t xTimer )
    {
        Timer_t * pxTimer = xTimer;
        BaseType_t xReturn;

        traceENTER_xTimerGetReloadMode( xTimer );

        configASSERT( xTimer );
        portBASE_TYPE_ENTER_CRITICAL();
        {
            if( ( pxTimer->ucStatus & tmrSTATUS_IS_AUTORELOAD ) == 0U )
            {
                /* 不是自动重载定时器。 */
                xReturn = pdFALSE;
            }
            else
            {
                /* 是自动重载定时器。 */
                xReturn = pdTRUE;
            }
        }
        portBASE_TYPE_EXIT_CRITICAL();

        traceRETURN_xTimerGetReloadMode( xReturn );

        return xReturn;
    }

    UBaseType_t uxTimerGetReloadMode( TimerHandle_t xTimer )
    {
        UBaseType_t uxReturn;

        traceENTER_uxTimerGetReloadMode( xTimer );

        uxReturn = ( UBaseType_t ) xTimerGetReloadMode( xTimer );

        traceRETURN_uxTimerGetReloadMode( uxReturn );

        return uxReturn;
    }
/*-----------------------------------------------------------*/

    TickType_t xTimerGetExpiryTime( TimerHandle_t xTimer )
    {
        Timer_t * pxTimer = xTimer;
        TickType_t xReturn;

        traceENTER_xTimerGetExpiryTime( xTimer );

        configASSERT( xTimer );
        xReturn = listGET_LIST_ITEM_VALUE( &( pxTimer->xTimerListItem ) );

        traceRETURN_xTimerGetExpiryTime( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
        BaseType_t xTimerGetStaticBuffer( TimerHandle_t xTimer,
                                          StaticTimer_t ** ppxTimerBuffer )
        {
            BaseType_t xReturn;
            Timer_t * pxTimer = xTimer;

            traceENTER_xTimerGetStaticBuffer( xTimer, ppxTimerBuffer );

            configASSERT( ppxTimerBuffer != NULL );

            if( ( pxTimer->ucStatus & tmrSTATUS_IS_STATICALLY_ALLOCATED ) != 0U )
            {
                /* MISRA 参考 11.3.1 [非对齐访问] */
                /* 详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
                /* coverity[misra_c_2012_rule_11_3_violation] —— MISRA C:2012 规则 11.3 违规抑制 */
                *ppxTimerBuffer = ( StaticTimer_t * ) pxTimer;
                xReturn = pdTRUE;
            }
            else
            {
                xReturn = pdFALSE;
            }

            traceRETURN_xTimerGetStaticBuffer( xReturn );

            return xReturn;
        }
    #endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

    const char * pcTimerGetName( TimerHandle_t xTimer )
    {
        Timer_t * pxTimer = xTimer;

        traceENTER_pcTimerGetName( xTimer );

        configASSERT( xTimer );

        traceRETURN_pcTimerGetName( pxTimer->pcTimerName );

        return pxTimer->pcTimerName;
    }
/*-----------------------------------------------------------*/

    static void prvReloadTimer( Timer_t * const pxTimer,
                                TickType_t xExpiredTime,
                                const TickType_t xTimeNow )
    {
        /*
         * 将定时器按下一次到期时间插入相应的链表。如果下一次到期时间已经过去，
         * 则推进到期时间、调用回调函数，然后再次尝试。
         */
        while( prvInsertTimerInActiveList( pxTimer, ( xExpiredTime + pxTimer->xTimerPeriodInTicks ), xTimeNow, xExpiredTime ) != pdFALSE )
        {
            /* 推进到期时间。 */
            xExpiredTime += pxTimer->xTimerPeriodInTicks;

            /* 调用定时器回调函数。 */
            traceTIMER_EXPIRED( pxTimer );
            pxTimer->pxCallbackFunction( ( TimerHandle_t ) pxTimer );
        }
    }
/*-----------------------------------------------------------*/

    static void prvProcessExpiredTimer( const TickType_t xNextExpireTime,
                                        const TickType_t xTimeNow )
    {
        /* MISRA 参考 11.5.3 [void 指针赋值] */
        /* 详情参见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation] —— MISRA C:2012 规则 11.5 违规抑制 */
        Timer_t * const pxTimer = ( Timer_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxCurrentTimerList );

        /*
         * 从活动定时器链表中移除该定时器。此前已做过检查，确保链表非空。
         */

        ( void ) uxListRemove( &( pxTimer->xTimerListItem ) );

        /*
         * 如果该定时器是自动重载定时器，则计算下一次到期时间，
         * 并将定时器重新插入活动定时器链表。
         */
        if( ( pxTimer->ucStatus & tmrSTATUS_IS_AUTORELOAD ) != 0U )
        {
            prvReloadTimer( pxTimer, xNextExpireTime, xTimeNow );
        }
        else
        {
            pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );
        }

        /* 调用定时器回调函数。 */
        traceTIMER_EXPIRED( pxTimer );
        pxTimer->pxCallbackFunction( ( TimerHandle_t ) pxTimer );
    }
/*-----------------------------------------------------------*/

    static portTASK_FUNCTION( prvTimerTask, pvParameters )
    {
        TickType_t xNextExpireTime;
        BaseType_t xListWasEmpty;

        /* 仅用于避免编译器警告。 */
        ( void ) pvParameters;

        #if ( configUSE_DAEMON_TASK_STARTUP_HOOK == 1 )
        {
            /*
             * 允许应用开发者在任务开始执行时，于该任务的上下文中执行一些代码。
             * 如果应用包含适合在调度器启动后执行的初始化代码，这将很有用。
             */
            vApplicationDaemonTaskStartupHook();
        }
        #endif /* 配置项 configUSE_DAEMON_TASK_STARTUP_HOOK */

        for( ; configCONTROL_INFINITE_LOOP(); )
        {
            /*
             * 查询定时器链表以判断其中是否有定时器；如果有，则获取下一个定时器的到期时间。
             */
            xNextExpireTime = prvGetNextExpireTime( &xListWasEmpty );

            /*
             * 如果有定时器到期，则处理该定时器。否则阻塞本任务，直到有定时器到期或收到命令。
             */
            prvProcessTimerOrBlockTask( xNextExpireTime, xListWasEmpty );

            /* 清空命令队列。 */
            prvProcessReceivedCommands();
        }
    }
/*-----------------------------------------------------------*/

    static void prvProcessTimerOrBlockTask( const TickType_t xNextExpireTime,
                                            BaseType_t xListWasEmpty )
    {
        TickType_t xTimeNow;
        BaseType_t xTimerListsWereSwitched;

        vTaskSuspendAll();
        {
            /*
             * 获取当前时间，以判断定时器是否已到期。如果获取时间导致链表切换，
             * 则不要处理该定时器，因为在链表切换时仍留在链表中的定时器，
             * 都已在 prvSampleTimeNow() 函数内部处理完毕。
             */
            xTimeNow = prvSampleTimeNow( &xTimerListsWereSwitched );

            if( xTimerListsWereSwitched == pdFALSE )
            {
                /* 节拍计数尚未溢出，定时器是否已到期？ */
                if( ( xListWasEmpty == pdFALSE ) && ( xNextExpireTime <= xTimeNow ) )
                {
                    ( void ) xTaskResumeAll();
                    prvProcessExpiredTimer( xNextExpireTime, xTimeNow );
                }
                else
                {
                    /*
                     * 节拍计数尚未溢出，且尚未到达下一次到期时间。因此本任务应阻塞，
                     * 等待下一次到期时间或收到命令，二者以先发生者为准。除非 xNextExpireTime > xTimeNow，
                     * 否则不会执行到下面这一行；当前定时器链表为空的情况除外。
                     */
                    if( xListWasEmpty != pdFALSE )
                    {
                        /*
                         * 当前定时器链表为空——溢出链表是否也为空？
                         */
                        xListWasEmpty = listLIST_IS_EMPTY( pxOverflowTimerList );
                    }

                    vQueueWaitForMessageRestricted( xTimerQueue, ( xNextExpireTime - xTimeNow ), xListWasEmpty );

                    if( xTaskResumeAll() == pdFALSE )
                    {
                        /*
                         * 让出 CPU，以等待命令到达或阻塞时间到期。如果在退出临界区之后、
                         * 执行本次让出之前已有命令到达，则本次让出不会导致任务阻塞。
                         */
                        taskYIELD_WITHIN_API();
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
            }
            else
            {
                ( void ) xTaskResumeAll();
            }
        }
    }
/*-----------------------------------------------------------*/

    static TickType_t prvGetNextExpireTime( BaseType_t * const pxListWasEmpty )
    {
        TickType_t xNextExpireTime;

        /*
         * 定时器按到期时间排序，链表头部引用的是最先到期的定时器。
         * 获取最近到期定时器的到期时间。如果没有活动定时器，则将下一次到期时间置为 0。
         * 这样本任务会在节拍计数溢出时解除阻塞，届时将切换定时器链表，
         * 并可重新评估下一次到期时间。
         */
        *pxListWasEmpty = listLIST_IS_EMPTY( pxCurrentTimerList );

        if( *pxListWasEmpty == pdFALSE )
        {
            xNextExpireTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxCurrentTimerList );
        }
        else
        {
            /* 确保任务在节拍计数回绕时解除阻塞。 */
            xNextExpireTime = ( TickType_t ) 0U;
        }

        return xNextExpireTime;
    }
/*-----------------------------------------------------------*/

    static TickType_t prvSampleTimeNow( BaseType_t * const pxTimerListsWereSwitched )
    {
        TickType_t xTimeNow;
        PRIVILEGED_DATA static TickType_t xLastTime = ( TickType_t ) 0U;

        xTimeNow = xTaskGetTickCount();

        if( xTimeNow < xLastTime )
        {
            prvSwitchTimerLists();
            *pxTimerListsWereSwitched = pdTRUE;
        }
        else
        {
            *pxTimerListsWereSwitched = pdFALSE;
        }

        xLastTime = xTimeNow;

        return xTimeNow;
    }
/*-----------------------------------------------------------*/

    static BaseType_t prvInsertTimerInActiveList( Timer_t * const pxTimer,
                                                  const TickType_t xNextExpiryTime,
                                                  const TickType_t xTimeNow,
                                                  const TickType_t xCommandTime )
    {
        BaseType_t xProcessTimerNow = pdFALSE;

        listSET_LIST_ITEM_VALUE( &( pxTimer->xTimerListItem ), xNextExpiryTime );
        listSET_LIST_ITEM_OWNER( &( pxTimer->xTimerListItem ), pxTimer );

        if( xNextExpiryTime <= xTimeNow )
        {
            /*
             * 在发出启动/复位定时器的命令与处理该命令之间，是否已经过了到期时间？
             */
            if( ( ( TickType_t ) ( xTimeNow - xCommandTime ) ) >= pxTimer->xTimerPeriodInTicks )
            {
                /*
                 * 从发出命令到处理命令之间的时间，实际已超过定时器周期。
                 */
                xProcessTimerNow = pdTRUE;
            }
            else
            {
                vListInsert( pxOverflowTimerList, &( pxTimer->xTimerListItem ) );
            }
        }
        else
        {
            if( ( xTimeNow < xCommandTime ) && ( xNextExpiryTime >= xCommandTime ) )
            {
                /*
                 * 如果自命令发出以来节拍计数已溢出，但到期时间尚未溢出，
                 * 则该定时器必定已经过了到期时间，应立即处理。
                 */
                xProcessTimerNow = pdTRUE;
            }
            else
            {
                vListInsert( pxCurrentTimerList, &( pxTimer->xTimerListItem ) );
            }
        }

        return xProcessTimerNow;
    }
/*-----------------------------------------------------------*/

    static void prvProcessReceivedCommands( void )
    {
        DaemonTaskMessage_t xMessage = { 0 };
        Timer_t * pxTimer;
        BaseType_t xTimerListsWereSwitched;
        TickType_t xTimeNow;

        while( xQueueReceive( xTimerQueue, &xMessage, tmrNO_DELAY ) != pdFAIL )
        {
            #if ( INCLUDE_xTimerPendFunctionCall == 1 )
            {
                /*
                 * 负值命令表示挂起的函数调用，而不是定时器命令。
                 */
                if( xMessage.xMessageID < ( BaseType_t ) 0 )
                {
                    const CallbackParameters_t * const pxCallback = &( xMessage.u.xCallbackParameters );

                    /*
                     * 定时器通过 xCallbackParameters 成员请求执行回调。检查该回调函数不为 NULL。
                     */
                    configASSERT( pxCallback );

                    /* 调用该函数。 */
                    pxCallback->pxCallbackFunction( pxCallback->pvParameter1, pxCallback->ulParameter2 );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            #endif /* 配置项 INCLUDE_xTimerPendFunctionCall */

            /*
             * 正值命令是定时器命令，而不是挂起的函数调用。
             */
            if( xMessage.xMessageID >= ( BaseType_t ) 0 )
            {
                /*
                 * 该消息使用 xTimerParameters 成员来操作软件定时器。
                 */
                pxTimer = xMessage.u.xTimerParameters.pxTimer;

                if( pxTimer != NULL )
                {
                    if( listIS_CONTAINED_WITHIN( NULL, &( pxTimer->xTimerListItem ) ) == pdFALSE )
                    {
                        /* 定时器位于某个链表中，将其移除。 */
                        ( void ) uxListRemove( &( pxTimer->xTimerListItem ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    traceTIMER_COMMAND_RECEIVED( pxTimer, xMessage.xMessageID, xMessage.u.xTimerParameters.xMessageValue );

                    /*
                     * 此处不使用 xTimerListsWereSwitched 参数，但函数调用中必须提供它。
                     * 必须在从 xTimerQueue 收到消息之后再调用 prvSampleTimeNow()，这样就不会出现
                     * 更高优先级任务把时间更早的消息加入消息队列的情况
                     * （因为它是在 xTimeNow 取值之后抢占定时器守护任务才加入的）。
                     */
                    xTimeNow = prvSampleTimeNow( &xTimerListsWereSwitched );

                    switch( xMessage.xMessageID )
                    {
                        case tmrCOMMAND_START:
                        case tmrCOMMAND_START_FROM_ISR:
                        case tmrCOMMAND_RESET:
                        case tmrCOMMAND_RESET_FROM_ISR:
                            /* 启动或重新启动定时器。 */
                            pxTimer->ucStatus |= ( uint8_t ) tmrSTATUS_IS_ACTIVE;

                            if( prvInsertTimerInActiveList( pxTimer, xMessage.u.xTimerParameters.xMessageValue + pxTimer->xTimerPeriodInTicks, xTimeNow, xMessage.u.xTimerParameters.xMessageValue ) != pdFALSE )
                            {
                                /*
                                 * 定时器在被加入活动定时器链表之前就已到期。立即处理该定时器。
                                 */
                                if( ( pxTimer->ucStatus & tmrSTATUS_IS_AUTORELOAD ) != 0U )
                                {
                                    prvReloadTimer( pxTimer, xMessage.u.xTimerParameters.xMessageValue + pxTimer->xTimerPeriodInTicks, xTimeNow );
                                }
                                else
                                {
                                    pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );
                                }

                                /* 调用定时器回调函数。 */
                                traceTIMER_EXPIRED( pxTimer );
                                pxTimer->pxCallbackFunction( ( TimerHandle_t ) pxTimer );
                            }
                            else
                            {
                                mtCOVERAGE_TEST_MARKER();
                            }

                            break;

                        case tmrCOMMAND_STOP:
                        case tmrCOMMAND_STOP_FROM_ISR:
                            /* 该定时器已从活动链表中移除。 */
                            pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );
                            break;

                        case tmrCOMMAND_CHANGE_PERIOD:
                        case tmrCOMMAND_CHANGE_PERIOD_FROM_ISR:
                            pxTimer->ucStatus |= ( uint8_t ) tmrSTATUS_IS_ACTIVE;
                            pxTimer->xTimerPeriodInTicks = xMessage.u.xTimerParameters.xMessageValue;
                            configASSERT( ( pxTimer->xTimerPeriodInTicks > 0 ) );

                            /*
                             * 新周期并没有可供参照的基准，可能比旧周期更长或更短。因此将命令时间设为当前时间；
                             * 由于周期不能为零，下一次到期时间只能位于未来，这意味着
                             * （不同于上面的 xTimerStart() 情形）此处无需处理失败情况。
                             */
                            ( void ) prvInsertTimerInActiveList( pxTimer, ( xTimeNow + pxTimer->xTimerPeriodInTicks ), xTimeNow, xTimeNow );
                            break;

                        case tmrCOMMAND_DELETE:
                            #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
                            {
                                /*
                                 * 该定时器已从活动链表中移除；如果内存是动态分配的，只需释放该内存。
                                 */
                                if( ( pxTimer->ucStatus & tmrSTATUS_IS_STATICALLY_ALLOCATED ) == ( uint8_t ) 0 )
                                {
                                    vPortFree( pxTimer );
                                }
                                else
                                {
                                    pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );
                                }
                            }
                            #else /* 条件不成立：if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) */
                            {
                                /*
                                 * 如果未启用动态分配，则内存不可能是动态分配的。
                                 * 因此无需释放内存，只需将定时器标记为“非活动”。
                                 */
                                pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );
                            }
                            #endif /* 配置项 configSUPPORT_DYNAMIC_ALLOCATION */
                            break;

                        default:
                            /* 不应执行到这里。 */
                            break;
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
        }
    }
/*-----------------------------------------------------------*/

    static void prvSwitchTimerLists( void )
    {
        TickType_t xNextExpireTime;
        List_t * pxTemp;

        /*
         * 节拍计数已溢出，必须切换定时器链表。如果当前定时器链表中仍引用某些定时器，
         * 则这些定时器必定已经到期，应在切换链表之前处理完毕。
         */
        while( listLIST_IS_EMPTY( pxCurrentTimerList ) == pdFALSE )
        {
            xNextExpireTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxCurrentTimerList );

            /*
             * 处理已到期的定时器。对于自动重载定时器，注意只处理在当前链表上发生的到期；
             * 后续到期必须等到链表切换之后再处理。
             */
            prvProcessExpiredTimer( xNextExpireTime, tmrMAX_TIME_BEFORE_OVERFLOW );
        }

        pxTemp = pxCurrentTimerList;
        pxCurrentTimerList = pxOverflowTimerList;
        pxOverflowTimerList = pxTemp;
    }
/*-----------------------------------------------------------*/

    static void prvCheckForValidListAndQueue( void )
    {
        /*
         * 检查用于引用活动定时器的链表，以及与定时器服务通信的队列是否已初始化。
         */
        taskENTER_CRITICAL();
        {
            if( xTimerQueue == NULL )
            {
                vListInitialise( &xActiveTimerList1 );
                vListInitialise( &xActiveTimerList2 );
                pxCurrentTimerList = &xActiveTimerList1;
                pxOverflowTimerList = &xActiveTimerList2;

                #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
                {
                    /*
                     * 定时器队列采用静态分配，以防 configSUPPORT_DYNAMIC_ALLOCATION 为 0。
                     */
                    PRIVILEGED_DATA static StaticQueue_t xStaticTimerQueue;
                    PRIVILEGED_DATA static uint8_t ucStaticTimerQueueStorage[ ( size_t ) configTIMER_QUEUE_LENGTH * sizeof( DaemonTaskMessage_t ) ];

                    xTimerQueue = xQueueCreateStatic( ( UBaseType_t ) configTIMER_QUEUE_LENGTH, ( UBaseType_t ) sizeof( DaemonTaskMessage_t ), &( ucStaticTimerQueueStorage[ 0 ] ), &xStaticTimerQueue );
                }
                #else
                {
                    xTimerQueue = xQueueCreate( ( UBaseType_t ) configTIMER_QUEUE_LENGTH, ( UBaseType_t ) sizeof( DaemonTaskMessage_t ) );
                }
                #endif /* 条件块结束：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */

                #if ( configQUEUE_REGISTRY_SIZE > 0 )
                {
                    if( xTimerQueue != NULL )
                    {
                        vQueueAddToRegistry( xTimerQueue, "TmrQ" );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                #endif /* 配置项 configQUEUE_REGISTRY_SIZE */
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();
    }
/*-----------------------------------------------------------*/

    BaseType_t xTimerIsTimerActive( TimerHandle_t xTimer )
    {
        BaseType_t xReturn;
        Timer_t * pxTimer = xTimer;

        traceENTER_xTimerIsTimerActive( xTimer );

        configASSERT( xTimer );

        /* 该定时器是否位于活动定时器链表中？ */
        portBASE_TYPE_ENTER_CRITICAL();
        {
            if( ( pxTimer->ucStatus & tmrSTATUS_IS_ACTIVE ) == 0U )
            {
                xReturn = pdFALSE;
            }
            else
            {
                xReturn = pdTRUE;
            }
        }
        portBASE_TYPE_EXIT_CRITICAL();

        traceRETURN_xTimerIsTimerActive( xReturn );

        return xReturn;
    }
/*-----------------------------------------------------------*/

    void * pvTimerGetTimerID( const TimerHandle_t xTimer )
    {
        Timer_t * const pxTimer = xTimer;
        void * pvReturn;

        traceENTER_pvTimerGetTimerID( xTimer );

        configASSERT( xTimer );

        taskENTER_CRITICAL();
        {
            pvReturn = pxTimer->pvTimerID;
        }
        taskEXIT_CRITICAL();

        traceRETURN_pvTimerGetTimerID( pvReturn );

        return pvReturn;
    }
/*-----------------------------------------------------------*/

    void vTimerSetTimerID( TimerHandle_t xTimer,
                           void * pvNewID )
    {
        Timer_t * const pxTimer = xTimer;

        traceENTER_vTimerSetTimerID( xTimer, pvNewID );

        configASSERT( xTimer );

        taskENTER_CRITICAL();
        {
            pxTimer->pvTimerID = pvNewID;
        }
        taskEXIT_CRITICAL();

        traceRETURN_vTimerSetTimerID();
    }
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTimerPendFunctionCall == 1 )

        BaseType_t xTimerPendFunctionCallFromISR( PendedFunction_t xFunctionToPend,
                                                  void * pvParameter1,
                                                  uint32_t ulParameter2,
                                                  BaseType_t * pxHigherPriorityTaskWoken )
        {
            DaemonTaskMessage_t xMessage;
            BaseType_t xReturn;

            traceENTER_xTimerPendFunctionCallFromISR( xFunctionToPend, pvParameter1, ulParameter2, pxHigherPriorityTaskWoken );

            /*
             * 用函数参数填充消息，并将其投递给守护任务。
             */
            xMessage.xMessageID = tmrCOMMAND_EXECUTE_CALLBACK_FROM_ISR;
            xMessage.u.xCallbackParameters.pxCallbackFunction = xFunctionToPend;
            xMessage.u.xCallbackParameters.pvParameter1 = pvParameter1;
            xMessage.u.xCallbackParameters.ulParameter2 = ulParameter2;

            xReturn = xQueueSendFromISR( xTimerQueue, &xMessage, pxHigherPriorityTaskWoken );

            tracePEND_FUNC_CALL_FROM_ISR( xFunctionToPend, pvParameter1, ulParameter2, xReturn );
            traceRETURN_xTimerPendFunctionCallFromISR( xReturn );

            return xReturn;
        }

    #endif /* 配置项 INCLUDE_xTimerPendFunctionCall */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTimerPendFunctionCall == 1 )

        BaseType_t xTimerPendFunctionCall( PendedFunction_t xFunctionToPend,
                                           void * pvParameter1,
                                           uint32_t ulParameter2,
                                           TickType_t xTicksToWait )
        {
            DaemonTaskMessage_t xMessage;
            BaseType_t xReturn;

            traceENTER_xTimerPendFunctionCall( xFunctionToPend, pvParameter1, ulParameter2, xTicksToWait );

            /*
             * 只有在创建定时器之后或调度器启动之后才能调用本函数，
             * 因为在此之前定时器队列尚不存在。
             */
            configASSERT( xTimerQueue );

            /*
             * 用函数参数填充消息，并将其投递给守护任务。
             */
            xMessage.xMessageID = tmrCOMMAND_EXECUTE_CALLBACK;
            xMessage.u.xCallbackParameters.pxCallbackFunction = xFunctionToPend;
            xMessage.u.xCallbackParameters.pvParameter1 = pvParameter1;
            xMessage.u.xCallbackParameters.ulParameter2 = ulParameter2;

            xReturn = xQueueSendToBack( xTimerQueue, &xMessage, xTicksToWait );

            tracePEND_FUNC_CALL( xFunctionToPend, pvParameter1, ulParameter2, xReturn );
            traceRETURN_xTimerPendFunctionCall( xReturn );

            return xReturn;
        }

    #endif /* 配置项 INCLUDE_xTimerPendFunctionCall */
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

        UBaseType_t uxTimerGetTimerNumber( TimerHandle_t xTimer )
        {
            traceENTER_uxTimerGetTimerNumber( xTimer );

            traceRETURN_uxTimerGetTimerNumber( ( ( Timer_t * ) xTimer )->uxTimerNumber );

            return ( ( Timer_t * ) xTimer )->uxTimerNumber;
        }

    #endif /* 配置项 configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

        void vTimerSetTimerNumber( TimerHandle_t xTimer,
                                   UBaseType_t uxTimerNumber )
        {
            traceENTER_vTimerSetTimerNumber( xTimer, uxTimerNumber );

            ( ( Timer_t * ) xTimer )->uxTimerNumber = uxTimerNumber;

            traceRETURN_vTimerSetTimerNumber();
        }

    #endif /* 配置项 configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。这些状态通常在启动时初始化。
 * 应用必须在重新启动调度器之前调用本函数。
 */
    void vTimerResetState( void )
    {
        xTimerQueue = NULL;
        xTimerTaskHandle = NULL;
    }
/*-----------------------------------------------------------*/

/*
 * 如果应用未配置软件定时器功能，则整个源文件都会被跳过。
 * 若要启用软件定时器功能，请确保在 FreeRTOSConfig.h 中
 * 将 configUSE_TIMERS 设置为 1。
 */
#endif /* 配置项 configUSE_TIMERS == 1 */
