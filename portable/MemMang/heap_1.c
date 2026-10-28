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
 * pvPortMalloc() 的最简单实现。注意，该实现
 * 不允许释放已分配的内存。
 *
 * 替代实现参见 heap_2.c、heap_3.c 和 heap_4.c，
 * 更多信息参见 https://www.FreeRTOS.org 的内存管理页面。
 */
#include <stdlib.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有
 * API 函数重定义为使用 MPU 包装器。只有在应用程序文件中包含 task.h
 * 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* 堆起始地址进行字节对齐时可能会损失几个字节。 */
#define configADJUSTED_HEAP_SIZE        ( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/* size_t 类型能容纳的最大值。 */
#define heapSIZE_MAX                    ( ~( ( size_t ) 0 ) )

/* 检查 a 与 b 相加是否会溢出。 */
#define heapADD_WILL_OVERFLOW( a, b )   ( ( a ) > ( heapSIZE_MAX - ( b ) ) )

/*-----------------------------------------------------------*/

/* 为堆分配内存。 */
#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )

/*
 * 应用程序编写者已自行定义 RTOS 堆所用的数组——
 * 通常是为了将其放置在特定的段或地址。
 */
    extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
    static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* 配置项 configAPPLICATION_ALLOCATED_HEAP */

/* ucHeap 数组的索引。 */
static size_t xNextFreeByte = ( size_t ) 0U;

/*-----------------------------------------------------------*/

void * pvPortMalloc( size_t xWantedSize )
{
    void * pvReturn = NULL;
    static uint8_t * pucAlignedHeap = NULL;

    /* 确保块始终对齐。 */
    #if ( portBYTE_ALIGNMENT != 1 )
    {
        size_t xAdditionalRequiredSize;

        if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
        {
            /* 需要进行字节对齐。 */
            xAdditionalRequiredSize = portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK );

            if( heapADD_WILL_OVERFLOW( xWantedSize, xAdditionalRequiredSize ) == 0 )
            {
                xWantedSize += xAdditionalRequiredSize;
            }
            else
            {
                xWantedSize = 0;
            }
        }
    }
    #endif /* 条件块结束：if ( portBYTE_ALIGNMENT != 1 ) */

    vTaskSuspendAll();
    {
        if( pucAlignedHeap == NULL )
        {
            /* 确保堆起始于正确对齐的边界。 */
            pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE ) &( ucHeap[ portBYTE_ALIGNMENT - 1 ] ) ) &
                                             ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );
        }

        /* 检查剩余空间是否足以完成本次分配。 */
        if( ( xWantedSize > 0 ) &&
            ( heapADD_WILL_OVERFLOW( xNextFreeByte, xWantedSize ) == 0 ) &&
            ( ( xNextFreeByte + xWantedSize ) < configADJUSTED_HEAP_SIZE ) )
        {
            /*
             * 返回下一个空闲字节的地址，然后将索引递增越过
             * 该块。
             */
            pvReturn = pucAlignedHeap + xNextFreeByte;
            xNextFreeByte += xWantedSize;
        }

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
    /*
     * 该方案无法释放内存。替代实现参见 heap_2.c、heap_3.c 和
     * heap_4.c，更多信息参见
     * https://www.FreeRTOS.org 的内存管理页面。
     */
    ( void ) pv;

    /* 调用该函数是无效操作，因此强制触发断言。 */
    configASSERT( pv == NULL );
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* 仅当静态内存未被清零时才需要调用。 */
    xNextFreeByte = ( size_t ) 0;
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return( configADJUSTED_HEAP_SIZE - xNextFreeByte );
}

/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。该状态通常在启动时初始化。
 * 应用程序必须在重启调度器之前调用此函数。
 */
void vPortHeapResetState( void )
{
    xNextFreeByte = ( size_t ) 0U;
}
/*-----------------------------------------------------------*/
