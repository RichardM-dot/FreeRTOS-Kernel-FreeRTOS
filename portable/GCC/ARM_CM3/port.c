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
 * ARM CM3 移植层中 portable.h 所声明函数的实现。
 * ----------------------------------------------------------
 */

/* 调度器相关头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* 所有中断服务程序（ISR）的原型。 */
typedef void ( * portISR_t )( void );

/* 操作内核所需的常量。先是寄存器…… */
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR2_REG                    ( *( ( volatile uint32_t * ) 0xe000ed1c ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ……然后是寄存器中的位。 */
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT              ( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_SET_BIT         ( 1UL << 26UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT       ( 1UL << 25UL )

#define portMIN_INTERRUPT_PRIORITY            ( 255UL )
#define portNVIC_PENDSV_PRI                   ( ( ( uint32_t ) portMIN_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI                  ( ( ( uint32_t ) portMIN_INTERRUPT_PRIORITY ) << 24UL )

/* 用于检查 FreeRTOS 中断处理程序安装情况的常量。 */
#define portSCB_VTOR_REG                      ( *( ( portISR_t ** ) 0xE000ED08 ) )
#define portVECTOR_INDEX_SVC                  ( 11 )
#define portVECTOR_INDEX_PENDSV               ( 14 )

/* 用于检查中断优先级有效性的常量。 */
#define portFIRST_USER_INTERRUPT_NUMBER       ( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16       ( 0xE000E3F0 )
#define portAIRCR_REG                         ( *( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE                   ( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE                   ( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS                 ( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK               ( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT                    ( 8UL )

/* 屏蔽 ICSR 寄存器中除 VECTACTIVE 位以外的所有位。 */
#define portVECTACTIVE_MASK                   ( 0xFFUL )

/* 建立初始栈所需的常量。 */
#define portINITIAL_XPSR                      ( 0x01000000UL )

/* SysTick 是 24 位计数器。 */
#define portMAX_24_BIT_NUMBER                 ( 0xffffffUL )

/*
 * 用于估算 tickless 空闲计算期间 SysTick 计数器停止时
 * 可能错过的 SysTick 计数值的经验修正系数。
 */
#define portMISSED_COUNTS_FACTOR              ( 94UL )

/*
 * 为严格符合 Cortex-M 规范，任务起始地址的 bit0 应清零，
 * 因为该地址在退出 ISR 时被加载到 PC。
 */
#define portSTART_ADDRESS_MASK                ( ( StackType_t ) 0xfffffffeUL )

/*
 * 允许用户覆盖默认的 SysTick 时钟频率。若由用户定义，
 * 当配置寄存器中的 CLK 位为 0 时，该符号必须等于 SysTick 时钟频率。
 */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ             ( configCPU_CLOCK_HZ )
    /* 确保 SysTick 与内核使用相同频率的时钟。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( portNVIC_SYSTICK_CLK_BIT )
#else
    /* 选择 SysTick 不使用内核时钟频率的配置选项。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( 0 )
#endif

/*
 * 允许用户覆盖将初始 LR 预加载为 prvTaskExitError() 地址的行为，
 * 以防该地址影响调试器中的栈回溯。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/*
 * 配置定时器以产生时钟节拍中断。本文件中的实现为弱符号，
 * 便于应用开发者更换产生节拍中断的定时器。
 */
void vPortSetupTimerInterrupt( void );

/*
 * 异常处理程序。
 */
void xPortPendSVHandler( void ) __attribute__( ( naked ) );
void xPortSysTickHandler( void );
void vPortSVCHandler( void ) __attribute__( ( naked ) );

/*
 * 启动首个任务的代码单独成函数，便于独立测试。
 */
static void prvPortStartFirstTask( void ) __attribute__( ( naked ) );

/*
 * 用于捕获试图从任务入口函数返回的任务。
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * 每个任务通过临界区嵌套计数变量维护自身的中断状态。
 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * 构成一个时钟节拍周期的 SysTick 计数值。
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE 条件编译结束 */

/*
 * 可抑制的最大时钟节拍周期数受 SysTick 定时器
 * 24 位分辨率的限制。
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE 条件编译结束 */

/*
 * 补偿 SysTick 停止期间流逝的 CPU 周期（仅低功耗功能使用）。
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE 条件编译结束 */

/*
 * 供 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 宏使用，确保
 * 不在优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中
 * 调用 FreeRTOS API 函数。
 */
#if ( configASSERT_DEFINED == 1 )
    static uint8_t ucMaxSysCallPriority = 0;
    static uint32_t ulMaxPRIGROUPValue = 0;
    static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * const ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif /* configASSERT_DEFINED 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * 详见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 模拟上下文切换中断所创建的栈帧。
     */
    pxTopOfStack--;                                                      /* 补偿 MCU 进出中断时使用栈的方式所加的偏移量。 */
    *pxTopOfStack = portINITIAL_XPSR;                                    /* xPSR 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK; /* PC 程序计数器 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS;             /* LR 链接寄存器 */
    pxTopOfStack -= 5;                                                   /* R12、R3、R2、R1 寄存器 */
    *pxTopOfStack = ( StackType_t ) pvParameters;                        /* R0 寄存器 */
    pxTopOfStack -= 8;                                                   /* R11、R10、R9、R8、R7、R6、R5、R4 寄存器 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0UL;

    /*
     * 实现任务的函数不得退出，也不得试图返回调用者，因为不存在可返回的位置。
     * 若任务需要退出，应改为调用 vTaskDelete( NULL )。
     *
     * 若定义了 configASSERT()，则人为触发 assert()，
     * 然后在此处停止，以便应用开发者捕获该错误。
     */
    configASSERT( uxCriticalNesting == ~0UL );
    portDISABLE_INTERRUPTS();

    while( ulDummy == 0 )
    {
        /*
         * 调度器启动后，本文件会调用 prvTaskExitError()，以消除“函数已定义
         * 但从未调用”的编译器警告。ulDummy 仅用于消除有关该函数调用之后
         * 出现代码的其他警告——将 ulDummy 声明为 volatile 可使编译器认为
         * 该函数可能返回，从而不会为之后的代码输出“不可达代码”警告。
         */
    }
}
/*-----------------------------------------------------------*/

void vPortSVCHandler( void )
{
    __asm volatile (
        "   ldr r3, =pxCurrentTCB           \n" /* 恢复上下文。 */
        "   ldr r1, [r3]                    \n" /* 获取 pxCurrentTCB 的地址。 */
        "   ldr r0, [r1]                    \n" /* pxCurrentTCB 的第一项是任务栈顶指针。 */
        "   ldmia r0!, {r4-r11}             \n" /* 弹出异常进入时未自动保存的寄存器以及临界区嵌套计数。 */
        "   msr psp, r0                     \n" /* 恢复任务栈指针。 */
        "   isb                             \n"
        "   mov r0, #0                      \n"
        "   msr basepri, r0                 \n"
        "   orr r14, #0xd                   \n"
        "   bx r14                          \n"
        "                                   \n"
        "   .ltorg                          \n"
        );
}
/*-----------------------------------------------------------*/

static void prvPortStartFirstTask( void )
{
    __asm volatile (
        " ldr r0, =0xE000ED08   \n" /* 使用 NVIC 偏移寄存器定位栈。 */
        " ldr r0, [r0]          \n"
        " ldr r0, [r0]          \n"
        " msr msp, r0           \n" /* 将 MSP 重新设置为栈起始地址。 */
        " cpsie i               \n" /* 全局使能中断。 */
        " cpsie f               \n"
        " dsb                   \n"
        " isb                   \n"
        " svc 0                 \n" /* 通过系统调用启动首个任务。 */
        " nop                   \n"
        " .ltorg                \n"
        );
}
/*-----------------------------------------------------------*/

/*
 * 详见头文件中的说明。
 */
BaseType_t xPortStartScheduler( void )
{
    /*
     * 应用可通过以下方式之一安装 FreeRTOS 中断处理程序：
     * 1. 直接路由——分别为 SVCall 和 PendSV 中断安装 vPortSVCHandler
     *    和 xPortPendSVHandler 函数。
     * 2. 间接路由——为 SVCall 和 PendSV 中断安装独立的处理程序，
     *    并将程序控制从这些处理程序转发到 vPortSVCHandler 和
     *    xPortPendSVHandler 函数。
     *
     * 使用间接路由的应用必须在 FreeRTOSConfig.h 中将
     * configCHECK_HANDLER_INSTALLATION 设为 0。当
     * configCHECK_HANDLER_INSTALLATION 为 1 时，此处校验直接路由；
     * 条件允许时应优先采用直接路由。
     */
    #if ( configCHECK_HANDLER_INSTALLATION == 1 )
    {
        const portISR_t * const pxVectorTable = portSCB_VTOR_REG;

        /*
         * 校验应用已正确安装 SVCall 和 PendSV 中断的 FreeRTOS
         * 处理程序。此处不检查 SysTick 处理程序的安装情况，因为应用可能
         * 通过重写弱函数 vPortSetupTimerInterrupt()，改用 SysTick 之外的
         * 定时器驱动 RTOS 时钟节拍。
         *
         * 此处断言失败表明 FreeRTOS 处理程序安装不正确。有关安装
         * FreeRTOS 处理程序的帮助，请参见
         * https://www.freertos.org/Why-FreeRTOS/FAQs。
         *
         * 对于中断向量表地址可配置的系统，若 VTOR 未正确指向
         * 应用的向量表，此处也可能出现断言失败甚至系统故障。
         */
        configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == vPortSVCHandler );
        configASSERT( pxVectorTable[ portVECTOR_INDEX_PENDSV ] == xPortPendSVHandler );
    }
    #endif /* configCHECK_HANDLER_INSTALLATION 条件编译结束 */

    #if ( configASSERT_DEFINED == 1 )
    {
        volatile uint8_t ucOriginalPriority;
        volatile uint32_t ulImplementedPrioBits = 0;
        volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定可从中调用 ISR 安全 FreeRTOS API 函数的最高优先级。
         * ISR 安全函数是以“FromISR”结尾的函数。FreeRTOS 维护独立的线程和
         * ISR API 函数，以确保中断进入尽可能快速、简单。
         *
         * 保存即将被改写的原中断优先级值。
         */
        ucOriginalPriority = *pucFirstUserPriorityRegister;

        /*
         * 确定可用的优先级位数。先向所有可能的位写入 1。
         */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

        /* 回读该值，查看哪些位保持为 1。 */
        ucMaxPriorityValue = *pucFirstUserPriorityRegister;

        /* 对最大系统调用优先级施加相同的掩码。 */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /*
         * 检查在考虑硬件支持的优先级位数后，最大系统调用优先级非零。
         * 优先级 0 无效，因为将 BASEPRI 寄存器设为 0 会使所有中断解除屏蔽，
         * 且优先级为 0 的中断无法使用 BASEPRI 屏蔽。
         * 参见 https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        configASSERT( ucMaxSysCallPriority );

        /*
         * 检查 configMAX_SYSCALL_INTERRUPT_PRIORITY 中
         * 硬件未实现的位为 0。
         */
        configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY & ( ~ucMaxPriorityValue ) ) == 0U );

        /*
         * 根据回读的位数计算可接受的最大优先级分组值。
         */

        while( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
        {
            ulImplementedPrioBits++;
            ucMaxPriorityValue <<= ( uint8_t ) 0x01;
        }

        if( ulImplementedPrioBits == 8 )
        {
            /*
             * 当硬件实现 8 个优先级位时，软件无法将 PRIGROUP 配置为
             * 不存在子优先级。因此最低有效位始终用于子优先级，
             * 共有 128 个抢占优先级和 2 个子优先级。
             *
             * 这在某些情况下可能引起困惑——例如，若
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 设为 5，则优先级 5 和 4
             * 的中断都会在临界区中被屏蔽，因为它们处于同一抢占优先级。
             * 这可能显得费解，因为 4 的优先级高于（数值上小于）
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，本不应被屏蔽。
             * 反之，若将 configMAX_SYSCALL_INTERRUPT_PRIORITY 设为 4，
             * 这种困惑便不会发生，而行为保持不变。
             *
             * 以下断言确保 configMAX_SYSCALL_INTERRUPT_PRIORITY 中的
             * 子优先级位为 0，以避免上述困惑。
             */
            configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY & 0x1U ) == 0U );
            ulMaxPRIGROUPValue = 0;
        }
        else
        {
            ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS - ulImplementedPrioBits;
        }

        /*
         * 将优先级分组值移回其在 AIRCR 寄存器中的位置。
         */
        ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
        ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;

        /*
         * 将先前被改写的中断优先级寄存器恢复为原值。
         */
        *pucFirstUserPriorityRegister = ucOriginalPriority;
    }
    #endif /* configASSERT_DEFINED 条件编译结束 */

    /*
     * 将 PendSV 和 SysTick 设为最低优先级中断，
     * 并将 SVCall 设为最高优先级。
     */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
    portNVIC_SHPR2_REG = 0;

    /*
     * 启动产生时钟节拍 ISR 的定时器。此处中断已处于禁用状态。
     */
    vPortSetupTimerInterrupt();

    /* 初始化临界区嵌套计数，为第一个任务做好准备。 */
    uxCriticalNesting = 0;

    /* 启动第一个任务。 */
    prvPortStartFirstTask();

    /*
     * 此时任务已在运行，绝不应到达这里！调用任务退出错误函数，
     * 以避免在应用开发者通过定义 configTASK_RETURN_ADDRESS 覆盖
     * 此功能时，出现“静态函数未被调用”的编译器警告。调用
     * vTaskSwitchContext()，以免链接时优化移除该符号。
     */
    vTaskSwitchContext();
    prvTaskExitError();

    /* 不应到达此处！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在无返回目标的移植层中不实现该函数。
     * 人为触发断言。
     */
    configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;

    /*
     * 本函数不是可中断安全版本的进入临界区函数，因此若在中断
     * 上下文中调用则触发 assert()。只有以“FromISR”结尾的 API
     * 函数才能在中断中使用。仅当临界区嵌套计数为 1 时才断言，
     * 以避免 assert 函数本身使用临界区时发生递归调用。
     */
    if( uxCriticalNesting == 1 )
    {
        configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
    }
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting );
    uxCriticalNesting--;

    if( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}
