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


#ifndef QUEUE_H

/*
 * 【中文导读】queue.h —— 队列/信号量 API（B级导读）
 * 注意信号量宏（xSemaphoreCreateBinary/Mutex/Counting）都是 xQueueCreate
 * 的马甲——"信号量即队列"在此得到验证。
 */
#define QUEUE_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h" must appear in source files before "include queue.h"
#endif

#include "task.h"

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进） */

/**
 * 用于引用队列的类型。例如，调用 xQueueCreate() 返回的
 * QueueHandle_t 变量可作为参数传给 xQueueSend()、xQueueReceive() 等函数。
 */
struct QueueDefinition; /* 采用旧命名约定，以免破坏内核感知调试器。 */
typedef struct QueueDefinition   * QueueHandle_t;

/**
 * 用于引用队列集的类型。例如，调用 xQueueCreateSet() 返回的 xQueueSet
 * 变量可作为参数传给 xQueueSelectFromSet()、xQueueAddToSet() 等函数。
 */
typedef struct QueueDefinition   * QueueSetHandle_t;

/**
 * 队列集中既可以包含队列，也可以包含信号量，因此定义
 * QueueSetMemberHandle_t 类型，用于参数或返回值可能为 QueueHandle_t
 * 或 SemaphoreHandle_t 的场合。
 */
typedef struct QueueDefinition   * QueueSetMemberHandle_t;

/* 仅供内部使用。 */
#define queueSEND_TO_BACK                     ( ( BaseType_t ) 0 )
#define queueSEND_TO_FRONT                    ( ( BaseType_t ) 1 )
#define queueOVERWRITE                        ( ( BaseType_t ) 2 )

/* 仅供内部使用。这些定义*必须*与 queue.c 中的定义一致。 */
#define queueQUEUE_TYPE_BASE                  ( ( uint8_t ) 0U )
#define queueQUEUE_TYPE_MUTEX                 ( ( uint8_t ) 1U )
#define queueQUEUE_TYPE_COUNTING_SEMAPHORE    ( ( uint8_t ) 2U )
#define queueQUEUE_TYPE_BINARY_SEMAPHORE      ( ( uint8_t ) 3U )
#define queueQUEUE_TYPE_RECURSIVE_MUTEX       ( ( uint8_t ) 4U )
#define queueQUEUE_TYPE_SET                   ( ( uint8_t ) 5U )

