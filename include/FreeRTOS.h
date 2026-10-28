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

#ifndef INC_FREERTOS_H

/*
 * 【中文导读】FreeRTOS.h —— 主头文件（B级导读）
 * 汇总全部子头文件，并对 FreeRTOSConfig.h 做"默认值+合法性检查"。
 * 改配置报错先看这里给出的提示，信息非常友好。
 */
#define INC_FREERTOS_H

/*
 * 包含当前所用 FreeRTOS 移植层所需的通用头文件。
 */
#include <stddef.h>

/*
 * 如果无法找到 stdint.h，则：
 *   + 使用 GCC 时，确保*未*使用 -nostdint 选项。
 *   + 确保工程的包含路径中包含编译器存放 stdint.h 的目录。
 *   + 设置必要的编译器选项以支持 C99，因为严格来说 stdint.h 只在 C99 中才是必需的（FreeRTOS 在其他方面并不要求 C99）。
 *   + FreeRTOS 下载包附带一个简化的 stdint.h 定义，可在编译器未提供该文件时使用。该文件只包含构建 FreeRTOS 所需的 typedef。更多信息请阅读 FreeRTOS/source/stdint.readme 中的说明。
 */
#include <stdint.h> /* 请阅读上方的注释。 */

/* configTICK_TYPE_WIDTH_IN_BITS 的可接受取值。 */
#define TICK_TYPE_WIDTH_16_BITS    0
#define TICK_TYPE_WIDTH_32_BITS    1
#define TICK_TYPE_WIDTH_64_BITS    2

/* 应用特定的配置选项。 */
#include "FreeRTOSConfig.h"

#if !defined( configUSE_16_BIT_TICKS ) && !defined( configTICK_TYPE_WIDTH_IN_BITS )
    #error Missing definition:  One of configUSE_16_BIT_TICKS and configTICK_TYPE_WIDTH_IN_BITS must be defined in FreeRTOSConfig.h.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

#if defined( configUSE_16_BIT_TICKS ) && defined( configTICK_TYPE_WIDTH_IN_BITS )
    #error Only one of configUSE_16_BIT_TICKS and configTICK_TYPE_WIDTH_IN_BITS must be defined in FreeRTOSConfig.h.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

/*
 * 根据 configUSE_16_BIT_TICKS 的取值定义 configTICK_TYPE_WIDTH_IN_BITS，以保持向后兼容性。
 */
#ifndef configTICK_TYPE_WIDTH_IN_BITS
    #if ( configUSE_16_BIT_TICKS == 1 )
        #define configTICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_16_BITS
    #else
        #define configTICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
    #endif
#endif

/* 将 configUSE_MPU_WRAPPERS_V1 设置为 1，以使用 MPU 封装层 v1。 */
#ifndef configUSE_MPU_WRAPPERS_V1
    #define configUSE_MPU_WRAPPERS_V1    0
#endif

/* 将 configENABLE_ACCESS_CONTROL_LIST 设置为 1，以启用访问控制列表支持。 */
#ifndef configENABLE_ACCESS_CONTROL_LIST
    #define configENABLE_ACCESS_CONTROL_LIST    0
#endif

/* 将 configNUMBER_OF_CORES 的默认值设为 1，以使用单核 FreeRTOS。 */
#ifndef configNUMBER_OF_CORES
    #define configNUMBER_OF_CORES    1
#endif

#ifndef configUSE_MALLOC_FAILED_HOOK
    #define configUSE_MALLOC_FAILED_HOOK    0
#endif

#ifndef configASSERT
    #define configASSERT( x )
    #define configASSERT_DEFINED    0
#else
    #define configASSERT_DEFINED    1
#endif

/*
 * 将 configENABLE_PAC 和/或 configENABLE_BTI 设置为 1，以启用 PAC 和/或 BTI 支持；设置为 0 则禁用。这些配置目前用于 ARMv8.1-M 移植层。
 */
#ifndef configENABLE_PAC
    #define configENABLE_PAC    0
#endif

#ifndef configENABLE_BTI
    #define configENABLE_BTI    0
#endif

/* FreeRTOS 基本定义。 */
#include "projdefs.h"

/* 所用移植层特有的定义。 */
#include "portable.h"

/* 必须在下方使用 configUSE_NEWLIB_REENTRANT 之前为其提供默认值。 */
#ifndef configUSE_NEWLIB_REENTRANT
    #define configUSE_NEWLIB_REENTRANT    0
#endif

/* 使用 struct _reent 时必须包含。 */
#if ( configUSE_NEWLIB_REENTRANT == 1 )

    #include "newlib-freertos.h"

#endif /* 结束：if ( configUSE_NEWLIB_REENTRANT == 1 ) */

/* 必须在下方使用 configUSE_PICOLIBC_TLS 之前为其提供默认值。 */
#ifndef configUSE_PICOLIBC_TLS
    #define configUSE_PICOLIBC_TLS    0
#endif

#if ( configUSE_PICOLIBC_TLS == 1 )

    #include "picolibc-freertos.h"

#endif /* 结束：if ( configUSE_PICOLIBC_TLS == 1 ) */

/* *INDENT-OFF* 关闭自动缩进 */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* 恢复自动缩进 */

#ifndef configUSE_C_RUNTIME_TLS_SUPPORT
    #define configUSE_C_RUNTIME_TLS_SUPPORT    0
#endif

#if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )

    #ifndef configTLS_BLOCK_TYPE
        #error Missing definition:  configTLS_BLOCK_TYPE must be defined in FreeRTOSConfig.h when configUSE_C_RUNTIME_TLS_SUPPORT is set to 1.
    #endif

    #ifndef configINIT_TLS_BLOCK
        #error Missing definition:  configINIT_TLS_BLOCK must be defined in FreeRTOSConfig.h when configUSE_C_RUNTIME_TLS_SUPPORT is set to 1.
    #endif

    #ifndef configSET_TLS_BLOCK
        #error Missing definition:  configSET_TLS_BLOCK must be defined in FreeRTOSConfig.h when configUSE_C_RUNTIME_TLS_SUPPORT is set to 1.
    #endif

    #ifndef configDEINIT_TLS_BLOCK
        #error Missing definition:  configDEINIT_TLS_BLOCK must be defined in FreeRTOSConfig.h when configUSE_C_RUNTIME_TLS_SUPPORT is set to 1.
    #endif
#endif /* 结束：if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 ) */

/*
 * 检查所有必需的应用特定宏是否均已定义。
 * 这些宏与应用相关，在下载的代码中定义于 FreeRTOSConfig.h 内。
 */

#ifndef configMINIMAL_STACK_SIZE
    #error Missing definition:  configMINIMAL_STACK_SIZE must be defined in FreeRTOSConfig.h.  configMINIMAL_STACK_SIZE defines the size (in words) of the stack allocated to the idle task.  Refer to the demo project provided for your port for a suitable value.
#endif

#ifndef configMAX_PRIORITIES
    #error Missing definition:  configMAX_PRIORITIES must be defined in FreeRTOSConfig.h.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

#if configMAX_PRIORITIES < 1
    #error configMAX_PRIORITIES must be defined to be greater than or equal to 1.
#endif

#ifndef configUSE_PREEMPTION
    #error Missing definition:  configUSE_PREEMPTION must be defined in FreeRTOSConfig.h as either 1 or 0.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

#ifndef configUSE_IDLE_HOOK
    #error Missing definition:  configUSE_IDLE_HOOK must be defined in FreeRTOSConfig.h as either 1 or 0.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

#if ( configNUMBER_OF_CORES > 1 )
    #ifndef configUSE_PASSIVE_IDLE_HOOK
        #error Missing definition:  configUSE_PASSIVE_IDLE_HOOK must be defined in FreeRTOSConfig.h as either 1 or 0.  See the Configuration section of the FreeRTOS API documentation for details.
    #endif
#endif

#ifndef configUSE_TICK_HOOK
    #error Missing definition:  configUSE_TICK_HOOK must be defined in FreeRTOSConfig.h as either 1 or 0.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

#if ( ( configTICK_TYPE_WIDTH_IN_BITS != TICK_TYPE_WIDTH_16_BITS ) && \
    ( configTICK_TYPE_WIDTH_IN_BITS != TICK_TYPE_WIDTH_32_BITS ) &&   \
    ( configTICK_TYPE_WIDTH_IN_BITS != TICK_TYPE_WIDTH_64_BITS ) )
    #error Macro configTICK_TYPE_WIDTH_IN_BITS is defined to incorrect value.  See the Configuration section of the FreeRTOS API documentation for details.
#endif

#ifndef configUSE_CO_ROUTINES
    #define configUSE_CO_ROUTINES    0
#endif

#ifndef INCLUDE_vTaskPrioritySet
    #define INCLUDE_vTaskPrioritySet    0
#endif

#ifndef INCLUDE_uxTaskPriorityGet
    #define INCLUDE_uxTaskPriorityGet    0
#endif

#ifndef INCLUDE_vTaskDelete
    #define INCLUDE_vTaskDelete    0
#endif

#ifndef INCLUDE_vTaskSuspend
    #define INCLUDE_vTaskSuspend    0
#endif

#ifdef INCLUDE_xTaskDelayUntil
    #ifdef INCLUDE_vTaskDelayUntil

/*
 * INCLUDE_vTaskDelayUntil 已被 INCLUDE_xTaskDelayUntil 取代。若仅定义其中一个，则可保持向后兼容；若两者同时定义，则会产生冲突。
 */
        #error INCLUDE_vTaskDelayUntil and INCLUDE_xTaskDelayUntil are both defined.  INCLUDE_vTaskDelayUntil is no longer required and should be removed
    #endif
#endif

#ifndef INCLUDE_xTaskDelayUntil
    #ifdef INCLUDE_vTaskDelayUntil

/*
 * 如果设置了 INCLUDE_vTaskDelayUntil 但未设置 INCLUDE_xTaskDelayUntil，
 * 说明工程的 FreeRTOSConfig.h 很可能早于 xTaskDelayUntil 的引入；
 * 此时将 INCLUDE_xTaskDelayUntil 设置为与 INCLUDE_vTaskDelayUntil 相同的值即可确保向后兼容。
 */
        #define INCLUDE_xTaskDelayUntil    INCLUDE_vTaskDelayUntil
    #endif
#endif

#ifndef INCLUDE_xTaskDelayUntil
    #define INCLUDE_xTaskDelayUntil    0
#endif

#ifndef INCLUDE_vTaskDelay
    #define INCLUDE_vTaskDelay    0
#endif

#ifndef INCLUDE_xTaskGetIdleTaskHandle
    #define INCLUDE_xTaskGetIdleTaskHandle    0
#endif

#ifndef INCLUDE_xTaskAbortDelay
    #define INCLUDE_xTaskAbortDelay    0
#endif

#ifndef INCLUDE_xQueueGetMutexHolder
    #define INCLUDE_xQueueGetMutexHolder    0
#endif

#ifndef INCLUDE_xSemaphoreGetMutexHolder
    #define INCLUDE_xSemaphoreGetMutexHolder    INCLUDE_xQueueGetMutexHolder
#endif

#ifndef INCLUDE_xTaskGetHandle
    #define INCLUDE_xTaskGetHandle    0
#endif

#ifndef INCLUDE_uxTaskGetStackHighWaterMark
    #define INCLUDE_uxTaskGetStackHighWaterMark    0
#endif

#ifndef INCLUDE_uxTaskGetStackHighWaterMark2
    #define INCLUDE_uxTaskGetStackHighWaterMark2    0
#endif

#ifndef INCLUDE_eTaskGetState
    #define INCLUDE_eTaskGetState    0
#endif

#ifndef INCLUDE_xTaskResumeFromISR
    #define INCLUDE_xTaskResumeFromISR    1
#endif

#ifndef INCLUDE_xTimerPendFunctionCall
    #define INCLUDE_xTimerPendFunctionCall    0
#endif

#ifndef INCLUDE_xTaskGetSchedulerState
    #define INCLUDE_xTaskGetSchedulerState    0
#endif

#ifndef INCLUDE_xTaskGetCurrentTaskHandle
    #define INCLUDE_xTaskGetCurrentTaskHandle    1
#endif

#if configUSE_CO_ROUTINES != 0
    #ifndef configMAX_CO_ROUTINE_PRIORITIES
        #error configMAX_CO_ROUTINE_PRIORITIES must be greater than or equal to 1.
    #endif
#endif

#ifndef configUSE_APPLICATION_TASK_TAG
    #define configUSE_APPLICATION_TASK_TAG    0
#endif

#ifndef configNUM_THREAD_LOCAL_STORAGE_POINTERS
    #define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0
#endif

#ifndef configUSE_RECURSIVE_MUTEXES
    #define configUSE_RECURSIVE_MUTEXES    0
#endif

#ifndef configUSE_MUTEXES
    #define configUSE_MUTEXES    0
#endif

#ifndef configUSE_TIMERS
    #define configUSE_TIMERS    0
#endif

#ifndef configUSE_EVENT_GROUPS
    #define configUSE_EVENT_GROUPS    1
#endif

#ifndef configUSE_STREAM_BUFFERS
    #define configUSE_STREAM_BUFFERS    1
#endif

#ifndef configUSE_DAEMON_TASK_STARTUP_HOOK
    #define configUSE_DAEMON_TASK_STARTUP_HOOK    0
#endif

#if ( configUSE_DAEMON_TASK_STARTUP_HOOK != 0 )
    #if ( configUSE_TIMERS == 0 )
        #error configUSE_DAEMON_TASK_STARTUP_HOOK is set, but the daemon task is not created because configUSE_TIMERS is 0.
    #endif
#endif

#ifndef configUSE_COUNTING_SEMAPHORES
    #define configUSE_COUNTING_SEMAPHORES    0
#endif

#ifndef configUSE_TASK_PREEMPTION_DISABLE
    #define configUSE_TASK_PREEMPTION_DISABLE    0
#endif

#ifndef configUSE_ALTERNATIVE_API
    #define configUSE_ALTERNATIVE_API    0
#endif

#ifndef portCRITICAL_NESTING_IN_TCB
    #define portCRITICAL_NESTING_IN_TCB    0
#endif

#ifndef configMAX_TASK_NAME_LEN
    #define configMAX_TASK_NAME_LEN    16
#endif

#ifndef configIDLE_SHOULD_YIELD
    #define configIDLE_SHOULD_YIELD    1
#endif

#ifndef configIDLE_AFFINITY
    #define configIDLE_AFFINITY    0
#endif

#if configMAX_TASK_NAME_LEN < 1
    #error configMAX_TASK_NAME_LEN must be set to a minimum of 1 in FreeRTOSConfig.h
#endif

/*
 * configPRECONDITION 应定义为 configASSERT。
 * CBMC 证明需要一种跟踪假设与断言的方法。
 * configPRECONDITION 语句应表达隐含的不变量或所作假设。
 * configASSERT 语句应表达在调用代码之前必须显式成立的不变量。
 */
#ifndef configPRECONDITION
    #define configPRECONDITION( X )    configASSERT( X )
    #define configPRECONDITION_DEFINED    0
#else
    #define configPRECONDITION_DEFINED    1
#endif

#ifndef configCHECK_HANDLER_INSTALLATION
    #define configCHECK_HANDLER_INSTALLATION    1
#else

/*
 * 应用已显式将 configCHECK_HANDLER_INSTALLATION 定义为 1。
 * 该检查要求定义 configASSERT()。
 */
    #if ( ( configCHECK_HANDLER_INSTALLATION == 1 ) && ( configASSERT_DEFINED == 0 ) )
        #error You must define configASSERT() when configCHECK_HANDLER_INSTALLATION is 1.
    #endif
#endif

#ifndef portMEMORY_BARRIER
    #define portMEMORY_BARRIER()
#endif

#ifndef portSOFTWARE_BARRIER
    #define portSOFTWARE_BARRIER()
#endif

#ifndef configRUN_MULTIPLE_PRIORITIES
    #define configRUN_MULTIPLE_PRIORITIES    0
#endif

#ifndef portGET_CORE_ID

    #if ( configNUMBER_OF_CORES == 1 )
        #define portGET_CORE_ID()    0
    #else
        #error configNUMBER_OF_CORES is set to more than 1 then portGET_CORE_ID must also be defined.
    #endif /* configNUMBER_OF_CORES 结束 */

