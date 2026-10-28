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
 *  自 V2.5.2 起的变更
 *
 * + usCriticalNesting 现在具有 volatile 限定符。
 */

/* 标准头文件包含。 */
#include <stdlib.h>
#include <signal.h>

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/*
 * -----------------------------------------------------------
 * MSP430 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

/*
 * 硬件配置所需的常量。时钟节拍 ISR 由 ACLK 驱动，而不是 MCLK。
 */
#define portACLK_FREQUENCY_HZ           ( ( TickType_t ) 32768 )
#define portINITIAL_CRITICAL_NESTING    ( ( uint16_t ) 10 )
#define portFLAGS_INT_ENABLED           ( ( StackType_t ) 0x08 )

/*
 * 需要 pxCurrentTCB 变量的地址，但无需了解其类型的任何细节。
 */
typedef void TCB_t;
extern volatile TCB_t * volatile pxCurrentTCB;

/*
 * 大多数移植层通过在禁止中断前将中断标志压栈来实现临界区。退出临界区只需
 * 从栈中弹出标志。由于 mspgcc 不使用帧指针，无法这样做，因为修改栈会破坏
 * 所有栈变量。因此每个任务维护临界区嵌套深度的计数。每次进入临界区时计数
 * 递增，每次离开临界区时计数递减 - 仅当计数为零时才重新使能中断。
 *
 * 调度器启动时 usCriticalNesting 会被置零，但不能将其初始化为零，否则会在
 * 启动序列期间引发问题。
 */
volatile uint16_t usCriticalNesting = portINITIAL_CRITICAL_NESTING;
/*-----------------------------------------------------------*/

/*
 * 将任务上下文保存到任务栈的宏。它先将所有 msp430 通用寄存器压入栈，然后压入
 * 该任务使用的 usCriticalNesting 值。最后将所得的栈指针值保存到任务控制块中，
 * 以便任务下次执行时取回。
 */
#define portSAVE_CONTEXT()                               \
    asm volatile ( "push   r4                      \n\t" \
                   "push   r5                      \n\t" \
                   "push   r6                      \n\t" \
                   "push   r7                      \n\t" \
                   "push   r8                      \n\t" \
                   "push   r9                      \n\t" \
                   "push   r10                     \n\t" \
                   "push   r11                     \n\t" \
                   "push   r12                     \n\t" \
                   "push   r13                     \n\t" \
                   "push   r14                     \n\t" \
                   "push   r15                     \n\t" \
                   "mov.w  usCriticalNesting, r14  \n\t" \
                   "push   r14                     \n\t" \
                   "mov.w  pxCurrentTCB, r12       \n\t" \
                   "mov.w  r1, @r12                \n\t" \
                   );

/*
 * 从任务栈恢复任务上下文的宏。它实际上是 portSAVE_CONTEXT() 的逆过程。首先
 * 从任务控制块加载栈指针值。接着从栈中取回该任务使用的 usCriticalNesting 值 -
 * 然后取回所有 msp430 通用寄存器的值。
 *
 * bic 指令确保即将从栈中弹出的状态寄存器中没有置位低功耗位。
 */
#define portRESTORE_CONTEXT()                            \
    asm volatile ( "mov.w  pxCurrentTCB, r12       \n\t" \
                   "mov.w  @r12, r1                \n\t" \
                   "pop    r15                     \n\t" \
                   "mov.w  r15, usCriticalNesting  \n\t" \
                   "pop    r15                     \n\t" \
                   "pop    r14                     \n\t" \
                   "pop    r13                     \n\t" \
                   "pop    r12                     \n\t" \
                   "pop    r11                     \n\t" \
                   "pop    r10                     \n\t" \
                   "pop    r9                      \n\t" \
                   "pop    r8                      \n\t" \
                   "pop    r7                      \n\t" \
                   "pop    r6                      \n\t" \
                   "pop    r5                      \n\t" \
                   "pop    r4                      \n\t" \
                   "bic    #(0xf0),0(r1)           \n\t" \
                   "reti                           \n\t" \
                   );
/*-----------------------------------------------------------*/

/*
 * 设置用于 RTOS 时钟节拍（tick）的周期性 ISR。此处使用定时器 0，
 * 也可以改用看门狗定时器或定时器 1。
 */
static void prvSetupTimerInterrupt( void );
/*-----------------------------------------------------------*/

/*
 * 初始化任务栈，使其与调用过 portSAVE_CONTEXT 后的状态完全一致。
 *
 * 参见头文件 portable.h。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     *  在栈底放置几个已知值。
     *  这仅用于调试，需要时可以加入。
     *
     * pxTopOfStack = ( StackType_t ) 0x1111;
     *  pxTopOfStack--;
     * pxTopOfStack = ( StackType_t ) 0x2222;
     *  pxTopOfStack--;
     * pxTopOfStack = ( StackType_t ) 0x3333;
     *  pxTopOfStack--;
     */

    /*
     * msp430 在执行 ISR 之前会自动将 PC 和 SR 压入栈。我们希望栈看起来就像发生了
     * 这一过程，因此先将指向任务起始地址的指针压栈 - 然后压入任务启动时要使用的标志位。
     */
    *pxTopOfStack = ( StackType_t ) pxCode;
    pxTopOfStack--;
    *pxTopOfStack = portFLAGS_INT_ENABLED;
    pxTopOfStack--;

    /* 接下来是通用寄存器。 */
    *pxTopOfStack = ( StackType_t ) 0x4444;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x5555;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x6666;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x7777;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x8888;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x9999;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0xaaaa;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0xbbbb;
    pxTopOfStack--;
