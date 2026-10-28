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
 * 消息缓冲的功能构建在 FreeRTOS 流缓冲之上。流缓冲用于在一个任务或中断与另一个任务或中断之间发送连续的数据流，而消息缓冲则用于在一个任务或中断与另一个任务或中断之间发送变长的离散消息。其实现轻量，因此特别适合中断到任务以及核间通信等场景。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 消息缓冲用于保存变长消息。为实现这一点，当消息写入消息缓冲时，还会额外写入 sizeof( size_t ) 个字节以保存消息长度（该过程由 API 函数在内部完成）。在 32 位架构上，sizeof( size_t ) 通常为 4 字节，因此在 32 位架构上向消息缓冲写入一条 10 字节的消息，实际会使消息缓冲的可用空间减少 14 字节（10 字节用于消息本身，4 字节用于保存消息长度）。
 */

#ifndef FREERTOS_MESSAGE_BUFFER_H
#define FREERTOS_MESSAGE_BUFFER_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include message_buffer.h"
#endif

/* 消息缓冲构建在流缓冲之上。 */
#include "stream_buffer.h"

/* *INDENT-OFF*（关闭自动缩进） */
#if defined( __cplusplus )
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进） */

/**
 * 用于引用消息缓冲的类型。例如，调用 xMessageBufferCreate() 会返回一个 MessageBufferHandle_t 变量，该变量随后可作为参数传给 xMessageBufferSend()、xMessageBufferReceive() 等函数。消息缓冲本质上构建为流缓冲，因此其句柄也被设置为与流缓冲句柄相同的类型。
 */
typedef StreamBufferHandle_t MessageBufferHandle_t;

/*-----------------------------------------------------------*/