#endif /* portGET_CORE_ID 结束 */

#ifndef portYIELD_CORE

    #if ( configNUMBER_OF_CORES == 1 )
        #define portYIELD_CORE( x )    portYIELD()
    #else
        #error configNUMBER_OF_CORES is set to more than 1 then portYIELD_CORE must also be defined.
    #endif /* configNUMBER_OF_CORES 结束 */

#endif /* portYIELD_CORE 结束 */

#ifndef portSET_INTERRUPT_MASK

    #if ( configNUMBER_OF_CORES > 1 )
        #error portSET_INTERRUPT_MASK is required in SMP
    #endif

#endif /* portSET_INTERRUPT_MASK 结束 */

#ifndef portCLEAR_INTERRUPT_MASK

    #if ( configNUMBER_OF_CORES > 1 )
        #error portCLEAR_INTERRUPT_MASK is required in SMP
    #endif

#endif /* portCLEAR_INTERRUPT_MASK 结束 */

#ifndef portRELEASE_TASK_LOCK

    #if ( configNUMBER_OF_CORES == 1 )
        #define portRELEASE_TASK_LOCK( xCoreID )
    #else
        #error portRELEASE_TASK_LOCK is required in SMP
    #endif

#endif /* portRELEASE_TASK_LOCK 结束 */

#ifndef portGET_TASK_LOCK

    #if ( configNUMBER_OF_CORES == 1 )
        #define portGET_TASK_LOCK( xCoreID )
    #else
        #error portGET_TASK_LOCK is required in SMP
    #endif

#endif /* portGET_TASK_LOCK 结束 */

#ifndef portRELEASE_ISR_LOCK

    #if ( configNUMBER_OF_CORES == 1 )
        #define portRELEASE_ISR_LOCK( xCoreID )
    #else
        #error portRELEASE_ISR_LOCK is required in SMP
    #endif

#endif /* portRELEASE_ISR_LOCK 结束 */

#ifndef portGET_ISR_LOCK

    #if ( configNUMBER_OF_CORES == 1 )
        #define portGET_ISR_LOCK( xCoreID )
    #else
        #error portGET_ISR_LOCK is required in SMP
    #endif

#endif /* portGET_ISR_LOCK 结束 */

#ifndef portENTER_CRITICAL_FROM_ISR

    #if ( configNUMBER_OF_CORES > 1 )
        #error portENTER_CRITICAL_FROM_ISR is required in SMP
    #endif

#endif

#ifndef portEXIT_CRITICAL_FROM_ISR

    #if ( configNUMBER_OF_CORES > 1 )
        #error portEXIT_CRITICAL_FROM_ISR is required in SMP
    #endif

#endif

#ifndef configUSE_CORE_AFFINITY
    #define configUSE_CORE_AFFINITY    0
#endif /* configUSE_CORE_AFFINITY 结束 */

#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_CORE_AFFINITY == 1 ) )
    #ifndef configTASK_DEFAULT_CORE_AFFINITY
        #define configTASK_DEFAULT_CORE_AFFINITY    tskNO_AFFINITY
    #endif
#endif

#ifndef configUSE_PASSIVE_IDLE_HOOK
    #define configUSE_PASSIVE_IDLE_HOOK    0
#endif /* configUSE_PASSIVE_IDLE_HOOK 结束 */

/* 定时器模块依赖 xTaskGetSchedulerState()。 */
#if configUSE_TIMERS == 1

    #ifndef configTIMER_TASK_PRIORITY
        #error If configUSE_TIMERS is set to 1 then configTIMER_TASK_PRIORITY must also be defined.
    #endif /* configTIMER_TASK_PRIORITY 结束 */

    #ifndef configTIMER_QUEUE_LENGTH
        #error If configUSE_TIMERS is set to 1 then configTIMER_QUEUE_LENGTH must also be defined.
    #endif /* configTIMER_QUEUE_LENGTH 结束 */

    #ifndef configTIMER_TASK_STACK_DEPTH
        #error If configUSE_TIMERS is set to 1 then configTIMER_TASK_STACK_DEPTH must also be defined.
    #endif /* configTIMER_TASK_STACK_DEPTH 结束 */

    #ifndef portTIMER_CALLBACK_ATTRIBUTE
        #define portTIMER_CALLBACK_ATTRIBUTE
    #endif /* portTIMER_CALLBACK_ATTRIBUTE 结束 */

#endif /* configUSE_TIMERS 结束 */

#ifndef portHAS_NESTED_INTERRUPTS
    #if defined( portSET_INTERRUPT_MASK_FROM_ISR ) && defined( portCLEAR_INTERRUPT_MASK_FROM_ISR )
        #define portHAS_NESTED_INTERRUPTS    1
    #else
        #define portHAS_NESTED_INTERRUPTS    0
    #endif
#endif

#ifndef portSET_INTERRUPT_MASK_FROM_ISR
    #if ( portHAS_NESTED_INTERRUPTS == 1 )
        #error portSET_INTERRUPT_MASK_FROM_ISR must be defined for ports that support nested interrupts (i.e. portHAS_NESTED_INTERRUPTS is set to 1)
    #else
        #define portSET_INTERRUPT_MASK_FROM_ISR()    0
    #endif
#else
    #if ( portHAS_NESTED_INTERRUPTS == 0 )
        #error portSET_INTERRUPT_MASK_FROM_ISR must not be defined for ports that do not support nested interrupts (i.e. portHAS_NESTED_INTERRUPTS is set to 0)
    #endif
#endif

#ifndef portCLEAR_INTERRUPT_MASK_FROM_ISR
    #if ( portHAS_NESTED_INTERRUPTS == 1 )
        #error portCLEAR_INTERRUPT_MASK_FROM_ISR must be defined for ports that support nested interrupts  (i.e. portHAS_NESTED_INTERRUPTS is set to 1)
    #else
        #define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedStatusValue )    ( void ) ( uxSavedStatusValue )
    #endif
#else
    #if ( portHAS_NESTED_INTERRUPTS == 0 )
        #error portCLEAR_INTERRUPT_MASK_FROM_ISR must not be defined for ports that do not support nested interrupts (i.e. portHAS_NESTED_INTERRUPTS is set to 0)
    #endif
#endif

#ifndef portCLEAN_UP_TCB
    #define portCLEAN_UP_TCB( pxTCB )    ( void ) ( pxTCB )
#endif

#ifndef portPRE_TASK_DELETE_HOOK
    #define portPRE_TASK_DELETE_HOOK( pvTaskToDelete, pxYieldPending )
#endif

#ifndef portSETUP_TCB
    #define portSETUP_TCB( pxTCB )    ( void ) ( pxTCB )
#endif

#ifndef portTASK_SWITCH_HOOK
    #define portTASK_SWITCH_HOOK( pxTCB )    ( void ) ( pxTCB )
#endif

#ifndef configQUEUE_REGISTRY_SIZE
    #define configQUEUE_REGISTRY_SIZE    0U
#endif

#if ( configQUEUE_REGISTRY_SIZE < 1 )
    #define vQueueAddToRegistry( xQueue, pcName )
    #define vQueueUnregisterQueue( xQueue )
    #define pcQueueGetName( xQueue )
#endif

#ifndef configUSE_MINI_LIST_ITEM
    #define configUSE_MINI_LIST_ITEM    1
#endif

#ifndef portPOINTER_SIZE_TYPE
    #define portPOINTER_SIZE_TYPE    uint32_t
#endif

/* 删除所有未使用的跟踪（trace）宏。 */
#ifndef traceSTART

/*
 * 用于执行必要的初始化——例如打开一个用于写入跟踪信息的文件。
 */
    #define traceSTART()
#endif

#ifndef traceEND

/*
 * 用于关闭跟踪，例如关闭已写入跟踪信息的文件。
 */
    #define traceEND()
#endif

#ifndef traceTASK_SWITCHED_IN

/*
 * 在选定要运行的任务之后调用。pxCurrentTCB 指向所选任务的任务控制块（TCB）。
 */
    #define traceTASK_SWITCHED_IN()
#endif

#ifndef traceSTARTING_SCHEDULER

/*
 * 在所有空闲任务和定时器任务（若已启用）成功创建之后、调度器启动之前调用。
 */
    #define traceSTARTING_SCHEDULER( xIdleTaskHandles )
#endif

#ifndef traceINCREASE_TICK_COUNT

/*
 * 从无节拍空闲睡眠唤醒后、递增时钟节拍计数之前调用。
 */
    #define traceINCREASE_TICK_COUNT( x )
#endif

#ifndef traceLOW_POWER_IDLE_BEGIN
    /* 进入无节拍空闲之前立即调用。 */
    #define traceLOW_POWER_IDLE_BEGIN()
#endif

#ifndef traceLOW_POWER_IDLE_END
    /* 无节拍空闲结束后返回空闲任务时调用。 */
    #define traceLOW_POWER_IDLE_END()
#endif

#ifndef traceTASK_SWITCHED_OUT

/*
 * 在选定要运行的任务之前调用。pxCurrentTCB 指向被切换出去的任务的任务控制块（TCB）。
 */
    #define traceTASK_SWITCHED_OUT()
#endif

#ifndef traceTASK_PRIORITY_INHERIT

/*
 * 当任务试图获取一个已被较低优先级任务持有的互斥量时调用。
 * pxTCBOfMutexHolder 指向持有该互斥量的任务的控制块（TCB）。
 * uxInheritedPriority 是互斥量持有者将继承的优先级（即试图获取该互斥量的任务的优先级）。
 */
    #define traceTASK_PRIORITY_INHERIT( pxTCBOfMutexHolder, uxInheritedPriority )
#endif

#ifndef traceTASK_PRIORITY_DISINHERIT

/*
 * 当任务释放某个互斥量时调用；该互斥量的持有曾使此任务继承更高优先级任务的优先级。
 * pxTCBOfMutexHolder 指向正在释放该互斥量的任务的控制块（TCB）。
 * uxOriginalPriority 是该任务配置的（基准）优先级。
 */
    #define traceTASK_PRIORITY_DISINHERIT( pxTCBOfMutexHolder, uxOriginalPriority )
#endif

#ifndef traceBLOCKING_ON_QUEUE_RECEIVE

/*
 * 任务因无法从队列/互斥量/信号量读取而即将阻塞。
 * pxQueue 指向尝试读取的队列/互斥量/信号量。
 * pxCurrentTCB 指向尝试读取的任务的控制块（TCB）。
 */
    #define traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue )
#endif

#ifndef traceBLOCKING_ON_QUEUE_PEEK

/*
 * 任务因无法从队列/互斥量/信号量读取而即将阻塞。
 * pxQueue 指向尝试读取的队列/互斥量/信号量。
 * pxCurrentTCB 指向尝试读取的任务的控制块（TCB）。
 */
    #define traceBLOCKING_ON_QUEUE_PEEK( pxQueue )
#endif

#ifndef traceBLOCKING_ON_QUEUE_SEND

/*
 * 任务因无法向队列/互斥量/信号量写入而即将阻塞。
 * pxQueue 指向尝试写入的队列/互斥量/信号量。
 * pxCurrentTCB 指向尝试写入的任务的控制块（TCB）。
 */
    #define traceBLOCKING_ON_QUEUE_SEND( pxQueue )
#endif

#ifndef configCHECK_FOR_STACK_OVERFLOW
    #define configCHECK_FOR_STACK_OVERFLOW    0
#endif

#ifndef configRECORD_STACK_HIGH_ADDRESS
    #define configRECORD_STACK_HIGH_ADDRESS    0
#endif

#ifndef configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H
    #define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H    0
#endif

/* 以下事件宏嵌入在内核 API 调用中。 */

#ifndef traceMOVED_TASK_TO_READY_STATE
    #define traceMOVED_TASK_TO_READY_STATE( pxTCB )
#endif

#ifndef tracePOST_MOVED_TASK_TO_READY_STATE
    #define tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB )
#endif

#ifndef traceMOVED_TASK_TO_DELAYED_LIST
    #define traceMOVED_TASK_TO_DELAYED_LIST()
#endif

#ifndef traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST
    #define traceMOVED_TASK_TO_OVERFLOW_DELAYED_LIST()
#endif

#ifndef traceQUEUE_CREATE
    #define traceQUEUE_CREATE( pxNewQueue )
#endif

#ifndef traceQUEUE_CREATE_FAILED
    #define traceQUEUE_CREATE_FAILED( ucQueueType )
#endif

#ifndef traceCREATE_MUTEX
    #define traceCREATE_MUTEX( pxNewQueue )
#endif

#ifndef traceCREATE_MUTEX_FAILED
    #define traceCREATE_MUTEX_FAILED()
#endif

#ifndef traceGIVE_MUTEX_RECURSIVE
    #define traceGIVE_MUTEX_RECURSIVE( pxMutex )
#endif

#ifndef traceGIVE_MUTEX_RECURSIVE_FAILED
    #define traceGIVE_MUTEX_RECURSIVE_FAILED( pxMutex )
#endif

#ifndef traceTAKE_MUTEX_RECURSIVE
    #define traceTAKE_MUTEX_RECURSIVE( pxMutex )
#endif

#ifndef traceTAKE_MUTEX_RECURSIVE_FAILED
    #define traceTAKE_MUTEX_RECURSIVE_FAILED( pxMutex )
#endif

#ifndef traceCREATE_COUNTING_SEMAPHORE
    #define traceCREATE_COUNTING_SEMAPHORE()
#endif

#ifndef traceCREATE_COUNTING_SEMAPHORE_FAILED
    #define traceCREATE_COUNTING_SEMAPHORE_FAILED()
#endif

#ifndef traceQUEUE_SET_SEND
    #define traceQUEUE_SET_SEND    traceQUEUE_SEND
#endif

#ifndef traceQUEUE_SEND
    #define traceQUEUE_SEND( pxQueue )
#endif

#ifndef traceQUEUE_SEND_FAILED
    #define traceQUEUE_SEND_FAILED( pxQueue )
#endif

#ifndef traceQUEUE_RECEIVE
    #define traceQUEUE_RECEIVE( pxQueue )
#endif

#ifndef traceQUEUE_PEEK
    #define traceQUEUE_PEEK( pxQueue )
#endif

#ifndef traceQUEUE_PEEK_FAILED
    #define traceQUEUE_PEEK_FAILED( pxQueue )
#endif

#ifndef traceQUEUE_PEEK_FROM_ISR
    #define traceQUEUE_PEEK_FROM_ISR( pxQueue )
#endif

#ifndef traceQUEUE_RECEIVE_FAILED
    #define traceQUEUE_RECEIVE_FAILED( pxQueue )
#endif

#ifndef traceQUEUE_SEND_FROM_ISR
    #define traceQUEUE_SEND_FROM_ISR( pxQueue )
#endif

#ifndef traceQUEUE_SEND_FROM_ISR_FAILED
    #define traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue )
#endif

#ifndef traceQUEUE_RECEIVE_FROM_ISR
    #define traceQUEUE_RECEIVE_FROM_ISR( pxQueue )
#endif

#ifndef traceQUEUE_RECEIVE_FROM_ISR_FAILED
    #define traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue )
#endif

#ifndef traceQUEUE_PEEK_FROM_ISR_FAILED
    #define traceQUEUE_PEEK_FROM_ISR_FAILED( pxQueue )
#endif

#ifndef traceQUEUE_DELETE
    #define traceQUEUE_DELETE( pxQueue )
#endif

#ifndef traceTASK_CREATE
    #define traceTASK_CREATE( pxNewTCB )
#endif

#ifndef traceTASK_CREATE_FAILED
    #define traceTASK_CREATE_FAILED()
#endif

#ifndef traceTASK_DELETE
    #define traceTASK_DELETE( pxTaskToDelete )
#endif

#ifndef traceTASK_DELAY_UNTIL
    #define traceTASK_DELAY_UNTIL( x )
#endif

#ifndef traceTASK_DELAY
    #define traceTASK_DELAY()
#endif

#ifndef traceTASK_PRIORITY_SET
    #define traceTASK_PRIORITY_SET( pxTask, uxNewPriority )
#endif

