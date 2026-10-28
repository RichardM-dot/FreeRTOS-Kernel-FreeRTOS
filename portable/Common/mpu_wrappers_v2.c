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
 * 用于在调用标准 FreeRTOS API 函数之前提升处理器特权级的包装函数实现。
 */

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有 API 函数
 * 重定义为使用 MPU 包装器。只有在应用程序文件中包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "stream_buffer.h"
#include "mpu_prototypes.h"
#include "mpu_syscall_numbers.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE
/*-----------------------------------------------------------*/

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    #ifndef configPROTECTED_KERNEL_OBJECT_POOL_SIZE
        #error configPROTECTED_KERNEL_OBJECT_POOL_SIZE must be defined to maximum number of kernel objects in the application.
    #endif

/**
 * @brief 在返回给用户之前添加到索引上的偏移量。
 *
 * 如果实际句柄存储在索引 i 处，则返回给用户的是
 * ( i + INDEX_OFFSET )。
 */
    #define INDEX_OFFSET    1

/**
 * @brief 内核对象的不透明类型。
 */
    struct OpaqueObject;
    typedef struct OpaqueObject * OpaqueObjectHandle_t;

/**
 * @brief 定义内核对象池中的内核对象。
 */
    typedef struct KernelObject
    {
        OpaqueObjectHandle_t xInternalObjectHandle;
        uint32_t ulKernelObjectType;
        void * pvKernelObjectData;
    } KernelObject_t;

/**
 * @brief 内核对象类型。
 */
    #define KERNEL_OBJECT_TYPE_INVALID          ( 0UL )
    #define KERNEL_OBJECT_TYPE_QUEUE            ( 1UL )
    #define KERNEL_OBJECT_TYPE_TASK             ( 2UL )
    #define KERNEL_OBJECT_TYPE_STREAM_BUFFER    ( 3UL )
    #define KERNEL_OBJECT_TYPE_EVENT_GROUP      ( 4UL )
    #define KERNEL_OBJECT_TYPE_TIMER            ( 5UL )

/**
 * @brief 检查外部索引是否有效。
 */
    #define IS_EXTERNAL_INDEX_VALID( lIndex )   \
    ( ( ( ( lIndex ) >= INDEX_OFFSET ) &&       \
        ( ( lIndex ) < ( configPROTECTED_KERNEL_OBJECT_POOL_SIZE + INDEX_OFFSET ) ) ) ? pdTRUE : pdFALSE )

/**
 * @brief 检查内部索引是否有效。
 */
    #define IS_INTERNAL_INDEX_VALID( lIndex )   \
    ( ( ( ( lIndex ) >= 0 ) &&                  \
        ( ( lIndex ) < ( configPROTECTED_KERNEL_OBJECT_POOL_SIZE ) ) ) ? pdTRUE : pdFALSE )

/**
 * @brief 将内部索引转换为外部索引。
 */
    #define CONVERT_TO_EXTERNAL_INDEX( lIndex )    ( ( lIndex ) + INDEX_OFFSET )

/**
 * @brief 将外部索引转换为内部索引。
 */
    #define CONVERT_TO_INTERNAL_INDEX( lIndex )    ( ( lIndex ) - INDEX_OFFSET )

/**
 * @brief uint32_t 类型能容纳的最大值。
 */
    #define mpuUINT32_MAX    ( ~( ( uint32_t ) 0 ) )

/**
 * @brief 检查 a 与 b 相乘是否会溢出。
 */
    #define mpuMULTIPLY_UINT32_WILL_OVERFLOW( a, b )    ( ( ( a ) > 0 ) && ( ( b ) > ( mpuUINT32_MAX / ( a ) ) ) )