/**
 * message_buffer.h
 *
 * @code{c}
 * MessageBufferHandle_t xMessageBufferCreate( size_t xBufferSizeBytes );
 * @endcode
 *
 * 使用动态分配的内存创建新的消息缓冲。若需使用静态分配内存（在编译时分配的内存）的版本，请参见 xMessageBufferCreateStatic()。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferCreate()，必须将 configSUPPORT_DYNAMIC_ALLOCATION 设置为 1，或将其保留为未定义状态。要在 FreeRTOSConfig.h 中使用 xMessageBufferCreate()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xBufferSizeBytes 消息缓冲在任意时刻能够容纳的总字节数（不是消息条数）。当消息写入消息缓冲时，还会额外写入 sizeof( size_t ) 个字节以保存消息长度。在 32 位架构上 sizeof( size_t ) 通常为 4 字节，因此在大多数 32 位架构上，一条 10 字节的消息将占用 14 字节的消息缓冲空间。
 *
 * @param pxSendCompletedCallback 当对消息缓冲的发送操作完成时调用的回调。如果该参数为 NULL，或调用 xMessageBufferCreate() 时未提供该参数，则使用 sbSEND_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @param pxReceiveCompletedCallback 当从消息缓冲的接收操作完成时调用的回调。如果该参数为 NULL，或调用 xMessageBufferCreate() 时未提供该参数，则使用 sbRECEIVE_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @return 如果返回 NULL，则表示无法创建消息缓冲，原因是 FreeRTOS 没有足够的堆内存来分配消息缓冲的数据结构和存储区域。返回非 NULL 值表示消息缓冲已成功创建——返回的值应保存为所创建消息缓冲的句柄。
 *
 * 示例用法：
 * @code{c}
 *
 * void vAFunction( void )
 * {
 * MessageBufferHandle_t xMessageBuffer;
 * const size_t xMessageBufferSizeBytes = 100;
 *
 *  // 创建一个可容纳 100 字节的消息缓冲。用于保存消息缓冲结构体及消息本身的内存
 *  // 均通过动态方式分配。每条添加到缓冲区的消息还会额外占用 4 个字节，
 *  // 用于保存消息长度。
 *  xMessageBuffer = xMessageBufferCreate( xMessageBufferSizeBytes );
 *
 *  if( xMessageBuffer == NULL )
 *  {
 *      // 没有足够的堆内存空间来创建消息缓冲。
 *  }
 *  else
 *  {
 *      // 消息缓冲创建成功，现在可以使用。
 *  }
 *
 * @endcode
 * \defgroup xMessageBufferCreate xMessageBufferCreate
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferCreate( xBufferSizeBytes ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( size_t ) 0, sbTYPE_MESSAGE_BUFFER, NULL, NULL )

#if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    #define xMessageBufferCreateWithCallback( xBufferSizeBytes, pxSendCompletedCallback, pxReceiveCompletedCallback ) \
    xStreamBufferGenericCreate( ( xBufferSizeBytes ), ( size_t ) 0, sbTYPE_MESSAGE_BUFFER, ( pxSendCompletedCallback ), ( pxReceiveCompletedCallback ) )
#endif

/**
 * message_buffer.h
 *
 * @code{c}
 * MessageBufferHandle_t xMessageBufferCreateStatic( size_t xBufferSizeBytes,
 *                                                   uint8_t *pucMessageBufferStorageArea,
 *                                                   StaticMessageBuffer_t *pxStaticMessageBuffer );
 * @endcode
 * 使用静态分配的内存创建新的消息缓冲。若需使用动态分配内存的版本，请参见 xMessageBufferCreate()。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferCreateStatic()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xBufferSizeBytes 由 pucMessageBufferStorageArea 参数所指向缓冲区的大小（字节数）。当消息写入消息缓冲时，还会额外写入 sizeof( size_t ) 个字节以保存消息长度。在 32 位架构上 sizeof( size_t ) 通常为 4 字节，因此在大多数 32 位架构上，一条 10 字节的消息将占用 14 字节的消息缓冲空间。消息缓冲实际可存储的最大字节数为 (xBufferSizeBytes - 1)。
 *
 * @param pucMessageBufferStorageArea 必须指向一个大小至少为 xBufferSizeBytes 的 uint8_t 数组。当消息写入消息缓冲时，消息将复制到该数组。
 *
 * @param pxStaticMessageBuffer 必须指向一个 StaticMessageBuffer_t 类型的变量，用于保存消息缓冲的数据结构。
 *
 * @param pxSendCompletedCallback 当新消息发送到消息缓冲时调用的回调。如果该参数为 NULL，或调用 xMessageBufferCreate() 时未提供该参数，则使用 sbSEND_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @param pxReceiveCompletedCallback 当从消息缓冲读取消息时调用的回调。如果该参数为 NULL，或调用 xMessageBufferCreate() 时未提供该参数，则使用 sbRECEIVE_COMPLETED 宏提供的默认实现。要启用该回调，必须在 FreeRTOSConfig.h 中将 configUSE_SB_COMPLETED_CALLBACK 设置为 1。
 *
 * @return 如果消息缓冲创建成功，则返回所创建消息缓冲的句柄。如果 pucMessageBufferStorageArea 或 pxStaticMessageBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 *
 * // 用于指定保存消息的数组大小。实际可用空间将比该值少 1，因此为 999。
 * #define STORAGE_SIZE_BYTES 1000
 *
 * // 定义实际用于保存消息缓冲中消息的内存。
 * static uint8_t ucStorageBuffer[ STORAGE_SIZE_BYTES ];
 *
 * // 用于保存消息缓冲结构体的变量。
 * StaticMessageBuffer_t xMessageBufferStruct;
 *
 * void MyFunction( void )
 * {
 * MessageBufferHandle_t xMessageBuffer;
 *
 *  xMessageBuffer = xMessageBufferCreateStatic( sizeof( ucStorageBuffer ),
 *                                               ucStorageBuffer,
 *                                               &xMessageBufferStruct );
 *
 *  // 由于 pucMessageBufferStorageArea 和 pxStaticMessageBuffer 参数均不为 NULL，
 *  // xMessageBuffer 不会为 NULL，可用于在其他消息缓冲 API 调用中引用
 *  // 已创建的消息缓冲。
 *
 *  // 使用该消息缓冲的其他代码可放在此处。
 * }
 *
 * @endcode
 * \defgroup xMessageBufferCreateStatic xMessageBufferCreateStatic
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferCreateStatic( xBufferSizeBytes, pucMessageBufferStorageArea, pxStaticMessageBuffer ) \
    xStreamBufferGenericCreateStatic( ( xBufferSizeBytes ), 0, sbTYPE_MESSAGE_BUFFER, ( pucMessageBufferStorageArea ), ( pxStaticMessageBuffer ), NULL, NULL )

#if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
    #define xMessageBufferCreateStaticWithCallback( xBufferSizeBytes, pucMessageBufferStorageArea, pxStaticMessageBuffer, pxSendCompletedCallback, pxReceiveCompletedCallback ) \
    xStreamBufferGenericCreateStatic( ( xBufferSizeBytes ), 0, sbTYPE_MESSAGE_BUFFER, ( pucMessageBufferStorageArea ), ( pxStaticMessageBuffer ), ( pxSendCompletedCallback ), ( pxReceiveCompletedCallback ) )
#endif

/**
 * message_buffer.h
 *
 * @code{c}
 * BaseType_t xMessageBufferGetStaticBuffers( MessageBufferHandle_t xMessageBuffer,
 *                                            uint8_t ** ppucMessageBufferStorageArea,
 *                                            StaticMessageBuffer_t ** ppxStaticMessageBuffer );
 * @endcode
 *
 * 检索静态创建的消息缓冲的数据结构缓冲区和存储区缓冲区的指针。这些缓冲区与创建时提供的缓冲区相同。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferGetStaticBuffers()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要检索其缓冲区的消息缓冲。
 *
 * @param ppucMessageBufferStorageArea 用于返回指向消息缓冲存储区缓冲区的指针。
 *
 * @param ppxStaticMessageBuffer 用于返回指向消息缓冲数据结构缓冲区的指针。
 *
 * @return 如果成功检索到缓冲区则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xMessageBufferGetStaticBuffers xMessageBufferGetStaticBuffers
 * \ingroup MessageBufferManagement
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #define xMessageBufferGetStaticBuffers( xMessageBuffer, ppucMessageBufferStorageArea, ppxStaticMessageBuffer ) \
    xStreamBufferGetStaticBuffers( ( xMessageBuffer ), ( ppucMessageBufferStorageArea ), ( ppxStaticMessageBuffer ) )
#endif /* configSUPPORT_STATIC_ALLOCATION 配置项 */

