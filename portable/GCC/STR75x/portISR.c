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
 * -----------------------------------------------------------
 * 可编译为 ARM 或 THUMB 模式的组件位于 port.c 中。只能编译为 ARM
 * 模式的 ISR 例程位于本文件中。
 * ----------------------------------------------------------
 */

/*
 */

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 处理临界区所需的常量。 */
#define portNO_CRITICAL_NESTING    ( ( uint32_t ) 0 )

volatile uint32_t ulCriticalNesting = 9999UL;

/*-----------------------------------------------------------*/

/*
 * 调度器只能从 ARM 模式启动，因此在此处包含此函数。
 */
void vPortISRStartFirstTask( void );
/*-----------------------------------------------------------*/

void vPortISRStartFirstTask( void )
{
    /*
     * 简单地启动调度器。此处包含它是因为只能从 ARM 模式调用。
     */
    asm volatile (                                                        \
        "LDR        R0, =pxCurrentTCB                               \n\t" \
        "LDR        R0, [R0]                                        \n\t" \
        "LDR        LR, [R0]                                        \n\t" \
                                                                          \
        /* 临界区嵌套深度是栈上的第一项。 */  \
        /* 将其加载到 ulCriticalNesting 变量中。 */                \
        "LDR        R0, =ulCriticalNesting                          \n\t" \
        "LDMFD  LR!, {R1}                                           \n\t" \
        "STR        R1, [R0]                                        \n\t" \
                                                                          \
        /* 从栈中获取 SPSR。 */                                \
        "LDMFD  LR!, {R0}                                           \n\t" \
        "MSR        SPSR, R0                                        \n\t" \
                                                                          \
        /* 恢复任务的所有系统模式寄存器。 */             \
        "LDMFD  LR, {R0-R14}^                                       \n\t" \
        "NOP                                                        \n\t" \
                                                                          \
        /* 恢复返回地址。 */                                 \
        "LDR        LR, [LR, #+60]                                  \n\t" \
                                                                          \
        /* 然后返回 - 修正 LR 中的偏移以获得 */  \
        /* 正确的地址。 */                                            \
        "SUBS PC, LR, #4                                            \n\t" \
        );
}
/*-----------------------------------------------------------*/

void vPortTickISR( void )
{
    /*
     * 递增 RTOS tick 计数，然后查找已就绪的最高优先级任务。
     */
    if( xTaskIncrementTick() != pdFALSE )
    {
        vTaskSwitchContext();
    }

    /* 为下一个中断做好准备。 */
    TB_ClearITPendingBit( TB_IT_Update );
}

/*-----------------------------------------------------------*/

/*
 * 中断管理工具只能从 ARM 模式调用。定义 THUMB_INTERWORK 时，这些工具在此处
 * 定义为函数，以确保切换到 ARM 模式。未定义 THUMB_INTERWORK 时，这些工具与
 * 其他移植层一样在 portmacro.h 中定义为宏。
 */
#ifdef THUMB_INTERWORK

    void vPortDisableInterruptsFromThumb( void ) __attribute__( ( naked ) );
    void vPortEnableInterruptsFromThumb( void ) __attribute__( ( naked ) );

    void vPortDisableInterruptsFromThumb( void )
    {
        asm volatile (
            "STMDB  SP!, {R0}       \n\t" /* 压入 R0。 */
            "MRS    R0, CPSR        \n\t" /* 获取 CPSR。 */
            "ORR    R0, R0, #0xC0   \n\t" /* 禁止 IRQ、FIQ。 */
            "MSR    CPSR, R0        \n\t" /* 写回修改后的值。 */
            "LDMIA  SP!, {R0}       \n\t" /* 弹出 R0。 */
            "BX     R14" );               /* 返回 Thumb 模式。 */
    }

    void vPortEnableInterruptsFromThumb( void )
    {
        asm volatile (
            "STMDB  SP!, {R0}       \n\t" /* 压入 R0。 */
            "MRS    R0, CPSR        \n\t" /* 获取 CPSR。 */
            "BIC    R0, R0, #0xC0   \n\t" /* 使能 IRQ、FIQ。 */
            "MSR    CPSR, R0        \n\t" /* 写回修改后的值。 */
            "LDMIA  SP!, {R0}       \n\t" /* 弹出 R0。 */
            "BX     R14" );               /* 返回 Thumb 模式。 */
    }

#endif /* THUMB_INTERWORK（宏） */
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    /* 按 portDISABLE_INTERRUPTS() 禁止中断； */
    asm volatile (
        "STMDB  SP!, {R0}           \n\t" /* 压入 R0。 */
        "MRS    R0, CPSR            \n\t" /* 获取 CPSR。 */
        "ORR    R0, R0, #0xC0       \n\t" /* 禁止 IRQ、FIQ。 */
        "MSR    CPSR, R0            \n\t" /* 写回修改后的值。 */
        "LDMIA  SP!, {R0}" );             /* 弹出 R0。 */

    /*
     * 中断现已禁止，可以直接访问 ulCriticalNesting。递增 ulCriticalNesting，
     * 以记录 portENTER_CRITICAL() 已被调用的次数。
     */
    ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    if( ulCriticalNesting > portNO_CRITICAL_NESTING )
    {
        /* 由于正在离开临界区，递减嵌套计数。 */
        ulCriticalNesting--;

        /*
         * 若嵌套层数已降为零，则应重新使能中断。
         */
        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /* 按 portEXIT_CRITICAL() 使能中断。 */
            asm volatile (
                "STMDB  SP!, {R0}       \n\t" /* 压入 R0。 */
                "MRS    R0, CPSR        \n\t" /* 获取 CPSR。 */
                "BIC    R0, R0, #0xC0   \n\t" /* 使能 IRQ、FIQ。 */
                "MSR    CPSR, R0        \n\t" /* 写回修改后的值。 */
                "LDMIA  SP!, {R0}" );         /* 弹出 R0。 */
        }
    }
}