#ifndef traceTASK_SUSPEND
    #define traceTASK_SUSPEND( pxTaskToSuspend )
#endif

#ifndef traceTASK_RESUME
    #define traceTASK_RESUME( pxTaskToResume )
#endif

#ifndef traceTASK_RESUME_FROM_ISR
    #define traceTASK_RESUME_FROM_ISR( pxTaskToResume )
#endif

#ifndef traceTASK_INCREMENT_TICK
    #define traceTASK_INCREMENT_TICK( xTickCount )
#endif

#ifndef traceTIMER_CREATE
    #define traceTIMER_CREATE( pxNewTimer )
#endif

#ifndef traceTIMER_CREATE_FAILED
    #define traceTIMER_CREATE_FAILED()
#endif

#ifndef traceTIMER_COMMAND_SEND
    #define traceTIMER_COMMAND_SEND( xTimer, xMessageID, xMessageValueValue, xReturn )
#endif

#ifndef traceTIMER_EXPIRED
    #define traceTIMER_EXPIRED( pxTimer )
#endif

#ifndef traceTIMER_COMMAND_RECEIVED
    #define traceTIMER_COMMAND_RECEIVED( pxTimer, xMessageID, xMessageValue )
#endif

#ifndef traceMALLOC
    #define traceMALLOC( pvAddress, uiSize )
#endif

#ifndef traceFREE
    #define traceFREE( pvAddress, uiSize )
#endif

#ifndef traceEVENT_GROUP_CREATE
    #define traceEVENT_GROUP_CREATE( xEventGroup )
#endif

#ifndef traceEVENT_GROUP_CREATE_FAILED
    #define traceEVENT_GROUP_CREATE_FAILED()
#endif

#ifndef traceEVENT_GROUP_SYNC_BLOCK
    #define traceEVENT_GROUP_SYNC_BLOCK( xEventGroup, uxBitsToSet, uxBitsToWaitFor )
#endif

#ifndef traceEVENT_GROUP_SYNC_END
    #define traceEVENT_GROUP_SYNC_END( xEventGroup, uxBitsToSet, uxBitsToWaitFor, xTimeoutOccurred )    ( void ) ( xTimeoutOccurred )
#endif

#ifndef traceEVENT_GROUP_WAIT_BITS_BLOCK
    #define traceEVENT_GROUP_WAIT_BITS_BLOCK( xEventGroup, uxBitsToWaitFor )
#endif

#ifndef traceEVENT_GROUP_WAIT_BITS_END
    #define traceEVENT_GROUP_WAIT_BITS_END( xEventGroup, uxBitsToWaitFor, xTimeoutOccurred )    ( void ) ( xTimeoutOccurred )
#endif

#ifndef traceEVENT_GROUP_CLEAR_BITS
    #define traceEVENT_GROUP_CLEAR_BITS( xEventGroup, uxBitsToClear )
#endif

#ifndef traceEVENT_GROUP_CLEAR_BITS_FROM_ISR
    #define traceEVENT_GROUP_CLEAR_BITS_FROM_ISR( xEventGroup, uxBitsToClear )
#endif

#ifndef traceEVENT_GROUP_SET_BITS
    #define traceEVENT_GROUP_SET_BITS( xEventGroup, uxBitsToSet )
#endif

#ifndef traceEVENT_GROUP_SET_BITS_FROM_ISR
    #define traceEVENT_GROUP_SET_BITS_FROM_ISR( xEventGroup, uxBitsToSet )
#endif

#ifndef traceEVENT_GROUP_DELETE
    #define traceEVENT_GROUP_DELETE( xEventGroup )
#endif

#ifndef tracePEND_FUNC_CALL
    #define tracePEND_FUNC_CALL( xFunctionToPend, pvParameter1, ulParameter2, ret )
#endif

#ifndef tracePEND_FUNC_CALL_FROM_ISR
    #define tracePEND_FUNC_CALL_FROM_ISR( xFunctionToPend, pvParameter1, ulParameter2, ret )
#endif

#ifndef traceQUEUE_REGISTRY_ADD
    #define traceQUEUE_REGISTRY_ADD( xQueue, pcQueueName )
#endif

#ifndef traceTASK_NOTIFY_TAKE_BLOCK
    #define traceTASK_NOTIFY_TAKE_BLOCK( uxIndexToWait )
#endif

#ifndef traceTASK_NOTIFY_TAKE
    #define traceTASK_NOTIFY_TAKE( uxIndexToWait )
#endif

#ifndef traceTASK_NOTIFY_WAIT_BLOCK
    #define traceTASK_NOTIFY_WAIT_BLOCK( uxIndexToWait )
#endif

#ifndef traceTASK_NOTIFY_WAIT
    #define traceTASK_NOTIFY_WAIT( uxIndexToWait )
#endif

#ifndef traceTASK_NOTIFY
    #define traceTASK_NOTIFY( uxIndexToNotify )
#endif

#ifndef traceTASK_NOTIFY_FROM_ISR
    #define traceTASK_NOTIFY_FROM_ISR( uxIndexToNotify )
#endif

#ifndef traceTASK_NOTIFY_GIVE_FROM_ISR
    #define traceTASK_NOTIFY_GIVE_FROM_ISR( uxIndexToNotify )
#endif

#ifndef traceISR_EXIT_TO_SCHEDULER
    #define traceISR_EXIT_TO_SCHEDULER()
#endif

#ifndef traceISR_EXIT
    #define traceISR_EXIT()
#endif

#ifndef traceISR_ENTER
    #define traceISR_ENTER()
#endif

#ifndef traceSTREAM_BUFFER_CREATE_FAILED
    #define traceSTREAM_BUFFER_CREATE_FAILED( xStreamBufferType )
#endif

#ifndef traceSTREAM_BUFFER_CREATE_STATIC_FAILED
    #define traceSTREAM_BUFFER_CREATE_STATIC_FAILED( xReturn, xStreamBufferType )
#endif

#ifndef traceSTREAM_BUFFER_CREATE
    #define traceSTREAM_BUFFER_CREATE( pxStreamBuffer, xStreamBufferType )
#endif

#ifndef traceSTREAM_BUFFER_DELETE
    #define traceSTREAM_BUFFER_DELETE( xStreamBuffer )
#endif

#ifndef traceSTREAM_BUFFER_RESET
    #define traceSTREAM_BUFFER_RESET( xStreamBuffer )
#endif

#ifndef traceSTREAM_BUFFER_RESET_FROM_ISR
    #define traceSTREAM_BUFFER_RESET_FROM_ISR( xStreamBuffer )
#endif

#ifndef traceBLOCKING_ON_STREAM_BUFFER_SEND
    #define traceBLOCKING_ON_STREAM_BUFFER_SEND( xStreamBuffer )
#endif

#ifndef traceSTREAM_BUFFER_SEND
    #define traceSTREAM_BUFFER_SEND( xStreamBuffer, xBytesSent )
#endif

#ifndef traceSTREAM_BUFFER_SEND_FAILED
    #define traceSTREAM_BUFFER_SEND_FAILED( xStreamBuffer )
#endif

#ifndef traceSTREAM_BUFFER_SEND_FROM_ISR
    #define traceSTREAM_BUFFER_SEND_FROM_ISR( xStreamBuffer, xBytesSent )
#endif

#ifndef traceBLOCKING_ON_STREAM_BUFFER_RECEIVE
    #define traceBLOCKING_ON_STREAM_BUFFER_RECEIVE( xStreamBuffer )
#endif

#ifndef traceSTREAM_BUFFER_RECEIVE
    #define traceSTREAM_BUFFER_RECEIVE( xStreamBuffer, xReceivedLength )
#endif

#ifndef traceSTREAM_BUFFER_RECEIVE_FAILED
    #define traceSTREAM_BUFFER_RECEIVE_FAILED( xStreamBuffer )
#endif

#ifndef traceSTREAM_BUFFER_RECEIVE_FROM_ISR
    #define traceSTREAM_BUFFER_RECEIVE_FROM_ISR( xStreamBuffer, xReceivedLength )
#endif

#ifndef traceENTER_xEventGroupCreateStatic
    #define traceENTER_xEventGroupCreateStatic( pxEventGroupBuffer )
#endif

#ifndef traceRETURN_xEventGroupCreateStatic
    #define traceRETURN_xEventGroupCreateStatic( pxEventBits )
#endif

#ifndef traceENTER_xEventGroupCreate
    #define traceENTER_xEventGroupCreate()
#endif

#ifndef traceRETURN_xEventGroupCreate
    #define traceRETURN_xEventGroupCreate( pxEventBits )
#endif

#ifndef traceENTER_xEventGroupSync
    #define traceENTER_xEventGroupSync( xEventGroup, uxBitsToSet, uxBitsToWaitFor, xTicksToWait )
#endif

#ifndef traceRETURN_xEventGroupSync
    #define traceRETURN_xEventGroupSync( uxReturn )
#endif

#ifndef traceENTER_xEventGroupWaitBits
    #define traceENTER_xEventGroupWaitBits( xEventGroup, uxBitsToWaitFor, xClearOnExit, xWaitForAllBits, xTicksToWait )
#endif

#ifndef traceRETURN_xEventGroupWaitBits
    #define traceRETURN_xEventGroupWaitBits( uxReturn )
#endif

#ifndef traceENTER_xEventGroupClearBits
    #define traceENTER_xEventGroupClearBits( xEventGroup, uxBitsToClear )
#endif

#ifndef traceRETURN_xEventGroupClearBits
    #define traceRETURN_xEventGroupClearBits( uxReturn )
#endif

#ifndef traceENTER_xEventGroupClearBitsFromISR
    #define traceENTER_xEventGroupClearBitsFromISR( xEventGroup, uxBitsToClear )
#endif

#ifndef traceRETURN_xEventGroupClearBitsFromISR
    #define traceRETURN_xEventGroupClearBitsFromISR( xReturn )
#endif

#ifndef traceENTER_xEventGroupGetBitsFromISR
    #define traceENTER_xEventGroupGetBitsFromISR( xEventGroup )
#endif

#ifndef traceRETURN_xEventGroupGetBitsFromISR
    #define traceRETURN_xEventGroupGetBitsFromISR( uxReturn )
#endif

#ifndef traceENTER_xEventGroupSetBits
    #define traceENTER_xEventGroupSetBits( xEventGroup, uxBitsToSet )
#endif

#ifndef traceRETURN_xEventGroupSetBits
    #define traceRETURN_xEventGroupSetBits( uxEventBits )
#endif

#ifndef traceENTER_vEventGroupDelete
    #define traceENTER_vEventGroupDelete( xEventGroup )
#endif

#ifndef traceRETURN_vEventGroupDelete
    #define traceRETURN_vEventGroupDelete()
#endif

#ifndef traceENTER_xEventGroupGetStaticBuffer
    #define traceENTER_xEventGroupGetStaticBuffer( xEventGroup, ppxEventGroupBuffer )
#endif

#ifndef traceRETURN_xEventGroupGetStaticBuffer
    #define traceRETURN_xEventGroupGetStaticBuffer( xReturn )
#endif

#ifndef traceENTER_vEventGroupSetBitsCallback
    #define traceENTER_vEventGroupSetBitsCallback( pvEventGroup, ulBitsToSet )
#endif

#ifndef traceRETURN_vEventGroupSetBitsCallback
    #define traceRETURN_vEventGroupSetBitsCallback()
#endif

#ifndef traceENTER_vEventGroupClearBitsCallback
    #define traceENTER_vEventGroupClearBitsCallback( pvEventGroup, ulBitsToClear )
#endif

#ifndef traceRETURN_vEventGroupClearBitsCallback
    #define traceRETURN_vEventGroupClearBitsCallback()
#endif

#ifndef traceENTER_xEventGroupSetBitsFromISR
    #define traceENTER_xEventGroupSetBitsFromISR( xEventGroup, uxBitsToSet, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xEventGroupSetBitsFromISR
    #define traceRETURN_xEventGroupSetBitsFromISR( xReturn )
#endif

#ifndef traceENTER_uxEventGroupGetNumber
    #define traceENTER_uxEventGroupGetNumber( xEventGroup )
#endif

#ifndef traceRETURN_uxEventGroupGetNumber
    #define traceRETURN_uxEventGroupGetNumber( xReturn )
#endif

#ifndef traceENTER_vEventGroupSetNumber
    #define traceENTER_vEventGroupSetNumber( xEventGroup, uxEventGroupNumber )
#endif

#ifndef traceRETURN_vEventGroupSetNumber
    #define traceRETURN_vEventGroupSetNumber()
#endif

#ifndef traceENTER_xQueueGenericReset
    #define traceENTER_xQueueGenericReset( xQueue, xNewQueue )
#endif

#ifndef traceRETURN_xQueueGenericReset
    #define traceRETURN_xQueueGenericReset( xReturn )
#endif

#ifndef traceENTER_xQueueGenericCreateStatic
    #define traceENTER_xQueueGenericCreateStatic( uxQueueLength, uxItemSize, pucQueueStorage, pxStaticQueue, ucQueueType )
#endif

#ifndef traceRETURN_xQueueGenericCreateStatic
    #define traceRETURN_xQueueGenericCreateStatic( pxNewQueue )
#endif

#ifndef traceENTER_xQueueGenericGetStaticBuffers
    #define traceENTER_xQueueGenericGetStaticBuffers( xQueue, ppucQueueStorage, ppxStaticQueue )
#endif

#ifndef traceRETURN_xQueueGenericGetStaticBuffers
    #define traceRETURN_xQueueGenericGetStaticBuffers( xReturn )
#endif

#ifndef traceENTER_xQueueGenericCreate
    #define traceENTER_xQueueGenericCreate( uxQueueLength, uxItemSize, ucQueueType )
#endif

#ifndef traceRETURN_xQueueGenericCreate
    #define traceRETURN_xQueueGenericCreate( pxNewQueue )
#endif

#ifndef traceENTER_xQueueCreateMutex
    #define traceENTER_xQueueCreateMutex( ucQueueType )
#endif

#ifndef traceRETURN_xQueueCreateMutex
    #define traceRETURN_xQueueCreateMutex( xNewQueue )
#endif

#ifndef traceENTER_xQueueCreateMutexStatic
    #define traceENTER_xQueueCreateMutexStatic( ucQueueType, pxStaticQueue )
#endif

#ifndef traceRETURN_xQueueCreateMutexStatic
    #define traceRETURN_xQueueCreateMutexStatic( xNewQueue )
#endif

#ifndef traceENTER_xQueueGetMutexHolder
    #define traceENTER_xQueueGetMutexHolder( xSemaphore )
#endif

#ifndef traceRETURN_xQueueGetMutexHolder
    #define traceRETURN_xQueueGetMutexHolder( pxReturn )
#endif

#ifndef traceENTER_xQueueGetMutexHolderFromISR
    #define traceENTER_xQueueGetMutexHolderFromISR( xSemaphore )
#endif

#ifndef traceRETURN_xQueueGetMutexHolderFromISR
    #define traceRETURN_xQueueGetMutexHolderFromISR( pxReturn )
#endif

#ifndef traceENTER_xQueueGiveMutexRecursive
    #define traceENTER_xQueueGiveMutexRecursive( xMutex )
#endif

#ifndef traceRETURN_xQueueGiveMutexRecursive
    #define traceRETURN_xQueueGiveMutexRecursive( xReturn )
#endif

#ifndef traceENTER_xQueueTakeMutexRecursive
    #define traceENTER_xQueueTakeMutexRecursive( xMutex, xTicksToWait )
#endif

#ifndef traceRETURN_xQueueTakeMutexRecursive
    #define traceRETURN_xQueueTakeMutexRecursive( xReturn )
#endif

#ifndef traceENTER_xQueueCreateCountingSemaphoreStatic
    #define traceENTER_xQueueCreateCountingSemaphoreStatic( uxMaxCount, uxInitialCount, pxStaticQueue )
#endif

#ifndef traceRETURN_xQueueCreateCountingSemaphoreStatic
    #define traceRETURN_xQueueCreateCountingSemaphoreStatic( xHandle )
#endif

#ifndef traceENTER_xQueueCreateCountingSemaphore
    #define traceENTER_xQueueCreateCountingSemaphore( uxMaxCount, uxInitialCount )
#endif