/**
 * message_buffer.h
 *
 * @code{c}
 * size_t xMessageBufferSend( MessageBufferHandle_t xMessageBuffer,
 *                            const void *pvTxData,
 *                            size_t xDataLengthBytes,
 *                            TickType_t xTicksToWait );
 * @endcode
 *
 * 向消息缓冲发送一条离散消息。消息可以是能够放入缓冲区空闲空间的任意长度，并将被复制到缓冲区中。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xMessageBufferSend() 从任务向消息缓冲写入。使用 xMessageBufferSendFromISR() 从中断服务程序（ISR）向消息缓冲写入。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferSend()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要将消息发送到的消息缓冲的句柄。
 *
 * @param pvTxData 指向要复制到消息缓冲中的消息的指针。
 *
 * @param xDataLengthBytes 消息的长度，即从 pvTxData 复制到消息缓冲中的字节数。当消息写入消息缓冲时，还会额外写入 sizeof( size_t ) 个字节以保存消息长度。在 32 位架构上 sizeof( size_t ) 通常为 4 字节，因此在大多数 32 位架构上，将 xDataLengthBytes 设置为 20 会使消息缓冲中的空闲空间减少 24 字节（20 字节消息数据加上 4 字节消息长度）。
 *
 * @param xTicksToWait 如果调用 xMessageBufferSend() 时消息缓冲空间不足，调用任务应保持在阻塞状态以等待消息缓冲中出现足够空间的最长时间。如果 xTicksToWait 为零，调用任务将永远不会阻塞。阻塞时间以时钟节拍周期为单位指定，因此其代表的绝对时间取决于时钟节拍频率。可使用 pdMS_TO_TICKS() 宏将按毫秒指定的时间转换为按时钟节拍指定的时间。将 xTicksToWait 设置为 portMAX_DELAY 将使任务无限期等待（不超时），前提是 FreeRTOSConfig.h 中将 INCLUDE_vTaskSuspend 设置为 1。任务处于阻塞状态时不占用任何 CPU 时间。
 *
 * @return 写入消息缓冲的字节数。如果对 xMessageBufferSend() 的调用在消息缓冲中有足够空间写入消息之前超时，则返回 0。如果调用未超时，则返回 xDataLengthBytes。
 *
 * 示例用法：
 * @code{c}
 * void vAFunction( MessageBufferHandle_t xMessageBuffer )
 * {
 * size_t xBytesSent;
 * uint8_t ucArrayToSend[] = { 0, 1, 2, 3 };
 * char *pcStringToSend = "String to send";
 * const TickType_t x100ms = pdMS_TO_TICKS( 100 );
 *
 *  // 向消息缓冲发送一个数组，最多阻塞 100ms 以等待消息缓冲中出现足够空间。
 *  xBytesSent = xMessageBufferSend( xMessageBuffer, ( void * ) ucArrayToSend, sizeof( ucArrayToSend ), x100ms );
 *
 *  if( xBytesSent != sizeof( ucArrayToSend ) )
 *  {
 *      // 对 xMessageBufferSend() 的调用在缓冲区中有足够空间写入数据之前超时。
 *  }
 *
 *  // 将字符串发送到消息缓冲。如果缓冲区中没有足够空间则立即返回。
 *  xBytesSent = xMessageBufferSend( xMessageBuffer, ( void * ) pcStringToSend, strlen( pcStringToSend ), 0 );
 *
 *  if( xBytesSent != strlen( pcStringToSend ) )
 *  {
 *      // 由于缓冲区中没有足够的空闲空间，字符串无法添加到消息缓冲。
 *  }
 * }
 * @endcode
 * \defgroup xMessageBufferSend xMessageBufferSend
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferSend( xMessageBuffer, pvTxData, xDataLengthBytes, xTicksToWait ) \
    xStreamBufferSend( ( xMessageBuffer ), ( pvTxData ), ( xDataLengthBytes ), ( xTicksToWait ) )

/**
 * message_buffer.h
 *
 * @code{c}
 * size_t xMessageBufferSendFromISR( MessageBufferHandle_t xMessageBuffer,
 *                                   const void *pvTxData,
 *                                   size_t xDataLengthBytes,
 *                                   BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 向消息缓冲发送一条离散消息的 API 函数的中断安全版本。消息可以是能够放入缓冲区空闲空间的任意长度，并将被复制到缓冲区中。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xMessageBufferSend() 从任务向消息缓冲写入。使用 xMessageBufferSendFromISR() 从中断服务程序（ISR）向消息缓冲写入。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferSendFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要将消息发送到的消息缓冲的句柄。
 *
 * @param pvTxData 指向要复制到消息缓冲中的消息的指针。
 *
 * @param xDataLengthBytes 消息的长度，即从 pvTxData 复制到消息缓冲中的字节数。当消息写入消息缓冲时，还会额外写入 sizeof( size_t ) 个字节以保存消息长度。在 32 位架构上 sizeof( size_t ) 通常为 4 字节，因此在大多数 32 位架构上，将 xDataLengthBytes 设置为 20 会使消息缓冲中的空闲空间减少 24 字节（20 字节消息数据加上 4 字节消息长度）。
 *
 * @param pxHigherPriorityTaskWoken 消息缓冲上可能有任务阻塞等待数据。调用 xMessageBufferSendFromISR() 可使数据变为可用，从而使等待数据的任务离开阻塞状态。如果调用 xMessageBufferSendFromISR() 导致某任务离开阻塞状态，且被解除阻塞的任务优先级高于当前正在执行的任务（被中断的任务），则 xMessageBufferSendFromISR() 会在内部将 *pxHigherPriorityTaskWoken 设置为 pdTRUE。如果 xMessageBufferSendFromISR() 将该值设置为 pdTRUE，通常应在退出中断之前执行一次上下文切换。这将确保中断直接返回到优先级最高的就绪态任务。在传入函数之前，应将 *pxHigherPriorityTaskWoken 设置为 pdFALSE。示例参见下面的代码示例。
 *
 * @return 实际写入消息缓冲的字节数。如果消息缓冲没有足够的空闲空间存储该消息，则返回 0，否则返回 xDataLengthBytes。
 *
 * 示例用法：
 * @code{c}
 * // 一个已经创建好的消息缓冲。
 * MessageBufferHandle_t xMessageBuffer;
 *
 * void vAnInterruptServiceRoutine( void )
 * {
 * size_t xBytesSent;
 * char *pcStringToSend = "String to send";
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE; // 初始化为 pdFALSE。
 *
 *  // 尝试将字符串发送到消息缓冲。
 *  xBytesSent = xMessageBufferSendFromISR( xMessageBuffer,
 *                                          ( void * ) pcStringToSend,
 *                                          strlen( pcStringToSend ),
 *                                          &xHigherPriorityTaskWoken );
 *
 *  if( xBytesSent != strlen( pcStringToSend ) )
 *  {
 *      // 由于缓冲区中没有足够的空闲空间，字符串无法添加到消息缓冲。
 *  }
 *
 *  // 如果在 xMessageBufferSendFromISR() 内部将 xHigherPriorityTaskWoken 设置为 pdTRUE，
 *  // 则说明有优先级高于当前正在执行任务的任务被解除阻塞，应执行上下文切换，
 *  // 以确保 ISR 返回到被解除阻塞的任务。在大多数 FreeRTOS 移植层中，
 *  // 只需将 xHigherPriorityTaskWoken 传给 portYIELD_FROM_ISR()，该宏会检查
 *  // 该变量的值，并在需要时执行上下文切换。具体移植层相关说明请查阅
 *  // 所用移植层的文档。
 *  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 * }
 * @endcode
 * \defgroup xMessageBufferSendFromISR xMessageBufferSendFromISR
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferSendFromISR( xMessageBuffer, pvTxData, xDataLengthBytes, pxHigherPriorityTaskWoken ) \
    xStreamBufferSendFromISR( ( xMessageBuffer ), ( pvTxData ), ( xDataLengthBytes ), ( pxHigherPriorityTaskWoken ) )

/**
 * message_buffer.h
 *
 * @code{c}
 * size_t xMessageBufferReceive( MessageBufferHandle_t xMessageBuffer,
 *                               void *pvRxData,
 *                               size_t xBufferLengthBytes,
 *                               TickType_t xTicksToWait );
 * @endcode
 *
 * 从消息缓冲接收一条离散消息。消息可以是变长的，并将从缓冲区复制出来。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xMessageBufferReceive() 从任务读取消息缓冲。使用 xMessageBufferReceiveFromISR() 从中断服务程序（ISR）读取消息缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferReceive()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要接收其消息的消息缓冲的句柄。
 *
 * @param pvRxData 指向将复制所接收消息的缓冲区的指针。
 *
 * @param xBufferLengthBytes pvRxData 参数所指向缓冲区的长度。这设置了可接收消息的最大长度。如果 xBufferLengthBytes 太小，不足以容纳下一条消息，则该消息将保留在消息缓冲中，并返回 0。
 *
 * @param xTicksToWait 如果消息缓冲为空，任务应保持在阻塞状态以等待消息的最长时间。如果 xTicksToWait 为零且消息缓冲为空，xMessageBufferReceive() 将立即返回。阻塞时间以时钟节拍周期为单位指定，因此其代表的绝对时间取决于时钟节拍频率。可使用 pdMS_TO_TICKS() 宏将按毫秒指定的时间转换为按时钟节拍指定的时间。将 xTicksToWait 设置为 portMAX_DELAY 将使任务无限期等待（不超时），前提是 FreeRTOSConfig.h 中将 INCLUDE_vTaskSuspend 设置为 1。任务处于阻塞状态时不占用任何 CPU 时间。
 *
 * @return 从消息缓冲读取的消息长度（字节数）（如果有）。如果 xMessageBufferReceive() 在消息变为可用之前超时，则返回 0。如果消息长度大于 xBufferLengthBytes，则该消息将保留在消息缓冲中，并返回 0。
 *
 * 示例用法：
 * @code{c}
 * void vAFunction( MessageBufferHandle_t xMessageBuffer )
 * {
 * uint8_t ucRxData[ 20 ];
 * size_t xReceivedBytes;
 * const TickType_t xBlockTime = pdMS_TO_TICKS( 20 );
 *
 *  // 从消息缓冲接收下一条消息。在阻塞状态（不占用任何 CPU 处理时间）
 *  // 最多等待 20ms，以等待消息变为可用。
 *  xReceivedBytes = xMessageBufferReceive( xMessageBuffer,
 *                                          ( void * ) ucRxData,
 *                                          sizeof( ucRxData ),
 *                                          xBlockTime );
 *
 *  if( xReceivedBytes > 0 )
 *  {
 *      // ucRxData 中包含一条长度为 xReceivedBytes 的消息。
 *      // 在此处处理该消息...
 *  }
 * }
 * @endcode
 * \defgroup xMessageBufferReceive xMessageBufferReceive
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferReceive( xMessageBuffer, pvRxData, xBufferLengthBytes, xTicksToWait ) \
    xStreamBufferReceive( ( xMessageBuffer ), ( pvRxData ), ( xBufferLengthBytes ), ( xTicksToWait ) )


/**
 * message_buffer.h
 *
 * @code{c}
 * size_t xMessageBufferReceiveFromISR( MessageBufferHandle_t xMessageBuffer,
 *                                      void *pvRxData,
 *                                      size_t xBufferLengthBytes,
 *                                      BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 从消息缓冲接收一条离散消息的 API 函数的中断安全版本。消息可以是变长的，并将从缓冲区复制出来。
 *
 * ***注意***：在 FreeRTOS 各类对象中，流缓冲实现（以及构建于流缓冲之上的消息缓冲实现）的独特之处在于：它假定只有一个任务或中断会向缓冲区写入（写者），并且只有一个任务或中断会从缓冲区读取（读者）。写者与读者可以是不同的任务或中断，这一点是安全的；但与其他 FreeRTOS 对象不同，存在多个不同的写者或多个不同的读者时不安全。如果有多个不同的写者，应用编写者必须对写入 API 函数（例如 xStreamBufferSend()）的调用进行串行化。同样，如果有多个不同的读者，应用编写者必须对读取 API 函数（例如 xStreamBufferReceive()）的调用进行串行化。在单核或 SMP 内核中实现这种串行化的一种方法是：将每次 API 调用放入临界区，并使用为 0 的阻塞时间。
 *
 * 使用 xMessageBufferReceive() 从任务读取消息缓冲。使用 xMessageBufferReceiveFromISR() 从中断服务程序（ISR）读取消息缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferReceiveFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要接收其消息的消息缓冲的句柄。
 *
 * @param pvRxData 指向将复制所接收消息的缓冲区的指针。
 *
 * @param xBufferLengthBytes pvRxData 参数所指向缓冲区的长度。这设置了可接收消息的最大长度。如果 xBufferLengthBytes 太小，不足以容纳下一条消息，则该消息将保留在消息缓冲中，并返回 0。
 *
 * @param pxHigherPriorityTaskWoken 消息缓冲上可能有任务阻塞等待空间变为可用。调用 xMessageBufferReceiveFromISR() 可使空间变为可用，从而使等待空间的任务离开阻塞状态。如果调用 xMessageBufferReceiveFromISR() 导致某任务离开阻塞状态，且被解除阻塞的任务优先级高于当前正在执行的任务（被中断的任务），则 xMessageBufferReceiveFromISR() 会在内部将 *pxHigherPriorityTaskWoken 设置为 pdTRUE。如果 xMessageBufferReceiveFromISR() 将该值设置为 pdTRUE，通常应在退出中断之前执行一次上下文切换。这将确保中断直接返回到优先级最高的就绪态任务。在传入函数之前，应将 *pxHigherPriorityTaskWoken 设置为 pdFALSE。示例参见下面的代码示例。
 *
 * @return 从消息缓冲读取的消息长度（字节数）（如果有）。
 *
 * 示例用法：
 * @code{c}
 * // 一个已经创建好的消息缓冲。
 * MessageBufferHandle_t xMessageBuffer;
 *
 * void vAnInterruptServiceRoutine( void )
 * {
 * uint8_t ucRxData[ 20 ];
 * size_t xReceivedBytes;
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // 初始化为 pdFALSE。
 *
 *  // 从消息缓冲接收下一条消息。
 *  xReceivedBytes = xMessageBufferReceiveFromISR( xMessageBuffer,
 *                                                ( void * ) ucRxData,
 *                                                sizeof( ucRxData ),
 *                                                &xHigherPriorityTaskWoken );
 *
 *  if( xReceivedBytes > 0 )
 *  {
 *      // ucRxData 中包含一条长度为 xReceivedBytes 的消息。
 *      // 在此处处理该消息...
 *  }
 *
 *  // 如果在 xMessageBufferReceiveFromISR() 内部将 xHigherPriorityTaskWoken 设置为 pdTRUE，
 *  // 则说明有优先级高于当前正在执行任务的任务被解除阻塞，应执行上下文切换，
 *  // 以确保 ISR 返回到被解除阻塞的任务。在大多数 FreeRTOS 移植层中，
 *  // 只需将 xHigherPriorityTaskWoken 传给 portYIELD_FROM_ISR()，该宏会检查
 *  // 该变量的值，并在需要时执行上下文切换。具体移植层相关说明请查阅
 *  // 所用移植层的文档。
 *  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 * }
 * @endcode
 * \defgroup xMessageBufferReceiveFromISR xMessageBufferReceiveFromISR
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferReceiveFromISR( xMessageBuffer, pvRxData, xBufferLengthBytes, pxHigherPriorityTaskWoken ) \
    xStreamBufferReceiveFromISR( ( xMessageBuffer ), ( pvRxData ), ( xBufferLengthBytes ), ( pxHigherPriorityTaskWoken ) )

/**
 * message_buffer.h
 *
 * @code{c}
 * void vMessageBufferDelete( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 *
 * 删除先前通过调用 xMessageBufferCreate() 或 xMessageBufferCreateStatic() 创建的消息缓冲。如果消息缓冲是使用动态内存创建的（即通过 xMessageBufferCreate() 创建），则释放已分配的内存。
 *
 * 消息缓冲被删除后，不得再使用其句柄。
 *
 * 要在 FreeRTOSConfig.h 中使用 vMessageBufferDelete()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要删除的消息缓冲的句柄。
 */