/*-----------------------------------------------------------*/

void xPortPendSVHandler( void )
{
    /* 这是一个裸函数。 */

    __asm volatile
    (
        "   mrs r0, psp                         \n"
        "   isb                                 \n"
        "                                       \n"
        "   ldr r3, =pxCurrentTCB               \n" /* 获取当前 TCB 的地址。 */
        "   ldr r2, [r3]                        \n"
        "                                       \n"
        "   stmdb r0!, {r4-r11}                 \n" /* 保存其余寄存器。 */
        "   str r0, [r2]                        \n" /* 将新的栈顶保存到 TCB 的第一个成员中。 */
        "                                       \n"
        "   stmdb sp!, {r3, r14}                \n"
        "   mov r0, %0                          \n"
        "   msr basepri, r0                     \n"
        "   bl vTaskSwitchContext               \n"
        "   mov r0, #0                          \n"
        "   msr basepri, r0                     \n"
        "   ldmia sp!, {r3, r14}                \n"
        "                                       \n" /* 恢复上下文，包括临界区嵌套计数。 */
        "   ldr r1, [r3]                        \n"
        "   ldr r0, [r1]                        \n" /* pxCurrentTCB 的第一项是任务栈顶指针。 */
        "   ldmia r0!, {r4-r11}                 \n" /* 弹出寄存器。 */
        "   msr psp, r0                         \n"
        "   isb                                 \n"
        "   bx r14                              \n"
        "                                       \n"
        "   .ltorg                              \n"
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
    );
}
/*-----------------------------------------------------------*/