#ifndef traceRETURN_xQueueCreateCountingSemaphore
    #define traceRETURN_xQueueCreateCountingSemaphore( xHandle )
#endif

#ifndef traceENTER_xQueueGenericSend
    #define traceENTER_xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, xCopyPosition )
#endif

#ifndef traceRETURN_xQueueGenericSend
    #define traceRETURN_xQueueGenericSend( xReturn )
#endif

#ifndef traceENTER_xQueueGenericSendFromISR
    #define traceENTER_xQueueGenericSendFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken, xCopyPosition )
#endif

#ifndef traceRETURN_xQueueGenericSendFromISR
    #define traceRETURN_xQueueGenericSendFromISR( xReturn )
#endif

#ifndef traceENTER_xQueueGiveFromISR
    #define traceENTER_xQueueGiveFromISR( xQueue, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xQueueGiveFromISR
    #define traceRETURN_xQueueGiveFromISR( xReturn )
#endif

#ifndef traceENTER_xQueueReceive
    #define traceENTER_xQueueReceive( xQueue, pvBuffer, xTicksToWait )
#endif

#ifndef traceRETURN_xQueueReceive
    #define traceRETURN_xQueueReceive( xReturn )
#endif

#ifndef traceENTER_xQueueSemaphoreTake
    #define traceENTER_xQueueSemaphoreTake( xQueue, xTicksToWait )
#endif

#ifndef traceRETURN_xQueueSemaphoreTake
    #define traceRETURN_xQueueSemaphoreTake( xReturn )
#endif

#ifndef traceENTER_xQueuePeek
    #define traceENTER_xQueuePeek( xQueue, pvBuffer, xTicksToWait )
#endif

#ifndef traceRETURN_xQueuePeek
    #define traceRETURN_xQueuePeek( xReturn )
#endif

#ifndef traceENTER_xQueueReceiveFromISR
    #define traceENTER_xQueueReceiveFromISR( xQueue, pvBuffer, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xQueueReceiveFromISR
    #define traceRETURN_xQueueReceiveFromISR( xReturn )
#endif

#ifndef traceENTER_xQueuePeekFromISR
    #define traceENTER_xQueuePeekFromISR( xQueue, pvBuffer )
#endif

#ifndef traceRETURN_xQueuePeekFromISR
    #define traceRETURN_xQueuePeekFromISR( xReturn )
#endif

#ifndef traceENTER_uxQueueMessagesWaiting
    #define traceENTER_uxQueueMessagesWaiting( xQueue )
#endif

#ifndef traceRETURN_uxQueueMessagesWaiting
    #define traceRETURN_uxQueueMessagesWaiting( uxReturn )
#endif

#ifndef traceENTER_uxQueueSpacesAvailable
    #define traceENTER_uxQueueSpacesAvailable( xQueue )
#endif

#ifndef traceRETURN_uxQueueSpacesAvailable
    #define traceRETURN_uxQueueSpacesAvailable( uxReturn )
#endif

#ifndef traceENTER_uxQueueMessagesWaitingFromISR
    #define traceENTER_uxQueueMessagesWaitingFromISR( xQueue )
#endif

#ifndef traceRETURN_uxQueueMessagesWaitingFromISR
    #define traceRETURN_uxQueueMessagesWaitingFromISR( uxReturn )
#endif

#ifndef traceENTER_vQueueDelete
    #define traceENTER_vQueueDelete( xQueue )
#endif

#ifndef traceRETURN_vQueueDelete
    #define traceRETURN_vQueueDelete()
#endif

#ifndef traceENTER_uxQueueGetQueueNumber
    #define traceENTER_uxQueueGetQueueNumber( xQueue )
#endif

#ifndef traceRETURN_uxQueueGetQueueNumber
    #define traceRETURN_uxQueueGetQueueNumber( uxQueueNumber )
#endif

#ifndef traceENTER_vQueueSetQueueNumber
    #define traceENTER_vQueueSetQueueNumber( xQueue, uxQueueNumber )
#endif

#ifndef traceRETURN_vQueueSetQueueNumber
    #define traceRETURN_vQueueSetQueueNumber()
#endif

#ifndef traceENTER_ucQueueGetQueueType
    #define traceENTER_ucQueueGetQueueType( xQueue )
#endif

#ifndef traceRETURN_ucQueueGetQueueType
    #define traceRETURN_ucQueueGetQueueType( ucQueueType )
#endif

#ifndef traceENTER_uxQueueGetQueueItemSize
    #define traceENTER_uxQueueGetQueueItemSize( xQueue )
#endif

#ifndef traceRETURN_uxQueueGetQueueItemSize
    #define traceRETURN_uxQueueGetQueueItemSize( uxItemSize )
#endif

#ifndef traceENTER_uxQueueGetQueueLength
    #define traceENTER_uxQueueGetQueueLength( xQueue )
#endif

#ifndef traceRETURN_uxQueueGetQueueLength
    #define traceRETURN_uxQueueGetQueueLength( uxLength )
#endif

#ifndef traceENTER_xQueueIsQueueEmptyFromISR
    #define traceENTER_xQueueIsQueueEmptyFromISR( xQueue )
#endif

#ifndef traceRETURN_xQueueIsQueueEmptyFromISR
    #define traceRETURN_xQueueIsQueueEmptyFromISR( xReturn )
#endif

#ifndef traceENTER_xQueueIsQueueFullFromISR
    #define traceENTER_xQueueIsQueueFullFromISR( xQueue )
#endif

#ifndef traceRETURN_xQueueIsQueueFullFromISR
    #define traceRETURN_xQueueIsQueueFullFromISR( xReturn )
#endif

#ifndef traceENTER_xQueueCRSend
    #define traceENTER_xQueueCRSend( xQueue, pvItemToQueue, xTicksToWait )
#endif

#ifndef traceRETURN_xQueueCRSend
    #define traceRETURN_xQueueCRSend( xReturn )
#endif

#ifndef traceENTER_xQueueCRReceive
    #define traceENTER_xQueueCRReceive( xQueue, pvBuffer, xTicksToWait )
#endif

#ifndef traceRETURN_xQueueCRReceive
    #define traceRETURN_xQueueCRReceive( xReturn )
#endif

#ifndef traceENTER_xQueueCRSendFromISR
    #define traceENTER_xQueueCRSendFromISR( xQueue, pvItemToQueue, xCoRoutinePreviouslyWoken )
#endif

#ifndef traceRETURN_xQueueCRSendFromISR
    #define traceRETURN_xQueueCRSendFromISR( xCoRoutinePreviouslyWoken )
#endif

#ifndef traceENTER_xQueueCRReceiveFromISR
    #define traceENTER_xQueueCRReceiveFromISR( xQueue, pvBuffer, pxCoRoutineWoken )
#endif

#ifndef traceRETURN_xQueueCRReceiveFromISR
    #define traceRETURN_xQueueCRReceiveFromISR( xReturn )
#endif

#ifndef traceENTER_vQueueAddToRegistry
    #define traceENTER_vQueueAddToRegistry( xQueue, pcQueueName )
#endif

#ifndef traceRETURN_vQueueAddToRegistry
    #define traceRETURN_vQueueAddToRegistry()
#endif

#ifndef traceENTER_pcQueueGetName
    #define traceENTER_pcQueueGetName( xQueue )
#endif

#ifndef traceRETURN_pcQueueGetName
    #define traceRETURN_pcQueueGetName( pcReturn )
#endif

#ifndef traceENTER_vQueueUnregisterQueue
    #define traceENTER_vQueueUnregisterQueue( xQueue )
#endif

#ifndef traceRETURN_vQueueUnregisterQueue
    #define traceRETURN_vQueueUnregisterQueue()
#endif

#ifndef traceENTER_vQueueWaitForMessageRestricted
    #define traceENTER_vQueueWaitForMessageRestricted( xQueue, xTicksToWait, xWaitIndefinitely )
#endif

#ifndef traceRETURN_vQueueWaitForMessageRestricted
    #define traceRETURN_vQueueWaitForMessageRestricted()
#endif

#ifndef traceENTER_xQueueCreateSet
    #define traceENTER_xQueueCreateSet( uxEventQueueLength )
#endif

#ifndef traceRETURN_xQueueCreateSet
    #define traceRETURN_xQueueCreateSet( pxQueue )
#endif

#ifndef traceENTER_xQueueCreateSetStatic
    #define traceENTER_xQueueCreateSetStatic( uxEventQueueLength )
#endif

#ifndef traceRETURN_xQueueCreateSetStatic
    #define traceRETURN_xQueueCreateSetStatic( pxQueue )
#endif

#ifndef traceENTER_xQueueAddToSet
    #define traceENTER_xQueueAddToSet( xQueueOrSemaphore, xQueueSet )
#endif

#ifndef traceRETURN_xQueueAddToSet
    #define traceRETURN_xQueueAddToSet( xReturn )
#endif

#ifndef traceENTER_xQueueRemoveFromSet
    #define traceENTER_xQueueRemoveFromSet( xQueueOrSemaphore, xQueueSet )
#endif

#ifndef traceRETURN_xQueueRemoveFromSet
    #define traceRETURN_xQueueRemoveFromSet( xReturn )
#endif

#ifndef traceENTER_xQueueSelectFromSet
    #define traceENTER_xQueueSelectFromSet( xQueueSet, xTicksToWait )
#endif

#ifndef traceRETURN_xQueueSelectFromSet
    #define traceRETURN_xQueueSelectFromSet( xReturn )
#endif

#ifndef traceENTER_xQueueSelectFromSetFromISR
    #define traceENTER_xQueueSelectFromSetFromISR( xQueueSet )
#endif

#ifndef traceRETURN_xQueueSelectFromSetFromISR
    #define traceRETURN_xQueueSelectFromSetFromISR( xReturn )
#endif

#ifndef traceENTER_xTimerCreateTimerTask
    #define traceENTER_xTimerCreateTimerTask()
#endif

#ifndef traceRETURN_xTimerCreateTimerTask
    #define traceRETURN_xTimerCreateTimerTask( xReturn )
#endif

#ifndef traceENTER_xTimerCreate
    #define traceENTER_xTimerCreate( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction )
#endif

#ifndef traceRETURN_xTimerCreate
    #define traceRETURN_xTimerCreate( pxNewTimer )
#endif

#ifndef traceENTER_xTimerCreateStatic
    #define traceENTER_xTimerCreateStatic( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction, pxTimerBuffer )
#endif

#ifndef traceRETURN_xTimerCreateStatic
    #define traceRETURN_xTimerCreateStatic( pxNewTimer )
#endif

#ifndef traceENTER_xTimerGenericCommandFromTask
    #define traceENTER_xTimerGenericCommandFromTask( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait )
#endif

#ifndef traceRETURN_xTimerGenericCommandFromTask
    #define traceRETURN_xTimerGenericCommandFromTask( xReturn )
#endif

#ifndef traceENTER_xTimerGenericCommandFromISR
    #define traceENTER_xTimerGenericCommandFromISR( xTimer, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait )
#endif

#ifndef traceRETURN_xTimerGenericCommandFromISR
    #define traceRETURN_xTimerGenericCommandFromISR( xReturn )
#endif

#ifndef traceENTER_xTimerGetTimerDaemonTaskHandle
    #define traceENTER_xTimerGetTimerDaemonTaskHandle()
#endif

#ifndef traceRETURN_xTimerGetTimerDaemonTaskHandle
    #define traceRETURN_xTimerGetTimerDaemonTaskHandle( xTimerTaskHandle )
#endif

#ifndef traceENTER_xTimerGetPeriod
    #define traceENTER_xTimerGetPeriod( xTimer )
#endif

#ifndef traceRETURN_xTimerGetPeriod
    #define traceRETURN_xTimerGetPeriod( xTimerPeriodInTicks )
#endif

#ifndef traceENTER_vTimerSetReloadMode
    #define traceENTER_vTimerSetReloadMode( xTimer, xAutoReload )
#endif

#ifndef traceRETURN_vTimerSetReloadMode
    #define traceRETURN_vTimerSetReloadMode()
#endif

#ifndef traceENTER_xTimerGetReloadMode
    #define traceENTER_xTimerGetReloadMode( xTimer )
#endif

#ifndef traceRETURN_xTimerGetReloadMode
    #define traceRETURN_xTimerGetReloadMode( xReturn )
#endif

#ifndef traceENTER_uxTimerGetReloadMode
    #define traceENTER_uxTimerGetReloadMode( xTimer )
#endif

#ifndef traceRETURN_uxTimerGetReloadMode
    #define traceRETURN_uxTimerGetReloadMode( uxReturn )
#endif

#ifndef traceENTER_xTimerGetExpiryTime
    #define traceENTER_xTimerGetExpiryTime( xTimer )
#endif

#ifndef traceRETURN_xTimerGetExpiryTime
    #define traceRETURN_xTimerGetExpiryTime( xReturn )
#endif

#ifndef traceENTER_xTimerGetStaticBuffer
    #define traceENTER_xTimerGetStaticBuffer( xTimer, ppxTimerBuffer )
#endif

#ifndef traceRETURN_xTimerGetStaticBuffer
    #define traceRETURN_xTimerGetStaticBuffer( xReturn )
#endif

#ifndef traceENTER_pcTimerGetName
    #define traceENTER_pcTimerGetName( xTimer )
#endif

#ifndef traceRETURN_pcTimerGetName
    #define traceRETURN_pcTimerGetName( pcTimerName )
#endif

#ifndef traceENTER_xTimerIsTimerActive
    #define traceENTER_xTimerIsTimerActive( xTimer )
#endif

#ifndef traceRETURN_xTimerIsTimerActive
    #define traceRETURN_xTimerIsTimerActive( xReturn )
#endif

#ifndef traceENTER_pvTimerGetTimerID
    #define traceENTER_pvTimerGetTimerID( xTimer )
#endif

#ifndef traceRETURN_pvTimerGetTimerID
    #define traceRETURN_pvTimerGetTimerID( pvReturn )
#endif

#ifndef traceENTER_vTimerSetTimerID
    #define traceENTER_vTimerSetTimerID( xTimer, pvNewID )
#endif

#ifndef traceRETURN_vTimerSetTimerID
    #define traceRETURN_vTimerSetTimerID()
#endif

#ifndef traceENTER_xTimerPendFunctionCallFromISR
    #define traceENTER_xTimerPendFunctionCallFromISR( xFunctionToPend, pvParameter1, ulParameter2, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xTimerPendFunctionCallFromISR
    #define traceRETURN_xTimerPendFunctionCallFromISR( xReturn )
#endif

#ifndef traceENTER_xTimerPendFunctionCall
    #define traceENTER_xTimerPendFunctionCall( xFunctionToPend, pvParameter1, ulParameter2, xTicksToWait )
#endif

#ifndef traceRETURN_xTimerPendFunctionCall
    #define traceRETURN_xTimerPendFunctionCall( xReturn )
#endif

#ifndef traceENTER_uxTimerGetTimerNumber
    #define traceENTER_uxTimerGetTimerNumber( xTimer )
#endif

#ifndef traceRETURN_uxTimerGetTimerNumber
    #define traceRETURN_uxTimerGetTimerNumber( uxTimerNumber )
#endif

#ifndef traceENTER_vTimerSetTimerNumber
    #define traceENTER_vTimerSetTimerNumber( xTimer, uxTimerNumber )
#endif

#ifndef traceRETURN_vTimerSetTimerNumber
    #define traceRETURN_vTimerSetTimerNumber()
#endif

#ifndef traceENTER_xTaskCreateStatic
    #define traceENTER_xTaskCreateStatic( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer )
#endif

#ifndef traceRETURN_xTaskCreateStatic
    #define traceRETURN_xTaskCreateStatic( xReturn )
#endif

#ifndef traceENTER_xTaskCreateStaticAffinitySet
    #define traceENTER_xTaskCreateStaticAffinitySet( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, uxCoreAffinityMask )
#endif

#ifndef traceRETURN_xTaskCreateStaticAffinitySet
    #define traceRETURN_xTaskCreateStaticAffinitySet( xReturn )
#endif

#ifndef traceENTER_xTaskCreateRestrictedStatic
    #define traceENTER_xTaskCreateRestrictedStatic( pxTaskDefinition, pxCreatedTask )
