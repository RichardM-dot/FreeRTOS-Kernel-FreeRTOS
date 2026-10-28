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
 * 流缓冲用于在一个任务或中断与另一个任务或中断之间发送连续的数据流。其实现轻量，因此特别适合中断到任务以及核间通信等场景。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 */

#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include stream_buffer.h"
#endif

/* *INDENT-OFF*（关闭自动缩进） */
#if defined( __cplusplus )
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进） */

/**
 * 流缓冲的类型。仅供内部使用。
 */
#define sbTYPE_STREAM_BUFFER             ( ( BaseType_t ) 0 )
#define sbTYPE_MESSAGE_BUFFER            ( ( BaseType_t ) 1 )
#define sbTYPE_STREAM_BATCHING_BUFFER    ( ( BaseType_t ) 2 )

/**
 * 用于引用流缓冲的类型。例如，调用 xStreamBufferCreate() 会返回一个 StreamBufferHandle_t 变量，该变量随后可作为参数传给 xStreamBufferSend()、xStreamBufferReceive() 等函数。
 */
struct StreamBufferDef_t;
typedef struct StreamBufferDef_t * StreamBufferHandle_t;

/**
 * 用作流缓冲可选回调的类型。
 */
typedef void (* StreamBufferCallbackFunction_t)( StreamBufferHandle_t xStreamBuffer,
                                                 BaseType_t xIsInsideISR,
                                                 BaseType_t * const pxHigherPriorityTaskWoken );

/**
 * stream_buffer.h
 *
 * @code{c}
 * StreamBufferHandle_t xStreamBufferCreate( size_t xBufferSizeBytes, size_t xTriggerLevelBytes );
 * @endcode
 *
 * 使用动态分配的内存创建新的流缓冲。若需使用静态分配内存（在编译时分配的内存）的版本，请参见 xStreamBufferCreateStatic()。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferCreate()，必须将 configSUPPORT_DYNAMIC_ALLOCATION 设置为 1，或将其保留为未定义状态。要在 FreeRTOSConfig.h 中使用 xStreamBufferCreate()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xBufferSizeBytes 流缓冲在任意时刻能够容纳的总字节数。
 *
 * @param xTriggerLevelBytes 在阻塞等待数据的任务被移出阻塞状态之前，流缓冲中必须达到的字节数。例如，若某任务阻塞读取一个触发水位为 1 的空流缓冲，则当向缓冲区写入一个字节或该任务的阻塞时间到期时，任务将被解除阻塞。再例如，若某任务阻塞读取一个触发水位为 10 的空流缓冲，则直到流缓冲中至少包含 10 个字节，或该任务的阻塞时间到期，任务才会被解除阻塞。如果读取任务的阻塞时间在达到触发水位之前到期，任务仍将接收到实际可用的字节。将触发水位设置为 0 将导致实际使用 1 作为触发水位。指定的触发水位不得大于缓冲区大小。
 *
 * @param pxSendCompletedCallback 当发送到流缓冲的字节数至少等于触发水位时调用的回调。如果该参数为 NULL，则使用 sbSEND_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @param pxReceiveCompletedCallback 当从流缓冲读取的字节数大于零时调用的回调。如果该参数为 NULL，则使用 sbRECEIVE_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @return 如果返回 NULL，则表示无法创建流缓冲，原因是 FreeRTOS 没有足够的堆内存来分配流缓冲的数据结构和存储区域。返回非 NULL 值表示流缓冲已成功创建——返回的值应保存为所创建流缓冲的句柄。
 *
 * 示例用法：
 * @code{c}
 *
 * void vAFunction( void )
 * {
 * StreamBufferHandle_t xStreamBuffer;
 * const size_t xStreamBufferSizeBytes = 100, xTriggerLevel = 10;
 *
 *  // 创建一个可容纳 100 字节的流缓冲。用于保存流缓冲结构体及流缓冲中数据的内存
 *  // 均通过动态方式分配。
 *  xStreamBuffer = xStreamBufferCreate( xStreamBufferSizeBytes, xTriggerLevel );
 *
 *  if( xStreamBuffer == NULL )
 *  {
 *      // 没有足够的堆内存空间来创建流缓冲。
 *  }
 *  else
 *  {
 *      // 流缓冲创建成功，现在可以使用。
 *  }
 * }
 * @endcode
 * \defgroup xStreamBufferCreate xStreamBufferCreate
 * \ingroup StreamBufferManagement
 */

#define xStreamBufferCreate( xBufferSizeBytes, xTriggerLevelBytes ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BUFFER, NULL, NULL )

#if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    #define xStreamBufferCreateWithCallback( xBufferSizeBytes, xTriggerLevelBytes, pxSendCompletedCallback, pxReceiveCompletedCallback ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BUFFER, ( pxSendCompletedCallback ), ( pxReceiveCompletedCallback ) )
#endif

