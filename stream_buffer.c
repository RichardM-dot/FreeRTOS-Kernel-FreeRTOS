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
 * 【中文导读】stream_buffer.c —— 流缓冲/消息缓冲（串口收发利器）
 * ----------------------------------------------------------------------------
 * StreamBuffer: 无边界的字节流（像 ring buffer，单读单写无锁 O(1)）；
 * MessageBuffer: 有边界的变长消息（每条前面带 4 字节长度头，基于前者实现）。
 * 比 queue 更轻量（无队列结构体开销），但只推荐单读单写场景。
 * ============================================================================
 */

/* 标准库头文件。 */
#include <string.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有 API
 * 函数重定义为使用 MPU 包装函数。仅当从应用文件包含 task.h 时，才应
 * 这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* FreeRTOS 头文件。 */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

/*
 * MPU 移植层要求为上述头文件定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE，
 * 但本文件不能定义该宏，以便生成正确的特权与非特权链接及放置方式。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*
 * 如果应用未配置启用流缓冲功能，则整个源文件将被跳过。该 #if 在本文件
 * 最末尾关闭。如需启用流缓冲，请确保 FreeRTOSConfig.h 中的
 * configUSE_STREAM_BUFFERS 设置为 1。
 */
#if ( configUSE_STREAM_BUFFERS == 1 )

    #if ( configUSE_TASK_NOTIFICATIONS != 1 )
        #error configUSE_TASK_NOTIFICATIONS must be set to 1 to build stream_buffer.c
    #endif

    #if ( INCLUDE_xTaskGetCurrentTaskHandle != 1 )
        #error INCLUDE_xTaskGetCurrentTaskHandle must be set to 1 to build stream_buffer.c
    #endif

/*
 * 如果用户未提供应用专用的接收通知宏，或已通过 #define 将该通知宏
 * 定义为空，则提供使用任务通知的默认实现。
 */
    #ifndef sbRECEIVE_COMPLETED
        #define sbRECEIVE_COMPLETED( pxStreamBuffer )                                 \
    do                                                                                \
    {                                                                                 \
        vTaskSuspendAll();                                                            \
        {                                                                             \
            if( ( pxStreamBuffer )->xTaskWaitingToSend != NULL )                      \
            {                                                                         \
                ( void ) xTaskNotifyIndexed( ( pxStreamBuffer )->xTaskWaitingToSend,  \
                                             ( pxStreamBuffer )->uxNotificationIndex, \
                                             ( uint32_t ) 0,                          \
                                             eNoAction );                             \
                ( pxStreamBuffer )->xTaskWaitingToSend = NULL;                        \
            }                                                                         \
        }                                                                             \
        ( void ) xTaskResumeAll();                                                    \
    } while( 0 )
    #endif /* sbRECEIVE_COMPLETED（接收完成） */

/*
 * 如果用户提供了每个实例的接收完成回调，则调用该回调；否则使用为
 * 所有实例默认提供的接收完成宏。
 */
    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
        #define prvRECEIVE_COMPLETED( pxStreamBuffer )                                           \
    do {                                                                                         \
        if( ( pxStreamBuffer )->pxReceiveCompletedCallback != NULL )                             \
        {                                                                                        \
            ( pxStreamBuffer )->pxReceiveCompletedCallback( ( pxStreamBuffer ), pdFALSE, NULL ); \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            sbRECEIVE_COMPLETED( ( pxStreamBuffer ) );                                           \
        }                                                                                        \
    } while( 0 )
    #else /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */
        #define prvRECEIVE_COMPLETED( pxStreamBuffer )    sbRECEIVE_COMPLETED( ( pxStreamBuffer ) )
    #endif /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */

    #ifndef sbRECEIVE_COMPLETED_FROM_ISR
        #define sbRECEIVE_COMPLETED_FROM_ISR( pxStreamBuffer,                                \
                                              pxHigherPriorityTaskWoken )                    \
    do {                                                                                     \
        UBaseType_t uxSavedInterruptStatus;                                                  \
                                                                                             \
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();                              \
        {                                                                                    \
            if( ( pxStreamBuffer )->xTaskWaitingToSend != NULL )                             \
            {                                                                                \
                ( void ) xTaskNotifyIndexedFromISR( ( pxStreamBuffer )->xTaskWaitingToSend,  \
                                                    ( pxStreamBuffer )->uxNotificationIndex, \
                                                    ( uint32_t ) 0,                          \
                                                    eNoAction,                               \
                                                    ( pxHigherPriorityTaskWoken ) );         \
                ( pxStreamBuffer )->xTaskWaitingToSend = NULL;                               \
            }                                                                                \
        }                                                                                    \
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );                                \
    } while( 0 )
    #endif /* sbRECEIVE_COMPLETED_FROM_ISR（ISR 中接收完成） */

    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
        #define prvRECEIVE_COMPLETED_FROM_ISR( pxStreamBuffer,                                                           \
                                               pxHigherPriorityTaskWoken )                                               \
    do {                                                                                                                 \
        if( ( pxStreamBuffer )->pxReceiveCompletedCallback != NULL )                                                     \
        {                                                                                                                \
            ( pxStreamBuffer )->pxReceiveCompletedCallback( ( pxStreamBuffer ), pdTRUE, ( pxHigherPriorityTaskWoken ) ); \
        }                                                                                                                \
        else                                                                                                             \
        {                                                                                                                \
            sbRECEIVE_COMPLETED_FROM_ISR( ( pxStreamBuffer ), ( pxHigherPriorityTaskWoken ) );                           \
        }                                                                                                                \
    } while( 0 )
    #else /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */
        #define prvRECEIVE_COMPLETED_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken ) \
    sbRECEIVE_COMPLETED_FROM_ISR( ( pxStreamBuffer ), ( pxHigherPriorityTaskWoken ) )
    #endif /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */

/*
 * 如果用户未提供应用专用的发送通知宏，或已通过 #define 将该通知宏
 * 定义为空，则提供使用任务通知的默认实现。
 */
    #ifndef sbSEND_COMPLETED
        #define sbSEND_COMPLETED( pxStreamBuffer )                                  \
    vTaskSuspendAll();                                                              \
    {                                                                               \
        if( ( pxStreamBuffer )->xTaskWaitingToReceive != NULL )                     \
        {                                                                           \
            ( void ) xTaskNotifyIndexed( ( pxStreamBuffer )->xTaskWaitingToReceive, \
                                         ( pxStreamBuffer )->uxNotificationIndex,   \
                                         ( uint32_t ) 0,                            \
                                         eNoAction );                               \
            ( pxStreamBuffer )->xTaskWaitingToReceive = NULL;                       \
        }                                                                           \
    }                                                                               \
    ( void ) xTaskResumeAll()
    #endif /* sbSEND_COMPLETED（发送完成） */

