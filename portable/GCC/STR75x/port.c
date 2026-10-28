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
 * ST STR75x ARM7 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

/* 库头文件包含。 */
#include "75x_tb.h"
#include "75x_eic.h"

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 设置初始栈所需的常量。 */
#define portINITIAL_SPSR           ( ( StackType_t ) 0x1f )      /* 系统模式、ARM 模式、中断使能。 */
#define portTHUMB_MODE_BIT         ( ( StackType_t ) 0x20 )
#define portINSTRUCTION_SIZE       ( ( StackType_t ) 4 )

/* 处理临界区所需的常量。 */
#define portNO_CRITICAL_NESTING    ( ( uint32_t ) 0 )

/* 计算 tick 周期时对定时器时钟使用的预分频值。 */
#define portPRESCALE               20


/*-----------------------------------------------------------*/

/* 配置 TB 以产生时钟节拍（tick）中断。 */
static void prvSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/

/*
 * 初始化任务栈，使其与调用过 portSAVE_CONTEXT 后的状态完全一致。
 *
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    StackType_t * pxOriginalTOS;

    pxOriginalTOS = pxTopOfStack;

    /*
     * 为确保 tasks.c 中的断言不会失败，尽管此处实际上并不需要该断言。
     */
    pxTopOfStack--;

    /*
     * 设置任务的初始栈。栈内容按 portRESTORE_CONTEXT() 宏的预期精确设置。
     */

    /*
     * 栈上首先是返回地址 - 此处即任务的起始地址。加上偏移量是为了让返回地址
     * 看起来像在 IRQ ISR 中一样。
     */
    *pxTopOfStack = ( StackType_t ) pxCode + portINSTRUCTION_SIZE;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0xaaaaaaaa;    /* R14 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxOriginalTOS; /* 任务启动时使用的栈放入 R13。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x12121212;    /* R12 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x11111111;    /* R11 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x10101010;    /* R10 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x09090909;    /* R9 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x08080808;    /* R8 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x07070707;    /* R7 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x06060606;    /* R6 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x05050505;    /* R5 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x04040404;    /* R4 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x03030303;    /* R3 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x02020202;    /* R2 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x01010101;    /* R1 寄存器 */
    pxTopOfStack--;

    /*
     * 任务启动时将期望在 R0 中找到函数参数。
     */
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器 */
    pxTopOfStack--;

    /* 状态寄存器设置为系统模式，并已使能中断。 */
    *pxTopOfStack = ( StackType_t ) portINITIAL_SPSR;

    #ifdef THUMB_INTERWORK
    {
        /* 希望任务以 Thumb 模式启动。 */
        *pxTopOfStack |= portTHUMB_MODE_BIT;
    }
    #endif

    pxTopOfStack--;

    /*
     * 中断标志无法总是保存在栈上，改为保存在变量中，然后作为任务上下文的一部分保存。
     */
    *pxTopOfStack = portNO_CRITICAL_NESTING;

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    extern void vPortISRStartFirstTask( void );

    /*
     * 启动产生时钟节拍 ISR 的定时器。此处中断已被禁止。
     */
    prvSetupTimerInterrupt();

    /* 启动第一个任务。 */
    vPortISRStartFirstTask();

    /* 不应执行到此处！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * ARM 移植层不太可能需要此函数，因为没有可供返回之处。
     */
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
    EIC_IRQInitTypeDef EIC_IRQInitStructure;
    TB_InitTypeDef TB_InitStructure;

    /* 为 TB 配置 EIC。 */
    EIC_IRQInitStructure.EIC_IRQChannelCmd = ENABLE;
    EIC_IRQInitStructure.EIC_IRQChannel = TB_IRQChannel;
    EIC_IRQInitStructure.EIC_IRQChannelPriority = 1;
    EIC_IRQInit( &EIC_IRQInitStructure );

    /* 配置 TB 以产生时钟节拍中断。 */
    TB_InitStructure.TB_Mode = TB_Mode_Timing;
    TB_InitStructure.TB_CounterMode = TB_CounterMode_Down;
    TB_InitStructure.TB_Prescaler = portPRESCALE - 1;
    TB_InitStructure.TB_AutoReload = ( ( configCPU_CLOCK_HZ / portPRESCALE ) / configTICK_RATE_HZ );
    TB_Init( &TB_InitStructure );

    /* 使能 TB 更新中断 */
    TB_ITConfig( TB_IT_Update, ENABLE );

    /* 清除 TB 更新中断挂起位 */
    TB_ClearITPendingBit( TB_IT_Update );

    /* 使能 TB */
    TB_Cmd( ENABLE );
}
/*-----------------------------------------------------------*/