/**
 * stream_buffer.h
 *
 * @code{c}
 * StreamBufferHandle_t xStreamBufferCreateStatic( size_t xBufferSizeBytes,
 *                                                 size_t xTriggerLevelBytes,
 *                                                 uint8_t *pucStreamBufferStorageArea,
 *                                                 StaticStreamBuffer_t *pxStaticStreamBuffer );
 * @endcode
 * 使用静态分配的内存创建新的流缓冲。若需使用动态分配内存的版本，请参见 xStreamBufferCreate()。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferCreateStatic()，必须将 configSUPPORT_STATIC_ALLOCATION 设置为 1。要在 FreeRTOSConfig.h 中使用 xStreamBufferCreateStatic()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xBufferSizeBytes 由 pucStreamBufferStorageArea 参数所指向缓冲区的大小（字节数）。
 *
 * @param xTriggerLevelBytes 在阻塞等待数据的任务被移出阻塞状态之前，流缓冲中必须达到的字节数。例如，若某任务阻塞读取一个触发水位为 1 的空流缓冲，则当向缓冲区写入一个字节或该任务的阻塞时间到期时，任务将被解除阻塞。再例如，若某任务阻塞读取一个触发水位为 10 的空流缓冲，则直到流缓冲中至少包含 10 个字节，或该任务的阻塞时间到期，任务才会被解除阻塞。如果读取任务的阻塞时间在达到触发水位之前到期，任务仍将接收到实际可用的字节。将触发水位设置为 0 将导致实际使用 1 作为触发水位。指定的触发水位不得大于缓冲区大小。
 *
 * @param pucStreamBufferStorageArea 必须指向一个大小至少为 xBufferSizeBytes 的 uint8_t 数组。当数据流写入流缓冲时，数据将复制到该数组。
 *
 * @param pxStaticStreamBuffer 必须指向一个 StaticStreamBuffer_t 类型的变量，用于保存流缓冲的数据结构。
 *
 * @param pxSendCompletedCallback 当发送到流缓冲的字节数至少等于触发水位时调用的回调。如果该参数为 NULL，则使用 sbSEND_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @param pxReceiveCompletedCallback 当从流缓冲读取的字节数大于零时调用的回调。如果该参数为 NULL，则使用 sbRECEIVE_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @return 如果流缓冲创建成功，则返回所创建流缓冲的句柄。如果 pucStreamBufferStorageArea 或 pxStaticStreamBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 *
 * // 用于指定保存数据流的数组大小。实际可用空间将比该值少 1，因此为 999。
 * #define STORAGE_SIZE_BYTES 1000
 *
 * // 定义实际用于保存流缓冲中数据流的内存。
 * static uint8_t ucStorageBuffer[ STORAGE_SIZE_BYTES ];
 *
 * // 用于保存流缓冲结构体的变量。
 * StaticStreamBuffer_t xStreamBufferStruct;
 *
 * void MyFunction( void )
 * {
 * StreamBufferHandle_t xStreamBuffer;
 * const size_t xTriggerLevel = 1;
 *
 *  xStreamBuffer = xStreamBufferCreateStatic( sizeof( ucStorageBuffer ),
 *                                             xTriggerLevel,
 *                                             ucStorageBuffer,
 *                                             &xStreamBufferStruct );
 *
 *  // 由于 pucStreamBufferStorageArea 和 pxStaticStreamBuffer 参数均不为 NULL，
 *  // xStreamBuffer 不会为 NULL，可用于在其他流缓冲 API 调用中引用
 *  // 已创建的流缓冲。
 *
 *  // 使用该流缓冲的其他代码可放在此处。
 * }
 *
 * @endcode
 * \defgroup xStreamBufferCreateStatic xStreamBufferCreateStatic
 * \ingroup StreamBufferManagement
 */

#define xStreamBufferCreateStatic( xBufferSizeBytes, xTriggerLevelBytes, pucStreamBufferStorageArea, pxStaticStreamBuffer ) \
    xStreamBufferGenericCreateStatic( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BUFFER, ( pucStreamBufferStorageArea ), ( pxStaticStreamBuffer ), NULL, NULL )

#if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    #define xStreamBufferCreateStaticWithCallback( xBufferSizeBytes, xTriggerLevelBytes, pucStreamBufferStorageArea, pxStaticStreamBuffer, pxSendCompletedCallback, pxReceiveCompletedCallback ) \
    xStreamBufferGenericCreateStatic( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BUFFER, ( pucStreamBufferStorageArea ), ( pxStaticStreamBuffer ), ( pxSendCompletedCallback ), ( pxReceiveCompletedCallback ) )
#endif

/**
 * stream_buffer.h
 *
 * @code{c}
 * StreamBufferHandle_t xStreamBatchingBufferCreate( size_t xBufferSizeBytes, size_t xTriggerLevelBytes );
 * @endcode
 *
 * 使用动态分配的内存创建新的流批量缓冲。若需使用静态分配内存（在编译时分配的内存）的版本，请参见 xStreamBatchingBufferCreateStatic()。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBatchingBufferCreate()，必须将 configSUPPORT_DYNAMIC_ALLOCATION 设置为 1，或将其保留为未定义状态。要在 FreeRTOSConfig.h 中使用 xStreamBatchingBufferCreate()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * 流缓冲与流批量缓冲的区别在于任务对非空缓冲区执行读取时的行为：
 * - 从非空流缓冲读取的任务立即返回，无论缓冲区中的数据量多少。
 * - 从非空流批量缓冲读取的任务将阻塞，直到缓冲区中的数据量超过触发水位，或阻塞时间到期。
 *
 * @param xBufferSizeBytes 流批量缓冲在任意时刻能够容纳的总字节数。
 *
 * @param xTriggerLevelBytes 在阻塞时间到期之前，为使调用 xStreamBufferReceive 的任务解除阻塞，流批量缓冲中必须达到的字节数。
 *
 * @param pxSendCompletedCallback 当发送到流批量缓冲的字节数至少等于触发水位时调用的回调。如果该参数为 NULL，则使用 sbSEND_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @param pxReceiveCompletedCallback 当从流批量缓冲读取的字节数大于零时调用的回调。如果该参数为 NULL，则使用 sbRECEIVE_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @return 如果返回 NULL，则表示无法创建流批量缓冲，原因是 FreeRTOS 没有足够的堆内存来分配流批量缓冲的数据结构和存储区域。返回非 NULL 值表示流批量缓冲已成功创建——返回的值应保存为所创建流批量缓冲的句柄。
 *
 * 示例用法：
 * @code{c}
 *
 * void vAFunction( void )
 * {
 * StreamBufferHandle_t xStreamBatchingBuffer;
 * const size_t xStreamBufferSizeBytes = 100, xTriggerLevel = 10;
 *
 *  // 创建一个可容纳 100 字节的流批量缓冲。用于保存流批量缓冲结构体及其
 *  // 数据的内存均通过动态方式分配。
 *  xStreamBatchingBuffer = xStreamBatchingBufferCreate( xStreamBufferSizeBytes, xTriggerLevel );
 *
 *  if( xStreamBatchingBuffer == NULL )
 *  {
 *      // 没有足够的堆内存空间来创建流批量缓冲。
 *  }
 *  else
 *  {
 *      // 流批量缓冲创建成功，现在可以使用。
 *  }
 * }
 * @endcode
 * \defgroup xStreamBatchingBufferCreate xStreamBatchingBufferCreate
 * \ingroup StreamBatchingBufferManagement
 */

