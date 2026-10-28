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
#define portSTACK_TYPE    uint8_t
#define portBASE_TYPE     char

typedef portSTACK_TYPE   StackType_t;
typedef signed char      BaseType_t;
typedef unsigned char    UBaseType_t;

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
#define portBYTE_ALIGNMENT    2
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portYIELD()    asm volatile ( "TRAPA #0" )
#define portNOP()      asm volatile ( "NOP" )
/*-----------------------------------------------------------*/

/* 临界区处理。 */
#define portENABLE_INTERRUPTS()     asm volatile ( "ANDC #0x7F, CCR" );
#define portDISABLE_INTERRUPTS()    asm volatile ( "ORC  #0x80, CCR" );

/* 压入 CCR，然后禁止中断。 */
#define portENTER_CRITICAL()            \
    asm volatile ( "STC  CCR, @-ER7" ); \
    portDISABLE_INTERRUPTS();

/* 弹出 CCR，将中断屏蔽恢复为先前状态。 */
#define  portEXIT_CRITICAL()    asm volatile ( "LDC  @ER7+, CCR" );
/*-----------------------------------------------------------*/

/* 任务工具函数。 */

/*
 * 上下文切换宏。这些宏非常简单，因为只需选择上下文切换中断服务程序的
 * saveall 属性即可保存上下文。这些宏负责将栈指针保存到 TCB 及从 TCB 恢复。
 */

#define portSAVE_STACK_POINTER()                  \
    extern void * pxCurrentTCB;                   \
                                                  \
    asm volatile (                                \
        "MOV.L  @_pxCurrentTCB, ER5         \n\t" \
        "MOV.L  ER7, @ER5                   \n\t" \
        );                                        \
    ( void ) pxCurrentTCB;


#define portRESTORE_STACK_POINTER()               \
    extern void * pxCurrentTCB;                   \
                                                  \
    asm volatile (                                \
        "MOV.L  @_pxCurrentTCB, ER5         \n\t" \
        "MOV.L  @ER5, ER7                   \n\t" \
        );                                        \
    ( void ) pxCurrentTCB;

/*-----------------------------------------------------------*/

/* 允许在应用程序 ISR 中执行上下文切换的宏。 */

#define portENTER_SWITCHING_ISR()    portSAVE_STACK_POINTER(); {
#define portEXIT_SWITCHING_ISR( x )             \
    if( x )                                     \
    {                                           \
        extern void vTaskSwitchContext( void ); \
        vTaskSwitchContext();                   \
    }                                           \
}                                               \
    portRESTORE_STACK_POINTER();
/*-----------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* PORTMACRO_H（头文件保护宏） */
