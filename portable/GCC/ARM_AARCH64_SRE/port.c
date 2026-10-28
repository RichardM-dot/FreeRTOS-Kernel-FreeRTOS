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

/* 标准头文件。 */
#include <stdlib.h>

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

#ifndef configUNIQUE_INTERRUPT_PRIORITIES
    #error "configUNIQUE_INTERRUPT_PRIORITIES must be defined.  See www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html"
#endif

#ifndef configSETUP_TICK_INTERRUPT
    #error "configSETUP_TICK_INTERRUPT() must be defined.  See www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html"
#endif /* configSETUP_TICK_INTERRUPT（设置 tick 中断） */

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must be defined.  See www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html"
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY == 0
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must not be set to 0"
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY > configUNIQUE_INTERRUPT_PRIORITIES
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must be less than or equal to configUNIQUE_INTERRUPT_PRIORITIES as the lower the numeric priority value the higher the logical interrupt priority"
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error "configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice."
    #endif
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION（使用移植层优化的任务选择） */

/* 在实现了安全扩展的情况下。 */
#if configMAX_API_CALL_INTERRUPT_PRIORITY <= ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must be greater than ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )"
#endif

/*
 * 某些厂商专用文件会在 portmacro.h 中默认定义 configCLEAR_TICK_INTERRUPT()。
 */
#ifndef configCLEAR_TICK_INTERRUPT
    #define configCLEAR_TICK_INTERRUPT()
#endif

/*
 * 临界区嵌套计数达到该值时，即退出临界区。
 */
#define portNO_CRITICAL_NESTING          ( ( size_t ) 0 )

/*
 * 在所有 GIC 中，可向优先级屏蔽寄存器写入 255，以解除所有
 * (最低优先级除外) 中断优先级的屏蔽。
 */
#define portUNMASK_VALUE                 ( 0xFFUL )

/*
 * 任务创建时没有浮点上下文，但创建后可为其分配浮点上下文。
 * 任务上下文中会保存一个变量：若任务没有 FPU 上下文，该变量保存
 * portNO_FLOATING_POINT_CONTEXT；若任务有 FPU 上下文，则保存其他值。
 */
#define portNO_FLOATING_POINT_CONTEXT    ( ( StackType_t ) 0 )

/* 设置初始任务上下文所需的常量。 */
#define portSP_ELx                       ( ( StackType_t ) 0x01 )
#define portSP_EL0                       ( ( StackType_t ) 0x00 )

#if defined( GUEST )
    #define portEL1                      ( ( StackType_t ) 0x04 )
    #define portINITIAL_PSTATE           ( portEL1 | portSP_EL0 )
#else
    #define portEL3                      ( ( StackType_t ) 0x0c )
    /* 撰写本文时，BSP 仅支持 EL3。 */
    #define portINITIAL_PSTATE           ( portEL3 | portSP_EL0 )
#endif

/* 屏蔽 APSR 中除模式位以外的所有位。 */
#define portAPSR_MODE_BITS_MASK    ( 0x0C )

/* DAIF 位中的 I 位。 */
#define portDAIF_I                 ( 0x80 )

/* 用于解除所有中断优先级屏蔽的宏。 */
/* s3_0_c4_c6_0 即 ICC_PMR_EL1。 */
#define portCLEAR_INTERRUPT_MASK()                     \
    {                                                  \
        __asm volatile ( "MSR DAIFSET, #2        \n"   \
                         "DSB SY                 \n"   \
                         "ISB SY                 \n"   \
                         "MSR s3_0_c4_c6_0, %0   \n"   \
                         "DSB SY                 \n"   \
                         "ISB SY                 \n"   \
                         "MSR DAIFCLR, #2        \n"   \
                         "DSB SY                 \n"   \
                         "ISB SY                 \n"   \
                         ::"r" ( portUNMASK_VALUE ) ); \
    }

/*
 * 保存 FPU 寄存器所需的栈空间。共有 32 个 128 位寄存器加 2 个 64 位状态寄存器。
 */
#define portFPU_REGISTER_WORDS     ( (32 * 2) + 2 )

/*-----------------------------------------------------------*/

/*
 * 启动第一个任务开始执行。本函数必须用汇编代码编写，因此在 portASM.s 中实现。
 */