/**
 * queue. h
 * @code{c}
 * QueueHandle_t xQueueCreate(
 *                            UBaseType_t uxQueueLength,
 *                            UBaseType_t uxItemSize
 *                        );
 * @endcode
 *
 * 创建新的队列实例，并返回可用于引用该新队列的句柄。
 *
 * 在 FreeRTOS 实现内部，队列使用两块内存：第一块用于存放队列的数据结构，
 * 第二块用于存放放入队列的项。若使用 xQueueCreate() 创建队列，这两块内存
 * 将在 xQueueCreate() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xQueueCreateStatic() 创建
 * 队列，则应用程序编写者必须提供队列所使用的内存。因此，
 * xQueueCreateStatic() 允许在不使用任何动态内存分配的情况下创建队列。
 *
 * https://www.FreeRTOS.org/Embedded-RTOS-Queues.html
 *
 * @param uxQueueLength 队列可容纳的最大项数。
 *
 * @param uxItemSize 队列中每一项所需的字节数。项按值（拷贝）入队，而不是
 * 按引用，因此该值为每投递一项所复制的字节数。队列中的每一项必须具有
 * 相同的大小。
 *
 * @return 若队列创建成功，则返回新创建队列的句柄；若无法创建队列，
 * 则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * };
 *
 * void vATask( void *pvParameters )
 * {
 * QueueHandle_t xQueue1, xQueue2;
 *
 *  // 创建可容纳 10 个 uint32_t 值的队列。
 *  xQueue1 = xQueueCreate( 10, sizeof( uint32_t ) );
 *  if( xQueue1 == NULL )
 *  {
 *      // 队列未创建成功，不得使用。
 *  }
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *  if( xQueue2 == NULL )
 *  {
 *      // 队列未创建成功，不得使用。
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueCreate xQueueCreate
 * \ingroup QueueManagement
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    #define xQueueCreate( uxQueueLength, uxItemSize )    xQueueGenericCreate( ( uxQueueLength ), ( uxItemSize ), ( queueQUEUE_TYPE_BASE ) )
#endif

/**
 * queue. h
 * @code{c}
 * QueueHandle_t xQueueCreateStatic(
 *                            UBaseType_t uxQueueLength,
 *                            UBaseType_t uxItemSize,
 *                            uint8_t *pucQueueStorage,
 *                            StaticQueue_t *pxQueueBuffer
 *                        );
 * @endcode
 *
 * 创建新的队列实例，并返回可用于引用该新队列的句柄。
 *
 * 在 FreeRTOS 实现内部，队列使用两块内存：第一块用于存放队列的数据结构，
 * 第二块用于存放放入队列的项。若使用 xQueueCreate() 创建队列，这两块内存
 * 将在 xQueueCreate() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xQueueCreateStatic() 创建
 * 队列，则应用程序编写者必须提供队列所使用的内存。因此，
 * xQueueCreateStatic() 允许在不使用任何动态内存分配的情况下创建队列。
 *
 * https://www.FreeRTOS.org/Embedded-RTOS-Queues.html
 *
 * @param uxQueueLength 队列可容纳的最大项数。
 *
 * @param uxItemSize 队列中每一项所需的字节数。项按值（拷贝）入队，而不是
 * 按引用，因此该值为每投递一项所复制的字节数。队列中的每一项必须具有
 * 相同的大小。
 *
 * @param pucQueueStorage 若 uxItemSize 不为零，则 pucQueueStorage 必须指向
 * 一个 uint8_t 数组，其大小至少足以容纳队列在任一时刻可包含的最大项数，
 * 即 ( uxQueueLength * uxItemSize ) 字节。若 uxItemSize 为零，则
 * pucQueueStorage 可为 NULL。
 *
 * @param pxQueueBuffer 必须指向一个 StaticQueue_t 类型的变量，用于存放
 * 队列的数据结构。
 *
 * @return 若队列创建成功，则返回所创建队列的句柄；若 pxQueueBuffer 为 NULL，
 * 则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * };
 *
 * #define QUEUE_LENGTH 10
 * #define ITEM_SIZE sizeof( uint32_t )
 *
 * // xQueueBuffer 用于存放队列结构体。
 * StaticQueue_t xQueueBuffer;
 *
 * // ucQueueStorage 用于存放投递到队列的项。其长度至少应为
 * // [(队列长度) * (队列项大小)] 字节。
 * uint8_t ucQueueStorage[ QUEUE_LENGTH * ITEM_SIZE ];
 *
 * void vATask( void *pvParameters )
 * {
 *  QueueHandle_t xQueue1;
 *
 *  // 创建可容纳 10 个 uint32_t 值的队列。
 *  xQueue1 = xQueueCreateStatic( QUEUE_LENGTH,             // 队列可容纳的项数。
 *                                ITEM_SIZE,                // 队列中每一项的大小。
 *                                &( ucQueueStorage[ 0 ] ), // 用于存放队列项的缓冲区。
 *                                &xQueueBuffer );          // 用于存放队列结构体的缓冲区。
 *
 *  // 由于未使用动态内存分配，可保证队列创建成功。
 *  // 因此 xQueue1 现在是指向有效队列的句柄。
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueCreateStatic xQueueCreateStatic
 * \ingroup QueueManagement
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #define xQueueCreateStatic( uxQueueLength, uxItemSize, pucQueueStorage, pxQueueBuffer )    xQueueGenericCreateStatic( ( uxQueueLength ), ( uxItemSize ), ( pucQueueStorage ), ( pxQueueBuffer ), ( queueQUEUE_TYPE_BASE ) )
#endif /* configSUPPORT_STATIC_ALLOCATION（静态分配支持） */

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueGetStaticBuffers( QueueHandle_t xQueue,
 *                                    uint8_t ** ppucQueueStorage,
 *                                    StaticQueue_t ** ppxStaticQueue );
 * @endcode
 *
 * 获取指向静态创建队列的数据结构缓冲区与存储区缓冲区的指针。
 * 这些缓冲区与创建队列时提供的缓冲区相同。
 *
 * @param xQueue 要获取缓冲区的队列。
 *
 * @param ppucQueueStorage 用于返回指向队列存储区缓冲区的指针。
 *
 * @param ppxStaticQueue 用于返回指向队列数据结构缓冲区的指针。
 *
 * @return 若成功获取缓冲区则返回 pdTRUE，否则返回 pdFALSE。
 *
 * \defgroup xQueueGetStaticBuffers xQueueGetStaticBuffers
 * \ingroup QueueManagement
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #define xQueueGetStaticBuffers( xQueue, ppucQueueStorage, ppxStaticQueue )    xQueueGenericGetStaticBuffers( ( xQueue ), ( ppucQueueStorage ), ( ppxStaticQueue ) )
#endif /* configSUPPORT_STATIC_ALLOCATION（静态分配支持） */

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueSendToFront(
 *                                 QueueHandle_t    xQueue,
 *                                 const void       *pvItemToQueue,
 *                                 TickType_t       xTicksToWait
 *                             );
 * @endcode
 *
 * 向队列前端投递一项。项按值（拷贝）入队，而不是按引用。不得在中断服务
 * 程序中调用此函数；可改用 xQueueSendFromISR()，它可在 ISR 中使用。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param xTicksToWait 若队列已满，任务等待队列出现可用空间的最长时间。
 * 若该值为 0 且队列已满，则调用立即返回。时间以 tick 周期为单位，
 * 若需转换为实际时间，应使用常量 portTICK_PERIOD_MS。
 *
 * @return 若成功投递则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * } xMessage;
 *
 * uint32_t ulVar = 10U;
 *
 * void vATask( void *pvParameters )
 * {
 * QueueHandle_t xQueue1, xQueue2;
 * struct AMessage *pxMessage;
 *
 *  // 创建可容纳 10 个 uint32_t 值的队列。
 *  xQueue1 = xQueueCreate( 10, sizeof( uint32_t ) );
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *
 *  // ...
 *
 *  if( xQueue1 != 0 )
 *  {
 *      // 发送一个 uint32_t。若有必要，等待 10 个 tick
 *      // 直到出现可用空间。
 *      if( xQueueSendToFront( xQueue1, ( void * ) &ulVar, ( TickType_t ) 10 ) != pdPASS )
 *      {
 *          // 即使等待 10 个 tick 后仍未能投递消息。
 *      }
 *  }
 *
 *  if( xQueue2 != 0 )
 *  {
 *      // 发送指向 struct AMessage 对象的指针。若队列已满，
 *      // 不要阻塞。
 *      pxMessage = & xMessage;
 *      xQueueSendToFront( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0 );
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueSend xQueueSend
 * \ingroup QueueManagement
 */
#define xQueueSendToFront( xQueue, pvItemToQueue, xTicksToWait ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), ( xTicksToWait ), queueSEND_TO_FRONT )

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueSendToBack(
 *                                 QueueHandle_t    xQueue,
 *                                 const void       *pvItemToQueue,
 *                                 TickType_t       xTicksToWait
 *                             );
 * @endcode
 *
 * 此宏调用 xQueueGenericSend()。
 *
 * 向队列后端投递一项。项按值（拷贝）入队，而不是按引用。不得在中断服务
 * 程序中调用此函数；可改用 xQueueSendFromISR()，它可在 ISR 中使用。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param xTicksToWait 若队列已满，任务等待队列出现可用空间的最长时间。
 * 若该值为 0 且队列已满，则调用立即返回。时间以 tick 周期为单位，
 * 若需转换为实际时间，应使用常量 portTICK_PERIOD_MS。
 *
 * @return 若成功投递则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * } xMessage;
 *
 * uint32_t ulVar = 10U;
 *
 * void vATask( void *pvParameters )
 * {
 * QueueHandle_t xQueue1, xQueue2;
 * struct AMessage *pxMessage;
 *
 *  // 创建可容纳 10 个 uint32_t 值的队列。
 *  xQueue1 = xQueueCreate( 10, sizeof( uint32_t ) );
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *
 *  // ...
 *
 *  if( xQueue1 != 0 )
 *  {
 *      // 发送一个 uint32_t。若有必要，等待 10 个 tick
 *      // 直到出现可用空间。
 *      if( xQueueSendToBack( xQueue1, ( void * ) &ulVar, ( TickType_t ) 10 ) != pdPASS )
 *      {
 *          // 即使等待 10 个 tick 后仍未能投递消息。
 *      }
 *  }
 *
 *  if( xQueue2 != 0 )
 *  {
 *      // 发送指向 struct AMessage 对象的指针。若队列已满，
 *      // 不要阻塞。
 *      pxMessage = & xMessage;
 *      xQueueSendToBack( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0 );
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueSend xQueueSend
 * \ingroup QueueManagement
 */
#define xQueueSendToBack( xQueue, pvItemToQueue, xTicksToWait ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), ( xTicksToWait ), queueSEND_TO_BACK )

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueSend(
 *                            QueueHandle_t xQueue,
 *                            const void * pvItemToQueue,
 *                            TickType_t xTicksToWait
 *                       );
 * @endcode
 *
 * 此宏调用 xQueueGenericSend()。保留它是为了向后兼容未提供
 * xQueueSendToFront() 和 xQueueSendToBack() 宏的 FreeRTOS.org 版本。
 * 它等效于 xQueueSendToBack()。
 *
 * 向队列投递一项。项按值（拷贝）入队，而不是按引用。不得在中断服务
 * 程序中调用此函数；可改用 xQueueSendFromISR()，它可在 ISR 中使用。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param xTicksToWait 若队列已满，任务等待队列出现可用空间的最长时间。
 * 若该值为 0 且队列已满，则调用立即返回。时间以 tick 周期为单位，
 * 若需转换为实际时间，应使用常量 portTICK_PERIOD_MS。
 *
 * @return 若成功投递则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * } xMessage;
 *
 * uint32_t ulVar = 10U;
 *
 * void vATask( void *pvParameters )
 * {
 * QueueHandle_t xQueue1, xQueue2;
 * struct AMessage *pxMessage;
 *
 *  // 创建可容纳 10 个 uint32_t 值的队列。
 *  xQueue1 = xQueueCreate( 10, sizeof( uint32_t ) );
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *
 *  // ...
 *
 *  if( xQueue1 != 0 )
 *  {
 *      // 发送一个 uint32_t。若有必要，等待 10 个 tick
 *      // 直到出现可用空间。
 *      if( xQueueSend( xQueue1, ( void * ) &ulVar, ( TickType_t ) 10 ) != pdPASS )
 *      {
 *          // 即使等待 10 个 tick 后仍未能投递消息。
 *      }
 *  }
 *
 *  if( xQueue2 != 0 )
 *  {
 *      // 发送指向 struct AMessage 对象的指针。若队列已满，
 *      // 不要阻塞。
 *      pxMessage = & xMessage;
 *      xQueueSend( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0 );
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueSend xQueueSend
 * \ingroup QueueManagement
 */
#define xQueueSend( xQueue, pvItemToQueue, xTicksToWait ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), ( xTicksToWait ), queueSEND_TO_BACK )

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueOverwrite(
 *                            QueueHandle_t xQueue,
 *                            const void * pvItemToQueue
 *                       );
 * @endcode
 *
 * 仅适用于长度为 1 的队列，即队列要么为空，要么已满。
 *
 * 向队列投递一项。若队列已满，则覆盖队列中保存的值。项按值（拷贝）
 * 入队，而不是按引用。
 *
 * 不得在中断服务程序中调用此函数。可改用 xQueueOverwriteFromISR()，
 * 它可在 ISR 中使用。
 *
 * @param xQueue 数据要发送到的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @return xQueueOverwrite() 是调用 xQueueGenericSend() 的宏，因此返回值
 * 与 xQueueSendToFront() 相同。但由于即使队列已满 xQueueOverwrite()
 * 也会写入队列，所以唯一可能返回的值是 pdPASS。
 *
 * 示例用法：
 * @code{c}
 *
 * void vFunction( void *pvParameters )
 * {
 * QueueHandle_t xQueue;
 * uint32_t ulVarToSend, ulValReceived;
 *
 *  // 创建用于保存一个 uint32_t 值的队列。强烈建议
 *  // *不要*在可包含多个值的队列上使用 xQueueOverwrite()；
 *  // 否则若定义了 configASSERT()，将触发断言。
 *  xQueue = xQueueCreate( 1, sizeof( uint32_t ) );
 *
 *  // 使用 xQueueOverwrite() 将值 10 写入队列。
 *  ulVarToSend = 10;
 *  xQueueOverwrite( xQueue, &ulVarToSend );
 *
 *  // 此时窥视（peek）队列应返回 10，但值 10 仍保留在
 *  // 队列中。由于已知队列中有值，阻塞时间设为 0。
 *  ulValReceived = 0;
 *  xQueuePeek( xQueue, &ulValReceived, 0 );
 *
 *  if( ulValReceived != 10 )
 *  {
 *      // 除非该项已被其他任务移除，否则说明出错。
 *  }
 *
 *  // 队列仍为满。使用 xQueueOverwrite() 将队列中保存的
 *  // 值覆盖为 100。
 *  ulVarToSend = 100;
 *  xQueueOverwrite( xQueue, &ulVarToSend );
 *
 *  // 这次从队列读取，使队列再次变为空。
 *  // 阻塞时间同样设为 0。
 *  xQueueReceive( xQueue, &ulValReceived, 0 );
 *
 *  // 读出的值应为最后写入的值，即使写入时
 *  // 队列已经为满。
 *  if( ulValReceived != 100 )
 *  {
 *      // 出错！
 *  }
 *
 *  // ...
 * }
 * @endcode
 * \defgroup xQueueOverwrite xQueueOverwrite
 * \ingroup QueueManagement
 */
#define xQueueOverwrite( xQueue, pvItemToQueue ) \
    xQueueGenericSend( ( xQueue ), ( pvItemToQueue ), 0, queueOVERWRITE )


/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueGenericSend(
 *                                  QueueHandle_t xQueue,
 *                                  const void * pvItemToQueue,
 *                                  TickType_t xTicksToWait,
 *                                  BaseType_t xCopyPosition
 *                              );
 * @endcode
 *
 * 建议使用宏 xQueueSend()、xQueueSendToFront() 和 xQueueSendToBack()，
 * 而不是直接调用此函数。
 *
 * 向队列投递一项。项按值（拷贝）入队，而不是按引用。不得在中断服务
 * 程序中调用此函数；可改用 xQueueSendFromISR()，它可在 ISR 中使用。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param xTicksToWait 若队列已满，任务等待队列出现可用空间的最长时间。
 * 若该值为 0 且队列已满，则调用立即返回。时间以 tick 周期为单位，
 * 若需转换为实际时间，应使用常量 portTICK_PERIOD_MS。
 *
 * @param xCopyPosition 可取 queueSEND_TO_BACK，将项放入队列后端；
 * 或取 queueSEND_TO_FRONT，将项放入队列前端（用于高优先级消息）。
 *
 * @return 若成功投递则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * } xMessage;
 *
 * uint32_t ulVar = 10U;
 *
 * void vATask( void *pvParameters )
 * {
 * QueueHandle_t xQueue1, xQueue2;
 * struct AMessage *pxMessage;
 *
 *  // 创建可容纳 10 个 uint32_t 值的队列。
 *  xQueue1 = xQueueCreate( 10, sizeof( uint32_t ) );
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *
 *  // ...
 *
 *  if( xQueue1 != 0 )
 *  {
 *      // 发送一个 uint32_t。若有必要，等待 10 个 tick
 *      // 直到出现可用空间。
 *      if( xQueueGenericSend( xQueue1, ( void * ) &ulVar, ( TickType_t ) 10, queueSEND_TO_BACK ) != pdPASS )
 *      {
 *          // 即使等待 10 个 tick 后仍未能投递消息。
 *      }
 *  }
 *
 *  if( xQueue2 != 0 )
 *  {
 *      // 发送指向 struct AMessage 对象的指针。若队列已满，
 *      // 不要阻塞。
 *      pxMessage = & xMessage;
 *      xQueueGenericSend( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0, queueSEND_TO_BACK );
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueSend xQueueSend
 * \ingroup QueueManagement
 */
BaseType_t xQueueGenericSend( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              TickType_t xTicksToWait,
                              const BaseType_t xCopyPosition ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueuePeek(
 *                           QueueHandle_t xQueue,
 *                           void * const pvBuffer,
 *                           TickType_t xTicksToWait
 *                       );
 * @endcode
 *
 * 从队列读取一项，但不将其从队列中移除。项按值（拷贝）接收，因此必须
 * 提供足够大的缓冲区。复制到缓冲区的字节数在创建队列时已确定。
 *
 * 成功接收的项仍保留在队列中，因此下一次调用或调用 xQueueReceive() 时
 * 会再次返回。
 *
 * 不得在中断服务程序中使用此宏。可改用 xQueuePeekFromISR()。
 *
 * @param xQueue 要从中接收项的队列句柄。
 *
 * @param pvBuffer 指向用于接收所读取项的缓冲区的指针。
 *
 * @param xTicksToWait 若调用时队列为空，任务等待可接收项的最长时间。
 * 时间以 tick 周期为单位，若需转换为实际时间，应使用常量
 * portTICK_PERIOD_MS。若 xTicksToWait 为 0 且队列为空，
 * xQueuePeek() 将立即返回。
 *
 * @return 若成功从队列接收到项则返回 pdPASS，否则返回 errQUEUE_EMPTY。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * } xMessage;
 *
 * QueueHandle_t xQueue;
 *
 * // 用于创建队列并投递值的任务。
 * void vATask( void *pvParameters )
 * {
 * struct AMessage *pxMessage;
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *  if( xQueue == 0 )
 *  {
 *      // 队列创建失败。
 *  }
 *
 *  // ...
 *
 *  // 发送指向 struct AMessage 对象的指针。若队列已满，
 *  // 不要阻塞。
 *  pxMessage = & xMessage;
 *  xQueueSend( xQueue, ( void * ) &pxMessage, ( TickType_t ) 0 );
 *
 *  // ... 任务其余代码。
 * }
 *
 * // 用于从队列窥视（peek）数据的任务。
 * void vADifferentTask( void *pvParameters )
 * {
 * struct AMessage *pxRxedMessage;
 *
 *  if( xQueue != 0 )
 *  {
 *      // 在已创建的队列上窥视消息。若消息不能立即获取，
 *      // 则阻塞 10 个 tick。
 *      if( xQueuePeek( xQueue, &( pxRxedMessage ), ( TickType_t ) 10 ) )
 *      {
 *          // 此时 pcRxedMessage 指向 vATask 投递的 struct AMessage
 *          // 变量，但该项仍保留在队列中。
 *      }
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueuePeek xQueuePeek
 * \ingroup QueueManagement
 */
BaseType_t xQueuePeek( QueueHandle_t xQueue,
                       void * const pvBuffer,
                       TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueuePeekFromISR(
 *                                  QueueHandle_t xQueue,
 *                                  void *pvBuffer,
 *                              );
 * @endcode
 *
 * 可在中断服务程序（ISR）中调用的 xQueuePeek() 版本。
 *
 * 从队列读取一项，但不将其从队列中移除。项按值（拷贝）接收，因此必须
 * 提供足够大的缓冲区。复制到缓冲区的字节数在创建队列时已确定。
 *
 * 成功接收的项仍保留在队列中，因此下一次调用或调用 xQueueReceive() 时
 * 会再次返回。
 *
 * @param xQueue 要从中接收项的队列句柄。
 *
 * @param pvBuffer 指向用于接收所读取项的缓冲区的指针。
 *
 * @return 若成功从队列接收到项则返回 pdPASS，否则返回 pdFAIL。
 *
 * \defgroup xQueuePeekFromISR xQueuePeekFromISR
 * \ingroup QueueManagement
 */
BaseType_t xQueuePeekFromISR( QueueHandle_t xQueue,
                              void * const pvBuffer ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueReceive(
 *                               QueueHandle_t xQueue,
 *                               void *pvBuffer,
 *                               TickType_t xTicksToWait
 *                          );
 * @endcode
 *
 * 从队列接收一项。项按值（拷贝）接收，因此必须提供足够大的缓冲区。
 * 复制到缓冲区的字节数在创建队列时已确定。
 *
 * 成功接收的项将从队列中移除。
 *
 * 不得在中断服务程序中使用此函数。可改用 xQueueReceiveFromISR。
 *
 * @param xQueue 要从中接收项的队列句柄。
 *
 * @param pvBuffer 指向用于接收所读取项的缓冲区的指针。
 *
 * @param xTicksToWait 若调用时队列为空，任务等待可接收项的最长时间。
 * 若 xTicksToWait 为零且队列为空，xQueueReceive() 将立即返回。
 * 时间以 tick 周期为单位，若需转换为实际时间，应使用常量
 * portTICK_PERIOD_MS。
 *
 * @return 若成功从队列接收到项则返回 pdPASS，否则返回 errQUEUE_EMPTY。
 *
 * 示例用法：
 * @code{c}
 * struct AMessage
 * {
 *  char ucMessageID;
 *  char ucData[ 20 ];
 * } xMessage;
 *
 * QueueHandle_t xQueue;
 *
 * // 用于创建队列并投递值的任务。
 * void vATask( void *pvParameters )
 * {
 * struct AMessage *pxMessage;
 *
 *  // 创建可容纳 10 个指向 AMessage 结构体指针的队列。
 *  // 这些结构体包含大量数据，应通过指针传递。
 *  xQueue = xQueueCreate( 10, sizeof( struct AMessage * ) );
 *  if( xQueue == 0 )
 *  {
 *      // 队列创建失败。
 *  }
 *
 *  // ...
 *
 *  // 发送指向 struct AMessage 对象的指针。若队列已满，
 *  // 不要阻塞。
 *  pxMessage = & xMessage;
 *  xQueueSend( xQueue, ( void * ) &pxMessage, ( TickType_t ) 0 );
 *
 *  // ... 任务其余代码。
 * }
 *
 * // 用于从队列接收数据的任务。
 * void vADifferentTask( void *pvParameters )
 * {
 * struct AMessage *pxRxedMessage;
 *
 *  if( xQueue != 0 )
 *  {
 *      // 在已创建的队列上接收消息。若消息不能立即获取，
 *      // 则阻塞 10 个 tick。
 *      if( xQueueReceive( xQueue, &( pxRxedMessage ), ( TickType_t ) 10 ) )
 *      {
 *          // 此时 pcRxedMessage 指向 vATask 投递的 struct AMessage
 *          // 变量。
 *      }
 *  }
 *
 *  // ... 任务其余代码。
 * }
 * @endcode
 * \defgroup xQueueReceive xQueueReceive
 * \ingroup QueueManagement
 */
BaseType_t xQueueReceive( QueueHandle_t xQueue,
                          void * const pvBuffer,
                          TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue );
 * @endcode
 *
 * 返回队列中保存的消息数量。
 *
 * @param xQueue 要查询的队列句柄。
 *
 * @return 队列中可用的消息数量。
 *
 * \defgroup uxQueueMessagesWaiting uxQueueMessagesWaiting
 * \ingroup QueueManagement
 */
UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * UBaseType_t uxQueueSpacesAvailable( const QueueHandle_t xQueue );
 * @endcode
 *
 * 返回队列中可用的空闲空间数量。若不移除任何项，该值等于在队列变满之前
 * 可发送到队列的项数。
 *
 * @param xQueue 要查询的队列句柄。
 *
 * @return 队列中可用的空间数量。
 *
 * \defgroup uxQueueMessagesWaiting uxQueueMessagesWaiting
 * \ingroup QueueManagement
 */
UBaseType_t uxQueueSpacesAvailable( const QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * void vQueueDelete( QueueHandle_t xQueue );
 * @endcode
 *
 * 删除队列，并释放为存储放入队列的项而分配的所有内存。
 *
 * @param xQueue 要删除的队列句柄。
 *
 * \defgroup vQueueDelete vQueueDelete
 * \ingroup QueueManagement
 */
void vQueueDelete( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueSendToFrontFromISR(
 *                                       QueueHandle_t xQueue,
 *                                       const void *pvItemToQueue,
 *                                       BaseType_t *pxHigherPriorityTaskWoken
 *                                    );
 * @endcode
 *
 * 此宏调用 xQueueGenericSendFromISR()。
 *
 * 向队列前端投递一项。在中断服务程序中使用此宏是安全的。
 *
 * 项按值（拷贝）入队，而不是按引用，因此最好只入队小项，尤其是在 ISR
 * 中调用时。大多数情况下，最好改为保存指向所入队项的指针。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param pxHigherPriorityTaskWoken 若向队列发送导致某个任务解除阻塞，且
 * 被解除阻塞的任务优先级高于当前正在运行的任务，
 * xQueueSendToFrontFromISR() 会将 *pxHigherPriorityTaskWoken 置为 pdTRUE。
 * 若 xQueueSendToFrontFromISR() 将该值置为 pdTRUE，则应在退出中断前
 * 请求一次上下文切换。
 *
 * @return 若数据成功发送到队列则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 带缓冲 IO 的示例用法（ISR 每次调用可获取多个值）：
 * @code{c}
 * void vBufferISR( void )
 * {
 * char cIn;
 * BaseType_t xHigherPriorityTaskWoken;
 *
 *  // ISR 开始时尚未唤醒任何任务。
 *  xHigherPriorityTaskWoken = pdFALSE;
 *
 *  // 循环直到缓冲区为空。
 *  do
 *  {
 *      // 从缓冲区获取一个字节。
 *      cIn = portINPUT_BYTE( RX_REGISTER_ADDRESS );
 *
 *      // 投递该字节。
 *      xQueueSendToFrontFromISR( xRxQueue, &cIn, &xHigherPriorityTaskWoken );
 *
 *  } while( portINPUT_BYTE( BUFFER_COUNT ) );
 *
 *  // 缓冲区现已为空，如有必要可切换上下文。
 *  if( xHigherPriorityTaskWoken )
 *  {
 *      // 由于 xHigherPriorityTaskWoken 现已置为 pdTRUE，
 *      // 应请求一次上下文切换。所用宏与移植层相关，
 *      // 可能是 portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()，
 *      // 请参阅所用移植层的文档。
 *      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *  }
 * }
 * @endcode
 *
 * \defgroup xQueueSendFromISR xQueueSendFromISR
 * \ingroup QueueManagement
 */
#define xQueueSendToFrontFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_FRONT )


/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueSendToBackFromISR(
 *                                       QueueHandle_t xQueue,
 *                                       const void *pvItemToQueue,
 *                                       BaseType_t *pxHigherPriorityTaskWoken
 *                                    );
 * @endcode
 *
 * 此宏调用 xQueueGenericSendFromISR()。
 *
 * 向队列后端投递一项。在中断服务程序中使用此宏是安全的。
 *
 * 项按值（拷贝）入队，而不是按引用，因此最好只入队小项，尤其是在 ISR
 * 中调用时。大多数情况下，最好改为保存指向所入队项的指针。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param pxHigherPriorityTaskWoken 若向队列发送导致某个任务解除阻塞，且
 * 被解除阻塞的任务优先级高于当前正在运行的任务，
 * xQueueSendToBackFromISR() 会将 *pxHigherPriorityTaskWoken 置为 pdTRUE。
 * 若 xQueueSendToBackFromISR() 将该值置为 pdTRUE，则应在退出中断前
 * 请求一次上下文切换。
 *
 * @return 若数据成功发送到队列则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 带缓冲 IO 的示例用法（ISR 每次调用可获取多个值）：
 * @code{c}
 * void vBufferISR( void )
 * {
 * char cIn;
 * BaseType_t xHigherPriorityTaskWoken;
 *
 *  // ISR 开始时尚未唤醒任何任务。
 *  xHigherPriorityTaskWoken = pdFALSE;
 *
 *  // 循环直到缓冲区为空。
 *  do
 *  {
 *      // 从缓冲区获取一个字节。
 *      cIn = portINPUT_BYTE( RX_REGISTER_ADDRESS );
 *
 *      // 投递该字节。
 *      xQueueSendToBackFromISR( xRxQueue, &cIn, &xHigherPriorityTaskWoken );
 *
 *  } while( portINPUT_BYTE( BUFFER_COUNT ) );
 *
 *  // 缓冲区现已为空，如有必要可切换上下文。
 *  if( xHigherPriorityTaskWoken )
 *  {
 *      // 由于 xHigherPriorityTaskWoken 现已置为 pdTRUE，
 *      // 应请求一次上下文切换。所用宏与移植层相关，
 *      // 可能是 portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()，
 *      // 请参阅所用移植层的文档。
 *      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *  }
 * }
 * @endcode
 *
 * \defgroup xQueueSendFromISR xQueueSendFromISR
 * \ingroup QueueManagement
 */
#define xQueueSendToBackFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_BACK )

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueOverwriteFromISR(
 *                            QueueHandle_t xQueue,
 *                            const void * pvItemToQueue,
 *                            BaseType_t *pxHigherPriorityTaskWoken
 *                       );
 * @endcode
 *
 * 可在中断服务程序（ISR）中使用的 xQueueOverwrite() 版本。
 *
 * 仅适用于只能保存一个项的队列，即队列要么为空，要么已满。
 *
 * 向队列投递一项。若队列已满，则覆盖队列中保存的值。项按值（拷贝）
 * 入队，而不是按引用。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param pxHigherPriorityTaskWoken 若向队列发送导致某个任务解除阻塞，且
 * 被解除阻塞的任务优先级高于当前正在运行的任务，
 * xQueueOverwriteFromISR() 会将 *pxHigherPriorityTaskWoken 置为 pdTRUE。
 * 若 xQueueOverwriteFromISR() 将该值置为 pdTRUE，则应在退出中断前
 * 请求一次上下文切换。
 *
 * @return xQueueOverwriteFromISR() 是调用 xQueueGenericSendFromISR() 的宏，
 * 因此返回值与 xQueueSendToFrontFromISR() 相同。但由于即使队列已满
 * xQueueOverwriteFromISR() 也会写入队列，所以唯一可能返回的值是 pdPASS。
 *
 * 示例用法：
 * @code{c}
 *
 * QueueHandle_t xQueue;
 *
 * void vFunction( void *pvParameters )
 * {
 *  // 创建用于保存一个 uint32_t 值的队列。强烈建议
 *  // *不要*在可包含多个值的队列上使用 xQueueOverwriteFromISR()；
 *  // 否则若定义了 configASSERT()，将触发断言。
 *  xQueue = xQueueCreate( 1, sizeof( uint32_t ) );
 * }
 *
 * void vAnInterruptHandler( void )
 * {
 * // 使用前必须将 xHigherPriorityTaskWoken 置为 pdFALSE。
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 * uint32_t ulVarToSend, ulValReceived;
 *
 *  // 使用 xQueueOverwriteFromISR() 将值 10 写入队列。
 *  ulVarToSend = 10;
 *  xQueueOverwriteFromISR( xQueue, &ulVarToSend, &xHigherPriorityTaskWoken );
 *
 *  // 队列已满，但再次调用 xQueueOverwriteFromISR() 仍会
 *  // 成功，因为队列中保存的值将被
 *  // 新值覆盖。
 *  ulVarToSend = 100;
 *  xQueueOverwriteFromISR( xQueue, &ulVarToSend, &xHigherPriorityTaskWoken );
 *
 *  // 此时从队列读取将返回 100。
 *
 *  // ...
 *
 *  if( xHigherPriorityTaskWoken == pdTRUE )
 *  {
 *      // 写入队列使某个任务解除阻塞，且被解除阻塞的任务
 *      // 优先级高于或等于当前正在执行的任务（即本中断所打断
 *      // 的任务）的优先级。应执行一次上下文切换，
 *      // 使本中断直接返回到被解除阻塞的任务。
 *      // 所用宏与移植层相关，可能是
 *      // portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()，请参阅
 *      // 所用移植层的文档。
 *      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *  }
 * }
 * @endcode
 * \defgroup xQueueOverwriteFromISR xQueueOverwriteFromISR
 * \ingroup QueueManagement
 */
#define xQueueOverwriteFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueOVERWRITE )

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueSendFromISR(
 *                                   QueueHandle_t xQueue,
 *                                   const void *pvItemToQueue,
 *                                   BaseType_t *pxHigherPriorityTaskWoken
 *                              );
 * @endcode
 *
 * 此宏调用 xQueueGenericSendFromISR()。保留它是为了向后兼容未提供
 * xQueueSendToBackFromISR() 和 xQueueSendToFrontFromISR() 宏的
 * FreeRTOS.org 版本。
 *
 * 向队列后端投递一项。在中断服务程序中使用此函数是安全的。
 *
 * 项按值（拷贝）入队，而不是按引用，因此最好只入队小项，尤其是在 ISR
 * 中调用时。大多数情况下，最好改为保存指向所入队项的指针。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param pxHigherPriorityTaskWoken 若向队列发送导致某个任务解除阻塞，且
 * 被解除阻塞的任务优先级高于当前正在运行的任务，xQueueSendFromISR()
 * 会将 *pxHigherPriorityTaskWoken 置为 pdTRUE。若 xQueueSendFromISR()
 * 将该值置为 pdTRUE，则应在退出中断前请求一次上下文切换。
 *
 * @return 若数据成功发送到队列则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 带缓冲 IO 的示例用法（ISR 每次调用可获取多个值）：
 * @code{c}
 * void vBufferISR( void )
 * {
 * char cIn;
 * BaseType_t xHigherPriorityTaskWoken;
 *
 *  // ISR 开始时尚未唤醒任何任务。
 *  xHigherPriorityTaskWoken = pdFALSE;
 *
 *  // 循环直到缓冲区为空。
 *  do
 *  {
 *      // 从缓冲区获取一个字节。
 *      cIn = portINPUT_BYTE( RX_REGISTER_ADDRESS );
 *
 *      // 投递该字节。
 *      xQueueSendFromISR( xRxQueue, &cIn, &xHigherPriorityTaskWoken );
 *
 *  } while( portINPUT_BYTE( BUFFER_COUNT ) );
 *
 *  // 缓冲区现已为空，如有必要可切换上下文。
 *  if( xHigherPriorityTaskWoken )
 *  {
 *       // 由于 xHigherPriorityTaskWoken 现已置为 pdTRUE，
 *       // 应请求一次上下文切换。所用宏与移植层相关，
 *       // 可能是 portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()，
 *       // 请参阅所用移植层的文档。
 *       portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *  }
 * }
 * @endcode
 *
 * \defgroup xQueueSendFromISR xQueueSendFromISR
 * \ingroup QueueManagement
 */
#define xQueueSendFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken ) \
    xQueueGenericSendFromISR( ( xQueue ), ( pvItemToQueue ), ( pxHigherPriorityTaskWoken ), queueSEND_TO_BACK )

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueGenericSendFromISR(
 *                                         QueueHandle_t    xQueue,
 *                                         const    void    *pvItemToQueue,
 *                                         BaseType_t  *pxHigherPriorityTaskWoken,
 *                                         BaseType_t  xCopyPosition
 *                                     );
 * @endcode
 *
 * 建议使用宏 xQueueSendFromISR()、xQueueSendToFrontFromISR() 和
 * xQueueSendToBackFromISR()，而不是直接调用此函数。对于不实际复制任何
 * 数据的信号量，xQueueGiveFromISR() 是等效函数。
 *
 * 向队列投递一项。在中断服务程序中使用此函数是安全的。
 *
 * 项按值（拷贝）入队，而不是按引用，因此最好只入队小项，尤其是在 ISR
 * 中调用时。大多数情况下，最好改为保存指向所入队项的指针。
 *
 * @param xQueue 要投递项的队列句柄。
 *
 * @param pvItemToQueue 指向要放入队列的项的指针。队列可容纳项的大小在
 * 创建队列时已确定，因此将从 pvItemToQueue 复制该数量的字节到队列
 * 存储区。
 *
 * @param pxHigherPriorityTaskWoken 若向队列发送导致某个任务解除阻塞，且
 * 被解除阻塞的任务优先级高于当前正在运行的任务，
 * xQueueGenericSendFromISR() 会将 *pxHigherPriorityTaskWoken 置为 pdTRUE。
 * 若 xQueueGenericSendFromISR() 将该值置为 pdTRUE，则应在退出中断前
 * 请求一次上下文切换。
 *
 * @param xCopyPosition 可取 queueSEND_TO_BACK，将项放入队列后端；
 * 或取 queueSEND_TO_FRONT，将项放入队列前端（用于高优先级消息）。
 *
 * @return 若数据成功发送到队列则返回 pdPASS，否则返回 errQUEUE_FULL。
 *
 * 带缓冲 IO 的示例用法（ISR 每次调用可获取多个值）：
 * @code{c}
 * void vBufferISR( void )
 * {
 * char cIn;
 * BaseType_t xHigherPriorityTaskWokenByPost;
 *
 *  // ISR 开始时尚未唤醒任何任务。
 *  xHigherPriorityTaskWokenByPost = pdFALSE;
 *
 *  // 循环直到缓冲区为空。
 *  do
 *  {
 *      // 从缓冲区获取一个字节。
 *      cIn = portINPUT_BYTE( RX_REGISTER_ADDRESS );
 *
 *      // 投递每个字节。
 *      xQueueGenericSendFromISR( xRxQueue, &cIn, &xHigherPriorityTaskWokenByPost, queueSEND_TO_BACK );
 *
 *  } while( portINPUT_BYTE( BUFFER_COUNT ) );
 *
 *  // 缓冲区现已为空，如有必要可切换上下文。
 *  if( xHigherPriorityTaskWokenByPost )
 *  {
 *       // 由于 xHigherPriorityTaskWokenByPost 现已置为 pdTRUE，
 *       // 应请求一次上下文切换。所用宏与移植层相关，
 *       // 可能是 portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()，
 *       // 请参阅所用移植层的文档。
 *       portYIELD_FROM_ISR( xHigherPriorityTaskWokenByPost );
 *  }
 * }
 * @endcode
 *
 * \defgroup xQueueSendFromISR xQueueSendFromISR
 * \ingroup QueueManagement
 */
BaseType_t xQueueGenericSendFromISR( QueueHandle_t xQueue,
                                     const void * const pvItemToQueue,
                                     BaseType_t * const pxHigherPriorityTaskWoken,
                                     const BaseType_t xCopyPosition ) PRIVILEGED_FUNCTION;
BaseType_t xQueueGiveFromISR( QueueHandle_t xQueue,
                              BaseType_t * const pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;

/**
 * queue. h
 * @code{c}
 * BaseType_t xQueueReceiveFromISR(
 *                                     QueueHandle_t    xQueue,
 *                                     void             *pvBuffer,
 *                                     BaseType_t       *pxTaskWoken
 *                                 );
 * @endcode
 *
 * 从队列接收一项。在中断服务程序中使用此函数是安全的。
 *
 * @param xQueue 要从中接收项的队列句柄。
 *
 * @param pvBuffer 指向用于接收所读取项的缓冲区的指针。
 *
 * @param pxHigherPriorityTaskWoken 可能有任务正阻塞等待队列出现可用空间。
 * 若 xQueueReceiveFromISR 使此类任务解除阻塞，*pxTaskWoken 将被置为
 * pdTRUE，否则 *pxTaskWoken 保持不变。
 *
 * @return 若成功从队列接收到项则返回 pdPASS，否则返回 pdFAIL。
 *
 * 示例用法：
 * @code{c}
 *
 * QueueHandle_t xQueue;
 *
 * // 用于创建队列并投递一些值的函数。
 * void vAFunction( void *pvParameters )
 * {
 * char cValueToPost;
 * const TickType_t xTicksToWait = ( TickType_t )0xff;
 *
 *  // 创建可容纳 10 个字符的队列。
 *  xQueue = xQueueCreate( 10, sizeof( char ) );
 *  if( xQueue == 0 )
 *  {
 *      // 队列创建失败。
 *  }
 *
 *  // ...
 *
 *  // 投递一些将在 ISR 中使用的字符。若队列
 *  // 已满，此任务将阻塞 xTicksToWait 个 tick。
 *  cValueToPost = 'a';
 *  xQueueSend( xQueue, ( void * ) &cValueToPost, xTicksToWait );
 *  cValueToPost = 'b';
 *  xQueueSend( xQueue, ( void * ) &cValueToPost, xTicksToWait );
 *
 *  // ... 继续投递字符 ... 若队列变满，此任务
 *  // 可能会阻塞。
 *
 *  cValueToPost = 'c';
 *  xQueueSend( xQueue, ( void * ) &cValueToPost, xTicksToWait );
 * }
 *
 * // 用于输出队列中接收到的所有字符的 ISR。
 * void vISR_Routine( void )
 * {
 * BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 * char cRxedChar;
 *
 *  while( xQueueReceiveFromISR( xQueue, ( void * ) &cRxedChar, &xHigherPriorityTaskWoken) )
 *  {
 *      // 已接收到一个字符。立即输出该字符。
 *      vOutputCharacter( cRxedChar );
 *
 *      // 若从队列移除字符唤醒了正在向队列
 *      // 投递的任务，则 xHigherPriorityTaskWoken 将被置为
 *      // pdTRUE。无论此循环迭代多少次，
 *      // 都只会唤醒一个任务。
 *  }
 *
 *  if( xHigherPriorityTaskWoken == pdTRUE )
 *  {
 *      // 由于 xHigherPriorityTaskWoken 现已置为 pdTRUE，
 *      // 应请求一次上下文切换。所用宏与移植层相关，
 *      // 可能是 portYIELD_FROM_ISR() 或 portEND_SWITCHING_ISR()，
 *      // 请参阅所用移植层的文档。
 *      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *  }
 * }
 * @endcode
 * \defgroup xQueueReceiveFromISR xQueueReceiveFromISR
 * \ingroup QueueManagement
 */
BaseType_t xQueueReceiveFromISR( QueueHandle_t xQueue,
                                 void * const pvBuffer,
                                 BaseType_t * const pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;

/*
 * 可在中断服务程序中安全使用的队列查询工具函数。这些工具函数只能
 * 在 ISR 内部或临界区内使用。
 */
BaseType_t xQueueIsQueueEmptyFromISR( const QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
BaseType_t xQueueIsQueueFullFromISR( const QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
UBaseType_t uxQueueMessagesWaitingFromISR( const QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

#if ( configUSE_CO_ROUTINES == 1 )

/*
 * 上述函数用于在任务之间传递数据。下面的函数则是用于
 * 在协程之间传递数据的等效函数。
 *
 * 这些函数由协程宏实现调用，不应由应用程序代码直接调用。
 * 请改用 croutine.h 中定义的宏封装。
 */
    BaseType_t xQueueCRSendFromISR( QueueHandle_t xQueue,
                                    const void * pvItemToQueue,
                                    BaseType_t xCoRoutinePreviouslyWoken );
    BaseType_t xQueueCRReceiveFromISR( QueueHandle_t xQueue,
                                       void * pvBuffer,
                                       BaseType_t * pxTaskWoken );
    BaseType_t xQueueCRSend( QueueHandle_t xQueue,
                             const void * pvItemToQueue,
                             TickType_t xTicksToWait );
    BaseType_t xQueueCRReceive( QueueHandle_t xQueue,
                                void * pvBuffer,
                                TickType_t xTicksToWait );

#endif /* if ( configUSE_CO_ROUTINES == 1 )（协程功能使能条件） */

/*
 * 仅供内部使用。请改用 xSemaphoreCreateMutex()、
 * xSemaphoreCreateCounting() 或 xSemaphoreGetMutexHolder()，
 * 不要直接调用这些函数。
 */
QueueHandle_t xQueueCreateMutex( const uint8_t ucQueueType ) PRIVILEGED_FUNCTION;

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    QueueHandle_t xQueueCreateMutexStatic( const uint8_t ucQueueType,
                                           StaticQueue_t * pxStaticQueue ) PRIVILEGED_FUNCTION;
#endif

#if ( configUSE_COUNTING_SEMAPHORES == 1 )
    QueueHandle_t xQueueCreateCountingSemaphore( const UBaseType_t uxMaxCount,
                                                 const UBaseType_t uxInitialCount ) PRIVILEGED_FUNCTION;
#endif

#if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    QueueHandle_t xQueueCreateCountingSemaphoreStatic( const UBaseType_t uxMaxCount,
                                                       const UBaseType_t uxInitialCount,
                                                       StaticQueue_t * pxStaticQueue ) PRIVILEGED_FUNCTION;
#endif

BaseType_t xQueueSemaphoreTake( QueueHandle_t xQueue,
                                TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )
    TaskHandle_t xQueueGetMutexHolder( QueueHandle_t xSemaphore ) PRIVILEGED_FUNCTION;
    TaskHandle_t xQueueGetMutexHolderFromISR( QueueHandle_t xSemaphore ) PRIVILEGED_FUNCTION;
#endif

/*
 * 仅供内部使用。请改用 xSemaphoreTakeRecursive() 或
 * xSemaphoreGiveRecursive()，不要直接调用这些函数。
 */
BaseType_t xQueueTakeMutexRecursive( QueueHandle_t xMutex,
                                     TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
BaseType_t xQueueGiveMutexRecursive( QueueHandle_t xMutex ) PRIVILEGED_FUNCTION;

/*
 * 将队列重置为初始的空状态。返回值现已废弃，始终为 pdPASS。
 */
#define xQueueReset( xQueue )    xQueueGenericReset( ( xQueue ), pdFALSE )

/*
 * 队列注册表为内核感知调试器提供定位队列、信号量和互斥量的手段。
 * 若希望句柄可供内核感知调试器使用，可调用 vQueueAddToRegistry()
 * 将队列、信号量或互斥量句柄添加到注册表。若未使用内核感知调试器，
 * 则可忽略此函数。
 *
 * configQUEUE_REGISTRY_SIZE 定义注册表能够保存的最大句柄数。
 * 要使注册表可用，FreeRTOSConfig.h 中的 configQUEUE_REGISTRY_SIZE
 * 必须大于 0。该值不影响可创建的队列、信号量和互斥量数量，
 * 仅影响注册表能够保存的数量。
 *
 * 若使用相同的 xQueue 参数多次调用 vQueueAddToRegistry，注册表将保存
 * 最近一次调用 vQueueAddToRegistry 时传入的 pcQueueName 参数。
 *
 * @param xQueue 要添加到注册表的队列句柄。该句柄由 xQueueCreate()
 * 调用返回。此处也可以传入信号量和互斥量句柄。
 *
 * @param pcQueueName 要与句柄关联的名称。该名称将显示在内核感知
 * 调试器中。队列注册表仅保存指向该字符串的指针，因此字符串必须
 * 持久存在（全局变量，最好位于 ROM/Flash 中），不能位于栈上。
 */
#if ( configQUEUE_REGISTRY_SIZE > 0 )
    void vQueueAddToRegistry( QueueHandle_t xQueue,
                              const char * pcQueueName ) PRIVILEGED_FUNCTION;
#endif

/*
 * 队列注册表为内核感知调试器提供定位队列、信号量和互斥量的手段。
 * 若希望句柄可供内核感知调试器使用，可调用 vQueueAddToRegistry()
 * 将队列、信号量或互斥量句柄添加到注册表，并调用
 * vQueueUnregisterQueue() 将其从注册表中移除。若未使用内核感知
 * 调试器，则可忽略此函数。
 *
 * @param xQueue 要从注册表中移除的队列句柄。
 */
#if ( configQUEUE_REGISTRY_SIZE > 0 )
    void vQueueUnregisterQueue( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
#endif

/*
 * 队列注册表为内核感知调试器提供定位队列、信号量和互斥量的手段。
 * 调用 pcQueueGetName() 可根据队列句柄查找并返回队列在
 * 队列注册表中的名称。
 *
 * @param xQueue 要返回其名称的队列句柄。
 * @return 若队列位于注册表中，则返回指向队列名称的指针；
 * 若队列不在注册表中，则返回 NULL。
 */
#if ( configQUEUE_REGISTRY_SIZE > 0 )
    const char * pcQueueGetName( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
#endif

/*
 * 使用动态内存分配创建队列的通用函数。创建其他以队列结构为基础的
 * RTOS 对象的函数和宏会调用此函数。
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    QueueHandle_t xQueueGenericCreate( const UBaseType_t uxQueueLength,
                                       const UBaseType_t uxItemSize,
                                       const uint8_t ucQueueType ) PRIVILEGED_FUNCTION;
#endif

/*
 * 使用动态内存分配创建队列的通用函数。创建其他以队列结构为基础的
 * RTOS 对象的函数和宏会调用此函数。
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    QueueHandle_t xQueueGenericCreateStatic( const UBaseType_t uxQueueLength,
                                             const UBaseType_t uxItemSize,
                                             uint8_t * pucQueueStorage,
                                             StaticQueue_t * pxStaticQueue,
                                             const uint8_t ucQueueType ) PRIVILEGED_FUNCTION;
#endif

/*
 * 用于获取静态创建队列缓冲区的通用函数。获取其他以队列结构为基础的
 * 静态创建 RTOS 对象缓冲区的函数和宏会调用此函数。
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xQueueGenericGetStaticBuffers( QueueHandle_t xQueue,
                                              uint8_t ** ppucQueueStorage,
                                              StaticQueue_t ** ppxStaticQueue ) PRIVILEGED_FUNCTION;
#endif

/*
 * 队列集提供一种机制，使任务能够同时在多个队列或信号量的读操作上
 * 阻塞（挂起）。
 *
 * 使用此函数的示例参见 FreeRTOS/Source/Demo/Common/Minimal/QueueSet.c。
 *
 * 必须先调用 xQueueCreateSet() 或 xQueueCreateSetStatic() 显式创建
 * 队列集，然后才能使用。创建后，可通过 xQueueAddToSet() 将标准
 * FreeRTOS 队列和信号量添加到队列集中。随后使用
 * xQueueSelectFromSet() 确定集合中是否有队列或信号量处于可成功
 * 执行队列读取或信号量获取（take）操作的状态。
 *
 * 注 1：关于为什么实践中很少需要队列集（因为存在更简单的多对象
 * 阻塞方法），请参阅
 * https://www.freertos.org/Documentation/02-Kernel/04-API-references/07-Queue-sets/00-RTOS-queue-sets
 * 上的文档。
 *
 * 注 2：在包含互斥量的队列集上阻塞，不会使互斥量持有者继承
 * 被阻塞任务的优先级。
 *
 * 注 3：加入队列集的每个队列的每个空间都需要额外 4 字节 RAM。
 * 因此，最大计数值较高的计数信号量不应加入队列集。
 *
 * 注 4：除非 xQueueSelectFromSet() 已先返回某个集合成员的句柄，
 * 否则不得对该队列集成员执行接收（针对队列）或获取（针对信号量）
 * 操作。
 *
 * @param uxEventQueueLength 队列集会保存集合中队列和信号量上发生的
 * 事件。uxEventQueueLength 指定一次最多可排队的事件数。若要绝对
 * 确保事件不丢失，uxEventQueueLength 应设置为加入集合的各队列长度
 * 之和，其中二值信号量和互斥量的长度为 1，计数信号量的长度由其
 * 最大计数值决定。示例：
 *  + 若队列集要包含一个长度为 5 的队列、另一个长度为 12 的队列
 *    以及一个二值信号量，则 uxEventQueueLength 应设为
 *    (5 + 12 + 1)，即 18。
 *  + 若队列集要包含三个二值信号量，则 uxEventQueueLength
 *    应设为 (1 + 1 + 1 )，即 3。
 *  + 若队列集要包含一个最大计数值为 5 的计数信号量，以及一个
 *    最大计数值为 3 的计数信号量，则 uxEventQueueLength 应设为
 *    (5 + 3)，即 8。
 *
 * @return 若队列集创建成功，则返回所创建队列集的句柄；否则返回 NULL。
 */
#if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    QueueSetHandle_t xQueueCreateSet( const UBaseType_t uxEventQueueLength ) PRIVILEGED_FUNCTION;
#endif

/*
 * 队列集提供一种机制，使任务能够同时在多个队列或信号量的读操作上
 * 阻塞（挂起）。
 *
 * 使用此函数的示例参见 FreeRTOS/Source/Demo/Common/Minimal/QueueSet.c。
 *
 * 必须先调用 xQueueCreateSet() 或 xQueueCreateSetStatic() 显式创建
 * 队列集，然后才能使用。创建后，可通过 xQueueAddToSet() 将标准
 * FreeRTOS 队列和信号量添加到队列集中。随后使用
 * xQueueSelectFromSet() 确定集合中是否有队列或信号量处于可成功
 * 执行队列读取或信号量获取（take）操作的状态。
 *
 * 注 1：关于为什么实践中很少需要队列集（因为存在更简单的多对象
 * 阻塞方法），请参阅
 * https://www.freertos.org/Documentation/02-Kernel/04-API-references/07-Queue-sets/00-RTOS-queue-sets
 * 上的文档。
 *
 * 注 2：在包含互斥量的队列集上阻塞，不会使互斥量持有者继承
 * 被阻塞任务的优先级。
 *
 * 注 3：加入队列集的每个队列的每个空间都需要额外 4 字节 RAM。
 * 因此，最大计数值较高的计数信号量不应加入队列集。
 *
 * 注 4：除非 xQueueSelectFromSet() 已先返回某个集合成员的句柄，
 * 否则不得对该队列集成员执行接收（针对队列）或获取（针对信号量）
 * 操作。
 *
 * @param uxEventQueueLength 队列集会保存集合中队列和信号量上发生的
 * 事件。uxEventQueueLength 指定一次最多可排队的事件数。若要绝对
 * 确保事件不丢失，uxEventQueueLength 应设置为加入集合的各队列长度
 * 之和，其中二值信号量和互斥量的长度为 1，计数信号量的长度由其
 * 最大计数值决定。示例：
 *  + 若队列集要包含一个长度为 5 的队列、另一个长度为 12 的队列
 *    以及一个二值信号量，则 uxEventQueueLength 应设为
 *    (5 + 12 + 1)，即 18。
 *  + 若队列集要包含三个二值信号量，则 uxEventQueueLength
 *    应设为 (1 + 1 + 1 )，即 3。
 *  + 若队列集要包含一个最大计数值为 5 的计数信号量，以及一个
 *    最大计数值为 3 的计数信号量，则 uxEventQueueLength 应设为
 *    (5 + 3)，即 8。
 *
 * @param pucQueueStorage pucQueueStorage 必须指向一个 uint8_t 数组，
 * 其大小至少足以容纳 uxEventQueueLength 个事件。
 *
 * @param pxQueueBuffer 必须指向一个 StaticQueue_t 类型的变量，用于
 * 存放队列的数据结构。
 *
 * @return 若队列集创建成功，则返回所创建队列集的句柄；若
 * pxQueueBuffer 为 NULL，则返回 NULL。
 */
#if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )
    QueueSetHandle_t xQueueCreateSetStatic( const UBaseType_t uxEventQueueLength,
                                            uint8_t * pucQueueStorage,
                                            StaticQueue_t * pxStaticQueue ) PRIVILEGED_FUNCTION;
#endif

/*
 * 将队列或信号量添加到先前由 xQueueCreateSet() 或
 * xQueueCreateSetStatic() 调用创建的队列集。
 *
 * 使用此函数的示例参见 FreeRTOS/Source/Demo/Common/Minimal/QueueSet.c。
 *
 * 注 1：除非 xQueueSelectFromSet() 已先返回某个集合成员的句柄，
 * 否则不得对该队列集成员执行接收（针对队列）或获取（针对信号量）
 * 操作。
 *
 * @param xQueueOrSemaphore 要添加到队列集的队列或信号量句柄
 * （强制转换为 QueueSetMemberHandle_t 类型）。
 *
 * @param xQueueSet 队列或信号量要添加到的队列集句柄。
 *
 * @return 若队列或信号量成功添加到队列集，则返回 pdPASS。若该队列
 * 已属于另一个队列集而无法成功添加，则返回 pdFAIL。
 */
#if ( configUSE_QUEUE_SETS == 1 )
    BaseType_t xQueueAddToSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                               QueueSetHandle_t xQueueSet ) PRIVILEGED_FUNCTION;
#endif

/*
 * 从队列集中移除队列或信号量。只有当队列或信号量为空时，
 * 才能将其从集合中移除。
 *
 * 使用此函数的示例参见 FreeRTOS/Source/Demo/Common/Minimal/QueueSet.c。
 *
 * @param xQueueOrSemaphore 要从队列集移除的队列或信号量句柄
 * （强制转换为 QueueSetMemberHandle_t 类型）。
 *
 * @param xQueueSet 包含该队列或信号量的队列集句柄。
 *
 * @return 若队列或信号量成功从队列集移除，则返回 pdPASS。若该队列
 * 不在队列集中，或队列（或信号量）非空，则返回 pdFAIL。
 */
#if ( configUSE_QUEUE_SETS == 1 )
    BaseType_t xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                                    QueueSetHandle_t xQueueSet ) PRIVILEGED_FUNCTION;
#endif

/*
 * xQueueSelectFromSet() 从队列集的成员中选择一个包含数据（针对队列）
 * 或可获取（针对信号量）的队列或信号量。xQueueSelectFromSet() 实际上
 * 允许任务同时在队列集中所有队列和信号量的读操作上阻塞（挂起）。
 *
 * 使用此函数的示例参见 FreeRTOS/Source/Demo/Common/Minimal/QueueSet.c。
 *
 * 注 1：关于为什么实践中很少需要队列集（因为存在更简单的多对象阻塞
 * 方法），请参阅
 * https://www.freertos.org/Documentation/02-Kernel/04-API-references/07-Queue-sets/00-RTOS-queue-sets
 * 上的文档。
 *
 * 注 2：在包含互斥量的队列集上阻塞，不会使互斥量持有者继承
 * 被阻塞任务的优先级。
 *
 * 注 3：除非 xQueueSelectFromSet() 已先返回某个集合成员的句柄，
 * 否则不得对该队列集成员执行接收（针对队列）或获取（针对信号量）
 * 操作。
 *
 * @param xQueueSet 任务将（可能）在其上阻塞的队列集。
 *
 * @param xTicksToWait 调用任务在阻塞状态（允许其他任务运行）下等待
 * 队列集某个成员变为可成功执行队列读取或信号量获取操作的最长时间，
 * 单位为 tick。
 *
 * @return xQueueSelectFromSet() 将返回队列集中包含数据的队列句柄
 * （强制转换为 QueueSetMemberHandle_t 类型），或队列集中可用的
 * 信号量句柄（强制转换为 QueueSetMemberHandle_t 类型）；若在指定
 * 阻塞时间到期前不存在这样的队列或信号量，则返回 NULL。
 */
#if ( configUSE_QUEUE_SETS == 1 )
    QueueSetMemberHandle_t xQueueSelectFromSet( QueueSetHandle_t xQueueSet,
                                                const TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;
#endif

/*
 * 可在 ISR 中使用的 xQueueSelectFromSet() 版本。
 */
#if ( configUSE_QUEUE_SETS == 1 )
    QueueSetMemberHandle_t xQueueSelectFromSetFromISR( QueueSetHandle_t xQueueSet ) PRIVILEGED_FUNCTION;
#endif

/* 非公共 API 函数。 */
void vQueueWaitForMessageRestricted( QueueHandle_t xQueue,
                                     TickType_t xTicksToWait,
                                     const BaseType_t xWaitIndefinitely ) PRIVILEGED_FUNCTION;
BaseType_t xQueueGenericReset( QueueHandle_t xQueue,
                               BaseType_t xNewQueue ) PRIVILEGED_FUNCTION;

#if ( configUSE_TRACE_FACILITY == 1 )
    void vQueueSetQueueNumber( QueueHandle_t xQueue,
                               UBaseType_t uxQueueNumber ) PRIVILEGED_FUNCTION;
#endif

#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxQueueGetQueueNumber( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
#endif

#if ( configUSE_TRACE_FACILITY == 1 )
    uint8_t ucQueueGetQueueType( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
#endif

UBaseType_t uxQueueGetQueueItemSize( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;
UBaseType_t uxQueueGetQueueLength( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* QUEUE_H（头文件保护宏） */
