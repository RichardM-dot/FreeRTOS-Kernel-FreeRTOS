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
 * 自 V3.2.3 的变更
 *
 * + 修改 portENTER_SWITCHING_ISR()，使其可与 GCC V4.0.1 配合使用。
 * +
 * +  自 V3.2.4 的变更
 * +
 * + 删除了汇编宏中 %0 参数的使用，
 * +    改用硬编码寄存器替代。这可确保汇编器
 * +    不会选择链接寄存器作为临时寄存器，
 * +    此前偶尔会出现这种情况。
 * +
 * + 汇编语句现在包含在单个 asm 块中，
 * +    而不是每行各自使用一个 asm 块。
 * +
 * +  自 V4.5.0 的变更
 * +
 * + 删除了 portENTER_SWITCHING_ISR() 和 portEXIT_SWITCHING_ISR() 宏，
 * +    并将其替换为 portYIELD_FROM_ISR() 宏。应用代码
 * +    现在应按照 V4.5.1 演示代码使用 portSAVE_CONTEXT() 和 portRESTORE_CONTEXT()
 * +    宏。
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

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
    #define portMAX_DELAY    ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY    ( TickType_t ) ( 0xFFFFFFFFUL )
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/* 硬件相关细节。 */
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT    8
#define portYIELD()    asm volatile ( "SWI 0" )
#define portNOP()      asm volatile ( "NOP" )

/*
 * 这些定义用于指定产生 tick 中断的定时器。
 * 它们放在本文件中，以便在 "port.c" 和 "portisr.c" 之间共享。
 */
#define portTIMER_REG_BASE_PTR      AT91C_BASE_TC0
#define portTIMER_CLK_ENABLE_BIT    AT91C_PS_TC0
#define portTIMER_AIC_CHANNEL       ( ( uint32_t ) 4 )
/*-----------------------------------------------------------*/

/* 任务工具。 */

/*
 * portRESTORE_CONTEXT、portRESTORE_CONTEXT、portENTER_SWITCHING_ISR
 * 和 portEXIT_SWITCHING_ISR 只能从 ARM 模式调用，
 * 但为提高效率而包含在此处。试图从 THUMB 模式代码调用它们
 * 将导致编译时错误。
 */

#define portRESTORE_CONTEXT()                                                 \
    {                                                                         \
        extern volatile void * volatile pxCurrentTCB;                         \
        extern volatile uint32_t ulCriticalNesting;                           \
                                                                              \
        /* Set the LR to the task stack. */                                   \
        asm volatile (                                                        \
            "LDR        R0, =pxCurrentTCB                               \n\t" \
            "LDR        R0, [R0]                                        \n\t" \
            "LDR        LR, [R0]                                        \n\t" \
                                                                              \
            /* The critical nesting depth is the first item on the stack. */  \
            /* Load it into the ulCriticalNesting variable. */                \
            "LDR        R0, =ulCriticalNesting                          \n\t" \
            "LDMFD  LR!, {R1}                                           \n\t" \
            "STR        R1, [R0]                                        \n\t" \
                                                                              \
            /* Get the SPSR from the stack. */                                \
            "LDMFD  LR!, {R0}                                           \n\t" \
            "MSR        SPSR, R0                                        \n\t" \
                                                                              \
            /* Restore all system mode registers for the task. */             \
            "LDMFD  LR, {R0-R14}^                                       \n\t" \
            "NOP                                                        \n\t" \
                                                                              \
            /* Restore the return address. */                                 \
            "LDR        LR, [LR, #+60]                                  \n\t" \
                                                                              \
            /* And return - correcting the offset in the LR to obtain the */  \
            /* correct address. */                                            \
            "SUBS   PC, LR, #4                                          \n\t" \
            );                                                                \
        ( void ) ulCriticalNesting;                                           \
        ( void ) pxCurrentTCB;                                                \
    }
/*-----------------------------------------------------------*/

