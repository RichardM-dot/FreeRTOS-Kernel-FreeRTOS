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

#ifndef MPU_WRAPPERS_H
#define MPU_WRAPPERS_H

/*
 * 此文件将 API 函数重定义为通过包装宏调用，但仅适用于
 * 使用 MPU 的移植层。
 */
#if ( portUSING_MPU_WRAPPERS == 1 )

/*
 * 当此文件从 queue.c 或 task.c 包含时，将定义
 * MPU_WRAPPERS_INCLUDED_FROM_API_FILE，以防止其在这些文件内生效。
 */
    #ifndef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*
 * 将标准（非 MPU）API 函数映射到以 "MPU_" 开头的等效函数。
 * 这将使应用程序代码调用 MPU_ 版本，该版本在非 MPU 版本
 * 外面包装了提升权限再降低权限的代码，因此内核代码始终以
 * 完整权限运行。
 */

/* 将标准 task.h API 函数映射到对应的 MPU 等效函数。 */
        #define vTaskDelay                            MPU_vTaskDelay
        #define xTaskDelayUntil                       MPU_xTaskDelayUntil
        #define xTaskAbortDelay                       MPU_xTaskAbortDelay
        #define uxTaskPriorityGet                     MPU_uxTaskPriorityGet
        #define eTaskGetState                         MPU_eTaskGetState
        #define vTaskGetInfo                          MPU_vTaskGetInfo
        #define vTaskSuspend                          MPU_vTaskSuspend
        #define vTaskResume                           MPU_vTaskResume
        #define xTaskGetTickCount                     MPU_xTaskGetTickCount
        #define uxTaskGetNumberOfTasks                MPU_uxTaskGetNumberOfTasks
        #define uxTaskGetStackHighWaterMark           MPU_uxTaskGetStackHighWaterMark
        #define uxTaskGetStackHighWaterMark2          MPU_uxTaskGetStackHighWaterMark2
        #define vTaskSetApplicationTaskTag            MPU_vTaskSetApplicationTaskTag
        #define xTaskGetApplicationTaskTag            MPU_xTaskGetApplicationTaskTag
        #define vTaskSetThreadLocalStoragePointer     MPU_vTaskSetThreadLocalStoragePointer
        #define pvTaskGetThreadLocalStoragePointer    MPU_pvTaskGetThreadLocalStoragePointer
        #define xTaskGetIdleTaskHandle                MPU_xTaskGetIdleTaskHandle
        #define uxTaskGetSystemState                  MPU_uxTaskGetSystemState
        #define ulTaskGetIdleRunTimeCounter           MPU_ulTaskGetIdleRunTimeCounter
        #define ulTaskGetIdleRunTimePercent           MPU_ulTaskGetIdleRunTimePercent
        #define xTaskGenericNotify                    MPU_xTaskGenericNotify
        #define xTaskGenericNotifyWait                MPU_xTaskGenericNotifyWait
        #define ulTaskGenericNotifyTake               MPU_ulTaskGenericNotifyTake
        #define xTaskGenericNotifyStateClear          MPU_xTaskGenericNotifyStateClear
        #define ulTaskGenericNotifyValueClear         MPU_ulTaskGenericNotifyValueClear
        #define vTaskSetTimeOutState                  MPU_vTaskSetTimeOutState
        #define xTaskCheckForTimeOut                  MPU_xTaskCheckForTimeOut
        #define xTaskGetCurrentTaskHandle             MPU_xTaskGetCurrentTaskHandle
        #define xTaskGetSchedulerState                MPU_xTaskGetSchedulerState

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define ulTaskGetRunTimeCounter           MPU_ulTaskGetRunTimeCounter
            #define ulTaskGetRunTimePercent           MPU_ulTaskGetRunTimePercent
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

/*
 * 任务 API 的仅特权包装器。这些包装器是必需的，以便应用程序
 * 可在所有 API 中使用由 mpu_wrappers.c 维护的不透明句柄。
 */
        #if ( configUSE_MPU_WRAPPERS_V1 == 1 )

