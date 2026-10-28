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
#include <string.h>

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
    #endif
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

#ifndef configSETUP_TICK_INTERRUPT
    #error configSETUP_TICK_INTERRUPT() must be defined in FreeRTOSConfig.h to call the function that sets up the tick interrupt.
#endif

#ifndef configCLEAR_TICK_INTERRUPT
    #error configCLEAR_TICK_INTERRUPT must be defined in FreeRTOSConfig.h to clear which ever interrupt was used to generate the tick interrupt.
#endif

/*
 * 临界区嵌套计数达到该值时，临界区退出。
 */
#define portNO_CRITICAL_NESTING          ( ( uint32_t ) 0 )

/*
 * 任务创建时没有浮点上下文，但可在创建后为其分配浮点上下文。
 * 任务上下文中的变量用于保存该状态：若任务没有 FPU 上下文，
 * 则保存 portNO_FLOATING_POINT_CONTEXT；否则保存其他值。
 */
#define portNO_FLOATING_POINT_CONTEXT    ( ( StackType_t ) 0 )

/* 初始化任务初始上下文所需的常量。 */
#ifdef __ARMEB__
    #define portINITIAL_SPSR             ( ( StackType_t ) 0x21f ) /* 系统模式、ARM 模式、使能 IRQ 和 FIQ、大端。 */
#else
    #define portINITIAL_SPSR             ( ( StackType_t ) 0x01f ) /* 系统模式、ARM 模式、使能 IRQ 和 FIQ、小端。 */
#endif
#define portTHUMB_MODE_BIT               ( ( StackType_t ) 0x20 )
#define portTHUMB_MODE_ADDRESS           ( 0x01UL )

/* 屏蔽 APSR 中除模式位以外的所有位。 */
#define portAPSR_MODE_BITS_MASK          ( 0x1F )

/*
 * CPU 以用户模式执行时 APSR 中模式位的值。
 */
#define portAPSR_USER_MODE               ( 0x10 )

/*
 * 允许用户覆盖初始 LR 的预装载值（即 prvTaskExitError() 的地址），
 * 以防调试器展开栈时受到干扰。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/* 保存 FPU 寄存器所需的栈空间。 */
#if ( configFPU_D32 == 1 )
    #define portFPU_REGISTER_WORDS     ( ( 32 * 2 ) + 1 ) /* D0-D31 与 FPSCR。 */
#else
    #define portFPU_REGISTER_WORDS     ( ( 16 * 2 ) + 1 ) /* D0-D15 与 FPSCR。 */
#endif /* configFPU_D32 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * 这些函数必须以汇编代码编写，因此实现在 portASM.S 中。
 */
extern void vPortRestoreTaskContext( void );
extern void vPortInitialiseFPSCR( void );
extern uint32_t ulReadAPSR( void );

/*
 * 用于捕获试图从任务实现函数返回的任务。
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * 该变量用于跟踪临界区嵌套计数。它必须作为任务上下文的一部分保存，
 * 且必须初始化为非零值，以确保调度器启动前中断不会被意外使能。
 * 由于它是任务上下文的一部分，在启动第一个任务时会自动置为 0。
 */
volatile uint32_t ulCriticalNesting = 9999UL;

/*
 * 作为任务上下文的一部分保存。若 ulPortTaskHasFPUContext 非零，
 * 则必须为该任务保存和恢复浮点上下文。
 */
volatile uint32_t ulPortTaskHasFPUContext = pdFALSE;

/* 置 1 表示从中断服务程序（ISR）中请求一次上下文切换。 */
volatile uint32_t ulPortYieldRequired = pdFALSE;

/*
 * 统计中断嵌套深度。仅当嵌套深度为 0 时才会执行上下文切换。
 */
volatile uint32_t ulPortInterruptNesting = 0UL;

/* 在汇编文件中用于清除中断。 */
__attribute__( ( used ) ) const uint32_t ulICCEOIR = configEOI_ADDRESS;

/*-----------------------------------------------------------*/

