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

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进） */

#include <machine/cpu.h>

/*
 * -----------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置使 FreeRTOS 针对给定的硬件和编译器正确配置。
 *
 * 这些设置不应被修改。
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
    #define portMAX_DELAY    ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY    ( TickType_t ) ( 0xFFFFFFFFUL )
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/* 体系结构相关定义。 */
#define portSTACK_GROWTH               ( -1 )
#define portTICK_PERIOD_MS             ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT             4
#define portNOP()    __asm__ volatile ( "mov r0, r0" )
#define portCRITICAL_NESTING_IN_TCB    1
#define portIRQ_TRAP_YIELD             31
/*-----------------------------------------------------------*/

/* 任务工具函数。 */

extern void vPortYield( void );

/*---------------------------------------------------------------------------*/

#define portYIELD()    asm __volatile__ ( " trap #%0 " : : "i" ( portIRQ_TRAP_YIELD ) : "memory" )
/*---------------------------------------------------------------------------*/

extern void vTaskEnterCritical( void );
extern void vTaskExitCritical( void );
#define portENTER_CRITICAL()    vTaskEnterCritical()
#define portEXIT_CRITICAL()     vTaskExitCritical()
/*---------------------------------------------------------------------------*/

/* 临界区管理。 */
#define portDISABLE_INTERRUPTS()    cpu_int_disable()
#define portENABLE_INTERRUPTS()     cpu_int_enable()

/*---------------------------------------------------------------------------*/

#define portYIELD_FROM_ISR( xHigherPriorityTaskWoken )    do { if( xHigherPriorityTaskWoken != pdFALSE ) vTaskSwitchContext( ); } while( 0 )

/*---------------------------------------------------------------------------*/

#define portSAVE_CONTEXT()                                                                                      \
    asm __volatile__                                                                                            \
    (                                                                                                           \
        "sub    r1, #68                 \n" /* Make space on the stack for the context. */                      \
        "std    r2, [r1] +  0           \n"                                                                     \
        "stq    r4, [r1] +  8           \n"                                                                     \
        "stq    r8, [r1] +  24          \n"                                                                     \
        "stq    r12, [r1] + 40          \n"                                                                     \
        "mov    r6, rtt                 \n"                                                                     \
        "mov    r7, psr                 \n"                                                                     \
        "std    r6, [r1] +  56          \n"                                                                     \
        "movhi  r2, #16384              \n"         /* Set the pointer to the IC. */                            \
        "ldub   r3, [r2] + 2            \n"         /* Load the current interrupt mask. */                      \
        "st     r3, [r1]+ 64            \n"         /* Store the interrupt mask on the stack. */                \
        "ld     r2, [r0]+short(pxCurrentTCB)    \n" /* Load the pointer to the TCB. */                          \
        "st     r1, [r2]                \n"         /* Save the stack pointer into the TCB. */                  \
        "mov    r14, r1                 \n"         /* Compiler expects r14 to be set to the function stack. */ \
    );
/*---------------------------------------------------------------------------*/

#define portRESTORE_CONTEXT()                                                                                 \
    asm __volatile__ (                                                                                        \
        "ld     r2, [r0]+short(pxCurrentTCB)    \n" /* Load the TCB to find the stack pointer and context. */ \
        "ld     r1, [r2]                \n"                                                                   \
        "movhi  r2, #16384              \n"         /* Set the pointer to the IC. */                          \
        "ld     r3, [r1] + 64           \n"         /* Load the previous interrupt mask. */                   \
        "stb    r3, [r2] + 2            \n"         /* Set the current interrupt mask to be the previous. */  \
        "ldd    r6, [r1] + 56           \n"         /* Restore context. */                                    \
        "mov    rtt, r6                 \n"                                                                   \
        "mov    psr, r7                 \n"                                                                   \
        "ldd    r2, [r1] + 0            \n"                                                                   \
        "ldq    r4, [r1] +  8           \n"                                                                   \
        "ldq    r8, [r1] +  24          \n"                                                                   \
        "ldq    r12, [r1] + 40          \n"                                                                   \
        "add    r1, #68                 \n"                                                                   \
        "rti                            \n"                                                                   \
        );

/*---------------------------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
/*---------------------------------------------------------------------------*/

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* PORTMACRO_H（头文件保护宏） */