/*
 * v2 中不需要这些包装器，因为它们不接收任务句柄，因而无需查找。
 * v1 中需要它们，因为在 v1 中这些函数以系统调用形式提供。
 */
            #define vTaskGetRunTimeStatistics            MPU_vTaskGetRunTimeStatistics
            #define vTaskListTasks                       MPU_vTaskListTasks
            #define vTaskSuspendAll                      MPU_vTaskSuspendAll
            #define xTaskCatchUpTicks                    MPU_xTaskCatchUpTicks
            #define xTaskResumeAll                       MPU_xTaskResumeAll
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 1 ) */

        #define xTaskCreate                              MPU_xTaskCreate
        #define xTaskCreateStatic                        MPU_xTaskCreateStatic
        #define vTaskDelete                              MPU_vTaskDelete
        #define vTaskPrioritySet                         MPU_vTaskPrioritySet
        #define xTaskGetHandle                           MPU_xTaskGetHandle
        #define xTaskCallApplicationTaskHook             MPU_xTaskCallApplicationTaskHook

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define pcTaskGetName                        MPU_pcTaskGetName
            #define xTaskCreateRestricted                MPU_xTaskCreateRestricted
            #define xTaskCreateRestrictedStatic          MPU_xTaskCreateRestrictedStatic
            #define vTaskAllocateMPURegions              MPU_vTaskAllocateMPURegions
            #define xTaskGetStaticBuffers                MPU_xTaskGetStaticBuffers
            #define uxTaskPriorityGetFromISR             MPU_uxTaskPriorityGetFromISR
            #define uxTaskBasePriorityGet                MPU_uxTaskBasePriorityGet
            #define uxTaskBasePriorityGetFromISR         MPU_uxTaskBasePriorityGetFromISR
            #define xTaskResumeFromISR                   MPU_xTaskResumeFromISR
            #define xTaskGetApplicationTaskTagFromISR    MPU_xTaskGetApplicationTaskTagFromISR
            #define xTaskGenericNotifyFromISR            MPU_xTaskGenericNotifyFromISR
            #define vTaskGenericNotifyGiveFromISR        MPU_vTaskGenericNotifyGiveFromISR
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

/* 将标准 queue.h API 函数映射到对应的 MPU 等效函数。 */
        #define xQueueGenericSend            MPU_xQueueGenericSend
        #define xQueueReceive                MPU_xQueueReceive
        #define xQueuePeek                   MPU_xQueuePeek
        #define xQueueSemaphoreTake          MPU_xQueueSemaphoreTake
        #define uxQueueMessagesWaiting       MPU_uxQueueMessagesWaiting
        #define uxQueueSpacesAvailable       MPU_uxQueueSpacesAvailable
        #define xQueueGetMutexHolder         MPU_xQueueGetMutexHolder
        #define xQueueTakeMutexRecursive     MPU_xQueueTakeMutexRecursive
        #define xQueueGiveMutexRecursive     MPU_xQueueGiveMutexRecursive
        #define xQueueAddToSet               MPU_xQueueAddToSet
        #define xQueueSelectFromSet          MPU_xQueueSelectFromSet

        #if ( configQUEUE_REGISTRY_SIZE > 0 )
            #define vQueueAddToRegistry      MPU_vQueueAddToRegistry
            #define vQueueUnregisterQueue    MPU_vQueueUnregisterQueue
            #define pcQueueGetName           MPU_pcQueueGetName
        #endif /* 条件编译结束：if ( configQUEUE_REGISTRY_SIZE > 0 ) */

/*
 * 队列 API 的仅特权包装器。这些包装器是必需的，以便应用程序
 * 可在所有 API 中使用由 mpu_wrappers.c 维护的不透明句柄。
 */
        #define vQueueDelete                           MPU_vQueueDelete
        #define xQueueCreateMutex                      MPU_xQueueCreateMutex
        #define xQueueCreateMutexStatic                MPU_xQueueCreateMutexStatic
        #define xQueueCreateCountingSemaphore          MPU_xQueueCreateCountingSemaphore
        #define xQueueCreateCountingSemaphoreStatic    MPU_xQueueCreateCountingSemaphoreStatic
        #define xQueueGenericCreate                    MPU_xQueueGenericCreate
        #define xQueueGenericCreateStatic              MPU_xQueueGenericCreateStatic
        #define xQueueGenericReset                     MPU_xQueueGenericReset
        #define xQueueCreateSet                        MPU_xQueueCreateSet
        #define xQueueCreateSetStatic                  MPU_xQueueCreateSetStatic
        #define xQueueRemoveFromSet                    MPU_xQueueRemoveFromSet

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define xQueueGenericGetStaticBuffers      MPU_xQueueGenericGetStaticBuffers
            #define xQueueGenericSendFromISR           MPU_xQueueGenericSendFromISR
            #define xQueueGiveFromISR                  MPU_xQueueGiveFromISR
            #define xQueuePeekFromISR                  MPU_xQueuePeekFromISR
            #define xQueueReceiveFromISR               MPU_xQueueReceiveFromISR
            #define xQueueIsQueueEmptyFromISR          MPU_xQueueIsQueueEmptyFromISR
            #define xQueueIsQueueFullFromISR           MPU_xQueueIsQueueFullFromISR
            #define uxQueueMessagesWaitingFromISR      MPU_uxQueueMessagesWaitingFromISR
            #define xQueueGetMutexHolderFromISR        MPU_xQueueGetMutexHolderFromISR
            #define xQueueSelectFromSetFromISR         MPU_xQueueSelectFromSetFromISR
        #endif /* 条件编译结束：if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

