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
#include <xintc_i.h>
#include <xil_exception.h>
#include <microblaze_exceptions_g.h>

/*
 * 任务启动时临界区嵌套计数为 0——但在调度器启动之前不应使能中断，
 * 因此临界嵌套变量初始化为非零值。
 */
#define portINITIAL_NESTING_VALUE    ( 0xff )

/*
 * MSR 寄存器中分别用于使能/禁用中断和异常的位。
 */
#define portMSR_IE                   ( 0x02U )
#define portMSR_EE                   ( 0x100U )

/*
 * 若 MicroBlaze 构建中包含浮点单元，则 FSR 寄存器作为任务上下文的一部分保存。
 * portINITIAL_FSR 是为新建任务设置初始上下文时赋给 FSR 寄存器的值。
 */
#define portINITIAL_FSR              ( 0U )
/*-----------------------------------------------------------*/

/*
 * 初始化中断控制器实例。
 */
static int32_t prvInitialiseInterruptController( void );

/*
 * 确保中断控制器实例变量在使用前完成初始化，
 * 并且只初始化一次。
 */
static int32_t prvEnsureInterruptControllerIsInitialised( void );

/*-----------------------------------------------------------*/

/*
 * 记录 portENTER_CRITICAL() 调用嵌套深度的计数器。
 * 每个任务维护自己的计数，因此该变量作为任务上下文的一部分保存。
 */
volatile UBaseType_t uxCriticalNesting = portINITIAL_NESTING_VALUE;

/*
 * 本移植层为中断使用独立的栈。这样可以避免每个任务的栈都需要大到
 * 能在任务栈之上容纳完整的中断栈。
 */
uint32_t * pulISRStack;

/*
 * 若中断请求上下文切换，则 ulTaskSwitchRequested 会被置为 1。
 * 主中断处理程序退出前会检查 ulTaskSwitchRequested。若此时该值为 1，
 * 内核将调用 vTaskSwitchContext()，以确保中断结束后立即运行的任务
 * 是能够运行的最高优先级任务。这是一种不常见的机制，
 * 但本移植层采用它，因为一次中断可能引发多个外设的服务请求——
 * 而每服务一个外设就调用一次 vTaskSwitchContext() 效率低下。
 */
volatile uint32_t ulTaskSwitchRequested = 0UL;

/*
 * 本移植层使用的中断控制器实例。Xilinx 库 API 函数需要它。
 */
