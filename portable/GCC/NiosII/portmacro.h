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

#ifndef PORTMACRO_H
#define PORTMACRO_H

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#include "sys/alt_irq.h"

/*
 * -----------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置使 FreeRTOS 针对给定硬件和编译器正确配置。
 *
 * 不应修改这些设置。
 * -----------------------------------------------------------
 */

/* 类型定义。 */
#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE     long

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffffffffUL

/*
 * 32 位架构上使用 32 位 tick 类型，因此读取 tick 计数
 * 无需用临界区保护。
 */
    #define portTICK_TYPE_IS_ATOMIC    1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/* 架构相关细节。 */
#define portSTACK_GROWTH               ( -1 )
#define portTICK_PERIOD_MS             ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT             4
#define portNOP()    asm volatile ( "NOP" )
#define portCRITICAL_NESTING_IN_TCB    1
/*-----------------------------------------------------------*/

extern void vTaskSwitchContext( void );
#define portYIELD()    asm volatile ( "trap" );
#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    do                                           \
    {                                            \
        if( xSwitchRequired != pdFALSE )         \
        {                                        \
            traceISR_EXIT_TO_SCHEDULER();        \
            vTaskSwitchContext();                \
        }                                        \
        else                                     \
        {                                        \
            traceISR_EXIT();                     \
        }                                        \
    } while( 0 )

/* 包含定义上下文保存/恢复的 port_asm.S 文件。 */
__asm__ ( "\n\t.globl    save_context" );

/*-----------------------------------------------------------*/

extern void vTaskEnterCritical( void );
extern void vTaskExitCritical( void );

#define portDISABLE_INTERRUPTS()    alt_irq_disable_all()
#define portENABLE_INTERRUPTS()     alt_irq_enable_all( 0x01 );
#define portENTER_CRITICAL()        vTaskEnterCritical()
#define portEXIT_CRITICAL()         vTaskExitCritical()
/*-----------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* PORTMACRO_H（移植宏头文件） */