/* 将标准 timer.h API 函数映射到对应的 MPU 等效函数。 */
        #define pvTimerGetTimerID                 MPU_pvTimerGetTimerID
        #define vTimerSetTimerID                  MPU_vTimerSetTimerID
        #define xTimerIsTimerActive               MPU_xTimerIsTimerActive
        #define xTimerGetTimerDaemonTaskHandle    MPU_xTimerGetTimerDaemonTaskHandle
        #define xTimerGenericCommandFromTask      MPU_xTimerGenericCommandFromTask
        #define pcTimerGetName                    MPU_pcTimerGetName
        #define vTimerSetReloadMode               MPU_vTimerSetReloadMode
        #define uxTimerGetReloadMode              MPU_uxTimerGetReloadMode
        #define xTimerGetPeriod                   MPU_xTimerGetPeriod
        #define xTimerGetExpiryTime               MPU_xTimerGetExpiryTime

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define xTimerGetReloadMode           MPU_xTimerGetReloadMode
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

/*
 * 定时器 API 的仅特权包装器。这些包装器是必需的，以便应用程序
 * 可在所有 API 中使用由 mpu_wrappers.c 维护的不透明句柄。
 */
        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define xTimerCreate                   MPU_xTimerCreate
            #define xTimerCreateStatic             MPU_xTimerCreateStatic
            #define xTimerGetStaticBuffer          MPU_xTimerGetStaticBuffer
            #define xTimerGenericCommandFromISR    MPU_xTimerGenericCommandFromISR
            #define xTimerDelete                   MPU_xTimerDelete
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

/* 将标准 event_group.h API 函数映射到对应的 MPU 等效函数。 */
        #define xEventGroupWaitBits          MPU_xEventGroupWaitBits
        #define xEventGroupClearBits         MPU_xEventGroupClearBits
        #define xEventGroupSetBits           MPU_xEventGroupSetBits
        #define xEventGroupSync              MPU_xEventGroupSync

        #if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )
            #define uxEventGroupGetNumber    MPU_uxEventGroupGetNumber
            #define vEventGroupSetNumber     MPU_vEventGroupSetNumber
        #endif /* 条件编译结束：#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) ) */

/*
 * 事件组 API 的仅特权包装器。这些包装器是必需的，以便应用程序
 * 可在所有 API 中使用由 mpu_wrappers.c 维护的不透明句柄。
 */
        #define xEventGroupCreate                  MPU_xEventGroupCreate
        #define xEventGroupCreateStatic            MPU_xEventGroupCreateStatic
        #define vEventGroupDelete                  MPU_vEventGroupDelete

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define xEventGroupGetStaticBuffer     MPU_xEventGroupGetStaticBuffer
            #define xEventGroupClearBitsFromISR    MPU_xEventGroupClearBitsFromISR
            #define xEventGroupSetBitsFromISR      MPU_xEventGroupSetBitsFromISR
            #define xEventGroupGetBitsFromISR      MPU_xEventGroupGetBitsFromISR
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

/*
 * 将标准 message/stream_buffer.h API 函数映射到对应的 MPU
 * 等效函数。
 */
        #define xStreamBufferSend                      MPU_xStreamBufferSend
        #define xStreamBufferReceive                   MPU_xStreamBufferReceive
        #define xStreamBufferIsFull                    MPU_xStreamBufferIsFull
        #define xStreamBufferIsEmpty                   MPU_xStreamBufferIsEmpty
        #define xStreamBufferSpacesAvailable           MPU_xStreamBufferSpacesAvailable
        #define xStreamBufferBytesAvailable            MPU_xStreamBufferBytesAvailable
        #define xStreamBufferSetTriggerLevel           MPU_xStreamBufferSetTriggerLevel
        #define xStreamBufferNextMessageLengthBytes    MPU_xStreamBufferNextMessageLengthBytes

