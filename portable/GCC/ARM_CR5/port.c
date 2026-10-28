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

#ifndef configINTERRUPT_CONTROLLER_BASE_ADDRESS
    #error configINTERRUPT_CONTROLLER_BASE_ADDRESS must be defined.  Refer to Cortex-A equivalent: http: /* 参见：www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html */
#endif

#ifndef configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET
    #error configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET must be defined.  Refer to Cortex-A equivalent: http: /* 参见：www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html */
#endif

#ifndef configUNIQUE_INTERRUPT_PRIORITIES
    #error configUNIQUE_INTERRUPT_PRIORITIES must be defined.  Refer to Cortex-A equivalent: http: /* 参见：www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html */
#endif

#ifndef configSETUP_TICK_INTERRUPT
    #error configSETUP_TICK_INTERRUPT() must be defined.  Refer to Cortex-A equivalent: http: /* 参见：www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html */
#endif /* configSETUP_TICK_INTERRUPT 条件编译结束 */

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
    #error configMAX_API_CALL_INTERRUPT_PRIORITY must be defined.  Refer to Cortex-A equivalent: http: /* 参见：www.FreeRTOS.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors.html */
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY == 0
    #error configMAX_API_CALL_INTERRUPT_PRIORITY must not be set to 0
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY > configUNIQUE_INTERRUPT_PRIORITIES
    #error configMAX_API_CALL_INTERRUPT_PRIORITY must be less than or equal to configUNIQUE_INTERRUPT_PRIORITIES as the lower the numeric priority value the higher the logical interrupt priority
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
    #endif
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

/* 安全扩展相关定义。 */
#if configMAX_API_CALL_INTERRUPT_PRIORITY <= ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )
    #error configMAX_API_CALL_INTERRUPT_PRIORITY must be greater than ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )
#endif

/*
 * __ARM_FP 在使能 FPU 支持时由 C 预处理器定义，通常借助 -mfpu= 和 -mfloat-abi= 参数。
 *
 * 注意：某些 C 标准库实现可能使用 FPU 寄存器执行通用内存操作（如 memcpy 等）。
 *       当设置 configUSE_TASK_FPU_SUPPORT == 1 时，必须确保在没有 FPU 上下文时
 *       不使用 FPU 寄存器。
 */
#if ( configUSE_TASK_FPU_SUPPORT == 0 )
    #ifdef __ARM_FP
        #error __ARM_FP is defined, so configUSE_TASK_FPU_SUPPORT must be set to either to 1 or 2.
    #endif /* __ARM_FP 条件编译结束 */
#elif ( configUSE_TASK_FPU_SUPPORT == 1 ) || ( configUSE_TASK_FPU_SUPPORT == 2 )
    #ifndef __ARM_FP
        #error __ARM_FP is not defined, so configUSE_TASK_FPU_SUPPORT must be set to 0.
    #endif /* __ARM_FP 条件编译结束 */
#endif /* configUSE_TASK_FPU_SUPPORT 条件编译结束 */

/*
 * 某些厂商特定文件会在 portmacro.h 中默认定义 configCLEAR_TICK_INTERRUPT()。
 */
#ifndef configCLEAR_TICK_INTERRUPT
    #define configCLEAR_TICK_INTERRUPT()
#endif

/*
 * 临界区嵌套计数达到该值时，临界区退出。
 */
#define portNO_CRITICAL_NESTING          ( ( uint32_t ) 0 )

/*
 * 在所有 GIC 中，可向优先级屏蔽寄存器写入 255，以解除屏蔽
 * 除最低优先级以外的所有中断优先级。
 */
#define portUNMASK_VALUE                 ( 0xFFUL )

/*
 * 任务创建时没有浮点上下文，但可在创建后为其分配浮点上下文。
 * 任务上下文中的变量用于保存该状态：若任务没有 FPU 上下文，
 * 则保存 portNO_FLOATING_POINT_CONTEXT；否则保存其他值。
 */
#define portNO_FLOATING_POINT_CONTEXT    ( ( StackType_t ) 0 )

/* 初始化任务初始上下文所需的常量。 */
#define portINITIAL_SPSR                 ( ( StackType_t ) 0x1f ) /* 系统模式、ARM 模式、使能 IRQ 和 FIQ。 */
#define portTHUMB_MODE_BIT               ( ( StackType_t ) 0x20 )
#define portINTERRUPT_ENABLE_BIT         ( 0x80UL )
#define portTHUMB_MODE_ADDRESS           ( 0x01UL )

/*
 * portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 在确保二进制小数点为零时使用。
 */