#endif

#ifndef traceRETURN_xTaskCreateRestrictedStatic
    #define traceRETURN_xTaskCreateRestrictedStatic( xReturn )
#endif

#ifndef traceENTER_xTaskCreateRestrictedStaticAffinitySet
    #define traceENTER_xTaskCreateRestrictedStaticAffinitySet( pxTaskDefinition, uxCoreAffinityMask, pxCreatedTask )
#endif

#ifndef traceRETURN_xTaskCreateRestrictedStaticAffinitySet
    #define traceRETURN_xTaskCreateRestrictedStaticAffinitySet( xReturn )
#endif

#ifndef traceENTER_xTaskCreateRestricted
    #define traceENTER_xTaskCreateRestricted( pxTaskDefinition, pxCreatedTask )
#endif

#ifndef traceRETURN_xTaskCreateRestricted
    #define traceRETURN_xTaskCreateRestricted( xReturn )
#endif

#ifndef traceENTER_xTaskCreateRestrictedAffinitySet
    #define traceENTER_xTaskCreateRestrictedAffinitySet( pxTaskDefinition, uxCoreAffinityMask, pxCreatedTask )
#endif

#ifndef traceRETURN_xTaskCreateRestrictedAffinitySet
    #define traceRETURN_xTaskCreateRestrictedAffinitySet( xReturn )
#endif

#ifndef traceENTER_xTaskCreate
    #define traceENTER_xTaskCreate( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask )
#endif

#ifndef traceRETURN_xTaskCreate
    #define traceRETURN_xTaskCreate( xReturn )
#endif

#ifndef traceENTER_xTaskCreateAffinitySet
    #define traceENTER_xTaskCreateAffinitySet( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, uxCoreAffinityMask, pxCreatedTask )
#endif

#ifndef traceRETURN_xTaskCreateAffinitySet
    #define traceRETURN_xTaskCreateAffinitySet( xReturn )
#endif

#ifndef traceENTER_vTaskDelete
    #define traceENTER_vTaskDelete( xTaskToDelete )
#endif

#ifndef traceRETURN_vTaskDelete
    #define traceRETURN_vTaskDelete()
#endif

#ifndef traceENTER_xTaskPeriodicDelay
    #define traceENTER_xTaskPeriodicDelay( pxPreviousWakeTime, xTimeIncrement )
#endif

#ifndef traceRETURN_xTaskPeriodicDelay
    #define traceRETURN_xTaskPeriodicDelay( xIncrements )
#endif

#ifndef traceENTER_xTaskDelayUntil
    #define traceENTER_xTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement )
#endif

#ifndef traceRETURN_xTaskDelayUntil
    #define traceRETURN_xTaskDelayUntil( xShouldDelay )
#endif

#ifndef traceENTER_vTaskDelay
    #define traceENTER_vTaskDelay( xTicksToDelay )
#endif

#ifndef traceRETURN_vTaskDelay
    #define traceRETURN_vTaskDelay()
#endif

#ifndef traceENTER_eTaskGetState
    #define traceENTER_eTaskGetState( xTask )
#endif

#ifndef traceRETURN_eTaskGetState
    #define traceRETURN_eTaskGetState( eReturn )
#endif

#ifndef traceENTER_uxTaskPriorityGet
    #define traceENTER_uxTaskPriorityGet( xTask )
#endif

#ifndef traceRETURN_uxTaskPriorityGet
    #define traceRETURN_uxTaskPriorityGet( uxReturn )
#endif

#ifndef traceENTER_uxTaskPriorityGetFromISR
    #define traceENTER_uxTaskPriorityGetFromISR( xTask )
#endif

#ifndef traceRETURN_uxTaskPriorityGetFromISR
    #define traceRETURN_uxTaskPriorityGetFromISR( uxReturn )
#endif

#ifndef traceENTER_uxTaskBasePriorityGet
    #define traceENTER_uxTaskBasePriorityGet( xTask )
#endif

#ifndef traceRETURN_uxTaskBasePriorityGet
    #define traceRETURN_uxTaskBasePriorityGet( uxReturn )
#endif

#ifndef traceENTER_uxTaskBasePriorityGetFromISR
    #define traceENTER_uxTaskBasePriorityGetFromISR( xTask )
#endif

#ifndef traceRETURN_uxTaskBasePriorityGetFromISR
    #define traceRETURN_uxTaskBasePriorityGetFromISR( uxReturn )
#endif

#ifndef traceENTER_vTaskPrioritySet
    #define traceENTER_vTaskPrioritySet( xTask, uxNewPriority )
#endif

#ifndef traceRETURN_vTaskPrioritySet
    #define traceRETURN_vTaskPrioritySet()
#endif

#ifndef traceENTER_vTaskCoreAffinitySet
    #define traceENTER_vTaskCoreAffinitySet( xTask, uxCoreAffinityMask )
#endif

#ifndef traceRETURN_vTaskCoreAffinitySet
    #define traceRETURN_vTaskCoreAffinitySet()
#endif

#ifndef traceENTER_vTaskCoreAffinityGet
    #define traceENTER_vTaskCoreAffinityGet( xTask )
#endif

#ifndef traceRETURN_vTaskCoreAffinityGet
    #define traceRETURN_vTaskCoreAffinityGet( uxCoreAffinityMask )
#endif

#ifndef traceENTER_vTaskPreemptionDisable
    #define traceENTER_vTaskPreemptionDisable( xTask )
#endif

#ifndef traceRETURN_vTaskPreemptionDisable
    #define traceRETURN_vTaskPreemptionDisable()
#endif

#ifndef traceENTER_vTaskPreemptionEnable
    #define traceENTER_vTaskPreemptionEnable( xTask )
#endif

#ifndef traceRETURN_vTaskPreemptionEnable
    #define traceRETURN_vTaskPreemptionEnable()
#endif

#ifndef traceENTER_vTaskSuspend
    #define traceENTER_vTaskSuspend( xTaskToSuspend )
#endif

#ifndef traceRETURN_vTaskSuspend
    #define traceRETURN_vTaskSuspend()
#endif

#ifndef traceENTER_vTaskResume
    #define traceENTER_vTaskResume( xTaskToResume )
#endif

#ifndef traceRETURN_vTaskResume
    #define traceRETURN_vTaskResume()
#endif

#ifndef traceENTER_xTaskResumeFromISR
    #define traceENTER_xTaskResumeFromISR( xTaskToResume )
#endif

#ifndef traceRETURN_xTaskResumeFromISR
    #define traceRETURN_xTaskResumeFromISR( xYieldRequired )
#endif

#ifndef traceENTER_vTaskStartScheduler
    #define traceENTER_vTaskStartScheduler()
#endif

#ifndef traceRETURN_vTaskStartScheduler
    #define traceRETURN_vTaskStartScheduler()
#endif

#ifndef traceENTER_vTaskEndScheduler
    #define traceENTER_vTaskEndScheduler()
#endif

#ifndef traceRETURN_vTaskEndScheduler
    #define traceRETURN_vTaskEndScheduler()
#endif

#ifndef traceENTER_vTaskSuspendAll
    #define traceENTER_vTaskSuspendAll()
#endif

#ifndef traceRETURN_vTaskSuspendAll
    #define traceRETURN_vTaskSuspendAll()
#endif

#ifndef traceENTER_xTaskResumeAll
    #define traceENTER_xTaskResumeAll()
#endif

#ifndef traceRETURN_xTaskResumeAll
    #define traceRETURN_xTaskResumeAll( xAlreadyYielded )
#endif

#ifndef traceENTER_xTaskGetTickCount
    #define traceENTER_xTaskGetTickCount()
#endif

#ifndef traceRETURN_xTaskGetTickCount
    #define traceRETURN_xTaskGetTickCount( xTicks )
#endif

#ifndef traceENTER_xTaskGetTickCountFromISR
    #define traceENTER_xTaskGetTickCountFromISR()
#endif

#ifndef traceRETURN_xTaskGetTickCountFromISR
    #define traceRETURN_xTaskGetTickCountFromISR( xReturn )
#endif

#ifndef traceENTER_uxTaskGetNumberOfTasks
    #define traceENTER_uxTaskGetNumberOfTasks()
#endif

#ifndef traceRETURN_uxTaskGetNumberOfTasks
    #define traceRETURN_uxTaskGetNumberOfTasks( uxCurrentNumberOfTasks )
#endif

#ifndef traceENTER_pcTaskGetName
    #define traceENTER_pcTaskGetName( xTaskToQuery )
#endif

#ifndef traceRETURN_pcTaskGetName
    #define traceRETURN_pcTaskGetName( pcTaskName )
#endif

#ifndef traceENTER_xTaskGetHandle
    #define traceENTER_xTaskGetHandle( pcNameToQuery )
#endif

#ifndef traceRETURN_xTaskGetHandle
    #define traceRETURN_xTaskGetHandle( pxTCB )
#endif

#ifndef traceENTER_xTaskGetStaticBuffers
    #define traceENTER_xTaskGetStaticBuffers( xTask, ppuxStackBuffer, ppxTaskBuffer )
#endif

#ifndef traceRETURN_xTaskGetStaticBuffers
    #define traceRETURN_xTaskGetStaticBuffers( xReturn )
#endif

#ifndef traceENTER_uxTaskGetSystemState
    #define traceENTER_uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, pulTotalRunTime )
#endif

#ifndef traceRETURN_uxTaskGetSystemState
    #define traceRETURN_uxTaskGetSystemState( uxTask )
#endif

#if ( configNUMBER_OF_CORES == 1 )
    #ifndef traceENTER_xTaskGetIdleTaskHandle
        #define traceENTER_xTaskGetIdleTaskHandle()
    #endif
#endif

#if ( configNUMBER_OF_CORES == 1 )
    #ifndef traceRETURN_xTaskGetIdleTaskHandle
        #define traceRETURN_xTaskGetIdleTaskHandle( xIdleTaskHandle )
    #endif
#endif

#ifndef traceENTER_xTaskGetIdleTaskHandleForCore
    #define traceENTER_xTaskGetIdleTaskHandleForCore( xCoreID )
#endif

#ifndef traceRETURN_xTaskGetIdleTaskHandleForCore
    #define traceRETURN_xTaskGetIdleTaskHandleForCore( xIdleTaskHandle )
#endif

#ifndef traceENTER_vTaskStepTick
    #define traceENTER_vTaskStepTick( xTicksToJump )
#endif

#ifndef traceRETURN_vTaskStepTick
    #define traceRETURN_vTaskStepTick()
#endif

#ifndef traceENTER_xTaskCatchUpTicks
    #define traceENTER_xTaskCatchUpTicks( xTicksToCatchUp )
#endif

#ifndef traceRETURN_xTaskCatchUpTicks
    #define traceRETURN_xTaskCatchUpTicks( xYieldOccurred )
#endif

#ifndef traceENTER_xTaskAbortDelay
    #define traceENTER_xTaskAbortDelay( xTask )
#endif

#ifndef traceRETURN_xTaskAbortDelay
    #define traceRETURN_xTaskAbortDelay( xReturn )
#endif

#ifndef traceENTER_xTaskIncrementTick
    #define traceENTER_xTaskIncrementTick()
#endif

#ifndef traceRETURN_xTaskIncrementTick
    #define traceRETURN_xTaskIncrementTick( xSwitchRequired )
#endif

#ifndef traceENTER_vTaskSetApplicationTaskTag
    #define traceENTER_vTaskSetApplicationTaskTag( xTask, pxHookFunction )
#endif

#ifndef traceRETURN_vTaskSetApplicationTaskTag
    #define traceRETURN_vTaskSetApplicationTaskTag()
#endif

#ifndef traceENTER_xTaskGetApplicationTaskTag
    #define traceENTER_xTaskGetApplicationTaskTag( xTask )
#endif

#ifndef traceRETURN_xTaskGetApplicationTaskTag
    #define traceRETURN_xTaskGetApplicationTaskTag( xReturn )
#endif

#ifndef traceENTER_xTaskGetApplicationTaskTagFromISR
    #define traceENTER_xTaskGetApplicationTaskTagFromISR( xTask )
#endif

#ifndef traceRETURN_xTaskGetApplicationTaskTagFromISR
    #define traceRETURN_xTaskGetApplicationTaskTagFromISR( xReturn )
#endif

#ifndef traceENTER_xTaskCallApplicationTaskHook
    #define traceENTER_xTaskCallApplicationTaskHook( xTask, pvParameter )
#endif

#ifndef traceRETURN_xTaskCallApplicationTaskHook
    #define traceRETURN_xTaskCallApplicationTaskHook( xReturn )
#endif

#ifndef traceENTER_vTaskSwitchContext
    #define traceENTER_vTaskSwitchContext()
#endif

#ifndef traceRETURN_vTaskSwitchContext
    #define traceRETURN_vTaskSwitchContext()
#endif

#ifndef traceENTER_vTaskPlaceOnEventList
    #define traceENTER_vTaskPlaceOnEventList( pxEventList, xTicksToWait )
#endif

#ifndef traceRETURN_vTaskPlaceOnEventList
    #define traceRETURN_vTaskPlaceOnEventList()
#endif

#ifndef traceENTER_vTaskPlaceOnUnorderedEventList
    #define traceENTER_vTaskPlaceOnUnorderedEventList( pxEventList, xItemValue, xTicksToWait )
#endif

#ifndef traceRETURN_vTaskPlaceOnUnorderedEventList
    #define traceRETURN_vTaskPlaceOnUnorderedEventList()
#endif

#ifndef traceENTER_vTaskPlaceOnEventListRestricted
    #define traceENTER_vTaskPlaceOnEventListRestricted( pxEventList, xTicksToWait, xWaitIndefinitely )
#endif

#ifndef traceRETURN_vTaskPlaceOnEventListRestricted
    #define traceRETURN_vTaskPlaceOnEventListRestricted()
#endif

#ifndef traceENTER_xTaskRemoveFromEventList
    #define traceENTER_xTaskRemoveFromEventList( pxEventList )
#endif

#ifndef traceRETURN_xTaskRemoveFromEventList
    #define traceRETURN_xTaskRemoveFromEventList( xReturn )
#endif

#ifndef traceENTER_vTaskRemoveFromUnorderedEventList
    #define traceENTER_vTaskRemoveFromUnorderedEventList( pxEventListItem, xItemValue )
#endif

#ifndef traceRETURN_vTaskRemoveFromUnorderedEventList
    #define traceRETURN_vTaskRemoveFromUnorderedEventList()
#endif

#ifndef traceENTER_vTaskSetTimeOutState
    #define traceENTER_vTaskSetTimeOutState( pxTimeOut )
#endif

#ifndef traceRETURN_vTaskSetTimeOutState
    #define traceRETURN_vTaskSetTimeOutState()
#endif

#ifndef traceENTER_vTaskInternalSetTimeOutState
    #define traceENTER_vTaskInternalSetTimeOutState( pxTimeOut )
#endif

#ifndef traceRETURN_vTaskInternalSetTimeOutState
    #define traceRETURN_vTaskInternalSetTimeOutState()
#endif

#ifndef traceENTER_xTaskCheckForTimeOut
    #define traceENTER_xTaskCheckForTimeOut( pxTimeOut, pxTicksToWait )
#endif

#ifndef traceRETURN_xTaskCheckForTimeOut
    #define traceRETURN_xTaskCheckForTimeOut( xReturn )
#endif

#ifndef traceENTER_vTaskMissedYield
    #define traceENTER_vTaskMissedYield()
#endif

#ifndef traceRETURN_vTaskMissedYield
    #define traceRETURN_vTaskMissedYield()
#endif

#ifndef traceENTER_uxTaskGetTaskNumber
    #define traceENTER_uxTaskGetTaskNumber( xTask )
#endif

#ifndef traceRETURN_uxTaskGetTaskNumber
    #define traceRETURN_uxTaskGetTaskNumber( uxReturn )
#endif

#ifndef traceENTER_vTaskSetTaskNumber
    #define traceENTER_vTaskSetTaskNumber( xTask, uxHandle )
#endif

#ifndef traceRETURN_vTaskSetTaskNumber
    #define traceRETURN_vTaskSetTaskNumber()
#endif