extern void vPortRestoreTaskContext( void );

/*
 * 若应用提供了 vApplicationIRQHandler() 的实现，则在中断入口处
 * 会直接调用它而不保存 FPU 寄存器；此处提供的 vApplicationFPUSafeIRQHandler()
 * 弱实现仅用于消除链接错误——它实际上永远不会被调用，
 * 因此其实现包含一个必定失败的 configASSERT() 调用。
 *
 * 若应用提供了自己的 vApplicationFPUSafeIRQHandler() 实现，
 * 则 portASM.S 中提供的 vApplicationIRQHandler() 会在调用它之前保存 FPU 寄存器。
 *
 * 因此，若应用编写者希望在中断入口保存 FPU 寄存器，其中断处理程序必须命名为
 * vApplicationFPUSafeIRQHandler()；若应用编写者不希望在中断入口保存 FPU 寄存器，
 * 其中断处理程序必须命名为 vApplicationIRQHandler()。
 */
void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR ) __attribute__((weak) );

/*-----------------------------------------------------------*/

/*
 * 用于记录临界区嵌套层次的变量。该变量必须作为任务上下文的一部分保存，
 * 并且必须初始化为非零值，以确保调度器启动前中断不会被意外解除屏蔽。
 * 由于它作为任务上下文的一部分保存，第一个任务启动时会自动将其置为 0。
 */
volatile uint64_t ullCriticalNesting = 9999ULL;

/*
 * 作为任务上下文的一部分保存。若 ullPortTaskHasFPUContext 非零，
 * 则必须为该任务保存和恢复浮点上下文。
 */
uint64_t ullPortTaskHasFPUContext = pdFALSE;

/* 置为 1 以从中断挂起一次上下文切换。 */
uint64_t ullPortYieldRequired = pdFALSE;

/*
 * 记录中断嵌套深度。仅当嵌套深度为 0 时才执行上下文切换。
 */
uint64_t ullPortInterruptNesting = 0;