#define portBINARY_POINT_BITS            ( ( uint8_t ) 0x03 )

/* 屏蔽 APSR 中除模式位以外的所有位。 */
#define portAPSR_MODE_BITS_MASK          ( 0x1F )

/*
 * CPU 以用户模式执行时 APSR 中模式位的值。
 */
#define portAPSR_USER_MODE               ( 0x10 )

/*
 * 临界区宏只屏蔽到应用确定的优先级级别。有时在修改某些硬件寄存器前，
 * 必须关闭 CPU 自身的中断。
 */
#define portCPU_IRQ_DISABLE()                  \
    __asm volatile ( "CPSID i" ::: "memory" ); \
    __asm volatile ( "DSB" );                  \
    __asm volatile ( "ISB" );

#define portCPU_IRQ_ENABLE()                   \
    __asm volatile ( "CPSIE i" ::: "memory" ); \
    __asm volatile ( "DSB" );                  \
    __asm volatile ( "ISB" );


/* 解除屏蔽所有中断优先级的宏。 */
#define portCLEAR_INTERRUPT_MASK()                            \
    {                                                         \
        portCPU_IRQ_DISABLE();                                \
        portICCPMR_PRIORITY_MASK_REGISTER = portUNMASK_VALUE; \
        __asm volatile ( "DSB       \n"                       \
                         "ISB       \n" );                    \
        portCPU_IRQ_ENABLE();                                 \
    }

#define portINTERRUPT_PRIORITY_REGISTER_OFFSET    0x400UL
#define portMAX_8_BIT_VALUE                       ( ( uint8_t ) 0xff )
#define portBIT_0_SET                             ( ( uint8_t ) 0x01 )

/*
 * 允许用户覆盖初始 LR 的预装载值（即 prvTaskExitError() 的地址），
 * 以防调试器展开栈时受到干扰。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

#if ( configUSE_TASK_FPU_SUPPORT != 0 )

/*
 * 保存 FPU 寄存器所需的栈空间。
 *
 * ARM Cortex-R5 处理器实现 VFPv3-D16 FPU 架构，仅包含 16 个双精度寄存器，
 * 而非 VFPv3 的 32 个。寄存器组既可视为 16 个 64 位双字寄存器（D0-D15），
 * 也可视为 32 个 32 位单字寄存器（S0-S31），两种情况下寄存器组大小相同。
 * FPU 还具有一个 32 位状态寄存器。
 */
    #define portFPU_REGISTER_WORDS    ( ( 16 * 2 ) + 1 )
#endif /* configUSE_TASK_FPU_SUPPORT != 0 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * 启动第一个任务。该函数必须以汇编代码编写，因此实现在 portASM.s 中。
 */
extern void vPortRestoreTaskContext( void );

/*
 * 用于捕获试图从任务实现函数返回的任务。
 */
static void prvTaskExitError( void );

#if ( configUSE_TASK_FPU_SUPPORT != 0 )

/*
 * 若应用提供了 vApplicationIRQHandler() 的实现，则该函数会被直接调用，
 * 中断进入时不保存 FPU 寄存器；此处提供的 vApplicationFPUSafeIRQHandler()
 * 弱实现仅用于消除链接错误——它实际上永远不会被调用，因此其实现包含
 * 一个必然失败的 configASSERT() 调用。
 *
 * 若应用提供了自己的 vApplicationFPUSafeIRQHandler() 实现，则 portASM.S 中
 * 提供的 vApplicationIRQHandler() 会在调用它之前保存 FPU 寄存器。
 *
 * 因此，若应用作者希望在中断进入时保存 FPU 寄存器，其中断处理程序必须
 * 命名为 vApplicationFPUSafeIRQHandler()；若不希望保存 FPU 寄存器，
 * 则其中断处理程序必须命名为 vApplicationIRQHandler()。
 */
    void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR ) __attribute__( ( weak ) );
#endif /* configUSE_TASK_FPU_SUPPORT != 0 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * 该变量用于跟踪临界区嵌套计数。它必须作为任务上下文的一部分保存，
 * 且必须初始化为非零值，以确保调度器启动前中断不会被意外使能。
 * 由于它是任务上下文的一部分，在启动第一个任务时会自动置为 0。
 */
volatile uint32_t ulCriticalNesting = 9999UL;

#if ( configUSE_TASK_FPU_SUPPORT != 0 )

/*
 * 作为任务上下文的一部分保存。若 ulPortTaskHasFPUContext 非零，
 * 则必须为该任务保存和恢复浮点上下文。
 */
    uint32_t ulPortTaskHasFPUContext = pdFALSE;
