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
 * pvPortMalloc() 与 vPortFree() 的实现，依赖编译器自带的 malloc() 和 free() 实现。
 *
 * 仅当链接器配置为生成堆内存区域时，才能使用本文件。
 *
 * 替代实现参见 heap_1.c、heap_2.c 和 heap_4.c，
 * 更多信息参见 https://www.FreeRTOS.org 的内存管理页面。
 */

#include <stdlib.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有 API 函数
 * 重定义为使用 MPU 包装器。只有在应用程序文件中包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/*-----------------------------------------------------------*/

void * pvPortMalloc( size_t xWantedSize )
{
    void * pvReturn;

    vTaskSuspendAll();
    {
        pvReturn = malloc( xWantedSize );
        traceMALLOC( pvReturn, xWantedSize );
    }
    ( void ) xTaskResumeAll();

    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
        if( pvReturn == NULL )
        {
            vApplicationMallocFailedHook();
        }
    }
    #endif

    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void * pv )
{
    if( pv != NULL )
    {
        vTaskSuspendAll();
        {
            free( pv );
            traceFREE( pv, 0 );
        }
        ( void ) xTaskResumeAll();
    }
}
/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。该状态通常在启动时初始化。
 * 应用程序必须在重启调度器之前调用此函数。
 */
void vPortHeapResetState( void )
{
    /* heap_3 无需重新初始化任何状态。 */
}
/*-----------------------------------------------------------*/