/**
 * @brief 获取内核对象池中空闲槽位的索引。
 *
 * 如果找到空闲槽位，该函数会将该槽位标记为
 * “非空闲”。
 *
 * @return 如果找到空闲槽位，则返回其索引；
 *         否则返回 -1。
 */
    static int32_t MPU_GetFreeIndexInKernelObjectPool( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 将给定索引标记为内核对象池中的空闲槽位。
 *
 * @param lIndex 要设置为空闲的索引。
 */
    static void MPU_SetIndexFreeInKernelObjectPool( int32_t lIndex ) PRIVILEGED_FUNCTION;

/**
 * @brief 获取给定内核对象所存储的索引。
 *
 * @param xHandle 给定的内核对象句柄。
 * @param ulKernelObjectType 内核对象类型。
 *
 * @return 如果句柄有效，则返回内核对象存储的索引；
 *         否则返回 -1。
 */
    static int32_t MPU_GetIndexForHandle( OpaqueObjectHandle_t xHandle,
                                          uint32_t ulKernelObjectType ) PRIVILEGED_FUNCTION;

/**
 * @brief 将给定的内核对象句柄存储到内核对象池中的
 *        给定索引处。
 *
 * @param lIndex 用于存储给定句柄的索引。
 * @param xHandle 要存储的内核对象句柄。
 * @param pvKernelObjectData 与内核对象关联的数据。
 *        目前仅用于定时器对象，以存储定时器回调。
 * @param ulKernelObjectType 内核对象类型。
 */
    static void MPU_StoreHandleAndDataAtIndex( int32_t lIndex,
                                               OpaqueObjectHandle_t xHandle,
                                               void * pvKernelObjectData,
                                               uint32_t ulKernelObjectType ) PRIVILEGED_FUNCTION;

/**
 * @brief 从内核对象池中获取给定索引处的
 *        内核对象句柄。
 *
 * @param lIndex 获取内核对象句柄的索引。
 * @param ulKernelObjectType 内核对象类型。
 *
 * @return 该索引处的内核对象句柄。
 */
    static OpaqueObjectHandle_t MPU_GetHandleAtIndex( int32_t lIndex,
                                                      uint32_t ulKernelObjectType ) PRIVILEGED_FUNCTION;

    #if ( configUSE_TIMERS == 1 )

/**
 * @brief 注册为所有定时器回调的函数。
 *
 * 拦截所有定时器回调，以便使用不透明句柄
 * 调用应用程序回调。
 *
 * @param xInternalHandle 内部定时器句柄。
 */
        static void MPU_TimerCallback( TimerHandle_t xInternalHandle ) PRIVILEGED_FUNCTION;

    #endif /* 条件块结束：#if ( configUSE_TIMERS == 1 ) */

/*
 * 将所有强制转换集中在一处的包装宏。
 */
    #define MPU_StoreQueueHandleAtIndex( lIndex, xHandle )                 MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle ), NULL, KERNEL_OBJECT_TYPE_QUEUE )
    #define MPU_GetQueueHandleAtIndex( lIndex )                            ( QueueHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_QUEUE )

    #if ( configUSE_QUEUE_SETS == 1 )
        #define MPU_StoreQueueSetHandleAtIndex( lIndex, xHandle )          MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle ), NULL, KERNEL_OBJECT_TYPE_QUEUE )
        #define MPU_GetQueueSetHandleAtIndex( lIndex )                     ( QueueSetHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_QUEUE )
        #define MPU_StoreQueueSetMemberHandleAtIndex( lIndex, xHandle )    MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle ), NULL, KERNEL_OBJECT_TYPE_QUEUE )
        #define MPU_GetQueueSetMemberHandleAtIndex( lIndex )               ( QueueSetMemberHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_QUEUE )
        #define MPU_GetIndexForQueueSetMemberHandle( xHandle )             MPU_GetIndexForHandle( ( OpaqueObjectHandle_t ) ( xHandle ), KERNEL_OBJECT_TYPE_QUEUE )
    #endif

/*
 * 任务 API 将所有强制转换集中在一处的包装宏。
 */
    #define MPU_StoreTaskHandleAtIndex( lIndex, xHandle )            MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle ), NULL, KERNEL_OBJECT_TYPE_TASK )
    #define MPU_GetTaskHandleAtIndex( lIndex )                       ( TaskHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_TASK )
    #define MPU_GetIndexForTaskHandle( xHandle )                     MPU_GetIndexForHandle( ( OpaqueObjectHandle_t ) ( xHandle ), KERNEL_OBJECT_TYPE_TASK )

    #if ( configUSE_EVENT_GROUPS == 1 )
/*
 * 事件组 API 将所有强制转换集中在一处的包装宏。
 */
        #define MPU_StoreEventGroupHandleAtIndex( lIndex, xHandle )      MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle ), NULL, KERNEL_OBJECT_TYPE_EVENT_GROUP )
        #define MPU_GetEventGroupHandleAtIndex( lIndex )                 ( EventGroupHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_EVENT_GROUP )
        #define MPU_GetIndexForEventGroupHandle( xHandle )               MPU_GetIndexForHandle( ( OpaqueObjectHandle_t ) ( xHandle ), KERNEL_OBJECT_TYPE_EVENT_GROUP )

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */

    #if ( configUSE_STREAM_BUFFERS == 1 )
/*
 * 流缓冲 API 将所有强制转换集中在一处的包装宏。
 */
        #define MPU_StoreStreamBufferHandleAtIndex( lIndex, xHandle )    MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle), NULL, KERNEL_OBJECT_TYPE_STREAM_BUFFER )
        #define MPU_GetStreamBufferHandleAtIndex( lIndex )               ( StreamBufferHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_STREAM_BUFFER )
        #define MPU_GetIndexForStreamBufferHandle( xHandle )             MPU_GetIndexForHandle( ( OpaqueObjectHandle_t ) ( xHandle ), KERNEL_OBJECT_TYPE_STREAM_BUFFER )

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */

    #if ( configUSE_TIMERS == 1 )
/*
 * 定时器 API 将所有强制转换集中在一处的包装宏。
 */
        #define MPU_StoreTimerHandleAtIndex( lIndex, xHandle, pxApplicationCallback )    MPU_StoreHandleAndDataAtIndex( ( lIndex ), ( OpaqueObjectHandle_t ) ( xHandle ), ( void * ) ( pxApplicationCallback ), KERNEL_OBJECT_TYPE_TIMER )
        #define MPU_GetTimerHandleAtIndex( lIndex )                                      ( TimerHandle_t ) MPU_GetHandleAtIndex( ( lIndex ), KERNEL_OBJECT_TYPE_TIMER )
        #define MPU_GetIndexForTimerHandle( xHandle )                                    MPU_GetIndexForHandle( ( OpaqueObjectHandle_t ) ( xHandle ), KERNEL_OBJECT_TYPE_TIMER )

    #endif /* 条件块结束：#if ( configUSE_TIMERS == 1 ) */

/*-----------------------------------------------------------*/

/**
 * @brief 内核对象池。
 */
    PRIVILEGED_DATA static KernelObject_t xKernelObjectPool[ configPROTECTED_KERNEL_OBJECT_POOL_SIZE ] = { 0 };
/*-----------------------------------------------------------*/

    static int32_t MPU_GetFreeIndexInKernelObjectPool( void ) /* 特权函数 */
    {
        int32_t i, lFreeIndex = -1;

        /*
         * 该函数仅由资源创建 API 调用，这些 API 不应
         * 在 ISR 中调用。因此只需挂起调度器，
         * 无需进入临界区。
         */
        vTaskSuspendAll();
        {
            for( i = 0; i < configPROTECTED_KERNEL_OBJECT_POOL_SIZE; i++ )
            {
                if( xKernelObjectPool[ i ].xInternalObjectHandle == NULL )
                {
                    /* 将该索引标记为非空闲。 */
                    xKernelObjectPool[ i ].xInternalObjectHandle = ( OpaqueObjectHandle_t ) ( ~0U );
                    lFreeIndex = i;
                    break;
                }
            }
        }
        ( void ) xTaskResumeAll();

        return lFreeIndex;
    }
/*-----------------------------------------------------------*/

    static void MPU_SetIndexFreeInKernelObjectPool( int32_t lIndex ) /* 特权函数 */
    {
        configASSERT( IS_INTERNAL_INDEX_VALID( lIndex ) != pdFALSE );

        taskENTER_CRITICAL();
        {
            xKernelObjectPool[ lIndex ].xInternalObjectHandle = NULL;
            xKernelObjectPool[ lIndex ].ulKernelObjectType = KERNEL_OBJECT_TYPE_INVALID;
            xKernelObjectPool[ lIndex ].pvKernelObjectData = NULL;
        }
        taskEXIT_CRITICAL();
    }
/*-----------------------------------------------------------*/

    static int32_t MPU_GetIndexForHandle( OpaqueObjectHandle_t xHandle,
                                          uint32_t ulKernelObjectType ) /* 特权函数 */
    {
        int32_t i, lIndex = -1;

        configASSERT( xHandle != NULL );

        for( i = 0; i < configPROTECTED_KERNEL_OBJECT_POOL_SIZE; i++ )
        {
            if( ( xKernelObjectPool[ i ].xInternalObjectHandle == xHandle ) &&
                ( xKernelObjectPool[ i ].ulKernelObjectType == ulKernelObjectType ) )
            {
                lIndex = i;
                break;
            }
        }

        return lIndex;
    }
/*-----------------------------------------------------------*/

    static void MPU_StoreHandleAndDataAtIndex( int32_t lIndex,
                                               OpaqueObjectHandle_t xHandle,
                                               void * pvKernelObjectData,
                                               uint32_t ulKernelObjectType ) /* 特权函数 */
    {
        configASSERT( IS_INTERNAL_INDEX_VALID( lIndex ) != pdFALSE );
        xKernelObjectPool[ lIndex ].xInternalObjectHandle = xHandle;
        xKernelObjectPool[ lIndex ].ulKernelObjectType = ulKernelObjectType;
        xKernelObjectPool[ lIndex ].pvKernelObjectData = pvKernelObjectData;
    }
/*-----------------------------------------------------------*/

    static OpaqueObjectHandle_t MPU_GetHandleAtIndex( int32_t lIndex,
                                                      uint32_t ulKernelObjectType ) /* 特权函数 */
    {
        OpaqueObjectHandle_t xObjectHandle = NULL;

        configASSERT( IS_INTERNAL_INDEX_VALID( lIndex ) != pdFALSE );

        if( xKernelObjectPool[ lIndex ].ulKernelObjectType == ulKernelObjectType )
        {
            xObjectHandle = xKernelObjectPool[ lIndex ].xInternalObjectHandle;
        }

        return xObjectHandle;
    }
/*-----------------------------------------------------------*/

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        void vGrantAccessToKernelObject( TaskHandle_t xExternalTaskHandle,
                                         int32_t lExternalKernelObjectHandle ) /* 特权函数 */
        {
            int32_t lExternalTaskIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( IS_EXTERNAL_INDEX_VALID( lExternalKernelObjectHandle ) != pdFALSE )
            {
                if( xExternalTaskHandle == NULL )
                {
                    vPortGrantAccessToKernelObject( xExternalTaskHandle, CONVERT_TO_INTERNAL_INDEX( lExternalKernelObjectHandle ) );
                }
                else
                {
                    lExternalTaskIndex = ( int32_t ) xExternalTaskHandle;

                    if( IS_EXTERNAL_INDEX_VALID( lExternalTaskIndex ) != pdFALSE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lExternalTaskIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            vPortGrantAccessToKernelObject( xInternalTaskHandle,
                                                            CONVERT_TO_INTERNAL_INDEX( lExternalKernelObjectHandle ) );
                        }
                    }
                }
            }
        }

    #endif /* 条件块结束：#if ( configENABLE_ACCESS_CONTROL_LIST == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        void vRevokeAccessToKernelObject( TaskHandle_t xExternalTaskHandle,
                                          int32_t lExternalKernelObjectHandle ) /* 特权函数 */
        {
            int32_t lExternalTaskIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( IS_EXTERNAL_INDEX_VALID( lExternalKernelObjectHandle ) != pdFALSE )
            {
                if( xExternalTaskHandle == NULL )
                {
                    vPortRevokeAccessToKernelObject( xExternalTaskHandle, CONVERT_TO_INTERNAL_INDEX( lExternalKernelObjectHandle ) );
                }
                else
                {
                    lExternalTaskIndex = ( int32_t ) xExternalTaskHandle;

                    if( IS_EXTERNAL_INDEX_VALID( lExternalTaskIndex ) != pdFALSE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lExternalTaskIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            vPortRevokeAccessToKernelObject( xInternalTaskHandle,
                                                             CONVERT_TO_INTERNAL_INDEX( lExternalKernelObjectHandle ) );
                        }
                    }
                }
            }
        }

    #endif /* 条件块结束：#if ( configENABLE_ACCESS_CONTROL_LIST == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        static void MPU_TimerCallback( TimerHandle_t xInternalHandle ) /* 特权函数 */
        {
            int32_t i, lIndex = -1;
            TimerHandle_t xExternalHandle = NULL;
            TimerCallbackFunction_t pxApplicationCallBack = NULL;

            /* 来自定时器任务，因此应当是有效的。 */
            configASSERT( xInternalHandle != NULL );

            for( i = 0; i < configPROTECTED_KERNEL_OBJECT_POOL_SIZE; i++ )
            {
                if( ( ( TimerHandle_t ) xKernelObjectPool[ i ].xInternalObjectHandle == xInternalHandle ) &&
                    ( xKernelObjectPool[ i ].ulKernelObjectType == KERNEL_OBJECT_TYPE_TIMER ) )
                {
                    lIndex = i;
                    break;
                }
            }

            configASSERT( lIndex != -1 );
            xExternalHandle = ( TimerHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );

            pxApplicationCallBack = ( TimerCallbackFunction_t ) xKernelObjectPool[ lIndex ].pvKernelObjectData;
            pxApplicationCallBack( xExternalHandle );
        }

    #endif /* 条件块结束：#if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* 任务 API 的 MPU 包装器。 */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTaskDelayUntil == 1 )

        BaseType_t MPU_xTaskDelayUntilImpl( TickType_t * const pxPreviousWakeTime,
                                            TickType_t xTimeIncrement ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTaskDelayUntilImpl( TickType_t * const pxPreviousWakeTime,
                                            TickType_t xTimeIncrement ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            BaseType_t xIsPreviousWakeTimeAccessible = pdFALSE;

            if( ( pxPreviousWakeTime != NULL ) && ( xTimeIncrement > 0U ) )
            {
                xIsPreviousWakeTimeAccessible = xPortIsAuthorizedToAccessBuffer( pxPreviousWakeTime,
                                                                                 sizeof( TickType_t ),
                                                                                 ( tskMPU_WRITE_PERMISSION | tskMPU_READ_PERMISSION ) );

                if( xIsPreviousWakeTimeAccessible == pdTRUE )
                {
                    xReturn = xTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_xTaskDelayUntil == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTaskAbortDelay == 1 )

        BaseType_t MPU_xTaskAbortDelayImpl( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTaskAbortDelayImpl( TaskHandle_t xTask ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xTask;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        xReturn = xTaskAbortDelay( xInternalTaskHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_xTaskAbortDelay == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_vTaskDelay == 1 )

        void MPU_vTaskDelayImpl( TickType_t xTicksToDelay ) PRIVILEGED_FUNCTION;

        void MPU_vTaskDelayImpl( TickType_t xTicksToDelay ) /* 特权函数 */
        {
            vTaskDelay( xTicksToDelay );
        }

    #endif /* 条件块结束：if ( INCLUDE_vTaskDelay == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_uxTaskPriorityGet == 1 )

        UBaseType_t MPU_uxTaskPriorityGetImpl( const TaskHandle_t pxTask ) PRIVILEGED_FUNCTION;

        UBaseType_t MPU_uxTaskPriorityGetImpl( const TaskHandle_t pxTask ) /* 特权函数 */
        {
            UBaseType_t uxReturn = configMAX_PRIORITIES;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( pxTask == NULL )
            {
                uxReturn = uxTaskPriorityGet( pxTask );
            }
            else
            {
                lIndex = ( int32_t ) pxTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            uxReturn = uxTaskPriorityGet( xInternalTaskHandle );
                        }
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_uxTaskPriorityGet == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_eTaskGetState == 1 )

        eTaskState MPU_eTaskGetStateImpl( TaskHandle_t pxTask ) PRIVILEGED_FUNCTION;

        eTaskState MPU_eTaskGetStateImpl( TaskHandle_t pxTask ) /* 特权函数 */
        {
            eTaskState eReturn = eInvalid;
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            lIndex = ( int32_t ) pxTask;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        eReturn = eTaskGetState( xInternalTaskHandle );
                    }
                }
            }

            return eReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_eTaskGetState == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

        void MPU_vTaskGetInfoImpl( TaskHandle_t xTask,
                                   TaskStatus_t * pxTaskStatus,
                                   BaseType_t xGetFreeStackSpace,
                                   eTaskState eState ) PRIVILEGED_FUNCTION;

        void MPU_vTaskGetInfoImpl( TaskHandle_t xTask,
                                   TaskStatus_t * pxTaskStatus,
                                   BaseType_t xGetFreeStackSpace,
                                   eTaskState eState ) /* 特权函数 */
        {
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xIsTaskStatusWriteable = pdFALSE;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            xIsTaskStatusWriteable = xPortIsAuthorizedToAccessBuffer( pxTaskStatus,
                                                                      sizeof( TaskStatus_t ),
                                                                      tskMPU_WRITE_PERMISSION );

            if( xIsTaskStatusWriteable == pdTRUE )
            {
                if( xTask == NULL )
                {
                    vTaskGetInfo( xTask, pxTaskStatus, xGetFreeStackSpace, eState );
                }
                else
                {
                    lIndex = ( int32_t ) xTask;

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                        {
                            xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalTaskHandle != NULL )
                            {
                                vTaskGetInfo( xInternalTaskHandle, pxTaskStatus, xGetFreeStackSpace, eState );
                            }
                        }
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( configUSE_TRACE_FACILITY == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )

        TaskHandle_t MPU_xTaskGetIdleTaskHandleImpl( void ) PRIVILEGED_FUNCTION;

        TaskHandle_t MPU_xTaskGetIdleTaskHandleImpl( void ) /* 特权函数 */
        {
            TaskHandle_t xIdleTaskHandle = NULL;

            xIdleTaskHandle = xTaskGetIdleTaskHandle();

            return xIdleTaskHandle;
        }

    #endif /* 条件块结束：if ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_vTaskSuspend == 1 )

        void MPU_vTaskSuspendImpl( TaskHandle_t pxTaskToSuspend ) PRIVILEGED_FUNCTION;

        void MPU_vTaskSuspendImpl( TaskHandle_t pxTaskToSuspend ) /* 特权函数 */
        {
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( pxTaskToSuspend == NULL )
            {
                vTaskSuspend( pxTaskToSuspend );
            }
            else
            {
                /*
                 * 调度器启动后，只有特权任务才能挂起其他任务。
                 */
                #if ( INCLUDE_xTaskGetSchedulerState == 1 )
                    if( ( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED ) || ( portIS_TASK_PRIVILEGED() == pdTRUE ) )
                #else
                    if( portIS_TASK_PRIVILEGED() == pdTRUE )
                #endif
                {
                    lIndex = ( int32_t ) pxTaskToSuspend;

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                        {
                            xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalTaskHandle != NULL )
                            {
                                vTaskSuspend( xInternalTaskHandle );
                            }
                        }
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( INCLUDE_vTaskSuspend == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_vTaskSuspend == 1 )

        void MPU_vTaskResumeImpl( TaskHandle_t pxTaskToResume ) PRIVILEGED_FUNCTION;

        void MPU_vTaskResumeImpl( TaskHandle_t pxTaskToResume ) /* 特权函数 */
        {
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            lIndex = ( int32_t ) pxTaskToResume;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        vTaskResume( xInternalTaskHandle );
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( INCLUDE_vTaskSuspend == 1 ) */
/*-----------------------------------------------------------*/

    TickType_t MPU_xTaskGetTickCountImpl( void ) PRIVILEGED_FUNCTION;

    TickType_t MPU_xTaskGetTickCountImpl( void ) /* 特权函数 */
    {
        TickType_t xReturn;

        xReturn = xTaskGetTickCount();

        return xReturn;
    }
/*-----------------------------------------------------------*/

    UBaseType_t MPU_uxTaskGetNumberOfTasksImpl( void ) PRIVILEGED_FUNCTION;

    UBaseType_t MPU_uxTaskGetNumberOfTasksImpl( void ) /* 特权函数 */
    {
        UBaseType_t uxReturn;

        uxReturn = uxTaskGetNumberOfTasks();

        return uxReturn;
    }
/*-----------------------------------------------------------*/

    #if ( configGENERATE_RUN_TIME_STATS == 1 )

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetRunTimeCounterImpl( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetRunTimeCounterImpl( const TaskHandle_t xTask ) /* 特权函数 */
        {
            configRUN_TIME_COUNTER_TYPE xReturn = 0;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTask == NULL )
            {
                xReturn = ulTaskGetRunTimeCounter( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            xReturn = ulTaskGetRunTimeCounter( xInternalTaskHandle );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( ( configGENERATE_RUN_TIME_STATS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configGENERATE_RUN_TIME_STATS == 1 )

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetRunTimePercentImpl( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetRunTimePercentImpl( const TaskHandle_t xTask ) /* 特权函数 */
        {
            configRUN_TIME_COUNTER_TYPE xReturn = 0;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTask == NULL )
            {
                xReturn = ulTaskGetRunTimePercent( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            xReturn = ulTaskGetRunTimePercent( xInternalTaskHandle );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( ( configGENERATE_RUN_TIME_STATS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetIdleRunTimePercentImpl( void ) PRIVILEGED_FUNCTION;

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetIdleRunTimePercentImpl( void ) /* 特权函数 */
        {
            configRUN_TIME_COUNTER_TYPE xReturn;

            xReturn = ulTaskGetIdleRunTimePercent();

            return xReturn;
        }

    #endif /* 条件块结束：if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetIdleRunTimeCounterImpl( void ) PRIVILEGED_FUNCTION;

        configRUN_TIME_COUNTER_TYPE MPU_ulTaskGetIdleRunTimeCounterImpl( void ) /* 特权函数 */
        {
            configRUN_TIME_COUNTER_TYPE xReturn;

            xReturn = ulTaskGetIdleRunTimeCounter();

            return xReturn;
        }

    #endif /* 条件块结束：if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_APPLICATION_TASK_TAG == 1 )

        void MPU_vTaskSetApplicationTaskTagImpl( TaskHandle_t xTask,
                                                 TaskHookFunction_t pxTagValue ) PRIVILEGED_FUNCTION;

        void MPU_vTaskSetApplicationTaskTagImpl( TaskHandle_t xTask,
                                                 TaskHookFunction_t pxTagValue ) /* 特权函数 */
        {
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTask == NULL )
            {
                vTaskSetApplicationTaskTag( xTask, pxTagValue );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            vTaskSetApplicationTaskTag( xInternalTaskHandle, pxTagValue );
                        }
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( configUSE_APPLICATION_TASK_TAG == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_APPLICATION_TASK_TAG == 1 )

        TaskHookFunction_t MPU_xTaskGetApplicationTaskTagImpl( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

        TaskHookFunction_t MPU_xTaskGetApplicationTaskTagImpl( TaskHandle_t xTask ) /* 特权函数 */
        {
            TaskHookFunction_t xReturn = NULL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTask == NULL )
            {
                xReturn = xTaskGetApplicationTaskTag( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            xReturn = xTaskGetApplicationTaskTag( xInternalTaskHandle );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_APPLICATION_TASK_TAG == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

        void MPU_vTaskSetThreadLocalStoragePointerImpl( TaskHandle_t xTaskToSet,
                                                        BaseType_t xIndex,
                                                        void * pvValue ) PRIVILEGED_FUNCTION;

        void MPU_vTaskSetThreadLocalStoragePointerImpl( TaskHandle_t xTaskToSet,
                                                        BaseType_t xIndex,
                                                        void * pvValue ) /* 特权函数 */
        {
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTaskToSet == NULL )
            {
                vTaskSetThreadLocalStoragePointer( xTaskToSet, xIndex, pvValue );
            }
            else
            {
                lIndex = ( int32_t ) xTaskToSet;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            vTaskSetThreadLocalStoragePointer( xInternalTaskHandle, xIndex, pvValue );
                        }
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 ) */
/*-----------------------------------------------------------*/

    #if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

        void * MPU_pvTaskGetThreadLocalStoragePointerImpl( TaskHandle_t xTaskToQuery,
                                                           BaseType_t xIndex ) PRIVILEGED_FUNCTION;

        void * MPU_pvTaskGetThreadLocalStoragePointerImpl( TaskHandle_t xTaskToQuery,
                                                           BaseType_t xIndex ) /* 特权函数 */
        {
            void * pvReturn = NULL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTaskToQuery == NULL )
            {
                pvReturn = pvTaskGetThreadLocalStoragePointer( xTaskToQuery, xIndex );
            }
            else
            {
                lIndex = ( int32_t ) xTaskToQuery;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            pvReturn = pvTaskGetThreadLocalStoragePointer( xInternalTaskHandle, xIndex );
                        }
                    }
                }
            }

            return pvReturn;
        }

    #endif /* 条件块结束：if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TRACE_FACILITY == 1 )

        UBaseType_t MPU_uxTaskGetSystemStateImpl( TaskStatus_t * pxTaskStatusArray,
                                                  UBaseType_t uxArraySize,
                                                  configRUN_TIME_COUNTER_TYPE * pulTotalRunTime ) PRIVILEGED_FUNCTION;

        UBaseType_t MPU_uxTaskGetSystemStateImpl( TaskStatus_t * pxTaskStatusArray,
                                                  UBaseType_t uxArraySize,
                                                  configRUN_TIME_COUNTER_TYPE * pulTotalRunTime ) /* 特权函数 */
        {
            UBaseType_t uxReturn = 0;
            BaseType_t xIsTaskStatusArrayWriteable = pdFALSE;
            BaseType_t xIsTotalRunTimeWriteable = pdFALSE;
            uint32_t ulArraySize = ( uint32_t ) uxArraySize;
            uint32_t ulTaskStatusSize = ( uint32_t ) sizeof( TaskStatus_t );

            if( mpuMULTIPLY_UINT32_WILL_OVERFLOW( ulTaskStatusSize, ulArraySize ) == 0 )
            {
                xIsTaskStatusArrayWriteable = xPortIsAuthorizedToAccessBuffer( pxTaskStatusArray,
                                                                               ulTaskStatusSize * ulArraySize,
                                                                               tskMPU_WRITE_PERMISSION );

                if( pulTotalRunTime != NULL )
                {
                    xIsTotalRunTimeWriteable = xPortIsAuthorizedToAccessBuffer( pulTotalRunTime,
                                                                                sizeof( configRUN_TIME_COUNTER_TYPE ),
                                                                                tskMPU_WRITE_PERMISSION );
                }

                if( ( xIsTaskStatusArrayWriteable == pdTRUE ) &&
                    ( ( pulTotalRunTime == NULL ) || ( xIsTotalRunTimeWriteable == pdTRUE ) ) )
                {
                    uxReturn = uxTaskGetSystemState( pxTaskStatusArray, ( UBaseType_t ) ulArraySize, pulTotalRunTime );
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TRACE_FACILITY == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

        UBaseType_t MPU_uxTaskGetStackHighWaterMarkImpl( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

        UBaseType_t MPU_uxTaskGetStackHighWaterMarkImpl( TaskHandle_t xTask ) /* 特权函数 */
        {
            UBaseType_t uxReturn = 0;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTask == NULL )
            {
                uxReturn = uxTaskGetStackHighWaterMark( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            uxReturn = uxTaskGetStackHighWaterMark( xInternalTaskHandle );
                        }
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )

        configSTACK_DEPTH_TYPE MPU_uxTaskGetStackHighWaterMark2Impl( TaskHandle_t xTask ) PRIVILEGED_FUNCTION;

        configSTACK_DEPTH_TYPE MPU_uxTaskGetStackHighWaterMark2Impl( TaskHandle_t xTask ) /* 特权函数 */
        {
            configSTACK_DEPTH_TYPE uxReturn = 0;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( xTask == NULL )
            {
                uxReturn = uxTaskGetStackHighWaterMark2( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                    {
                        xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalTaskHandle != NULL )
                        {
                            uxReturn = uxTaskGetStackHighWaterMark2( xInternalTaskHandle );
                        }
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) */
/*-----------------------------------------------------------*/

    #if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_RECURSIVE_MUTEXES == 1 ) )

        TaskHandle_t MPU_xTaskGetCurrentTaskHandleImpl( void ) PRIVILEGED_FUNCTION;

        TaskHandle_t MPU_xTaskGetCurrentTaskHandleImpl( void ) /* 特权函数 */
        {
            TaskHandle_t xInternalTaskHandle = NULL;
            TaskHandle_t xExternalTaskHandle = NULL;
            int32_t lIndex;

            xInternalTaskHandle = xTaskGetCurrentTaskHandle();

            if( xInternalTaskHandle != NULL )
            {
                lIndex = MPU_GetIndexForTaskHandle( xInternalTaskHandle );

                if( lIndex != -1 )
                {
                    xExternalTaskHandle = ( TaskHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
            }

            return xExternalTaskHandle;
        }

    #endif /* 条件块结束：if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_RECURSIVE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTaskGetSchedulerState == 1 )

        BaseType_t MPU_xTaskGetSchedulerStateImpl( void ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTaskGetSchedulerStateImpl( void ) /* 特权函数 */
        {
            BaseType_t xReturn = taskSCHEDULER_NOT_STARTED;

            xReturn = xTaskGetSchedulerState();

            return xReturn;
        }

    #endif /* 条件块结束：if ( INCLUDE_xTaskGetSchedulerState == 1 ) */
/*-----------------------------------------------------------*/

    void MPU_vTaskSetTimeOutStateImpl( TimeOut_t * const pxTimeOut ) PRIVILEGED_FUNCTION;

    void MPU_vTaskSetTimeOutStateImpl( TimeOut_t * const pxTimeOut ) /* 特权函数 */
    {
        BaseType_t xIsTimeOutWriteable = pdFALSE;

        if( pxTimeOut != NULL )
        {
            xIsTimeOutWriteable = xPortIsAuthorizedToAccessBuffer( pxTimeOut,
                                                                   sizeof( TimeOut_t ),
                                                                   tskMPU_WRITE_PERMISSION );

            if( xIsTimeOutWriteable == pdTRUE )
            {
                vTaskSetTimeOutState( pxTimeOut );
            }
        }
    }
/*-----------------------------------------------------------*/

    BaseType_t MPU_xTaskCheckForTimeOutImpl( TimeOut_t * const pxTimeOut,
                                             TickType_t * const pxTicksToWait ) PRIVILEGED_FUNCTION;

    BaseType_t MPU_xTaskCheckForTimeOutImpl( TimeOut_t * const pxTimeOut,
                                             TickType_t * const pxTicksToWait ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFALSE;
        BaseType_t xIsTimeOutWriteable = pdFALSE;
        BaseType_t xIsTicksToWaitWriteable = pdFALSE;

        if( ( pxTimeOut != NULL ) && ( pxTicksToWait != NULL ) )
        {
            xIsTimeOutWriteable = xPortIsAuthorizedToAccessBuffer( pxTimeOut,
                                                                   sizeof( TimeOut_t ),
                                                                   tskMPU_WRITE_PERMISSION );
            xIsTicksToWaitWriteable = xPortIsAuthorizedToAccessBuffer( pxTicksToWait,
                                                                       sizeof( TickType_t ),
                                                                       tskMPU_WRITE_PERMISSION );

            if( ( xIsTimeOutWriteable == pdTRUE ) && ( xIsTicksToWaitWriteable == pdTRUE ) )
            {
                xReturn = xTaskCheckForTimeOut( pxTimeOut, pxTicksToWait );
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        BaseType_t MPU_xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                                           UBaseType_t uxIndexToNotify,
                                           uint32_t ulValue,
                                           eNotifyAction eAction,
                                           uint32_t * pulPreviousNotificationValue ) /* FREERTOS 系统调用 */
        {
            BaseType_t xReturn = pdFAIL;
            xTaskGenericNotifyParams_t xParams;

            xParams.xTaskToNotify = xTaskToNotify;
            xParams.uxIndexToNotify = uxIndexToNotify;
            xParams.ulValue = ulValue;
            xParams.eAction = eAction;
            xParams.pulPreviousNotificationValue = pulPreviousNotificationValue;

            xReturn = MPU_xTaskGenericNotifyEntry( &( xParams ) );

            return xReturn;
        }

        BaseType_t MPU_xTaskGenericNotifyImpl( const xTaskGenericNotifyParams_t * pxParams ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTaskGenericNotifyImpl( const xTaskGenericNotifyParams_t * pxParams ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xIsPreviousNotificationValueWriteable = pdFALSE;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;
            BaseType_t xAreParamsReadable = pdFALSE;

            if( pxParams != NULL )
            {
                xAreParamsReadable = xPortIsAuthorizedToAccessBuffer( pxParams,
                                                                      sizeof( xTaskGenericNotifyParams_t ),
                                                                      tskMPU_READ_PERMISSION );
            }

            if( xAreParamsReadable == pdTRUE )
            {
                if( ( pxParams->uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES ) &&
                    ( ( pxParams->eAction == eNoAction ) ||
                      ( pxParams->eAction == eSetBits ) ||
                      ( pxParams->eAction == eIncrement ) ||
                      ( pxParams->eAction == eSetValueWithOverwrite ) ||
                      ( pxParams->eAction == eSetValueWithoutOverwrite ) ) )
                {
                    if( pxParams->pulPreviousNotificationValue != NULL )
                    {
                        xIsPreviousNotificationValueWriteable = xPortIsAuthorizedToAccessBuffer( pxParams->pulPreviousNotificationValue,
                                                                                                 sizeof( uint32_t ),
                                                                                                 tskMPU_WRITE_PERMISSION );
                    }

                    if( ( pxParams->pulPreviousNotificationValue == NULL ) ||
                        ( xIsPreviousNotificationValueWriteable == pdTRUE ) )
                    {
                        lIndex = ( int32_t ) ( pxParams->xTaskToNotify );

                        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                        {
                            xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                            {
                                xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                                if( xInternalTaskHandle != NULL )
                                {
                                    xReturn = xTaskGenericNotify( xInternalTaskHandle,
                                                                  pxParams->uxIndexToNotify,
                                                                  pxParams->ulValue,
                                                                  pxParams->eAction,
                                                                  pxParams->pulPreviousNotificationValue );
                                }
                            }
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        BaseType_t MPU_xTaskGenericNotifyWait( UBaseType_t uxIndexToWaitOn,
                                               uint32_t ulBitsToClearOnEntry,
                                               uint32_t ulBitsToClearOnExit,
                                               uint32_t * pulNotificationValue,
                                               TickType_t xTicksToWait )
        {
            BaseType_t xReturn = pdFAIL;
            xTaskGenericNotifyWaitParams_t xParams;

            xParams.uxIndexToWaitOn = uxIndexToWaitOn;
            xParams.ulBitsToClearOnEntry = ulBitsToClearOnEntry;
            xParams.ulBitsToClearOnExit = ulBitsToClearOnExit;
            xParams.pulNotificationValue = pulNotificationValue;
            xParams.xTicksToWait = xTicksToWait;

            xReturn = MPU_xTaskGenericNotifyWaitEntry( &( xParams ) );

            return xReturn;
        }

        BaseType_t MPU_xTaskGenericNotifyWaitImpl( const xTaskGenericNotifyWaitParams_t * pxParams ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTaskGenericNotifyWaitImpl( const xTaskGenericNotifyWaitParams_t * pxParams ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            BaseType_t xIsNotificationValueWritable = pdFALSE;
            BaseType_t xAreParamsReadable = pdFALSE;

            if( pxParams != NULL )
            {
                xAreParamsReadable = xPortIsAuthorizedToAccessBuffer( pxParams,
                                                                      sizeof( xTaskGenericNotifyWaitParams_t ),
                                                                      tskMPU_READ_PERMISSION );
            }

            if( xAreParamsReadable == pdTRUE )
            {
                if( pxParams->uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES )
                {
                    if( pxParams->pulNotificationValue != NULL )
                    {
                        xIsNotificationValueWritable = xPortIsAuthorizedToAccessBuffer( pxParams->pulNotificationValue,
                                                                                        sizeof( uint32_t ),
                                                                                        tskMPU_WRITE_PERMISSION );
                    }

                    if( ( pxParams->pulNotificationValue == NULL ) ||
                        ( xIsNotificationValueWritable == pdTRUE ) )
                    {
                        xReturn = xTaskGenericNotifyWait( pxParams->uxIndexToWaitOn,
                                                          pxParams->ulBitsToClearOnEntry,
                                                          pxParams->ulBitsToClearOnExit,
                                                          pxParams->pulNotificationValue,
                                                          pxParams->xTicksToWait );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        uint32_t MPU_ulTaskGenericNotifyTakeImpl( UBaseType_t uxIndexToWaitOn,
                                                  BaseType_t xClearCountOnExit,
                                                  TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

        uint32_t MPU_ulTaskGenericNotifyTakeImpl( UBaseType_t uxIndexToWaitOn,
                                                  BaseType_t xClearCountOnExit,
                                                  TickType_t xTicksToWait ) /* 特权函数 */
        {
            uint32_t ulReturn = 0;

            if( uxIndexToWaitOn < configTASK_NOTIFICATION_ARRAY_ENTRIES )
            {
                ulReturn = ulTaskGenericNotifyTake( uxIndexToWaitOn, xClearCountOnExit, xTicksToWait );
            }

            return ulReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        BaseType_t MPU_xTaskGenericNotifyStateClearImpl( TaskHandle_t xTask,
                                                         UBaseType_t uxIndexToClear ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTaskGenericNotifyStateClearImpl( TaskHandle_t xTask,
                                                         UBaseType_t uxIndexToClear ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES )
            {
                if( xTask == NULL )
                {
                    xReturn = xTaskGenericNotifyStateClear( xTask, uxIndexToClear );
                }
                else
                {
                    lIndex = ( int32_t ) xTask;

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                        {
                            xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalTaskHandle != NULL )
                            {
                                xReturn = xTaskGenericNotifyStateClear( xInternalTaskHandle, uxIndexToClear );
                            }
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        uint32_t MPU_ulTaskGenericNotifyValueClearImpl( TaskHandle_t xTask,
                                                        UBaseType_t uxIndexToClear,
                                                        uint32_t ulBitsToClear ) PRIVILEGED_FUNCTION;

        uint32_t MPU_ulTaskGenericNotifyValueClearImpl( TaskHandle_t xTask,
                                                        UBaseType_t uxIndexToClear,
                                                        uint32_t ulBitsToClear ) /* 特权函数 */
        {
            uint32_t ulReturn = 0;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessTask = pdFALSE;

            if( uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES )
            {
                if( xTask == NULL )
                {
                    ulReturn = ulTaskGenericNotifyValueClear( xTask, uxIndexToClear, ulBitsToClear );
                }
                else
                {
                    lIndex = ( int32_t ) xTask;

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessTask = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessTask == pdTRUE )
                        {
                            xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalTaskHandle != NULL )
                            {
                                ulReturn = ulTaskGenericNotifyValueClear( xInternalTaskHandle, uxIndexToClear, ulBitsToClear );
                            }
                        }
                    }
                }
            }

            return ulReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*-----------------------------------------------------------*/

/*
 * 任务 API 的仅特权包装函数。需要这些包装函数，
 * 以便应用程序在所有 API 中使用由 mpu_wrappers.c 维护的
 * 不透明句柄。
 */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

        BaseType_t MPU_xTaskCreate( TaskFunction_t pvTaskCode,
                                    const char * const pcName,
                                    const configSTACK_DEPTH_TYPE uxStackDepth,
                                    void * pvParameters,
                                    UBaseType_t uxPriority,
                                    TaskHandle_t * pxCreatedTask ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                /* 在 MPU 移植层中，xTaskCreate() 只能用于创建特权任务。 */
                if( ( uxPriority & portPRIVILEGE_BIT ) != 0 )
                {
                    xReturn = xTaskCreate( pvTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, &( xInternalTaskHandle ) );

                    if( ( xReturn == pdPASS ) && ( xInternalTaskHandle != NULL ) )
                    {
                        MPU_StoreTaskHandleAtIndex( lIndex, xInternalTaskHandle );

                        if( pxCreatedTask != NULL )
                        {
                            *pxCreatedTask = ( TaskHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                        }
                    }
                    else
                    {
                        MPU_SetIndexFreeInKernelObjectPool( lIndex );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 配置项 configSUPPORT_DYNAMIC_ALLOCATION */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

        TaskHandle_t MPU_xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                            const char * const pcName,
                                            const configSTACK_DEPTH_TYPE uxStackDepth,
                                            void * const pvParameters,
                                            UBaseType_t uxPriority,
                                            StackType_t * const puxStackBuffer,
                                            StaticTask_t * const pxTaskBuffer ) /* 特权函数 */
        {
            TaskHandle_t xExternalTaskHandle = NULL;
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalTaskHandle = xTaskCreateStatic( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer );

                if( xInternalTaskHandle != NULL )
                {
                    MPU_StoreTaskHandleAtIndex( lIndex, xInternalTaskHandle );

                    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
                    {
                        /* 默认情况下，非特权任务可以访问自身。 */
                        if( ( uxPriority & portPRIVILEGE_BIT ) == 0 )
                        {
                            vPortGrantAccessToKernelObject( xInternalTaskHandle, lIndex );
                        }
                    }
                    #endif

                    xExternalTaskHandle = ( TaskHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalTaskHandle;
        }

    #endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_vTaskDelete == 1 )

        void MPU_vTaskDelete( TaskHandle_t pxTaskToDelete ) /* 特权函数 */
        {
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;

            if( pxTaskToDelete == NULL )
            {
                xInternalTaskHandle = xTaskGetCurrentTaskHandle();
                lIndex = MPU_GetIndexForTaskHandle( xInternalTaskHandle );

                if( lIndex != -1 )
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }

                vTaskDelete( xInternalTaskHandle );
            }
            else
            {
                lIndex = ( int32_t ) pxTaskToDelete;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        MPU_SetIndexFreeInKernelObjectPool( CONVERT_TO_INTERNAL_INDEX( lIndex ) );
                        vTaskDelete( xInternalTaskHandle );
                    }
                }
            }
        }

    #endif /* 条件块结束：#if ( INCLUDE_vTaskDelete == 1 ) */
/*-----------------------------------------------------------*/


    #if ( INCLUDE_vTaskPrioritySet == 1 )

        void MPU_vTaskPrioritySet( TaskHandle_t pxTask,
                                   UBaseType_t uxNewPriority ) /* 特权函数 */
        {
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;

            if( pxTask == NULL )
            {
                vTaskPrioritySet( pxTask, uxNewPriority );
            }
            else
            {
                lIndex = ( int32_t ) pxTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        vTaskPrioritySet( xInternalTaskHandle, uxNewPriority );
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( INCLUDE_vTaskPrioritySet == 1 ) */
/*-----------------------------------------------------------*/

    #if ( INCLUDE_xTaskGetHandle == 1 )

        TaskHandle_t MPU_xTaskGetHandle( const char * pcNameToQuery ) /* 特权函数 */
        {
            TaskHandle_t xInternalTaskHandle = NULL;
            TaskHandle_t xExternalTaskHandle = NULL;
            int32_t lIndex;

            xInternalTaskHandle = xTaskGetHandle( pcNameToQuery );

            if( xInternalTaskHandle != NULL )
            {
                lIndex = MPU_GetIndexForTaskHandle( xInternalTaskHandle );

                if( lIndex != -1 )
                {
                    xExternalTaskHandle = ( TaskHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
            }

            return xExternalTaskHandle;
        }

    #endif /* 条件块结束：if ( INCLUDE_xTaskGetHandle == 1 ) */
/*-----------------------------------------------------------*/


    #if ( configUSE_APPLICATION_TASK_TAG == 1 )

        BaseType_t MPU_xTaskCallApplicationTaskHook( TaskHandle_t xTask,
                                                     void * pvParameter ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( xTask == NULL )
            {
                xReturn = xTaskCallApplicationTaskHook( xTask, pvParameter );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        xReturn = xTaskCallApplicationTaskHook( xInternalTaskHandle, pvParameter );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_APPLICATION_TASK_TAG == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

        BaseType_t MPU_xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
                                              TaskHandle_t * pxCreatedTask ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xReturn = xTaskCreateRestricted( pxTaskDefinition, &( xInternalTaskHandle ) );

                if( ( xReturn == pdPASS ) && ( xInternalTaskHandle != NULL ) )
                {
                    MPU_StoreTaskHandleAtIndex( lIndex, xInternalTaskHandle );

                    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
                    {
                        /* 默认情况下，非特权任务可以访问自身。 */
                        if( ( pxTaskDefinition->uxPriority & portPRIVILEGE_BIT ) == 0 )
                        {
                            vPortGrantAccessToKernelObject( xInternalTaskHandle, lIndex );
                        }
                    }
                    #endif

                    if( pxCreatedTask != NULL )
                    {
                        *pxCreatedTask = ( TaskHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                    }
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xReturn;
        }

    #endif /* 配置项 configSUPPORT_DYNAMIC_ALLOCATION */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

        BaseType_t MPU_xTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
                                                    TaskHandle_t * pxCreatedTask ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xReturn = xTaskCreateRestrictedStatic( pxTaskDefinition, &( xInternalTaskHandle ) );

                if( ( xReturn == pdPASS ) && ( xInternalTaskHandle != NULL ) )
                {
                    MPU_StoreTaskHandleAtIndex( lIndex, xInternalTaskHandle );

                    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
                    {
                        /* 默认情况下，非特权任务可以访问自身。 */
                        if( ( pxTaskDefinition->uxPriority & portPRIVILEGE_BIT ) == 0 )
                        {
                            vPortGrantAccessToKernelObject( xInternalTaskHandle, lIndex );
                        }
                    }
                    #endif

                    if( pxCreatedTask != NULL )
                    {
                        *pxCreatedTask = ( TaskHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                    }
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xReturn;
        }

    #endif /* 配置项 configSUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

    void MPU_vTaskAllocateMPURegions( TaskHandle_t xTaskToModify,
                                      const MemoryRegion_t * const xRegions ) /* 特权函数 */
    {
        TaskHandle_t xInternalTaskHandle = NULL;
        int32_t lIndex;

        if( xTaskToModify == NULL )
        {
            vTaskAllocateMPURegions( xTaskToModify, xRegions );
        }
        else
        {
            lIndex = ( int32_t ) xTaskToModify;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTaskHandle != NULL )
                {
                    vTaskAllocateMPURegions( xInternalTaskHandle, xRegions );
                }
            }
        }
    }
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

        BaseType_t MPU_xTaskGetStaticBuffers( TaskHandle_t xTask,
                                              StackType_t ** ppuxStackBuffer,
                                              StaticTask_t ** ppxTaskBuffer ) /* 特权函数 */
        {
            TaskHandle_t xInternalTaskHandle = NULL;
            int32_t lIndex;
            BaseType_t xReturn = pdFALSE;

            if( xTask == NULL )
            {
                xInternalTaskHandle = xTaskGetCurrentTaskHandle();
                xReturn = xTaskGetStaticBuffers( xInternalTaskHandle, ppuxStackBuffer, ppxTaskBuffer );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        xReturn = xTaskGetStaticBuffers( xInternalTaskHandle, ppuxStackBuffer, ppxTaskBuffer );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
/*-----------------------------------------------------------*/

    char * MPU_pcTaskGetName( TaskHandle_t xTaskToQuery ) /* 特权函数 */
    {
        char * pcReturn = NULL;
        int32_t lIndex;
        TaskHandle_t xInternalTaskHandle = NULL;

        if( xTaskToQuery == NULL )
        {
            pcReturn = pcTaskGetName( xTaskToQuery );
        }
        else
        {
            lIndex = ( int32_t ) xTaskToQuery;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTaskHandle != NULL )
                {
                    pcReturn = pcTaskGetName( xInternalTaskHandle );
                }
            }
        }

        return pcReturn;
    }
/*-----------------------------------------------------------*/

    #if ( INCLUDE_uxTaskPriorityGet == 1 )

        UBaseType_t MPU_uxTaskPriorityGetFromISR( const TaskHandle_t xTask ) /* 特权函数 */
        {
            UBaseType_t uxReturn = configMAX_PRIORITIES;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( xTask == NULL )
            {
                uxReturn = uxTaskPriorityGetFromISR( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        uxReturn = uxTaskPriorityGetFromISR( xInternalTaskHandle );
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：#if ( INCLUDE_uxTaskPriorityGet == 1 ) */
/*-----------------------------------------------------------*/

    #if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) )

        UBaseType_t MPU_uxTaskBasePriorityGet( const TaskHandle_t xTask ) /* 特权函数 */
        {
            UBaseType_t uxReturn = configMAX_PRIORITIES;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( xTask == NULL )
            {
                uxReturn = uxTaskBasePriorityGet( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        uxReturn = uxTaskBasePriorityGet( xInternalTaskHandle );
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) )

        UBaseType_t MPU_uxTaskBasePriorityGetFromISR( const TaskHandle_t xTask ) /* 特权函数 */
        {
            UBaseType_t uxReturn = configMAX_PRIORITIES;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( xTask == NULL )
            {
                uxReturn = uxTaskBasePriorityGetFromISR( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        uxReturn = uxTaskBasePriorityGetFromISR( xInternalTaskHandle );
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：#if ( ( INCLUDE_uxTaskPriorityGet == 1 ) && ( configUSE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

        BaseType_t MPU_xTaskResumeFromISR( TaskHandle_t xTaskToResume ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            lIndex = ( int32_t ) xTaskToResume;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTaskHandle != NULL )
                {
                    xReturn = xTaskResumeFromISR( xInternalTaskHandle );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) ) */
/*---------------------------------------------------------------------------------------*/

    #if ( configUSE_APPLICATION_TASK_TAG == 1 )

        TaskHookFunction_t MPU_xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask ) /* 特权函数 */
        {
            TaskHookFunction_t xReturn = NULL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            if( xTask == NULL )
            {
                xReturn = xTaskGetApplicationTaskTagFromISR( xTask );
            }
            else
            {
                lIndex = ( int32_t ) xTask;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTaskHandle != NULL )
                    {
                        xReturn = xTaskGetApplicationTaskTagFromISR( xInternalTaskHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_APPLICATION_TASK_TAG == 1 ) */
/*---------------------------------------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        BaseType_t MPU_xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                                  UBaseType_t uxIndexToNotify,
                                                  uint32_t ulValue,
                                                  eNotifyAction eAction,
                                                  uint32_t * pulPreviousNotificationValue,
                                                  BaseType_t * pxHigherPriorityTaskWoken ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            lIndex = ( int32_t ) xTaskToNotify;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTaskHandle != NULL )
                {
                    xReturn = xTaskGenericNotifyFromISR( xInternalTaskHandle, uxIndexToNotify, ulValue, eAction, pulPreviousNotificationValue, pxHigherPriorityTaskWoken );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*---------------------------------------------------------------------------------------*/

    #if ( configUSE_TASK_NOTIFICATIONS == 1 )

        void MPU_vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                                UBaseType_t uxIndexToNotify,
                                                BaseType_t * pxHigherPriorityTaskWoken ) /* 特权函数 */
        {
            int32_t lIndex;
            TaskHandle_t xInternalTaskHandle = NULL;

            lIndex = ( int32_t ) xTaskToNotify;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTaskHandle = MPU_GetTaskHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTaskHandle != NULL )
                {
                    vTaskGenericNotifyGiveFromISR( xInternalTaskHandle, uxIndexToNotify, pxHigherPriorityTaskWoken );
                }
            }
        }
    #endif /* 条件块结束：#if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* 队列 API 的 MPU 包装器。 */
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueGenericSendImpl( QueueHandle_t xQueue,
                                          const void * const pvItemToQueue,
                                          TickType_t xTicksToWait,
                                          BaseType_t xCopyPosition ) PRIVILEGED_FUNCTION;

    BaseType_t MPU_xQueueGenericSendImpl( QueueHandle_t xQueue,
                                          const void * const pvItemToQueue,
                                          TickType_t xTicksToWait,
                                          BaseType_t xCopyPosition ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        BaseType_t xReturn = pdFAIL;
        BaseType_t xIsItemToQueueReadable = pdFALSE;
        BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;
        UBaseType_t uxQueueItemSize, uxQueueLength;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    uxQueueItemSize = uxQueueGetQueueItemSize( xInternalQueueHandle );
                    uxQueueLength = uxQueueGetQueueLength( xInternalQueueHandle );

                    if( ( !( ( pvItemToQueue == NULL ) && ( uxQueueItemSize != ( UBaseType_t ) 0U ) ) ) &&
                        ( !( ( xCopyPosition == queueOVERWRITE ) && ( uxQueueLength != ( UBaseType_t ) 1U ) ) )
                        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
                            && ( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0U ) ) )
                        #endif
                        )
                    {
                        if( pvItemToQueue != NULL )
                        {
                            xIsItemToQueueReadable = xPortIsAuthorizedToAccessBuffer( pvItemToQueue,
                                                                                      uxQueueItemSize,
                                                                                      tskMPU_READ_PERMISSION );
                        }

                        if( ( pvItemToQueue == NULL ) || ( xIsItemToQueueReadable == pdTRUE ) )
                        {
                            xReturn = xQueueGenericSend( xInternalQueueHandle, pvItemToQueue, xTicksToWait, xCopyPosition );
                        }
                    }
                }
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    UBaseType_t MPU_uxQueueMessagesWaitingImpl( const QueueHandle_t pxQueue ) PRIVILEGED_FUNCTION;

    UBaseType_t MPU_uxQueueMessagesWaitingImpl( const QueueHandle_t pxQueue ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        UBaseType_t uxReturn = 0;
        BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

        lIndex = ( int32_t ) pxQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    uxReturn = uxQueueMessagesWaiting( xInternalQueueHandle );
                }
            }
        }

        return uxReturn;
    }
/*-----------------------------------------------------------*/

    UBaseType_t MPU_uxQueueSpacesAvailableImpl( const QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

    UBaseType_t MPU_uxQueueSpacesAvailableImpl( const QueueHandle_t xQueue ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        UBaseType_t uxReturn = 0;
        BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    uxReturn = uxQueueSpacesAvailable( xInternalQueueHandle );
                }
            }
        }

        return uxReturn;
    }
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueReceiveImpl( QueueHandle_t pxQueue,
                                      void * const pvBuffer,
                                      TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

    BaseType_t MPU_xQueueReceiveImpl( QueueHandle_t pxQueue,
                                      void * const pvBuffer,
                                      TickType_t xTicksToWait ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        BaseType_t xReturn = pdFAIL;
        BaseType_t xIsReceiveBufferWritable = pdFALSE;
        BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;
        UBaseType_t uxQueueItemSize;

        lIndex = ( int32_t ) pxQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    uxQueueItemSize = uxQueueGetQueueItemSize( xInternalQueueHandle );

                    if( ( !( ( ( pvBuffer ) == NULL ) && ( uxQueueItemSize != ( UBaseType_t ) 0U ) ) )
                        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
                            && ( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0U ) ) )
                        #endif
                        )
                    {
                        xIsReceiveBufferWritable = xPortIsAuthorizedToAccessBuffer( pvBuffer,
                                                                                    uxQueueItemSize,
                                                                                    tskMPU_WRITE_PERMISSION );

                        if( xIsReceiveBufferWritable == pdTRUE )
                        {
                            xReturn = xQueueReceive( xInternalQueueHandle, pvBuffer, xTicksToWait );
                        }
                    }
                }
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueuePeekImpl( QueueHandle_t xQueue,
                                   void * const pvBuffer,
                                   TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

    BaseType_t MPU_xQueuePeekImpl( QueueHandle_t xQueue,
                                   void * const pvBuffer,
                                   TickType_t xTicksToWait ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        BaseType_t xReturn = pdFAIL;
        BaseType_t xIsReceiveBufferWritable = pdFALSE;
        UBaseType_t uxQueueItemSize;
        BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    uxQueueItemSize = uxQueueGetQueueItemSize( xInternalQueueHandle );

                    if( ( !( ( ( pvBuffer ) == NULL ) && ( uxQueueItemSize != ( UBaseType_t ) 0U ) ) )
                        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
                            && ( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0U ) ) )
                        #endif
                        )
                    {
                        xIsReceiveBufferWritable = xPortIsAuthorizedToAccessBuffer( pvBuffer,
                                                                                    uxQueueItemSize,
                                                                                    tskMPU_WRITE_PERMISSION );

                        if( xIsReceiveBufferWritable == pdTRUE )
                        {
                            xReturn = xQueuePeek( xInternalQueueHandle, pvBuffer, xTicksToWait );
                        }
                    }
                }
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueSemaphoreTakeImpl( QueueHandle_t xQueue,
                                            TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

    BaseType_t MPU_xQueueSemaphoreTakeImpl( QueueHandle_t xQueue,
                                            TickType_t xTicksToWait ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        BaseType_t xReturn = pdFAIL;
        UBaseType_t uxQueueItemSize;
        BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    uxQueueItemSize = uxQueueGetQueueItemSize( xInternalQueueHandle );

                    if( ( uxQueueItemSize == 0U )
                        #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
                            && ( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0U ) ) )
                        #endif
                        )
                    {
                        xReturn = xQueueSemaphoreTake( xInternalQueueHandle, xTicksToWait );
                    }
                }
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )

        TaskHandle_t MPU_xQueueGetMutexHolderImpl( QueueHandle_t xSemaphore ) PRIVILEGED_FUNCTION;

        TaskHandle_t MPU_xQueueGetMutexHolderImpl( QueueHandle_t xSemaphore ) /* 特权函数 */
        {
            TaskHandle_t xMutexHolderTaskInternalHandle = NULL;
            TaskHandle_t xMutexHolderTaskExternalHandle = NULL;
            int32_t lIndex, lMutexHolderTaskIndex;
            QueueHandle_t xInternalQueueHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;


            lIndex = ( int32_t ) xSemaphore;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
                {
                    xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalQueueHandle != NULL )
                    {
                        xMutexHolderTaskInternalHandle = xQueueGetMutexHolder( xInternalQueueHandle );

                        if( xMutexHolderTaskInternalHandle != NULL )
                        {
                            lMutexHolderTaskIndex = MPU_GetIndexForTaskHandle( xMutexHolderTaskInternalHandle );

                            if( lMutexHolderTaskIndex != -1 )
                            {
                                xMutexHolderTaskExternalHandle = ( TaskHandle_t ) ( CONVERT_TO_EXTERNAL_INDEX( lMutexHolderTaskIndex ) );
                            }
                        }
                    }
                }
            }

            return xMutexHolderTaskExternalHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_RECURSIVE_MUTEXES == 1 )

        BaseType_t MPU_xQueueTakeMutexRecursiveImpl( QueueHandle_t xMutex,
                                                     TickType_t xBlockTime ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xQueueTakeMutexRecursiveImpl( QueueHandle_t xMutex,
                                                     TickType_t xBlockTime ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;
            int32_t lIndex;
            QueueHandle_t xInternalQueueHandle = NULL;
            UBaseType_t uxQueueItemSize;

            lIndex = ( int32_t ) xMutex;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
                {
                    xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalQueueHandle != NULL )
                    {
                        uxQueueItemSize = uxQueueGetQueueItemSize( xInternalQueueHandle );

                        if( uxQueueItemSize == 0 )
                        {
                            xReturn = xQueueTakeMutexRecursive( xInternalQueueHandle, xBlockTime );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_RECURSIVE_MUTEXES == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_RECURSIVE_MUTEXES == 1 )

        BaseType_t MPU_xQueueGiveMutexRecursiveImpl( QueueHandle_t xMutex ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xQueueGiveMutexRecursiveImpl( QueueHandle_t xMutex ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;
            int32_t lIndex;
            QueueHandle_t xInternalQueueHandle = NULL;

            lIndex = ( int32_t ) xMutex;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
                {
                    xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalQueueHandle != NULL )
                    {
                        xReturn = xQueueGiveMutexRecursive( xInternalQueueHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_RECURSIVE_MUTEXES == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_QUEUE_SETS == 1 )

        QueueSetMemberHandle_t MPU_xQueueSelectFromSetImpl( QueueSetHandle_t xQueueSet,
                                                            TickType_t xBlockTimeTicks ) PRIVILEGED_FUNCTION;

        QueueSetMemberHandle_t MPU_xQueueSelectFromSetImpl( QueueSetHandle_t xQueueSet,
                                                            TickType_t xBlockTimeTicks ) /* 特权函数 */
        {
            QueueSetHandle_t xInternalQueueSetHandle = NULL;
            QueueSetMemberHandle_t xSelectedMemberInternal = NULL;
            QueueSetMemberHandle_t xSelectedMemberExternal = NULL;
            int32_t lIndexQueueSet, lIndexSelectedMember;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueueSet = pdFALSE;

            lIndexQueueSet = ( int32_t ) xQueueSet;

            if( IS_EXTERNAL_INDEX_VALID( lIndexQueueSet ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueueSet = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSet ) );

                if( xCallingTaskIsAuthorizedToAccessQueueSet == pdTRUE )
                {
                    xInternalQueueSetHandle = MPU_GetQueueSetHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSet ) );

                    if( xInternalQueueSetHandle != NULL )
                    {
                        xSelectedMemberInternal = xQueueSelectFromSet( xInternalQueueSetHandle, xBlockTimeTicks );

                        if( xSelectedMemberInternal != NULL )
                        {
                            lIndexSelectedMember = MPU_GetIndexForQueueSetMemberHandle( xSelectedMemberInternal );

                            if( lIndexSelectedMember != -1 )
                            {
                                xSelectedMemberExternal = ( QueueSetMemberHandle_t ) ( CONVERT_TO_EXTERNAL_INDEX( lIndexSelectedMember ) );
                            }
                        }
                    }
                }
            }

            return xSelectedMemberExternal;
        }

    #endif /* 条件块结束：if ( configUSE_QUEUE_SETS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_QUEUE_SETS == 1 )

        BaseType_t MPU_xQueueAddToSetImpl( QueueSetMemberHandle_t xQueueOrSemaphore,
                                           QueueSetHandle_t xQueueSet ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xQueueAddToSetImpl( QueueSetMemberHandle_t xQueueOrSemaphore,
                                           QueueSetHandle_t xQueueSet ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            QueueSetMemberHandle_t xInternalQueueSetMemberHandle = NULL;
            QueueSetHandle_t xInternalQueueSetHandle = NULL;
            int32_t lIndexQueueSet, lIndexQueueSetMember;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueueSet = pdFALSE;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueueSetMember = pdFALSE;

            lIndexQueueSet = ( int32_t ) xQueueSet;
            lIndexQueueSetMember = ( int32_t ) xQueueOrSemaphore;

            if( ( IS_EXTERNAL_INDEX_VALID( lIndexQueueSet ) != pdFALSE ) &&
                ( IS_EXTERNAL_INDEX_VALID( lIndexQueueSetMember ) != pdFALSE ) )
            {
                xCallingTaskIsAuthorizedToAccessQueueSet = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSet ) );
                xCallingTaskIsAuthorizedToAccessQueueSetMember = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSetMember ) );

                if( ( xCallingTaskIsAuthorizedToAccessQueueSet == pdTRUE ) && ( xCallingTaskIsAuthorizedToAccessQueueSetMember == pdTRUE ) )
                {
                    xInternalQueueSetHandle = MPU_GetQueueSetHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSet ) );
                    xInternalQueueSetMemberHandle = MPU_GetQueueSetMemberHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSetMember ) );

                    if( ( xInternalQueueSetHandle != NULL ) && ( xInternalQueueSetMemberHandle != NULL ) )
                    {
                        xReturn = xQueueAddToSet( xInternalQueueSetMemberHandle, xInternalQueueSetHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_QUEUE_SETS == 1 ) */
/*-----------------------------------------------------------*/

    #if configQUEUE_REGISTRY_SIZE > 0

        void MPU_vQueueAddToRegistryImpl( QueueHandle_t xQueue,
                                          const char * pcName ) PRIVILEGED_FUNCTION;

        void MPU_vQueueAddToRegistryImpl( QueueHandle_t xQueue,
                                          const char * pcName ) /* 特权函数 */
        {
            int32_t lIndex;
            QueueHandle_t xInternalQueueHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

            lIndex = ( int32_t ) xQueue;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
                {
                    xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalQueueHandle != NULL )
                    {
                        vQueueAddToRegistry( xInternalQueueHandle, pcName );
                    }
                }
            }
        }

    #endif /* 条件块结束：if configQUEUE_REGISTRY_SIZE > 0 */
/*-----------------------------------------------------------*/

    #if configQUEUE_REGISTRY_SIZE > 0

        void MPU_vQueueUnregisterQueueImpl( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

        void MPU_vQueueUnregisterQueueImpl( QueueHandle_t xQueue ) /* 特权函数 */
        {
            int32_t lIndex;
            QueueHandle_t xInternalQueueHandle = NULL;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

            lIndex = ( int32_t ) xQueue;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
                {
                    xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalQueueHandle != NULL )
                    {
                        vQueueUnregisterQueue( xInternalQueueHandle );
                    }
                }
            }
        }

    #endif /* 条件块结束：if configQUEUE_REGISTRY_SIZE > 0 */
/*-----------------------------------------------------------*/

    #if configQUEUE_REGISTRY_SIZE > 0

        const char * MPU_pcQueueGetNameImpl( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;

        const char * MPU_pcQueueGetNameImpl( QueueHandle_t xQueue ) /* 特权函数 */
        {
            const char * pcReturn = NULL;
            QueueHandle_t xInternalQueueHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessQueue = pdFALSE;

            lIndex = ( int32_t ) xQueue;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessQueue = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessQueue == pdTRUE )
                {
                    xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalQueueHandle != NULL )
                    {
                        pcReturn = pcQueueGetName( xInternalQueueHandle );
                    }
                }
            }

            return pcReturn;
        }

    #endif /* 条件块结束：if configQUEUE_REGISTRY_SIZE > 0 */
/*-----------------------------------------------------------*/

/*
 * 队列 API 的仅特权包装函数。需要这些包装函数，
 * 以便应用程序在所有 API 中使用由 mpu_wrappers.c 维护的
 * 不透明句柄。
 */
/*-----------------------------------------------------------*/

    void MPU_vQueueDelete( QueueHandle_t xQueue ) /* 特权函数 */
    {
        QueueHandle_t xInternalQueueHandle = NULL;
        int32_t lIndex;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                vQueueDelete( xInternalQueueHandle );
                MPU_SetIndexFreeInKernelObjectPool( CONVERT_TO_INTERNAL_INDEX( lIndex ) );
            }
        }
    }
/*-----------------------------------------------------------*/

    #if ( ( configUSE_MUTEXES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

        QueueHandle_t MPU_xQueueCreateMutex( const uint8_t ucQueueType ) /* 特权函数 */
        {
            QueueHandle_t xInternalQueueHandle = NULL;
            QueueHandle_t xExternalQueueHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueHandle = xQueueCreateMutex( ucQueueType );

                if( xInternalQueueHandle != NULL )
                {
                    MPU_StoreQueueHandleAtIndex( lIndex, xInternalQueueHandle );
                    xExternalQueueHandle = ( QueueHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_MUTEXES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_MUTEXES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

        QueueHandle_t MPU_xQueueCreateMutexStatic( const uint8_t ucQueueType,
                                                   StaticQueue_t * pxStaticQueue ) /* 特权函数 */
        {
            QueueHandle_t xInternalQueueHandle = NULL;
            QueueHandle_t xExternalQueueHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueHandle = xQueueCreateMutexStatic( ucQueueType, pxStaticQueue );

                if( xInternalQueueHandle != NULL )
                {
                    MPU_StoreQueueHandleAtIndex( lIndex, xInternalQueueHandle );
                    xExternalQueueHandle = ( QueueHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_MUTEXES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

        QueueHandle_t MPU_xQueueCreateCountingSemaphore( UBaseType_t uxCountValue,
                                                         UBaseType_t uxInitialCount ) /* 特权函数 */
        {
            QueueHandle_t xInternalQueueHandle = NULL;
            QueueHandle_t xExternalQueueHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueHandle = xQueueCreateCountingSemaphore( uxCountValue, uxInitialCount );

                if( xInternalQueueHandle != NULL )
                {
                    MPU_StoreQueueHandleAtIndex( lIndex, xInternalQueueHandle );
                    xExternalQueueHandle = ( QueueHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

        QueueHandle_t MPU_xQueueCreateCountingSemaphoreStatic( const UBaseType_t uxMaxCount,
                                                               const UBaseType_t uxInitialCount,
                                                               StaticQueue_t * pxStaticQueue ) /* 特权函数 */
        {
            QueueHandle_t xInternalQueueHandle = NULL;
            QueueHandle_t xExternalQueueHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueHandle = xQueueCreateCountingSemaphoreStatic( uxMaxCount, uxInitialCount, pxStaticQueue );

                if( xInternalQueueHandle != NULL )
                {
                    MPU_StoreQueueHandleAtIndex( lIndex, xInternalQueueHandle );
                    xExternalQueueHandle = ( QueueHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_COUNTING_SEMAPHORES == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

        QueueHandle_t MPU_xQueueGenericCreate( UBaseType_t uxQueueLength,
                                               UBaseType_t uxItemSize,
                                               uint8_t ucQueueType ) /* 特权函数 */
        {
            QueueHandle_t xInternalQueueHandle = NULL;
            QueueHandle_t xExternalQueueHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueHandle = xQueueGenericCreate( uxQueueLength, uxItemSize, ucQueueType );

                if( xInternalQueueHandle != NULL )
                {
                    MPU_StoreQueueHandleAtIndex( lIndex, xInternalQueueHandle );
                    xExternalQueueHandle = ( QueueHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueHandle;
        }

    #endif /* 条件块结束：if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

        QueueHandle_t MPU_xQueueGenericCreateStatic( const UBaseType_t uxQueueLength,
                                                     const UBaseType_t uxItemSize,
                                                     uint8_t * pucQueueStorage,
                                                     StaticQueue_t * pxStaticQueue,
                                                     const uint8_t ucQueueType ) /* 特权函数 */
        {
            QueueHandle_t xInternalQueueHandle = NULL;
            QueueHandle_t xExternalQueueHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueHandle = xQueueGenericCreateStatic( uxQueueLength, uxItemSize, pucQueueStorage, pxStaticQueue, ucQueueType );

                if( xInternalQueueHandle != NULL )
                {
                    MPU_StoreQueueHandleAtIndex( lIndex, xInternalQueueHandle );
                    xExternalQueueHandle = ( QueueHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueHandle;
        }

    #endif /* 条件块结束：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueGenericReset( QueueHandle_t xQueue,
                                       BaseType_t xNewQueue ) /* 特权函数 */
    {
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;
        BaseType_t xReturn = pdFAIL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueueGenericReset( xInternalQueueHandle, xNewQueue );
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    #if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

        QueueSetHandle_t MPU_xQueueCreateSet( UBaseType_t uxEventQueueLength ) /* 特权函数 */
        {
            QueueSetHandle_t xInternalQueueSetHandle = NULL;
            QueueSetHandle_t xExternalQueueSetHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueSetHandle = xQueueCreateSet( uxEventQueueLength );

                if( xInternalQueueSetHandle != NULL )
                {
                    MPU_StoreQueueSetHandleAtIndex( lIndex, xInternalQueueSetHandle );
                    xExternalQueueSetHandle = ( QueueSetHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueSetHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

        QueueSetHandle_t MPU_xQueueCreateSetStatic( const UBaseType_t uxEventQueueLength,
                                                    uint8_t * pucQueueStorage,
                                                    StaticQueue_t * pxStaticQueue ) /* 特权函数 */
        {
            QueueSetHandle_t xInternalQueueSetHandle = NULL;
            QueueSetHandle_t xExternalQueueSetHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalQueueSetHandle = xQueueCreateSetStatic( uxEventQueueLength, pucQueueStorage, pxStaticQueue );

                if( xInternalQueueSetHandle != NULL )
                {
                    MPU_StoreQueueSetHandleAtIndex( lIndex, xInternalQueueSetHandle );
                    xExternalQueueSetHandle = ( QueueSetHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalQueueSetHandle;
        }

    #endif /* 条件块结束：if ( ( configUSE_QUEUE_SETS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_QUEUE_SETS == 1 )

        BaseType_t MPU_xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                                            QueueSetHandle_t xQueueSet ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            QueueSetMemberHandle_t xInternalQueueSetMemberHandle = NULL;
            QueueSetHandle_t xInternalQueueSetHandle = NULL;
            int32_t lIndexQueueSet, lIndexQueueSetMember;

            lIndexQueueSet = ( int32_t ) xQueueSet;
            lIndexQueueSetMember = ( int32_t ) xQueueOrSemaphore;

            if( ( IS_EXTERNAL_INDEX_VALID( lIndexQueueSet ) != pdFALSE ) &&
                ( IS_EXTERNAL_INDEX_VALID( lIndexQueueSetMember ) != pdFALSE ) )
            {
                xInternalQueueSetHandle = MPU_GetQueueSetHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSet ) );
                xInternalQueueSetMemberHandle = MPU_GetQueueSetMemberHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSetMember ) );

                if( ( xInternalQueueSetHandle != NULL ) && ( xInternalQueueSetMemberHandle != NULL ) )
                {
                    xReturn = xQueueRemoveFromSet( xInternalQueueSetMemberHandle, xInternalQueueSetHandle );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_QUEUE_SETS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )

        BaseType_t MPU_xQueueGenericGetStaticBuffers( QueueHandle_t xQueue,
                                                      uint8_t ** ppucQueueStorage,
                                                      StaticQueue_t ** ppxStaticQueue ) /* 特权函数 */
        {
            int32_t lIndex;
            QueueHandle_t xInternalQueueHandle = NULL;
            BaseType_t xReturn = pdFALSE;

            lIndex = ( int32_t ) xQueue;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalQueueHandle != NULL )
                {
                    xReturn = xQueueGenericGetStaticBuffers( xInternalQueueHandle, ppucQueueStorage, ppxStaticQueue );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueGenericSendFromISR( QueueHandle_t xQueue,
                                             const void * const pvItemToQueue,
                                             BaseType_t * const pxHigherPriorityTaskWoken,
                                             const BaseType_t xCopyPosition ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFAIL;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueueGenericSendFromISR( xInternalQueueHandle, pvItemToQueue, pxHigherPriorityTaskWoken, xCopyPosition );
            }
        }

        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueGiveFromISR( QueueHandle_t xQueue,
                                      BaseType_t * const pxHigherPriorityTaskWoken ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFAIL;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueueGiveFromISR( xInternalQueueHandle, pxHigherPriorityTaskWoken );
            }
        }

        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueuePeekFromISR( QueueHandle_t xQueue,
                                      void * const pvBuffer ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFAIL;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueuePeekFromISR( xInternalQueueHandle, pvBuffer );
            }
        }

        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueReceiveFromISR( QueueHandle_t xQueue,
                                         void * const pvBuffer,
                                         BaseType_t * const pxHigherPriorityTaskWoken ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFAIL;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueueReceiveFromISR( xInternalQueueHandle, pvBuffer, pxHigherPriorityTaskWoken );
            }
        }

        return xReturn;
    }

/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueIsQueueEmptyFromISR( const QueueHandle_t xQueue ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFAIL;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueueIsQueueEmptyFromISR( xInternalQueueHandle );
            }
        }

        return xReturn;
    }
/*-----------------------------------------------------------*/

    BaseType_t MPU_xQueueIsQueueFullFromISR( const QueueHandle_t xQueue ) /* 特权函数 */
    {
        BaseType_t xReturn = pdFAIL;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                xReturn = xQueueIsQueueFullFromISR( xInternalQueueHandle );
            }
        }

        return xReturn;
    }

/*-----------------------------------------------------------*/

    UBaseType_t MPU_uxQueueMessagesWaitingFromISR( const QueueHandle_t xQueue ) /* 特权函数 */
    {
        UBaseType_t uxReturn = 0;
        int32_t lIndex;
        QueueHandle_t xInternalQueueHandle = NULL;

        lIndex = ( int32_t ) xQueue;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalQueueHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalQueueHandle != NULL )
            {
                uxReturn = uxQueueMessagesWaitingFromISR( xInternalQueueHandle );
            }
        }

        return uxReturn;
    }

/*-----------------------------------------------------------*/

    #if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )

        TaskHandle_t MPU_xQueueGetMutexHolderFromISR( QueueHandle_t xSemaphore ) /* 特权函数 */
        {
            TaskHandle_t xMutexHolderTaskInternalHandle = NULL;
            TaskHandle_t xMutexHolderTaskExternalHandle = NULL;
            int32_t lIndex, lMutexHolderTaskIndex;
            QueueHandle_t xInternalSemaphoreHandle = NULL;

            lIndex = ( int32_t ) xSemaphore;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalSemaphoreHandle = MPU_GetQueueHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalSemaphoreHandle != NULL )
                {
                    xMutexHolderTaskInternalHandle = xQueueGetMutexHolder( xInternalSemaphoreHandle );

                    if( xMutexHolderTaskInternalHandle != NULL )
                    {
                        lMutexHolderTaskIndex = MPU_GetIndexForTaskHandle( xMutexHolderTaskInternalHandle );

                        if( lMutexHolderTaskIndex != -1 )
                        {
                            xMutexHolderTaskExternalHandle = ( TaskHandle_t ) ( CONVERT_TO_EXTERNAL_INDEX( lMutexHolderTaskIndex ) );
                        }
                    }
                }
            }

            return xMutexHolderTaskExternalHandle;
        }

    #endif /* 条件块结束：#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_QUEUE_SETS == 1 )

        QueueSetMemberHandle_t MPU_xQueueSelectFromSetFromISR( QueueSetHandle_t xQueueSet ) /* 特权函数 */
        {
            QueueSetHandle_t xInternalQueueSetHandle = NULL;
            QueueSetMemberHandle_t xSelectedMemberInternal = NULL;
            QueueSetMemberHandle_t xSelectedMemberExternal = NULL;
            int32_t lIndexQueueSet, lIndexSelectedMember;

            lIndexQueueSet = ( int32_t ) xQueueSet;

            if( IS_EXTERNAL_INDEX_VALID( lIndexQueueSet ) != pdFALSE )
            {
                xInternalQueueSetHandle = MPU_GetQueueSetHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndexQueueSet ) );

                if( xInternalQueueSetHandle != NULL )
                {
                    xSelectedMemberInternal = xQueueSelectFromSetFromISR( xInternalQueueSetHandle );

                    if( xSelectedMemberInternal != NULL )
                    {
                        lIndexSelectedMember = MPU_GetIndexForQueueSetMemberHandle( xSelectedMemberInternal );

                        if( lIndexSelectedMember != -1 )
                        {
                            xSelectedMemberExternal = ( QueueSetMemberHandle_t ) ( CONVERT_TO_EXTERNAL_INDEX( lIndexSelectedMember ) );
                        }
                    }
                }
            }

            return xSelectedMemberExternal;
        }

    #endif /* 条件块结束：if ( configUSE_QUEUE_SETS == 1 ) */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* 定时器 API 的 MPU 包装器。 */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        void * MPU_pvTimerGetTimerIDImpl( const TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        void * MPU_pvTimerGetTimerIDImpl( const TimerHandle_t xTimer ) /* 特权函数 */
        {
            void * pvReturn = NULL;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        pvReturn = pvTimerGetTimerID( xInternalTimerHandle );
                    }
                }
            }

            return pvReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        void MPU_vTimerSetTimerIDImpl( TimerHandle_t xTimer,
                                       void * pvNewID ) PRIVILEGED_FUNCTION;

        void MPU_vTimerSetTimerIDImpl( TimerHandle_t xTimer,
                                       void * pvNewID ) /* 特权函数 */
        {
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        vTimerSetTimerID( xInternalTimerHandle, pvNewID );
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        BaseType_t MPU_xTimerIsTimerActiveImpl( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTimerIsTimerActiveImpl( TimerHandle_t xTimer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        xReturn = xTimerIsTimerActive( xInternalTimerHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        TaskHandle_t MPU_xTimerGetTimerDaemonTaskHandleImpl( void ) PRIVILEGED_FUNCTION;

        TaskHandle_t MPU_xTimerGetTimerDaemonTaskHandleImpl( void ) /* 特权函数 */
        {
            TaskHandle_t xReturn;

            xReturn = xTimerGetTimerDaemonTaskHandle();

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        BaseType_t MPU_xTimerGenericCommandFromTask( TimerHandle_t xTimer,
                                                     const BaseType_t xCommandID,
                                                     const TickType_t xOptionalValue,
                                                     BaseType_t * const pxHigherPriorityTaskWoken,
                                                     const TickType_t xTicksToWait ) /* FREERTOS 系统调用 */
        {
            BaseType_t xReturn = pdFALSE;
            xTimerGenericCommandFromTaskParams_t xParams;

            xParams.xTimer = xTimer;
            xParams.xCommandID = xCommandID;
            xParams.xOptionalValue = xOptionalValue;
            xParams.pxHigherPriorityTaskWoken = pxHigherPriorityTaskWoken;
            xParams.xTicksToWait = xTicksToWait;

            xReturn = MPU_xTimerGenericCommandFromTaskEntry( &( xParams ) );

            return xReturn;
        }

        BaseType_t MPU_xTimerGenericCommandFromTaskImpl( const xTimerGenericCommandFromTaskParams_t * pxParams ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTimerGenericCommandFromTaskImpl( const xTimerGenericCommandFromTaskParams_t * pxParams ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xIsHigherPriorityTaskWokenWriteable = pdFALSE;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;
            BaseType_t xAreParamsReadable = pdFALSE;

            if( pxParams != NULL )
            {
                xAreParamsReadable = xPortIsAuthorizedToAccessBuffer( pxParams,
                                                                      sizeof( xTimerGenericCommandFromTaskParams_t ),
                                                                      tskMPU_READ_PERMISSION );
            }

            if( xAreParamsReadable == pdTRUE )
            {
                if( pxParams->xCommandID < tmrFIRST_FROM_ISR_COMMAND )
                {
                    if( pxParams->pxHigherPriorityTaskWoken != NULL )
                    {
                        xIsHigherPriorityTaskWokenWriteable = xPortIsAuthorizedToAccessBuffer( pxParams->pxHigherPriorityTaskWoken,
                                                                                               sizeof( BaseType_t ),
                                                                                               tskMPU_WRITE_PERMISSION );
                    }

                    if( ( pxParams->pxHigherPriorityTaskWoken == NULL ) ||
                        ( xIsHigherPriorityTaskWokenWriteable == pdTRUE ) )
                    {
                        lIndex = ( int32_t ) ( pxParams->xTimer );

                        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                        {
                            xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                            {
                                xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                                if( xInternalTimerHandle != NULL )
                                {
                                    xReturn = xTimerGenericCommandFromTask( xInternalTimerHandle,
                                                                            pxParams->xCommandID,
                                                                            pxParams->xOptionalValue,
                                                                            pxParams->pxHigherPriorityTaskWoken,
                                                                            pxParams->xTicksToWait );
                                }
                            }
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        const char * MPU_pcTimerGetNameImpl( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        const char * MPU_pcTimerGetNameImpl( TimerHandle_t xTimer ) /* 特权函数 */
        {
            const char * pcReturn = NULL;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        pcReturn = pcTimerGetName( xInternalTimerHandle );
                    }
                }
            }

            return pcReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        void MPU_vTimerSetReloadModeImpl( TimerHandle_t xTimer,
                                          const BaseType_t xAutoReload ) PRIVILEGED_FUNCTION;

        void MPU_vTimerSetReloadModeImpl( TimerHandle_t xTimer,
                                          const BaseType_t xAutoReload ) /* 特权函数 */
        {
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        vTimerSetReloadMode( xInternalTimerHandle, xAutoReload );
                    }
                }
            }
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        BaseType_t MPU_xTimerGetReloadModeImpl( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xTimerGetReloadModeImpl( TimerHandle_t xTimer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        xReturn = xTimerGetReloadMode( xInternalTimerHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        UBaseType_t MPU_uxTimerGetReloadModeImpl( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        UBaseType_t MPU_uxTimerGetReloadModeImpl( TimerHandle_t xTimer ) /* 特权函数 */
        {
            UBaseType_t uxReturn = 0;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        uxReturn = uxTimerGetReloadMode( xInternalTimerHandle );
                    }
                }
            }

            return uxReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        TickType_t MPU_xTimerGetPeriodImpl( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        TickType_t MPU_xTimerGetPeriodImpl( TimerHandle_t xTimer ) /* 特权函数 */
        {
            TickType_t xReturn = 0;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        xReturn = xTimerGetPeriod( xInternalTimerHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        TickType_t MPU_xTimerGetExpiryTimeImpl( TimerHandle_t xTimer ) PRIVILEGED_FUNCTION;

        TickType_t MPU_xTimerGetExpiryTimeImpl( TimerHandle_t xTimer ) /* 特权函数 */
        {
            TickType_t xReturn = 0;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessTimer = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessTimer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessTimer == pdTRUE )
                {
                    xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalTimerHandle != NULL )
                    {
                        xReturn = xTimerGetExpiryTime( xInternalTimerHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

/*
 * 定时器 API 的仅特权包装函数。需要这些包装函数，
 * 以便应用程序在所有 API 中使用由 mpu_wrappers.c 维护的
 * 不透明句柄。
 */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_TIMERS == 1 )

        TimerHandle_t MPU_xTimerCreate( const char * const pcTimerName,
                                        const TickType_t xTimerPeriodInTicks,
                                        const BaseType_t xAutoReload,
                                        void * const pvTimerID,
                                        TimerCallbackFunction_t pxCallbackFunction ) /* 特权函数 */
        {
            TimerHandle_t xInternalTimerHandle = NULL;
            TimerHandle_t xExternalTimerHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalTimerHandle = xTimerCreate( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, MPU_TimerCallback );

                if( xInternalTimerHandle != NULL )
                {
                    MPU_StoreTimerHandleAtIndex( lIndex, xInternalTimerHandle, pxCallbackFunction );
                    xExternalTimerHandle = ( TimerHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalTimerHandle;
        }

    #endif /* 条件块结束：if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_TIMERS == 1 )

        TimerHandle_t MPU_xTimerCreateStatic( const char * const pcTimerName,
                                              const TickType_t xTimerPeriodInTicks,
                                              const BaseType_t xAutoReload,
                                              void * const pvTimerID,
                                              TimerCallbackFunction_t pxCallbackFunction,
                                              StaticTimer_t * pxTimerBuffer ) /* 特权函数 */
        {
            TimerHandle_t xInternalTimerHandle = NULL;
            TimerHandle_t xExternalTimerHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalTimerHandle = xTimerCreateStatic( pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, MPU_TimerCallback, pxTimerBuffer );

                if( xInternalTimerHandle != NULL )
                {
                    MPU_StoreTimerHandleAtIndex( lIndex, xInternalTimerHandle, pxCallbackFunction );
                    xExternalTimerHandle = ( TimerHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalTimerHandle;
        }

    #endif /* 条件块结束：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_TIMERS == 1 )

        BaseType_t MPU_xTimerGetStaticBuffer( TimerHandle_t xTimer,
                                              StaticTimer_t ** ppxTimerBuffer ) /* 特权函数 */
        {
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;
            BaseType_t xReturn = pdFALSE;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTimerHandle != NULL )
                {
                    xReturn = xTimerGetStaticBuffer( xInternalTimerHandle, ppxTimerBuffer );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        BaseType_t MPU_xTimerGenericCommandFromISR( TimerHandle_t xTimer,
                                                    const BaseType_t xCommandID,
                                                    const TickType_t xOptionalValue,
                                                    BaseType_t * const pxHigherPriorityTaskWoken,
                                                    const TickType_t xTicksToWait ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTimerHandle != NULL )
                {
                    xReturn = xTimerGenericCommandFromISR( xInternalTimerHandle, xCommandID, xOptionalValue, pxHigherPriorityTaskWoken, xTicksToWait );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_TIMERS == 1 )

        BaseType_t MPU_xTimerDelete( TimerHandle_t xTimer, TickType_t xTicksToWait ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            TimerHandle_t xInternalTimerHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xTimer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalTimerHandle = MPU_GetTimerHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalTimerHandle != NULL )
                {
                    xReturn = xTimerGenericCommandFromTask( xInternalTimerHandle,
                                                            tmrCOMMAND_DELETE,
                                                            0U, /* xOptionalValue 可选值 */
                                                            NULL, /* pxHigherPriorityTaskWoken 更高优先级任务唤醒标志 */
                                                            xTicksToWait );
                    if( xReturn != pdFALSE )
                    {
                        MPU_SetIndexFreeInKernelObjectPool( CONVERT_TO_INTERNAL_INDEX( lIndex ) );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：if ( configUSE_TIMERS == 1 ) */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* 事件组 API 的 MPU 包装器。 */
/*-----------------------------------------------------------*/

    #if ( configUSE_EVENT_GROUPS == 1 )

        EventBits_t MPU_xEventGroupWaitBits( EventGroupHandle_t xEventGroup,
                                             const EventBits_t uxBitsToWaitFor,
                                             const BaseType_t xClearOnExit,
                                             const BaseType_t xWaitForAllBits,
                                             TickType_t xTicksToWait ) /* FREERTOS 系统调用 */
        {
            EventBits_t xReturn = 0;
            xEventGroupWaitBitsParams_t xParams;

            xParams.xEventGroup = xEventGroup;
            xParams.uxBitsToWaitFor = uxBitsToWaitFor;
            xParams.xClearOnExit = xClearOnExit;
            xParams.xWaitForAllBits = xWaitForAllBits;
            xParams.xTicksToWait = xTicksToWait;

            xReturn = MPU_xEventGroupWaitBitsEntry( &( xParams ) );

            return xReturn;
        }

        EventBits_t MPU_xEventGroupWaitBitsImpl( const xEventGroupWaitBitsParams_t * pxParams ) PRIVILEGED_FUNCTION;

        EventBits_t MPU_xEventGroupWaitBitsImpl( const xEventGroupWaitBitsParams_t * pxParams ) /* 特权函数 */
        {
            EventBits_t xReturn = 0;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessEventGroup = pdFALSE;
            BaseType_t xAreParamsReadable = pdFALSE;

            if( pxParams != NULL )
            {
                xAreParamsReadable = xPortIsAuthorizedToAccessBuffer( pxParams,
                                                                      sizeof( xEventGroupWaitBitsParams_t ),
                                                                      tskMPU_READ_PERMISSION );
            }

            if( xAreParamsReadable == pdTRUE )
            {
                if( ( ( pxParams->uxBitsToWaitFor & eventEVENT_BITS_CONTROL_BYTES ) == 0U ) &&
                    ( pxParams->uxBitsToWaitFor != 0U )
                    #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
                        && ( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( pxParams->xTicksToWait != 0U ) ) )
                    #endif
                    )
                {
                    lIndex = ( int32_t ) ( pxParams->xEventGroup );

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessEventGroup = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessEventGroup == pdTRUE )
                        {
                            xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalEventGroupHandle != NULL )
                            {
                                xReturn = xEventGroupWaitBits( xInternalEventGroupHandle,
                                                               pxParams->uxBitsToWaitFor,
                                                               pxParams->xClearOnExit,
                                                               pxParams->xWaitForAllBits,
                                                               pxParams->xTicksToWait );
                            }
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_EVENT_GROUPS == 1 )

        EventBits_t MPU_xEventGroupClearBitsImpl( EventGroupHandle_t xEventGroup,
                                                  const EventBits_t uxBitsToClear ) PRIVILEGED_FUNCTION;

        EventBits_t MPU_xEventGroupClearBitsImpl( EventGroupHandle_t xEventGroup,
                                                  const EventBits_t uxBitsToClear ) /* 特权函数 */
        {
            EventBits_t xReturn = 0;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessEventGroup = pdFALSE;

            if( ( uxBitsToClear & eventEVENT_BITS_CONTROL_BYTES ) == 0U )
            {
                lIndex = ( int32_t ) xEventGroup;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessEventGroup = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessEventGroup == pdTRUE )
                    {
                        xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalEventGroupHandle != NULL )
                        {
                            xReturn = xEventGroupClearBits( xInternalEventGroupHandle, uxBitsToClear );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_EVENT_GROUPS == 1 )

        EventBits_t MPU_xEventGroupSetBitsImpl( EventGroupHandle_t xEventGroup,
                                                const EventBits_t uxBitsToSet ) PRIVILEGED_FUNCTION;

        EventBits_t MPU_xEventGroupSetBitsImpl( EventGroupHandle_t xEventGroup,
                                                const EventBits_t uxBitsToSet ) /* 特权函数 */
        {
            EventBits_t xReturn = 0;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessEventGroup = pdFALSE;

            if( ( uxBitsToSet & eventEVENT_BITS_CONTROL_BYTES ) == 0U )
            {
                lIndex = ( int32_t ) xEventGroup;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessEventGroup = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessEventGroup == pdTRUE )
                    {
                        xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalEventGroupHandle != NULL )
                        {
                            xReturn = xEventGroupSetBits( xInternalEventGroupHandle, uxBitsToSet );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_EVENT_GROUPS == 1 )

        EventBits_t MPU_xEventGroupSyncImpl( EventGroupHandle_t xEventGroup,
                                             const EventBits_t uxBitsToSet,
                                             const EventBits_t uxBitsToWaitFor,
                                             TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

        EventBits_t MPU_xEventGroupSyncImpl( EventGroupHandle_t xEventGroup,
                                             const EventBits_t uxBitsToSet,
                                             const EventBits_t uxBitsToWaitFor,
                                             TickType_t xTicksToWait ) /* 特权函数 */
        {
            EventBits_t xReturn = 0;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessEventGroup = pdFALSE;

            if( ( ( uxBitsToWaitFor & eventEVENT_BITS_CONTROL_BYTES ) == 0U ) &&
                ( uxBitsToWaitFor != 0U )
                #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
                    && ( !( ( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) && ( xTicksToWait != 0U ) ) )
                #endif
                )
            {
                lIndex = ( int32_t ) xEventGroup;

                if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                {
                    xCallingTaskIsAuthorizedToAccessEventGroup = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xCallingTaskIsAuthorizedToAccessEventGroup == pdTRUE )
                    {
                        xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xInternalEventGroupHandle != NULL )
                        {
                            xReturn = xEventGroupSync( xInternalEventGroupHandle, uxBitsToSet, uxBitsToWaitFor, xTicksToWait );
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_EVENT_GROUPS == 1 ) && ( configUSE_TRACE_FACILITY == 1 ) )

        UBaseType_t MPU_uxEventGroupGetNumberImpl( void * xEventGroup ) PRIVILEGED_FUNCTION;

        UBaseType_t MPU_uxEventGroupGetNumberImpl( void * xEventGroup ) /* 特权函数 */
        {
            UBaseType_t xReturn = 0;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessEventGroup = pdFALSE;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessEventGroup = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessEventGroup == pdTRUE )
                {
                    xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalEventGroupHandle != NULL )
                    {
                        xReturn = uxEventGroupGetNumber( xInternalEventGroupHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( ( configUSE_EVENT_GROUPS == 1 ) && ( configUSE_TRACE_FACILITY == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_EVENT_GROUPS == 1 ) && ( configUSE_TRACE_FACILITY == 1 ) )

        void MPU_vEventGroupSetNumberImpl( void * xEventGroup,
                                           UBaseType_t uxEventGroupNumber ) PRIVILEGED_FUNCTION;

        void MPU_vEventGroupSetNumberImpl( void * xEventGroup,
                                           UBaseType_t uxEventGroupNumber ) /* 特权函数 */
        {
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessEventGroup = pdFALSE;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessEventGroup = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessEventGroup == pdTRUE )
                {
                    xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalEventGroupHandle != NULL )
                    {
                        vEventGroupSetNumber( xInternalEventGroupHandle, uxEventGroupNumber );
                    }
                }
            }
        }

    #endif /* 条件块结束：#if ( ( configUSE_EVENT_GROUPS == 1 ) && ( configUSE_TRACE_FACILITY == 1 ) ) */
/*-----------------------------------------------------------*/

/*
 * 事件组 API 的仅特权包装函数。需要这些包装函数，
 * 以便应用程序在所有 API 中使用由 mpu_wrappers.c 维护的
 * 不透明句柄。
 */
/*-----------------------------------------------------------*/

    #if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_EVENT_GROUPS == 1 ) )

        EventGroupHandle_t MPU_xEventGroupCreate( void ) /* 特权函数 */
        {
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            EventGroupHandle_t xExternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalEventGroupHandle = xEventGroupCreate();

                if( xInternalEventGroupHandle != NULL )
                {
                    MPU_StoreEventGroupHandleAtIndex( lIndex, xInternalEventGroupHandle );
                    xExternalEventGroupHandle = ( EventGroupHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalEventGroupHandle;
        }

    #endif /* 条件块结束：#if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_EVENT_GROUPS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_EVENT_GROUPS == 1 ) )

        EventGroupHandle_t MPU_xEventGroupCreateStatic( StaticEventGroup_t * pxEventGroupBuffer ) /* 特权函数 */
        {
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            EventGroupHandle_t xExternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = MPU_GetFreeIndexInKernelObjectPool();

            if( lIndex != -1 )
            {
                xInternalEventGroupHandle = xEventGroupCreateStatic( pxEventGroupBuffer );

                if( xInternalEventGroupHandle != NULL )
                {
                    MPU_StoreEventGroupHandleAtIndex( lIndex, xInternalEventGroupHandle );
                    xExternalEventGroupHandle = ( EventGroupHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                }
                else
                {
                    MPU_SetIndexFreeInKernelObjectPool( lIndex );
                }
            }

            return xExternalEventGroupHandle;
        }

    #endif /* 条件块结束：#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_EVENT_GROUPS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_EVENT_GROUPS == 1 )

        void MPU_vEventGroupDelete( EventGroupHandle_t xEventGroup ) /* 特权函数 */
        {
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalEventGroupHandle != NULL )
                {
                    vEventGroupDelete( xInternalEventGroupHandle );
                    MPU_SetIndexFreeInKernelObjectPool( CONVERT_TO_INTERNAL_INDEX( lIndex ) );
                }
            }
        }

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_EVENT_GROUPS == 1 ) )

        BaseType_t MPU_xEventGroupGetStaticBuffer( EventGroupHandle_t xEventGroup,
                                                   StaticEventGroup_t ** ppxEventGroupBuffer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalEventGroupHandle != NULL )
                {
                    xReturn = xEventGroupGetStaticBuffer( xInternalEventGroupHandle, ppxEventGroupBuffer );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_EVENT_GROUPS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_EVENT_GROUPS == 1 ) && ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) )

        BaseType_t MPU_xEventGroupClearBitsFromISR( EventGroupHandle_t xEventGroup,
                                                    const EventBits_t uxBitsToClear ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalEventGroupHandle != NULL )
                {
                    xReturn = xEventGroupClearBitsFromISR( xInternalEventGroupHandle, uxBitsToClear );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( ( configUSE_EVENT_GROUPS == 1 ) && ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configUSE_EVENT_GROUPS == 1 ) && ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) )

        BaseType_t MPU_xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup,
                                                  const EventBits_t uxBitsToSet,
                                                  BaseType_t * pxHigherPriorityTaskWoken ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalEventGroupHandle != NULL )
                {
                    xReturn = xEventGroupSetBitsFromISR( xInternalEventGroupHandle, uxBitsToSet, pxHigherPriorityTaskWoken );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( ( configUSE_EVENT_GROUPS == 1 ) && ( INCLUDE_xTimerPendFunctionCall == 1 ) && ( configUSE_TIMERS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_EVENT_GROUPS == 1 )

        EventBits_t MPU_xEventGroupGetBitsFromISR( EventGroupHandle_t xEventGroup ) /* 特权函数 */
        {
            EventBits_t xReturn = 0;
            EventGroupHandle_t xInternalEventGroupHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xEventGroup;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalEventGroupHandle = MPU_GetEventGroupHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalEventGroupHandle != NULL )
                {
                    xReturn = xEventGroupGetBitsFromISR( xInternalEventGroupHandle );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_EVENT_GROUPS == 1 ) */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* 流缓冲 API 的 MPU 包装器。 */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        size_t MPU_xStreamBufferSendImpl( StreamBufferHandle_t xStreamBuffer,
                                          const void * pvTxData,
                                          size_t xDataLengthBytes,
                                          TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

        size_t MPU_xStreamBufferSendImpl( StreamBufferHandle_t xStreamBuffer,
                                          const void * pvTxData,
                                          size_t xDataLengthBytes,
                                          TickType_t xTicksToWait ) /* 特权函数 */
        {
            size_t xReturn = 0;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xIsTxDataBufferReadable = pdFALSE;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            if( pvTxData != NULL )
            {
                xIsTxDataBufferReadable = xPortIsAuthorizedToAccessBuffer( pvTxData,
                                                                           xDataLengthBytes,
                                                                           tskMPU_READ_PERMISSION );

                if( xIsTxDataBufferReadable == pdTRUE )
                {
                    lIndex = ( int32_t ) xStreamBuffer;

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                        {
                            xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalStreamBufferHandle != NULL )
                            {
                                xReturn = xStreamBufferSend( xInternalStreamBufferHandle, pvTxData, xDataLengthBytes, xTicksToWait );
                            }
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        size_t MPU_xStreamBufferReceiveImpl( StreamBufferHandle_t xStreamBuffer,
                                             void * pvRxData,
                                             size_t xBufferLengthBytes,
                                             TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;

        size_t MPU_xStreamBufferReceiveImpl( StreamBufferHandle_t xStreamBuffer,
                                             void * pvRxData,
                                             size_t xBufferLengthBytes,
                                             TickType_t xTicksToWait ) /* 特权函数 */
        {
            size_t xReturn = 0;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xIsRxDataBufferWriteable = pdFALSE;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            if( pvRxData != NULL )
            {
                xIsRxDataBufferWriteable = xPortIsAuthorizedToAccessBuffer( pvRxData,
                                                                            xBufferLengthBytes,
                                                                            tskMPU_WRITE_PERMISSION );

                if( xIsRxDataBufferWriteable == pdTRUE )
                {
                    lIndex = ( int32_t ) xStreamBuffer;

                    if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
                    {
                        xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                        if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                        {
                            xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                            if( xInternalStreamBufferHandle != NULL )
                            {
                                xReturn = xStreamBufferReceive( xInternalStreamBufferHandle, pvRxData, xBufferLengthBytes, xTicksToWait );
                            }
                        }
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferIsFullImpl( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xStreamBufferIsFullImpl( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                {
                    xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        xReturn = xStreamBufferIsFull( xInternalStreamBufferHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferIsEmptyImpl( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xStreamBufferIsEmptyImpl( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                {
                    xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        xReturn = xStreamBufferIsEmpty( xInternalStreamBufferHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        size_t MPU_xStreamBufferSpacesAvailableImpl( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

        size_t MPU_xStreamBufferSpacesAvailableImpl( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            size_t xReturn = 0;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                {
                    xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        xReturn = xStreamBufferSpacesAvailable( xInternalStreamBufferHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        size_t MPU_xStreamBufferBytesAvailableImpl( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

        size_t MPU_xStreamBufferBytesAvailableImpl( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            size_t xReturn = 0;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                {
                    xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        xReturn = xStreamBufferBytesAvailable( xInternalStreamBufferHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferSetTriggerLevelImpl( StreamBufferHandle_t xStreamBuffer,
                                                         size_t xTriggerLevel ) PRIVILEGED_FUNCTION;

        BaseType_t MPU_xStreamBufferSetTriggerLevelImpl( StreamBufferHandle_t xStreamBuffer,
                                                         size_t xTriggerLevel ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                {
                    xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        xReturn = xStreamBufferSetTriggerLevel( xInternalStreamBufferHandle, xTriggerLevel );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        size_t MPU_xStreamBufferNextMessageLengthBytesImpl( StreamBufferHandle_t xStreamBuffer ) PRIVILEGED_FUNCTION;

        size_t MPU_xStreamBufferNextMessageLengthBytesImpl( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            size_t xReturn = 0;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;
            BaseType_t xCallingTaskIsAuthorizedToAccessStreamBuffer = pdFALSE;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xCallingTaskIsAuthorizedToAccessStreamBuffer = xPortIsAuthorizedToAccessKernelObject( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xCallingTaskIsAuthorizedToAccessStreamBuffer == pdTRUE )
                {
                    xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        xReturn = xStreamBufferNextMessageLengthBytes( xInternalStreamBufferHandle );
                    }
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

/*
 * 流缓冲 API 的仅特权包装函数。需要这些包装函数，
 * 以便应用程序在所有 API 中使用由 mpu_wrappers.c 维护的
 * 不透明句柄。
 */
/*-----------------------------------------------------------*/

    #if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_STREAM_BUFFERS == 1 ) )

        StreamBufferHandle_t MPU_xStreamBufferGenericCreate( size_t xBufferSizeBytes,
                                                             size_t xTriggerLevelBytes,
                                                             BaseType_t xStreamBufferType,
                                                             StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                                             StreamBufferCallbackFunction_t pxReceiveCompletedCallback ) /* 特权函数 */
        {
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            StreamBufferHandle_t xExternalStreamBufferHandle = NULL;
            int32_t lIndex;

            /**
             * 流缓冲的应用程序级回调功能在启用 MPU 的移植层中被禁用。
             */
            configASSERT( ( pxSendCompletedCallback == NULL ) &&
                          ( pxReceiveCompletedCallback == NULL ) );

            if( ( pxSendCompletedCallback == NULL ) &&
                ( pxReceiveCompletedCallback == NULL ) )
            {
                lIndex = MPU_GetFreeIndexInKernelObjectPool();

                if( lIndex != -1 )
                {
                    xInternalStreamBufferHandle = xStreamBufferGenericCreate( xBufferSizeBytes,
                                                                              xTriggerLevelBytes,
                                                                              xStreamBufferType,
                                                                              NULL,
                                                                              NULL );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        MPU_StoreStreamBufferHandleAtIndex( lIndex, xInternalStreamBufferHandle );
                        xExternalStreamBufferHandle = ( StreamBufferHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                    }
                    else
                    {
                        MPU_SetIndexFreeInKernelObjectPool( lIndex );
                    }
                }
            }
            else
            {
                traceSTREAM_BUFFER_CREATE_FAILED( xStreamBufferType );
                xExternalStreamBufferHandle = NULL;
            }

            return xExternalStreamBufferHandle;
        }

    #endif /* 条件块结束：#if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configUSE_STREAM_BUFFERS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_STREAM_BUFFERS == 1 ) )

        StreamBufferHandle_t MPU_xStreamBufferGenericCreateStatic( size_t xBufferSizeBytes,
                                                                   size_t xTriggerLevelBytes,
                                                                   BaseType_t xStreamBufferType,
                                                                   uint8_t * const pucStreamBufferStorageArea,
                                                                   StaticStreamBuffer_t * const pxStaticStreamBuffer,
                                                                   StreamBufferCallbackFunction_t pxSendCompletedCallback,
                                                                   StreamBufferCallbackFunction_t pxReceiveCompletedCallback ) /* 特权函数 */
        {
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            StreamBufferHandle_t xExternalStreamBufferHandle = NULL;
            int32_t lIndex;

            /**
             * 流缓冲的应用程序级回调功能在启用 MPU 的移植层中被禁用。
             */
            configASSERT( ( pxSendCompletedCallback == NULL ) &&
                          ( pxReceiveCompletedCallback == NULL ) );

            if( ( pxSendCompletedCallback == NULL ) &&
                ( pxReceiveCompletedCallback == NULL ) )
            {
                lIndex = MPU_GetFreeIndexInKernelObjectPool();

                if( lIndex != -1 )
                {
                    xInternalStreamBufferHandle = xStreamBufferGenericCreateStatic( xBufferSizeBytes,
                                                                                    xTriggerLevelBytes,
                                                                                    xStreamBufferType,
                                                                                    pucStreamBufferStorageArea,
                                                                                    pxStaticStreamBuffer,
                                                                                    NULL,
                                                                                    NULL );

                    if( xInternalStreamBufferHandle != NULL )
                    {
                        MPU_StoreStreamBufferHandleAtIndex( lIndex, xInternalStreamBufferHandle );
                        xExternalStreamBufferHandle = ( StreamBufferHandle_t ) CONVERT_TO_EXTERNAL_INDEX( lIndex );
                    }
                    else
                    {
                        MPU_SetIndexFreeInKernelObjectPool( lIndex );
                    }
                }
            }
            else
            {
                traceSTREAM_BUFFER_CREATE_STATIC_FAILED( xReturn, xStreamBufferType );
                xExternalStreamBufferHandle = NULL;
            }

            return xExternalStreamBufferHandle;
        }

    #endif /* 条件块结束：#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_STREAM_BUFFERS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        void MPU_vStreamBufferDelete( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalStreamBufferHandle != NULL )
                {
                    vStreamBufferDelete( xInternalStreamBufferHandle );
                }

                MPU_SetIndexFreeInKernelObjectPool( CONVERT_TO_INTERNAL_INDEX( lIndex ) );
            }
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferReset( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalStreamBufferHandle != NULL )
                {
                    xReturn = xStreamBufferReset( xInternalStreamBufferHandle );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_STREAM_BUFFERS == 1 ) )

        BaseType_t MPU_xStreamBufferGetStaticBuffers( StreamBufferHandle_t xStreamBuffers,
                                                      uint8_t * ppucStreamBufferStorageArea,
                                                      StaticStreamBuffer_t * ppxStaticStreamBuffer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xStreamBuffers;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalStreamBufferHandle != NULL )
                {
                    xReturn = MPU_xStreamBufferGetStaticBuffers( xInternalStreamBufferHandle, ppucStreamBufferStorageArea, ppxStaticStreamBuffer );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configUSE_STREAM_BUFFERS == 1 ) ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

    size_t MPU_xStreamBufferSendFromISR( StreamBufferHandle_t xStreamBuffer,
                                         const void * pvTxData,
                                         size_t xDataLengthBytes,
                                         BaseType_t * const pxHigherPriorityTaskWoken ) /* 特权函数 */
    {
        size_t xReturn = 0;
        StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
        int32_t lIndex;

        lIndex = ( int32_t ) xStreamBuffer;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalStreamBufferHandle != NULL )
            {
                xReturn = xStreamBufferSendFromISR( xInternalStreamBufferHandle, pvTxData, xDataLengthBytes, pxHigherPriorityTaskWoken );
            }
        }

        return xReturn;
    }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

    size_t MPU_xStreamBufferReceiveFromISR( StreamBufferHandle_t xStreamBuffer,
                                            void * pvRxData,
                                            size_t xBufferLengthBytes,
                                            BaseType_t * const pxHigherPriorityTaskWoken ) /* 特权函数 */
    {
        size_t xReturn = 0;
        StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
        int32_t lIndex;

        lIndex = ( int32_t ) xStreamBuffer;

        if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
        {
            xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

            if( xInternalStreamBufferHandle != NULL )
            {
                xReturn = xStreamBufferReceiveFromISR( xInternalStreamBufferHandle, pvRxData, xBufferLengthBytes, pxHigherPriorityTaskWoken );
            }
        }

        return xReturn;
    }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferSendCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                                          BaseType_t * pxHigherPriorityTaskWoken ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalStreamBufferHandle != NULL )
                {
                    xReturn = xStreamBufferSendCompletedFromISR( xInternalStreamBufferHandle, pxHigherPriorityTaskWoken );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */
/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferReceiveCompletedFromISR( StreamBufferHandle_t xStreamBuffer,
                                                             BaseType_t * pxHigherPriorityTaskWoken ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFALSE;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalStreamBufferHandle != NULL )
                {
                    xReturn = xStreamBufferReceiveCompletedFromISR( xInternalStreamBufferHandle, pxHigherPriorityTaskWoken );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */

/*-----------------------------------------------------------*/

    #if ( configUSE_STREAM_BUFFERS == 1 )

        BaseType_t MPU_xStreamBufferResetFromISR( StreamBufferHandle_t xStreamBuffer ) /* 特权函数 */
        {
            BaseType_t xReturn = pdFAIL;
            StreamBufferHandle_t xInternalStreamBufferHandle = NULL;
            int32_t lIndex;

            lIndex = ( int32_t ) xStreamBuffer;

            if( IS_EXTERNAL_INDEX_VALID( lIndex ) != pdFALSE )
            {
                xInternalStreamBufferHandle = MPU_GetStreamBufferHandleAtIndex( CONVERT_TO_INTERNAL_INDEX( lIndex ) );

                if( xInternalStreamBufferHandle != NULL )
                {
                    xReturn = xStreamBufferResetFromISR( xInternalStreamBufferHandle );
                }
            }

            return xReturn;
        }

    #endif /* 条件块结束：#if ( configUSE_STREAM_BUFFERS == 1 ) */

/*-----------------------------------------------------------*/

/*
 * 应用程序编写者希望在特权模式下执行的函数
 * 可定义在 application_defined_privileged_functions.h 中。
 */

    #if configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS == 1
        #include "application_defined_privileged_functions.h"
    #endif
/*-----------------------------------------------------------*/

/**
 * @brief 系统调用实现函数数组。
 *
 * 数组中的索引必须与 mpu_wrappers.h 中
 * 定义的相应系统调用编号匹配。
 */
    PRIVILEGED_DATA UBaseType_t uxSystemCallImplementations[ NUM_SYSTEM_CALLS ] =
    {
        #if ( configUSE_TASK_NOTIFICATIONS == 1 )
            ( UBaseType_t ) MPU_xTaskGenericNotifyImpl,                     /* 系统调用 SYSTEM_CALL_xTaskGenericNotify. */
            ( UBaseType_t ) MPU_xTaskGenericNotifyWaitImpl,                 /* 系统调用 SYSTEM_CALL_xTaskGenericNotifyWait. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGenericNotify. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGenericNotifyWait. */
        #endif

        #if ( configUSE_TIMERS == 1 )
            ( UBaseType_t ) MPU_xTimerGenericCommandFromTaskImpl,           /* 系统调用 SYSTEM_CALL_xTimerGenericCommandFromTask. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTimerGenericCommandFromTask. */
        #endif

        #if ( configUSE_EVENT_GROUPS == 1 )
            ( UBaseType_t ) MPU_xEventGroupWaitBitsImpl,                    /* 系统调用 SYSTEM_CALL_xEventGroupWaitBits. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xEventGroupWaitBits. */
        #endif

        /* 此行以上的系统调用均接受 5 个参数。 */

        #if ( INCLUDE_xTaskDelayUntil == 1 )
            ( UBaseType_t ) MPU_xTaskDelayUntilImpl,                        /* 系统调用 SYSTEM_CALL_xTaskDelayUntil. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskDelayUntil. */
        #endif

        #if ( INCLUDE_xTaskAbortDelay == 1 )
            ( UBaseType_t ) MPU_xTaskAbortDelayImpl,                        /* 系统调用 SYSTEM_CALL_xTaskAbortDelay. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskAbortDelay. */
        #endif

        #if ( INCLUDE_vTaskDelay == 1 )
            ( UBaseType_t ) MPU_vTaskDelayImpl,                             /* 系统调用 SYSTEM_CALL_vTaskDelay. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTaskDelay. */
        #endif

        #if ( INCLUDE_uxTaskPriorityGet == 1 )
            ( UBaseType_t ) MPU_uxTaskPriorityGetImpl,                      /* 系统调用 SYSTEM_CALL_uxTaskPriorityGet. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_uxTaskPriorityGet. */
        #endif

        #if ( INCLUDE_eTaskGetState == 1 )
            ( UBaseType_t ) MPU_eTaskGetStateImpl,                          /* 系统调用 SYSTEM_CALL_eTaskGetState. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_eTaskGetState. */
        #endif

        #if ( configUSE_TRACE_FACILITY == 1 )
            ( UBaseType_t ) MPU_vTaskGetInfoImpl,                           /* 系统调用 SYSTEM_CALL_vTaskGetInfo. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTaskGetInfo. */
        #endif

        #if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
            ( UBaseType_t ) MPU_xTaskGetIdleTaskHandleImpl,                 /* 系统调用 SYSTEM_CALL_xTaskGetIdleTaskHandle. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGetIdleTaskHandle. */
        #endif

        #if ( INCLUDE_vTaskSuspend == 1 )
            ( UBaseType_t ) MPU_vTaskSuspendImpl,                           /* 系统调用 SYSTEM_CALL_vTaskSuspend. */
            ( UBaseType_t ) MPU_vTaskResumeImpl,                            /* 系统调用 SYSTEM_CALL_vTaskResume. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTaskSuspend. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTaskResume. */
        #endif

        ( UBaseType_t ) MPU_xTaskGetTickCountImpl,                          /* 系统调用 SYSTEM_CALL_xTaskGetTickCount. */
        ( UBaseType_t ) MPU_uxTaskGetNumberOfTasksImpl,                     /* 系统调用 SYSTEM_CALL_uxTaskGetNumberOfTasks. */

        #if ( configGENERATE_RUN_TIME_STATS == 1 )
            ( UBaseType_t ) MPU_ulTaskGetRunTimeCounterImpl,                /* 系统调用 SYSTEM_CALL_ulTaskGetRunTimeCounter. */
            ( UBaseType_t ) MPU_ulTaskGetRunTimePercentImpl,                /* 系统调用 SYSTEM_CALL_ulTaskGetRunTimePercent. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_ulTaskGetRunTimeCounter. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_ulTaskGetRunTimePercent. */
        #endif

        #if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )
            ( UBaseType_t ) MPU_ulTaskGetIdleRunTimePercentImpl,            /* 系统调用 SYSTEM_CALL_ulTaskGetIdleRunTimePercent. */
            ( UBaseType_t ) MPU_ulTaskGetIdleRunTimeCounterImpl,            /* 系统调用 SYSTEM_CALL_ulTaskGetIdleRunTimeCounter. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_ulTaskGetIdleRunTimePercent. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_ulTaskGetIdleRunTimeCounter. */
        #endif

        #if ( configUSE_APPLICATION_TASK_TAG == 1 )
            ( UBaseType_t ) MPU_vTaskSetApplicationTaskTagImpl,             /* 系统调用 SYSTEM_CALL_vTaskSetApplicationTaskTag. */
            ( UBaseType_t ) MPU_xTaskGetApplicationTaskTagImpl,             /* 系统调用 SYSTEM_CALL_xTaskGetApplicationTaskTag. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTaskSetApplicationTaskTag. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGetApplicationTaskTag. */
        #endif

        #if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )
            ( UBaseType_t ) MPU_vTaskSetThreadLocalStoragePointerImpl,      /* 系统调用 SYSTEM_CALL_vTaskSetThreadLocalStoragePointer. */
            ( UBaseType_t ) MPU_pvTaskGetThreadLocalStoragePointerImpl,     /* 系统调用 SYSTEM_CALL_pvTaskGetThreadLocalStoragePointer. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTaskSetThreadLocalStoragePointer. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_pvTaskGetThreadLocalStoragePointer. */
        #endif

        #if ( configUSE_TRACE_FACILITY == 1 )
            ( UBaseType_t ) MPU_uxTaskGetSystemStateImpl,                   /* 系统调用 SYSTEM_CALL_uxTaskGetSystemState. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_uxTaskGetSystemState. */
        #endif

        #if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
            ( UBaseType_t ) MPU_uxTaskGetStackHighWaterMarkImpl,            /* 系统调用 SYSTEM_CALL_uxTaskGetStackHighWaterMark. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_uxTaskGetStackHighWaterMark. */
        #endif

        #if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )
            ( UBaseType_t ) MPU_uxTaskGetStackHighWaterMark2Impl,           /* 系统调用 SYSTEM_CALL_uxTaskGetStackHighWaterMark2. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_uxTaskGetStackHighWaterMark2. */
        #endif

        #if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )
            ( UBaseType_t ) MPU_xTaskGetCurrentTaskHandleImpl,              /* 系统调用 SYSTEM_CALL_xTaskGetCurrentTaskHandle. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGetCurrentTaskHandle. */
        #endif

        #if ( INCLUDE_xTaskGetSchedulerState == 1 )
            ( UBaseType_t ) MPU_xTaskGetSchedulerStateImpl,                 /* 系统调用 SYSTEM_CALL_xTaskGetSchedulerState. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGetSchedulerState. */
        #endif

        ( UBaseType_t ) MPU_vTaskSetTimeOutStateImpl,                       /* 系统调用 SYSTEM_CALL_vTaskSetTimeOutState. */
        ( UBaseType_t ) MPU_xTaskCheckForTimeOutImpl,                       /* 系统调用 SYSTEM_CALL_xTaskCheckForTimeOut. */

        #if ( configUSE_TASK_NOTIFICATIONS == 1 )
            ( UBaseType_t ) MPU_ulTaskGenericNotifyTakeImpl,                /* 系统调用 SYSTEM_CALL_ulTaskGenericNotifyTake. */
            ( UBaseType_t ) MPU_xTaskGenericNotifyStateClearImpl,           /* 系统调用 SYSTEM_CALL_xTaskGenericNotifyStateClear. */
            ( UBaseType_t ) MPU_ulTaskGenericNotifyValueClearImpl,          /* 系统调用 SYSTEM_CALL_ulTaskGenericNotifyValueClear. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_ulTaskGenericNotifyTake. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTaskGenericNotifyStateClear. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_ulTaskGenericNotifyValueClear. */
        #endif

        ( UBaseType_t ) MPU_xQueueGenericSendImpl,                          /* 系统调用 SYSTEM_CALL_xQueueGenericSend. */
        ( UBaseType_t ) MPU_uxQueueMessagesWaitingImpl,                     /* 系统调用 SYSTEM_CALL_uxQueueMessagesWaiting. */
        ( UBaseType_t ) MPU_uxQueueSpacesAvailableImpl,                     /* 系统调用 SYSTEM_CALL_uxQueueSpacesAvailable. */
        ( UBaseType_t ) MPU_xQueueReceiveImpl,                              /* 系统调用 SYSTEM_CALL_xQueueReceive. */
        ( UBaseType_t ) MPU_xQueuePeekImpl,                                 /* 系统调用 SYSTEM_CALL_xQueuePeek. */
        ( UBaseType_t ) MPU_xQueueSemaphoreTakeImpl,                        /* 系统调用 SYSTEM_CALL_xQueueSemaphoreTake. */

        #if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xSemaphoreGetMutexHolder == 1 ) )
            ( UBaseType_t ) MPU_xQueueGetMutexHolderImpl,                   /* 系统调用 SYSTEM_CALL_xQueueGetMutexHolder. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xQueueGetMutexHolder. */
        #endif

        #if ( configUSE_RECURSIVE_MUTEXES == 1 )
            ( UBaseType_t ) MPU_xQueueTakeMutexRecursiveImpl,               /* 系统调用 SYSTEM_CALL_xQueueTakeMutexRecursive. */
            ( UBaseType_t ) MPU_xQueueGiveMutexRecursiveImpl,               /* 系统调用 SYSTEM_CALL_xQueueGiveMutexRecursive. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xQueueTakeMutexRecursive. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xQueueGiveMutexRecursive. */
        #endif

        #if ( configUSE_QUEUE_SETS == 1 )
            ( UBaseType_t ) MPU_xQueueSelectFromSetImpl,                    /* 系统调用 SYSTEM_CALL_xQueueSelectFromSet. */
            ( UBaseType_t ) MPU_xQueueAddToSetImpl,                         /* 系统调用 SYSTEM_CALL_xQueueAddToSet. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xQueueSelectFromSet. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xQueueAddToSet. */
        #endif

        #if configQUEUE_REGISTRY_SIZE > 0
            ( UBaseType_t ) MPU_vQueueAddToRegistryImpl,                    /* 系统调用 SYSTEM_CALL_vQueueAddToRegistry. */
            ( UBaseType_t ) MPU_vQueueUnregisterQueueImpl,                  /* 系统调用 SYSTEM_CALL_vQueueUnregisterQueue. */
            ( UBaseType_t ) MPU_pcQueueGetNameImpl,                         /* 系统调用 SYSTEM_CALL_pcQueueGetName. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vQueueAddToRegistry. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vQueueUnregisterQueue. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_pcQueueGetName. */
        #endif

        #if ( configUSE_TIMERS == 1 )
            ( UBaseType_t ) MPU_pvTimerGetTimerIDImpl,                      /* 系统调用 SYSTEM_CALL_pvTimerGetTimerID. */
            ( UBaseType_t ) MPU_vTimerSetTimerIDImpl,                       /* 系统调用 SYSTEM_CALL_vTimerSetTimerID. */
            ( UBaseType_t ) MPU_xTimerIsTimerActiveImpl,                    /* 系统调用 SYSTEM_CALL_xTimerIsTimerActive. */
            ( UBaseType_t ) MPU_xTimerGetTimerDaemonTaskHandleImpl,         /* 系统调用 SYSTEM_CALL_xTimerGetTimerDaemonTaskHandle. */
            ( UBaseType_t ) MPU_pcTimerGetNameImpl,                         /* 系统调用 SYSTEM_CALL_pcTimerGetName. */
            ( UBaseType_t ) MPU_vTimerSetReloadModeImpl,                    /* 系统调用 SYSTEM_CALL_vTimerSetReloadMode. */
            ( UBaseType_t ) MPU_xTimerGetReloadModeImpl,                    /* 系统调用 SYSTEM_CALL_xTimerGetReloadMode. */
            ( UBaseType_t ) MPU_uxTimerGetReloadModeImpl,                   /* 系统调用 SYSTEM_CALL_uxTimerGetReloadMode. */
            ( UBaseType_t ) MPU_xTimerGetPeriodImpl,                        /* 系统调用 SYSTEM_CALL_xTimerGetPeriod. */
            ( UBaseType_t ) MPU_xTimerGetExpiryTimeImpl,                    /* 系统调用 SYSTEM_CALL_xTimerGetExpiryTime. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_pvTimerGetTimerID. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTimerSetTimerID. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTimerIsTimerActive. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTimerGetTimerDaemonTaskHandle. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_pcTimerGetName. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vTimerSetReloadMode. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTimerGetReloadMode. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_uxTimerGetReloadMode. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTimerGetPeriod. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xTimerGetExpiryTime. */
        #endif

        #if ( configUSE_EVENT_GROUPS == 1 )
            ( UBaseType_t ) MPU_xEventGroupClearBitsImpl,                   /* 系统调用 SYSTEM_CALL_xEventGroupClearBits. */
            ( UBaseType_t ) MPU_xEventGroupSetBitsImpl,                     /* 系统调用 SYSTEM_CALL_xEventGroupSetBits. */
            ( UBaseType_t ) MPU_xEventGroupSyncImpl,                        /* 系统调用 SYSTEM_CALL_xEventGroupSync. */

            #if ( configUSE_TRACE_FACILITY == 1 )
                ( UBaseType_t ) MPU_uxEventGroupGetNumberImpl,              /* 系统调用 SYSTEM_CALL_uxEventGroupGetNumber. */
                ( UBaseType_t ) MPU_vEventGroupSetNumberImpl,               /* 系统调用 SYSTEM_CALL_vEventGroupSetNumber. */
            #else
                ( UBaseType_t ) 0,                                          /* 系统调用 SYSTEM_CALL_uxEventGroupGetNumber. */
                ( UBaseType_t ) 0,                                          /* 系统调用 SYSTEM_CALL_vEventGroupSetNumber. */
            #endif
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xEventGroupClearBits. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xEventGroupSetBits. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xEventGroupSync. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_uxEventGroupGetNumber. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_vEventGroupSetNumber. */
        #endif

        #if ( configUSE_STREAM_BUFFERS == 1 )
            ( UBaseType_t ) MPU_xStreamBufferSendImpl,                      /* 系统调用 SYSTEM_CALL_xStreamBufferSend. */
            ( UBaseType_t ) MPU_xStreamBufferReceiveImpl,                   /* 系统调用 SYSTEM_CALL_xStreamBufferReceive. */
            ( UBaseType_t ) MPU_xStreamBufferIsFullImpl,                    /* 系统调用 SYSTEM_CALL_xStreamBufferIsFull. */
            ( UBaseType_t ) MPU_xStreamBufferIsEmptyImpl,                   /* 系统调用 SYSTEM_CALL_xStreamBufferIsEmpty. */
            ( UBaseType_t ) MPU_xStreamBufferSpacesAvailableImpl,           /* 系统调用 SYSTEM_CALL_xStreamBufferSpacesAvailable. */
            ( UBaseType_t ) MPU_xStreamBufferBytesAvailableImpl,            /* 系统调用 SYSTEM_CALL_xStreamBufferBytesAvailable. */
            ( UBaseType_t ) MPU_xStreamBufferSetTriggerLevelImpl,           /* 系统调用 SYSTEM_CALL_xStreamBufferSetTriggerLevel. */
            ( UBaseType_t ) MPU_xStreamBufferNextMessageLengthBytesImpl     /* 系统调用 SYSTEM_CALL_xStreamBufferNextMessageLengthBytes. */
        #else
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferSend. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferReceive. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferIsFull. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferIsEmpty. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferSpacesAvailable. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferBytesAvailable. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferSetTriggerLevel. */
            ( UBaseType_t ) 0,                                              /* 系统调用 SYSTEM_CALL_xStreamBufferNextMessageLengthBytes. */
        #endif

    };
/*-----------------------------------------------------------*/

#endif /* 条件块结束：#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) ) */
/*-----------------------------------------------------------*/
