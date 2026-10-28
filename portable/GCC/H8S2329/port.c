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
 * -----------------------------------------------------------
 * H8S 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */


/*-----------------------------------------------------------*/

/* 任务启动时应使能中断。 */
#define portINITIAL_CCR                    ( ( StackType_t ) 0x00 )

/* 用于从 TPU 产生 RTOS 时钟节拍（tick）的硬件专用常量。 */
#define portCLEAR_ON_TGRA_COMPARE_MATCH    ( ( uint8_t ) 0x20 )
#define portCLOCK_DIV_64                   ( ( uint8_t ) 0x03 )
#define portCLOCK_DIV                      ( ( uint32_t ) 64 )
#define portTGRA_INTERRUPT_ENABLE          ( ( uint8_t ) 0x01 )
#define portTIMER_CHANNEL                  ( ( uint8_t ) 0x02 )
#define portMSTP13                         ( ( uint16_t ) 0x2000 )

/*
 * 按所请求的频率配置 TPU 通道 1 以产生 RTOS 时钟节拍（tick）。
 */
static void prvSetupTimerInterrupt( void );

/*
 * portYIELD() 使用的 ISR。它被安装为陷阱处理程序。
 */
void vPortYield( void ) __attribute__( ( saveall, interrupt_handler ) );

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    uint32_t ulValue;

    /* 这要求使用偶地址。 */
    ulValue = ( uint32_t ) pxTopOfStack;

    if( ulValue & 1UL )
    {
        pxTopOfStack = pxTopOfStack - 1;
    }

    /*
     * 在栈底放置几个已知值。
     * 这仅用于调试。
     */
    pxTopOfStack--;
    *pxTopOfStack = 0xaa;
    pxTopOfStack--;
    *pxTopOfStack = 0xbb;
    pxTopOfStack--;
    *pxTopOfStack = 0xcc;
    pxTopOfStack--;
    *pxTopOfStack = 0xdd;

    /*
     * 初始栈模拟中断栈。首先是程序计数器（24 位）。
     */
    ulValue = ( uint32_t ) pxCode;

    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );
    pxTopOfStack--;
    ulValue >>= 8UL;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );
    pxTopOfStack--;
    ulValue >>= 8UL;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );

    /* 其后是 CCR。 */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_CCR;

    /*
     * 接着是所有通用寄存器 - 参数通过 ER0 传递。参数顺序必须与编译器
     * 使用 "saveall" 函数属性时采用的顺序一致。
     */

    /* ER6 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x66;
    pxTopOfStack--;
    *pxTopOfStack = 0x66;
    pxTopOfStack--;
    *pxTopOfStack = 0x66;
    pxTopOfStack--;
    *pxTopOfStack = 0x66;

    /* ER0 寄存器 */
    ulValue = ( uint32_t ) pvParameters;

    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );
    pxTopOfStack--;
    ulValue >>= 8UL;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );
    pxTopOfStack--;
    ulValue >>= 8UL;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );
    pxTopOfStack--;
    ulValue >>= 8UL;
    *pxTopOfStack = ( StackType_t ) ( ulValue & 0xff );

    /* ER1 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x11;
    pxTopOfStack--;
    *pxTopOfStack = 0x11;
    pxTopOfStack--;
    *pxTopOfStack = 0x11;
    pxTopOfStack--;
    *pxTopOfStack = 0x11;

    /* ER2 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x22;
    pxTopOfStack--;
    *pxTopOfStack = 0x22;
    pxTopOfStack--;
    *pxTopOfStack = 0x22;
    pxTopOfStack--;
    *pxTopOfStack = 0x22;

    /* ER3 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x33;
    pxTopOfStack--;
    *pxTopOfStack = 0x33;
    pxTopOfStack--;
    *pxTopOfStack = 0x33;
    pxTopOfStack--;
    *pxTopOfStack = 0x33;

    /* ER4 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x44;
    pxTopOfStack--;
    *pxTopOfStack = 0x44;
    pxTopOfStack--;
    *pxTopOfStack = 0x44;
    pxTopOfStack--;
    *pxTopOfStack = 0x44;

    /* ER5 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x55;
    pxTopOfStack--;
    *pxTopOfStack = 0x55;
    pxTopOfStack--;
    *pxTopOfStack = 0x55;
    pxTopOfStack--;
    *pxTopOfStack = 0x55;

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    extern void * pxCurrentTCB;

    /* 配置硬件以产生时钟节拍（tick）。 */
    prvSetupTimerInterrupt();

    /*
     * 恢复将要运行的第一个任务的上下文。这与编译器使用 "saveall" 函数属性时
     * 生成的函数收尾代码相对应。
     */
    asm volatile (
        "MOV.L      @_pxCurrentTCB, ER6         \n\t"
        "MOV.L      @ER6, ER7                   \n\t"
        "LDM.L      @SP+, (ER4-ER5)             \n\t"
        "LDM.L      @SP+, (ER0-ER3)             \n\t"
        "MOV.L      @ER7+, ER6                  \n\t"
        "RTE                                    \n\t"
        );

    ( void ) pxCurrentTCB;

    /* 不应执行到此处。 */
    return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* H8 移植层不太可能被停止。 */
}
/*-----------------------------------------------------------*/