#define vMessageBufferDelete( xMessageBuffer ) \
    vStreamBufferDelete( xMessageBuffer )

/**
 * message_buffer.h
 * @code{c}
 * BaseType_t xMessageBufferIsFull( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 *
 * 测试消息缓冲是否已满。如果消息缓冲无法再接收任何大小的消息，直到有消息从消息缓冲中移除、腾出空间为止，则消息缓冲为满。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferIsFull()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要查询的消息缓冲的句柄。
 *
 * @return 如果 xMessageBuffer 引用的消息缓冲已满则返回 pdTRUE，否则返回 pdFALSE。
 */
#define xMessageBufferIsFull( xMessageBuffer ) \
    xStreamBufferIsFull( xMessageBuffer )

/**
 * message_buffer.h
 * @code{c}
 * BaseType_t xMessageBufferIsEmpty( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 *
 * 测试消息缓冲是否为空（不包含任何消息）。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferIsEmpty()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要查询的消息缓冲的句柄。
 *
 * @return 如果 xMessageBuffer 引用的消息缓冲为空则返回 pdTRUE，否则返回 pdFALSE。
 */
#define xMessageBufferIsEmpty( xMessageBuffer ) \
    xStreamBufferIsEmpty( xMessageBuffer )

/**
 * message_buffer.h
 * @code{c}
 * BaseType_t xMessageBufferReset( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 *
 * 将消息缓冲重置为初始的空状态，并丢弃其中包含的任何消息。
 *
 * 只有在消息缓冲上没有任务阻塞时，才能重置消息缓冲。
 *
 * 使用 xMessageBufferReset() 从任务重置消息缓冲。使用 xMessageBufferResetFromISR() 从中断服务程序（ISR）重置消息缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferReset()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要重置的消息缓冲的句柄。
 *
 * @return 如果消息缓冲已重置则返回 pdPASS。如果无法重置消息缓冲，原因是有任务阻塞在消息队列上等待空间变为可用，或等待消息变为可用，则返回 pdFAIL。
 *
 * \defgroup xMessageBufferReset xMessageBufferReset
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferReset( xMessageBuffer ) \
    xStreamBufferReset( xMessageBuffer )


/**
 * message_buffer.h
 * @code{c}
 * BaseType_t xMessageBufferResetFromISR( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 *
 * 重置消息缓冲的 API 函数的中断安全版本。将消息缓冲重置为初始的空状态，并丢弃其中包含的任何消息。
 *
 * 只有在消息缓冲上没有任务阻塞时，才能重置消息缓冲。
 *
 * 使用 xMessageBufferReset() 从任务重置消息缓冲。使用 xMessageBufferResetFromISR() 从中断服务程序（ISR）重置消息缓冲。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferResetFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要重置的消息缓冲的句柄。
 *
 * @return 如果消息缓冲已重置则返回 pdPASS。如果无法重置消息缓冲，原因是有任务阻塞在消息队列上等待空间变为可用，或等待消息变为可用，则返回 pdFAIL。
 *
 * \defgroup xMessageBufferResetFromISR xMessageBufferResetFromISR
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferResetFromISR( xMessageBuffer ) \
    xStreamBufferResetFromISR( xMessageBuffer )

/**
 * message_buffer.h
 * @code{c}
 * size_t xMessageBufferSpaceAvailable( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 * 返回消息缓冲中空闲空间的字节数。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferSpaceAvailable()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要查询的消息缓冲的句柄。
 *
 * @return 在消息缓冲变满之前可写入消息缓冲的字节数。当消息写入消息缓冲时，还会额外写入 sizeof( size_t ) 个字节以保存消息长度。在 32 位架构上 sizeof( size_t ) 通常为 4 字节，因此如果 xMessageBufferSpacesAvailable() 返回 10，则可写入消息缓冲的最大消息大小为 6 字节。
 *
 * \defgroup xMessageBufferSpaceAvailable xMessageBufferSpaceAvailable
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferSpaceAvailable( xMessageBuffer ) \
    xStreamBufferSpacesAvailable( xMessageBuffer )
#define xMessageBufferSpacesAvailable( xMessageBuffer ) \
    xStreamBufferSpacesAvailable( xMessageBuffer ) /* 修正原始宏名称中的拼写错误。 */

