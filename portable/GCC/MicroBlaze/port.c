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
 * portable.h 中各函数针对 MicroBlaze 移植层的实现。
 * ----------------------------------------------------------
 */


/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* 标准头文件。 */
#include <string.h>

/* 硬件头文件。 */
#include <xintc.h>
#include <xintc_i.h>
#include <xtmrctr.h>

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error configSUPPORT_DYNAMIC_ALLOCATION must be set to 1 to use this port.
#endif

/* 任务启动时中断处于使能状态。 */
#define portINITIAL_MSR_STATE        ( ( StackType_t ) 0x02 )

/*
 * 任务启动时临界区嵌套计数为 0——但在调度器启动之前，
 * 不希望临界嵌套层次降到零，因此将其初始化为一个较大的值。
 */
#define portINITIAL_NESTING_VALUE    ( 0xff )

/* 我们的硬件配置仅使用一个计数器。 */
#define portCOUNTER_0                0

/*
 * ISR 使用的栈填充了已知值，以辅助调试。
 */
#define portISR_STACK_FILL_VALUE     0x55555555

/*
 * 记录 portENTER_CRITICAL() 调用嵌套深度的计数器。
 * 每个任务维护自己的计数，因此该变量作为任务上下文的一部分保存。
 */
volatile UBaseType_t uxCriticalNesting = portINITIAL_NESTING_VALUE;

/*
 * 为限制每个任务所需的栈量，本移植层为中断使用独立的栈。
 */
uint32_t * pulISRStack;

/*-----------------------------------------------------------*/

/*
 * 设置用于 RTOS tick 的周期性 ISR。此处使用定时器 0，
 * 也可以改用看门狗定时器或定时器 1。
 */
static void prvSetupTimerInterrupt( void );
/*-----------------------------------------------------------*/

/*
 * 初始化任务栈，使其外观与刚调用过 portSAVE_CONTEXT 完全一致。
 *
 * 参见 portable.h 头文件。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    extern void * _SDA2_BASE_;
    extern void * _SDA_BASE_;
    const uint32_t ulR2 = ( uint32_t ) &_SDA2_BASE_;
    const uint32_t ulR13 = ( uint32_t ) &_SDA_BASE_;

    /*
     * 在栈底放置几个已知值。
     * 这对 MicroBlaze 移植层至关重要，这些代码行不得省略。
     * 函数序言期间，参数值将覆盖 0x22222222。
     */
    *pxTopOfStack = ( StackType_t ) 0x11111111;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x22222222;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x33333333;
    pxTopOfStack--;

    /*
     * 首先压入临界区嵌套的初始值。由于任务启动时中断已使能，
     * 该值初始化为零。
     */
    *pxTopOfStack = ( StackType_t ) 0x00; /* R0 寄存器。 */

    /* 为所有通用寄存器放置初始值。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) ulR2;         /* R2 - 小数据区。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x03;         /* R3 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x04;         /* R4 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R5 包含函数调用参数。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x06;         /* R6 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x07;         /* R7 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x08;         /* R8 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x09;         /* R9 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x0a;         /* R10 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x0b;         /* R11 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x0c;         /* R12 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) ulR13;        /* R13 - 小数据读写区。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode;       /* R14 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x0f;         /* R15 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x10;         /* R16 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x11;         /* R17 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x12;         /* R18 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x13;         /* R19 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x14;         /* R20 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x15;         /* R21 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x16;         /* R22 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x17;         /* R23 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x18;         /* R24 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x19;         /* R25 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x1a;         /* R26 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x1b;         /* R27 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x1c;         /* R28 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x1d;         /* R29 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x1e;         /* R30 寄存器。 */
    pxTopOfStack--;

    /* MSR 压在 R30 和 R31 之间。 */
    *pxTopOfStack = portINITIAL_MSR_STATE;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) 0x1f; /* R31 寄存器。 */
    pxTopOfStack--;

    /*
     * 返回指向所生成栈顶的指针，以便将其保存到该任务的任务控制块中。
     */
    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    extern void( __FreeRTOS_interrupt_Handler )( void );
    extern void( vStartFirstTask )( void );


    /* 设置 FreeRTOS 中断处理程序。代码复制自 crt0.s。 */
    asm volatile ( "la r6, r0, __FreeRTOS_interrupt_handler        \n\t" \
                   "sw r6, r1, r0                                  \n\t" \
                   "lhu r7, r1, r0                                 \n\t" \
                   "shi r7, r0, 0x12                               \n\t" \
                   "shi r6, r0, 0x16 " );

    /*
     * 设置硬件以产生 tick。调用本函数时中断处于禁用状态。
     */
    prvSetupTimerInterrupt();

    /* 分配供中断处理程序使用的栈。 */
    pulISRStack = ( uint32_t * ) pvPortMalloc( configMINIMAL_STACK_SIZE * sizeof( StackType_t ) );

    /* 恢复即将运行的第一个任务的上下文。 */
    if( pulISRStack != NULL )
    {
        /* 用已知值填充 ISR 栈，便于调试。 */
        memset( pulISRStack, portISR_STACK_FILL_VALUE, configMINIMAL_STACK_SIZE * sizeof( StackType_t ) );
        pulISRStack += ( configMINIMAL_STACK_SIZE - 1 );

        /* 启动第一个任务。 */
        vStartFirstTask();
    }

    /* 不应到达此处，因为任务现在已在运行！ */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* 未实现。 */
}
/*-----------------------------------------------------------*/