#endif /* configUSE_TASK_FPU_SUPPORT != 0 条件编译结束 */

/* 置 1 表示从中断服务程序（ISR）中请求一次上下文切换。 */
uint32_t ulPortYieldRequired = pdFALSE;

/*
 * 统计中断嵌套深度。仅当嵌套深度为 0 时才会执行上下文切换。
 */
uint32_t ulPortInterruptNesting = 0UL;

/* 在汇编代码中使用。 */
__attribute__( ( used ) ) const uint32_t ulICCIAR = portICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS;
__attribute__( ( used ) ) const uint32_t ulICCEOIR = portICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS;
__attribute__( ( used ) ) const uint32_t ulICCPMR = portICCPMR_PRIORITY_MASK_REGISTER_ADDRESS;
__attribute__( ( used ) ) const uint32_t ulMaxAPIPriorityMask = ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );

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

    /*
     * 任务将以临界区嵌套计数 0 启动，因为此时中断已使能。
     */
    pxTopOfStack--;
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

        pxTopOfStack--;
        *pxTopOfStack = pdTRUE;
        ulPortTaskHasFPUContext = pdTRUE;
    }
    #elif ( configUSE_TASK_FPU_SUPPORT != 0 )
    {
        #error Invalid configUSE_TASK_FPU_SUPPORT setting - configUSE_TASK_FPU_SUPPORT must be set to 0, 1, or 2.
    }
    #endif /* configUSE_TASK_FPU_SUPPORT 条件编译结束 */

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

#if ( configUSE_TASK_FPU_SUPPORT != 0 )

    void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR ) /* 弱符号属性：__attribute__( ( weak ) ) */
    {
        ( void ) ulICCIAR;
        configASSERT( ( volatile void * ) NULL );
    }

#endif /* configUSE_TASK_FPU_SUPPORT != 0 条件编译结束 */
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    uint32_t ulAPSR, ulCycles = 8; /* 每字节 8 位。 */

    #if ( configASSERT_DEFINED == 1 )
    {
        volatile uint8_t ucOriginalPriority;
        volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( configINTERRUPT_CONTROLLER_BASE_ADDRESS + portINTERRUPT_PRIORITY_REGISTER_OFFSET );
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定 GIC 中实现了多少个优先级位。
         * 保存即将被覆盖的中断优先级值。
         */
        ucOriginalPriority = *pucFirstUserPriorityRegister;

        /*
         * 确定可用的优先级位数。首先向所有可能的位写入。
         */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

        /* 读回该值，查看有多少位保持有效。 */
        ucMaxPriorityValue = *pucFirstUserPriorityRegister;

        /* 移位到最低有效位。 */
        while( ( ucMaxPriorityValue & portBIT_0_SET ) != portBIT_0_SET )
        {
            ucMaxPriorityValue >>= ( uint8_t ) 0x01;

            /*
             * 若 ulCycles 减到 0，则说明读到的 ucMaxPriorityValue 为 0，
             * 表明配置有误。
             */
            ulCycles--;

            if( ulCycles == 0 )
            {
                break;
            }
        }

        /*
         * 检查 configUNIQUE_INTERRUPT_PRIORITIES 是否与读到的值匹配。
         */
        configASSERT( ucMaxPriorityValue == portLOWEST_INTERRUPT_PRIORITY );

        /*
         * 将此前被覆盖的中断优先级寄存器恢复为原值。
         */
        *pucFirstUserPriorityRegister = ucOriginalPriority;
    }
    #endif /* configASSERT_DEFINED 条件编译结束 */

    /*
     * 仅当 CPU 不处于用户模式时才继续。调度器启动时
     * CPU 必须处于特权模式。
     */
    __asm volatile ( "MRS %0, APSR" : "=r" ( ulAPSR )::"memory" );
    ulAPSR &= portAPSR_MODE_BITS_MASK;
    configASSERT( ulAPSR != portAPSR_USER_MODE );

    if( ulAPSR != portAPSR_USER_MODE )
    {
        /*
         * 仅当二进制小数点值设置为最低可能值时才继续。
         * 更多信息参见下面 vPortValidateInterruptPriority() 中的注释。
         */
        configASSERT( ( portICCBPR_BINARY_POINT_REGISTER & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE );

        if( ( portICCBPR_BINARY_POINT_REGISTER & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE )
        {
            /*
             * 在 CPU 自身中关闭中断，以确保调度器启动期间不会执行时钟节拍。
             * 第一个任务开始执行时，CPU 会自动重新使能中断。
             */
            portCPU_IRQ_DISABLE();

            /* 启动产生时钟节拍 ISR 的定时器。 */
            configSETUP_TICK_INTERRUPT();

            /* 启动第一个任务。 */
            vPortRestoreTaskContext();
        }
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
    /* 屏蔽中断至最大系统调用中断优先级。 */
    ulPortSetInterruptMask();

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
        /* 退出临界区时递减嵌套计数。 */
        ulCriticalNesting--;

        /*
         * 若嵌套计数已归零，则必须重新使能所有中断优先级。
         */
        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /*
             * 临界区嵌套已归零，因此应取消屏蔽所有中断优先级。
             */
            portCLEAR_INTERRUPT_MASK();
        }
    }
}
/*-----------------------------------------------------------*/

