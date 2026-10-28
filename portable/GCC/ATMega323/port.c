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
 * 自 V2.6.0 起的变更
 *
 * + AVR 移植层 - 用直接内存访问替换了 inb() 和 outb() 函数。
 * +    这使得该移植层可以使用 20050414 版本的 WinAVR 构建。
 */

#include <stdlib.h>
#include <avr/interrupt.h>

#include "FreeRTOS.h"
#include "task.h"

/*
 * -----------------------------------------------------------
 * AVR 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

/* 在中断使能的状态下启动任务。 */
#define portFLAGS_INT_ENABLED                   ( ( StackType_t ) 0x80 )

/* 定时器 1 的硬件常量。 */
#define portCLEAR_COUNTER_ON_MATCH              ( ( uint8_t ) 0x08 )
#define portPRESCALE_64                         ( ( uint8_t ) 0x03 )
#define portCLOCK_PRESCALER                     ( ( uint32_t ) 64 )
#define portCOMPARE_MATCH_A_INTERRUPT_ENABLE    ( ( uint8_t ) 0x10 )

/*-----------------------------------------------------------*/

/*
 * 需要 pxCurrentTCB 变量的地址，但无需了解其类型的任何细节。
 */
typedef void TCB_t;
extern volatile TCB_t * volatile pxCurrentTCB;

/*-----------------------------------------------------------*/

/*
 * 保存所有通用寄存器的宏，并将栈指针保存到 TCB 中。
 *
 * 执行的第一步是保存标志位然后禁止中断。这样可防止在寄存器已压入栈之后
 * 又发生上下文切换中断，导致 32 个寄存器在栈上出现两次。
 *
 * r1 被置零，因为编译器期望如此，不过某些数学例程会使用 R1。
 *
 * 调用 portSAVE_CONTEXT() 期间中断已被禁止，因此无需担心对栈指针的读写。
 */

#define portSAVE_CONTEXT()                               \
    asm volatile ( "push   r0                      \n\t" \
                   "in     r0, __SREG__            \n\t" \
                   "cli                            \n\t" \
                   "push   r0                      \n\t" \
                   "push   r1                      \n\t" \
                   "clr    r1                      \n\t" \
                   "push   r2                      \n\t" \
                   "push   r3                      \n\t" \
                   "push   r4                      \n\t" \
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
                   "push   r16                     \n\t" \
                   "push   r17                     \n\t" \
                   "push   r18                     \n\t" \
                   "push   r19                     \n\t" \
                   "push   r20                     \n\t" \
                   "push   r21                     \n\t" \
                   "push   r22                     \n\t" \
                   "push   r23                     \n\t" \
                   "push   r24                     \n\t" \
                   "push   r25                     \n\t" \
                   "push   r26                     \n\t" \
                   "push   r27                     \n\t" \
                   "push   r28                     \n\t" \
                   "push   r29                     \n\t" \
                   "push   r30                     \n\t" \
                   "push   r31                     \n\t" \
                   "lds    r26, pxCurrentTCB       \n\t" \
                   "lds    r27, pxCurrentTCB + 1   \n\t" \
                   "in     r0, 0x3d                \n\t" \
                   "st     x+, r0                  \n\t" \
                   "in     r0, 0x3e                \n\t" \
                   "st     x+, r0                  \n\t" \
                   );

/*
 * 与 portSAVE_CONTEXT() 相反。上下文保存期间中断已被禁止，因此可以写入栈指针。
 */