/*
 * 流缓冲 API 的仅特权包装器。这些包装器是必需的，以便应用程序
 * 可在所有 API 中使用由 mpu_wrappers.c 维护的不透明句柄。
 */

        #define xStreamBufferGenericCreate                  MPU_xStreamBufferGenericCreate
        #define xStreamBufferGenericCreateStatic            MPU_xStreamBufferGenericCreateStatic
        #define vStreamBufferDelete                         MPU_vStreamBufferDelete
        #define xStreamBufferReset                          MPU_xStreamBufferReset

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            #define xStreamBufferGetStaticBuffers           MPU_xStreamBufferGetStaticBuffers
            #define xStreamBufferSendFromISR                MPU_xStreamBufferSendFromISR
            #define xStreamBufferReceiveFromISR             MPU_xStreamBufferReceiveFromISR
            #define xStreamBufferSendCompletedFromISR       MPU_xStreamBufferSendCompletedFromISR
            #define xStreamBufferReceiveCompletedFromISR    MPU_xStreamBufferReceiveCompletedFromISR
            #define xStreamBufferResetFromISR               MPU_xStreamBufferResetFromISR
        #endif /* 条件编译结束：#if ( configUSE_MPU_WRAPPERS_V1 == 0 ) */

        #if ( ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

            #define vGrantAccessToTask( xTask, xTaskToGrantAccess )                        vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xTaskToGrantAccess ) )

/*
 * 特权任务必须在删除对象之前撤销对该对象的访问权限，
 * 否则索引复用可能导致意外的权限。
 */
            #define vRevokeAccessToTask( xTask, xTaskToRevokeAccess )                      vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xTaskToRevokeAccess ) )

            #define vGrantAccessToSemaphore( xTask, xSemaphoreToGrantAccess )              vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xSemaphoreToGrantAccess ) )
            #define vRevokeAccessToSemaphore( xTask, xSemaphoreToRevokeAccess )            vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xSemaphoreToRevokeAccess ) )

            #define vGrantAccessToQueue( xTask, xQueueToGrantAccess )                      vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xQueueToGrantAccess ) )
            #define vRevokeAccessToQueue( xTask, xQueueToRevokeAccess )                    vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xQueueToRevokeAccess ) )

            #define vGrantAccessToQueueSet( xTask, xQueueSetToGrantAccess )                vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xQueueSetToGrantAccess ) )
            #define vRevokeAccessToQueueSet( xTask, xQueueSetToRevokeAccess )              vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xQueueSetToRevokeAccess ) )

            #define vGrantAccessToEventGroup( xTask, xEventGroupToGrantAccess )            vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xEventGroupToGrantAccess ) )
            #define vRevokeAccessToEventGroup( xTask, xEventGroupToRevokeAccess )          vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xEventGroupToRevokeAccess ) )

            #define vGrantAccessToStreamBuffer( xTask, xStreamBufferToGrantAccess )        vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xStreamBufferToGrantAccess ) )
            #define vRevokeAccessToStreamBuffer( xTask, xStreamBufferToRevokeAccess )      vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xStreamBufferToRevokeAccess ) )

            #define vGrantAccessToMessageBuffer( xTask, xMessageBufferToGrantAccess )      vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xMessageBufferToGrantAccess ) )
            #define vRevokeAccessToMessageBuffer( xTask, xMessageBufferToRevokeAccess )    vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xMessageBufferToRevokeAccess ) )

            #define vGrantAccessToTimer( xTask, xTimerToGrantAccess )                      vGrantAccessToKernelObject( ( xTask ), ( int32_t ) ( xTimerToGrantAccess ) )
            #define vRevokeAccessToTimer( xTask, xTimerToRevokeAccess )                    vRevokeAccessToKernelObject( ( xTask ), ( int32_t ) ( xTimerToRevokeAccess ) )

        #endif /* 条件编译结束：#if ( ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) ) */

    #endif /* 配置项 MPU_WRAPPERS_INCLUDED_FROM_API_FILE */

    #define PRIVILEGED_FUNCTION     __attribute__( ( section( "privileged_functions" ) ) )
    #define PRIVILEGED_DATA         __attribute__( ( section( "privileged_data" ) ) )
    #define FREERTOS_SYSTEM_CALL    __attribute__( ( section( "freertos_system_calls" ) ) )

#else /* 条件不成立：portUSING_MPU_WRAPPERS */

    #ifndef PRIVILEGED_FUNCTION
        #define PRIVILEGED_FUNCTION
    #endif

    #ifndef PRIVILEGED_DATA
        #define PRIVILEGED_DATA
    #endif

    #ifndef FREERTOS_SYSTEM_CALL
        #define FREERTOS_SYSTEM_CALL
    #endif

#endif /* 条件编译结束：portUSING_MPU_WRAPPERS */


#endif /* 头文件保护：MPU_WRAPPERS_H */