static XIntc xInterruptControllerInstance;

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
     */
    *pxTopOfStack = ( StackType_t ) 0x00000000;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;
    pxTopOfStack--;

    #if ( XPAR_MICROBLAZE_USE_FPU != 0 )
        /* 放入初始任务上下文的 FSR 值就是 0。 */
        *pxTopOfStack = portINITIAL_FSR;
        pxTopOfStack--;
    #endif

    /*
     * 放入初始任务上下文的 MSR 值应禁用中断。每个任务首次进入运行态时
     * 会自动使能中断。
     */
    *pxTopOfStack = mfmsr() & ~portMSR_IE;

    #if ( MICROBLAZE_EXCEPTIONS_ENABLED == 1 )
    {
        /* 确保任务已使能异常。 */
        *pxTopOfStack |= portMSR_EE;
    }
    #endif

    pxTopOfStack--;

    /*
     * 首先压入临界区嵌套的初始值，该值初始化为零。
     */
    *pxTopOfStack = ( StackType_t ) 0x00;

    /* R0 始终为零。 */
    /* R1 为栈指针（SP）。 */

    /* 为所有通用寄存器放置初始值。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) ulR2;         /* R2 - 只读小数据区。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x03;         /* R3 - 返回值和临时寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x04;         /* R4 - 返回值和临时寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R5 包含函数调用参数。 */

    #ifdef portPRE_LOAD_STACK_FOR_DEBUGGING
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x06; /* R6 - 其他参数和临时寄存器。用作 vPortTaskEntryPoint 的返回地址。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x07; /* R7 - 其他参数和临时寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x08; /* R8 - 其他参数和临时寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x09; /* R9 - 其他参数和临时寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x0a; /* R10 - 其他参数和临时寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x0b; /* R11 - 临时寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x0c; /* R12 - 临时寄存器。 */
        pxTopOfStack--;
    #else /* ifdef portPRE_LOAD_STACK_FOR_DEBUGGING（若定义：为调试预装载栈） */
        pxTopOfStack -= 8;
    #endif /* ifdef portPRE_LOAD_STACK_FOR_DEBUGGING（若定义：为调试预装载栈） */

    *pxTopOfStack = ( StackType_t ) ulR13;  /* R13 - 读/写小数据区。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode; /* R14 - 中断返回地址。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) NULL;   /* R15 - 子程序返回地址。 */

    #ifdef portPRE_LOAD_STACK_FOR_DEBUGGING
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x10; /* R16 - 陷阱（调试器）返回地址。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x11; /* R17 - 异常返回地址（若已配置）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x12; /* R18 - 保留给汇编器和编译器的临时寄存器。 */
        pxTopOfStack--;
    #else
        pxTopOfStack -= 4;
    #endif

    *pxTopOfStack = ( StackType_t ) 0x00; /* R19 - 跨函数调用必须保存（被调用者保存）。似乎被解释为帧指针。 */

    #ifdef portPRE_LOAD_STACK_FOR_DEBUGGING
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x14; /* R20 - 在位置无关代码（PIC）中保留用于存储全局偏移表（GOT）指针。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x15; /* R21 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x16; /* R22 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x17; /* R23 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x18; /* R24 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x19; /* R25 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x1a; /* R26 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x1b; /* R27 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x1c; /* R28 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x1d; /* R29 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x1e; /* R30 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x1f; /* R31 - 跨函数调用必须保存（被调用者保存）。 */
        pxTopOfStack--;
    #else /* ifdef portPRE_LOAD_STACK_FOR_DEBUGGING（若定义：为调试预装载栈） */
        pxTopOfStack -= 13;
    #endif /* ifdef portPRE_LOAD_STACK_FOR_DEBUGGING（若定义：为调试预装载栈） */

    /*
     * 返回指向所生成栈顶的指针，以便将其保存到该任务的任务控制块中。
     */
    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    extern void( vPortStartFirstTask )( void );
    extern uint32_t _stack[];

    /*
     * 设置硬件以产生 tick。调用本函数时中断处于禁用状态。
     *
     * 本移植层使用应用定义的回调函数安装 tick 中断处理程序，
     * 因为内核要运行在多种 MicroBlaze 和 FPGA 配置上——
     * 并非所有配置都定义或提供相同的定时器外设。
     * 本移植层附带的官方演示应用中提供了 vApplicationSetupTimerInterrupt() 的示例定义。
     */
    vApplicationSetupTimerInterrupt();

    /* 复用 main() 的栈作为中断/异常使用的栈。 */
    pulISRStack = ( uint32_t * ) _stack;

    /*
     * 确保有足够空间，供中断服务例程调用的函数
     * 回写到调用者的栈帧。
     */
    pulISRStack -= 2;

    /*
     * 恢复即将运行的第一个任务的上下文。此后，已创建的任务将开始执行。
     */
    vPortStartFirstTask();

    /* 不应到达此处，因为任务现在已在运行！ */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在没有可返回之处的移植层中不实现。
     * 人为强制触发断言。
     */
    configASSERT( uxCriticalNesting == 1000UL );
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
    {
        /*
         * 直接跳转到让出函数，以确保不产生编译器生成的序言代码。
         */
        asm volatile ( "bralid r14, VPortYieldASM      \n\t" \
                       "or r0, r0, r0                  \n\t" );
    }
    portEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

void vPortEnableInterrupt( uint8_t ucInterruptID )
{
    int32_t lReturn;

    /*
     * 提供 API 函数用于在中断控制器中使能指定中断，
     * 因为中断控制器实例变量仅在本文件内可见。
     */
    lReturn = prvEnsureInterruptControllerIsInitialised();

    if( lReturn == pdPASS )
    {
        XIntc_Enable( &xInterruptControllerInstance, ucInterruptID );
    }

    configASSERT( lReturn );
}
/*-----------------------------------------------------------*/

