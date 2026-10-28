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

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h" must appear in source files before "include semphr.h"
#endif

#include "queue.h"

typedef QueueHandle_t SemaphoreHandle_t;

#define semBINARY_SEMAPHORE_QUEUE_LENGTH    ( ( UBaseType_t ) 1U )
#define semSEMAPHORE_QUEUE_ITEM_LENGTH      ( ( UBaseType_t ) 0U )
#define semGIVE_BLOCK_TIME                  ( ( TickType_t ) 0U )


/**
 * semphr. h
 * @code{c}
 * vSemaphoreCreateBinary( SemaphoreHandle_t xSemaphore );
 * @endcode
 *
 * 在许多使用场景中，使用直接任务通知代替二值信号量速度更快、内存效率更高！
 * https://www.FreeRTOS.org/RTOS-task-notifications.html
 *
 * 旧的 vSemaphoreCreateBinary() 宏现已弃用，建议改用 xSemaphoreCreateBinary() 函数。
 * 注意：使用 vSemaphoreCreateBinary() 宏创建的二值信号量处于这样一种状态——
 * 第一次“获取（take）”该信号量会成功；而使用 xSemaphoreCreateBinary() 创建的
 * 二值信号量处于另一种状态——必须先“释放（give）”该信号量，然后才能“获取（take）”。
 *
 * <i>宏</i>，利用现有的队列机制实现信号量。
 * 由于这是二值信号量，队列长度设为 1。数据大小设为 0，
 * 因为并不需要真正存储任何数据——只需判断队列是空还是满。
 *
 * 这种信号量可用于任务之间或中断与任务之间的纯同步。
 * 信号量获取后无需归还，因此一个任务或中断可以持续“释放（give）”信号量，
 * 而另一个任务持续“获取（take）”信号量。正因如此，这种信号量
 * 不使用优先级继承机制。若需要采用优先级继承的替代方案，
 * 参见 xSemaphoreCreateMutex()。
 *
 * @param xSemaphore 所创建信号量的句柄。类型应为 SemaphoreHandle_t。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 在调用 vSemaphoreCreateBinary() 之前不能使用信号量。
 *  // 该接口是宏，因此直接传入变量。
 *  vSemaphoreCreateBinary( xSemaphore );
 *
 *  if( xSemaphore != NULL )
 *  {
 *      // 信号量创建成功。
 *      // 现在可以使用该信号量。
 *  }
 * }
 * @endcode
 * \defgroup vSemaphoreCreateBinary vSemaphoreCreateBinary
 * \ingroup Semaphores
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    #define vSemaphoreCreateBinary( xSemaphore )                                                                                     \
    do {                                                                                                                             \
        ( xSemaphore ) = xQueueGenericCreate( ( UBaseType_t ) 1, semSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_BINARY_SEMAPHORE ); \
        if( ( xSemaphore ) != NULL )                                                                                                 \
        {                                                                                                                            \
            ( void ) xSemaphoreGive( ( xSemaphore ) );                                                                               \
        }                                                                                                                            \
    } while( 0 )
#endif

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateBinary( void );
 * @endcode
 *
 * 创建一个新的二值信号量实例，并返回可用于引用该新信号量的句柄。
 *
 * 在许多使用场景中，使用直接任务通知代替二值信号量速度更快、内存效率更高！
 * https://www.FreeRTOS.org/RTOS-task-notifications.html
 *
 * 在 FreeRTOS 内部实现中，二值信号量使用一块内存来存储信号量结构。
 * 若使用 xSemaphoreCreateBinary() 创建二值信号量，则所需内存会在
 * xSemaphoreCreateBinary() 函数内部自动动态分配
 * （参见 https://www.FreeRTOS.org/a00111.html）。若使用
 * xSemaphoreCreateBinaryStatic() 创建二值信号量，则应用程序编写者
 * 必须提供内存。因此，xSemaphoreCreateBinaryStatic() 允许在不使用
 * 任何动态内存分配的情况下创建二值信号量。
 *
 * 旧的 vSemaphoreCreateBinary() 宏现已弃用，建议改用
 * xSemaphoreCreateBinary() 函数。注意：使用 vSemaphoreCreateBinary() 宏
 * 创建的二值信号量处于这样一种状态——第一次“获取（take）”该信号量会成功；
 * 而使用 xSemaphoreCreateBinary() 创建的二值信号量处于另一种状态——
 * 必须先“释放（give）”该信号量，然后才能“获取（take）”。
 *
 * 这种信号量可用于任务之间或中断与任务之间的纯同步。
 * 信号量获取后无需归还，因此一个任务或中断可以持续“释放（give）”信号量，
 * 而另一个任务持续“获取（take）”信号量。正因如此，这种信号量
 * 不使用优先级继承机制。若需要采用优先级继承的替代方案，
 * 参见 xSemaphoreCreateMutex()。
 *
 * @return 所创建信号量的句柄；若无法分配保存信号量数据结构所需的内存，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 在调用 xSemaphoreCreateBinary() 之前不能使用信号量。
 *  // 该接口是宏，因此直接传入变量。
 *  xSemaphore = xSemaphoreCreateBinary();
 *
 *  if( xSemaphore != NULL )
 *  {
 *      // 信号量创建成功。
 *      // 现在可以使用该信号量。
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreCreateBinary xSemaphoreCreateBinary
 * \ingroup Semaphores
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    #define xSemaphoreCreateBinary()    xQueueGenericCreate( ( UBaseType_t ) 1, semSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_BINARY_SEMAPHORE )
#endif

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateBinaryStatic( StaticSemaphore_t *pxSemaphoreBuffer );
 * @endcode
 *
 * 创建一个新的二值信号量实例，并返回可用于引用该新信号量的句柄。
 *
 * 注意：在许多使用场景中，使用直接任务通知代替二值信号量速度更快、内存效率更高！
 * https://www.FreeRTOS.org/RTOS-task-notifications.html
 *
 * 在 FreeRTOS 内部实现中，二值信号量使用一块内存来存储信号量结构。
 * 若使用 xSemaphoreCreateBinary() 创建二值信号量，则所需内存会在
 * xSemaphoreCreateBinary() 函数内部自动动态分配
 * （参见 https://www.FreeRTOS.org/a00111.html）。若使用
 * xSemaphoreCreateBinaryStatic() 创建二值信号量，则应用程序编写者
 * 必须提供内存。因此，xSemaphoreCreateBinaryStatic() 允许在不使用
 * 任何动态内存分配的情况下创建二值信号量。
 *
 * 这种信号量可用于任务之间或中断与任务之间的纯同步。
 * 信号量获取后无需归还，因此一个任务或中断可以持续“释放（give）”信号量，
 * 而另一个任务持续“获取（take）”信号量。正因如此，这种信号量
 * 不使用优先级继承机制。若需要采用优先级继承的替代方案，
 * 参见 xSemaphoreCreateMutex()。
 *
 * @param pxSemaphoreBuffer 必须指向 StaticSemaphore_t 类型的变量，该变量随后用于
 * 保存信号量的数据结构，从而无需动态分配内存。
 *
 * @return 若信号量创建成功，则返回所创建信号量的句柄。若 pxSemaphoreBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore = NULL;
 * StaticSemaphore_t xSemaphoreBuffer;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 在调用 xSemaphoreCreateBinary() 之前不能使用信号量。
 *  // 信号量的数据结构将存放在 xSemaphoreBuffer 变量中，
 *  // 该变量的地址会传入函数。函数参数不为 NULL，
 *  // 因此函数不会尝试任何动态内存分配，也就不会返回 NULL。
 *  xSemaphore = xSemaphoreCreateBinary( &xSemaphoreBuffer );
 *
 *  // 任务其余代码放在此处。
 * }
 * @endcode
 * \defgroup xSemaphoreCreateBinaryStatic xSemaphoreCreateBinaryStatic
 * \ingroup Semaphores
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #define xSemaphoreCreateBinaryStatic( pxStaticSemaphore )    xQueueGenericCreateStatic( ( UBaseType_t ) 1, semSEMAPHORE_QUEUE_ITEM_LENGTH, NULL, ( pxStaticSemaphore ), queueQUEUE_TYPE_BINARY_SEMAPHORE )
#endif /* configSUPPORT_STATIC_ALLOCATION 条件编译结束 */