/*
 * 如果用户提供了每个实例的发送完成回调，则调用该回调；否则使用为
 * 所有实例默认提供的发送完成宏。
 */
    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
        #define prvSEND_COMPLETED( pxStreamBuffer )                                           \
    do {                                                                                      \
        if( ( pxStreamBuffer )->pxSendCompletedCallback != NULL )                             \
        {                                                                                     \
            ( pxStreamBuffer )->pxSendCompletedCallback( ( pxStreamBuffer ), pdFALSE, NULL ); \
        }                                                                                     \
        else                                                                                  \
        {                                                                                     \
            sbSEND_COMPLETED( ( pxStreamBuffer ) );                                           \
        }                                                                                     \
    } while( 0 )
    #else /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */
        #define prvSEND_COMPLETED( pxStreamBuffer )    sbSEND_COMPLETED( ( pxStreamBuffer ) )
    #endif /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */


    #ifndef sbSEND_COMPLETE_FROM_ISR
        #define sbSEND_COMPLETE_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken )          \
    do {                                                                                       \
        UBaseType_t uxSavedInterruptStatus;                                                    \
                                                                                               \
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();                                \
        {                                                                                      \
            if( ( pxStreamBuffer )->xTaskWaitingToReceive != NULL )                            \
            {                                                                                  \
                ( void ) xTaskNotifyIndexedFromISR( ( pxStreamBuffer )->xTaskWaitingToReceive, \
                                                    ( pxStreamBuffer )->uxNotificationIndex,   \
                                                    ( uint32_t ) 0,                            \
                                                    eNoAction,                                 \
                                                    ( pxHigherPriorityTaskWoken ) );           \
                ( pxStreamBuffer )->xTaskWaitingToReceive = NULL;                              \
            }                                                                                  \
        }                                                                                      \
        taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );                                  \
    } while( 0 )
    #endif /* sbSEND_COMPLETE_FROM_ISR（ISR 中发送完成） */


    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
        #define prvSEND_COMPLETE_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken )                                \
    do {                                                                                                              \
        if( ( pxStreamBuffer )->pxSendCompletedCallback != NULL )                                                     \
        {                                                                                                             \
            ( pxStreamBuffer )->pxSendCompletedCallback( ( pxStreamBuffer ), pdTRUE, ( pxHigherPriorityTaskWoken ) ); \
        }                                                                                                             \
        else                                                                                                          \
        {                                                                                                             \
            sbSEND_COMPLETE_FROM_ISR( ( pxStreamBuffer ), ( pxHigherPriorityTaskWoken ) );                            \
        }                                                                                                             \
    } while( 0 )
    #else /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */
        #define prvSEND_COMPLETE_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken ) \
    sbSEND_COMPLETE_FROM_ISR( ( pxStreamBuffer ), ( pxHigherPriorityTaskWoken ) )
    #endif /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */

/* 缓冲区中用于保存消息长度的字节数。 */
    #define sbBYTES_TO_STORE_MESSAGE_LENGTH    ( sizeof( configMESSAGE_BUFFER_LENGTH_TYPE ) )

/* 存储在流缓冲 ucFlags 字段中的标志位。 */
    #define sbFLAGS_IS_MESSAGE_BUFFER          ( ( uint8_t ) 1 ) /* 若流缓冲被创建为消息缓冲（即保存离散消息而非字节流）则置位。 */
    #define sbFLAGS_IS_STATICALLY_ALLOCATED    ( ( uint8_t ) 2 ) /* 若流缓冲使用静态分配的内存创建则置位。 */
    #define sbFLAGS_IS_BATCHING_BUFFER         ( ( uint8_t ) 4 ) /* 若流缓冲被创建为批量缓冲（即仅当超过触发阈值时接收任务才会解除阻塞）则置位。 */

/*-----------------------------------------------------------*/

/* 保存缓冲区状态信息的结构体。 */
typedef struct StreamBufferDef_t
{
    volatile size_t xTail;                       /* 缓冲区中下一个待读位置的索引。 */
    volatile size_t xHead;                       /* 缓冲区中下一个待写位置的索引。 */
    size_t xLength;                              /* pucBuffer 所指向缓冲区的长度。 */
    size_t xTriggerLevelBytes;                   /* 在等待数据的任务解除阻塞之前，流缓冲中必须存在的字节数。 */
    volatile TaskHandle_t xTaskWaitingToReceive; /* 保存正在等待数据的任务句柄；若无任务等待则为 NULL。 */
    volatile TaskHandle_t xTaskWaitingToSend;    /* 保存因消息缓冲已满而等待发送数据的任务句柄。 */
    uint8_t * pucBuffer;                         /* 指向缓冲区本身，即存储流经缓冲区的数据的 RAM。 */
    uint8_t ucFlags;

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxStreamBufferNumber; /* 用于跟踪功能。 */
    #endif

    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
        StreamBufferCallbackFunction_t pxSendCompletedCallback;    /* 可选的发送完成回调。若为 NULL，则调用 sbSEND_COMPLETED。 */
        StreamBufferCallbackFunction_t pxReceiveCompletedCallback; /* 可选的接收完成回调。若为 NULL，则调用 sbRECEIVE_COMPLETED。 */
    #endif
    UBaseType_t uxNotificationIndex;                               /* 用于通知的索引，默认为 tskDEFAULT_INDEX_TO_NOTIFY。 */
} StreamBuffer_t;

/*
 * 缓冲区中可供读取的字节数。
 */
static size_t prvBytesInBuffer( const StreamBuffer_t * const pxStreamBuffer ) PRIVILEGED_FUNCTION;

/*
 * 当缓冲数据量应使等待接收数据的任务解除阻塞时返回 pdTRUE。流批量
 * 缓冲要求缓冲数据量超过触发阈值，而流缓冲与消息缓冲在达到触发阈值时
 * 即解除阻塞。
 */
static BaseType_t prvBytesInBufferMeetTriggerLevel( const StreamBuffer_t * const pxStreamBuffer,
                                                    size_t xBytesInBuffer ) PRIVILEGED_FUNCTION;

/*
 * 将 pucData 中的 xCount 个字节添加到 pxStreamBuffer 的数据存储区。
 * 本函数不更新缓冲区的 xHead 指针，因此可将多次写操作“原子地”串联起来。
 * 这适用于消息缓冲：消息长度和数据字节分两块写入，且在所有数据复制完成
 * 之前，不希望读取方看到缓冲区已增长。本函数接收自定义的 xHead 值以
 * 指示写入位置（串联操作所需），并返回更新后的 xHead 位置。
 * 若要标记写入完成，请用本函数返回的 xHead 手动设置缓冲区的 xHead 字段。
 */
static size_t prvWriteBytesToBuffer( StreamBuffer_t * const pxStreamBuffer,
                                     const uint8_t * pucData,
                                     size_t xCount,
                                     size_t xHead ) PRIVILEGED_FUNCTION;

/*
 * 若流缓冲用作消息缓冲，则从缓冲区中读取一条完整消息；若用作流缓冲，
 * 则尽可能多地读取字节。实际从缓冲区数据存储区提取字节由
 * prvReadBytesFromBuffer() 完成。
 */
static size_t prvReadMessageFromBuffer( StreamBuffer_t * pxStreamBuffer,
                                        void * pvRxData,
                                        size_t xBufferLengthBytes,
                                        size_t xBytesAvailable ) PRIVILEGED_FUNCTION;

/*
 * 若流缓冲用作消息缓冲，则向缓冲区写入一条完整消息；若用作流缓冲，
 * 则尽可能多地写入字节。实际向缓冲区数据存储区发送字节由
 * prvWriteBytestoBuffer() 完成。
 */
static size_t prvWriteMessageToBuffer( StreamBuffer_t * const pxStreamBuffer,
                                       const void * pvTxData,
                                       size_t xDataLengthBytes,
                                       size_t xSpace,
                                       size_t xRequiredSpace ) PRIVILEGED_FUNCTION;

/*
 * 将 pxStreamBuffer 数据存储区中的 xCount 个字节复制到 pucData。
 * 本函数不更新缓冲区的 xTail 指针，因此可将多次读操作“原子地”串联起来。
 * 这适用于消息缓冲：消息长度和数据字节分两块读取，且在所有数据复制完成
 * 之前，不希望写入方看到缓冲区有更多空闲空间，尤其是在因接收空间不足
 * 而必须中止读取时。本函数接收自定义的 xTail 值以指示读取位置（串联
 * 操作所需），并返回更新后的 xTail 位置。
 * 若要标记读取完成，请用本函数返回的 xTail 手动设置缓冲区的 xTail 字段。
 */
static size_t prvReadBytesFromBuffer( StreamBuffer_t * pxStreamBuffer,
                                      uint8_t * pucData,
                                      size_t xCount,
                                      size_t xTail ) PRIVILEGED_FUNCTION;

/*
 * 由 pxStreamBufferCreate() 和 pxStreamBufferCreateStatic() 共同调用，
 * 用于初始化新建流缓冲结构体的成员。
 */
static void prvInitialiseNewStreamBuffer( StreamBuffer_t * const pxStreamBuffer,
                                          uint8_t * const pucBuffer,
                                          size_t xBufferSizeBytes,
                                          size_t xTriggerLevelBytes,
                                          uint8_t ucFlags,
                                          StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                          StreamBufferCallbackFunction_t pxReceiveCompletedCallback ) PRIVILEGED_FUNCTION;