/*
 * 手动上下文切换。这是一个陷阱处理程序。由于使用 "saveall" 函数属性，
 * 上下文由编译器序言保存。只需再保存栈指针。
 */
void vPortYield( void )
{
    portSAVE_STACK_POINTER();
    vTaskSwitchContext();
    portRESTORE_STACK_POINTER();
}
/*-----------------------------------------------------------*/

/*
 * 为 RTOS 时钟节拍安装的中断处理程序取决于使用的是抢占式还是协作式调度器。
 */
#if ( configUSE_PREEMPTION == 1 )

/*
 * 使用抢占式调度器，因此 ISR 调用 vTaskSwitchContext()。
 * 函数序言会保存上下文，因此只需再保存栈指针。
 */
    void vTickISR( void ) __attribute__( ( saveall, interrupt_handler ) );
    void vTickISR( void )
    {
        portSAVE_STACK_POINTER();

        if( xTaskIncrementTick() != pdFALSE )
        {
            vTaskSwitchContext();
        }

        /* 清除中断。 */
        TSR1 &= ~0x01;

        portRESTORE_STACK_POINTER();
    }

#else /* 若 ( configUSE_PREEMPTION == 1 ) */

/*
 * 使用协作式调度器，因此只需定期递增 tick。这可以是普通的 ISR，
 * 不需要 "saveall" 属性。
 */
    void vTickISR( void ) __attribute__( ( interrupt_handler ) );
    void vTickISR( void )
    {
        xTaskIncrementTick();

        /* 清除中断。 */
        TSR1 &= ~0x01;
    }

#endif /* 若 ( configUSE_PREEMPTION == 1 ) */
/*-----------------------------------------------------------*/

/*
 * 配置定时器 1 的比较匹配以产生时钟节拍（tick）中断。
 */
static void prvSetupTimerInterrupt( void )
{
    const uint32_t ulCompareMatch = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) / portCLOCK_DIV;

    /* 开启该模块。 */
    MSTPCR &= ~portMSTP13;

    /* 配置定时器 1。 */
    TCR1 = portCLEAR_ON_TGRA_COMPARE_MATCH | portCLOCK_DIV_64;

    /* 配置比较匹配值，使 tick 频率为 configTICK_RATE_HZ。 */
    TGR1A = ulCompareMatch;

    /*
     * 启动定时器并使能中断 - 可以在此处执行，因为调用此函数时中断已全局禁止。
     */
    TIER1 |= portTGRA_INTERRUPT_ENABLE;
    TSTR |= portTIMER_CHANNEL;
}
/*-----------------------------------------------------------*/