#ifdef __MSPGCC__
    *pxTopOfStack = ( StackType_t ) 0xcccc;
#else
    /* MSP430 EABI 期望函数参数位于 R12。 */
    *pxTopOfStack = ( StackType_t ) pvParameters;
#endif
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0xdddd;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0xeeee;
    pxTopOfStack--;
#ifdef __MSPGCC__
    /* mspgcc ABI 期望函数参数位于 R15。 */
    *pxTopOfStack = ( StackType_t ) pvParameters;
#else
    *pxTopOfStack = ( StackType_t ) 0xffff;
#endif
    pxTopOfStack--;

    /*
     * mspgcc 编译器生成的代码不维护独立的栈指针和帧指针。因此 portENTER_CRITICAL
     * 宏不能像其他移植层那样使用栈。改为使用变量跟踪临界区嵌套。该变量必须作为
     * 任务上下文的一部分保存，并初始化为零。
     */
    *pxTopOfStack = ( StackType_t ) portNO_CRITICAL_SECTION_NESTING;

    /*
     * 返回指向所生成栈顶的指针，以便将其存入该任务的任务控制块中。
     */
    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /*
     * 配置硬件以产生时钟节拍（tick）。调用此函数时中断已被禁止。
     */
    prvSetupTimerInterrupt();

    /* 恢复将要运行的第一个任务的上下文。 */
    portRESTORE_CONTEXT();

    /* 任务已在运行，不应执行到此处！ */
    return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * MSP430 移植层不太可能被停止。如有需要，在此处禁止时钟节拍中断即可。
     */
}
/*-----------------------------------------------------------*/

/*
 * 由 portYIELD 或 taskYIELD 调用的手动上下文切换。
 *
 * 首先要保存寄存器，以便使用 naked 属性。
 */
void vPortYield( void ) __attribute__( ( naked ) );
void vPortYield( void )
{
    /*
     * 希望被保存任务的栈看起来与在抢占式 RTOS 时钟节拍 ISR 中保存时完全一致。
     * 在调用 ISR 之前，msp430 会将状态寄存器压入栈。由于这是函数调用而非 ISR，
     * 必须手动完成此操作。
     */
    asm volatile ( "push    r2" );
    _DINT();

    /* 保存当前任务的上下文。 */
    portSAVE_CONTEXT();

    /* 切换到已就绪的最高优先级任务。 */
    vTaskSwitchContext();

    /* 恢复新任务的上下文。 */
    portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

/*
 * 产生 RTOS 时钟节拍（tick）的硬件初始化。此处使用定时器 0，
 * 也可以改用看门狗定时器或定时器 1。
 */
static void prvSetupTimerInterrupt( void )
{
    /* 确保定时器停止。 */
    TACTL = 0;

    /* 让定时器由 ACLK 驱动运行。 */
    TACTL = TASSEL_1;

    /* 先清除所有内容。 */
    TACTL |= TACLR;

    /* 根据所需的 tick 速率设置比较匹配值。 */
    TACCR0 = portACLK_FREQUENCY_HZ / configTICK_RATE_HZ;

    /* 使能中断。 */
    TACCTL0 = CCIE;

    /* 从干净状态启动。 */
    TACTL |= TACLR;

    /* 向上模式。 */
    TACTL |= MC_1;
}
/*-----------------------------------------------------------*/

/*
 * 所使用的中断服务程序取决于是否使用抢占式调度器。
 */

#if configUSE_PREEMPTION == 1

/*
 * 抢占式调度器的时钟节拍 ISR。由于上下文在 vPortYieldFromTick() 开头
 * 已保存，可以使用 naked 属性。上下文保存之后再递增 tick 计数。
 */
    interrupt( TIMERA0_VECTOR ) void prvTickISR( void ) __attribute__( ( naked ) );
    interrupt( TIMERA0_VECTOR ) void prvTickISR( void )
    {
        /* 保存被中断任务的上下文。 */
        portSAVE_CONTEXT();

        /*
         * 递增 tick 计数，然后切换到已就绪的最高优先级任务。
         */
        if( xTaskIncrementTick() != pdFALSE )
        {
            vTaskSwitchContext();
        }

        /* 恢复新任务的上下文。 */
        portRESTORE_CONTEXT();
    }

#else /* 若 configUSE_PREEMPTION == 1 */

/*
 * 协作式调度器的时钟节拍 ISR。仅递增 tick 计数。无需切换上下文，
 * 上下文切换只能通过手动调用 taskYIELD() 完成。
 */
    interrupt( TIMERA0_VECTOR ) void prvTickISR( void );
    interrupt( TIMERA0_VECTOR ) void prvTickISR( void )
    {
        xTaskIncrementTick();
    }
#endif /* 若 configUSE_PREEMPTION == 1 */