#ifndef traceENTER_eTaskConfirmSleepModeStatus
    #define traceENTER_eTaskConfirmSleepModeStatus()
#endif

#ifndef traceRETURN_eTaskConfirmSleepModeStatus
    #define traceRETURN_eTaskConfirmSleepModeStatus( eReturn )
#endif

#ifndef traceENTER_vTaskSetThreadLocalStoragePointer
    #define traceENTER_vTaskSetThreadLocalStoragePointer( xTaskToSet, xIndex, pvValue )
#endif

#ifndef traceRETURN_vTaskSetThreadLocalStoragePointer
    #define traceRETURN_vTaskSetThreadLocalStoragePointer()
#endif

#ifndef traceENTER_pvTaskGetThreadLocalStoragePointer
    #define traceENTER_pvTaskGetThreadLocalStoragePointer( xTaskToQuery, xIndex )
#endif

#ifndef traceRETURN_pvTaskGetThreadLocalStoragePointer
    #define traceRETURN_pvTaskGetThreadLocalStoragePointer( pvReturn )
#endif

#ifndef traceENTER_vTaskAllocateMPURegions
    #define traceENTER_vTaskAllocateMPURegions( xTaskToModify, pxRegions )
#endif

#ifndef traceRETURN_vTaskAllocateMPURegions
    #define traceRETURN_vTaskAllocateMPURegions()
#endif

#ifndef traceENTER_vTaskGetInfo
    #define traceENTER_vTaskGetInfo( xTask, pxTaskStatus, xGetFreeStackSpace, eState )
#endif

#ifndef traceRETURN_vTaskGetInfo
    #define traceRETURN_vTaskGetInfo()
#endif

#ifndef traceENTER_uxTaskGetStackHighWaterMark2
    #define traceENTER_uxTaskGetStackHighWaterMark2( xTask )
#endif

#ifndef traceRETURN_uxTaskGetStackHighWaterMark2
    #define traceRETURN_uxTaskGetStackHighWaterMark2( uxReturn )
#endif

#ifndef traceENTER_uxTaskGetStackHighWaterMark
    #define traceENTER_uxTaskGetStackHighWaterMark( xTask )
#endif

#ifndef traceRETURN_uxTaskGetStackHighWaterMark
    #define traceRETURN_uxTaskGetStackHighWaterMark( uxReturn )
#endif

#ifndef traceENTER_xTaskGetCurrentTaskHandle
    #define traceENTER_xTaskGetCurrentTaskHandle()
#endif

#ifndef traceRETURN_xTaskGetCurrentTaskHandle
    #define traceRETURN_xTaskGetCurrentTaskHandle( xReturn )
#endif

#ifndef traceENTER_xTaskGetCurrentTaskHandleForCore
    #define traceENTER_xTaskGetCurrentTaskHandleForCore( xCoreID )
#endif

#ifndef traceRETURN_xTaskGetCurrentTaskHandleForCore
    #define traceRETURN_xTaskGetCurrentTaskHandleForCore( xReturn )
#endif

#ifndef traceENTER_xTaskGetSchedulerState
    #define traceENTER_xTaskGetSchedulerState()
#endif

#ifndef traceRETURN_xTaskGetSchedulerState
    #define traceRETURN_xTaskGetSchedulerState( xReturn )
#endif

#ifndef traceENTER_xTaskPriorityInherit
    #define traceENTER_xTaskPriorityInherit( pxMutexHolder )
#endif

#ifndef traceRETURN_xTaskPriorityInherit
    #define traceRETURN_xTaskPriorityInherit( xReturn )
#endif

#ifndef traceENTER_xTaskPriorityDisinherit
    #define traceENTER_xTaskPriorityDisinherit( pxMutexHolder )
#endif

#ifndef traceRETURN_xTaskPriorityDisinherit
    #define traceRETURN_xTaskPriorityDisinherit( xReturn )
#endif

#ifndef traceENTER_vTaskPriorityDisinheritAfterTimeout
    #define traceENTER_vTaskPriorityDisinheritAfterTimeout( pxMutexHolder, uxHighestPriorityWaitingTask )
#endif

#ifndef traceRETURN_vTaskPriorityDisinheritAfterTimeout
    #define traceRETURN_vTaskPriorityDisinheritAfterTimeout()
#endif

#ifndef traceENTER_vTaskYieldWithinAPI
    #define traceENTER_vTaskYieldWithinAPI()
#endif

#ifndef traceRETURN_vTaskYieldWithinAPI
    #define traceRETURN_vTaskYieldWithinAPI()
#endif

#ifndef traceENTER_vTaskEnterCritical
    #define traceENTER_vTaskEnterCritical()
#endif

#ifndef traceRETURN_vTaskEnterCritical
    #define traceRETURN_vTaskEnterCritical()
#endif

#ifndef traceENTER_vTaskEnterCriticalFromISR
    #define traceENTER_vTaskEnterCriticalFromISR()
#endif

#ifndef traceRETURN_vTaskEnterCriticalFromISR
    #define traceRETURN_vTaskEnterCriticalFromISR( uxSavedInterruptStatus )
#endif

#ifndef traceENTER_vTaskExitCritical
    #define traceENTER_vTaskExitCritical()
#endif

#ifndef traceRETURN_vTaskExitCritical
    #define traceRETURN_vTaskExitCritical()
#endif

#ifndef traceENTER_vTaskExitCriticalFromISR
    #define traceENTER_vTaskExitCriticalFromISR( uxSavedInterruptStatus )
#endif

#ifndef traceRETURN_vTaskExitCriticalFromISR
    #define traceRETURN_vTaskExitCriticalFromISR()
#endif

#ifndef traceENTER_vTaskListTasks
    #define traceENTER_vTaskListTasks( pcWriteBuffer, uxBufferLength )
#endif

#ifndef traceRETURN_vTaskListTasks
    #define traceRETURN_vTaskListTasks()
#endif

#ifndef traceENTER_vTaskGetRunTimeStatistics
    #define traceENTER_vTaskGetRunTimeStatistics( pcWriteBuffer, uxBufferLength )
#endif

#ifndef traceRETURN_vTaskGetRunTimeStatistics
    #define traceRETURN_vTaskGetRunTimeStatistics()
#endif

#ifndef traceENTER_uxTaskResetEventItemValue
    #define traceENTER_uxTaskResetEventItemValue()
#endif

#ifndef traceRETURN_uxTaskResetEventItemValue
    #define traceRETURN_uxTaskResetEventItemValue( uxReturn )
#endif

#ifndef traceENTER_pvTaskIncrementMutexHeldCount
    #define traceENTER_pvTaskIncrementMutexHeldCount()
#endif

#ifndef traceRETURN_pvTaskIncrementMutexHeldCount
    #define traceRETURN_pvTaskIncrementMutexHeldCount( pxTCB )
#endif

#ifndef traceENTER_ulTaskGenericNotifyTake
    #define traceENTER_ulTaskGenericNotifyTake( uxIndexToWaitOn, xClearCountOnExit, xTicksToWait )
#endif

#ifndef traceRETURN_ulTaskGenericNotifyTake
    #define traceRETURN_ulTaskGenericNotifyTake( ulReturn )
#endif

#ifndef traceENTER_xTaskGenericNotifyWait
    #define traceENTER_xTaskGenericNotifyWait( uxIndexToWaitOn, ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait )
#endif

#ifndef traceRETURN_xTaskGenericNotifyWait
    #define traceRETURN_xTaskGenericNotifyWait( xReturn )
#endif

#ifndef traceENTER_xTaskGenericNotify
    #define traceENTER_xTaskGenericNotify( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue )
#endif

#ifndef traceRETURN_xTaskGenericNotify
    #define traceRETURN_xTaskGenericNotify( xReturn )
#endif

#ifndef traceENTER_xTaskGenericNotifyFromISR
    #define traceENTER_xTaskGenericNotifyFromISR( xTaskToNotify, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xTaskGenericNotifyFromISR
    #define traceRETURN_xTaskGenericNotifyFromISR( xReturn )
#endif

#ifndef traceENTER_vTaskGenericNotifyGiveFromISR
    #define traceENTER_vTaskGenericNotifyGiveFromISR( xTaskToNotify, uxIndexToNotify, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_vTaskGenericNotifyGiveFromISR
    #define traceRETURN_vTaskGenericNotifyGiveFromISR()
#endif

#ifndef traceENTER_xTaskGenericNotifyStateClear
    #define traceENTER_xTaskGenericNotifyStateClear( xTask, uxIndexToClear )
#endif

#ifndef traceRETURN_xTaskGenericNotifyStateClear
    #define traceRETURN_xTaskGenericNotifyStateClear( xReturn )
#endif

#ifndef traceENTER_ulTaskGenericNotifyValueClear
    #define traceENTER_ulTaskGenericNotifyValueClear( xTask, uxIndexToClear, ulBitsToClear )
#endif

#ifndef traceRETURN_ulTaskGenericNotifyValueClear
    #define traceRETURN_ulTaskGenericNotifyValueClear( ulReturn )
#endif

#ifndef traceENTER_ulTaskGetRunTimeCounter
    #define traceENTER_ulTaskGetRunTimeCounter( xTask )
#endif

#ifndef traceRETURN_ulTaskGetRunTimeCounter
    #define traceRETURN_ulTaskGetRunTimeCounter( ulRunTimeCounter )
#endif

#ifndef traceENTER_ulTaskGetRunTimePercent
    #define traceENTER_ulTaskGetRunTimePercent( xTask )
#endif

#ifndef traceRETURN_ulTaskGetRunTimePercent
    #define traceRETURN_ulTaskGetRunTimePercent( ulReturn )
#endif

#ifndef traceENTER_ulTaskGetIdleRunTimeCounter
    #define traceENTER_ulTaskGetIdleRunTimeCounter()
#endif

#ifndef traceRETURN_ulTaskGetIdleRunTimeCounter
    #define traceRETURN_ulTaskGetIdleRunTimeCounter( ulReturn )
#endif

#ifndef traceENTER_ulTaskGetIdleRunTimePercent
    #define traceENTER_ulTaskGetIdleRunTimePercent()
#endif

#ifndef traceRETURN_ulTaskGetIdleRunTimePercent
    #define traceRETURN_ulTaskGetIdleRunTimePercent( ulReturn )
#endif

#ifndef traceENTER_xTaskGetMPUSettings
    #define traceENTER_xTaskGetMPUSettings( xTask )
#endif

#ifndef traceRETURN_xTaskGetMPUSettings
    #define traceRETURN_xTaskGetMPUSettings( xMPUSettings )
#endif

#ifndef traceENTER_xStreamBufferGenericCreate
    #define traceENTER_xStreamBufferGenericCreate( xBufferSizeBytes, xTriggerLevelBytes, xStreamBufferType, pxSendCompletedCallback, pxReceiveCompletedCallback )
#endif

#ifndef traceRETURN_xStreamBufferGenericCreate
    #define traceRETURN_xStreamBufferGenericCreate( pvAllocatedMemory )
#endif

#ifndef traceENTER_xStreamBufferGenericCreateStatic
    #define traceENTER_xStreamBufferGenericCreateStatic( xBufferSizeBytes, xTriggerLevelBytes, xStreamBufferType, pucStreamBufferStorageArea, pxStaticStreamBuffer, pxSendCompletedCallback, pxReceiveCompletedCallback )
#endif

#ifndef traceRETURN_xStreamBufferGenericCreateStatic
    #define traceRETURN_xStreamBufferGenericCreateStatic( xReturn )
#endif

#ifndef traceENTER_xStreamBufferGetStaticBuffers
    #define traceENTER_xStreamBufferGetStaticBuffers( xStreamBuffer, ppucStreamBufferStorageArea, ppxStaticStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferGetStaticBuffers
    #define traceRETURN_xStreamBufferGetStaticBuffers( xReturn )
#endif

#ifndef traceENTER_vStreamBufferDelete
    #define traceENTER_vStreamBufferDelete( xStreamBuffer )
#endif

#ifndef traceRETURN_vStreamBufferDelete
    #define traceRETURN_vStreamBufferDelete()
#endif

#ifndef traceENTER_xStreamBufferReset
    #define traceENTER_xStreamBufferReset( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferReset
    #define traceRETURN_xStreamBufferReset( xReturn )
#endif

#ifndef traceENTER_xStreamBufferResetFromISR
    #define traceENTER_xStreamBufferResetFromISR( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferResetFromISR
    #define traceRETURN_xStreamBufferResetFromISR( xReturn )
#endif

#ifndef traceENTER_xStreamBufferSetTriggerLevel
    #define traceENTER_xStreamBufferSetTriggerLevel( xStreamBuffer, xTriggerLevel )
#endif

#ifndef traceRETURN_xStreamBufferSetTriggerLevel
    #define traceRETURN_xStreamBufferSetTriggerLevel( xReturn )
#endif

#ifndef traceENTER_xStreamBufferSpacesAvailable
    #define traceENTER_xStreamBufferSpacesAvailable( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferSpacesAvailable
    #define traceRETURN_xStreamBufferSpacesAvailable( xSpace )
#endif

#ifndef traceENTER_xStreamBufferBytesAvailable
    #define traceENTER_xStreamBufferBytesAvailable( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferBytesAvailable
    #define traceRETURN_xStreamBufferBytesAvailable( xReturn )
#endif

#ifndef traceENTER_xStreamBufferSend
    #define traceENTER_xStreamBufferSend( xStreamBuffer, pvTxData, xDataLengthBytes, xTicksToWait )
#endif

#ifndef traceRETURN_xStreamBufferSend
    #define traceRETURN_xStreamBufferSend( xReturn )
#endif

#ifndef traceENTER_xStreamBufferSendFromISR
    #define traceENTER_xStreamBufferSendFromISR( xStreamBuffer, pvTxData, xDataLengthBytes, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xStreamBufferSendFromISR
    #define traceRETURN_xStreamBufferSendFromISR( xReturn )
#endif

#ifndef traceENTER_xStreamBufferReceive
    #define traceENTER_xStreamBufferReceive( xStreamBuffer, pvRxData, xBufferLengthBytes, xTicksToWait )
#endif

#ifndef traceRETURN_xStreamBufferReceive
    #define traceRETURN_xStreamBufferReceive( xReceivedLength )
#endif

#ifndef traceENTER_xStreamBufferNextMessageLengthBytes
    #define traceENTER_xStreamBufferNextMessageLengthBytes( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferNextMessageLengthBytes
    #define traceRETURN_xStreamBufferNextMessageLengthBytes( xReturn )
#endif

#ifndef traceENTER_xStreamBufferReceiveFromISR
    #define traceENTER_xStreamBufferReceiveFromISR( xStreamBuffer, pvRxData, xBufferLengthBytes, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xStreamBufferReceiveFromISR
    #define traceRETURN_xStreamBufferReceiveFromISR( xReceivedLength )
#endif

#ifndef traceENTER_xStreamBufferIsEmpty
    #define traceENTER_xStreamBufferIsEmpty( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferIsEmpty
    #define traceRETURN_xStreamBufferIsEmpty( xReturn )
#endif

#ifndef traceENTER_xStreamBufferIsFull
    #define traceENTER_xStreamBufferIsFull( xStreamBuffer )
#endif

#ifndef traceRETURN_xStreamBufferIsFull
    #define traceRETURN_xStreamBufferIsFull( xReturn )
#endif

#ifndef traceENTER_xStreamBufferSendCompletedFromISR
    #define traceENTER_xStreamBufferSendCompletedFromISR( xStreamBuffer, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xStreamBufferSendCompletedFromISR
    #define traceRETURN_xStreamBufferSendCompletedFromISR( xReturn )
#endif

#ifndef traceENTER_xStreamBufferReceiveCompletedFromISR
    #define traceENTER_xStreamBufferReceiveCompletedFromISR( xStreamBuffer, pxHigherPriorityTaskWoken )
#endif

#ifndef traceRETURN_xStreamBufferReceiveCompletedFromISR
    #define traceRETURN_xStreamBufferReceiveCompletedFromISR( xReturn )
#endif

#ifndef traceENTER_uxStreamBufferGetStreamBufferNotificationIndex
    #define traceENTER_uxStreamBufferGetStreamBufferNotificationIndex( xStreamBuffer )
