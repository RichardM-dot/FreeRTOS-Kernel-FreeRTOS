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

#ifndef EVENT_GROUPS_H
#define EVENT_GROUPS_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h" must appear in source files before "include event_groups.h"
#endif

/* FreeRTOS 头文件包含。 */
#include "timers.h"

/*
 * 以下位域在任务的事件链表项值中传递控制信息。它们不得与
 * taskEVENT_LIST_ITEM_VALUE_IN_USE 定义发生冲突，这一点很重要。
 */
#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    #define eventCLEAR_EVENTS_ON_EXIT_BIT    ( ( uint16_t ) 0x0100U )
    #define eventUNBLOCKED_DUE_TO_BIT_SET    ( ( uint16_t ) 0x0200U )
    #define eventWAIT_FOR_ALL_BITS           ( ( uint16_t ) 0x0400U )
    #define eventEVENT_BITS_CONTROL_BYTES    ( ( uint16_t ) 0xff00U )
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    #define eventCLEAR_EVENTS_ON_EXIT_BIT    ( ( uint32_t ) 0x01000000U )
    #define eventUNBLOCKED_DUE_TO_BIT_SET    ( ( uint32_t ) 0x02000000U )
    #define eventWAIT_FOR_ALL_BITS           ( ( uint32_t ) 0x04000000U )
    #define eventEVENT_BITS_CONTROL_BYTES    ( ( uint32_t ) 0xff000000U )
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
    #define eventCLEAR_EVENTS_ON_EXIT_BIT    ( ( uint64_t ) 0x0100000000000000U )
    #define eventUNBLOCKED_DUE_TO_BIT_SET    ( ( uint64_t ) 0x0200000000000000U )
    #define eventWAIT_FOR_ALL_BITS           ( ( uint64_t ) 0x0400000000000000U )
    #define eventEVENT_BITS_CONTROL_BYTES    ( ( uint64_t ) 0xff00000000000000U )
#endif /* 当 configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS 时 */

/* *INDENT-OFF* —— 暂停自动缩进（uncrustify 指令） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* —— 恢复自动缩进（uncrustify 指令） */

/**
 * 事件组是一组位，应用可为每一位赋予含义。例如，应用可创建一个事件组来表示
 * 各种 CAN 总线相关事件的状态：位 0 可表示“已收到一条 CAN 报文并准备好处理”，
 * 位 1 可表示“应用已排队一条准备发送到 CAN 网络的报文”，位 2 可表示“该向 CAN
 * 网络发送 SYNC 报文了”，等等。任务随后可检测这些位的值，以确定哪些事件处于
 * 活动状态，并可选择进入阻塞态，等待指定的一个位或一组位变为活动状态。继续以
 * CAN 总线为例，一个 CAN 控制任务可以进入阻塞态（因而不消耗任何处理时间），
 * 直到位 0、位 1 或位 2 中的任意一个变为活动状态；此时，实际活动的位会告知任务
 * 它需要执行哪个操作（处理已收到的报文、发送报文或发送 SYNC）。
 *
 * 事件组实现包含相应机制，可避免应用使用简单变量实现相同功能时可能出现的竞态
 * 条件。对于何时清除事件组中的某个位、以及何时必须以原子方式置位并检测这些位，
 * 这一点尤为重要——事件组用于在多个任务之间建立同步点（“会合点”）时正是如此。
 */



/**
 * event_groups.h
 *
 * 事件组的引用类型。例如，调用 xEventGroupCreate() 会返回一个 EventGroupHandle_t
 * 变量，随后可作为参数传给其他事件组函数。
 *
 * \defgroup EventGroupHandle_t EventGroupHandle_t
 * \ingroup EventGroup
 */
struct EventGroupDef_t;
typedef struct EventGroupDef_t   * EventGroupHandle_t;

/*
 * 用于保存事件位的类型始终与 TickType_t 一致——因此其可容纳的位数由
 * configTICK_TYPE_WIDTH_IN_BITS 设置（设为 0 时为 16 位，设为 1 时为 32 位，
 * 设为 2 时为 64 位）。
 *
 * \defgroup EventBits_t EventBits_t
 * \ingroup EventGroup
 */
typedef TickType_t               EventBits_t;