void xPortSysTickHandler( void )
{
    /*
     * SysTick 以最低中断优先级运行，因此该中断执行时
     * 所有中断必然处于未屏蔽状态。因此无需保存并恢复中断
     * 屏蔽值，因为其值已知。
     */
    portDISABLE_INTERRUPTS();
    traceISR_ENTER();
    {
        /* 递增 RTOS 时钟节拍。 */
        if( xTaskIncrementTick() != pdFALSE )
        {
            traceISR_EXIT_TO_SCHEDULER();

            /*
             * 需要进行上下文切换。上下文切换在 PendSV 中断中执行。
             * 挂起 PendSV 中断。
             */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
        else
        {
            traceISR_EXIT();
        }
    }
    portENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE == 1 )

    __attribute__( ( weak ) ) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
    {
        uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickDecrementsLeft;
        TickType_t xModifiableIdleTime;

        /* 确保 SysTick 重装载值不会使计数器溢出。 */
        if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
        {
            xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
        }

        /*
         * 进入临界区，但不能使用 taskENTER_CRITICAL()
         * 方法，因为该方法会屏蔽本应唤醒睡眠模式的中断。
         */
        __asm volatile ( "cpsid i" ::: "memory" );
        __asm volatile ( "dsb" );
        __asm volatile ( "isb" );

        /*
         * 若有上下文切换挂起，或某任务正在等待调度器解除挂起，
         * 则放弃进入低功耗模式。
         */
        if( eTaskConfirmSleepModeStatus() == eAbortSleep )
        {
            /*
             * 重新使能中断——见上方 cpsid 指令之前的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
        else
        {
            /*
             * 暂时停止 SysTick。停止期间的时间已尽可能补偿，
             * 但使用 tickless 模式仍不可避免地会使内核维护的时间
             * 相对日历时间产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /*
             * 使用 SysTick 当前值寄存器确定距离下一个时钟节拍中断
             * 还剩多少次 SysTick 递减。若当前值寄存器为零，则实际
             * 还剩 ulTimerCountsForOneTick 次递减而非零，因为 SysTick
             * 在从 1 递减到 0 时请求中断。
             */
            ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;

            if( ulSysTickDecrementsLeft == 0 )
            {
                ulSysTickDecrementsLeft = ulTimerCountsForOneTick;
            }

            /*
             * 计算等待 xExpectedIdleTime 个时钟节拍周期所需的重装载值。
             * 使用 -1 是因为本代码通常在第一个节拍周期进行到一半时执行。
             * 但若 SysTick IRQ 当前挂起，则清除该 IRQ（抑制第一个节拍），
             * 并修正重装载值以反映第二个节拍周期已经开始。
             * 预期空闲时间始终至少为两个节拍。
             */
            ulReloadValue = ulSysTickDecrementsLeft + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );

            if( ( portNVIC_INT_CTRL_REG & portNVIC_PEND_SYSTICK_SET_BIT ) != 0 )
            {
                portNVIC_INT_CTRL_REG = portNVIC_PEND_SYSTICK_CLEAR_BIT;
                ulReloadValue -= ulTimerCountsForOneTick;
            }

            if( ulReloadValue > ulStoppedTimerCompensation )
            {
                ulReloadValue -= ulStoppedTimerCompensation;
            }

            /* 设置新的重装载值。 */
            portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

            /*
             * 清除 SysTick 计数标志并将计数值重新设为零。
             */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

            /* 重新启动 SysTick。 */
            portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

            /*
             * 休眠直到有事件发生。configPRE_SLEEP_PROCESSING() 可
             * 将其参数设为 0，以表示其实现自身包含等待中断（WFI）或
             * 等待事件（WFE）指令，因此不应再次执行 wfi。不过，
             * 原始预期空闲时间变量必须保持不变，因此此处取用其副本。
             */
            xModifiableIdleTime = xExpectedIdleTime;
            configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

            if( xModifiableIdleTime > 0 )
            {
                __asm volatile ( "dsb" ::: "memory" );
                __asm volatile ( "wfi" );
                __asm volatile ( "isb" );
            }

            configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

            /*
             * 重新使能中断，使将 MCU 唤醒的中断能够立即执行。
             * 见上方 cpsid 指令之前的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 再次禁用中断，因为时钟即将停止，而在时钟停止期间
             * 执行的中断会加剧 RTOS 维护时间与日历时间之间的偏差。
             */
            __asm volatile ( "cpsid i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 禁用 SysTick 时钟，且不读取
             * portNVIC_SYSTICK_CTRL_REG 寄存器，以确保
             * portNVIC_SYSTICK_COUNT_FLAG_BIT 若已置位则不会被清除。
             * 同样，停止 SysTick 期间的时间已尽可能补偿，但使用
             * tickless 模式仍不可避免地会使内核维护的时间相对日历
             * 时间产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /* 判断 SysTick 是否已计数到零。 */
            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                uint32_t ulCalculatedLoadValue;

                /*
                 * 时钟节拍中断结束了休眠（或当前挂起），且
                 * 新的节拍周期已经开始。用新节拍周期的剩余部分
                 * 重置 portNVIC_SYSTICK_LOAD_REG。
                 */
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

                /*
                 * 不允许使用过小的值，也不允许使用因休眠后钩子执行
                 * 耗时过长或 SysTick 当前值寄存器为零而发生下溢的值。
                 */
                if( ( ulCalculatedLoadValue <= ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
                {
                    ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
                }

                portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

                /*
                 * 由于挂起的节拍将在此函数退出后立即处理，
                 * 节拍维护的计数值向前推进的量为等待时间减一。
                 */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /* 时钟节拍中断以外的其他事件结束了休眠。 */

                /*
                 * 使用 SysTick 当前值寄存器确定距离预期空闲时间
                 * 结束还剩多少次 SysTick 递减。
                 */
                ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;
                #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG != portNVIC_SYSTICK_CLK_BIT )
                {
                    /*
                     * 若 SysTick 未使用内核时钟，则当前值寄存器此时可能仍为零。
                     * 此时 SysTick 尚未从重装载寄存器装载，预期空闲时间内
                     * 还剩 ulReloadValue 次递减，而非零。
                     */
                    if( ulSysTickDecrementsLeft == 0 )
                    {
                        ulSysTickDecrementsLeft = ulReloadValue;
                    }
                }
                #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG 条件编译结束 */

                /*
                 * 计算休眠持续时间并取整为完整时钟节拍周期
                 * （而非包含部分节拍的 ulReloadValue 值）。
                 */
                ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - ulSysTickDecrementsLeft;

                /*
                 * 处理器等待期间经过了多少个完整的时钟节拍周期？
                 */
                ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

                /*
                 * 将重装载值设为单个节拍周期剩余的部分。
                 */
                portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
            }

            /*
             * 重新启动 SysTick，使其再次从 portNVIC_SYSTICK_LOAD_REG 开始
             * 计数，然后将 portNVIC_SYSTICK_LOAD_REG 恢复为标准值。若
             * SysTick 未使用内核时钟，则临时将其配置为使用内核时钟。
             * 此配置强制 SysTick 立即从 portNVIC_SYSTICK_LOAD_REG 装载，
             * 而不是在另一时钟的下一个周期才装载。这样
             * portNVIC_SYSTICK_LOAD_REG 就能立即接收标准值。
             */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
            portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG == portNVIC_SYSTICK_CLK_BIT )
            {
                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
            }
            #else
            {
                /*
                 * 如上所述，临时使用内核时钟已完成其使命。
                 * 恢复使用另一时钟。
                 */
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT;

                if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
                {
                    /*
                     * 部分节拍周期已经结束。确保 SysTick
                     * 仅对其计数一次。
                     */
                    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
                }

                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            }
            #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG 条件编译结束 */

            /* 推进时钟节拍，以补偿已经流逝的节拍周期。 */
            vTaskStepTick( ulCompleteTickPeriods );

            /* 在中断使能状态下退出。 */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
    }

#endif /* configUSE_TICKLESS_IDLE 条件编译结束 */
/*-----------------------------------------------------------*/

/*
 * 配置 SysTick 定时器，以所需频率产生时钟节拍中断。
 */
__attribute__( ( weak ) ) void vPortSetupTimerInterrupt( void )
{
    /* 计算配置时钟节拍中断所需的常量。 */
    #if ( configUSE_TICKLESS_IDLE == 1 )
    {
        ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
        xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
        ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
    }
    #endif /* configUSE_TICKLESS_IDLE 条件编译结束 */

    /* 停止并清除 SysTick。 */
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* 配置 SysTick 以按请求的速率产生中断。 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}
/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

    void vPortValidateInterruptPriority( void )
    {
        uint32_t ulCurrentInterrupt;
        uint8_t ucCurrentPriority;

        /* 获取当前正在执行的中断编号。 */
        __asm volatile ( "mrs %0, ipsr" : "=r" ( ulCurrentInterrupt )::"memory" );

        /* 该中断编号是否为用户定义的中断？ */
        if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
        {
            /* 查询该中断的优先级。 */
            ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

            /*
             * 若某中断已被分配的优先级高于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，而其中断服务程序（ISR）
             * 调用了 ISR 安全的 FreeRTOS API 函数，则以下断言将失败。
             * ISR 安全的 FreeRTOS API 函数*只能*在优先级等于或低于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
             *
             * 数值较小的中断优先级编号表示逻辑上较高的中断优先级，
             * 因此中断的优先级必须设置为等于或数值*大于*
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
             *
             * 使用 FreeRTOS API 的中断不得保持默认优先级 0，
             * 因为 0 是可能的最高优先级，必然高于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，因此也必然是无效的。
             *
             * FreeRTOS 维护独立的线程和 ISR API 函数，以确保
             * 中断进入尽可能快速、简单。
             *
             * 以下链接提供详细信息：
             * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
             * https://www.freertos.org/Why-FreeRTOS/FAQs
             */
            configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
        }

        /*
         * 优先级分组：中断控制器（NVIC）允许将定义每个中断优先级的位
         * 划分为定义中断抢占优先级的位和定义子优先级的位。为简化起见，
         * 所有位都必须定义为抢占优先级位。若情况并非如此
         * （某些位表示子优先级），则以下断言将失败。
         *
         * 若应用仅使用 CMSIS 库配置中断，则可在启动调度器前调用
         * NVIC_SetPriorityGrouping( 0 );，从而在所有 Cortex-M 器件上
         * 获得正确设置。但请注意，某些厂商特定的外设库假定优先级
         * 分组设置为非零值，此时使用 0 会导致不可预测的行为。
         */
        configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
    }

#endif /* configASSERT_DEFINED 条件编译结束 */