/*
 * 由 portYIELD 或 taskYIELD 调用的手动上下文切换。
 */
void vPortYield( void )
{
    extern void VPortYieldASM( void );

    /*
     * 在临界区中执行上下文切换，以确保不被 tick ISR 打断。
     * 这样做没有问题，因为每个任务都维护自己的中断状态。
     */
    portENTER_CRITICAL();

    /*
     * 直接跳转到让出函数，以确保不产生编译器生成的序言代码。
     */
    asm volatile ( "bralid r14, VPortYieldASM      \n\t" \
                   "or r0, r0, r0                  \n\t" );
    portEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

/*
 * 用于产生 RTOS tick 的硬件初始化。
 */
static void prvSetupTimerInterrupt( void )
{
    XTmrCtr xTimer;
    const uint32_t ulCounterValue = configCPU_CLOCK_HZ / configTICK_RATE_HZ;
    UBaseType_t uxMask;

    /*
     * 使用 OPB 定时器 1 产生 tick。使用提供的库函数使能定时器并设置 tick 频率。
     */
    XTmrCtr_mDisable( XPAR_OPB_TIMER_1_BASEADDR, XPAR_OPB_TIMER_1_DEVICE_ID );
    XTmrCtr_Initialize( &xTimer, XPAR_OPB_TIMER_1_DEVICE_ID );
    XTmrCtr_mSetLoadReg( XPAR_OPB_TIMER_1_BASEADDR, portCOUNTER_0, ulCounterValue );
    XTmrCtr_mSetControlStatusReg( XPAR_OPB_TIMER_1_BASEADDR, portCOUNTER_0, XTC_CSR_LOAD_MASK | XTC_CSR_INT_occurred_MASK );

    /*
     * 在保持其他位状态不变的情况下，设置定时器中断使能位。
     */
    uxMask = XIntc_In32( ( XPAR_OPB_INTC_0_BASEADDR + XIN_IER_OFFSET ) );
    uxMask |= XPAR_OPB_TIMER_1_INTERRUPT_MASK;
    XIntc_Out32( ( XPAR_OPB_INTC_0_BASEADDR + XIN_IER_OFFSET ), ( uxMask ) );

    XTmrCtr_Start( &xTimer, XPAR_OPB_TIMER_1_DEVICE_ID );
    XTmrCtr_mSetControlStatusReg( XPAR_OPB_TIMER_1_BASEADDR, portCOUNTER_0, XTC_CSR_ENABLE_TMR_MASK | XTC_CSR_ENABLE_INT_MASK | XTC_CSR_AUTO_RELOAD_MASK | XTC_CSR_DOWN_COUNT_MASK | XTC_CSR_INT_occurred_MASK );
    XIntc_mAckIntr( XPAR_INTC_SINGLE_BASEADDR, 1 );
}
/*-----------------------------------------------------------*/

/*
 * 调度器启动时放入中断向量的中断处理程序。调用它时任务上下文已保存。
 * 该处理程序确定中断源并调用相应的外设处理程序。
 */
void vTaskISRHandler( void )
{
    static uint32_t ulPending;

    /* 哪些中断处于挂起状态？ */
    ulPending = XIntc_In32( ( XPAR_INTC_SINGLE_BASEADDR + XIN_IVR_OFFSET ) );

    if( ulPending < XPAR_INTC_MAX_NUM_INTR_INPUTS )
    {
        static XIntc_VectorTableEntry * pxTablePtr;
        static XIntc_Config * pxConfig;
        static uint32_t ulInterruptMask;

        ulInterruptMask = ( uint32_t ) 1 << ulPending;

        /* 使用设备 ID 获取配置数据。 */
        pxConfig = &XIntc_ConfigTable[ ( uint32_t ) XPAR_INTC_SINGLE_DEVICE_ID ];

        pxTablePtr = &( pxConfig->HandlerTable[ ulPending ] );

        if( pxConfig->AckBeforeService & ( ulInterruptMask ) )
        {
            XIntc_mAckIntr( pxConfig->BaseAddress, ulInterruptMask );
            pxTablePtr->Handler( pxTablePtr->CallBackRef );
        }
        else
        {
            pxTablePtr->Handler( pxTablePtr->CallBackRef );
            XIntc_mAckIntr( pxConfig->BaseAddress, ulInterruptMask );
        }
    }
}
/*-----------------------------------------------------------*/

/*
 * 定时器中断处理程序。
 */
void vTickISR( void * pvBaseAddress )
{
    uint32_t ulCSR;

    /* 递增 RTOS tick 计数——这可能使某个任务解除阻塞。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        vTaskSwitchContext();
    }

    /* 清除定时器中断。 */
    ulCSR = XTmrCtr_mGetControlStatusReg( XPAR_OPB_TIMER_1_BASEADDR, 0 );
    XTmrCtr_mSetControlStatusReg( XPAR_OPB_TIMER_1_BASEADDR, portCOUNTER_0, ulCSR );
}
/*-----------------------------------------------------------*/