/**
 * semphr. h
 * @code{c}
 * xSemaphoreTake(
 *                   SemaphoreHandle_t xSemaphore,
 *                   TickType_t xBlockTime
 *               );
 * @endcode
 *
 * <i>宏</i>，用于获取信号量。该信号量必须已通过调用 xSemaphoreCreateBinary()、
 * xSemaphoreCreateMutex() 或 xSemaphoreCreateCounting() 创建。
 *
 * @param xSemaphore 正在获取的信号量句柄——在创建信号量时获得。
 *
 * @param xBlockTime 等待信号量变为可用的时间，单位为时钟节拍（tick）。可使用宏
 * portTICK_PERIOD_MS 将其转换为实际时间。阻塞时间设为 0 可用于轮询信号量。
 * 阻塞时间设为 portMAX_DELAY 可用于无限期阻塞（前提是在 FreeRTOSConfig.h 中
 * 将 INCLUDE_vTaskSuspend 设为 1）。
 *
 * @return 若成功获取信号量则返回 pdTRUE。若 xBlockTime 超时后
 * 信号量仍不可用，则返回 pdFALSE。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 * // 创建信号量的任务。
 * void vATask( void * pvParameters )
 * {
 *  // 创建信号量以保护共享资源。
 *  xSemaphore = xSemaphoreCreateBinary();
 * }
 *
 * // 使用信号量的任务。
 * void vAnotherTask( void * pvParameters )
 * {
 *  // ... 执行其他操作。
 *
 *  if( xSemaphore != NULL )
 *  {
 *      // 尝试获取信号量。若信号量不可用，
 *      // 则等待 10 个时钟节拍，看它是否会变为可用。
 *      if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
 *      {
 *          // 成功获取信号量，现在可以访问
 *          // 共享资源。
 *
 *          // ...
 *
 *          // 已结束对共享资源的访问。释放
 *          // 信号量。
 *          xSemaphoreGive( xSemaphore );
 *      }
 *      else
 *      {
 *          // 未能获取信号量，因此无法安全地
 *          // 访问共享资源。
 *      }
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreTake xSemaphoreTake
 * \ingroup Semaphores
 */
#define xSemaphoreTake( xSemaphore, xBlockTime )    xQueueSemaphoreTake( ( xSemaphore ), ( xBlockTime ) )

