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
 * PPC405 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */


/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 库头文件包含。 */
#include "xtime_l.h"
#include "xintc.h"
#include "xintc_i.h"

/*-----------------------------------------------------------*/

/* 用于设置每个任务初始 MSR 的定义。 */
#define portCRITICAL_INTERRUPT_ENABLE    ( 1UL << 17UL )
#define portEXTERNAL_INTERRUPT_ENABLE    ( 1UL << 15UL )
#define portMACHINE_CHECK_ENABLE         ( 1UL << 12UL )

#if configUSE_FPU == 1
    #define portAPU_PRESENT              ( 1UL << 25UL )
    #define portFCM_FPU_PRESENT          ( 1UL << 13UL )
#else
    #define portAPU_PRESENT              ( 0UL )
    #define portFCM_FPU_PRESENT          ( 0UL )
#endif

#define portINITIAL_MSR                  ( portCRITICAL_INTERRUPT_ENABLE | portEXTERNAL_INTERRUPT_ENABLE | portMACHINE_CHECK_ENABLE | portAPU_PRESENT | portFCM_FPU_PRESENT )


extern const unsigned _SDA_BASE_;
extern const unsigned _SDA2_BASE_;

/*-----------------------------------------------------------*/

/*
 * 配置系统定时器以产生时钟节拍（tick）中断。
 */
static void prvSetupTimerInterrupt( void );

/*
 * 时钟节拍中断的处理程序 - 在 portasm.s 中定义。
 */
extern void vPortTickISR( void );

/*
 * 让出（yield）函数的处理程序 - 在 portasm.s 中定义。
 */
extern void vPortYield( void );

/*
 * 通过启动迄今已创建的最高优先级任务来启动调度器运行的函数。
 */
extern void vPortStartFirstTask( void );

/*-----------------------------------------------------------*/

/* 用于保存中断控制器状态的结构体。 */
static XIntc xInterruptController;

/*-----------------------------------------------------------*/

/*
 * 初始化任务栈，使其与任务被中断后的状态完全一致。
 *
 * 参见头文件 portable.h。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /* 在栈底放置已知值以便调试。 */
    *pxTopOfStack = 0xDEADBEEF;
    pxTopOfStack--;

    /* EABI 栈帧。 */
    pxTopOfStack -= 20; /* 前一回溯链与 LR、R31 至 R4（含）。 */

    /* 参数位于 R13。 */
    *pxTopOfStack = ( StackType_t ) &_SDA_BASE_; /* 第一个小数据区的地址 */
    pxTopOfStack -= 10;

    /* 参数位于 R3。 */
    *pxTopOfStack = ( StackType_t ) pvParameters;
    pxTopOfStack--;

    /* 参数位于 R2。 */
    *pxTopOfStack = ( StackType_t ) &_SDA2_BASE_; /* 第二个小数据区的地址 */
    pxTopOfStack--;

    /* R1 为栈指针，因此省略。 */

    *pxTopOfStack = 0x10000001UL;                      /* R0 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x00000000UL;                      /* USPRG0 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x00000000UL;                      /* CR 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x00000000UL;                      /* XER 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x00000000UL;                      /* CTR 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) vPortEndScheduler; /* LR 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode;            /* SRR0 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_MSR;                   /* SRR1 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) vPortEndScheduler; /* 随后的 LR。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x00000000UL;                      /* 回溯链。 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    prvSetupTimerInterrupt();
    XExc_RegisterHandler( XEXC_ID_SYSTEM_CALL, ( XExceptionHandler ) vPortYield, ( void * ) 0 );
    vPortStartFirstTask();

    /* 任务已在运行，不应执行到此处！ */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* 未实现。 */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/*
 * 产生 RTOS 时钟节拍（tick）的硬件初始化。
 */
static void prvSetupTimerInterrupt( void )
{
    const uint32_t ulInterval = ( ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL );

    XTime_PITClearInterrupt();
    XTime_FITClearInterrupt();
    XTime_WDTClearInterrupt();
    XTime_WDTDisableInterrupt();
    XTime_FITDisableInterrupt();

    XExc_RegisterHandler( XEXC_ID_PIT_INT, ( XExceptionHandler ) vPortTickISR, ( void * ) 0 );

    XTime_PITEnableAutoReload();
    XTime_PITSetInterval( ulInterval );
    XTime_PITEnableInterrupt();
}
/*-----------------------------------------------------------*/

void vPortISRHandler( void * pvNullDoNotUse )
{
    uint32_t ulInterruptStatus, ulInterruptMask = 1UL;
    BaseType_t xInterruptNumber;
    XIntc_Config * pxInterruptController;
    XIntc_VectorTableEntry * pxTable;

    /* 仅为消除编译器警告。 */
    ( void ) pvNullDoNotUse;

    /*
     * 通过器件 ID 获取配置 - 此处的假设是只使用一个中断控制器。
     */
    pxInterruptController = &XIntc_ConfigTable[ XPAR_XPS_INTC_0_DEVICE_ID ];

    /* 哪些中断处于挂起状态？ */
    ulInterruptStatus = XIntc_mGetIntrStatus( pxInterruptController->BaseAddress );

    for( xInterruptNumber = 0; xInterruptNumber < XPAR_INTC_MAX_NUM_INTR_INPUTS; xInterruptNumber++ )
    {
        if( ulInterruptStatus & 0x01UL )
        {
            /* 清除挂起的中断。 */
            XIntc_mAckIntr( pxInterruptController->BaseAddress, ulInterruptMask );

            /* 调用已注册的处理程序。 */
            pxTable = &( pxInterruptController->HandlerTable[ xInterruptNumber ] );
            pxTable->Handler( pxTable->CallBackRef );
        }

        /* 检查下一个中断。 */
        ulInterruptMask <<= 0x01UL;
        ulInterruptStatus >>= 0x01UL;

        /* 是否已处理完所有中断？ */
        if( ulInterruptStatus == 0UL )
        {
            break;
        }
    }
}
/*-----------------------------------------------------------*/

void vPortSetupInterruptController( void )
{
    extern void vPortISRWrapper( void );

    /*
     * 执行初始化异常表和中断控制器所需的所有库调用。此代码假定只使用一个
     * 中断控制器。
     */
    XExc_mDisableExceptions( XEXC_NON_CRITICAL );
    XExc_Init();

    /*
     * 库函数负责保存上下文 - 然后跳转到包装函数，将栈保存到 TCB 中。
     * 包装函数随后调用上面定义的处理程序。
     */
    XExc_RegisterHandler( XEXC_ID_NON_CRITICAL_INT, ( XExceptionHandler ) vPortISRWrapper, NULL );
    XIntc_Initialize( &xInterruptController, XPAR_XPS_INTC_0_DEVICE_ID );
    XIntc_Start( &xInterruptController, XIN_REAL_MODE );
}
/*-----------------------------------------------------------*/

BaseType_t xPortInstallInterruptHandler( uint8_t ucInterruptID,
                                         XInterruptHandler pxHandler,
                                         void * pvCallBackRef )
{
    BaseType_t xReturn = pdFAIL;

    /*
     * 在此定义此函数，以便 xInterruptController 的作用域保持在本文件内。
     */

    if( XST_SUCCESS == XIntc_Connect( &xInterruptController, ucInterruptID, pxHandler, pvCallBackRef ) )
    {
        XIntc_Enable( &xInterruptController, ucInterruptID );
        xReturn = pdPASS;
    }

    return xReturn;
}