/**
 * event_groups.h
 * @code{c}
 * EventGroupHandle_t xEventGroupCreate( void );
 * @endcode
 *
 * 创建一个新的事件组。
 *
 * 在 FreeRTOS 内部实现中，事件组使用一块[较小的]内存来存放事件组结构。若使用
 * xEventGroupCreate() 创建事件组，所需内存会在 xEventGroupCreate() 函数内部自动
 * 动态分配（参见 https://www.FreeRTOS.org/a00111.html）。若使用
 * xEventGroupCreateStatic() 创建事件组，则必须由应用编写者提供该事件组将要使用的
 * 内存。因此，xEventGroupCreateStatic() 允许在不使用任何动态内存分配的情况下
 * 创建事件组。
 *
 * 尽管事件组与 tick 无直接关系，但出于内部实现原因，事件组中可用的位数取决于
 * FreeRTOSConfig.h 中的 configTICK_TYPE_WIDTH_IN_BITS 设置。若
 * configTICK_TYPE_WIDTH_IN_BITS 为 0，则每个事件组包含 8 个可用位（位 0 至
 * 位 7）。若 configTICK_TYPE_WIDTH_IN_BITS 设为 1，则每个事件组有 24 个可用位
 * （位 0 至位 23）。若 configTICK_TYPE_WIDTH_IN_BITS 设为 2，则每个事件组有 56 个
 * 可用位（位 0 至位 53）。EventBits_t 类型用于在事件组中存储事件位。
 *
 * 要使 xEventGroupCreate() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @return 若事件组创建成功，则返回该事件组的句柄。若可用 FreeRTOS 堆不足，无法
 * 创建事件组，则返回 NULL。参见 https://www.FreeRTOS.org/a00111.html
 *
 * 用法示例：
 * @code{c}
 *  // 声明一个变量，用于保存所创建的事件组。
 *  EventGroupHandle_t xCreatedEventGroup;
 *
 *  // 尝试创建事件组。
 *  xCreatedEventGroup = xEventGroupCreate();
 *
 *  // 事件组是否创建成功？
 *  if( xCreatedEventGroup == NULL )
 *  {
 *      // 事件组未创建，因为可用 FreeRTOS 堆不足。
 *  }
 *  else
 *  {
 *      // 事件组已创建。
 *  }
 * @endcode
 * \defgroup xEventGroupCreate xEventGroupCreate
 * \ingroup EventGroup
 */
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    EventGroupHandle_t xEventGroupCreate( void ) PRIVILEGED_FUNCTION;
#endif