/**
 * semphr. h
 * @code{c}
 * xSemaphoreTakeRecursive(
 *                          SemaphoreHandle_t xMutex,
 *                          TickType_t xBlockTime
 *                        );
 * @endcode
 *
 * <i>宏</i>，用于递归获取（take）互斥量类型的信号量。
 * 该互斥量必须已通过调用 xSemaphoreCreateRecursiveMutex() 创建。
 *
 * 要使此宏可用，必须在 FreeRTOSConfig.h 中将 configUSE_RECURSIVE_MUTEXES 设为 1。
 *
 * 此宏不得用于通过 xSemaphoreCreateMutex() 创建的互斥量。
 *
 * 递归使用的互斥量可被其所有者重复“获取（take）”。在所有者为每次成功的
 * “获取”请求调用 xSemaphoreGiveRecursive() 之前，该互斥量不会再次变为可用。
 * 例如，若某任务成功“获取”同一互斥量 5 次，则该任务也必须恰好
 * “释放”该互斥量 5 次之后，其他任务才能使用它。
 *
 * @param xMutex 正在获取的互斥量句柄。该句柄由 xSemaphoreCreateRecursiveMutex() 返回。
 *
 * @param xBlockTime 等待信号量变为可用的时间，单位为时钟节拍（tick）。可使用宏
 * portTICK_PERIOD_MS 将其转换为实际时间。阻塞时间设为 0 可用于轮询信号量。
 * 若任务已拥有该信号量，则无论 xBlockTime 取值如何，xSemaphoreTakeRecursive()
 * 都会立即返回。
 *
 * @return 若成功获取信号量则返回 pdTRUE。若 xBlockTime 超时后
 * 信号量仍不可用，则返回 pdFALSE。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xMutex = NULL;
 *
 * // 创建互斥量的任务。
 * void vATask( void * pvParameters )
 * {
 *  // 创建互斥量以保护共享资源。
 *  xMutex = xSemaphoreCreateRecursiveMutex();
 * }
 *
 * // 使用互斥量的任务。
 * void vAnotherTask( void * pvParameters )
 * {
 *  // ... 执行其他操作。
 *
 *  if( xMutex != NULL )
 *  {
 *      // 尝试获取互斥量。若互斥量不可用，
 *      // 则等待 10 个时钟节拍，看它是否会变为可用。
 *      if( xSemaphoreTakeRecursive( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
 *      {
 *          // 成功获取互斥量，现在可以访问
 *          // 共享资源。
 *
 *          // ...
 *          // 由于代码逻辑的需要，会对同一互斥量进一步调用
 *          // xSemaphoreTakeRecursive()。在实际代码中，
 *          // 这些调用不会只是顺序调用，因为那样毫无意义。
 *          // 相反，这些调用很可能嵌套在
 *          // 更复杂的调用结构中。
 *          xSemaphoreTakeRecursive( xMutex, ( TickType_t ) 10 );
 *          xSemaphoreTakeRecursive( xMutex, ( TickType_t ) 10 );
 *
 *          // 该互斥量至此已被“获取”三次，因此在同样被释放三次之前，
 *          // 其他任务无法使用它。同样，实际代码不太可能
 *          // 顺序进行这些调用，而是嵌入更复杂的
 *          // 调用结构中。此处仅作示意。
 *          xSemaphoreGiveRecursive( xMutex );
 *          xSemaphoreGiveRecursive( xMutex );
 *          xSemaphoreGiveRecursive( xMutex );
 *
 *          // 现在其他任务可以获取该互斥量。
 *      }
 *      else
 *      {
 *          // 未能获取互斥量，因此无法安全地
 *          // 访问共享资源。
 *      }
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreTakeRecursive xSemaphoreTakeRecursive
 * \ingroup Semaphores
 */
#if ( configUSE_RECURSIVE_MUTEXES == 1 )
    #define xSemaphoreTakeRecursive( xMutex, xBlockTime )    xQueueTakeMutexRecursive( ( xMutex ), ( xBlockTime ) )
#endif

/**
 * semphr. h
 * @code{c}
 * xSemaphoreGive( SemaphoreHandle_t xSemaphore );
 * @endcode
 *
 * <i>宏</i>，用于释放信号量。该信号量必须已通过调用 xSemaphoreCreateBinary()、
 * xSemaphoreCreateMutex() 或 xSemaphoreCreateCounting() 创建，并使用 sSemaphoreTake() 获取。
 *
 * 此宏不得在中断服务程序（ISR）中使用。若需在 ISR 中释放信号量，可使用替代接口 xSemaphoreGiveFromISR()。
 *
 * 此宏也不得用于通过 xSemaphoreCreateRecursiveMutex() 创建的信号量。
 *
 * @param xSemaphore 正在释放的信号量句柄。该句柄在创建信号量时返回。
 *
 * @return 若信号量释放成功则返回 pdTRUE。若发生错误则返回 pdFALSE。
 * 信号量是使用队列实现的。如果队列中没有空间来投递消息，就会发生错误——
 * 这表明此前没有正确获取信号量。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 创建信号量以保护共享资源。
 *  xSemaphore = vSemaphoreCreateBinary();
 *
 *  if( xSemaphore != NULL )
 *  {
 *      if( xSemaphoreGive( xSemaphore ) != pdTRUE )
 *      {
 *          // 预期此调用会失败，因为不能在未先“获取”信号量的情况下
 *          // “释放”它！
 *      }
 *
 *      // 获取信号量——若信号量不能立即变为可用，
 *      // 则不要阻塞。
 *      if( xSemaphoreTake( xSemaphore, ( TickType_t ) 0 ) )
 *      {
 *          // 现在已获得信号量，可以访问共享资源。
 *
 *          // ...
 *
 *          // 已结束对共享资源的访问，因此可以释放
 *          // 信号量。
 *          if( xSemaphoreGive( xSemaphore ) != pdTRUE )
 *          {
 *              // 预期此调用不会失败，因为能执行到这里就说明
 *              // 一定已经获取了信号量。
 *          }
 *      }
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreGive xSemaphoreGive
 * \ingroup Semaphores
 */
#define xSemaphoreGive( xSemaphore )    xQueueGenericSend( ( QueueHandle_t ) ( xSemaphore ), NULL, semGIVE_BLOCK_TIME, queueSEND_TO_BACK )

