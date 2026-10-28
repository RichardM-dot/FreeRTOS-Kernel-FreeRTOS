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
 * 可编译为 ARM 或 THUMB 模式的组件包含在 port.c 中。
 * 只能编译为 ARM 模式的 ISR 例程包含在本文件中。
 * ----------------------------------------------------------
 */

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* 处理中断所需的常量。 */
#define portTIMER_MATCH_ISR_BIT    ( ( uint8_t ) 0x01 )
#define portCLEAR_VIC_INTERRUPT    ( ( uint32_t ) 0 )

/* 处理临界区所需的常量。 */
#define portNO_CRITICAL_NESTING    ( ( uint32_t ) 0 )
volatile uint32_t ulCriticalNesting = 9999UL;

/*-----------------------------------------------------------*/

/* 处理手动上下文切换（来自 taskYIELD() 调用）的 ISR。 */
void vPortYieldProcessor( void ) __attribute__( ( interrupt( "SWI" ), naked ) );

/*
 * 调度器只能从 ARM 模式启动，因此在此处包含该函数。
 */
void vPortISRStartFirstTask( void );
/*-----------------------------------------------------------*/

void vPortISRStartFirstTask( void )
{
    /*
     * 直接启动调度器。之所以包含在此处，是因为它只能从 ARM 模式调用。
     */
    portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

/*
 * 由 portYIELD() 或 taskYIELD() 调用，用于手动强制上下文切换。
 *
 * 从任务级执行上下文切换时，保存的任务上下文会被构造成
 * 如同在 tick ISR 中发生切换一样。这样，在恢复从 ISR 保存的上下文
 * 或从 vPortYieldProcessor 调用保存的上下文时，可以使用同一个恢复上下文函数。
 */
void vPortYieldProcessor( void )
{
    /*
     * 在 IRQ ISR 中，链接寄存器与真实返回地址之间存在偏移，而 SWI ISR 没有。
     * 手动加上该偏移，以便两种情况都能使用相同的 ISR 返回代码。
     */
    __asm volatile ( "ADD       LR, LR, #4" );

    /* 执行上下文切换。首先保存当前任务的上下文。 */
    portSAVE_CONTEXT();

    /* 查找已就绪的最高优先级任务。 */
    __asm volatile ( "bl         vTaskSwitchContext" );

    /* 恢复新任务的上下文。 */
    portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

/*
 * 调度器 tick 使用的 ISR 取决于使用协作式还是抢占式调度器。
 */


#if configUSE_PREEMPTION == 0

/*
 * 协作式调度器需要一个常规 IRQ 服务例程来简单地递增系统 tick。
 */
    void vNonPreemptiveTick( void ) __attribute__( ( interrupt( "IRQ" ) ) );
    void vNonPreemptiveTick( void )
    {
        xTaskIncrementTick();
        T0IR = 2;
        VICVectAddr = portCLEAR_VIC_INTERRUPT;
    }

#else /* if configUSE_PREEMPTION == 0（未启用抢占） */

/*
 * 抢占式调度器定义为 "naked"（裸函数），因为入口处会作为上下文切换的一部分保存完整上下文。
 */
    void vPreemptiveTick( void ) __attribute__( ( naked ) );
    void vPreemptiveTick( void )
    {
        /* 保存被中断任务的上下文。 */
        portSAVE_CONTEXT();

        /*
         * 递增 RTOS tick 计数，然后查找已就绪的最高优先级任务。
         */
        __asm volatile
        (
            "   bl xTaskIncrementTick   \t\n" \
            "   cmp r0, #0              \t\n" \
            "   beq SkipContextSwitch   \t\n" \
            "   bl vTaskSwitchContext   \t\n" \
            "SkipContextSwitch:         \t\n"
        );

        /* 准备下一次中断。 */
        T0IR = 2;
        VICVectAddr = portCLEAR_VIC_INTERRUPT;

        /* 恢复新任务的上下文。 */
        portRESTORE_CONTEXT();
    }

#endif /* if configUSE_PREEMPTION == 0（未启用抢占） */
/*-----------------------------------------------------------*/

/*
 * 中断管理工具只能从 ARM 模式调用。定义 THUMB_INTERWORK 时，
 * 这些工具在此处定义为函数，以确保切换到 ARM 模式；
 * 未定义 THUMB_INTERWORK 时，这些工具按其他移植层的做法在 portmacro.h 中定义为宏。
 */
#ifdef THUMB_INTERWORK

    void vPortDisableInterruptsFromThumb( void ) __attribute__( ( naked ) );
    void vPortEnableInterruptsFromThumb( void ) __attribute__( ( naked ) );

    void vPortDisableInterruptsFromThumb( void )
    {
        __asm volatile (
            "STMDB  SP!, {R0}       \n\t" /* 压入 R0。 */
            "MRS    R0, CPSR        \n\t" /* 获取 CPSR。 */
            "ORR    R0, R0, #0xC0   \n\t" /* 禁用 IRQ、FIQ。 */
            "MSR    CPSR, R0        \n\t" /* 回写修改后的值。 */
            "LDMIA  SP!, {R0}       \n\t" /* 弹出 R0。 */
            "BX     R14" );               /* 返回 Thumb 模式。 */
    }

    void vPortEnableInterruptsFromThumb( void )
    {
        __asm volatile (
            "STMDB  SP!, {R0}       \n\t" /* 压入 R0。 */
            "MRS    R0, CPSR        \n\t" /* 获取 CPSR。 */
            "BIC    R0, R0, #0xC0   \n\t" /* 使能 IRQ、FIQ。 */
            "MSR    CPSR, R0        \n\t" /* 回写修改后的值。 */
            "LDMIA  SP!, {R0}       \n\t" /* 弹出 R0。 */
            "BX     R14" );               /* 返回 Thumb 模式。 */
    }

#endif /* THUMB_INTERWORK（Thumb 与 ARM 互操作） */

/*
 * GCC 编译器生成的代码在不同优化级别下以不同方式使用栈。
 * 因此中断标志无法总是保存到栈上。取而代之的是将临界区嵌套层次
 * 保存到变量中，该变量再作为栈上下文的一部分保存。
 */
void vPortEnterCritical( void )
{
    /* 按 portDISABLE_INTERRUPTS() 的方式禁用中断。 */
    __asm volatile (
        "STMDB  SP!, {R0}           \n\t" /* 压入 R0。 */
        "MRS    R0, CPSR            \n\t" /* 获取 CPSR。 */
        "ORR    R0, R0, #0xC0       \n\t" /* 禁用 IRQ、FIQ。 */
        "MSR    CPSR, R0            \n\t" /* 回写修改后的值。 */
        "LDMIA  SP!, {R0}" );             /* 弹出 R0。 */

    /*
     * 现在中断已禁用，可以直接访问 ulCriticalNesting。
     * 递增 ulCriticalNesting，以记录 portENTER_CRITICAL() 被调用的次数。
     */
    ulCriticalNesting++;
}

void vPortExitCritical( void )
{
    if( ulCriticalNesting > portNO_CRITICAL_NESTING )
    {
        /* 离开临界区时递减嵌套计数。 */
        ulCriticalNesting--;

        /*
         * 若嵌套层次已归零，则应重新使能中断。
         */
        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /* 按 portEXIT_CRITICAL() 的方式使能中断。 */
            __asm volatile (
                "STMDB  SP!, {R0}       \n\t" /* 压入 R0。 */
                "MRS    R0, CPSR        \n\t" /* 获取 CPSR。 */
                "BIC    R0, R0, #0xC0   \n\t" /* 使能 IRQ、FIQ。 */
                "MSR    CPSR, R0        \n\t" /* 回写修改后的值。 */
                "LDMIA  SP!, {R0}" );         /* 弹出 R0。 */
        }
    }
}
