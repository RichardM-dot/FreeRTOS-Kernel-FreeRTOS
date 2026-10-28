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

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/*
 * 临界区嵌套值初始化为非零值，以确保在调度器启动之前中断不会意外被使能。
 */
#define portINITIAL_CRITICAL_NESTING    ( ( uint16_t ) 10 )

/*
 * 分配给新建任务的初始 PSW 值。
 *   11000110
 *   ||||||||-------------- 填充字节
 *   |||||||--------------- 进位标志清零
 *   |||||----------------- 服务中优先级标志置为低电平
 *   ||||------------------ 寄存器组选择 0 标志清零
 *   |||------------------- 辅助进位标志清零
 *   ||-------------------- 寄存器组选择 1 标志清零
 *   |--------------------- 零标志置位
 *   ---------------------- 全局中断标志置位（使能）
 */
#define portPSW                         ( 0xc6UL )

/*
 * 每个任务维护临界区嵌套深度的计数。每次进入临界区时计数递增，每次退出临界区时
 * 计数递减 - 仅当计数为零时才重新使能中断。
 *
 * 调度器启动时 usCriticalNesting 会被置零，但不能将其初始化为零，否则会在启动
 * 序列期间引发问题。
 */
volatile uint16_t usCriticalNesting = portINITIAL_CRITICAL_NESTING;

/*-----------------------------------------------------------*/

/*
 * 设置用于 RTOS 时钟节拍（tick）的周期性 ISR。
 */
__attribute__( ( weak ) ) void vApplicationSetupTimerInterrupt( void );

/*
 * 通过加载第一个要运行任务的上下文来启动调度器。
 * （在 portasm.S 中定义）。
 */
extern void vPortStartFirstTask( void );

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
    uint32_t * pulLocal;

    /* 栈类型以及指向栈类型的指针均为 2 字节。 */

    /*
     * 参数通过栈传递，并按 32 位值写入，因此为第二个两字节留出空间。
     */
    pxTopOfStack--;

    /* 写入参数值。 */
    pulLocal = ( uint32_t * ) pxTopOfStack;
    *pulLocal = ( StackType_t ) pvParameters;
    pxTopOfStack--;

    /*
     * 返回地址，并为 32 位值的前两个字节留出空间。
     */
    pxTopOfStack--;
    pulLocal = ( uint32_t * ) pxTopOfStack;
    *pulLocal = ( uint32_t ) 0;
    pxTopOfStack--;

    /*
     * 起始地址 / PSW 值也按 32 位值写入，因此为第二个两字节留出空间。
     */
    pxTopOfStack--;

    /* 任务函数起始地址与 PSW 组合。 */
    pulLocal = ( uint32_t * ) pxTopOfStack;
    *pulLocal = ( ( ( uint32_t ) pxCode ) | ( portPSW << 24UL ) );
    pxTopOfStack--;

    /* AX 寄存器的初始值。 */
    *pxTopOfStack = ( StackType_t ) 0x1111;
    pxTopOfStack--;

    /* HL 寄存器的初始值。 */
    *pxTopOfStack = ( StackType_t ) 0x2222;
    pxTopOfStack--;

    /* CS 和 ES 寄存器。 */
    *pxTopOfStack = ( StackType_t ) 0x0F00;
    pxTopOfStack--;

    /*
     * 寄存器组 0 中其余的通用寄存器（DE 和 BC）以及另外两个寄存器组……寄存器组 3
     * 专供中断使用，因此不作为任务上下文的一部分保存。
     */
    pxTopOfStack -= 10;

    /*
     * 最后，在任务首次启动时将临界区嵌套计数置为零。
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
    vApplicationSetupTimerInterrupt();

    /* 恢复将要运行的第一个任务的上下文。 */
    vPortStartFirstTask();

    /* 任务已在运行，执行不应到达此处！ */
    return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* RL78 移植层不太可能被停止。 */
}
/*-----------------------------------------------------------*/

__attribute__( ( weak ) ) void vApplicationSetupTimerInterrupt( void )
{
    const uint16_t usClockHz = 15000UL; /* 内部时钟。 */
    const uint16_t usCompareMatch = ( usClockHz / configTICK_RATE_HZ ) + 1UL;

    /* 使用内部 15K 时钟。 */
    OSMC = ( unsigned char ) 0x16;

    #ifdef RTCEN
    {
        /* 提供间隔定时器时钟。 */
        RTCEN = ( unsigned char ) 1U;

        /* 禁止 INTIT 中断。 */
        ITMK = ( unsigned char ) 1;

        /* 停止 ITMC 工作。 */
        ITMC = ( unsigned char ) 0x0000;

        /* 清除 INIT 中断。 */
        ITIF = ( unsigned char ) 0;

        /* 设置间隔并使能中断工作。 */
        ITMC = usCompareMatch | 0x8000U;

        /* 使能 INTIT 中断。 */
        ITMK = ( unsigned char ) 0;
    }
    #endif /* 若定义了 RTCEN */

    #ifdef TMKAEN
    {
        /* 提供间隔定时器时钟。 */
        TMKAEN = ( unsigned char ) 1U;

        /* 禁止 INTIT 中断。 */
        TMKAMK = ( unsigned char ) 1;

        /* 停止 ITMC 工作。 */
        ITMC = ( unsigned char ) 0x0000;

        /* 清除 INIT 中断。 */
        TMKAIF = ( unsigned char ) 0;

        /* 设置间隔并使能中断工作。 */
        ITMC = usCompareMatch | 0x8000U;

        /* 使能 INTIT 中断。 */
        TMKAMK = ( unsigned char ) 0;
    }
    #endif /* 若定义了 TMKAEN */
}
/*-----------------------------------------------------------*/