/**
 * semphr. h
 * @code{c}
 * xSemaphoreGiveRecursive( SemaphoreHandle_t xMutex );
 * @endcode
 *
 * <i>宏</i>，用于递归释放（give）互斥量类型的信号量。
 * 该互斥量必须已通过调用 xSemaphoreCreateRecursiveMutex() 创建。
 *
 * 要使此宏可用，必须在 FreeRTOSConfig.h 中将 configUSE_RECURSIVE_MUTEXES 设为 1。
 *
 * 此宏不得用于通过 xSemaphoreCreateMutex() 创建的互斥量。
 *
 * 递归使用的互斥量可被其所有者重复“获取（take）”。在所有者为每次成功的
 * “获取”请求调用 xSemaphoreGiveRecursive() 之前，该互斥量不会再次变为可用。
 * 例如，若某任务成功“获取”同一互斥量 5 次，则该任务也必须恰好
 * “释放”该互斥量 5 次之后，其他任务才能使用它。
 *
 * @param xMutex 正在释放（give）的互斥量句柄。该句柄由 xSemaphoreCreateMutex() 返回。
 *
 * @return 若信号量已释放，则返回 pdTRUE。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xMutex = NULL;
 *
 * // 创建互斥量的任务。
 * void vATask( void * pvParameters )
 * {
 *  // 创建互斥量以保护共享资源。
 *  xMutex = xSemaphoreCreateRecursiveMutex();
 * }
 *
 * // 使用互斥量的任务。
 * void vAnotherTask( void * pvParameters )
 * {
 *  // ... 执行其他操作。
 *
 *  if( xMutex != NULL )
 *  {
 *      // 尝试获取互斥量。若互斥量不可用，
 *      // 则等待 10 个时钟节拍，看它是否会变为可用。
 *      if( xSemaphoreTakeRecursive( xMutex, ( TickType_t ) 10 ) == pdTRUE )
 *      {
 *          // 成功获取互斥量，现在可以访问
 *          // 共享资源。
 *
 *          // ...
 *          // 由于代码逻辑的需要，会对同一互斥量进一步调用
 *          // xSemaphoreTakeRecursive()。在实际代码中，
 *          // 这些调用不会只是顺序调用，因为那样毫无意义。
 *          // 相反，这些调用很可能嵌套在
 *          // 更复杂的调用结构中。
 *          xSemaphoreTakeRecursive( xMutex, ( TickType_t ) 10 );
 *          xSemaphoreTakeRecursive( xMutex, ( TickType_t ) 10 );
 *
 *          // 该互斥量至此已被“获取”三次，因此在同样被释放三次之前，
 *          // 其他任务无法使用它。同样，实际代码不太可能
 *          // 顺序进行这些调用，更可能的情况是：这些
 *          // xSemaphoreGiveRecursive() 调用会随着调用栈
 *          // 展开而被调用。此处仅作演示。
 *          xSemaphoreGiveRecursive( xMutex );
 *          xSemaphoreGiveRecursive( xMutex );
 *          xSemaphoreGiveRecursive( xMutex );
 *
 *          // 现在其他任务可以获取该互斥量。
 *      }
 *      else
 *      {
 *          // 未能获取互斥量，因此无法安全地
 *          // 访问共享资源。
 *      }
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreGiveRecursive xSemaphoreGiveRecursive
 * \ingroup Semaphores
 */
#if ( configUSE_RECURSIVE_MUTEXES == 1 )
    #define xSemaphoreGiveRecursive( xMutex )    xQueueGiveMutexRecursive( ( xMutex ) )
#endif

/**
 * semphr. h
 * @code{c}
 * xSemaphoreGiveFromISR(
 *                        SemaphoreHandle_t xSemaphore,
 *                        BaseType_t *pxHigherPriorityTaskWoken
 *                    );
 * @endcode
 *
 * <i>宏</i>，用于释放信号量。该信号量必须已通过调用 xSemaphoreCreateBinary() 或
 * xSemaphoreCreateCounting() 创建。
 *
 * 互斥量类型的信号量（通过调用 xSemaphoreCreateMutex() 创建）
 * 不得与此宏一起使用。
 *
 * 此宏可在 ISR 中使用。
 *
 * @param xSemaphore 正在释放的信号量句柄。该句柄在创建信号量时返回。
 *
 * @param pxHigherPriorityTaskWoken 若释放信号量导致某个任务解除阻塞，
 * 且被解除阻塞的任务优先级高于当前正在运行的任务，则 xSemaphoreGiveFromISR()
 * 会将 *pxHigherPriorityTaskWoken 设为 pdTRUE。若 xSemaphoreGiveFromISR()
 * 将该值设为 pdTRUE，则应在退出中断前请求一次上下文切换。
 *
 * @return 若信号量释放成功则返回 pdTRUE，否则返回 errQUEUE_FULL。
 *
 * 示例用法：
 * @code{c}
 * \#define LONG_TIME 0xffff
 * \#define TICKS_TO_WAIT 10
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 * // 重复执行的任务。
 * void vATask( void * pvParameters )
 * {
 *  for( ;; )
 *  {
 *      // 希望该任务每 10 个时钟节拍运行一次。信号量
 *      // 已在该任务启动之前创建。
 *
 *      // 阻塞等待信号量变为可用。
 *      if( xSemaphoreTake( xSemaphore, LONG_TIME ) == pdTRUE )
 *      {
 *          // 到了该执行的时间。
 *
 *          // ...
 *
 *          // 已完成任务。返回到循环顶部，
 *          // 在此阻塞等待信号量，直到再次需要执行。
 *          // 注意：以这种方式使用信号量与 ISR 同步时，
 *          // 无需“释放”信号量。
 *      }
 *  }
 * }
 *
 * // 定时器 ISR
 * void vTimerISR( void * pvParameters )
 * {
 * static uint8_t ucLocalTickCount = 0;
 * static BaseType_t xHigherPriorityTaskWoken;
 *
 *  // 发生了定时器时钟节拍。
 *
 *  // ... 执行其他与时间相关的功能。
 *
 *  // 是否到了 vATask() 运行的时间？
 *  xHigherPriorityTaskWoken = pdFALSE;
 *  ucLocalTickCount++;
 *  if( ucLocalTickCount >= TICKS_TO_WAIT )
 *  {
 *      // 通过释放信号量解除任务的阻塞。
 *      xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
 *
 *      // 复位计数，以便在 10 个时钟节拍后再次释放信号量。
 *      ucLocalTickCount = 0;
 *  }
 *
 *  if( xHigherPriorityTaskWoken != pdFALSE )
 *  {
 *      // 可在此处强制进行上下文切换。从
 *      // ISR 进行上下文切换使用移植层特定的语法。请查看所用移植层的
 *      // 演示任务，以了解所需语法。
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreGiveFromISR xSemaphoreGiveFromISR
 * \ingroup Semaphores
 */