/**
 * event_groups.h
 * @code{c}
 * EventGroupHandle_t xEventGroupCreateStatic( EventGroupHandle_t * pxEventGroupBuffer );
 * @endcode
 *
 * 创建一个新的事件组。
 *
 * 在 FreeRTOS 内部实现中，事件组使用一块[较小的]内存来存放事件组结构。若使用
 * xEventGroupCreate() 创建事件组，所需内存会在 xEventGroupCreate() 函数内部自动
 * 动态分配（参见 https://www.FreeRTOS.org/a00111.html）。若使用
 * xEventGroupCreateStatic() 创建事件组，则必须由应用编写者提供该事件组将要使用的
 * 内存。因此，xEventGroupCreateStatic() 允许在不使用任何动态内存分配的情况下
 * 创建事件组。
 *
 * 尽管事件组与 tick 无直接关系，但出于内部实现原因，事件组中可用的位数取决于
 * FreeRTOSConfig.h 中的 configTICK_TYPE_WIDTH_IN_BITS 设置。若
 * configTICK_TYPE_WIDTH_IN_BITS 为 0，则每个事件组包含 8 个可用位（位 0 至
 * 位 7）。若 configTICK_TYPE_WIDTH_IN_BITS 设为 1，则每个事件组有 24 个可用位
 * （位 0 至位 23）。若 configTICK_TYPE_WIDTH_IN_BITS 设为 2，则每个事件组有 56 个
 * 可用位（位 0 至位 53）。EventBits_t 类型用于在事件组中存储事件位。
 *
 * 要使 xEventGroupCreateStatic() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置
 * 为 1。
 *
 * @param pxEventGroupBuffer pxEventGroupBuffer 必须指向一个 StaticEventGroup_t
 * 类型的变量，该变量随后用于存放事件组的数据结构，从而无需动态分配内存。
 *
 * @return 若事件组创建成功，则返回该事件组的句柄。若 pxEventGroupBuffer 为 NULL，
 * 则返回 NULL。
 *
 * 用法示例：
 * @code{c}
 *  // StaticEventGroup_t 是一个公开可访问的结构，其大小和对齐要求与真实事件组
 *  // 结构相同。它提供一种机制，使应用能够了解事件组的大小（取决于体系结构和
 *  // 配置文件设置），同时不会暴露真实事件组的内部实现，从而不破坏严格的数据
 *  // 隐藏策略。该 StaticEventGroup_t 变量会传入
 *  // xSemaphoreCreateEventGroupStatic() 函数，并用于存放事件组的数据结构。
 *  StaticEventGroup_t xEventGroupBuffer;
 *
 *  // 在不动态分配任何内存的情况下创建事件组。
 *  xEventGroup = xEventGroupCreateStatic( &xEventGroupBuffer );
 * @endcode
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    EventGroupHandle_t xEventGroupCreateStatic( StaticEventGroup_t * pxEventGroupBuffer ) PRIVILEGED_FUNCTION;
#endif

/**
 * event_groups.h
 * @code{c}
 *  EventBits_t xEventGroupWaitBits(    EventGroupHandle_t xEventGroup,
 *                                      const EventBits_t uxBitsToWaitFor,
 *                                      const BaseType_t xClearOnExit,
 *                                      const BaseType_t xWaitForAllBits,
 *                                      const TickType_t xTicksToWait );
 * @endcode
 *
 * [可能] 阻塞等待先前创建的事件组中一个或多个位被置位。
 *
 * 此函数不能从中断中调用。
 *
 * 要使 xEventGroupWaitBits() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @param xEventGroup 正在检测其中事件位的事件组。该事件组必须已通过调用
 * xEventGroupCreate() 创建。
 *
 * @param uxBitsToWaitFor 一个按位值，表示要在事件组中检测的位。例如，要等待
 * 位 0 和/或位 2，应将 uxBitsToWaitFor 设为 0x05；要等待位 0 和/或位 1 和/或
 * 位 2，应将 uxBitsToWaitFor 设为 0x07；依此类推。
 *
 * @param xClearOnExit 若 xClearOnExit 设为 pdTRUE，则在等待条件满足时（即函数
 * 因超时以外的原因返回），uxBitsToWaitFor 中已在事件组中置位的位将在
 * xEventGroupWaitBits() 返回之前被清除。若 xClearOnExit 设为 pdFALSE，则调用
 * xEventGroupWaitBits() 返回时，事件组中已置位的位不会被修改。
 *
 * @param xWaitForAllBits 若 xWaitForAllBits 设为 pdTRUE，则当 uxBitsToWaitFor
 * 中的所有位均已置位，或指定的阻塞时间到期时，xEventGroupWaitBits() 将返回。若
 * xWaitForAllBits 设为 pdFALSE，则当 uxBitsToWaitFor 中任意一个位被置位，或指定
 * 的阻塞时间到期时，xEventGroupWaitBits() 将返回。阻塞时间由 xTicksToWait 参数
 * 指定。
 *
 * @param xTicksToWait 等待 uxBitsToWaitFor 指定的一个/全部位（取决于
 * xWaitForAllBits 的值）变为置位的最大时间（以“tick”为单位）。可使用
 * portMAX_DELAY 值实现无限期阻塞（前提是 FreeRTOSConfig.h 中
 * INCLUDE_vTaskSuspend 设置为 1）。
 *
 * @return 在等待的位变为置位时，或阻塞时间到期时，事件组的值。通过检测返回值
 * 可确定哪些位已置位。若 xEventGroupWaitBits() 因超时到期而返回，则等待的位
 * 并非全部置位。若 xEventGroupWaitBits() 因等待的位已置位而返回，则在
 * xClearOnExit 参数设为 pdTRUE 的情况下，返回值为自动清除任何位之前的事件组值。
 *
 * 用法示例：
 * @code{c}
 * #define BIT_0 ( 1 << 0 )
 * #define BIT_4 ( 1 << 4 )
 *
 * void aFunction( EventGroupHandle_t xEventGroup )
 * {
 * EventBits_t uxBits;
 * const TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;
 *
 *      // 最多等待 100ms，直到事件组中的位 0 或位 4 被置位。
 *      // 退出前清除这些位。
 *      uxBits = xEventGroupWaitBits(
 *                  xEventGroup,    // 被检测的事件组。
 *                  BIT_0 | BIT_4,  // 要等待的事件组中的位。
 *                  pdTRUE,         // 返回前应清除 BIT_0 和 BIT_4。
 *                  pdFALSE,        // 不等待两个位都置位，任一位置位即可。
 *                  xTicksToWait ); // 最多等待 100ms，直到任一位被置位。
 *
 *      if( ( uxBits & ( BIT_0 | BIT_4 ) ) == ( BIT_0 | BIT_4 ) )
 *      {
 *          // xEventGroupWaitBits() 因两个位均已置位而返回。
 *      }
 *      else if( ( uxBits & BIT_0 ) != 0 )
 *      {
 *          // xEventGroupWaitBits() 仅因 BIT_0 被置位而返回。
 *      }
 *      else if( ( uxBits & BIT_4 ) != 0 )
 *      {
 *          // xEventGroupWaitBits() 仅因 BIT_4 被置位而返回。
 *      }
 *      else
 *      {
 *          // xEventGroupWaitBits() 因 xTicksToWait 个 tick 已过，且 BIT_0 和
 *          // BIT_4 均未变为置位而返回。
 *      }
 * }
 * @endcode
 * \defgroup xEventGroupWaitBits xEventGroupWaitBits
 * \ingroup EventGroup
 */
