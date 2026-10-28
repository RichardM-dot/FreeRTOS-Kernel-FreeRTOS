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

/* 标准头文件包含。 */
#include <stdlib.h>

/* 内核头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 机器相关头文件包含。 */
#include <machine/counter.h>
#include <machine/ic.h>
/*-----------------------------------------------------------*/

/* 初始 PSR 中已设置先前中断使能（PIEN）标志。 */
#define portINITIAL_PSR    ( 0x00020000 )

/*-----------------------------------------------------------*/

/*
 * 执行产生时钟节拍（tick）中断所需的硬件配置。
 */
static void prvSetupTimerInterrupt( void );
/*-----------------------------------------------------------*/

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 在栈上为上下文腾出空间 - 这会留下几个空闲位置。
     */
    pxTopOfStack -= 20;

    /* 用已知值填充寄存器以辅助调试。 */
    pxTopOfStack[ 16 ] = 0;
    pxTopOfStack[ 15 ] = portINITIAL_PSR;
    pxTopOfStack[ 14 ] = ( uint32_t ) pxCode;
    pxTopOfStack[ 13 ] = 0x00000000UL; /* R15 寄存器。 */
    pxTopOfStack[ 12 ] = 0x00000000UL; /* R14 寄存器。 */
    pxTopOfStack[ 11 ] = 0x0d0d0d0dUL;
    pxTopOfStack[ 10 ] = 0x0c0c0c0cUL;
    pxTopOfStack[ 9 ] = 0x0b0b0b0bUL;
    pxTopOfStack[ 8 ] = 0x0a0a0a0aUL;
    pxTopOfStack[ 7 ] = 0x09090909UL;
    pxTopOfStack[ 6 ] = 0x08080808UL;
    pxTopOfStack[ 5 ] = 0x07070707UL;
    pxTopOfStack[ 4 ] = 0x06060606UL;
    pxTopOfStack[ 3 ] = 0x05050505UL;
    pxTopOfStack[ 2 ] = 0x04040404UL;
    pxTopOfStack[ 1 ] = 0x03030303UL;
    pxTopOfStack[ 0 ] = ( uint32_t ) pvParameters;

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /* 设置定时器中断。 */
    prvSetupTimerInterrupt();

    /* 集成中断控制器：使能所有中断。 */
    ic->ien = 1;

    /* 恢复被调用者保存寄存器。 */
    portRESTORE_CONTEXT();

    /* 不应执行到此处。 */
    return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
    /* 使能定时器中断 */
    counter1->reload = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1;
    counter1->value = counter1->reload;
    counter1->mask = 1;

    /* 设置 IRQ 处理程序的优先级并使能它。 */
    irq[ IRQ_COUNTER1 ].ien = 1;
}
/*-----------------------------------------------------------*/

/* 陷阱 31 处理程序。 */
void interrupt31_handler( void ) __attribute__( ( naked ) );
void interrupt31_handler( void )
{
    portSAVE_CONTEXT();
    __asm volatile ( "call vTaskSwitchContext" );
    portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

static void prvProcessTick( void ) __attribute__( ( noinline ) );
static void prvProcessTick( void )
{
    if( xTaskIncrementTick() != pdFALSE )
    {
        vTaskSwitchContext();
    }

    /* 清除时钟节拍中断。 */
    counter1->expired = 0;
}
/*-----------------------------------------------------------*/

/* 定时器 1 中断处理程序，用于时钟节拍中断。 */
void interrupt7_handler( void ) __attribute__( ( naked ) );
void interrupt7_handler( void )
{
    portSAVE_CONTEXT();
    prvProcessTick();
    portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* 无需执行任何操作。不太可能希望结束。 */
}
/*-----------------------------------------------------------*/