#define xSemaphoreGiveFromISR( xSemaphore, pxHigherPriorityTaskWoken )    xQueueGiveFromISR( ( QueueHandle_t ) ( xSemaphore ), ( pxHigherPriorityTaskWoken ) )

/**
 * semphr. h
 * @code{c}
 * xSemaphoreTakeFromISR(
 *                        SemaphoreHandle_t xSemaphore,
 *                        BaseType_t *pxHigherPriorityTaskWoken
 *                    );
 * @endcode
 *
 * <i>宏</i>，用于从 ISR 获取信号量。该信号量必须已
 * 通过调用 xSemaphoreCreateBinary() 或 xSemaphoreCreateCounting() 创建。
 *
 * 互斥量类型的信号量（通过调用 xSemaphoreCreateMutex() 创建）
 * 不得与此宏一起使用。
 *
 * 此宏可在 ISR 中使用，不过从 ISR 获取信号量并不常见。
 * 它可能仅在以下情形有用：当中断从资源池获取对象时获取计数信号量
 * （此时信号量计数值表示可用资源的数量）。
 *
 * @param xSemaphore 正在获取的信号量句柄。该句柄在创建信号量时返回。
 *
 * @param pxHigherPriorityTaskWoken 若获取信号量导致某个任务解除阻塞，
 * 且被解除阻塞的任务优先级高于当前正在运行的任务，则 xSemaphoreTakeFromISR()
 * 会将 *pxHigherPriorityTaskWoken 设为 pdTRUE。若 xSemaphoreTakeFromISR()
 * 将该值设为 pdTRUE，则应在退出中断前请求一次上下文切换。
 *
 * @return 若信号量获取成功则返回 pdTRUE，否则返回
 * pdFALSE
 */
#define xSemaphoreTakeFromISR( xSemaphore, pxHigherPriorityTaskWoken )    xQueueReceiveFromISR( ( QueueHandle_t ) ( xSemaphore ), NULL, ( pxHigherPriorityTaskWoken ) )

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateMutex( void );
 * @endcode
 *
 * 创建一个新的互斥量类型的信号量实例，并返回可用于引用该新互斥量的句柄。
 *
 * 在 FreeRTOS 内部实现中，互斥量信号量使用一块内存来存储互斥量结构。
 * 若使用 xSemaphoreCreateMutex() 创建互斥量，则所需内存会在
 * xSemaphoreCreateMutex() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xSemaphoreCreateMutexStatic()
 * 创建互斥量，则应用程序编写者必须提供内存。因此，xSemaphoreCreateMutexStatic()
 * 允许在不使用任何动态内存分配的情况下创建互斥量。
 *
 * 使用此函数创建的互斥量可通过 xSemaphoreTake() 和 xSemaphoreGive() 宏访问。
 * 不得使用 xSemaphoreTakeRecursive() 和 xSemaphoreGiveRecursive() 宏。
 *
 * 这种信号量使用优先级继承机制，因此任务“获取（take）”信号量之后，
 * 一旦不再需要该信号量，就必须始终“释放（give）”它。
 *
 * 互斥量类型的信号量不能在中断服务程序（ISR）内部使用。
 *
 * 关于可用于纯同步（一个任务或中断始终“释放”信号量，另一个任务始终“获取”
 * 信号量）且可在中断服务程序内部使用的替代实现，参见 xSemaphoreCreateBinary()。
 *
 * @return 若互斥量创建成功，则返回所创建信号量的句柄。若堆内存不足以分配
 * 互斥量的数据结构，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 在调用 xSemaphoreCreateMutex() 之前不能使用信号量。
 *  // 该接口是宏，因此直接传入变量。
 *  xSemaphore = xSemaphoreCreateMutex();
 *
 *  if( xSemaphore != NULL )
 *  {
 *      // 信号量创建成功。
 *      // 现在可以使用该信号量。
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreCreateMutex xSemaphoreCreateMutex
 * \ingroup Semaphores
 */
#if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_MUTEXES == 1 ) )
    #define xSemaphoreCreateMutex()    xQueueCreateMutex( queueQUEUE_TYPE_MUTEX )
#endif

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateMutexStatic( StaticSemaphore_t *pxMutexBuffer );
 * @endcode
 *
 * 创建一个新的互斥量类型的信号量实例，并返回可用于引用该新互斥量的句柄。
 *
 * 在 FreeRTOS 内部实现中，互斥量信号量使用一块内存来存储互斥量结构。
 * 若使用 xSemaphoreCreateMutex() 创建互斥量，则所需内存会在
 * xSemaphoreCreateMutex() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xSemaphoreCreateMutexStatic()
 * 创建互斥量，则应用程序编写者必须提供内存。因此，xSemaphoreCreateMutexStatic()
 * 允许在不使用任何动态内存分配的情况下创建互斥量。
 *
 * 使用此函数创建的互斥量可通过 xSemaphoreTake() 和 xSemaphoreGive() 宏访问。
 * 不得使用 xSemaphoreTakeRecursive() 和 xSemaphoreGiveRecursive() 宏。
 *
 * 这种信号量使用优先级继承机制，因此任务“获取（take）”信号量之后，
 * 一旦不再需要该信号量，就必须始终“释放（give）”它。
 *
 * 互斥量类型的信号量不能在中断服务程序（ISR）内部使用。
 *
 * 关于可用于纯同步（一个任务或中断始终“释放”信号量，另一个任务始终“获取”
 * 信号量）且可在中断服务程序内部使用的替代实现，参见 xSemaphoreCreateBinary()。
 *
 * @param pxMutexBuffer 必须指向 StaticSemaphore_t 类型的变量，
 * 该变量将用于保存互斥量的数据结构，从而无需动态分配内存。
 *
 * @return 若互斥量创建成功，则返回所创建互斥量的句柄。若 pxMutexBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore;
 * StaticSemaphore_t xMutexBuffer;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 互斥量在创建之后才能使用。xMutexBuffer 会被
 *  // 传入 xSemaphoreCreateMutexStatic()，因此不会尝试任何动态内存分配。
 *  xSemaphore = xSemaphoreCreateMutexStatic( &xMutexBuffer );
 *
 *  // 由于未执行动态内存分配，xSemaphore 不可能为 NULL，
 *  // 因此无需检查。
 * }
 * @endcode
 * \defgroup xSemaphoreCreateMutexStatic xSemaphoreCreateMutexStatic
 * \ingroup Semaphores
 */