/*-----------------------------------------------------------*/
    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    StreamBufferHandle_t xStreamBufferGenericCreate( size_t xBufferSizeBytes,
                                                     size_t xTriggerLevelBytes,
                                                     BaseType_t xStreamBufferType,
                                                     StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                                     StreamBufferCallbackFunction_t pxReceiveCompletedCallback )
    {
        void * pvAllocatedMemory;
        uint8_t ucFlags;

        traceENTER_xStreamBufferGenericCreate( xBufferSizeBytes, xTriggerLevelBytes, xStreamBufferType, pxSendCompletedCallback, pxReceiveCompletedCallback );

        /*
         * 若流缓冲将用作消息缓冲（即保存带少量元数据的离散消息，元数据指明
         * 下一条消息的大小），则检查缓冲区是否足够大，至少能容纳一条消息。
         */
        if( xStreamBufferType == sbTYPE_MESSAGE_BUFFER )
        {
            /* 是消息缓冲，但非静态分配。 */
            ucFlags = sbFLAGS_IS_MESSAGE_BUFFER;
            configASSERT( xBufferSizeBytes > sbBYTES_TO_STORE_MESSAGE_LENGTH );
        }
        else if( xStreamBufferType == sbTYPE_STREAM_BATCHING_BUFFER )
        {
            /* 是批量缓冲，但非静态分配。 */
            ucFlags = sbFLAGS_IS_BATCHING_BUFFER;
            configASSERT( xBufferSizeBytes > 0 );
        }
        else
        {
            /* 既非消息缓冲，也非静态分配。 */
            ucFlags = 0;
            configASSERT( xBufferSizeBytes > 0 );
        }

        configASSERT( xTriggerLevelBytes <= xBufferSizeBytes );

        /*
         * 触发级别为 0 会导致即使缓冲区为空，等待任务也会解除阻塞。
         */
        if( xTriggerLevelBytes == ( size_t ) 0 )
        {
            xTriggerLevelBytes = ( size_t ) 1;
        }

        /*
         * 流缓冲需要一个 StreamBuffer_t 结构体和一块缓冲区，二者通过一次
         * pvPortMalloc() 调用分配。StreamBuffer_t 结构体位于已分配内存的起始处，
         * 缓冲区紧随其后。请求的大小会被递增，使返回的空闲空间符合用户预期——
         * 这是实现上的一个特殊处理，否则报告的空闲空间会比逻辑预期少一个字节。
         */
        if( xBufferSizeBytes < ( xBufferSizeBytes + 1U + sizeof( StreamBuffer_t ) ) )
        {
            xBufferSizeBytes++;
            pvAllocatedMemory = pvPortMalloc( xBufferSizeBytes + sizeof( StreamBuffer_t ) );
        }
        else
        {
            pvAllocatedMemory = NULL;
        }

        if( pvAllocatedMemory != NULL )
        {
            /* MISRA 参考 11.5.1 [Malloc 内存分配] */
            /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
            /* coverity[misra_c_2012_rule_11_5_violation]（Coverity 违规抑制） */
            prvInitialiseNewStreamBuffer( ( StreamBuffer_t * ) pvAllocatedMemory,                         /* 已分配内存起始处的结构体。 */
                                                                                                          /* MISRA 参考 11.5.1 [Malloc 内存分配] */
                                                                                                          /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
                                                                                                          /* coverity[misra_c_2012_rule_11_5_violation]（Coverity 违规抑制） */
                                          ( ( uint8_t * ) pvAllocatedMemory ) + sizeof( StreamBuffer_t ), /* 存储区紧随其后。 */
                                          xBufferSizeBytes,
                                          xTriggerLevelBytes,
                                          ucFlags,
                                          pxSendCompletedCallback,
                                          pxReceiveCompletedCallback );

            traceSTREAM_BUFFER_CREATE( ( ( StreamBuffer_t * ) pvAllocatedMemory ), xStreamBufferType );
        }
        else
        {
            traceSTREAM_BUFFER_CREATE_FAILED( xStreamBufferType );
        }

        traceRETURN_xStreamBufferGenericCreate( pvAllocatedMemory );

        /* MISRA 参考 11.5.1 [Malloc 内存分配] */
        /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation]（Coverity 违规抑制） */
        return ( StreamBufferHandle_t ) pvAllocatedMemory;
    }
    #endif /* configSUPPORT_DYNAMIC_ALLOCATION：支持动态分配 */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

    StreamBufferHandle_t xStreamBufferGenericCreateStatic( size_t xBufferSizeBytes,
                                                           size_t xTriggerLevelBytes,
                                                           BaseType_t xStreamBufferType,
                                                           uint8_t * const pucStreamBufferStorageArea,
                                                           StaticStreamBuffer_t * const pxStaticStreamBuffer,
                                                           StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                                           StreamBufferCallbackFunction_t pxReceiveCompletedCallback )
    {
        /* MISRA 参考 11.3.1 [非对齐访问] */
        /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
        /* coverity[misra_c_2012_rule_11_3_violation]（Coverity 违规抑制） */
        StreamBuffer_t * const pxStreamBuffer = ( StreamBuffer_t * ) pxStaticStreamBuffer;
        StreamBufferHandle_t xReturn;
        uint8_t ucFlags;

        traceENTER_xStreamBufferGenericCreateStatic( xBufferSizeBytes, xTriggerLevelBytes, xStreamBufferType, pucStreamBufferStorageArea, pxStaticStreamBuffer, pxSendCompletedCallback, pxReceiveCompletedCallback );

        configASSERT( pucStreamBufferStorageArea );
        configASSERT( pxStaticStreamBuffer );
        configASSERT( xTriggerLevelBytes <= xBufferSizeBytes );

        /*
         * 触发级别为 0 会导致即使缓冲区为空，等待任务也会解除阻塞。
         */
        if( xTriggerLevelBytes == ( size_t ) 0 )
        {
            xTriggerLevelBytes = ( size_t ) 1;
        }

        /*
         * 若流缓冲将用作消息缓冲（即保存带少量元数据的离散消息，元数据指明
         * 下一条消息的大小），则检查缓冲区是否足够大，至少能容纳一条消息。
         */

        if( xStreamBufferType == sbTYPE_MESSAGE_BUFFER )
        {
            /* 静态分配的消息缓冲。 */
            ucFlags = sbFLAGS_IS_MESSAGE_BUFFER | sbFLAGS_IS_STATICALLY_ALLOCATED;
            configASSERT( xBufferSizeBytes > sbBYTES_TO_STORE_MESSAGE_LENGTH );
        }
        else if( xStreamBufferType == sbTYPE_STREAM_BATCHING_BUFFER )
        {
            /* 静态分配的批量缓冲。 */
            ucFlags = sbFLAGS_IS_BATCHING_BUFFER | sbFLAGS_IS_STATICALLY_ALLOCATED;
            configASSERT( xBufferSizeBytes > 0 );
        }
        else
        {
            /* 静态分配的流缓冲。 */
            ucFlags = sbFLAGS_IS_STATICALLY_ALLOCATED;
        }

        #if ( configASSERT_DEFINED == 1 )
        {
            /*
             * 检查用于声明 StaticStreamBuffer_t 类型变量的结构体大小是否等于
             * 实际消息缓冲结构体的大小。
             */
            volatile size_t xSize = sizeof( StaticStreamBuffer_t );
            configASSERT( xSize == sizeof( StreamBuffer_t ) );
        }
        #endif /* configASSERT_DEFINED：configASSERT 已定义 */

        if( ( pucStreamBufferStorageArea != NULL ) && ( pxStaticStreamBuffer != NULL ) )
        {
            prvInitialiseNewStreamBuffer( pxStreamBuffer,
                                          pucStreamBufferStorageArea,
                                          xBufferSizeBytes,
                                          xTriggerLevelBytes,
                                          ucFlags,
                                          pxSendCompletedCallback,
                                          pxReceiveCompletedCallback );

            /*
             * 记录该缓冲区为静态分配，以便将来再次删除时使用。
             */
            pxStreamBuffer->ucFlags |= sbFLAGS_IS_STATICALLY_ALLOCATED;

            traceSTREAM_BUFFER_CREATE( pxStreamBuffer, xStreamBufferType );

            /* MISRA 参考 11.3.1 [非对齐访问] */
            /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
            /* coverity[misra_c_2012_rule_11_3_violation]（Coverity 违规抑制） */
            xReturn = ( StreamBufferHandle_t ) pxStaticStreamBuffer;
        }
        else
        {
            xReturn = NULL;
            traceSTREAM_BUFFER_CREATE_STATIC_FAILED( xReturn, xStreamBufferType );
        }

        traceRETURN_xStreamBufferGenericCreateStatic( xReturn );

        return xReturn;
    }
    #endif /* ( configSUPPORT_STATIC_ALLOCATION == 1 )：支持静态分配 */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xStreamBufferGetStaticBuffers( StreamBufferHandle_t xStreamBuffer,
                                              uint8_t ** ppucStreamBufferStorageArea,
                                              StaticStreamBuffer_t ** ppxStaticStreamBuffer )
    {
        BaseType_t xReturn;
        StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;

        traceENTER_xStreamBufferGetStaticBuffers( xStreamBuffer, ppucStreamBufferStorageArea, ppxStaticStreamBuffer );

        configASSERT( pxStreamBuffer );
        configASSERT( ppucStreamBufferStorageArea );
        configASSERT( ppxStaticStreamBuffer );

        if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_STATICALLY_ALLOCATED ) != ( uint8_t ) 0 )
        {
            *ppucStreamBufferStorageArea = pxStreamBuffer->pucBuffer;
            /* MISRA 参考 11.3.1 [非对齐访问] */
            /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-113 */
            /* coverity[misra_c_2012_rule_11_3_violation]（Coverity 违规抑制） */
            *ppxStaticStreamBuffer = ( StaticStreamBuffer_t * ) pxStreamBuffer;
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }

        traceRETURN_xStreamBufferGetStaticBuffers( xReturn );

        return xReturn;
    }
    #endif /* configSUPPORT_STATIC_ALLOCATION：支持静态分配 */