/*
 * 详见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 设置任务的初始栈。栈的布置与 portRESTORE_CONTEXT() 宏的预期完全一致。
     *
     * 栈上的第一个实际值是状态寄存器，其被设置为系统模式并使能中断。
     * 先添加几个 NULL，以确保 GDB 不会尝试解码不存在的返回地址。
     */
    *pxTopOfStack = ( StackType_t ) NULL;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) NULL;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) NULL;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) portINITIAL_SPSR;

    if( ( ( uint32_t ) pxCode & portTHUMB_MODE_ADDRESS ) != 0x00UL )
    {
        /* 任务将以 THUMB 模式启动。 */
        *pxTopOfStack |= portTHUMB_MODE_BIT;
    }

    pxTopOfStack--;

    /* 接下来是返回地址，此处为任务的入口地址。 */
    *pxTopOfStack = ( StackType_t ) pxCode;
    pxTopOfStack--;

    /* 接下来是除栈指针外的所有寄存器。 */
    *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* R14（链接寄存器） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x12121212;              /* R12（IP 寄存器） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x11111111;              /* R11（帧指针） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x10101010;              /* R10（栈限制） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x09090909;              /* R9（静态基址） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x08080808;              /* R8 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x07070707;              /* R7 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x06060606;              /* R6 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x05050505;              /* R5 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x04040404;              /* R4 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x03030303;              /* R3（参数寄存器 3） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x02020202;              /* R2（参数寄存器 2） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x01010101;              /* R1（参数寄存器 1） */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pvParameters;            /* R0（参数寄存器 0） */
    pxTopOfStack--;

    /*
     * 任务将以临界区嵌套计数 0 启动，因为此时中断已使能。
     */
    *pxTopOfStack = portNO_CRITICAL_NESTING;

    #if ( configUSE_TASK_FPU_SUPPORT == 1 )
    {
        /*
         * 任务启动时没有浮点上下文。使用浮点硬件的任务必须在执行
         * 任何浮点指令前调用 vPortTaskUsesFPU()。
         */
        pxTopOfStack--;
        *pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;
    }
    #elif ( configUSE_TASK_FPU_SUPPORT == 2 )
    {
        /*
         * 任务启动时具有浮点上下文。为寄存器预留足够空间，
         * 并确保将其初始化为 0。
         */
        pxTopOfStack -= portFPU_REGISTER_WORDS;
        memset( pxTopOfStack, 0x00, portFPU_REGISTER_WORDS * sizeof( StackType_t ) );

        /*
         * 将保存 ulPortTaskHasFPUContext 的槽位初始化为真，
         * 因为任务启动时具有浮点上下文。
         */
        pxTopOfStack--;
        *pxTopOfStack = pdTRUE;
        ulPortTaskHasFPUContext = pdTRUE;
    }
    #else
    {
        #error "Invalid configUSE_TASK_FPU_SUPPORT value - configUSE_TASK_FPU_SUPPORT must be set to 1, 2, or left undefined."
    }
    #endif /* if ( configUSE_TASK_FPU_SUPPORT == 1 ) 条件结束 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    /*
     * 实现任务的函数不得退出，也不得试图返回调用者，因为不存在可返回的位置。
     * 若任务需要退出，应改为调用 vTaskDelete( NULL )。
     *
     * 若定义了 configASSERT()，则人为触发 assert()，
     * 然后在此处停止，以便应用开发者捕获该错误。
     */
    configASSERT( ulPortInterruptNesting == ~0UL );
    portDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    uint32_t ulAPSR;

    /*
     * 仅当 CPU 不处于用户模式时才继续。调度器启动时
     * CPU 必须处于特权模式。
     */
    ulAPSR = ulReadAPSR();

    ulAPSR &= portAPSR_MODE_BITS_MASK;
    configASSERT( ulAPSR != portAPSR_USER_MODE );

    if( ulAPSR != portAPSR_USER_MODE )
    {
        /* 启动产生时钟节拍 ISR 的定时器。 */
        portDISABLE_INTERRUPTS();
        configSETUP_TICK_INTERRUPT();

        /* 启动第一个任务。 */
        vPortRestoreTaskContext();
    }

    /*
     * 仅当以非特权模式调用 vTaskStartScheduler()，或二进制小数点寄存器
     * 未设置为最低可能值时才会到达此处。引用 prvTaskExitError() 是为了
     * 防止在用户自定义退出地址时编译器报告“已定义但未引用”警告。
     */
    ( void ) prvTaskExitError;
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在没有可返回位置的移植层中未实现。
     * 人为触发 assert。
     */
    configASSERT( ulCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();

    /*
     * 现在中断已禁止，可直接访问 ulCriticalNesting。递增 ulCriticalNesting
     * 以统计 portENTER_CRITICAL() 的调用次数。
     */
    ulCriticalNesting++;

    /*
     * 这不是中断安全的进入临界区版本，因此若从中断上下文调用则触发 assert()。
     * 只有以“FromISR”结尾的 API 函数可在中断中使用。仅当临界区嵌套计数为 1
     * 时才断言，以防止 assert 函数本身使用临界区时出现递归调用。
     */
    if( ulCriticalNesting == 1 )
    {
        configASSERT( ulPortInterruptNesting == 0 );
    }
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    if( ulCriticalNesting > portNO_CRITICAL_NESTING )
    {
        /*
         * 退出临界区时递减嵌套计数。
         */
        ulCriticalNesting--;

        /*
         * 若嵌套计数已归零，则必须重新使能所有中断优先级。
         */
        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /*
             * 临界区嵌套已归零，因此应取消屏蔽所有中断优先级。
             */
            portENABLE_INTERRUPTS();
        }
    }
}
/*-----------------------------------------------------------*/

void FreeRTOS_Tick_Handler( void )
{
    uint32_t ulInterruptStatus;

    ulInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    /* 递增 RTOS 时钟节拍。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        ulPortYieldRequired = pdTRUE;
    }

    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulInterruptStatus );

    configCLEAR_TICK_INTERRUPT();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_FPU_SUPPORT != 2 )

    void vPortTaskUsesFPU( void )
    {
        /*
         * 任务正在声明其需要 FPU 上下文。设置 FPU 标志
         * （该标志作为任务上下文的一部分保存）。
         */
        ulPortTaskHasFPUContext = pdTRUE;

        /* 初始化浮点状态寄存器。 */
        vPortInitialiseFPSCR();
    }

#endif /* configUSE_TASK_FPU_SUPPORT 条件编译结束 */
/*-----------------------------------------------------------*/