/**
 * message_buffer.h
 * @code{c}
 * size_t xMessageBufferNextLengthBytes( MessageBufferHandle_t xMessageBuffer );
 * @endcode
 * 返回消息缓冲中下一条消息的长度（字节数）。如果 xMessageBufferReceive() 因传入的缓冲区太小而无法容纳下一条消息并返回 0，则该函数很有用。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferNextLengthBytes()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 要查询的消息缓冲的句柄。
 *
 * @return 消息缓冲中下一条消息的长度（字节数）；如果消息缓冲为空则返回 0。
 *
 * \defgroup xMessageBufferNextLengthBytes xMessageBufferNextLengthBytes
 * \ingroup MessageBufferManagement
 */
#define xMessageBufferNextLengthBytes( xMessageBuffer ) \
    xStreamBufferNextMessageLengthBytes( xMessageBuffer )

/**
 * message_buffer.h
 *
 * @code{c}
 * BaseType_t xMessageBufferSendCompletedFromISR( MessageBufferHandle_t xMessageBuffer, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 仅供高级用户使用。
 *
 * 当数据被发送到消息缓冲或流缓冲时，FreeRTOS API 内部会调用 sbSEND_COMPLETED() 宏。如果有任务阻塞在消息缓冲或流缓冲上等待数据到达，则 sbSEND_COMPLETED() 宏会向该任务发送通知，使其离开阻塞状态。xMessageBufferSendCompletedFromISR() 完成相同的工作。提供该函数是为了让应用编写者实现自己的 sbSEND_COMPLETED() 版本，且不得在任何其他时间使用。
 *
 * 有关更多信息，请参见 FreeRTOS/Demo/Minimal/MessageBufferAMP.c 中实现的示例。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferSendCompletedFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 已向其中写入数据的流缓冲的句柄。
 *
 * @param pxHigherPriorityTaskWoken 在将 *pxHigherPriorityTaskWoken 传入 xMessageBufferSendCompletedFromISR() 之前，应将其初始化为 pdFALSE。如果调用 xMessageBufferSendCompletedFromISR() 使某任务离开阻塞状态，且该任务优先级高于当前正在运行的任务，则 *pxHigherPriorityTaskWoken 将被设置为 pdTRUE，表示应在退出 ISR 之前执行上下文切换。
 *
 * @return 如果有任务被移出阻塞状态则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xMessageBufferSendCompletedFromISR xMessageBufferSendCompletedFromISR
 * \ingroup StreamBufferManagement
 */