#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_MUTEXES == 1 ) )
    #define xSemaphoreCreateMutexStatic( pxMutexBuffer )    xQueueCreateMutexStatic( queueQUEUE_TYPE_MUTEX, ( pxMutexBuffer ) )
#endif


/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateRecursiveMutex( void );
 * @endcode
 *
 * 创建一个新的递归互斥量类型的信号量实例，并返回可用于引用该新递归互斥量的句柄。
 *
 * 在 FreeRTOS 内部实现中，递归互斥量使用一块内存来存储互斥量结构。
 * 若使用 xSemaphoreCreateRecursiveMutex() 创建递归互斥量，则所需内存会在
 * xSemaphoreCreateRecursiveMutex() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用
 * xSemaphoreCreateRecursiveMutexStatic() 创建递归互斥量，则应用程序编写者必须
 * 提供该互斥量将要使用的内存。因此，xSemaphoreCreateRecursiveMutexStatic()
 * 允许在不使用任何动态内存分配的情况下创建递归互斥量。
 *
 * 使用此宏创建的互斥量可通过 xSemaphoreTakeRecursive() 和
 * xSemaphoreGiveRecursive() 宏访问。不得使用 xSemaphoreTake() 和
 * xSemaphoreGive() 宏。
 *
 * 递归使用的互斥量可被其所有者重复“获取（take）”。在所有者为每次成功的
 * “获取”请求调用 xSemaphoreGiveRecursive() 之前，该互斥量不会再次变为可用。
 * 例如，若某任务成功“获取”同一互斥量 5 次，则该任务也必须恰好
 * “释放”该互斥量 5 次之后，其他任务才能使用它。
 *
 * 这种信号量使用优先级继承机制，因此任务“获取（take）”信号量之后，
 * 一旦不再需要该信号量，就必须始终“释放（give）”它。
 *
 * 互斥量类型的信号量不能在中断服务程序（ISR）内部使用。
 *
 * 关于可用于纯同步（一个任务或中断始终“释放”信号量，另一个任务始终“获取”
 * 信号量）且可在中断服务程序内部使用的替代实现，参见 xSemaphoreCreateBinary()。
 *
 * @return xSemaphore 所创建互斥量信号量的句柄。类型应为
 * SemaphoreHandle_t。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 在调用 xSemaphoreCreateMutex() 之前不能使用信号量。
 *  // 该接口是宏，因此直接传入变量。
 *  xSemaphore = xSemaphoreCreateRecursiveMutex();
 *
 *  if( xSemaphore != NULL )
 *  {
 *      // 信号量创建成功。
 *      // 现在可以使用该信号量。
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreCreateRecursiveMutex xSemaphoreCreateRecursiveMutex
 * \ingroup Semaphores
 */
#if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_RECURSIVE_MUTEXES == 1 ) )
    #define xSemaphoreCreateRecursiveMutex()    xQueueCreateMutex( queueQUEUE_TYPE_RECURSIVE_MUTEX )
#endif

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateRecursiveMutexStatic( StaticSemaphore_t *pxMutexBuffer );
 * @endcode
 *
 * 创建一个新的递归互斥量类型的信号量实例，并返回可用于引用该新递归互斥量的句柄。
 *
 * 在 FreeRTOS 内部实现中，递归互斥量使用一块内存来存储互斥量结构。
 * 若使用 xSemaphoreCreateRecursiveMutex() 创建递归互斥量，则所需内存会在
 * xSemaphoreCreateRecursiveMutex() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用
 * xSemaphoreCreateRecursiveMutexStatic() 创建递归互斥量，则应用程序编写者必须
 * 提供该互斥量将要使用的内存。因此，xSemaphoreCreateRecursiveMutexStatic()
 * 允许在不使用任何动态内存分配的情况下创建递归互斥量。
 *
 * 使用此宏创建的互斥量可通过 xSemaphoreTakeRecursive() 和
 * xSemaphoreGiveRecursive() 宏访问。不得使用 xSemaphoreTake() 和
 * xSemaphoreGive() 宏。
 *
 * 递归使用的互斥量可被其所有者重复“获取（take）”。在所有者为每次成功的
 * “获取”请求调用 xSemaphoreGiveRecursive() 之前，该互斥量不会再次变为可用。
 * 例如，若某任务成功“获取”同一互斥量 5 次，则该任务也必须恰好
 * “释放”该互斥量 5 次之后，其他任务才能使用它。
 *
 * 这种信号量使用优先级继承机制，因此任务“获取（take）”信号量之后，
 * 一旦不再需要该信号量，就必须始终“释放（give）”它。
 *
 * 互斥量类型的信号量不能在中断服务程序（ISR）内部使用。
 *
 * 关于可用于纯同步（一个任务或中断始终“释放”信号量，另一个任务始终“获取”
 * 信号量）且可在中断服务程序内部使用的替代实现，参见 xSemaphoreCreateBinary()。
 *
 * @param pxMutexBuffer 必须指向 StaticSemaphore_t 类型的变量，
 * 该变量随后用于保存递归互斥量的数据结构，从而无需动态分配内存。
 *
 * @return 若递归互斥量创建成功，则返回所创建递归互斥量的句柄。
 * 若 pxMutexBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore;
 * StaticSemaphore_t xMutexBuffer;
 *
 * void vATask( void * pvParameters )
 * {
 *  // 递归信号量在创建之后才能使用。此处使用 xSemaphoreCreateRecursiveMutexStatic()
 *  // 创建递归互斥量。
 *  // xMutexBuffer 的地址会传入函数，并将保存
 *  // 互斥量的数据结构——因此不会尝试任何动态内存分配。
 *  xSemaphore = xSemaphoreCreateRecursiveMutexStatic( &xMutexBuffer );
 *
 *  // 由于未执行动态内存分配，xSemaphore 不可能为 NULL，
 *  // 因此无需检查。
 * }
 * @endcode
 * \defgroup xSemaphoreCreateRecursiveMutexStatic xSemaphoreCreateRecursiveMutexStatic
 * \ingroup Semaphores
 */
#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_RECURSIVE_MUTEXES == 1 ) )
    #define xSemaphoreCreateRecursiveMutexStatic( pxStaticSemaphore )    xQueueCreateMutexStatic( queueQUEUE_TYPE_RECURSIVE_MUTEX, ( pxStaticSemaphore ) )
#endif /* configSUPPORT_STATIC_ALLOCATION 条件编译结束 */

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateCounting( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount );
 * @endcode
 *
 * 创建一个新的计数信号量实例，并返回可用于引用该新计数信号量的句柄。
 *
 * 在许多使用场景中，使用直接任务通知代替计数信号量速度更快、内存效率更高！
 * https://www.FreeRTOS.org/RTOS-task-notifications.html
 *
 * 在 FreeRTOS 内部实现中，计数信号量使用一块内存来存储计数信号量结构。
 * 若使用 xSemaphoreCreateCounting() 创建计数信号量，则所需内存会在
 * xSemaphoreCreateCounting() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xSemaphoreCreateCountingStatic()
 * 创建计数信号量，则应用程序编写者可以选择提供该计数信号量将要使用的内存。
 * 因此，xSemaphoreCreateCountingStatic() 允许在不使用任何动态内存分配的情况下
 * 创建计数信号量。
 *
 * 计数信号量通常用于以下两种用途：
 *
 * 1) 计数事件。
 *
 *    在这种使用场景中，每当事件发生时，事件处理程序会“释放（give）”一次信号量
 *    （递增信号量计数值）；处理任务每处理一个事件就“获取（take）”一次信号量
 *    （递减信号量计数值）。因此，计数值等于已发生事件数与已处理事件数之差。
 *    在这种情况下，初始计数值宜设为 0。
 *
 * 2) 资源管理。
 *
 *    在这种使用场景中，计数值表示可用资源的数量。任务若要获得对某个资源的控制权，
 *    必须先获取信号量——递减信号量计数值。当计数值降到 0 时，表示没有空闲资源。
 *    任务用完资源后“释放”信号量——递增信号量计数值。在这种情况下，
 *    初始计数值宜等于最大计数值，表示所有资源均空闲。
 *
 * @param uxMaxCount 可达到的最大计数值。当信号量达到该值时，
 *        就不能再被“释放（give）”。
 *
 * @param uxInitialCount 创建信号量时赋予它的计数值。
 *
 * @return 所创建信号量的句柄。若无法创建信号量，
 *         则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore;
 *
 * void vATask( void * pvParameters )
 * {
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 *  // 在调用 xSemaphoreCreateCounting() 之前不能使用信号量。
 *  // 信号量可计数的最大值应为 10，
 *  // 赋予计数值的初始值应为 0。
 *  xSemaphore = xSemaphoreCreateCounting( 10, 0 );
 *
 *  if( xSemaphore != NULL )
 *  {
 *      // 信号量创建成功。
 *      // 现在可以使用该信号量。
 *  }
 * }
 * @endcode
 * \defgroup xSemaphoreCreateCounting xSemaphoreCreateCounting
 * \ingroup Semaphores
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    #define xSemaphoreCreateCounting( uxMaxCount, uxInitialCount )    xQueueCreateCountingSemaphore( ( uxMaxCount ), ( uxInitialCount ) )
#endif