#define portSAVE_CONTEXT()                                                    \
    {                                                                         \
        extern volatile void * volatile pxCurrentTCB;                         \
        extern volatile uint32_t ulCriticalNesting;                           \
                                                                              \
        /* Push R0 as we are going to use the register. */                    \
        asm volatile (                                                        \
            "STMDB  SP!, {R0}                                           \n\t" \
                                                                              \
            /* Set R0 to point to the task stack pointer. */                  \
            "STMDB  SP,{SP}^                                            \n\t" \
            "NOP                                                        \n\t" \
            "SUB    SP, SP, #4                                          \n\t" \
            "LDMIA  SP!,{R0}                                            \n\t" \
                                                                              \
            /* Push the return address onto the stack. */                     \
            "STMDB  R0!, {LR}                                           \n\t" \
                                                                              \
            /* Now we have saved LR we can use it instead of R0. */           \
            "MOV    LR, R0                                              \n\t" \
                                                                              \
            /* Pop R0 so we can save it onto the system mode stack. */        \
            "LDMIA  SP!, {R0}                                           \n\t" \
                                                                              \
            /* Push all the system mode registers onto the task stack. */     \
            "STMDB  LR,{R0-LR}^                                         \n\t" \
            "NOP                                                        \n\t" \
            "SUB    LR, LR, #60                                         \n\t" \
                                                                              \
            /* Push the SPSR onto the task stack. */                          \
            "MRS    R0, SPSR                                            \n\t" \
            "STMDB  LR!, {R0}                                           \n\t" \
                                                                              \
            "LDR    R0, =ulCriticalNesting                              \n\t" \
            "LDR    R0, [R0]                                            \n\t" \
            "STMDB  LR!, {R0}                                           \n\t" \
                                                                              \
            /* Store the new top of stack for the task. */                    \
            "LDR    R0, =pxCurrentTCB                                   \n\t" \
            "LDR    R0, [R0]                                            \n\t" \
            "STR    LR, [R0]                                            \n\t" \
            );                                                                \
        ( void ) ulCriticalNesting;                                           \
        ( void ) pxCurrentTCB;                                                \
    }

#define portYIELD_FROM_ISR()    vTaskSwitchContext()

/* 临界区处理。 */

/*
 * 中断管理工具只能从 ARM 模式调用。定义 THUMB_INTERWORK 时，
 * 这些工具在 portISR.c 中定义为函数，以确保切换到 ARM 模式；
 * 未定义 THUMB_INTERWORK 时，这些工具按其他移植层的做法在此处定义为宏。
 */

#ifdef THUMB_INTERWORK

    extern void vPortDisableInterruptsFromThumb( void ) __attribute__( ( naked ) );
    extern void vPortEnableInterruptsFromThumb( void ) __attribute__( ( naked ) );

    #define portDISABLE_INTERRUPTS()    vPortDisableInterruptsFromThumb()
    #define portENABLE_INTERRUPTS()     vPortEnableInterruptsFromThumb()

#else

    #define portDISABLE_INTERRUPTS()                                       \
    asm volatile (                                                         \
        "STMDB  SP!, {R0}       \n\t"   /* Push R0.                     */ \
        "MRS    R0, CPSR        \n\t"   /* Get CPSR.                    */ \
        "ORR    R0, R0, #0xC0   \n\t"   /* Disable IRQ, FIQ.            */ \
        "MSR    CPSR, R0        \n\t"   /* Write back modified value.   */ \
        "LDMIA  SP!, {R0}           " ) /* 弹出 R0。 */

    #define portENABLE_INTERRUPTS()                                        \
    asm volatile (                                                         \
        "STMDB  SP!, {R0}       \n\t"   /* Push R0.                     */ \
        "MRS    R0, CPSR        \n\t"   /* Get CPSR.                    */ \
        "BIC    R0, R0, #0xC0   \n\t"   /* Enable IRQ, FIQ.             */ \
        "MSR    CPSR, R0        \n\t"   /* Write back modified value.   */ \
        "LDMIA  SP!, {R0}           " ) /* 弹出 R0。 */

#endif /* THUMB_INTERWORK（Thumb 与 ARM 互操作） */

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portENTER_CRITICAL()    vPortEnterCritical();
#define portEXIT_CRITICAL()     vPortExitCritical();

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