#define xStreamBatchingBufferCreate( xBufferSizeBytes, xTriggerLevelBytes ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BATCHING_BUFFER, NULL, NULL )

#if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    #define xStreamBatchingBufferCreateWithCallback( xBufferSizeBytes, xTriggerLevelBytes, pxSendCompletedCallback, pxReceiveCompletedCallback ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BATCHING_BUFFER, ( pxSendCompletedCallback ), ( pxReceiveCompletedCallback ) )
#endif

/**
 * stream_buffer.h
 *
 * @code{c}
 * StreamBufferHandle_t xStreamBatchingBufferCreateStatic( size_t xBufferSizeBytes,
 *                                                         size_t xTriggerLevelBytes,
 *                                                         uint8_t *pucStreamBufferStorageArea,
 *                                                         StaticStreamBuffer_t *pxStaticStreamBuffer );
 * @endcode
 * 使用静态分配的内存创建新的流批量缓冲。若需使用动态分配内存的版本，请参见 xStreamBatchingBufferCreate()。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBatchingBufferCreateStatic()，必须将 configSUPPORT_STATIC_ALLOCATION 设置为 1。要在 FreeRTOSConfig.h 中使用 xStreamBatchingBufferCreateStatic()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * 流缓冲与流批量缓冲的区别在于任务对非空缓冲区执行读取时的行为：
 * - 从非空流缓冲读取的任务立即返回，无论缓冲区中的数据量多少。
 * - 从非空流批量缓冲读取的任务将阻塞，直到缓冲区中的数据量超过触发水位，或阻塞时间到期。
 *
 * @param xBufferSizeBytes 由 pucStreamBufferStorageArea 参数所指向缓冲区的大小（字节数）。
 *
 * @param xTriggerLevelBytes 在阻塞时间到期之前，为使调用 xStreamBufferReceive 的任务解除阻塞，流批量缓冲中必须达到的字节数。
 *
 * @param pucStreamBufferStorageArea 必须指向一个大小至少为 xBufferSizeBytes 的 uint8_t 数组。当数据流写入流批量缓冲时，数据将复制到该数组。
 *
 * @param pxStaticStreamBuffer 必须指向一个 StaticStreamBuffer_t 类型的变量，用于保存流批量缓冲的数据结构。
 *
 * @param pxSendCompletedCallback 当发送到流批量缓冲的字节数至少等于触发水位时调用的回调。如果该参数为 NULL，则使用 sbSEND_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @param pxReceiveCompletedCallback 当从流批量缓冲读取的字节数大于零时调用的回调。如果该参数为 NULL，则使用 sbRECEIVE_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @return 如果流批量缓冲创建成功，则返回所创建流批量缓冲的句柄。如果 pucStreamBufferStorageArea 或 pxStaticStreamBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 *
 * // 用于指定保存数据流的数组大小。实际可用空间将比该值少 1，因此为 999。
 * #define STORAGE_SIZE_BYTES 1000
 *
 * // 定义实际用于保存流批量缓冲中数据流的内存。
 * static uint8_t ucStorageBuffer[ STORAGE_SIZE_BYTES ];
 *
 * // 用于保存流批量缓冲结构体的变量。
 * StaticStreamBuffer_t xStreamBufferStruct;
 *
 * void MyFunction( void )
 * {
 * StreamBufferHandle_t xStreamBatchingBuffer;
 * const size_t xTriggerLevel = 1;
 *
 *  xStreamBatchingBuffer = xStreamBatchingBufferCreateStatic( sizeof( ucStorageBuffer ),
 *                                                             xTriggerLevel,
 *                                                             ucStorageBuffer,
 *                                                             &xStreamBufferStruct );
 *
 *  // 由于 pucStreamBufferStorageArea 和 pxStaticStreamBuffer 参数均不为 NULL，
 *  // xStreamBatchingBuffer 不会为 NULL，可用于在其他流缓冲 API 调用中
 *  // 引用已创建的流批量缓冲。
 *
 *  // 使用该流批量缓冲的其他代码可放在此处。
 * }
 *
 * @endcode
 * \defgroup xStreamBatchingBufferCreateStatic xStreamBatchingBufferCreateStatic
 * \ingroup StreamBatchingBufferManagement
 */

#define xStreamBatchingBufferCreateStatic( xBufferSizeBytes, xTriggerLevelBytes, pucStreamBufferStorageArea, pxStaticStreamBuffer ) \
    xStreamBufferGenericCreateStatic( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BATCHING_BUFFER, ( pucStreamBufferStorageArea ), ( pxStaticStreamBuffer ), NULL, NULL )