#define xMessageBufferSendCompletedFromISR( xMessageBuffer, pxHigherPriorityTaskWoken ) \
    xStreamBufferSendCompletedFromISR( ( xMessageBuffer ), ( pxHigherPriorityTaskWoken ) )

/**
 * message_buffer.h
 *
 * @code{c}
 * BaseType_t xMessageBufferReceiveCompletedFromISR( MessageBufferHandle_t xMessageBuffer, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * 仅供高级用户使用。
 *
 * 当从消息缓冲或流缓冲读取数据时，FreeRTOS API 内部会调用 sbRECEIVE_COMPLETED() 宏。如果有任务阻塞在消息缓冲或流缓冲上等待数据到达，则 sbRECEIVE_COMPLETED() 宏会向该任务发送通知，使其离开阻塞状态。xMessageBufferReceiveCompletedFromISR() 完成相同的工作。提供该函数是为了让应用编写者实现自己的 sbRECEIVE_COMPLETED() 版本，且不得在任何其他时间使用。
 *
 * 有关更多信息，请参见 FreeRTOS/Demo/Minimal/MessageBufferAMP.c 中实现的示例。
 *
 * 要在 FreeRTOSConfig.h 中使用 xMessageBufferReceiveCompletedFromISR()，必须将 configUSE_STREAM_BUFFERS 设置为 1。
 *
 * @param xMessageBuffer 已从其中读取数据的流缓冲的句柄。
 *
 * @param pxHigherPriorityTaskWoken 在将 *pxHigherPriorityTaskWoken 传入 xMessageBufferReceiveCompletedFromISR() 之前，应将其初始化为 pdFALSE。如果调用 xMessageBufferReceiveCompletedFromISR() 使某任务离开阻塞状态，且该任务优先级高于当前正在运行的任务，则 *pxHigherPriorityTaskWoken 将被设置为 pdTRUE，表示应在退出 ISR 之前执行上下文切换。
 *
 * @return 如果有任务被移出阻塞状态则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xMessageBufferReceiveCompletedFromISR xMessageBufferReceiveCompletedFromISR
 * \ingroup StreamBufferManagement
 */
#define xMessageBufferReceiveCompletedFromISR( xMessageBuffer, pxHigherPriorityTaskWoken ) \
    xStreamBufferReceiveCompletedFromISR( ( xMessageBuffer ), ( pxHigherPriorityTaskWoken ) )

/* *INDENT-OFF*（关闭自动缩进） */
#if defined( __cplusplus )
    } /* extern "C"（C++ 链接声明结束） */
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* !defined( FREERTOS_MESSAGE_BUFFER_H )（头文件包含保护） */