#endif

#ifndef traceRETURN_uxStreamBufferGetStreamBufferNotificationIndex
    #define traceRETURN_uxStreamBufferGetStreamBufferNotificationIndex( uxNotificationIndex )
#endif

#ifndef traceENTER_vStreamBufferSetStreamBufferNotificationIndex
    #define traceENTER_vStreamBufferSetStreamBufferNotificationIndex( xStreamBuffer, uxNotificationIndex )
#endif

#ifndef traceRETURN_vStreamBufferSetStreamBufferNotificationIndex
    #define traceRETURN_vStreamBufferSetStreamBufferNotificationIndex()
#endif

#ifndef traceENTER_uxStreamBufferGetStreamBufferNumber
    #define traceENTER_uxStreamBufferGetStreamBufferNumber( xStreamBuffer )
#endif

#ifndef traceRETURN_uxStreamBufferGetStreamBufferNumber
    #define traceRETURN_uxStreamBufferGetStreamBufferNumber( uxStreamBufferNumber )
#endif

#ifndef traceENTER_vStreamBufferSetStreamBufferNumber
    #define traceENTER_vStreamBufferSetStreamBufferNumber( xStreamBuffer, uxStreamBufferNumber )
#endif

#ifndef traceRETURN_vStreamBufferSetStreamBufferNumber
    #define traceRETURN_vStreamBufferSetStreamBufferNumber()
#endif

#ifndef traceENTER_ucStreamBufferGetStreamBufferType
    #define traceENTER_ucStreamBufferGetStreamBufferType( xStreamBuffer )
#endif

#ifndef traceRETURN_ucStreamBufferGetStreamBufferType
    #define traceRETURN_ucStreamBufferGetStreamBufferType( ucStreamBufferType )
#endif

#ifndef traceENTER_vListInitialise
    #define traceENTER_vListInitialise( pxList )
#endif

#ifndef traceRETURN_vListInitialise
    #define traceRETURN_vListInitialise()
#endif

#ifndef traceENTER_vListInitialiseItem
    #define traceENTER_vListInitialiseItem( pxItem )
#endif

#ifndef traceRETURN_vListInitialiseItem
    #define traceRETURN_vListInitialiseItem()
#endif

#ifndef traceENTER_vListInsertEnd
    #define traceENTER_vListInsertEnd( pxList, pxNewListItem )
#endif

#ifndef traceRETURN_vListInsertEnd
    #define traceRETURN_vListInsertEnd()
#endif

#ifndef traceENTER_vListInsert
    #define traceENTER_vListInsert( pxList, pxNewListItem )
#endif

#ifndef traceRETURN_vListInsert
    #define traceRETURN_vListInsert()
#endif

#ifndef traceENTER_uxListRemove
    #define traceENTER_uxListRemove( pxItemToRemove )
#endif

#ifndef traceRETURN_uxListRemove
    #define traceRETURN_uxListRemove( uxNumberOfItems )
#endif

#ifndef traceENTER_xCoRoutineCreate
    #define traceENTER_xCoRoutineCreate( pxCoRoutineCode, uxPriority, uxIndex )
#endif

#ifndef traceRETURN_xCoRoutineCreate
    #define traceRETURN_xCoRoutineCreate( xReturn )
#endif

#ifndef traceENTER_vCoRoutineAddToDelayedList
    #define traceENTER_vCoRoutineAddToDelayedList( xTicksToDelay, pxEventList )
#endif

#ifndef traceRETURN_vCoRoutineAddToDelayedList
    #define traceRETURN_vCoRoutineAddToDelayedList()
#endif

#ifndef traceENTER_vCoRoutineSchedule
    #define traceENTER_vCoRoutineSchedule()
#endif

#ifndef traceRETURN_vCoRoutineSchedule
    #define traceRETURN_vCoRoutineSchedule()
#endif

#ifndef traceENTER_xCoRoutineRemoveFromEventList
    #define traceENTER_xCoRoutineRemoveFromEventList( pxEventList )
#endif

#ifndef traceRETURN_xCoRoutineRemoveFromEventList
    #define traceRETURN_xCoRoutineRemoveFromEventList( xReturn )
#endif

#ifndef configGENERATE_RUN_TIME_STATS
    #define configGENERATE_RUN_TIME_STATS    0
#endif

#if ( configGENERATE_RUN_TIME_STATS == 1 )

    #ifndef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
        #error If configGENERATE_RUN_TIME_STATS is defined then portCONFIGURE_TIMER_FOR_RUN_TIME_STATS must also be defined.  portCONFIGURE_TIMER_FOR_RUN_TIME_STATS should call a port layer function to setup a peripheral timer/counter that can then be used as the run time counter time base.
    #endif /* portCONFIGURE_TIMER_FOR_RUN_TIME_STATS 结束 */

    #ifndef portGET_RUN_TIME_COUNTER_VALUE
        #ifndef portALT_GET_RUN_TIME_COUNTER_VALUE
            #error If configGENERATE_RUN_TIME_STATS is defined then either portGET_RUN_TIME_COUNTER_VALUE or portALT_GET_RUN_TIME_COUNTER_VALUE must also be defined.  See the examples provided and the FreeRTOS web site for more information.
        #endif /* portALT_GET_RUN_TIME_COUNTER_VALUE 结束 */
    #endif /* portGET_RUN_TIME_COUNTER_VALUE 结束 */

#endif /* configGENERATE_RUN_TIME_STATS 结束 */

#ifndef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
    #define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#endif

#ifndef portPRIVILEGE_BIT
    #define portPRIVILEGE_BIT    ( ( UBaseType_t ) 0x00 )
#endif

#ifndef portYIELD_WITHIN_API
    #define portYIELD_WITHIN_API    portYIELD
#endif

#ifndef portSUPPRESS_TICKS_AND_SLEEP
    #define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime )
#endif

#ifndef configEXPECTED_IDLE_TIME_BEFORE_SLEEP
    #define configEXPECTED_IDLE_TIME_BEFORE_SLEEP    2
#endif

#if configEXPECTED_IDLE_TIME_BEFORE_SLEEP < 2
    #error configEXPECTED_IDLE_TIME_BEFORE_SLEEP must not be less than 2
#endif

#ifndef configUSE_TICKLESS_IDLE
    #define configUSE_TICKLESS_IDLE    0
#endif

#ifndef configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING
    #define configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING( x )
#endif

#ifndef configPRE_SLEEP_PROCESSING
    #define configPRE_SLEEP_PROCESSING( x )
#endif

#ifndef configPOST_SLEEP_PROCESSING
    #define configPOST_SLEEP_PROCESSING( x )
#endif

#ifndef configUSE_QUEUE_SETS
    #define configUSE_QUEUE_SETS    0
#endif

#ifndef portTASK_USES_FLOATING_POINT
    #define portTASK_USES_FLOATING_POINT()
#endif

#ifndef portALLOCATE_SECURE_CONTEXT
    #define portALLOCATE_SECURE_CONTEXT( ulSecureStackSize )
#endif

#ifndef portDONT_DISCARD
    #define portDONT_DISCARD
#endif

#ifndef configUSE_TIME_SLICING
    #define configUSE_TIME_SLICING    1
#endif

#ifndef configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS
    #define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS    0
#endif

#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
    #define configUSE_STATS_FORMATTING_FUNCTIONS    0
#endif

#ifndef portASSERT_IF_INTERRUPT_PRIORITY_INVALID
    #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()
#endif

#ifndef configUSE_TRACE_FACILITY
    #define configUSE_TRACE_FACILITY    0
#endif

#ifndef mtCOVERAGE_TEST_MARKER
    #define mtCOVERAGE_TEST_MARKER()
#endif

#ifndef mtCOVERAGE_TEST_DELAY
    #define mtCOVERAGE_TEST_DELAY()
#endif

#ifndef portASSERT_IF_IN_ISR
    #define portASSERT_IF_IN_ISR()
#endif

#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
#endif

#ifndef configAPPLICATION_ALLOCATED_HEAP
    #define configAPPLICATION_ALLOCATED_HEAP    0
#endif

#ifndef configENABLE_HEAP_PROTECTOR
    #define configENABLE_HEAP_PROTECTOR    0
#endif

#ifndef configUSE_TASK_NOTIFICATIONS
    #define configUSE_TASK_NOTIFICATIONS    1
#endif

#ifndef configTASK_NOTIFICATION_ARRAY_ENTRIES
    #define configTASK_NOTIFICATION_ARRAY_ENTRIES    1
#endif

#if configTASK_NOTIFICATION_ARRAY_ENTRIES < 1
    #error configTASK_NOTIFICATION_ARRAY_ENTRIES must be at least 1
#endif

#ifndef configUSE_POSIX_ERRNO
    #define configUSE_POSIX_ERRNO    0
#endif

#ifndef configUSE_SB_COMPLETED_CALLBACK

/* 默认情况下，流缓冲和消息缓冲不启用按实例（per-instance）回调。 */
    #define configUSE_SB_COMPLETED_CALLBACK    0
#endif

#ifndef portTICK_TYPE_IS_ATOMIC
    #define portTICK_TYPE_IS_ATOMIC    0
#endif

#ifndef configSUPPORT_STATIC_ALLOCATION
    /* 为保持向后兼容，默认值为 0。 */
    #define configSUPPORT_STATIC_ALLOCATION    0
#endif

#ifndef configKERNEL_PROVIDED_STATIC_MEMORY
    #define configKERNEL_PROVIDED_STATIC_MEMORY    0
#endif

#ifndef configSUPPORT_DYNAMIC_ALLOCATION
    /* 为保持向后兼容，默认值为 1。 */
    #define configSUPPORT_DYNAMIC_ALLOCATION    1
#endif

#if ( ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION != 1 ) )
    #error configUSE_STATS_FORMATTING_FUNCTIONS cannot be used without dynamic allocation, but configSUPPORT_DYNAMIC_ALLOCATION is not set to 1.
#endif

#if ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 )
    #if ( ( configUSE_TRACE_FACILITY != 1 ) && ( configGENERATE_RUN_TIME_STATS != 1 ) )
        #error configUSE_STATS_FORMATTING_FUNCTIONS is 1 but the functions it enables are not used because neither configUSE_TRACE_FACILITY or configGENERATE_RUN_TIME_STATS are 1.  Set configUSE_STATS_FORMATTING_FUNCTIONS to 0 in FreeRTOSConfig.h.
    #endif
#endif

#ifndef configSTATS_BUFFER_MAX_LENGTH
    #define configSTATS_BUFFER_MAX_LENGTH    0xFFFF
#endif

#ifndef configSTACK_DEPTH_TYPE

/*
 * 为保持向后兼容，默认为 StackType_t；若 StackType_t 限制过严，可在 FreeRTOSConfig.h 中覆盖。
 */
    #define configSTACK_DEPTH_TYPE    StackType_t
#endif

#ifndef configRUN_TIME_COUNTER_TYPE

/*
 * 为保持向后兼容，默认为 uint32_t；若 uint32_t 限制过严，可在 FreeRTOSConfig.h 中覆盖。
 */

    #define configRUN_TIME_COUNTER_TYPE    uint32_t
#endif

#ifndef configMESSAGE_BUFFER_LENGTH_TYPE

/*
 * 为保持向后兼容，默认为 size_t；若长度始终小于 size_t 的字节数，可在 FreeRTOSConfig.h 中覆盖。
 */
    #define configMESSAGE_BUFFER_LENGTH_TYPE    size_t
#endif

/* 对配置进行合理性检查。 */
#if ( ( configSUPPORT_STATIC_ALLOCATION == 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 0 ) )
    #error configSUPPORT_STATIC_ALLOCATION and configSUPPORT_DYNAMIC_ALLOCATION cannot both be 0, but can both be 1.
#endif

#if ( ( configUSE_RECURSIVE_MUTEXES == 1 ) && ( configUSE_MUTEXES != 1 ) )
    #error configUSE_MUTEXES must be set to 1 to use recursive mutexes
#endif

#if ( ( configRUN_MULTIPLE_PRIORITIES == 0 ) && ( configUSE_TASK_PREEMPTION_DISABLE != 0 ) )
    #error configRUN_MULTIPLE_PRIORITIES must be set to 1 to use task preemption disable
#endif

#if ( ( configUSE_PREEMPTION == 0 ) && ( configUSE_TASK_PREEMPTION_DISABLE != 0 ) )
    #error configUSE_PREEMPTION must be set to 1 to use task preemption disable
#endif

#if ( ( configNUMBER_OF_CORES == 1 ) && ( configUSE_TASK_PREEMPTION_DISABLE != 0 ) )
    #error configUSE_TASK_PREEMPTION_DISABLE is not supported in single core FreeRTOS
#endif

#if ( ( configNUMBER_OF_CORES == 1 ) && ( configUSE_CORE_AFFINITY != 0 ) )
    #error configUSE_CORE_AFFINITY is not supported in single core FreeRTOS
#endif

#if ( ( configNUMBER_OF_CORES > 1 ) && ( configUSE_PORT_OPTIMISED_TASK_SELECTION != 0 ) )
    #error configUSE_PORT_OPTIMISED_TASK_SELECTION is not supported in SMP FreeRTOS
#endif

#ifndef configINITIAL_TICK_COUNT
    #define configINITIAL_TICK_COUNT    0
#endif

#if ( portTICK_TYPE_IS_ATOMIC == 0 )

/*
 * 时钟节拍类型的变量无法原子读取，或未设置 portTICK_TYPE_IS_ATOMIC ——
 * 此时将读取节拍计数时使用的临界区映射为标准临界区宏。
 */
    #define portTICK_TYPE_ENTER_CRITICAL()                      portENTER_CRITICAL()
    #define portTICK_TYPE_EXIT_CRITICAL()                       portEXIT_CRITICAL()
    #define portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR()         portSET_INTERRUPT_MASK_FROM_ISR()
    #define portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( x )    portCLEAR_INTERRUPT_MASK_FROM_ISR( ( x ) )
#else

/*
 * 时钟节拍类型可原子读取，因此读取节拍计数时使用的临界区可定义为空。
 */
    #define portTICK_TYPE_ENTER_CRITICAL()
    #define portTICK_TYPE_EXIT_CRITICAL()
    #define portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR()         0
    #define portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( x )    ( void ) ( x )
#endif /* 结束：if ( portTICK_TYPE_IS_ATOMIC == 0 ) */

/*
 * 用于在需要时提供与 V8 之前 FreeRTOS 版本的向后兼容性定义。
 */
#ifndef configENABLE_BACKWARD_COMPATIBILITY
    #define configENABLE_BACKWARD_COMPATIBILITY    1
#endif

#ifndef configPRINTF

/*
 * 未定义 configPRINTF()，因此将其定义为空。若要使用
 * configPRINTF()，请按如下方式定义（其中 MyPrintFunction() 由应用编写者提供）：
 *
 * void MyPrintFunction(const char *pcFormat, ... );
 * #define configPRINTF( X )   MyPrintFunction X
 *
 * 然后像标准 printf() 函数一样调用，但需用括号将所有参数括起来，
 * 使其作为单个参数传递。例如：
 * configPRINTF( ("Value = %d", MyVariable) );
 */
    #define configPRINTF( X )
#endif

#ifndef configMAX

/*
 * 应用编写者未提供自定义的 MAX 宏，因此定义以下通用实现。
 */
    #define configMAX( a, b )    ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

#ifndef configMIN

/*
 * 应用编写者未提供自定义的 MIN 宏，因此定义以下通用实现。
 */
    #define configMIN( a, b )    ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#if configENABLE_BACKWARD_COMPATIBILITY == 1
    #define eTaskStateGet                 eTaskGetState
    #define portTickType                  TickType_t
    #define xTaskHandle                   TaskHandle_t
    #define xQueueHandle                  QueueHandle_t
    #define xSemaphoreHandle              SemaphoreHandle_t
    #define xQueueSetHandle               QueueSetHandle_t
    #define xQueueSetMemberHandle         QueueSetMemberHandle_t
    #define xTimeOutType                  TimeOut_t
    #define xMemoryRegion                 MemoryRegion_t
    #define xTaskParameters               TaskParameters_t
    #define xTaskStatusType               TaskStatus_t
    #define xTimerHandle                  TimerHandle_t
    #define xCoRoutineHandle              CoRoutineHandle_t
    #define pdTASK_HOOK_CODE              TaskHookFunction_t
    #define portTICK_RATE_MS              portTICK_PERIOD_MS
    #define pcTaskGetTaskName             pcTaskGetName
    #define pcTimerGetTimerName           pcTimerGetName
    #define pcQueueGetQueueName           pcQueueGetName
    #define vTaskGetTaskInfo              vTaskGetInfo
    #define xTaskGetIdleRunTimeCounter    ulTaskGetIdleRunTimeCounter