/**
 * semphr. h
 * @code{c}
 * SemaphoreHandle_t xSemaphoreCreateCountingStatic( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount, StaticSemaphore_t *pxSemaphoreBuffer );
 * @endcode
 *
 * 创建一个新的计数信号量实例，并返回可用于引用该新计数信号量的句柄。
 *
 * 在许多使用场景中，使用直接任务通知代替计数信号量速度更快、内存效率更高！
 * https://www.FreeRTOS.org/RTOS-task-notifications.html
 *
 * 在 FreeRTOS 内部实现中，计数信号量使用一块内存来存储计数信号量结构。
 * 若使用 xSemaphoreCreateCounting() 创建计数信号量，则所需内存会在
 * xSemaphoreCreateCounting() 函数内部自动动态分配（参见
 * https://www.FreeRTOS.org/a00111.html）。若使用 xSemaphoreCreateCountingStatic()
 * 创建计数信号量，则应用程序编写者必须提供内存。
 * 因此，xSemaphoreCreateCountingStatic() 允许在不使用任何动态内存分配的情况下
 * 创建计数信号量。
 *
 * 计数信号量通常用于以下两种用途：
 *
 * 1) 计数事件。
 *
 *    在这种使用场景中，每当事件发生时，事件处理程序会“释放（give）”一次信号量
 *    （递增信号量计数值）；处理任务每处理一个事件就“获取（take）”一次信号量
 *    （递减信号量计数值）。因此，计数值等于已发生事件数与已处理事件数之差。
 *    在这种情况下，初始计数值宜设为 0。
 *
 * 2) 资源管理。
 *
 *    在这种使用场景中，计数值表示可用资源的数量。任务若要获得对某个资源的控制权，
 *    必须先获取信号量——递减信号量计数值。当计数值降到 0 时，表示没有空闲资源。
 *    任务用完资源后“释放”信号量——递增信号量计数值。在这种情况下，
 *    初始计数值宜等于最大计数值，表示所有资源均空闲。
 *
 * @param uxMaxCount 可达到的最大计数值。当信号量达到该值时，
 *        就不能再被“释放（give）”。
 *
 * @param uxInitialCount 创建信号量时赋予它的计数值。
 *
 * @param pxSemaphoreBuffer 必须指向 StaticSemaphore_t 类型的变量，
 * 该变量随后用于保存信号量的数据结构，从而无需动态分配内存。
 *
 * @return 若计数信号量创建成功，则返回所创建计数信号量的句柄。
 * 若 pxSemaphoreBuffer 为 NULL，则返回 NULL。
 *
 * 示例用法：
 * @code{c}
 * SemaphoreHandle_t xSemaphore;
 * StaticSemaphore_t xSemaphoreBuffer;
 *
 * void vATask( void * pvParameters )
 * {
 * SemaphoreHandle_t xSemaphore = NULL;
 *
 *  // 计数信号量在创建之后才能使用。使用
 *  // xSemaphoreCreateCountingStatic() 创建计数信号量。信号量可计数的最大
 *  // 值为 10，赋予计数值的初始值将为 0。xSemaphoreBuffer 的地址
 *  // 会传入，并将用于保存信号量结构，因此不会使用动态内存分配。
 *  xSemaphore = xSemaphoreCreateCounting( 10, 0, &xSemaphoreBuffer );
 *
 *  // 未尝试内存分配，因此 xSemaphore 不可能为 NULL，
 *  // 无需检查其值。
 * }
 * @endcode
 * \defgroup xSemaphoreCreateCountingStatic xSemaphoreCreateCountingStatic
 * \ingroup Semaphores
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #define xSemaphoreCreateCountingStatic( uxMaxCount, uxInitialCount, pxSemaphoreBuffer )    xQueueCreateCountingSemaphoreStatic( ( uxMaxCount ), ( uxInitialCount ), ( pxSemaphoreBuffer ) )
#endif /* configSUPPORT_STATIC_ALLOCATION 条件编译结束 */