#if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    #define xStreamBatchingBufferCreateStaticWithCallback( xBufferSizeBytes, xTriggerLevelBytes, pucStreamBufferStorageArea, pxStaticStreamBuffer, pxSendCompletedCallback, pxReceiveCompletedCallback ) \
    xStreamBufferGenericCreateStatic( ( xBufferSizeBytes ), ( xTriggerLevelBytes ), sbTYPE_STREAM_BATCHING_BUFFER, ( pucStreamBufferStorageArea ), ( pxStaticStreamBuffer ), ( pxSendCompletedCallback ), ( pxReceiveCompletedCallback ) )
#endif

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferGetStaticBuffers( StreamBufferHandle_t xStreamBuffer,
 *                                           uint8_t ** ppucStreamBufferStorageArea,
 *                                           StaticStreamBuffer_t ** ppxStaticStreamBuffer );
 * @endcode
 *
 * 检索静态创建的流缓冲的数据结构缓冲区和存储区缓冲区的指针。这些缓冲区与创建时提供的缓冲区相同。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferGetStaticBuffers()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要检索其缓冲区的流缓冲。
 *
 * @param ppucStreamBufferStorageArea 用于返回指向流缓冲存储区缓冲区的指针。
 *
 * @param ppxStaticStreamBuffer 用于返回指向流缓冲数据结构缓冲区的指针。
 *
 * @return 如果成功检索到缓冲区则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xStreamBufferGetStaticBuffers xStreamBufferGetStaticBuffers
 * \ingroup StreamBufferManagement
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xStreamBufferGetStaticBuffers( StreamBufferHandle_t xStreamBuffer,
                                              uint8_t ** ppucStreamBufferStorageArea,
                                              StaticStreamBuffer_t ** ppxStaticStreamBuffer ) PRIVILEGED_FUNCTION;
#endif /* configSUPPORT_STATIC_ALLOCATION 配置项 */

/**
 * stream_buffer.h
 *
 * @code{c}
 * size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
 *                           const void *pvTxData,
 *                           size_t xDataLengthBytes,
 *                           TickType_t xTicksToWait );
 * @endcode
 *
 * 向流缓冲发送字节。这些字节将被复制到流缓冲中。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xStreamBufferSend() 从任务向流缓冲写入。使用 xStreamBufferSendFromISR() 从中断服务程序（ISR）向流缓冲写入。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferSend()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要将数据流发送到的流缓冲的句柄。
 *
 * @param pvTxData 指向包含要复制到流缓冲中的字节的缓冲区的指针。
 *
 * @param xDataLengthBytes 从 pvTxData 复制到流缓冲中的最大字节数。
 *
 * @param xTicksToWait 如果流缓冲中的空间不足以再容纳 xDataLengthBytes 个字节，任务应保持在阻塞状态以等待流缓冲中出现足够空间的最长时间。阻塞时间以时钟节拍周期为单位指定，因此其代表的绝对时间取决于时钟节拍频率。可使用 pdMS_TO_TICKS() 宏将按毫秒指定的时间转换为按时钟节拍指定的时间。将 xTicksToWait 设置为 portMAX_DELAY 将使任务无限期等待（不超时），前提是 FreeRTOSConfig.h 中将 INCLUDE_vTaskSuspend 设置为 1。如果任务在将全部 xDataLengthBytes 写入缓冲区之前超时，它仍将尽可能多地写入字节。任务处于阻塞状态时不占用任何 CPU 时间。
 *
 * @return 写入流缓冲的字节数。如果任务在将全部 xDataLengthBytes 写入缓冲区之前超时，它仍将尽可能多地写入字节。
 *
 * 示例用法：
 * @code{c}
 * void vAFunction( StreamBufferHandle_t xStreamBuffer )
 * {
 * size_t xBytesSent;
 * uint8_t ucArrayToSend[] = { 0, 1, 2, 3 };
 * char *pcStringToSend = "String to send";
 * const TickType_t x100ms = pdMS_TO_TICKS( 100 );
 *
 *  // 向流缓冲发送一个数组，最多阻塞 100ms 以等待流缓冲中出现足够空间。
 *  xBytesSent = xStreamBufferSend( xStreamBuffer, ( void * ) ucArrayToSend, sizeof( ucArrayToSend ), x100ms );
 *
 *  if( xBytesSent != sizeof( ucArrayToSend ) )
 *  {
 *      // 对 xStreamBufferSend() 的调用在缓冲区中有足够空间写入数据之前超时，
 *      // 但已成功写入 xBytesSent 个字节。
 *  }
 *
 *  // 将字符串发送到流缓冲。如果缓冲区中没有足够空间则立即返回。
 *  xBytesSent = xStreamBufferSend( xStreamBuffer, ( void * ) pcStringToSend, strlen( pcStringToSend ), 0 );
 *
 *  if( xBytesSent != strlen( pcStringToSend ) )
 *  {
 *      // 由于缓冲区中没有足够的空闲空间，整个字符串无法全部添加到流缓冲，
 *      // 但已发送 xBytesSent 个字节。可以再次尝试发送剩余字节。
 *  }
 * }
 * @endcode
 * \defgroup xStreamBufferSend xStreamBufferSend
 * \ingroup StreamBufferManagement
 */