EventBits_t xEventGroupWaitBits( EventGroupHandle_t xEventGroup,
                                 const EventBits_t uxBitsToWaitFor,
                                 const BaseType_t xClearOnExit,
                                 const BaseType_t xWaitForAllBits,
                                 TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

/**
 * event_groups.h
 * @code{c}
 *  EventBits_t xEventGroupClearBits( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear );
 * @endcode
 *
 * 清除事件组中的位。此函数不能从中断中调用。
 *
 * 要使 xEventGroupClearBits() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @param xEventGroup 要清除其中位的事件组。
 *
 * @param uxBitsToClear 一个按位值，表示要清除的事件组中的位。例如，仅清除位 3 时，
 * 应将 uxBitsToClear 设为 0x08；清除位 3 和位 0 时，应将 uxBitsToClear 设为 0x09。
 *
 * @return 清除指定位之前的事件组值。
 *
 * 用法示例：
 * @code{c}
 * #define BIT_0 ( 1 << 0 )
 * #define BIT_4 ( 1 << 4 )
 *
 * void aFunction( EventGroupHandle_t xEventGroup )
 * {
 * EventBits_t uxBits;
 *
 *      // 清除 xEventGroup 中的位 0 和位 4。
 *      uxBits = xEventGroupClearBits(
 *                              xEventGroup,    // 被更新的事件组。
 *                              BIT_0 | BIT_4 );// 被清除的位。
 *
 *      if( ( uxBits & ( BIT_0 | BIT_4 ) ) == ( BIT_0 | BIT_4 ) )
 *      {
 *          // 调用 xEventGroupClearBits() 之前，位 0 和位 4 均已置位。
 *          // 现在两者都已被清除（未置位）。
 *      }
 *      else if( ( uxBits & BIT_0 ) != 0 )
 *      {
 *          // 调用 xEventGroupClearBits() 之前位 0 已置位。现在它已被清除。
 *      }
 *      else if( ( uxBits & BIT_4 ) != 0 )
 *      {
 *          // 调用 xEventGroupClearBits() 之前位 4 已置位。现在它已被清除。
 *      }
 *      else
 *      {
 *          // 位 0 和位 4 原本就均未置位。
 *      }
 * }
 * @endcode
 * \defgroup xEventGroupClearBits xEventGroupClearBits
 * \ingroup EventGroup
 */
EventBits_t xEventGroupClearBits( EventGroupHandle_t xEventGroup,
                                  const EventBits_t uxBitsToClear ) PRIVILEGED_FUNCTION;

/**
 * event_groups.h
 * @code{c}
 *  BaseType_t xEventGroupClearBitsFromISR( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet );
 * @endcode
 *
 * xEventGroupClearBits() 的可从中断调用的版本。
 *
 * 在事件组中置位不是确定性操作，因为可能有未知数量的任务正在等待所置位的位。
 * FreeRTOS 不允许在中断关闭期间执行非确定性操作，因此对于从任务访问的事件组，
 * 通过挂起调度器而不是关闭中断来保护。因此，事件组不能直接从中断服务程序中
 * 访问。所以，xEventGroupClearBitsFromISR() 会向定时器任务发送一条消息，由定时器
 * 任务在其上下文中执行清除操作。
 *
 * @note 若此函数返回 pdPASS，则定时器任务已准备好运行，应执行
 * portYIELD_FROM_ISR(pdTRUE)，以对事件组执行所需的清除操作。此行为与
 * xEventGroupSetBitsFromISR 不同，因为此处没有 xHigherPriorityTaskWoken 参数。
 *
 * @param xEventGroup 要清除其中位的事件组。
 *
 * @param uxBitsToClear 一个按位值，表示要清除的位。例如，仅清除位 3 时，应将
 * uxBitsToClear 设为 0x08；清除位 3 和位 0 时，应将 uxBitsToClear 设为 0x09。
 *
 * @return 若执行该函数的请求成功提交，则返回 pdPASS；否则返回 pdFALSE。若定时器
 * 服务队列已满，则返回 pdFALSE。
 *
 * 用法示例：
 * @code{c}
 * #define BIT_0 ( 1 << 0 )
 * #define BIT_4 ( 1 << 4 )
 *
 * // 一个假定已通过调用 xEventGroupCreate() 创建的事件组。
 * EventGroupHandle_t xEventGroup;
 *
 * void anInterruptHandler( void )
 * {
 *      // 清除 xEventGroup 中的位 0 和位 4。
 *      xResult = xEventGroupClearBitsFromISR(
 *                          xEventGroup,     // 被更新的事件组。
 *                          BIT_0 | BIT_4 ); // 被置位的位。
 *
 *      if( xResult == pdPASS )
 *      {
 *          // 消息提交成功。
 *          portYIELD_FROM_ISR(pdTRUE);
 *      }
 * }
 * @endcode
 * \defgroup xEventGroupClearBitsFromISR xEventGroupClearBitsFromISR
 * \ingroup EventGroup
 */
#if ( ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) )
    BaseType_t xEventGroupClearBitsFromISR( EventGroupHandle_t xEventGroup,
                                            const EventBits_t uxBitsToClear ) PRIVILEGED_FUNCTION;
#endif /* 当 INCLUDE_xTimerPendFunctionCall == 1 且 configUSE_TIMERS == 1 时 */

/**
 * event_groups.h
 * @code{c}
 *  EventBits_t xEventGroupSetBits( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet );
 * @endcode
 *
 * 置位事件组中的位。
 * 此函数不能从中断中调用。xEventGroupSetBitsFromISR() 是可从中断调用的版本。
 *
 * 置位事件组中的位会自动解除等待这些位的任务的阻塞。
 *
 * 要使 xEventGroupSetBits() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @param xEventGroup 要置位其中位的事件组。
 *
 * @param uxBitsToSet 一个按位值，表示要置位的位。例如，仅置位位 3 时，应将
 * uxBitsToSet 设为 0x08；置位位 3 和位 0 时，应将 uxBitsToSet 设为 0x09。
 *
 * @return 调用 xEventGroupSetBits() 返回时事件组的值。若置位某位导致正在等待
 * 该位的任务离开阻塞态，则该位可能被自动清除，因此返回值中由 uxBitsToSet 参数
 * 指定的位可能已被清除（参见 xEventGroupWaitBits() 的 xClearBitOnExit 参数）。
 *
 * 用法示例：
 * @code{c}
 * #define BIT_0 ( 1 << 0 )
 * #define BIT_4 ( 1 << 4 )
 *
 * void aFunction( EventGroupHandle_t xEventGroup )
 * {
 * EventBits_t uxBits;
 *
 *      // 置位 xEventGroup 中的位 0 和位 4。
 *      uxBits = xEventGroupSetBits(
 *                          xEventGroup,    // 被更新的事件组。
 *                          BIT_0 | BIT_4 );// 被置位的位。
 *
 *      if( ( uxBits & ( BIT_0 | BIT_4 ) ) == ( BIT_0 | BIT_4 ) )
 *      {
 *          // 函数返回时，位 0 和位 4 均保持置位。
 *      }
 *      else if( ( uxBits & BIT_0 ) != 0 )
 *      {
 *          // 函数返回时位 0 保持置位，但位 4 已被清除。可能是等待位 4 的
 *          // 任务离开阻塞态时，位 4 被自动清除。
 *      }
 *      else if( ( uxBits & BIT_4 ) != 0 )
 *      {
 *          // 函数返回时位 4 保持置位，但位 0 已被清除。可能是等待位 0 的
 *          // 任务离开阻塞态时，位 0 被自动清除。
 *      }
 *      else
 *      {
 *          // 位 0 和位 4 均未保持置位。可能是某个任务正在等待这两个位
 *          // 均置位，而任务离开阻塞态时这些位被清除。
 *      }
 * }
 * @endcode
 * \defgroup xEventGroupSetBits xEventGroupSetBits
 * \ingroup EventGroup
 */
EventBits_t xEventGroupSetBits( EventGroupHandle_t xEventGroup,
                                const EventBits_t uxBitsToSet ) PRIVILEGED_FUNCTION;

/**
 * event_groups.h
 * @code{c}
 *  BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet, BaseType_t *pxHigherPriorityTaskWoken );
 * @endcode
 *
 * xEventGroupSetBits() 的可从中断调用的版本。
 *
 * 在事件组中置位不是确定性操作，因为可能有未知数量的任务正在等待所置位的位。
 * FreeRTOS 不允许在中断或临界区中执行非确定性操作。因此，
 * xEventGroupSetBitsFromISR() 会向定时器任务发送一条消息，由定时器任务在其
 * 上下文中执行置位操作——在定时器任务中使用调度器锁代替临界区。
 *
 * @param xEventGroup 要置位其中位的事件组。
 *
 * @param uxBitsToSet 一个按位值，表示要置位的位。例如，仅置位位 3 时，应将
 * uxBitsToSet 设为 0x08；置位位 3 和位 0 时，应将 uxBitsToSet 设为 0x09。
 *
 * @param pxHigherPriorityTaskWoken 如上所述，调用此函数会向定时器守护任务发送
 * 一条消息。若定时器守护任务的优先级高于当前正在运行的任务（被中断的任务）的
 * 优先级，则 xEventGroupSetBitsFromISR() 会将 *pxHigherPriorityTaskWoken 设为
 * pdTRUE，表示应在中断退出之前请求上下文切换。因此，*pxHigherPriorityTaskWoken
 * 必须初始化为 pdFALSE。参见下面的示例代码。
 *
 * @return 若执行该函数的请求成功提交，则返回 pdPASS；否则返回 pdFALSE。若定时器
 * 服务队列已满，则返回 pdFALSE。
 *
 * 用法示例：
 * @code{c}
 * #define BIT_0 ( 1 << 0 )
 * #define BIT_4 ( 1 << 4 )
 *
 * // 一个假定已通过调用 xEventGroupCreate() 创建的事件组。
 * EventGroupHandle_t xEventGroup;
 *
 * void anInterruptHandler( void )
 * {
 * BaseType_t xHigherPriorityTaskWoken, xResult;
 *
 *      // xHigherPriorityTaskWoken 必须初始化为 pdFALSE。
 *      xHigherPriorityTaskWoken = pdFALSE;
 *
 *      // 置位 xEventGroup 中的位 0 和位 4。
 *      xResult = xEventGroupSetBitsFromISR(
 *                          xEventGroup,    // 被更新的事件组。
 *                          BIT_0 | BIT_4   // 被置位的位。
 *                          &xHigherPriorityTaskWoken );
 *
 *      // 消息是否提交成功？
 *      if( xResult == pdPASS )
 *      {
 *          // 若 xHigherPriorityTaskWoken 现在被设为 pdTRUE，则应请求上下文
 *          // 切换。所用宏与移植层相关，为 portYIELD_FROM_ISR() 或
 *          // portEND_SWITCHING_ISR() 之一——请参阅所用移植层的文档页面。
 *          portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 *      }
 * }
 * @endcode
 * \defgroup xEventGroupSetBitsFromISR xEventGroupSetBitsFromISR
 * \ingroup EventGroup
 */
#if ( ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) )
    BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup,
                                          const EventBits_t uxBitsToSet,
                                          BaseType_t * pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
#endif /* 当 INCLUDE_xTimerPendFunctionCall == 1 且 configUSE_TIMERS == 1 时 */

/**
 * event_groups.h
 * @code{c}
 *  EventBits_t xEventGroupSync(    EventGroupHandle_t xEventGroup,
 *                                  const EventBits_t uxBitsToSet,
 *                                  const EventBits_t uxBitsToWaitFor,
 *                                  TickType_t xTicksToWait );
 * @endcode
 *
 * 原子地置位事件组中的位，然后等待同一事件组中一组位的组合被置位。该功能通常
 * 用于同步多个任务，其中每个任务都必须等待其他任务到达同步点后才能继续执行。
 *
 * 此函数不能从中断中使用。
 *
 * 若 uxBitsToWait 指定的位已置位，或在该时间内变为置位，则函数将在阻塞时间
 * 到期之前返回。此时，uxBitsToWait 指定的所有位将在函数返回之前被自动清除。
 *
 * 要使 xEventGroupSync() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @param xEventGroup 正在检测其中事件位的事件组。该事件组必须已通过调用
 * xEventGroupCreate() 创建。
 *
 * @param uxBitsToSet 在确定 uxBitsToWait 参数指定的所有位是否已置位（以及可能
 * 进行等待）之前，要在事件组中置位的位。
 *
 * @param uxBitsToWaitFor 一个按位值，表示要在事件组中检测的位。例如，要等待
 * 位 0 和位 2，应将 uxBitsToWaitFor 设为 0x05；要等待位 0、位 1 和位 2，应将
 * uxBitsToWaitFor 设为 0x07；依此类推。
 *
 * @param xTicksToWait 等待 uxBitsToWaitFor 指定的所有位变为置位的最大时间
 * （以“tick”为单位）。
 *
 * @return 在等待的位变为置位时，或阻塞时间到期时，事件组的值。通过检测返回值
 * 可确定哪些位已置位。若 xEventGroupSync() 因超时到期而返回，则等待的位并非
 * 全部置位。若 xEventGroupSync() 因等待的所有位均已置位而返回，则返回值为自动
 * 清除任何位之前的事件组值。
 *
 * 用法示例：
 * @code{c}
 * // 三个任务使用的位。
 * #define TASK_0_BIT     ( 1 << 0 )
 * #define TASK_1_BIT     ( 1 << 1 )
 * #define TASK_2_BIT     ( 1 << 2 )
 *
 * #define ALL_SYNC_BITS ( TASK_0_BIT | TASK_1_BIT | TASK_2_BIT )
 *
 * // 使用一个事件组同步三个任务。假定该事件组已在其他地方创建。
 * EventGroupHandle_t xEventBits;
 *
 * void vTask0( void *pvParameters )
 * {
 * EventBits_t uxReturn;
 * TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;
 *
 *   for( ;; )
 *   {
 *      // 在此执行任务功能。
 *
 *      // 置位事件标志中的位 0，以表明此任务已到达同步点。另外两个任务
 *      // 将置位 ALL_SYNC_BITS 定义的另外两个位。当 ALL_SYNC_BITS 中的
 *      // 所有位均置位时，三个任务都已到达同步点。最多等待 100ms 以使
 *      // 此条件成立。
 *      uxReturn = xEventGroupSync( xEventBits, TASK_0_BIT, ALL_SYNC_BITS, xTicksToWait );
 *
 *      if( ( uxReturn & ALL_SYNC_BITS ) == ALL_SYNC_BITS )
 *      {
 *          // 三个任务均在调用 xEventGroupSync() 超时之前到达同步点。
 *      }
 *  }
 * }
 *
 * void vTask1( void *pvParameters )
 * {
 *   for( ;; )
 *   {
 *      // 在此执行任务功能。
 *
 *      // 置位事件标志中的位 1，以表明此任务已到达同步点。另外两个任务
 *      // 将置位 ALL_SYNC_BITS 定义的另外两个位。当 ALL_SYNC_BITS 中的
 *      // 所有位均置位时，三个任务都已到达同步点。无限期等待此条件成立。
 *      xEventGroupSync( xEventBits, TASK_1_BIT, ALL_SYNC_BITS, portMAX_DELAY );
 *
 *      // xEventGroupSync() 以无限期阻塞时间调用，因此只有三个任务均完成
 *      // 同步后此任务才会执行到这里，无需检测返回值。
 *   }
 * }
 *
 * void vTask2( void *pvParameters )
 * {
 *   for( ;; )
 *   {
 *      // 在此执行任务功能。
 *
 *      // 置位事件标志中的位 2，以表明此任务已到达同步点。另外两个任务
 *      // 将置位 ALL_SYNC_BITS 定义的另外两个位。当 ALL_SYNC_BITS 中的
 *      // 所有位均置位时，三个任务都已到达同步点。无限期等待此条件成立。
 *      xEventGroupSync( xEventBits, TASK_2_BIT, ALL_SYNC_BITS, portMAX_DELAY );
 *
 *      // xEventGroupSync() 以无限期阻塞时间调用，因此只有三个任务均完成
 *      // 同步后此任务才会执行到这里，无需检测返回值。
 *  }
 * }
 *
 * @endcode
 * \defgroup xEventGroupSync xEventGroupSync
 * \ingroup EventGroup
 */
EventBits_t xEventGroupSync( EventGroupHandle_t xEventGroup,
                             const EventBits_t uxBitsToSet,
                             const EventBits_t uxBitsToWaitFor,
                             TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;


/**
 * event_groups.h
 * @code{c}
 *  EventBits_t xEventGroupGetBits( EventGroupHandle_t xEventGroup );
 * @endcode
 *
 * 返回事件组中事件位的当前值。此函数不能从中断中使用。
 *
 * 要使 xEventGroupGetBits() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @param xEventGroup 被查询的事件组。
 *
 * @return 调用 xEventGroupGetBits() 时的事件组位值。
 *
 * \defgroup xEventGroupGetBits xEventGroupGetBits
 * \ingroup EventGroup
 */
#define xEventGroupGetBits( xEventGroup )    xEventGroupClearBits( ( xEventGroup ), 0 )

/**
 * event_groups.h
 * @code{c}
 *  EventBits_t xEventGroupGetBitsFromISR( EventGroupHandle_t xEventGroup );
 * @endcode
 *
 * xEventGroupGetBits() 的可从中断服务程序（ISR）调用的版本。
 *
 * 要使 xEventGroupGetBitsFromISR() 可用，配置常量 configUSE_EVENT_GROUPS 必须
 * 设置为 1。
 *
 * @param xEventGroup 被查询的事件组。
 *
 * @return 调用 xEventGroupGetBitsFromISR() 时的事件组位值。
 *
 * \defgroup xEventGroupGetBitsFromISR xEventGroupGetBitsFromISR
 * \ingroup EventGroup
 */
EventBits_t xEventGroupGetBitsFromISR( EventGroupHandle_t xEventGroup ) PRIVILEGED_FUNCTION;

/**
 * event_groups.h
 * @code{c}
 *  void xEventGroupDelete( EventGroupHandle_t xEventGroup );
 * @endcode
 *
 * 删除先前通过调用 xEventGroupCreate() 创建的事件组。阻塞在该事件组上的任务
 * 将被解除阻塞，并获得 0 作为事件组的值。
 *
 * 要使 vEventGroupDelete() 可用，配置常量 configUSE_EVENT_GROUPS 必须设置为 1。
 *
 * @param xEventGroup 被删除的事件组。
 */
void vEventGroupDelete( EventGroupHandle_t xEventGroup ) PRIVILEGED_FUNCTION;

/**
 * event_groups.h
 * @code{c}
 *  BaseType_t xEventGroupGetStaticBuffer( EventGroupHandle_t xEventGroup,
 *                                         StaticEventGroup_t ** ppxEventGroupBuffer );
 * @endcode
 *
 * 获取指向静态创建的事件组数据结构缓冲区的指针。该缓冲区与创建事件组时提供的
 * 缓冲区相同。
 *
 * 要使 xEventGroupGetStaticBuffer() 可用，配置常量 configUSE_EVENT_GROUPS 必须
 * 设置为 1。
 *
 * @param xEventGroup 要获取其缓冲区的事件组。
 *
 * @param ppxEventGroupBuffer 用于返回指向事件组数据结构缓冲区的指针。
 *
 * @return 若成功获取缓冲区，则返回 pdTRUE；否则返回 pdFALSE。
 */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    BaseType_t xEventGroupGetStaticBuffer( EventGroupHandle_t xEventGroup,
                                           StaticEventGroup_t ** ppxEventGroupBuffer ) PRIVILEGED_FUNCTION;
#endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */

/* 仅供内部使用。 */
void vEventGroupSetBitsCallback( void * pvEventGroup,
                                 uint32_t ulBitsToSet ) PRIVILEGED_FUNCTION;
void vEventGroupClearBitsCallback( void * pvEventGroup,
                                   uint32_t ulBitsToClear ) PRIVILEGED_FUNCTION;


#if ( configUSE_TRACE_FACILITY == 1 )
    UBaseType_t uxEventGroupGetNumber( void * xEventGroup ) PRIVILEGED_FUNCTION;
    void vEventGroupSetNumber( void * xEventGroup,
                               UBaseType_t uxEventGroupNumber ) PRIVILEGED_FUNCTION;
#endif

/* *INDENT-OFF* —— 暂停自动缩进（uncrustify 指令） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* —— 恢复自动缩进（uncrustify 指令） */

#endif /* 头文件保护宏 EVENT_GROUPS_H */