/* 在汇编代码中使用。 */
__attribute__( ( used ) ) const uint64_t ullMaxAPIPriorityMask = ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 设置任务的初始栈。栈的布局与 portRESTORE_CONTEXT() 宏的期望完全一致。
     */

    /* 首先是所有通用寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0101010101010101ULL;        /* R1 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0303030303030303ULL;        /* R3 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0202020202020202ULL;        /* R2 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0505050505050505ULL;        /* R5 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0404040404040404ULL;        /* R4 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0707070707070707ULL;        /* R7 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0606060606060606ULL;        /* R6 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0909090909090909ULL;        /* R9 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0808080808080808ULL;        /* R8 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1111111111111111ULL;        /* R11 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1010101010101010ULL;        /* R10 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1313131313131313ULL;        /* R13 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1212121212121212ULL;        /* R12 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1515151515151515ULL;        /* R15 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1414141414141414ULL;        /* R14 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1717171717171717ULL;        /* R17 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1616161616161616ULL;        /* R16 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1919191919191919ULL;        /* R19 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1818181818181818ULL;        /* R18 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2121212121212121ULL;        /* R21 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2020202020202020ULL;        /* R20 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2323232323232323ULL;        /* R23 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2222222222222222ULL;        /* R22 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2525252525252525ULL;        /* R25 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2424242424242424ULL;        /* R24 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2727272727272727ULL;        /* R27 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2626262626262626ULL;        /* R26 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2929292929292929ULL;        /* R29 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x2828282828282828ULL;        /* R28 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00;         /* XZR - 无实际作用，用于使寄存器数量为偶数。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00;         /* R30 - 过程调用链接寄存器。 */

    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_PSTATE;

    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode; /* 异常返回地址。 */

    #if ( configUSE_TASK_FPU_SUPPORT == 1 )
    {
        /*
         * 任务启动时临界区嵌套计数为 0，因为中断已使能。
         */
        pxTopOfStack--;
        *pxTopOfStack = portNO_CRITICAL_NESTING;

        /*
         * 任务启动时没有浮点上下文。使用浮点硬件的任务必须在执行任何浮点指令之前
         * 调用 vPortTaskUsesFPU()。
         */
        pxTopOfStack--;
        *pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;
    }
    #elif ( configUSE_TASK_FPU_SUPPORT == 2 )
    {
        /*
         * 任务启动时将拥有浮点上下文。为寄存器预留足够空间，并确保将它们初始化为 0。
         */
        pxTopOfStack -= portFPU_REGISTER_WORDS;
        memset( pxTopOfStack, 0x00, portFPU_REGISTER_WORDS * sizeof( StackType_t ) );

        /*
         * 任务启动时临界区嵌套计数为 0，因为中断已使能。
         */
        pxTopOfStack--;
        *pxTopOfStack = portNO_CRITICAL_NESTING;

        pxTopOfStack--;
        *pxTopOfStack = pdTRUE;
        ullPortTaskHasFPUContext = pdTRUE;
    }
    #else /* if ( configUSE_TASK_FPU_SUPPORT == 1 )（任务 FPU 支持为 1） */
    {
        #error "Invalid configUSE_TASK_FPU_SUPPORT setting - configUSE_TASK_FPU_SUPPORT must be set to 1, 2, or left undefined."
    }
    #endif /* if ( configUSE_TASK_FPU_SUPPORT == 1 )（任务 FPU 支持为 1） */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    uint32_t ulAPSR;

    __asm volatile ( "MRS %0, CurrentEL" : "=r" ( ulAPSR ) );

    ulAPSR &= portAPSR_MODE_BITS_MASK;

    #if defined( GUEST )
        configASSERT( ulAPSR == portEL1 );

        if( ulAPSR == portEL1 )
    #else
        configASSERT( ulAPSR == portEL3 );

        if( ulAPSR == portEL3 )
    #endif
    {
        /*
         * 在 CPU 内部关闭中断，以确保调度器启动期间不会执行 tick。
         * 第一个任务开始执行时，CPU 会自动重新使能中断。
         */
        portDISABLE_INTERRUPTS();

        /* 启动产生 tick ISR 的定时器。 */
        configSETUP_TICK_INTERRUPT();

        /* 启动第一个任务开始执行。 */
        vPortRestoreTaskContext();
    }

    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在没有可返回之处的移植层中不实现。
     * 人为强制触发断言。
     */
    configASSERT( ullCriticalNesting == 1000ULL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    /* 将中断屏蔽到系统调用中断最大优先级。 */
    uxPortSetInterruptMask();

    /*
     * 现在中断已禁用，可以直接访问 ullCriticalNesting。
     * 递增 ullCriticalNesting，以记录 portENTER_CRITICAL() 被调用的次数。
     */
    ullCriticalNesting++;

    /*
     * 这不是进入临界区函数的中断安全版本，因此若从中断上下文调用则触发 assert()。
     * 中断中只能使用以 "FromISR" 结尾的 API 函数。
     * 仅当临界嵌套计数为 1 时才触发断言，以防断言函数也使用临界区而导致递归调用。
     */
    if( ullCriticalNesting == 1ULL )
    {
        configASSERT( ullPortInterruptNesting == 0 );
    }
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    if( ullCriticalNesting > portNO_CRITICAL_NESTING )
    {
        /*
         * 退出临界区时递减嵌套计数。
         */
        ullCriticalNesting--;

        /*
         * 若嵌套层次已归零，则必须重新使能所有中断优先级。
         */
        if( ullCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /*
             * 临界区嵌套计数已归零，因此应解除所有中断优先级的屏蔽。
             */
            portCLEAR_INTERRUPT_MASK();
        }
    }
}
/*-----------------------------------------------------------*/