size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
                          const void * pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * size_t xStreamBufferSendFromISR( StreamBufferHandle_t xStreamBuffer,
 *                                  const void *pvTxData,
 *                                  size_t xDataLengthBytes,
 *                                  BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 向流缓冲发送字节流的 API 函数的中断安全版本。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xStreamBufferSend() 从任务向流缓冲写入。使用 xStreamBufferSendFromISR() 从中断服务程序（ISR）向流缓冲写入。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferSendFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要将数据流发送到的流缓冲的句柄。
 *
 * @param pvTxData 指向要复制到流缓冲中的数据的指针。
 *
 * @param xDataLengthBytes 从 pvTxData 复制到流缓冲中的最大字节数。
 *
 * @param pxHigherPriorityTaskWoken 流缓冲上可能有任务阻塞等待数据。调用 xStreamBufferSendFromISR() 可使数据变为可用，从而使等待数据的任务离开阻塞状态。如果调用 xStreamBufferSendFromISR() 导致某任务离开阻塞状态，且被解除阻塞的任务优先级高于当前正在执行的任务（被中断的任务），则 xStreamBufferSendFromISR() 会在内部将 *pxHigherPriorityTaskWoken 设置为 pdTRUE。如果 xStreamBufferSendFromISR() 将该值设置为 pdTRUE，通常应在退出中断之前执行一次上下文切换。这将确保中断直接返回到优先级最高的就绪态任务。在传入函数之前，应将 *pxHigherPriorityTaskWoken 设置为 pdFALSE。示例参见下面的示例代码。
 *
 * @return 实际写入流缓冲的字节数；如果流缓冲没有足够的空闲空间写入全部字节，则该值小于 xDataLengthBytes。
 *
 * 示例用法：
 * @code{c}
 * // 一个已经创建好的流缓冲。
 * StreamBufferHandle_t xStreamBuffer;
 *
 * void vAnInterruptServiceRoutine( void )
 * {
 * size_t xBytesSent;
 * char *pcStringToSend = "String to send";
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE; // 初始化为 pdFALSE。
 *
 *  // 尝试将字符串发送到流缓冲。
 *  xBytesSent = xStreamBufferSendFromISR( xStreamBuffer,
 *                                         ( void * ) pcStringToSend,
 *                                         strlen( pcStringToSend ),
 *                                         &xHigherPriorityTaskWoken );
 *
 *  if( xBytesSent != strlen( pcStringToSend ) )
 *  {
 *      // 流缓冲中没有足够的空闲空间写入整个字符串，但已写入 xBytesSent 个字节。
 *  }
 *
 *  // 如果在 xStreamBufferSendFromISR() 内部将 xHigherPriorityTaskWoken 设置为 pdTRUE，
 *  // 则说明有优先级高于当前正在执行任务的任务被解除阻塞，应执行上下文切换，
 *  // 以确保 ISR 返回到被解除阻塞的任务。在大多数 FreeRTOS 移植层中，
 *  // 只需将 xHigherPriorityTaskWoken 传给 portYIELD_FROM_ISR()，该宏会检查
 *  // 该变量的值，并在需要时执行上下文切换。具体移植层相关说明请查阅
 *  // 所用移植层的文档。
 *  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 * }
 * @endcode
 * \defgroup xStreamBufferSendFromISR xStreamBufferSendFromISR
 * \ingroup StreamBufferManagement
 */
size_t xStreamBufferSendFromISR( StreamBufferHandle_t xStreamBuffer,
                                 const void * pvTxData,
                                 size_t xDataLengthBytes,
                                 BaseType_t * const pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
 *                              void *pvRxData,
 *                              size_t xBufferLengthBytes,
 *                              TickType_t xTicksToWait );
 * @endcode
 *
 * 从流缓冲接收字节。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xStreamBufferReceive() 从任务读取流缓冲。使用 xStreamBufferReceiveFromISR() 从中断服务程序（ISR）读取流缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferReceive()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要接收其字节的流缓冲的句柄。
 *
 * @param pvRxData 指向将复制所接收字节的缓冲区的指针。
 *
 * @param xBufferLengthBytes pvRxData 参数所指向缓冲区的长度。这设置了一次调用中可接收的最大字节数。xStreamBufferReceive 将在 xBufferLengthBytes 设置的上限内返回尽可能多的字节。
 *
 * @param xTicksToWait 如果流缓冲为空，任务应保持在阻塞状态以等待数据变为可用的最长时间。如果 xTicksToWait 为零，xStreamBufferReceive() 将立即返回。阻塞时间以时钟节拍周期为单位指定，因此其代表的绝对时间取决于时钟节拍频率。可使用 pdMS_TO_TICKS() 宏将按毫秒指定的时间转换为按时钟节拍指定的时间。将 xTicksToWait 设置为 portMAX_DELAY 将使任务无限期等待（不超时），前提是 FreeRTOSConfig.h 中将 INCLUDE_vTaskSuspend 设置为 1。任务处于阻塞状态时不占用任何 CPU 时间。
 *
 * @return 实际从流缓冲读取的字节数；如果对 xStreamBufferReceive() 的调用在获得 xBufferLengthBytes 个字节之前超时，则该值小于 xBufferLengthBytes。
 *
 * 示例用法：
 * @code{c}
 * void vAFunction( StreamBufferHandle_t xStreamBuffer )
 * {
 * uint8_t ucRxData[ 20 ];
 * size_t xReceivedBytes;
 * const TickType_t xBlockTime = pdMS_TO_TICKS( 20 );
 *
 *  // 从流缓冲再接收最多 sizeof( ucRxData ) 个字节。
 *  // 在阻塞状态（不占用任何 CPU 处理时间）最多等待 20ms，
 *  // 以等待完整的 sizeof( ucRxData ) 个字节可用。
 *  xReceivedBytes = xStreamBufferReceive( xStreamBuffer,
 *                                         ( void * ) ucRxData,
 *                                         sizeof( ucRxData ),
 *                                         xBlockTime );
 *
 *  if( xReceivedBytes > 0 )
 *  {
 *      // ucRxData 中包含另外 xReceivedBytes 个字节的数据，可在此处处理...
 *  }
 * }
 * @endcode
 * \defgroup xStreamBufferReceive xStreamBufferReceive
 * \ingroup StreamBufferManagement
 */
size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
                             void * pvRxData,
                             size_t xBufferLengthBytes,
                             TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * size_t xStreamBufferReceiveFromISR( StreamBufferHandle_t xStreamBuffer,
 *                                     void *pvRxData,
 *                                     size_t xBufferLengthBytes,
 *                                     BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 从流缓冲接收字节的 API 函数的中断安全版本。
 *
 * 使用 xStreamBufferReceive() 从任务读取流缓冲中的字节。使用 xStreamBufferReceiveFromISR() 从中断服务程序（ISR）读取流缓冲中的字节。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferReceiveFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要接收其数据流的流缓冲的句柄。
 *
 * @param pvRxData 指向将复制所接收字节的缓冲区的指针。
 *
 * @param xBufferLengthBytes pvRxData 参数所指向缓冲区的长度。这设置了一次调用中可接收的最大字节数。xStreamBufferReceive 将在 xBufferLengthBytes 设置的上限内返回尽可能多的字节。
 *
 * @param pxHigherPriorityTaskWoken 流缓冲上可能有任务阻塞等待空间变为可用。调用 xStreamBufferReceiveFromISR() 可使空间变为可用，从而使等待空间的任务离开阻塞状态。如果调用 xStreamBufferReceiveFromISR() 导致某任务离开阻塞状态，且被解除阻塞的任务优先级高于当前正在执行的任务（被中断的任务），则 xStreamBufferReceiveFromISR() 会在内部将 *pxHigherPriorityTaskWoken 设置为 pdTRUE。如果 xStreamBufferReceiveFromISR() 将该值设置为 pdTRUE，通常应在退出中断之前执行一次上下文切换。这将确保中断直接返回到优先级最高的就绪态任务。在传入函数之前，应将 *pxHigherPriorityTaskWoken 设置为 pdFALSE。示例参见下面的代码示例。
 *
 * @return 从流缓冲读取的字节数（如果有）。
 *
 * 示例用法：
 * @code{c}
 * // 一个已经创建好的流缓冲。
 * StreamBufferHandle_t xStreamBuffer;
 *
 * void vAnInterruptServiceRoutine( void )
 * {
 * uint8_t ucRxData[ 20 ];
 * size_t xReceivedBytes;
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // 初始化为 pdFALSE。
 *
 *  // 从流缓冲接收下一个数据流。
 *  xReceivedBytes = xStreamBufferReceiveFromISR( xStreamBuffer,
 *                                                ( void * ) ucRxData,
 *                                                sizeof( ucRxData ),
 *                                                &xHigherPriorityTaskWoken );
 *
 *  if( xReceivedBytes > 0 )
 *  {
 *      // ucRxData 中包含从流缓冲读取的 xReceivedBytes 个字节。
 *      // 在此处处理该数据流...
 *  }
 *
 *  // 如果在 xStreamBufferReceiveFromISR() 内部将 xHigherPriorityTaskWoken 设置为 pdTRUE，
 *  // 则说明有优先级高于当前正在执行任务的任务被解除阻塞，应执行上下文切换，
 *  // 以确保 ISR 返回到被解除阻塞的任务。在大多数 FreeRTOS 移植层中，
 *  // 只需将 xHigherPriorityTaskWoken 传给 portYIELD_FROM_ISR()，该宏会检查
 *  // 该变量的值，并在需要时执行上下文切换。具体移植层相关说明请查阅
 *  // 所用移植层的文档。
 *  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 * }
 * @endcode
 * \defgroup xStreamBufferReceiveFromISR xStreamBufferReceiveFromISR
 * \ingroup StreamBufferManagement
 */
size_t xStreamBufferReceiveFromISR( StreamBufferHandle_t xStreamBuffer,
                                    void * pvRxData,
                                    size_t xBufferLengthBytes,
                                    BaseType_t * const pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * void vStreamBufferDelete( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 删除先前通过调用 xStreamBufferCreate() 或 xStreamBufferCreateStatic() 创建的流缓冲。如果流缓冲是使用动态内存创建的（即通过 xStreamBufferCreate() 创建），则释放已分配的内存。
 *
 * 流缓冲被删除后，不得再使用其句柄。
 *
 * 要在 FreeRTOSConfig.h 中使用 vStreamBufferDelete()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要删除的流缓冲的句柄。
 *
 * \defgroup vStreamBufferDelete vStreamBufferDelete
 * \ingroup StreamBufferManagement
 */
void vStreamBufferDelete( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferIsFull( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 查询流缓冲是否已满。如果流缓冲没有任何空闲空间，因而不能再接收任何数据，则流缓冲为满。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferIsFull()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要查询的流缓冲的句柄。
 *
 * @return 如果流缓冲已满则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xStreamBufferIsFull xStreamBufferIsFull
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferIsFull( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferIsEmpty( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 查询流缓冲是否为空。如果流缓冲不包含任何数据，则流缓冲为空。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferIsEmpty()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要查询的流缓冲的句柄。
 *
 * @return 如果流缓冲为空则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xStreamBufferIsEmpty xStreamBufferIsEmpty
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferIsEmpty( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferReset( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 将流缓冲重置为初始的空状态。流缓冲中原有的任何数据都将被丢弃。只有在没有任务阻塞等待向流缓冲发送或从流缓冲接收时，才能重置流缓冲。
 *
 * 使用 xStreamBufferReset() 从任务重置流缓冲。使用 xStreamBufferResetFromISR() 从中断服务程序（ISR）重置流缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferReset()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要重置的流缓冲的句柄。
 *
 * @return 如果流缓冲已重置则返回 pdPASS。如果有任务阻塞等待向流缓冲发送或从流缓冲读取，则流缓冲不会被重置，并返回 pdFAIL。
 *
 * \defgroup xStreamBufferReset xStreamBufferReset
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferReset( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferResetFromISR( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 重置流缓冲的 API 函数的中断安全版本。
 *
 * 将流缓冲重置为初始的空状态。流缓冲中原有的任何数据都将被丢弃。只有在没有任务阻塞等待向流缓冲发送或从流缓冲接收时，才能重置流缓冲。
 *
 * 使用 xStreamBufferReset() 从任务重置流缓冲。使用 xStreamBufferResetFromISR() 从中断服务程序（ISR）重置流缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferResetFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要重置的流缓冲的句柄。
 *
 * @return 如果流缓冲已重置则返回 pdPASS。如果有任务阻塞等待向流缓冲发送或从流缓冲读取，则流缓冲不会被重置，并返回 pdFAIL。
 *
 * \defgroup xStreamBufferResetFromISR xStreamBufferResetFromISR
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferResetFromISR( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * size_t xStreamBufferSpacesAvailable( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 查询流缓冲中包含多少空闲空间，该值等于在流缓冲变满之前还可发送到流缓冲的数据量。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferSpacesAvailable()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要查询的流缓冲的句柄。
 *
 * @return 在流缓冲变满之前可写入流缓冲的字节数。
 *
 * \defgroup xStreamBufferSpacesAvailable xStreamBufferSpacesAvailable
 * \ingroup StreamBufferManagement
 */
size_t xStreamBufferSpacesAvailable( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * size_t xStreamBufferBytesAvailable( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 查询流缓冲中包含多少数据，该值等于在流缓冲变空之前可从流缓冲读取的字节数。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferBytesAvailable()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要查询的流缓冲的句柄。
 *
 * @return 在流缓冲变空之前可从流缓冲读取的字节数。
 *
 * \defgroup xStreamBufferBytesAvailable xStreamBufferBytesAvailable
 * \ingroup StreamBufferManagement
 */
size_t xStreamBufferBytesAvailable( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferSetTriggerLevel( StreamBufferHandle_t xStreamBuffer, size_t xTriggerLevel );
 * @endcode
 *
 * 流缓冲的触发水位是指：在阻塞等待数据的任务被移出阻塞状态之前，流缓冲中必须达到的字节数。例如，若某任务阻塞读取一个触发水位为 1 的空流缓冲，则当向缓冲区写入一个字节或该任务的阻塞时间到期时，任务将被解除阻塞。再例如，若某任务阻塞读取一个触发水位为 10 的空流缓冲，则直到流缓冲中至少包含 10 个字节，或该任务的阻塞时间到期，任务才会被解除阻塞。如果读取任务的阻塞时间在达到触发水位之前到期，任务仍将接收到实际可用的字节。将触发水位设置为 0 将导致实际使用 1 作为触发水位。指定的触发水位不得大于缓冲区大小。
 *
 * 创建流缓冲时会设置触发水位，并可使用 xStreamBufferSetTriggerLevel() 修改。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferSetTriggerLevel()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要更新的流缓冲的句柄。
 *
 * @param xTriggerLevel 流缓冲的新触发水位。
 *
 * @return 如果 xTriggerLevel 小于或等于流缓冲长度，则更新触发水位并返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xStreamBufferSetTriggerLevel xStreamBufferSetTriggerLevel
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferSetTriggerLevel( StreamBufferHandle_t xStreamBuffer,
                                         size_t xTriggerLevel ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferSendCompletedFromISR( StreamBufferHandle_t xStreamBuffer, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 仅供高级用户使用。
 *
 * 当数据被发送到消息缓冲或流缓冲时，FreeRTOS API 内部会调用 sbSEND_COMPLETED() 宏。如果有任务阻塞在消息缓冲或流缓冲上等待数据到达，则 sbSEND_COMPLETED() 宏会向该任务发送通知，使其离开阻塞状态。xStreamBufferSendCompletedFromISR() 完成相同的工作。提供该函数是为了让应用编写者实现自己的 sbSEND_COMPLETED() 版本，且不得在任何其他时间使用。
 *
 * 有关更多信息，请参见 FreeRTOS/Demo/Minimal/MessageBufferAMP.c 中实现的示例。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferSendCompletedFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 已向其中写入数据的流缓冲的句柄。
 *
 * @param pxHigherPriorityTaskWoken 在将 *pxHigherPriorityTaskWoken 传入 xStreamBufferSendCompletedFromISR() 之前，应将其初始化为 pdFALSE。如果调用 xStreamBufferSendCompletedFromISR() 使某任务离开阻塞状态，且该任务优先级高于当前正在运行的任务，则 *pxHigherPriorityTaskWoken 将被设置为 pdTRUE，表示应在退出 ISR 之前执行上下文切换。
 *
 * @return 如果有任务被移出阻塞状态则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xStreamBufferSendCompletedFromISR xStreamBufferSendCompletedFromISR
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferSendCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                              BaseType_t * pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * BaseType_t xStreamBufferReceiveCompletedFromISR( StreamBufferHandle_t xStreamBuffer, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 仅供高级用户使用。
 *
 * 当从消息缓冲或流缓冲读取数据时，FreeRTOS API 内部会调用 sbRECEIVE_COMPLETED() 宏。如果有任务阻塞在消息缓冲或流缓冲上等待数据到达，则 sbRECEIVE_COMPLETED() 宏会向该任务发送通知，使其离开阻塞状态。xStreamBufferReceiveCompletedFromISR() 完成相同的工作。提供该函数是为了让应用编写者实现自己的 sbRECEIVE_COMPLETED() 版本，且不得在任何其他时间使用。
 *
 * 有关更多信息，请参见 FreeRTOS/Demo/Minimal/MessageBufferAMP.c 中实现的示例。
 *
 * 要在 FreeRTOSConfig.h 中使用 xStreamBufferReceiveCompletedFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 已从其中读取数据的流缓冲的句柄。
 *
 * @param pxHigherPriorityTaskWoken 在将 *pxHigherPriorityTaskWoken 传入 xStreamBufferReceiveCompletedFromISR() 之前，应将其初始化为 pdFALSE。如果调用 xStreamBufferReceiveCompletedFromISR() 使某任务离开阻塞状态，且该任务优先级高于当前正在运行的任务，则 *pxHigherPriorityTaskWoken 将被设置为 pdTRUE，表示应在退出 ISR 之前执行上下文切换。
 *
 * @return 如果有任务被移出阻塞状态则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xStreamBufferReceiveCompletedFromISR xStreamBufferReceiveCompletedFromISR
 * \ingroup StreamBufferManagement
 */
BaseType_t xStreamBufferReceiveCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                                 BaseType_t * pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * UBaseType_t uxStreamBufferGetStreamBufferNotificationIndex( StreamBufferHandle_t xStreamBuffer );
 * @endcode
 *
 * 获取所提供流缓冲使用的任务通知索引，该索引可使用 vStreamBufferSetStreamBufferNotificationIndex 设置。如果未使用 vStreamBufferSetStreamBufferNotificationIndex 更改流缓冲的任务通知索引，则本函数返回默认值（tskDEFAULT_INDEX_TO_NOTIFY）。
 *
 * 要在 FreeRTOSConfig.h 中使用 uxStreamBufferGetStreamBufferNotificationIndex()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要检索其任务通知索引的流缓冲的句柄。
 *
 * @return 该流缓冲的任务通知索引。
 *
 * \defgroup uxStreamBufferGetStreamBufferNotificationIndex uxStreamBufferGetStreamBufferNotificationIndex
 * \ingroup StreamBufferManagement
 */
UBaseType_t uxStreamBufferGetStreamBufferNotificationIndex( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

/**
 * stream_buffer.h
 *
 * @code{c}
 * void vStreamBufferSetStreamBufferNotificationIndex ( StreamBufferHandle_t xStreamBuffer, UBaseType_t uxNotificationIndex );
 * @endcode
 *
 * 设置所提供流缓冲使用的任务通知索引。此后针对该流缓冲的流缓冲 API 调用（如 xStreamBufferSend 或 xStreamBufferReceive）将使用该新索引发送任务通知。
 *
 * 如果未调用本函数，任务通知将使用默认索引（tskDEFAULT_INDEX_TO_NOTIFY）。建议在尝试向流缓冲发送或接收数据之前调用本函数，以避免出现不一致。
 *
 * 要在 FreeRTOSConfig.h 中使用 vStreamBufferSetStreamBufferNotificationIndex()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xStreamBuffer 要设置其任务通知索引的流缓冲的句柄。
 *
 * @param uxNotificationIndex 要设置的任务通知索引。
 *
 * \defgroup vStreamBufferSetStreamBufferNotificationIndex vStreamBufferSetStreamBufferNotificationIndex
 * \ingroup StreamBufferManagement
 */
void vStreamBufferSetStreamBufferNotificationIndex( StreamBufferHandle_t xStreamBuffer,
                                                    UBaseType_t uxNotificationIndex ) PRIVILEGED_FUNCTION;

/* 以下函数不属于公共 API。 */
StreamBufferHandle_t xStreamBufferGenericCreate( size_t xBufferSizeBytes,
                                                 size_t xTriggerLevelBytes,
                                                 BaseType_t xStreamBufferType,
                                                 StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                                 StreamBufferCallbackFunction_t pxReceiveCompletedCallback ) PRIVILEGED_FUNCTION;

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    StreamBufferHandle_t xStreamBufferGenericCreateStatic( size_t xBufferSizeBytes,
                                                           size_t xTriggerLevelBytes,
                                                           BaseType_t xStreamBufferType,
                                                           uint8_t * const pucStreamBufferStorageArea,
                                                           StaticStreamBuffer_t * const pxStaticStreamBuffer,
                                                           StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                                           StreamBufferCallbackFunction_t pxReceiveCompletedCallback ) PRIVILEGED_FUNCTION;
#endif

size_t xStreamBufferNextMessageLengthBytes( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

#if ( configUSE_TRACE_FACILITY == 1 )
    void vStreamBufferSetStreamBufferNumber( StreamBufferHandle_t xStreamBuffer,
                                             UBaseType_t uxStreamBufferNumber ) PRIVILEGED_FUNCTION;
    UBaseType_t uxStreamBufferGetStreamBufferNumber( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;
    uint8_t ucStreamBufferGetStreamBufferType( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;
#endif

/* *INDENT-OFF*（关闭自动缩进） */
#if defined( __cplusplus )
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* !defined( STREAM_BUFFER_H )（头文件包含保护） */