#define portRESTORE_CONTEXT()                            \
    asm volatile ( "lds    r26, pxCurrentTCB       \n\t" \
                   "lds    r27, pxCurrentTCB + 1   \n\t" \
                   "ld     r28, x+                 \n\t" \
                   "out    __SP_L__, r28           \n\t" \
                   "ld     r29, x+                 \n\t" \
                   "out    __SP_H__, r29           \n\t" \
                   "pop    r31                     \n\t" \
                   "pop    r30                     \n\t" \
                   "pop    r29                     \n\t" \
                   "pop    r28                     \n\t" \
                   "pop    r27                     \n\t" \
                   "pop    r26                     \n\t" \
                   "pop    r25                     \n\t" \
                   "pop    r24                     \n\t" \
                   "pop    r23                     \n\t" \
                   "pop    r22                     \n\t" \
                   "pop    r21                     \n\t" \
                   "pop    r20                     \n\t" \
                   "pop    r19                     \n\t" \
                   "pop    r18                     \n\t" \
                   "pop    r17                     \n\t" \
                   "pop    r16                     \n\t" \
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
                   "pop    r3                      \n\t" \
                   "pop    r2                      \n\t" \
                   "pop    r1                      \n\t" \
                   "pop    r0                      \n\t" \
                   "out    __SREG__, r0            \n\t" \
                   "pop    r0                      \n\t" \
                   );

/*-----------------------------------------------------------*/

/*
 * 执行硬件初始化，使定时器 1 的比较匹配 A 产生时钟节拍（tick）。
 */
