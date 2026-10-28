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

/*
 * 使用 FIT 配置器或 Smart Configurator 时，必须使用 platform.h。
 */
#ifndef configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H
    #define configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H    0
#endif

/*
 * 类型定义 - 这些定义有些过时，现在除 portSTACK_TYPE 和 portBASE_TYPE 外
 * 实际已不再使用。
 */
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
 * 32 位体系结构上使用 32 位 tick 类型，因此读取 tick 计数无需用临界区保护。
 */
    #define portTICK_TYPE_IS_ATOMIC    1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/* 硬件相关定义。 */
#define portBYTE_ALIGNMENT    8         /* 根据手册，可以设为 4。 */
#define portSTACK_GROWTH      -1
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portNOP()    __asm volatile ( "NOP" )

/*
 * 让出（yield）等效于 "*portITU_SWINTR = 0x01; ( void ) *portITU_SWINTR;"，
 * 其中 portITU_SWINTR 是软件中断寄存器的地址（0x000872E0）。不要依赖汇编器
 * 选择寄存器，因此改为手动保存和恢复被破坏的寄存器。
 */
#define portYIELD()                     \
    __asm volatile                      \
    (                                   \
        "PUSH.L R10                 \n" \
        "MOV.L  #0x872E0, R10       \n" \
        "MOV.B  #0x1, [R10]         \n" \
        "MOV.L  [R10], R10          \n" \
        "POP    R10                 \n" \
    )

#define portYIELD_FROM_ISR( x )                           if( x != pdFALSE ) portYIELD( )

/*
 * 这些宏不应直接调用，而应通过 taskENTER_CRITICAL() 和 taskEXIT_CRITICAL()
 * 宏调用。若定义了 configASSERT()，则执行额外检查，以确保当 ISR 安全的 FreeRTOS
 * API 函数执行时发现 IPL 值高于 configMAX_SYSCALL_INTERRUPT_PRIORITY 而触发
 * 断言时，断言处理程序不会无意中尝试降低 IPL。ISR 安全的 FreeRTOS API 函数是
 * 以 FromISR 结尾的函数。FreeRTOS 维护单独的中断 API，以确保 API 函数和中断
 * 入口尽可能快且简单。
 */
#define portENABLE_INTERRUPTS()                           __asm volatile ( "MVTIPL    #0" )
#if ( configASSERT_DEFINED == 1 )
    #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()    configASSERT( ( ulPortGetIPL() <= configMAX_SYSCALL_INTERRUPT_PRIORITY ) )
    #define portDISABLE_INTERRUPTS()                      if( ulPortGetIPL() < configMAX_SYSCALL_INTERRUPT_PRIORITY ) __asm volatile( "MVTIPL    %0" ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) )
#else
    #define portDISABLE_INTERRUPTS()                      __asm volatile ( "MVTIPL    %0" ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) )
#endif

/* 临界区嵌套计数存储在 TCB 中。 */
#define portCRITICAL_NESTING_IN_TCB    ( 1 )

/* 在 tasks.c 中定义的临界区嵌套函数。 */
extern void vTaskEnterCritical( void );
extern void vTaskExitCritical( void );
#define portENTER_CRITICAL()    vTaskEnterCritical()
#define portEXIT_CRITICAL()     vTaskExitCritical()

/* 由于此移植层允许中断嵌套…… */
uint32_t ulPortGetIPL( void ) __attribute__( ( naked ) );
void vPortSetIPL( uint32_t ulNewIPL ) __attribute__( ( naked ) );
#define portSET_INTERRUPT_MASK_FROM_ISR()                              ulPortGetIPL(); portDISABLE_INTERRUPTS()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus )    vPortSetIPL( uxSavedInterruptStatus )

/*-----------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

#define portMEMORY_BARRIER()                                  __asm volatile ( "" ::: "memory" )

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* PORTMACRO_H（头文件保护宏） */
