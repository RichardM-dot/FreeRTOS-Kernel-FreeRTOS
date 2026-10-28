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

/* 硬件相关定义。 */
#define portBYTE_ALIGNMENT    4
#define portSTACK_GROWTH      -1
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
/*-----------------------------------------------------------*/
uint32_t ulPortSetIPL( uint32_t );
#define portDISABLE_INTERRUPTS()    ulPortSetIPL( configMAX_SYSCALL_INTERRUPT_PRIORITY )
#define portENABLE_INTERRUPTS()     ulPortSetIPL( 0 )


extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#define portENTER_CRITICAL()    vPortEnterCritical()
#define portEXIT_CRITICAL()     vPortExitCritical()

extern UBaseType_t uxPortSetInterruptMaskFromISR( void );
extern void vPortClearInterruptMaskFromISR( UBaseType_t );
#define portSET_INTERRUPT_MASK_FROM_ISR()                             ulPortSetIPL( configMAX_SYSCALL_INTERRUPT_PRIORITY )
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedStatusRegister )    ulPortSetIPL( uxSavedStatusRegister )

/*-----------------------------------------------------------*/

/* 任务工具函数。 */

#define portNOP()      asm volatile ( "nop" )

/* 注意这会覆盖强制寄存器中的所有其他位，这样做是为了提高速度。 */
#define portYIELD()    MCF_INTC0_INTFRCL = ( 1UL << configYIELD_INTERRUPT_VECTOR ); portNOP(); portNOP()

/*-----------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters ) __attribute__( ( noreturn ) )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
/*-----------------------------------------------------------*/

#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    do                                           \
    {                                            \
        if( xSwitchRequired != pdFALSE )         \
        {                                        \
            traceISR_EXIT_TO_SCHEDULER();        \
            portYIELD();                         \
        }                                        \
        else                                     \
        {                                        \
            traceISR_EXIT();                     \
        }                                        \
    } while( 0 )


/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* PORTMACRO_H（头文件保护宏） */