static void prvSetupTimerInterrupt( void );
/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    uint16_t usAddress;

    /*
     * 在栈底放置几个已知值。
     * 这仅用于调试。
     */

    *pxTopOfStack = 0x11;
    pxTopOfStack--;
    *pxTopOfStack = 0x22;
    pxTopOfStack--;
    *pxTopOfStack = 0x33;
    pxTopOfStack--;

    /*
     * 模拟编译器生成的对 vPortYield() 调用之后的栈布局。
     */

    /* lint -e950 -e611 -e923：Lint 对此颇有微词，但无能为力。 */

    /*
     * 任务代码的起始地址将最后从栈中弹出，因此先将其放入。
     */
    usAddress = ( uint16_t ) pxCode;
    *pxTopOfStack = ( StackType_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    usAddress >>= 8;
    *pxTopOfStack = ( StackType_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    /*
     * 接着模拟调用 portSAVE_CONTEXT() 之后的栈。
     *  portSAVE_CONTEXT 将标志位紧接在 r0 之后压入栈，
     * 以便尽快禁止中断，从而在发生上下文切换中断时保证栈用量最小。
     */
    *pxTopOfStack = ( StackType_t ) 0x00; /* R0 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = portFLAGS_INT_ENABLED;
    pxTopOfStack--;


    /* 然后是其余寄存器。编译器期望 R1 为 0。 */
    *pxTopOfStack = ( StackType_t ) 0x00; /* R1 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x02; /* R2 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x03; /* R3 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x04; /* R4 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x05; /* R5 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x06; /* R6 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x07; /* R7 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x08; /* R8 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x09; /* R9 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x10; /* R10 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x11; /* R11 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x12; /* R12 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x13; /* R13 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x14; /* R14 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x15; /* R15 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x16; /* R16 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x17; /* R17 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x18; /* R18 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x19; /* R19 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x20; /* R20 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x21; /* R21 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x22; /* R22 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x23; /* R23 寄存器 */
    pxTopOfStack--;

    /* 将参数放置在栈上的预期位置。 */
    usAddress = ( uint16_t ) pvParameters;
    *pxTopOfStack = ( StackType_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    usAddress >>= 8;
    *pxTopOfStack = ( StackType_t ) ( usAddress & ( uint16_t ) 0x00ff );
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0x26;  /* R26（X）寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x27;  /* R27 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x28;  /* R28（Y）寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x29;  /* R29 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x30;  /* R30（Z）寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x031; /* R31 寄存器 */
    pxTopOfStack--;

    /* lint +e950 +e611 +e923 检查指令 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /* 配置硬件以产生时钟节拍（tick）。 */
    prvSetupTimerInterrupt();

    /* 恢复将要运行的第一个任务的上下文。 */
    portRESTORE_CONTEXT();

    /*
     * 模拟编译器生成的函数调用结束过程。现在跳转到刚刚恢复上下文的
     * 任务起始处。
     */
    asm volatile ( "ret" );

    /* 不应执行到此处。 */
    return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * AVR 移植层不太可能被停止。如有需要，在此处禁止时钟节拍中断即可。
     */
}
/*-----------------------------------------------------------*/

/*
 * 手动上下文切换。首先保存寄存器，以便使用 naked 属性。
 */
void vPortYield( void ) __attribute__( ( naked ) );
void vPortYield( void )
{
    portSAVE_CONTEXT();
    vTaskSwitchContext();
    portRESTORE_CONTEXT();

    asm volatile ( "ret" );
}
/*-----------------------------------------------------------*/

/*
 * 时钟节拍使用的上下文切换函数。从调用 vTaskSwitchContext() 起，
 * 其行为必须与 vPortYield() 完全一致。与 vPortYield() 的唯一区别是，
 * 由于调用来自时钟节拍 ISR，需要递增 tick 计数。
 */
void vPortYieldFromTick( void ) __attribute__( ( naked ) );
void vPortYieldFromTick( void )
{
    portSAVE_CONTEXT();

    if( xTaskIncrementTick() != pdFALSE )
    {
        vTaskSwitchContext();
    }

    portRESTORE_CONTEXT();

    asm volatile ( "ret" );
}
/*-----------------------------------------------------------*/

/*
 * 配置定时器 1 的比较匹配 A 以产生时钟节拍中断。
 */
static void prvSetupTimerInterrupt( void )
{
    uint32_t ulCompareMatch;
    uint8_t ucHighByte, ucLowByte;

    /*
     * 使用 16 位定时器 1 产生时钟节拍（tick）。必须为 configCPU_CLOCK_HZ
     * 时钟选择正确的熔丝位。
     */

    ulCompareMatch = configCPU_CLOCK_HZ / configTICK_RATE_HZ;

    /* 只有 16 位，因此必须分频才能得到所需的 tick 速率。 */
    ulCompareMatch /= portCLOCK_PRESCALER;

    /* 调整为正确的数值。 */
    ulCompareMatch -= ( uint32_t ) 1;

    /*
     * 为比较匹配 A 设置比较匹配值。调用此函数前中断已被禁止，因此此处无需担心。
     */
    ucLowByte = ( uint8_t ) ( ulCompareMatch & ( uint32_t ) 0xff );
    ulCompareMatch >>= 8;
    ucHighByte = ( uint8_t ) ( ulCompareMatch & ( uint32_t ) 0xff );
    OCR1AH = ucHighByte;
    OCR1AL = ucLowByte;

    /* 设置时钟源和比较匹配行为。 */
    ucLowByte = portCLEAR_COUNTER_ON_MATCH | portPRESCALE_64;
    TCCR1B = ucLowByte;

    /*
     * 使能中断 - 这是安全的，因为当前中断在全局范围内处于禁止状态。
     */
    ucLowByte = TIMSK;
    ucLowByte |= portCOMPARE_MATCH_A_INTERRUPT_ENABLE;
    TIMSK = ucLowByte;
}
/*-----------------------------------------------------------*/

#if configUSE_PREEMPTION == 1

/*
 * 抢占式调度器的时钟节拍 ISR。由于上下文在 vPortYieldFromTick() 开头
 * 已保存，可以使用 naked 属性。上下文保存之后再递增 tick 计数。
 */
    void TIMER1_COMPA_vect( void ) __attribute__( ( signal, naked ) );
    void TIMER1_COMPA_vect( void )
    {
        vPortYieldFromTick();
        asm volatile ( "reti" );
    }
#else

/*
 * 协作式调度器的时钟节拍 ISR。仅递增 tick 计数。无需切换上下文，
 * 上下文切换只能通过手动调用 taskYIELD() 完成。
 */
    void TIMER1_COMPA_vect( void ) __attribute__( ( signal ) );
    void TIMER1_COMPA_vect( void )
    {
        xTaskIncrementTick();
    }
#endif /* 若 configUSE_PREEMPTION == 1 */