/*
 * 仅用于调度器代码内部的向后兼容——这些定义并非真正必需，但为保证完整性而保留。
 */
    #define tmrTIMER_CALLBACK             TimerCallbackFunction_t
    #define pdTASK_CODE                   TaskFunction_t
    #define xListItem                     ListItem_t
    #define xList                         List_t

/*
 * 供那些破坏链表数据隐藏、直接访问链表结构成员的库使用（不应这样做）。
 */
    #define pxContainer                   pvContainer
#endif /* configENABLE_BACKWARD_COMPATIBILITY 结束 */

#if ( configUSE_ALTERNATIVE_API != 0 )
    #error The alternative API was deprecated some time ago, and was removed in FreeRTOS V9.0 0
#endif

/*
 * 将 configUSE_TASK_FPU_SUPPORT 设置为 0，即使所用 FreeRTOS 移植层支持浮点硬件，也省略浮点支持。
 * 并非所有包含浮点支持的 FreeRTOS 移植层都支持该常量。
 */
#ifndef configUSE_TASK_FPU_SUPPORT
    #define configUSE_TASK_FPU_SUPPORT    1
#endif

/*
 * 将 configENABLE_MPU 设置为 1 以启用 MPU 支持，设置为 0 则禁用。该配置目前用于 ARMv8M 移植层。
 */
#ifndef configENABLE_MPU
    #define configENABLE_MPU    0
#endif

/*
 * 将 configENABLE_FPU 设置为 1 以启用 FPU 支持，设置为 0 则禁用。该配置目前用于 ARMv8M 移植层。
 */
#ifndef configENABLE_FPU
    #define configENABLE_FPU    1
#endif

/*
 * 将 configENABLE_MVE 设置为 1 以启用 MVE 支持，设置为 0 则禁用。该配置目前用于 ARMv8M 移植层。
 */
#ifndef configENABLE_MVE
    #define configENABLE_MVE    0
#endif

/*
 * 将 configENABLE_TRUSTZONE 设置为 1 以启用 TrustZone 支持，设置为 0 则禁用。该配置目前用于 ARMv8M 移植层。
 */
#ifndef configENABLE_TRUSTZONE
    #define configENABLE_TRUSTZONE    1
#endif

/*
 * 将 configRUN_FREERTOS_SECURE_ONLY 设置为 1，使 FreeRTOS ARMv8M 移植层仅在安全侧运行。
 */
#ifndef configRUN_FREERTOS_SECURE_ONLY
    #define configRUN_FREERTOS_SECURE_ONLY    0
#endif

#ifndef configRUN_ADDITIONAL_TESTS
    #define configRUN_ADDITIONAL_TESTS    0
#endif

/*
 * 以下配置允许对死循环进行控制。例如，在执行单元测试时控制空闲任务函数中的死循环。
 */
#ifndef configCONTROL_INFINITE_LOOP
    #define configCONTROL_INFINITE_LOOP()
#endif

/*
 * 有时 FreeRTOSConfig.h 的配置只允许使用动态分配的 RAM 创建任务，此时删除任何任务时，已知需要释放任务的栈和 TCB。
 * 有时 FreeRTOSConfig.h 的配置只允许使用静态分配的 RAM 创建任务，此时删除任何任务时，已知既不应释放任务的栈也不应释放 TCB。
 * 有时 FreeRTOSConfig.h 的配置允许使用静态或动态分配的 RAM 创建任务，此时 TCB 中的某个成员用于记录栈和/或 TCB 是静态还是动态分配的，
 * 因此删除任务时会释放动态分配的 RAM，而不会尝试释放静态分配的 RAM。
 * tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE 仅当任务既可使用静态分配的 RAM 也可使用动态分配的 RAM 创建时才为真。
 * 注意，若 portUSING_MPU_WRAPPERS 为 1，则可创建栈为静态分配、TCB 为动态分配的受保护任务。
 *
 * 下表列出 portUSING_MPU_WRAPPERS、configSUPPORT_DYNAMIC_ALLOCATION 和 configSUPPORT_STATIC_ALLOCATION 的各种组合，
 * 以及何时可能同时采用静态和动态分配：
 *  +-----+---------+--------+-----------------------------+-----------------------------------+------------------+-----------+
 * | MPU | Dynamic | Static |     Available Functions     |       Possible Allocations        | Both Dynamic and | Need Free |
 * |     |         |        |                             |                                   | Static Possible  |           |
 * +-----+---------+--------+-----------------------------+-----------------------------------+------------------+-----------+
 * | 0   | 0       | 1      | xTaskCreateStatic           | TCB - Static, Stack - Static      | No               | No        |
 * +-----|---------|--------|-----------------------------|-----------------------------------|------------------|-----------|
 * | 0   | 1       | 0      | xTaskCreate                 | TCB - Dynamic, Stack - Dynamic    | No               | Yes       |
 * +-----|---------|--------|-----------------------------|-----------------------------------|------------------|-----------|
 * | 0   | 1       | 1      | xTaskCreate,                | 1. TCB - Dynamic, Stack - Dynamic | Yes              | Yes       |
 * |     |         |        | xTaskCreateStatic           | 2. TCB - Static, Stack - Static   |                  |           |
 * +-----|---------|--------|-----------------------------|-----------------------------------|------------------|-----------|
 * | 1   | 0       | 1      | xTaskCreateStatic,          | TCB - Static, Stack - Static      | No               | No        |
 * |     |         |        | xTaskCreateRestrictedStatic |                                   |                  |           |
 * +-----|---------|--------|-----------------------------|-----------------------------------|------------------|-----------|
 * | 1   | 1       | 0      | xTaskCreate,                | 1. TCB - Dynamic, Stack - Dynamic | Yes              | Yes       |
 * |     |         |        | xTaskCreateRestricted       | 2. TCB - Dynamic, Stack - Static  |                  |           |
 * +-----|---------|--------|-----------------------------|-----------------------------------|------------------|-----------|
 * | 1   | 1       | 1      | xTaskCreate,                | 1. TCB - Dynamic, Stack - Dynamic | Yes              | Yes       |
 * |     |         |        | xTaskCreateStatic,          | 2. TCB - Dynamic, Stack - Static  |                  |           |
 * |     |         |        | xTaskCreateRestricted,      | 3. TCB - Static, Stack - Static   |                  |           |
 * |     |         |        | xTaskCreateRestrictedStatic |                                   |                  |           |
 * +-----+---------+--------+-----------------------------+-----------------------------------+------------------+-----------+
 */
#define tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE                                                                                     \
    ( ( ( portUSING_MPU_WRAPPERS == 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) || \
      ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) )

/*
 * 遵循软件工程最佳实践，FreeRTOS 实现了严格的数据隐藏策略，因此 FreeRTOS 内部用于维护任务、队列、信号量等状态的真实结构对应用代码不可见。
 * 但是，如果应用编写者希望静态分配此类对象，则需要知道对象的大小。
 * 为此使用虚拟结构（dummy structure），保证其大小和对齐要求与真实对象相同。
 * 下面的虚拟链表和链表项结构用于嵌入此类虚拟结构中。
 */
struct xSTATIC_LIST_ITEM
{
    #if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 1 )
        TickType_t xDummy1;
    #endif
    TickType_t xDummy2;
    void * pvDummy3[ 4 ];
    #if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 1 )
        TickType_t xDummy4;
    #endif
};
typedef struct xSTATIC_LIST_ITEM StaticListItem_t;

#if ( configUSE_MINI_LIST_ITEM == 1 )
    /* 参见 struct xSTATIC_LIST_ITEM 定义上方的注释。 */
    struct xSTATIC_MINI_LIST_ITEM
    {
        #if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 1 )
            TickType_t xDummy1;
        #endif
        TickType_t xDummy2;
        void * pvDummy3[ 2 ];
    };
    typedef struct xSTATIC_MINI_LIST_ITEM StaticMiniListItem_t;
#else /* 结束：if ( configUSE_MINI_LIST_ITEM == 1 ) */
    typedef struct xSTATIC_LIST_ITEM      StaticMiniListItem_t;
#endif /* 结束：if ( configUSE_MINI_LIST_ITEM == 1 ) */

/* 参见 struct xSTATIC_LIST_ITEM 定义上方的注释。 */
typedef struct xSTATIC_LIST
{
    #if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 1 )
        TickType_t xDummy1;
    #endif
    UBaseType_t uxDummy2;
    void * pvDummy3;
    StaticMiniListItem_t xDummy4;
    #if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 1 )
        TickType_t xDummy5;
    #endif
} StaticList_t;

/*
 * 遵循软件工程最佳实践，特别是在提供可能在未来版本中变化的库时，FreeRTOS 实现了严格的数据隐藏策略。
 * 这意味着 FreeRTOS 内部使用的任务结构对应用代码不可见。
 * 但是，如果应用编写者希望静态分配创建任务所需的内存，则需要知道任务对象的大小。
 * 为此提供了下面的 StaticTask_t 结构。无论使用何种架构、无论 FreeRTOSConfig.h 中的值如何设置，
 * 其大小和对齐要求都保证与真实结构一致。其内容经过一定程度的混淆，希望用户认识到直接使用结构成员是不明智的。
 */
typedef struct xSTATIC_TCB
{
    void * pxDummy1;
    #if ( portUSING_MPU_WRAPPERS == 1 )
        xMPU_SETTINGS xDummy2;
    #endif
    #if ( configUSE_CORE_AFFINITY == 1 ) && ( configNUMBER_OF_CORES > 1 )
        UBaseType_t uxDummy26;
    #endif
    StaticListItem_t xDummy3[ 2 ];
    UBaseType_t uxDummy5;
    void * pxDummy6;
    #if ( configNUMBER_OF_CORES > 1 )
        BaseType_t xDummy23;
        UBaseType_t uxDummy24;
    #endif
    uint8_t ucDummy7[ configMAX_TASK_NAME_LEN ];
    #if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
        BaseType_t xDummy25;
    #endif
    #if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
        void * pxDummy8;
    #endif
    #if ( portCRITICAL_NESTING_IN_TCB == 1 )
        UBaseType_t uxDummy9;
    #endif
    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxDummy10[ 2 ];
    #endif
    #if ( configUSE_MUTEXES == 1 )
        UBaseType_t uxDummy12[ 2 ];
    #endif
    #if ( configUSE_APPLICATION_TASK_TAG == 1 )
        void * pxDummy14;
    #endif
    #if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
        void * pvDummy15[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
    #endif
    #if ( configGENERATE_RUN_TIME_STATS == 1 )
        configRUN_TIME_COUNTER_TYPE ulDummy16;
    #endif
    #if ( configUSE_C_RUNTIME_TLS_SUPPORT == 1 )
        configTLS_BLOCK_TYPE xDummy17;
    #endif
    #if ( configUSE_TASK_NOTIFICATIONS == 1 )
        uint32_t ulDummy18[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
        uint8_t ucDummy19[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
    #endif
    #if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
        uint8_t uxDummy20;
    #endif

    #if ( INCLUDE_xTaskAbortDelay == 1 )
        uint8_t ucDummy21;
    #endif
    #if ( configUSE_POSIX_ERRNO == 1 )
        int iDummy22;
    #endif
} StaticTask_t;

/*
 * 遵循软件工程最佳实践，特别是在提供可能在未来版本中变化的库时，FreeRTOS 实现了严格的数据隐藏策略。
 * 这意味着 FreeRTOS 内部使用的队列结构对应用代码不可见。
 * 但是，如果应用编写者希望静态分配创建队列所需的内存，则需要知道队列对象的大小。
 * 为此提供了下面的 StaticQueue_t 结构。无论使用何种架构、无论 FreeRTOSConfig.h 中的值如何设置，
 * 其大小和对齐要求都保证与真实结构一致。其内容经过一定程度的混淆，希望用户认识到直接使用结构成员是不明智的。
 */
typedef struct xSTATIC_QUEUE
{
    void * pvDummy1[ 3 ];

    union
    {
        void * pvDummy2;
        UBaseType_t uxDummy2;
    } u;

    StaticList_t xDummy3[ 2 ];
    UBaseType_t uxDummy4[ 3 ];
    uint8_t ucDummy5[ 2 ];

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
        uint8_t ucDummy6;
    #endif

    #if ( configUSE_QUEUE_SETS == 1 )
        void * pvDummy7;
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxDummy8;
        uint8_t ucDummy9;
    #endif
} StaticQueue_t;
typedef StaticQueue_t StaticSemaphore_t;

/*
 * 遵循软件工程最佳实践，特别是在提供可能在未来版本中变化的库时，FreeRTOS 实现了严格的数据隐藏策略。
 * 这意味着 FreeRTOS 内部使用的事件组结构对应用代码不可见。
 * 但是，如果应用编写者希望静态分配创建事件组所需的内存，则需要知道事件组对象的大小。
 * 为此提供了下面的 StaticEventGroup_t 结构。无论使用何种架构、无论 FreeRTOSConfig.h 中的值如何设置，
 * 其大小和对齐要求都保证与真实结构一致。其内容经过一定程度的混淆，希望用户认识到直接使用结构成员是不明智的。
 */
typedef struct xSTATIC_EVENT_GROUP
{
    TickType_t xDummy1;
    StaticList_t xDummy2;

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxDummy3;
    #endif

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
        uint8_t ucDummy4;
    #endif
} StaticEventGroup_t;

/*
 * 遵循软件工程最佳实践，特别是在提供可能在未来版本中变化的库时，FreeRTOS 实现了严格的数据隐藏策略。
 * 这意味着 FreeRTOS 内部使用的软件定时器结构对应用代码不可见。
 * 但是，如果应用编写者希望静态分配创建软件定时器所需的内存，则需要知道软件定时器对象的大小。
 * 为此提供了下面的 StaticTimer_t 结构。无论使用何种架构、无论 FreeRTOSConfig.h 中的值如何设置，
 * 其大小和对齐要求都保证与真实结构一致。其内容经过一定程度的混淆，希望用户认识到直接使用结构成员是不明智的。
 */
typedef struct xSTATIC_TIMER
{
    void * pvDummy1;
    StaticListItem_t xDummy2;
    TickType_t xDummy3;
    void * pvDummy5;
    TaskFunction_t pvDummy6;
    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxDummy7;
    #endif
    uint8_t ucDummy8;
} StaticTimer_t;

/*
 * 遵循软件工程最佳实践，特别是在提供可能在未来版本中变化的库时，FreeRTOS 实现了严格的数据隐藏策略。
 * 这意味着 FreeRTOS 内部使用的流缓冲结构对应用代码不可见。
 * 但是，如果应用编写者希望静态分配创建流缓冲所需的内存，则需要知道流缓冲对象的大小。
 * 为此提供了下面的 StaticStreamBuffer_t 结构。无论使用何种架构、无论 FreeRTOSConfig.h 中的值如何设置，
 * 其大小和对齐要求都保证与真实结构一致。其内容经过一定程度的混淆，希望用户认识到直接使用结构成员是不明智的。
 */
typedef struct xSTATIC_STREAM_BUFFER
{
    size_t uxDummy1[ 4 ];
    void * pvDummy2[ 3 ];
    uint8_t ucDummy3;
    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxDummy4;
    #endif
    #if ( configUSE_SB_COMPLETED_CALLBACK == 1 )
        void * pvDummy5[ 2 ];
    #endif
    UBaseType_t uxDummy6;
} StaticStreamBuffer_t;

/* 消息缓冲建立在流缓冲之上。 */
typedef StaticStreamBuffer_t StaticMessageBuffer_t;

/* *INDENT-OFF* 关闭自动缩进 */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* 恢复自动缩进 */

#endif /* INC_FREERTOS_H 结束 */
