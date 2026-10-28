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
#define portBYTE_ALIGNMENT    1
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portYIELD()    __asm( "swi" );
/*-----------------------------------------------------------*/

/* 临界区处理。 */
#define portENABLE_INTERRUPTS()     __asm( "cli" )
#define portDISABLE_INTERRUPTS()    __asm( "sei" )

/*
 * 在递增临界区嵌套计数之前禁止中断。维护嵌套计数是为了知道何时应重新
 * 使能中断。中断一旦禁止，就可以直接访问嵌套计数。每个任务维护各自的嵌套计数。
 */
#define portENTER_CRITICAL()                           \
    {                                                  \
        extern volatile UBaseType_t uxCriticalNesting; \
                                                       \
        portDISABLE_INTERRUPTS();                      \
        uxCriticalNesting++;                           \
    }

/*
 * 中断已禁止，可以直接访问嵌套计数。若发现嵌套计数为 0（无嵌套），
 * 则表示正在离开临界区，可以重新使能中断。
 */
#define  portEXIT_CRITICAL()                           \
    {                                                  \
        extern volatile UBaseType_t uxCriticalNesting; \
                                                       \
        uxCriticalNesting--;                           \
        if( uxCriticalNesting == 0 )                   \
        {                                              \
            portENABLE_INTERRUPTS();                   \
        }                                              \
    }
/*-----------------------------------------------------------*/

/* 任务工具函数。 */

/*
 * 这些宏非常简单，因为处理器在进入和退出中断时会自动保存和恢复其寄存器。
 * 除（自动压栈的）寄存器外，还将临界区嵌套计数压栈。每个任务维护各自的临界区
 * 嵌套计数，因为任务在临界区内让出 CPU 是合法的。若使用分 bank 存储模型，
 * PPAGE 寄存器也会作为任务上下文的一部分保存。
 */

#ifdef BANKED_MODEL

/*
 * 加载任务的栈指针，然后从栈中弹出临界区嵌套计数和 PPAGE 寄存器。
 * 其余上下文由 RTI 指令恢复。
 */
    #define portRESTORE_CONTEXT() \
    {                             \
        __asm( "                                \n\
        .globl pxCurrentTCB         ; void *            \n\
        .globl uxCriticalNesting        ; char              \n\
                                            \n\
        ldx  pxCurrentTCB                           \n\
        lds  0,x                ; Stack             \n\
                                            \n\
        movb 1,sp+,uxCriticalNesting                        \n\
        movb 1,sp+,0x30             ; PPAGE             \n\
        " );                      \
    }

/*
 * 调用此宏时处理器已压栈保存寄存器。只需将嵌套计数和 PPAGE 值压栈，
 * 然后保存任务栈指针。
 */
    #define portSAVE_CONTEXT() \
    {                          \
        __asm( "                                \n\
        .globl pxCurrentTCB         ; void *            \n\
        .globl uxCriticalNesting        ; char              \n\
                                            \n\
        movb 0x30, 1,-sp            ; PPAGE             \n\
        movb uxCriticalNesting, 1,-sp                       \n\
                                            \n\
        ldx  pxCurrentTCB                           \n\
        sts  0,x                ; Stack             \n\
        " );                   \
    }
#else /* 若定义了 BANKED_MODEL */

/*
 * 这些宏与上述 BANKED 版本相同，但不保存和恢复 PPAGE 寄存器。
 */

    #define portRESTORE_CONTEXT() \
    {                             \
        __asm( "                                \n\
        .globl pxCurrentTCB         ; void *            \n\
        .globl uxCriticalNesting        ; char              \n\
                                            \n\
        ldx  pxCurrentTCB                           \n\
        lds  0,x                ; Stack             \n\
                                            \n\
        movb 1,sp+,uxCriticalNesting                        \n\
        " );                      \
    }

    #define portSAVE_CONTEXT() \
    {                          \
        __asm( "                                \n\
        .globl pxCurrentTCB         ; void *            \n\
        .globl uxCriticalNesting        ; char              \n\
                                            \n\
        movb uxCriticalNesting, 1,-sp                       \n\
                                            \n\
        ldx  pxCurrentTCB                           \n\
        sts  0,x                ; Stack             \n\
        " );                   \
    }
#endif /* 若定义了 BANKED_MODEL */

/*
 * 用于为 GCC 保存/恢复正确软件寄存器的工具宏。当 GCC 未生成适当的 ISR
 * 头/尾代码时很有用。
 */
#define portISR_HEAD() \
    {                  \
        __asm( "                                 \n\
        movw _.frame, 2,-sp                         \n\
        movw _.tmp, 2,-sp                           \n\
        movw _.z, 2,-sp                             \n\
        movw _.xy, 2,-sp                            \n\
        ;movw _.d2, 2,-sp                           \n\
        ;movw _.d1, 2,-sp                           \n\
        " );           \
    }

#define portISR_TAIL() \
    {                  \
        __asm( "                                 \n\
        movw 2,sp+, _.xy                            \n\
        movw 2,sp+, _.z                             \n\
        movw 2,sp+, _.tmp                           \n\
        movw 2,sp+, _.frame                         \n\
        ;movw 2,sp+, _.d1                           \n\
        ;movw 2,sp+, _.d2                           \n\
        rti                                 \n\
        " );           \
    }

/*
 * 用于按正确顺序调用上述宏、以便在标准 ISR 中执行任务切换的工具宏。
 * 仅当 ISR 不使用任何局部（栈）变量时才能使用此宏。若 ISR 使用栈变量，
 * 应改用 portYIELD()。
 */

#define portTASK_SWITCH_FROM_ISR() \
    portSAVE_CONTEXT();            \
    vTaskSwitchContext();          \
    portRESTORE_CONTEXT();


/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* PORTMACRO_H（头文件保护宏） */