/**
 * semphr. h
 * @code{c}
 * void vSemaphoreDelete( SemaphoreHandle_t xSemaphore );
 * @endcode
 *
 * 删除信号量。使用此函数必须格外小心。例如，
 * 若互斥量类型的信号量正被某个任务持有，则不要删除它。
 *
 * @param xSemaphore 要删除的信号量的句柄。
 *
 * \defgroup vSemaphoreDelete vSemaphoreDelete
 * \ingroup Semaphores
 */
#define vSemaphoreDelete( xSemaphore )    vQueueDelete( ( QueueHandle_t ) ( xSemaphore ) )

/**
 * semphr.h
 * @code{c}
 * TaskHandle_t xSemaphoreGetMutexHolder( SemaphoreHandle_t xMutex );
 * @endcode
 *
 * 如果 xMutex 确实是互斥量类型的信号量，则返回当前持有该互斥量的任务。
 * 如果 xMutex 不是互斥量类型的信号量，或者该互斥量可用（未被任何任务持有），
 * 则返回 NULL。
 *
 * 注意：这是判断调用任务是否为互斥量持有者的好方法，但不适合用于
 * 确定互斥量持有者的身份，因为在函数返回到测试返回值之间，
 * 持有者可能已经改变。
 */
#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )
    #define xSemaphoreGetMutexHolder( xSemaphore )    xQueueGetMutexHolder( ( xSemaphore ) )
#endif

/**
 * semphr.h
 * @code{c}
 * TaskHandle_t xSemaphoreGetMutexHolderFromISR( SemaphoreHandle_t xMutex );
 * @endcode
 *
 * 如果 xMutex 确实是互斥量类型的信号量，则返回当前持有该互斥量的任务。
 * 如果 xMutex 不是互斥量类型的信号量，或者该互斥量可用（未被任何任务持有），
 * 则返回 NULL。
 */
#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )
    #define xSemaphoreGetMutexHolderFromISR( xSemaphore )    xQueueGetMutexHolderFromISR( ( xSemaphore ) )
#endif

/**
 * semphr.h
 * @code{c}
 * UBaseType_t uxSemaphoreGetCount( SemaphoreHandle_t xSemaphore );
 * @endcode
 *
 * 若信号量是计数信号量，则 uxSemaphoreGetCount() 返回其当前计数值。
 * 若信号量是二值信号量，则 uxSemaphoreGetCount() 在信号量可用时返回 1，
 * 在信号量不可用时返回 0。
 */
#define uxSemaphoreGetCount( xSemaphore )           uxQueueMessagesWaiting( ( QueueHandle_t ) ( xSemaphore ) )

/**
 * semphr.h
 * @code{c}
 * UBaseType_t uxSemaphoreGetCountFromISR( SemaphoreHandle_t xSemaphore );
 * @endcode
 *
 * 若信号量是计数信号量，则 uxSemaphoreGetCountFromISR() 返回其当前计数值。
 * 若信号量是二值信号量，则 uxSemaphoreGetCountFromISR() 在信号量可用时返回 1，
 * 在信号量不可用时返回 0。
 */
#define uxSemaphoreGetCountFromISR( xSemaphore )    uxQueueMessagesWaitingFromISR( ( QueueHandle_t ) ( xSemaphore ) )

/**
 * semphr.h
 * @code{c}
 * BaseType_t xSemaphoreGetStaticBuffer( SemaphoreHandle_t xSemaphore,
 *                                       StaticSemaphore_t ** ppxSemaphoreBuffer );
 * @endcode
 *
 * 获取指向静态创建的二值信号量、计数信号量或互斥量信号量的数据结构缓冲区的指针。
 * 该缓冲区与创建时提供的缓冲区相同。
 *
 * @param xSemaphore 要获取其缓冲区的信号量。
 *
 * @param ppxSemaphoreBuffer 用于返回指向信号量数据结构缓冲区的指针。
 *
 * @return 若成功获取缓冲区则返回 pdTRUE，否则返回 pdFALSE。
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #define xSemaphoreGetStaticBuffer( xSemaphore, ppxSemaphoreBuffer )    xQueueGenericGetStaticBuffers( ( QueueHandle_t ) ( xSemaphore ), NULL, ( ppxSemaphoreBuffer ) )
#endif /* configSUPPORT_STATIC_ALLOCATION 条件编译结束 */

#endif /* SEMAPHORE_H 头文件包含保护 */