void FreeRTOS_Tick_Handler( void )
{
    /* 必须为可能的最低优先级。 */
    #if !defined( QEMU )
    {
        uint64_t ullRunningInterruptPriority;
        /* s3_0_c12_c11_3 即 ICC_RPR_EL1。 */
        __asm volatile ( "MRS %0, s3_0_c12_c11_3" : "=r" ( ullRunningInterruptPriority ) );
        configASSERT( ullRunningInterruptPriority == ( portLOWEST_USABLE_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );
    }
    #endif

    /* 此处之前不应使能中断。 */
    #if ( configASSERT_DEFINED == 1 )
    {
        uint32_t ulMaskBits;

        __asm volatile ( "MRS %0, DAIF" : "=r" ( ulMaskBits )::"memory" );
        configASSERT( ( ulMaskBits & portDAIF_I ) != 0 );
    }
    #endif /* configASSERT_DEFINED（已定义 configASSERT） */

    /*
     * 在修改调度器结构之前设置中断屏蔽。tick 处理程序以最低优先级运行，
     * 因此中断不可能已被屏蔽，无需保存和恢复当前屏蔽值。
     * 更新 ICCPMR 期间必须在 CPU 内部关闭中断。
     */
    /* s3_0_c4_c6_0 即 ICC_PMR_EL1。 */
    __asm volatile ( "MSR s3_0_c4_c6_0, %0      \n"
                     "DSB SY                    \n"
                     "ISB SY                    \n"
                     ::"r" ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) : "memory" );

    /* 清除中断源后即可使能中断。 */
    configCLEAR_TICK_INTERRUPT();
    portENABLE_INTERRUPTS();

    /* 递增 RTOS tick。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        ullPortYieldRequired = pdTRUE;
    }

    /* 确保所有中断优先级重新处于活动状态。 */
    portCLEAR_INTERRUPT_MASK();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_FPU_SUPPORT != 2 )

void vPortTaskUsesFPU( void )
{
    /*
     * 任务正在登记其需要 FPU 上下文。设置 FPU 标志（该标志作为任务上下文的一部分保存）。
     */
    ullPortTaskHasFPUContext = pdTRUE;

    /*
     * 可以考虑在此处初始化 FPSR，但在 AArch64 中可能没有必要。
     */
}

#endif /* configUSE_TASK_FPU_SUPPORT（任务 FPU 支持） */
/*-----------------------------------------------------------*/

void vPortClearInterruptMask( UBaseType_t uxNewMaskValue )
{
    if( uxNewMaskValue == pdFALSE )
    {
        portCLEAR_INTERRUPT_MASK();
    }
}
/*-----------------------------------------------------------*/

UBaseType_t uxPortSetInterruptMask( void )
{
    uint32_t ulReturn;
    uint64_t ullPMRValue;

    /*
     * 更新 ICCPMR 期间必须关闭 CPU 中断。
     */
    portDISABLE_INTERRUPTS();
    /* s3_0_c4_c6_0 即 ICC_PMR_EL1。 */
    __asm volatile ( "MRS %0, s3_0_c4_c6_0" : "=r" ( ullPMRValue ) );

    if( ullPMRValue == ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) )
    {
        /* 中断此前已被屏蔽。 */
        ulReturn = pdTRUE;
    }
    else
    {
        ulReturn = pdFALSE;
        /* s3_0_c4_c6_0 即 ICC_PMR_EL1。 */
        __asm volatile ( "MSR s3_0_c4_c6_0, %0      \n"
                         "DSB SY                    \n"
                         "ISB SY                    \n"
                         ::"r" ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) : "memory" );
    }

    portENABLE_INTERRUPTS();

    return ulReturn;
}
/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

    void vPortValidateInterruptPriority( void )
    {
        /*
         * 若分配给某个中断的优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，
         * 而该中断的服务例程（ISR）调用了中断安全 FreeRTOS API 函数，
         * 则下面的断言将失败。中断安全 FreeRTOS API 函数*只能*从优先级
         * 等于或低于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
         *
         * 数值较小的中断优先级编号表示逻辑上较高的中断优先级，
         * 因此中断优先级必须设置为等于或数值上*高于*
         * configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
         *
         * FreeRTOS 维护独立的线程 API 和 ISR API 函数，
         * 以确保中断入口尽可能快速、简单。
         */
        uint64_t ullRunningInterruptPriority;
        /* s3_0_c12_c11_3 即 ICC_RPR_EL1。 */
        __asm volatile ( "MRS %0, s3_0_c12_c11_3" : "=r" ( ullRunningInterruptPriority ) );

        configASSERT( ullRunningInterruptPriority >= ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );
    }

#endif /* configASSERT_DEFINED（已定义 configASSERT） */
/*-----------------------------------------------------------*/

void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR )
{
    ( void ) ulICCIAR;
    configASSERT( ( volatile void * ) NULL );
}
