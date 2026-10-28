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

/* 内核头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

#define portINITIAL_FORMAT_VECTOR      ( ( StackType_t ) 0x4000 )

/* 已设置为超级用户模式。 */
#define portINITIAL_STATUS_REGISTER    ( ( StackType_t ) 0x2000 )

/*
 * 用于记录 taskENTER_CRITICAL() 的嵌套调用次数。
 * 在第一个任务启动之前将其置为 0。
 */
static uint32_t ulCriticalNesting = 0x9999UL;

/*-----------------------------------------------------------*/

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    *pxTopOfStack = ( StackType_t ) pvParameters;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0xDEADBEEF;
    pxTopOfStack--;

    /* 异常栈帧以返回地址开始。 */
    *pxTopOfStack = ( StackType_t ) pxCode;
    pxTopOfStack--;

    *pxTopOfStack = ( portINITIAL_FORMAT_VECTOR << 16UL ) | ( portINITIAL_STATUS_REGISTER );
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0x0; /* FP 寄存器 */
    pxTopOfStack -= 14;                  /* A5 到 D0 寄存器。 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    extern void vPortStartFirstTask( void );

    ulCriticalNesting = 0UL;

    /* 配置此移植层使用的中断。 */
    vApplicationSetupInterrupts();

    /* 启动第一个任务执行。 */
    vPortStartFirstTask();

    return pdFALSE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* 未实现，因为没有可供返回之处。 */
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    if( ulCriticalNesting == 0UL )
    {
        /*
         * 防止在进入临界区的同时挂起上下文切换。
         */
        do
        {
            portDISABLE_INTERRUPTS();

            if( MCF_INTC0_INTFRCL == 0UL )
            {
                break;
            }

            portENABLE_INTERRUPTS();
        } while( 1 );
    }

    ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    ulCriticalNesting--;

    if( ulCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}
/*-----------------------------------------------------------*/

void vPortYieldHandler( void )
{
    uint32_t ulSavedInterruptMask;

    ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
    /* 注意这会清除所有强制中断 - 这样做是为了提高速度。 */
    MCF_INTC0_INTFRCL = 0;
    vTaskSwitchContext();
    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulSavedInterruptMask );
}
