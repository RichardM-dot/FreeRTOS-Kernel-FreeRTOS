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
 * portable.h 中各函数针对 ARM7 移植层的实现。
 *
 * 可编译为 ARM 或 THUMB 模式的组件包含在本文件中。
 * 只能编译为 ARM 模式的 ISR 例程包含在 portISR.c 中。
 * ----------------------------------------------------------
 */

/* 标准头文件。 */
#include <stdlib.h>

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* 处理器常量。 */
#include "AT91SAM7X256.h"

/* 设置任务上下文所需的常量。 */
#define portINITIAL_SPSR                   ( ( StackType_t ) 0x1f ) /* 系统模式、ARM 模式、中断使能。 */
#define portTHUMB_MODE_BIT                 ( ( StackType_t ) 0x20 )
#define portINSTRUCTION_SIZE               ( ( StackType_t ) 4 )
#define portNO_CRITICAL_SECTION_NESTING    ( ( StackType_t ) 0 )

/* 设置 tick ISR 所需的常量。 */
#define portENABLE_TIMER                   ( ( uint8_t ) 0x01 )
#define portPRESCALE_VALUE                 0x00
#define portINTERRUPT_ON_MATCH             ( ( uint32_t ) 0x01 )
#define portRESET_COUNT_ON_MATCH           ( ( uint32_t ) 0x02 )

/* 设置 PIT 所需的常量。 */
#define portPIT_CLOCK_DIVISOR              ( ( uint32_t ) 16 )
#define portPIT_COUNTER_VALUE              ( ( ( configCPU_CLOCK_HZ / portPIT_CLOCK_DIVISOR ) / 1000UL ) * portTICK_PERIOD_MS )

#define portINT_LEVEL_SENSITIVE            0
#define portPIT_ENABLE                     ( ( uint16_t ) 0x1 << 24 )
#define portPIT_INT_ENABLE                 ( ( uint16_t ) 0x1 << 25 )
/*-----------------------------------------------------------*/

/* 设置定时器以产生 tick 中断。 */
static void prvSetupTimerInterrupt( void );

/*
 * 调度器只能从 ARM 模式启动，因此 vPortISRStartFirstSTask() 定义在 portISR.c 中。
 */
extern void vPortISRStartFirstTask( void );

/*-----------------------------------------------------------*/

/*
 * 初始化任务栈，使其外观与刚调用过 portSAVE_CONTEXT 完全一致。
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
     * 确保 tasks.c 中的断言不会失败，尽管此处实际上并不需要该断言。
     */
    pxTopOfStack--;

    /*
     * 设置任务的初始栈。栈的布局与 portRESTORE_CONTEXT() 宏的期望完全一致。
     */

    /*
     * 栈中的第一项是返回地址——此处即任务的起始地址。
     * 加上偏移量后，返回地址看起来就像在 IRQ ISR 中一样。
     */
    *pxTopOfStack = ( StackType_t ) pxCode + portINSTRUCTION_SIZE;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0x00000000;    /* R14 寄存器 */
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
     * 任务启动时会期望在 R0 中找到函数参数。
     */
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器 */
    pxTopOfStack--;

    /*
     * 最后压入栈的是状态寄存器，其设置为系统模式并使能中断。
     */
    *pxTopOfStack = ( StackType_t ) portINITIAL_SPSR;

    #ifdef THUMB_INTERWORK
    {
        /* 希望任务以 Thumb 模式启动。 */
        *pxTopOfStack |= portTHUMB_MODE_BIT;
    }
    #endif

    pxTopOfStack--;

    /*
     * 某些优化级别下栈的使用方式与其他级别不同。这意味着中断标志
     * 无法总是保存到栈上，而是保存到变量中，该变量再作为任务上下文的一部分保存。
     */
    *pxTopOfStack = portNO_CRITICAL_SECTION_NESTING;

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /*
     * 启动产生 tick ISR 的定时器。此处中断已禁用。
     */
    prvSetupTimerInterrupt();

    /* 启动第一个任务。 */
    vPortISRStartFirstTask();

    /* 不应到达此处！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * ARM 移植层不太可能需要此函数，因为没有可返回之处。
     */
}
/*-----------------------------------------------------------*/

/*
 * 将定时器 0 设置为按所需频率产生 tick 中断。
 */
static void prvSetupTimerInterrupt( void )
{
    AT91PS_PITC pxPIT = AT91C_BASE_PITC;

    /*
     * 为 PIT 中断配置 AIC。所选中断例程取决于
     * 使用抢占式还是协作式调度器。
     */
    #if configUSE_PREEMPTION == 0
        extern void( vNonPreemptiveTick ) ( void );
        AT91F_AIC_ConfigureIt( AT91C_ID_SYS, AT91C_AIC_PRIOR_HIGHEST, portINT_LEVEL_SENSITIVE, ( void ( * )( void ) )vNonPreemptiveTick );
    #else
        extern void( vPreemptiveTick )( void );
        AT91F_AIC_ConfigureIt( AT91C_ID_SYS, AT91C_AIC_PRIOR_HIGHEST, portINT_LEVEL_SENSITIVE, ( void ( * )( void ) )vPreemptiveTick );
    #endif

    /* 配置 PIT 周期。 */
    pxPIT->PITC_PIMR = portPIT_ENABLE | portPIT_INT_ENABLE | portPIT_COUNTER_VALUE;

    /*
     * 使能中断。此时全局中断已被禁用，
     * 因此此操作是安全的。
     */
    AT91C_BASE_AIC->AIC_IECR = 0x1 << AT91C_ID_SYS;
}
/*-----------------------------------------------------------*/
