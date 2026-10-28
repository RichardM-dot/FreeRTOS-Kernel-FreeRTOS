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

#define portCHAR                 char
#define portFLOAT                float
#define portDOUBLE               double
#define portLONG                 long
#define portSHORT                short
#define portSTACK_TYPE           uint16_t
#define portBASE_TYPE            short
#define portPOINTER_SIZE_TYPE    uint16_t

typedef portSTACK_TYPE   StackType_t;
typedef short            BaseType_t;
typedef unsigned short   UBaseType_t;

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

/* 中断控制宏。 */
#define portDISABLE_INTERRUPTS()    __asm volatile ( "DI" )
#define portENABLE_INTERRUPTS()     __asm volatile ( "EI" )
/*-----------------------------------------------------------*/

/* 临界区控制宏。 */
#define portNO_CRITICAL_SECTION_NESTING    ( ( unsigned short ) 0 )

#define portENTER_CRITICAL()                                                      \
    {                                                                             \
        extern volatile uint16_t usCriticalNesting;                               \
                                                                                  \
        portDISABLE_INTERRUPTS();                                                 \
                                                                                  \
        /* Now that interrupts are disabled, ulCriticalNesting can be accessed */ \
        /* directly.  Increment ulCriticalNesting to keep a count of how many */  \
        /* times portENTER_CRITICAL() has been called. */                         \
        usCriticalNesting++;                                                      \
    }

#define portEXIT_CRITICAL()                                                         \
    {                                                                               \
        extern volatile uint16_t usCriticalNesting;                                 \
                                                                                    \
        if( usCriticalNesting > portNO_CRITICAL_SECTION_NESTING )                   \
        {                                                                           \
            /* Decrement the nesting count as we are leaving a critical section. */ \
            usCriticalNesting--;                                                    \
                                                                                    \
            /* If the nesting level has reached zero then interrupts should be */   \
            /* re-enabled. */                                                       \
            if( usCriticalNesting == portNO_CRITICAL_SECTION_NESTING )              \
            {                                                                       \
                portENABLE_INTERRUPTS();                                            \
            }                                                                       \
        }                                                                           \
    }
/*-----------------------------------------------------------*/

/* 任务工具函数。 */
#define portYIELD()                                       __asm volatile ( "BRK" )
#ifndef configREQUIRE_ASM_ISR_WRAPPER
    #define configREQUIRE_ASM_ISR_WRAPPER    1
#endif
#if( configREQUIRE_ASM_ISR_WRAPPER == 1 )
    /*
     * 若需要 ISR 触发上下文切换，必须实现汇编 ISR 包装函数（详见下文）。
     * https://www.freertos.org/Documentation/02-Kernel/03-Supported-devices/04-Demos/Renesas/RTOS_RL78_IAR_Demos#writing-interrupt-service-routines
     */
    #define portYIELD_FROM_ISR( xHigherPriorityTaskWoken )    do { if( xHigherPriorityTaskWoken != pdFALSE ) vTaskSwitchContext(); } while( 0 )
#else
    /*
     * 即使需要 ISR 触发上下文切换，也不得实现汇编 ISR 包装函数。
     * portYIELD 的作用类似于汇编 ISR 包装函数，仅在需要上下文切换时运行。
     */
    #define portYIELD_FROM_ISR( xHigherPriorityTaskWoken )    do { if( xHigherPriorityTaskWoken != pdFALSE ) portYIELD(); } while( 0 )
#endif
#define portNOP()                                         __asm volatile ( "NOP" )
/*-----------------------------------------------------------*/

/* 硬件相关定义。 */
#define portBYTE_ALIGNMENT    2
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
/*-----------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

#endif /* PORTMACRO_H（头文件保护宏） */