void vPortDisableInterrupt( uint8_t ucInterruptID )
{
    int32_t lReturn;

    /*
     * 提供 API 函数用于在中断控制器中禁用指定中断，
     * 因为中断控制器实例变量仅在本文件内可见。
     */
    lReturn = prvEnsureInterruptControllerIsInitialised();

    if( lReturn == pdPASS )
    {
        XIntc_Disable( &xInterruptControllerInstance, ucInterruptID );
    }

    configASSERT( lReturn );
}
/*-----------------------------------------------------------*/

BaseType_t xPortInstallInterruptHandler( uint8_t ucInterruptID,
                                         XInterruptHandler pxHandler,
                                         void * pvCallBackRef )
{
    int32_t lReturn;

    /*
     * 提供 API 函数用于安装中断处理程序，
     * 因为中断控制器实例变量仅在本文件内可见。
     */

    lReturn = prvEnsureInterruptControllerIsInitialised();

    if( lReturn == pdPASS )
    {
        lReturn = XIntc_Connect( &xInterruptControllerInstance, ucInterruptID, pxHandler, pvCallBackRef );
    }

    if( lReturn == XST_SUCCESS )
    {
        lReturn = pdPASS;
    }

    configASSERT( lReturn == pdPASS );

    return lReturn;
}
/*-----------------------------------------------------------*/

static int32_t prvEnsureInterruptControllerIsInitialised( void )
{
    static int32_t lInterruptControllerInitialised = pdFALSE;
    int32_t lReturn;

    /*
     * 确保中断控制器实例变量在使用前完成初始化，
     * 并且只初始化一次。
     */
    if( lInterruptControllerInitialised != pdTRUE )
    {
        lReturn = prvInitialiseInterruptController();

        if( lReturn == pdPASS )
        {
            lInterruptControllerInitialised = pdTRUE;
        }
    }
    else
    {
        lReturn = pdPASS;
    }

    return lReturn;
}
/*-----------------------------------------------------------*/

/*
 * 定时器中断处理程序。应用定义的 vApplicationSetupTimerInterrupt()
 * 回调函数应安装此处理程序。
 */
void vPortTickISR( void * pvUnused )
{
    extern void vApplicationClearTimerInterrupt( void );

    /* 确保未使用的参数不会产生编译器警告。 */
    ( void ) pvUnused;

    /*
     * 本移植层使用应用定义的回调函数来清除 tick 中断，
     * 因为内核要运行在多种 MicroBlaze 和 FPGA 配置上——
     * 并非所有配置都定义或提供相同的定时器外设。
     * 本移植层附带的官方演示应用中提供了 vApplicationClearTimerInterrupt() 的示例定义。
     */
    vApplicationClearTimerInterrupt();

    /* 递增 RTOS tick 计数——这可能使某个任务解除阻塞。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        /* 在中断退出时强制调用 vTaskSwitchContext()。 */
        ulTaskSwitchRequested = 1;
    }
}
/*-----------------------------------------------------------*/

static int32_t prvInitialiseInterruptController( void )
{
    int32_t lStatus;

    lStatus = XIntc_Initialize( &xInterruptControllerInstance, configINTERRUPT_CONTROLLER_TO_USE );

    if( lStatus == XST_SUCCESS )
    {
        /* 初始化异常表。 */
        Xil_ExceptionInit();

        /* 每次进入处理程序时服务所有挂起中断。 */
        XIntc_SetIntrSvcOption( xInterruptControllerInstance.BaseAddress, XIN_SVC_ALL_ISRS_OPTION );

        /*
         * 若 MicroBlaze 配置为处理异常，且应用定义的常量
         * configINSTALL_EXCEPTION_HANDLERS 设置为 1，则安装异常处理程序。
         */
        #if ( MICROBLAZE_EXCEPTIONS_ENABLED == 1 ) && ( configINSTALL_EXCEPTION_HANDLERS == 1 )
        {
            vPortExceptionsInstallHandlers();
        }
        #endif /* MICROBLAZE_EXCEPTIONS_ENABLED（MicroBlaze 异常功能使能） */

        /*
         * 启动中断控制器。调度器启动时使能中断。
         */
        lStatus = XIntc_Start( &xInterruptControllerInstance, XIN_REAL_MODE );

        if( lStatus == XST_SUCCESS )
        {
            lStatus = pdPASS;
        }
        else
        {
            lStatus = pdFAIL;
        }
    }

    configASSERT( lStatus == pdPASS );

    return lStatus;
}
/*-----------------------------------------------------------*/