void FreeRTOS_Tick_Handler( void )
{
    /*
     * 在修改调度器结构前设置中断屏蔽。时钟节拍处理程序以最低优先级运行，
     * 因此中断不可能已被屏蔽，无需保存和恢复当前屏蔽值。更新 ICCPMR 时
     * 必须关闭 CPU 自身的中断。
     */
    portCPU_IRQ_DISABLE();
    portICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );
    __asm volatile ( "dsb       \n"
                     "isb       \n" ::: "memory" );
    portCPU_IRQ_ENABLE();

    /* 递增 RTOS 时钟节拍。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        ulPortYieldRequired = pdTRUE;
    }

    /* 确保所有中断优先级重新生效。 */
    portCLEAR_INTERRUPT_MASK();
    configCLEAR_TICK_INTERRUPT();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_FPU_SUPPORT == 1 )

    void vPortTaskUsesFPU( void )
    {
        uint32_t ulInitialFPSCR = 0;

        /*
         * 任务正在声明其需要 FPU 上下文。设置 FPU 标志
         * （该标志作为任务上下文的一部分保存）。
         */
        ulPortTaskHasFPUContext = pdTRUE;

        /* 初始化浮点状态寄存器。 */
        __asm volatile ( "FMXR  FPSCR, %0" ::"r" ( ulInitialFPSCR ) : "memory" );
    }

#endif /* configUSE_TASK_FPU_SUPPORT == 1 条件结束 */
/*-----------------------------------------------------------*/

void vPortClearInterruptMask( uint32_t ulNewMaskValue )
{
    if( ulNewMaskValue == pdFALSE )
    {
        portCLEAR_INTERRUPT_MASK();
    }
}
/*-----------------------------------------------------------*/

uint32_t ulPortSetInterruptMask( void )
{
    uint32_t ulReturn;

    /* 更新 ICCPMR 时必须屏蔽中断。 */
    portCPU_IRQ_DISABLE();

    if( portICCPMR_PRIORITY_MASK_REGISTER == ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) )
    {
        /* 中断此前已被屏蔽。 */
        ulReturn = pdTRUE;
    }
    else
    {
        ulReturn = pdFALSE;
        portICCPMR_PRIORITY_MASK_REGISTER = ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );
        __asm volatile ( "dsb       \n"
                         "isb       \n" ::: "memory" );
    }

    portCPU_IRQ_ENABLE();

    return ulReturn;
}
/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

    void vPortValidateInterruptPriority( void )
    {
        /*
         * 若某中断服务程序（ISR）的优先级高于
         * configMAX_SYSCALL_INTERRUPT_PRIORITY，却调用了中断安全的 FreeRTOS API 函数，
         * 则下面的断言将失败。中断安全的 FreeRTOS API 函数*只能*从优先级等于或低于
         * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
         *
         * 数值越小表示逻辑优先级越高，因此中断优先级必须设置为等于或数值上*高于*
         * configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
         *
         * FreeRTOS 分别维护线程和 ISR API 函数，以确保中断进入尽可能快速、简单。
         */

        configASSERT( portICCRPR_RUNNING_PRIORITY_REGISTER >= ( uint32_t ) ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );

        /*
         * 优先级分组：中断控制器（GIC）允许将定义各中断优先级的位划分为
         * 抢占优先级位和子优先级位。为简化处理，所有位都必须定义为抢占优先级位。
         * 若并非如此（某些位表示子优先级），下面的断言将失败。
         *
         * 优先级分组由 GIC 的二进制小数点寄存器（ICCBPR）配置。向 ICCBPR 写入 0
         * 可确保其设置为最低可能值（可能高于 0）。
         */
        configASSERT( ( portICCBPR_BINARY_POINT_REGISTER & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE );
    }

#endif /* configASSERT_DEFINED 条件编译结束 */
/*-----------------------------------------------------------*/