/*-----------------------------------------------------------*/

void vStreamBufferDelete( StreamBufferHandle_t xStreamBuffer )
{
    StreamBuffer_t * pxStreamBuffer = xStreamBuffer;

    traceENTER_vStreamBufferDelete( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    traceSTREAM_BUFFER_DELETE( xStreamBuffer );

    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_STATICALLY_ALLOCATED ) == ( uint8_t ) pdFALSE )
    {
        #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
        {
            /*
             * 结构体和缓冲区通过一次 pvPortMalloc() 调用分配，因此只需调用一次
             * vPortFree()。
             */
            vPortFree( ( void * ) pxStreamBuffer );
        }
        #else
        {
            /*
             * 不应执行到此处，说明 ucFlags 已损坏。强制触发断言。
             */
            configASSERT( xStreamBuffer == ( StreamBufferHandle_t ) ~0 );
        }
        #endif
    }
    else
    {
        /*
         * 结构体和缓冲区并非动态分配，无法释放——仅将结构体清除，以便后续
         * 使用时触发断言。
         */
        ( void ) memset( pxStreamBuffer, 0x00, sizeof( StreamBuffer_t ) );
    }

    traceRETURN_vStreamBufferDelete();
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferReset( StreamBufferHandle_t xStreamBuffer )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    BaseType_t xReturn = pdFAIL;
    StreamBufferCallbackFunction_t pxSendCallback = NULL, pxReceiveCallback = NULL;

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxStreamBufferNumber;
    #endif

    traceENTER_xStreamBufferReset( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    #if ( configUSE_TRACE_FACILITY == 1 )
    {
        /*
         * 保存流缓冲编号，以便复位后恢复。
         */
        uxStreamBufferNumber = pxStreamBuffer->uxStreamBufferNumber;
    }
    #endif

    /* 只有在没有任务阻塞于消息缓冲时才能复位该缓冲。 */
    taskENTER_CRITICAL();
    {
        if( ( pxStreamBuffer->xTaskWaitingToReceive == NULL ) && ( pxStreamBuffer->xTaskWaitingToSend == NULL ) )
        {
            #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
            {
                pxSendCallback = pxStreamBuffer->pxSendCompletedCallback;
                pxReceiveCallback = pxStreamBuffer->pxReceiveCompletedCallback;
            }
            #endif

            prvInitialiseNewStreamBuffer( pxStreamBuffer,
                                          pxStreamBuffer->pucBuffer,
                                          pxStreamBuffer->xLength,
                                          pxStreamBuffer->xTriggerLevelBytes,
                                          pxStreamBuffer->ucFlags,
                                          pxSendCallback,
                                          pxReceiveCallback );

            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                pxStreamBuffer->uxStreamBufferNumber = uxStreamBufferNumber;
            }
            #endif

            traceSTREAM_BUFFER_RESET( xStreamBuffer );

            xReturn = pdPASS;
        }
    }
    taskEXIT_CRITICAL();

    traceRETURN_xStreamBufferReset( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferResetFromISR( StreamBufferHandle_t xStreamBuffer )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    BaseType_t xReturn = pdFAIL;
    StreamBufferCallbackFunction_t pxSendCallback = NULL, pxReceiveCallback = NULL;
    UBaseType_t uxSavedInterruptStatus;

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxStreamBufferNumber;
    #endif

    traceENTER_xStreamBufferResetFromISR( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    #if ( configUSE_TRACE_FACILITY == 1 )
    {
        /*
         * 保存流缓冲编号，以便复位后恢复。
         */
        uxStreamBufferNumber = pxStreamBuffer->uxStreamBufferNumber;
    }
    #endif

    /* 只有在没有任务阻塞于消息缓冲时才能复位该缓冲。 */
    /* MISRA 参考 4.7.1 [返回值必须检查] */
    /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 违规抑制） */
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    {
        if( ( pxStreamBuffer->xTaskWaitingToReceive == NULL ) && ( pxStreamBuffer->xTaskWaitingToSend == NULL ) )
        {
            #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
            {
                pxSendCallback = pxStreamBuffer->pxSendCompletedCallback;
                pxReceiveCallback = pxStreamBuffer->pxReceiveCompletedCallback;
            }
            #endif

            prvInitialiseNewStreamBuffer( pxStreamBuffer,
                                          pxStreamBuffer->pucBuffer,
                                          pxStreamBuffer->xLength,
                                          pxStreamBuffer->xTriggerLevelBytes,
                                          pxStreamBuffer->ucFlags,
                                          pxSendCallback,
                                          pxReceiveCallback );

            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                pxStreamBuffer->uxStreamBufferNumber = uxStreamBufferNumber;
            }
            #endif

            traceSTREAM_BUFFER_RESET_FROM_ISR( xStreamBuffer );

            xReturn = pdPASS;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xStreamBufferResetFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferSetTriggerLevel( StreamBufferHandle_t xStreamBuffer,
                                         size_t xTriggerLevel )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    BaseType_t xReturn;

    traceENTER_xStreamBufferSetTriggerLevel( xStreamBuffer, xTriggerLevel );

    configASSERT( pxStreamBuffer );

    /* 触发级别为 0 是无效的。 */
    if( xTriggerLevel == ( size_t ) 0 )
    {
        xTriggerLevel = ( size_t ) 1;
    }

    /*
     * 触发级别是指等待数据的任务解除阻塞之前，流缓冲中必须存在的字节数。
     */
    if( xTriggerLevel < pxStreamBuffer->xLength )
    {
        pxStreamBuffer->xTriggerLevelBytes = xTriggerLevel;
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFALSE;
    }

    traceRETURN_xStreamBufferSetTriggerLevel( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferSpacesAvailable( StreamBufferHandle_t xStreamBuffer )
{
    const StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xSpace;
    size_t xOriginalTail;

    traceENTER_xStreamBufferSpacesAvailable( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    /*
     * 下面的代码先读取 xTail，再读取 xHead。若流缓冲在两次读取之间仅更新
     * 一次，则此操作是安全的；若更新超过一次则不安全——因此使用循环。
     */
    do
    {
        xOriginalTail = pxStreamBuffer->xTail;
        xSpace = pxStreamBuffer->xLength + pxStreamBuffer->xTail;
        xSpace -= pxStreamBuffer->xHead;
    } while( xOriginalTail != pxStreamBuffer->xTail );

    xSpace -= ( size_t ) 1;

    if( xSpace >= pxStreamBuffer->xLength )
    {
        xSpace -= pxStreamBuffer->xLength;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    traceRETURN_xStreamBufferSpacesAvailable( xSpace );

    return xSpace;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferBytesAvailable( StreamBufferHandle_t xStreamBuffer )
{
    const StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReturn;

    traceENTER_xStreamBufferBytesAvailable( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    xReturn = prvBytesInBuffer( pxStreamBuffer );

    traceRETURN_xStreamBufferBytesAvailable( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
                          const void * pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReturn, xSpace = 0;
    size_t xRequiredSpace = xDataLengthBytes;
    TimeOut_t xTimeOut;
    size_t xMaxReportedSpace = 0;

    traceENTER_xStreamBufferSend( xStreamBuffer, pvTxData, xDataLengthBytes, xTicksToWait );

    configASSERT( pvTxData );
    configASSERT( pxStreamBuffer );

    /*
     * 流缓冲能够报告的最大空间为其长度减 1。
     */
    xMaxReportedSpace = pxStreamBuffer->xLength - ( size_t ) 1;

    /*
     * 本发送函数同时用于消息缓冲和流缓冲。若为消息缓冲，则所需空间还必须
     * 加上保存消息长度所需的字节数。
     */
    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        xRequiredSpace += sbBYTES_TO_STORE_MESSAGE_LENGTH;

        /* 是否溢出？ */
        configASSERT( xRequiredSpace > xDataLengthBytes );

        /*
         * 若为消息缓冲，则必须能够写入整条消息。
         */
        if( xRequiredSpace > xMaxReportedSpace )
        {
            /*
             * 即使整个缓冲区为空也放不下该消息，因此不等待空间。
             */
            xTicksToWait = ( TickType_t ) 0;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        /*
         * 若为流缓冲，则允许只将消息的一部分写入缓冲区。将长度限制为
         * 缓冲区的总长度。
         */
        if( xRequiredSpace > xMaxReportedSpace )
        {
            xRequiredSpace = xMaxReportedSpace;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }

    if( xTicksToWait != ( TickType_t ) 0 )
    {
        vTaskSetTimeOutState( &xTimeOut );

        do
        {
            /*
             * 等待消息缓冲中出现所需数量的空闲字节。
             */
            taskENTER_CRITICAL();
            {
                xSpace = xStreamBufferSpacesAvailable( pxStreamBuffer );

                if( xSpace < xRequiredSpace )
                {
                    /* 即将等待空间，清除通知状态。 */
                    ( void ) xTaskNotifyStateClearIndexed( NULL, pxStreamBuffer->uxNotificationIndex );

                    /* 应当只有一个写入方。 */
                    configASSERT( pxStreamBuffer->xTaskWaitingToSend == NULL );
                    pxStreamBuffer->xTaskWaitingToSend = xTaskGetCurrentTaskHandle();
                }
                else
                {
                    taskEXIT_CRITICAL();
                    break;
                }
            }
            taskEXIT_CRITICAL();

            traceBLOCKING_ON_STREAM_BUFFER_SEND( xStreamBuffer );
            ( void ) xTaskNotifyWaitIndexed( pxStreamBuffer->uxNotificationIndex, ( uint32_t ) 0, ( uint32_t ) 0, NULL, xTicksToWait );
            pxStreamBuffer->xTaskWaitingToSend = NULL;
        } while( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    if( xSpace == ( size_t ) 0 )
    {
        xSpace = xStreamBufferSpacesAvailable( pxStreamBuffer );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    xReturn = prvWriteMessageToBuffer( pxStreamBuffer, pvTxData, xDataLengthBytes, xSpace, xRequiredSpace );

    if( xReturn > ( size_t ) 0 )
    {
        traceSTREAM_BUFFER_SEND( xStreamBuffer, xReturn );

        /* 是否有任务在等待数据？ */
        if( prvBytesInBufferMeetTriggerLevel( pxStreamBuffer, prvBytesInBuffer( pxStreamBuffer ) ) != pdFALSE )
        {
            prvSEND_COMPLETED( pxStreamBuffer );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
        traceSTREAM_BUFFER_SEND_FAILED( xStreamBuffer );
    }

    traceRETURN_xStreamBufferSend( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferSendFromISR( StreamBufferHandle_t xStreamBuffer,
                                 const void * pvTxData,
                                 size_t xDataLengthBytes,
                                 BaseType_t * const pxHigherPriorityTaskWoken )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReturn, xSpace;
    size_t xRequiredSpace = xDataLengthBytes;

    traceENTER_xStreamBufferSendFromISR( xStreamBuffer, pvTxData, xDataLengthBytes, pxHigherPriorityTaskWoken );

    configASSERT( pvTxData );
    configASSERT( pxStreamBuffer );

    /*
     * 本发送函数同时用于消息缓冲和流缓冲。若为消息缓冲，则所需空间还必须
     * 加上保存消息长度所需的字节数。
     */
    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        xRequiredSpace += sbBYTES_TO_STORE_MESSAGE_LENGTH;

        /* 是否溢出？ */
        configASSERT( xRequiredSpace > xDataLengthBytes );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    xSpace = xStreamBufferSpacesAvailable( pxStreamBuffer );
    xReturn = prvWriteMessageToBuffer( pxStreamBuffer, pvTxData, xDataLengthBytes, xSpace, xRequiredSpace );

    if( xReturn > ( size_t ) 0 )
    {
        /* 是否有任务在等待数据？ */
        if( prvBytesInBufferMeetTriggerLevel( pxStreamBuffer, prvBytesInBuffer( pxStreamBuffer ) ) != pdFALSE )
        {
            /* MISRA 参考 4.7.1 [返回值必须检查] */
            /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
            /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 违规抑制） */
            prvSEND_COMPLETE_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken );
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

    traceSTREAM_BUFFER_SEND_FROM_ISR( xStreamBuffer, xReturn );
    traceRETURN_xStreamBufferSendFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

static size_t prvWriteMessageToBuffer( StreamBuffer_t * const pxStreamBuffer,
                                       const void * pvTxData,
                                       size_t xDataLengthBytes,
                                       size_t xSpace,
                                       size_t xRequiredSpace )
{
    size_t xNextHead = pxStreamBuffer->xHead;
    configMESSAGE_BUFFER_LENGTH_TYPE xMessageLength;

    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        /* 这是消息缓冲，而非流缓冲。 */

        /* 将 xDataLengthBytes 转换为消息长度类型。 */
        xMessageLength = ( configMESSAGE_BUFFER_LENGTH_TYPE ) xDataLengthBytes;

        /* 确保给定的数据长度在 configMESSAGE_BUFFER_LENGTH_TYPE 范围内。 */
        configASSERT( ( size_t ) xMessageLength == xDataLengthBytes );

        if( xSpace >= xRequiredSpace )
        {
            /*
             * 缓冲区中有足够空间同时写入消息长度和消息本身。先写入数据长度，
             * 数据本身将在本函数稍后写入。
             */
            xNextHead = prvWriteBytesToBuffer( pxStreamBuffer, ( const uint8_t * ) &( xMessageLength ), sbBYTES_TO_STORE_MESSAGE_LENGTH, xNextHead );
        }
        else
        {
            /* 空间不足，因此不向缓冲区写入数据。 */
            xDataLengthBytes = 0;
        }
    }
    else
    {
        /*
         * 这是流缓冲而非消息缓冲，因此写入的是字节流而非离散消息。
         * 计划尽可能多地写入字节。
         */
        xDataLengthBytes = configMIN( xDataLengthBytes, xSpace );
    }

    if( xDataLengthBytes != ( size_t ) 0 )
    {
        /* 将数据写入缓冲区。 */
        /* MISRA 参考 11.5.5 [void 指针赋值] */
        /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation]（Coverity 违规抑制） */
        pxStreamBuffer->xHead = prvWriteBytesToBuffer( pxStreamBuffer, ( const uint8_t * ) pvTxData, xDataLengthBytes, xNextHead );
    }

    return xDataLengthBytes;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
                             void * pvRxData,
                             size_t xBufferLengthBytes,
                             TickType_t xTicksToWait )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReceivedLength = 0, xBytesAvailable, xBytesToStoreMessageLength;

    traceENTER_xStreamBufferReceive( xStreamBuffer, pvRxData, xBufferLengthBytes, xTicksToWait );

    configASSERT( pvRxData );
    configASSERT( pxStreamBuffer );

    /*
     * 本接收函数供消息缓冲（保存离散消息）和流缓冲（保存连续字节流）
     * 共同使用。离散消息额外包含 sbBYTES_TO_STORE_MESSAGE_LENGTH 个字节，
     * 用于保存消息长度。
     */
    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        xBytesToStoreMessageLength = sbBYTES_TO_STORE_MESSAGE_LENGTH;
    }
    else if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_BATCHING_BUFFER ) != ( uint8_t ) 0 )
    {
        /*
         * 若批量缓冲中的字节数少于触发级别，则强制任务阻塞。
         */
        xBytesToStoreMessageLength = pxStreamBuffer->xTriggerLevelBytes;
    }
    else
    {
        xBytesToStoreMessageLength = 0;
    }

    if( xTicksToWait != ( TickType_t ) 0 )
    {
        /*
         * 检查是否有数据与清除通知状态必须原子地完成。
         */
        taskENTER_CRITICAL();
        {
            xBytesAvailable = prvBytesInBuffer( pxStreamBuffer );

            /*
             * 若本函数由消息缓冲读取调用，则 xBytesToStoreMessageLength 保存用于
             * 保存下一条离散消息长度的字节数。若本函数由流缓冲读取调用，则
             * xBytesToStoreMessageLength 为 0。若本函数由流批量缓冲读取调用，则
             * xBytesToStoreMessageLength 为缓冲区的 xTriggerLevelBytes 值。
             */
            if( xBytesAvailable <= xBytesToStoreMessageLength )
            {
                /* 即将等待数据，清除通知状态。 */
                ( void ) xTaskNotifyStateClearIndexed( NULL, pxStreamBuffer->uxNotificationIndex );

                /* 应当只有一个读取方。 */
                configASSERT( pxStreamBuffer->xTaskWaitingToReceive == NULL );
                pxStreamBuffer->xTaskWaitingToReceive = xTaskGetCurrentTaskHandle();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        taskEXIT_CRITICAL();

        if( xBytesAvailable <= xBytesToStoreMessageLength )
        {
            /* 等待数据可用。 */
            traceBLOCKING_ON_STREAM_BUFFER_RECEIVE( xStreamBuffer );
            ( void ) xTaskNotifyWaitIndexed( pxStreamBuffer->uxNotificationIndex, ( uint32_t ) 0, ( uint32_t ) 0, NULL, xTicksToWait );
            pxStreamBuffer->xTaskWaitingToReceive = NULL;

            /* 阻塞结束后重新检查可用数据。 */
            xBytesAvailable = prvBytesInBuffer( pxStreamBuffer );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        xBytesAvailable = prvBytesInBuffer( pxStreamBuffer );
    }

    /*
     * 无论是接收离散消息（xBytesToStoreMessageLength 保存用于存储消息长度
     * 的字节数）还是字节流（xBytesToStoreMessageLength 为零），可用字节数
     * 都必须大于 xBytesToStoreMessageLength，才能从缓冲区读取字节。
     */
    if( xBytesAvailable > xBytesToStoreMessageLength )
    {
        xReceivedLength = prvReadMessageFromBuffer( pxStreamBuffer, pvRxData, xBufferLengthBytes, xBytesAvailable );

        /* 是否有任务在等待缓冲区空间？ */
        if( xReceivedLength != ( size_t ) 0 )
        {
            traceSTREAM_BUFFER_RECEIVE( xStreamBuffer, xReceivedLength );
            prvRECEIVE_COMPLETED( xStreamBuffer );
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        traceSTREAM_BUFFER_RECEIVE_FAILED( xStreamBuffer );
        mtCOVERAGE_TEST_MARKER();
    }

    traceRETURN_xStreamBufferReceive( xReceivedLength );

    return xReceivedLength;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferNextMessageLengthBytes( StreamBufferHandle_t xStreamBuffer )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReturn, xBytesAvailable;
    configMESSAGE_BUFFER_LENGTH_TYPE xTempReturn;

    traceENTER_xStreamBufferNextMessageLengthBytes( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    /* 确保该流缓冲正被用作消息缓冲。 */
    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        xBytesAvailable = prvBytesInBuffer( pxStreamBuffer );

        if( xBytesAvailable > sbBYTES_TO_STORE_MESSAGE_LENGTH )
        {
            /*
             * 可用字节数大于保存下一条消息长度所需的字节数，因此还有一条消息
             * 可供读取。
             */
            ( void ) prvReadBytesFromBuffer( pxStreamBuffer, ( uint8_t * ) &xTempReturn, sbBYTES_TO_STORE_MESSAGE_LENGTH, pxStreamBuffer->xTail );
            xReturn = ( size_t ) xTempReturn;
        }
        else
        {
            /*
             * 消息缓冲中的最小字节数为 ( sbBYTES_TO_STORE_MESSAGE_LENGTH + 1 )，
             * 因此若 xBytesAvailable 小于 sbBYTES_TO_STORE_MESSAGE_LENGTH，则唯一
             * 其他有效值就是 0。
             */
            configASSERT( xBytesAvailable == 0 );
            xReturn = 0;
        }
    }
    else
    {
        xReturn = 0;
    }

    traceRETURN_xStreamBufferNextMessageLengthBytes( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

size_t xStreamBufferReceiveFromISR( StreamBufferHandle_t xStreamBuffer,
                                    void * pvRxData,
                                    size_t xBufferLengthBytes,
                                    BaseType_t * const pxHigherPriorityTaskWoken )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReceivedLength = 0, xBytesAvailable, xBytesToStoreMessageLength;

    traceENTER_xStreamBufferReceiveFromISR( xStreamBuffer, pvRxData, xBufferLengthBytes, pxHigherPriorityTaskWoken );

    configASSERT( pvRxData );
    configASSERT( pxStreamBuffer );

    /*
     * 本接收函数供消息缓冲（保存离散消息）和流缓冲（保存连续字节流）
     * 共同使用。离散消息额外包含 sbBYTES_TO_STORE_MESSAGE_LENGTH 个字节，
     * 用于保存消息长度。
     */
    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        xBytesToStoreMessageLength = sbBYTES_TO_STORE_MESSAGE_LENGTH;
    }
    else
    {
        xBytesToStoreMessageLength = 0;
    }

    xBytesAvailable = prvBytesInBuffer( pxStreamBuffer );

    /*
     * 无论是接收离散消息（xBytesToStoreMessageLength 保存用于存储消息长度
     * 的字节数）还是字节流（xBytesToStoreMessageLength 为零），可用字节数
     * 都必须大于 xBytesToStoreMessageLength，才能从缓冲区读取字节。
     */
    if( xBytesAvailable > xBytesToStoreMessageLength )
    {
        xReceivedLength = prvReadMessageFromBuffer( pxStreamBuffer, pvRxData, xBufferLengthBytes, xBytesAvailable );

        /* 是否有任务在等待缓冲区空间？ */
        if( xReceivedLength != ( size_t ) 0 )
        {
            /* MISRA 参考 4.7.1 [返回值必须检查] */
            /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
            /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 违规抑制） */
            prvRECEIVE_COMPLETED_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken );
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

    traceSTREAM_BUFFER_RECEIVE_FROM_ISR( xStreamBuffer, xReceivedLength );
    traceRETURN_xStreamBufferReceiveFromISR( xReceivedLength );

    return xReceivedLength;
}
/*-----------------------------------------------------------*/

static size_t prvReadMessageFromBuffer( StreamBuffer_t * pxStreamBuffer,
                                        void * pvRxData,
                                        size_t xBufferLengthBytes,
                                        size_t xBytesAvailable )
{
    size_t xCount, xNextMessageLength;
    configMESSAGE_BUFFER_LENGTH_TYPE xTempNextMessageLength;
    size_t xNextTail = pxStreamBuffer->xTail;

    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        /*
         * 正在接收一条离散消息。首先读取该消息的长度。
         */
        xNextTail = prvReadBytesFromBuffer( pxStreamBuffer, ( uint8_t * ) &xTempNextMessageLength, sbBYTES_TO_STORE_MESSAGE_LENGTH, xNextTail );
        xNextMessageLength = ( size_t ) xTempNextMessageLength;

        /*
         * 从可用字节数中减去刚刚读出的字节数。
         */
        xBytesAvailable -= sbBYTES_TO_STORE_MESSAGE_LENGTH;

        /*
         * 检查用户提供的缓冲区中是否有足够空间。
         */
        if( xNextMessageLength > xBufferLengthBytes )
        {
            /* 用户提供的空间不足以读取该消息。 */
            xNextMessageLength = 0;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        /*
         * 正在接收字节流（而非离散消息），因此尽可能多地读取字节。
         */
        xNextMessageLength = xBufferLengthBytes;
    }

    /* 取所需字节数与可用字节数中的较小值。 */
    xCount = configMIN( xNextMessageLength, xBytesAvailable );

    if( xCount != ( size_t ) 0 )
    {
        /* 读取实际数据并更新 tail，将该数据标记为已正式消费。 */
        /* MISRA 参考 11.5.5 [void 指针赋值] */
        /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-115 */
        /* coverity[misra_c_2012_rule_11_5_violation]（Coverity 违规抑制） */
        pxStreamBuffer->xTail = prvReadBytesFromBuffer( pxStreamBuffer, ( uint8_t * ) pvRxData, xCount, xNextTail );
    }

    return xCount;
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferIsEmpty( StreamBufferHandle_t xStreamBuffer )
{
    const StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    BaseType_t xReturn;
    size_t xTail;

    traceENTER_xStreamBufferIsEmpty( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    /* 若无可用字节则为真。 */
    xTail = pxStreamBuffer->xTail;

    if( pxStreamBuffer->xHead == xTail )
    {
        xReturn = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }

    traceRETURN_xStreamBufferIsEmpty( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferIsFull( StreamBufferHandle_t xStreamBuffer )
{
    BaseType_t xReturn;
    size_t xBytesToStoreMessageLength;
    const StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;

    traceENTER_xStreamBufferIsFull( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    /*
     * 本通用接收函数供消息缓冲（保存离散消息）和流缓冲（保存连续字节流）
     * 共同使用。离散消息额外包含 sbBYTES_TO_STORE_MESSAGE_LENGTH 个字节，
     * 用于保存消息长度。
     */
    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) != ( uint8_t ) 0 )
    {
        xBytesToStoreMessageLength = sbBYTES_TO_STORE_MESSAGE_LENGTH;
    }
    else
    {
        xBytesToStoreMessageLength = 0;
    }

    /* 若可用空间等于零则为真。 */
    if( xStreamBufferSpacesAvailable( xStreamBuffer ) <= xBytesToStoreMessageLength )
    {
        xReturn = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }

    traceRETURN_xStreamBufferIsFull( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferSendCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                              BaseType_t * pxHigherPriorityTaskWoken )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;

    traceENTER_xStreamBufferSendCompletedFromISR( xStreamBuffer, pxHigherPriorityTaskWoken );

    configASSERT( pxStreamBuffer );

    /* MISRA 参考 4.7.1 [返回值必须检查] */
    /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 违规抑制） */
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    {
        if( ( pxStreamBuffer )->xTaskWaitingToReceive != NULL )
        {
            ( void ) xTaskNotifyIndexedFromISR( ( pxStreamBuffer )->xTaskWaitingToReceive,
                                                ( pxStreamBuffer )->uxNotificationIndex,
                                                ( uint32_t ) 0,
                                                eNoAction,
                                                pxHigherPriorityTaskWoken );
            ( pxStreamBuffer )->xTaskWaitingToReceive = NULL;
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xStreamBufferSendCompletedFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xStreamBufferReceiveCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                                 BaseType_t * pxHigherPriorityTaskWoken )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    BaseType_t xReturn;
    UBaseType_t uxSavedInterruptStatus;

    traceENTER_xStreamBufferReceiveCompletedFromISR( xStreamBuffer, pxHigherPriorityTaskWoken );

    configASSERT( pxStreamBuffer );

    /* MISRA 参考 4.7.1 [返回值必须检查] */
    /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#dir-47 */
    /* coverity[misra_c_2012_directive_4_7_violation]（Coverity 违规抑制） */
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    {
        if( ( pxStreamBuffer )->xTaskWaitingToSend != NULL )
        {
            ( void ) xTaskNotifyIndexedFromISR( ( pxStreamBuffer )->xTaskWaitingToSend,
                                                ( pxStreamBuffer )->uxNotificationIndex,
                                                ( uint32_t ) 0,
                                                eNoAction,
                                                pxHigherPriorityTaskWoken );
            ( pxStreamBuffer )->xTaskWaitingToSend = NULL;
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    taskEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );

    traceRETURN_xStreamBufferReceiveCompletedFromISR( xReturn );

    return xReturn;
}
/*-----------------------------------------------------------*/

static size_t prvWriteBytesToBuffer( StreamBuffer_t * const pxStreamBuffer,
                                     const uint8_t * pucData,
                                     size_t xCount,
                                     size_t xHead )
{
    size_t xFirstLength;

    configASSERT( xCount > ( size_t ) 0 );

    /*
     * 计算首次写入可添加的字节数——若缓冲区需要回绕到起始位置，该值可能
     * 小于总共需要添加的字节数。
     */
    xFirstLength = configMIN( pxStreamBuffer->xLength - xHead, xCount );

    /* 在首次写入中写入尽可能多的字节。 */
    configASSERT( ( xHead + xFirstLength ) <= pxStreamBuffer->xLength );
    ( void ) memcpy( ( void * ) ( &( pxStreamBuffer->pucBuffer[ xHead ] ) ), ( const void * ) pucData, xFirstLength );

    /*
     * 若已写入的字节数少于首次写入本可写入的字节数……
     */
    if( xCount > xFirstLength )
    {
        /* ……则将剩余字节写入缓冲区起始处。 */
        configASSERT( ( xCount - xFirstLength ) <= pxStreamBuffer->xLength );
        ( void ) memcpy( ( void * ) pxStreamBuffer->pucBuffer, ( const void * ) &( pucData[ xFirstLength ] ), xCount - xFirstLength );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    xHead += xCount;

    if( xHead >= pxStreamBuffer->xLength )
    {
        xHead -= pxStreamBuffer->xLength;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    return xHead;
}
/*-----------------------------------------------------------*/

static size_t prvReadBytesFromBuffer( StreamBuffer_t * pxStreamBuffer,
                                      uint8_t * pucData,
                                      size_t xCount,
                                      size_t xTail )
{
    size_t xFirstLength;

    configASSERT( xCount != ( size_t ) 0 );

    /*
     * 计算可读取的字节数——若数据回绕到缓冲区起始处，该值可能小于
     * 所需的字节数。
     */
    xFirstLength = configMIN( pxStreamBuffer->xLength - xTail, xCount );

    /*
     * 获取首次读取能够获得的字节数。断言用于检查读写的边界。
     */
    configASSERT( xFirstLength <= xCount );
    configASSERT( ( xTail + xFirstLength ) <= pxStreamBuffer->xLength );
    ( void ) memcpy( ( void * ) pucData, ( const void * ) &( pxStreamBuffer->pucBuffer[ xTail ] ), xFirstLength );

    /*
     * 若所需的总字节数大于首次读取本可读取的字节数……
     */
    if( xCount > xFirstLength )
    {
        /* ……则从缓冲区起始处读取剩余字节。 */
        ( void ) memcpy( ( void * ) &( pucData[ xFirstLength ] ), ( void * ) ( pxStreamBuffer->pucBuffer ), xCount - xFirstLength );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* 移动 tail 指针，从而有效移除已从缓冲区读取的数据。 */
    xTail += xCount;

    if( xTail >= pxStreamBuffer->xLength )
    {
        xTail -= pxStreamBuffer->xLength;
    }

    return xTail;
}
/*-----------------------------------------------------------*/

static size_t prvBytesInBuffer( const StreamBuffer_t * const pxStreamBuffer )
{
    /* 返回 xTail 与 xHead 之间的距离。 */
    size_t xCount;

    xCount = pxStreamBuffer->xLength + pxStreamBuffer->xHead;
    xCount -= pxStreamBuffer->xTail;

    if( xCount >= pxStreamBuffer->xLength )
    {
        xCount -= pxStreamBuffer->xLength;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    return xCount;
}
/*-----------------------------------------------------------*/

static BaseType_t prvBytesInBufferMeetTriggerLevel( const StreamBuffer_t * const pxStreamBuffer,
                                                    size_t xBytesInBuffer )
{
    BaseType_t xReturn = pdFALSE;

    if( ( pxStreamBuffer->ucFlags & sbFLAGS_IS_BATCHING_BUFFER ) != ( uint8_t ) 0 )
    {
        if( xBytesInBuffer > pxStreamBuffer->xTriggerLevelBytes )
        {
            xReturn = pdTRUE;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    else if( xBytesInBuffer >= pxStreamBuffer->xTriggerLevelBytes )
    {
        xReturn = pdTRUE;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

static void prvInitialiseNewStreamBuffer( StreamBuffer_t * const pxStreamBuffer,
                                          uint8_t * const pucBuffer,
                                          size_t xBufferSizeBytes,
                                          size_t xTriggerLevelBytes,
                                          uint8_t ucFlags,
                                          StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                          StreamBufferCallbackFunction_t pxReceiveCompletedCallback )
{
    /*
     * 此处的断言故意写入整个缓冲区，以确保可写入而不产生异常，同时将
     * 缓冲区设为已知值，便于开发/调试。
     */
    #if ( configASSERT_DEFINED == 1 )
    {
        /*
         * 写入的值只需在查看内存时能够识别即可。不要使用 0xA5，因为它是栈
         * 填充值，可能导致难以分辨实际观察到的内容。
         */
        #define STREAM_BUFFER_BUFFER_WRITE_VALUE    ( 0x55 )
        configASSERT( memset( pucBuffer, ( int ) STREAM_BUFFER_BUFFER_WRITE_VALUE, xBufferSizeBytes ) == pucBuffer );
    }
    #endif

    ( void ) memset( ( void * ) pxStreamBuffer, 0x00, sizeof( StreamBuffer_t ) );
    pxStreamBuffer->pucBuffer = pucBuffer;
    pxStreamBuffer->xLength = xBufferSizeBytes;
    pxStreamBuffer->xTriggerLevelBytes = xTriggerLevelBytes;
    pxStreamBuffer->ucFlags = ucFlags;
    pxStreamBuffer->uxNotificationIndex = tskDEFAULT_INDEX_TO_NOTIFY;
    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    {
        pxStreamBuffer->pxSendCompletedCallback = pxSendCompletedCallback;
        pxStreamBuffer->pxReceiveCompletedCallback = pxReceiveCompletedCallback;
    }
    #else
    {
        /* MISRA 参考 11.1.1 [对象类型转换] */
        /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-111 */
        /* coverity[misra_c_2012_rule_11_1_violation]（Coverity 违规抑制） */
        ( void ) pxSendCompletedCallback;

        /* MISRA 参考 11.1.1 [对象类型转换] */
        /* 详见：https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/MISRA.md#rule-111 */
        /* coverity[misra_c_2012_rule_11_1_violation]（Coverity 违规抑制） */
        ( void ) pxReceiveCompletedCallback;
    }
    #endif /* if ( configUSE_SB_COMPLETED_CALLBACK == 1 )：启用完成回调 */
}
/*-----------------------------------------------------------*/

UBaseType_t uxStreamBufferGetStreamBufferNotificationIndex( StreamBufferHandle_t xStreamBuffer )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;

    traceENTER_uxStreamBufferGetStreamBufferNotificationIndex( xStreamBuffer );

    configASSERT( pxStreamBuffer );

    traceRETURN_uxStreamBufferGetStreamBufferNotificationIndex( pxStreamBuffer->uxNotificationIndex );

    return pxStreamBuffer->uxNotificationIndex;
}
/*-----------------------------------------------------------*/

void vStreamBufferSetStreamBufferNotificationIndex( StreamBufferHandle_t xStreamBuffer,
                                                    UBaseType_t uxNotificationIndex )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;

    traceENTER_vStreamBufferSetStreamBufferNotificationIndex( xStreamBuffer, uxNotificationIndex );

    /* 不应有任务在等待，否则将永远无法恢复它们。 */
    configASSERT( ( pxStreamBuffer != NULL ) && ( pxStreamBuffer->xTaskWaitingToReceive == NULL ) );
    configASSERT( ( pxStreamBuffer != NULL ) && ( pxStreamBuffer->xTaskWaitingToSend == NULL ) );

    /* 检查任务通知索引是否有效。 */
    configASSERT( uxNotificationIndex < configTASK_NOTIFICATION_ARRAY_ENTRIES );

    pxStreamBuffer->uxNotificationIndex = uxNotificationIndex;

    traceRETURN_vStreamBufferSetStreamBufferNotificationIndex();
}
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

    UBaseType_t uxStreamBufferGetStreamBufferNumber( StreamBufferHandle_t xStreamBuffer )
    {
        traceENTER_uxStreamBufferGetStreamBufferNumber( xStreamBuffer );

        traceRETURN_uxStreamBufferGetStreamBufferNumber( xStreamBuffer->uxStreamBufferNumber );

        return xStreamBuffer->uxStreamBufferNumber;
    }

    #endif /* configUSE_TRACE_FACILITY：启用跟踪功能 */
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

    void vStreamBufferSetStreamBufferNumber( StreamBufferHandle_t xStreamBuffer,
                                             UBaseType_t uxStreamBufferNumber )
    {
        traceENTER_vStreamBufferSetStreamBufferNumber( xStreamBuffer, uxStreamBufferNumber );

        xStreamBuffer->uxStreamBufferNumber = uxStreamBufferNumber;

        traceRETURN_vStreamBufferSetStreamBufferNumber();
    }

    #endif /* configUSE_TRACE_FACILITY：启用跟踪功能 */
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

    uint8_t ucStreamBufferGetStreamBufferType( StreamBufferHandle_t xStreamBuffer )
    {
        traceENTER_ucStreamBufferGetStreamBufferType( xStreamBuffer );

        traceRETURN_ucStreamBufferGetStreamBufferType( ( uint8_t ) ( xStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) );

        return( ( uint8_t ) ( xStreamBuffer->ucFlags & sbFLAGS_IS_MESSAGE_BUFFER ) );
    }

    #endif /* configUSE_TRACE_FACILITY：启用跟踪功能 */
/*-----------------------------------------------------------*/

/*
 * 如果应用未配置启用流缓冲功能，则整个源文件将被跳过。该 #if 在本文件
 * 最末尾关闭。如需启用流缓冲，请确保 FreeRTOSConfig.h 中的
 * configUSE_STREAM_BUFFERS 设置为 1。
 */
#endif /* configUSE_STREAM_BUFFERS == 1：启用流缓冲功能 */
