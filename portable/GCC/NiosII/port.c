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
 * portable.h 中各函数针对 NIOS2 移植层的实现。
 * ----------------------------------------------------------
 */

/* 标准头文件。 */
#include <string.h>
#include <errno.h>

/* Altera 头文件。 */
#include "sys/alt_irq.h"
#include "sys/alt_exceptions.h"
#include "altera_avalon_timer_regs.h"
#include "priv/alt_irq_table.h"

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* 中断已使能。 */
#define portINITIAL_ESTATUS    ( StackType_t ) 0x01

int _alt_ic_isr_register( alt_u32 ic_id,
                          alt_u32 irq,
                          alt_isr_func isr,
                          void * isr_context,
                          void * flags );
/*-----------------------------------------------------------*/

/*
 * 设置定时器以产生 tick 中断。
 */
static void prvSetupTimerInterrupt( void );

/*
 * 闹钟函数的回调。
 */
void vPortSysTickHandler( void * context );

/*-----------------------------------------------------------*/

static void prvReadGp( uint32_t * ulValue )
{
    asm ( "stw gp, (%0)" ::"r" ( ulValue ) );
}
/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    StackType_t * pxFramePointer = pxTopOfStack - 1;
    StackType_t xGlobalPointer;

    prvReadGp( &xGlobalPointer );

    /* 栈结束标记。 */
    *pxTopOfStack = 0xdeadbeef;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) pxFramePointer;
    pxTopOfStack--;

    *pxTopOfStack = xGlobalPointer;

    /* R23 到 R16 的空间。 */
    pxTopOfStack -= 9;

    *pxTopOfStack = ( StackType_t ) pxCode;
    pxTopOfStack--;

    *pxTopOfStack = portINITIAL_ESTATUS;

    /* R15 到 R5 的空间。 */
    pxTopOfStack -= 12;

    *pxTopOfStack = ( StackType_t ) pvParameters;

    /* R3 到 R1、乘除法结果和 RA 的空间。 */
    pxTopOfStack -= 5;

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
BaseType_t xPortStartScheduler( void )
{
    /*
     * 启动产生 tick ISR 的定时器。此处中断已禁用。
     */
    prvSetupTimerInterrupt();

    /* 启动第一个任务。 */
    asm volatile ( " movia r2, restore_sp_from_pxCurrentTCB        \n"
                   " jmp r2                                          " );

    /* 不应到达此处！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * NIOS2 移植层不太可能需要此函数，因为没有可返回之处。
     */
}
/*-----------------------------------------------------------*/

/*
 * 将 SysTick 定时器设置为按所需频率产生 tick 中断。
 */
void prvSetupTimerInterrupt( void )
{
    /* 尝试注册中断处理程序。 */
    if( -EINVAL == _alt_ic_isr_register( SYS_CLK_IRQ_INTERRUPT_CONTROLLER_ID, SYS_CLK_IRQ, vPortSysTickHandler, 0x0, 0x0 ) )
    {
        /* 安装中断处理程序失败。 */
        asm ( "break" );
    }
    else
    {
        /* 将 SysTick 配置为按请求的频率产生中断。 */
        IOWR_ALTERA_AVALON_TIMER_CONTROL( SYS_CLK_BASE, ALTERA_AVALON_TIMER_CONTROL_STOP_MSK );
        IOWR_ALTERA_AVALON_TIMER_PERIODL( SYS_CLK_BASE, ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) & 0xFFFF );
        IOWR_ALTERA_AVALON_TIMER_PERIODH( SYS_CLK_BASE, ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) >> 16 );
        IOWR_ALTERA_AVALON_TIMER_CONTROL( SYS_CLK_BASE, ALTERA_AVALON_TIMER_CONTROL_CONT_MSK | ALTERA_AVALON_TIMER_CONTROL_START_MSK | ALTERA_AVALON_TIMER_CONTROL_ITO_MSK );
    }

    /* 清除定时器已经产生的所有挂起中断。 */
    IOWR_ALTERA_AVALON_TIMER_STATUS( SYS_CLK_BASE, ~ALTERA_AVALON_TIMER_STATUS_TO_MSK );
}
/*-----------------------------------------------------------*/

void vPortSysTickHandler( void * context )
{
    /* 递增内核 tick。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        vTaskSwitchContext();
    }

    /* 清除中断。 */
    IOWR_ALTERA_AVALON_TIMER_STATUS( SYS_CLK_BASE, ~ALTERA_AVALON_TIMER_STATUS_TO_MSK );
}
/*-----------------------------------------------------------*/

/**
 * 本函数是 Altera 所提供函数的重新实现。
 * 重新实现旨在防止在注册中断时立即使能该中断。
 * 只有在 FreeRTOS.org 内核启动调度器之后才应使能中断，
 * 以保证上下文能够正确保存和切换。
 */
int _alt_ic_isr_register( alt_u32 ic_id,
                          alt_u32 irq,
                          alt_isr_func isr,
                          void * isr_context,
                          void * flags )
{
    int rc = -EINVAL;
    alt_irq_context status;
    int id = irq; /* 将 IRQ 解释为中断 ID。 */

    if( id < ALT_NIRQ )
    {
        /*
         * 更新处理程序表期间禁用中断，以确保表处于不一致状态时不会发生中断。
         */

        status = alt_irq_disable_all();

        alt_irq[ id ].handler = isr;
        alt_irq[ id ].context = isr_context;

        rc = ( isr ) ? alt_ic_irq_enable( ic_id, id ) : alt_ic_irq_disable( ic_id, id );

        /* alt_irq_enable_all(status); 此行已被删除，以防止中断在注册后立即被使能。 */
    }

    return rc;
}
/*-----------------------------------------------------------*/
