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
 * ARM CM7 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

/* 调度器相关头文件。 */
#include "FreeRTOS.h"
#include "task.h"

#ifndef __ARM_FP
    #error This port can only be used when the project options are configured to enable hardware floating point support.
#endif

/* 所有中断服务程序（ISR）的原型。 */
typedef void ( * portISR_t )( void );

/* 操作内核所需的常量。首先是寄存器…… */
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

/* 操作 VFP 所需的常量。 */
#define portFPCCR                             ( ( volatile uint32_t * ) 0xe000ef34 ) /* 浮点上下文控制寄存器。 */
#define portASPEN_AND_LSPEN_BITS              ( 0x3UL << 30UL )

/* 建立初始栈所需的常量。 */
#define portINITIAL_XPSR                      ( 0x01000000 )
#define portINITIAL_EXC_RETURN                ( 0xfffffffd )

/* SysTick 是 24 位计数器。 */
#define portMAX_24_BIT_NUMBER                 ( 0xffffffUL )

/*
 * 为严格符合 Cortex-M 规范，任务起始地址的
 * bit 0 应为 0，因为从 ISR 退出时该地址会被装入 PC。
 */
#define portSTART_ADDRESS_MASK                ( ( StackType_t ) 0xfffffffeUL )

/*
 * 用于估算在 tickless 空闲计算期间 SysTick 计数器停止时
 * 本应发生的 SysTick 计数次数的修正系数。
 */
#define portMISSED_COUNTS_FACTOR              ( 94UL )

/*
 * 允许用户覆盖默认的 SysTick 时钟频率。若由用户定义，
 * 该符号必须等于配置寄存器中 CLK 位为 0 时的 SysTick 时钟频率。
 */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ             ( configCPU_CLOCK_HZ )
    /* 确保 SysTick 与内核使用相同频率的时钟。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( portNVIC_SYSTICK_CLK_BIT )
#else
    /* 选择 SysTick 不与内核同频的时钟选项。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( 0 )
#endif

/*
 * 允许用户覆盖初始 LR 中预加载的 prvTaskExitError() 地址，
 * 以防该地址干扰调试器中的栈回溯。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/*
 * 初始化定时器以产生时钟节拍中断。本文件中的实现为弱定义，
 * 以便应用程序编写者更换用于产生时钟节拍中断的定时器。
 */
void vPortSetupTimerInterrupt( void );

/*
 * 异常处理程序。
 */
void xPortPendSVHandler( void ) __attribute__( ( naked ) );
void xPortSysTickHandler( void );
void vPortSVCHandler( void ) __attribute__( ( naked ) );

/*
 * 启动第一个任务的函数单独实现，以便进行隔离测试。
 */
static void prvPortStartFirstTask( void ) __attribute__( ( naked ) );

/*
 * 使能 VFP 的函数。
 */
static void vPortEnableVFP( void ) __attribute__( ( naked ) );

/*
 * 用于捕获试图从其实现函数返回的任务。
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * 每个任务通过临界区嵌套计数变量维护自身的中断状态。
 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * 构成一个时钟节拍周期的 SysTick 递增次数。
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */

/*
 * 可抑制的最大时钟节拍周期数受 SysTick 定时器 24 位分辨率的限制。
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */

/*
 * 补偿 SysTick 停止期间流逝的 CPU 周期（仅用于低功耗功能）。
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */

/*
 * 供 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 宏使用，
 * 以确保不会从优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY
 * 的中断中调用 FreeRTOS API 函数。
 */
#if ( configASSERT_DEFINED == 1 )
    static uint8_t ucMaxSysCallPriority = 0;
    static uint32_t ulMaxPRIGROUPValue = 0;
    static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * const ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif /* configASSERT_DEFINED（已定义 configASSERT） */

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 模拟上下文切换中断会创建的栈帧。
     */

    /*
     * 用于补偿 MCU 在中断进入/退出时使用栈的方式并确保对齐
     * 而增加的偏移量。
     */
    pxTopOfStack--;

    *pxTopOfStack = portINITIAL_XPSR;                                    /* xPSR 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK; /* PC 寄存器。 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS;             /* LR 寄存器。 */

    /* 跳过寄存器初始化以节省代码空间。 */
    pxTopOfStack -= 5;                            /* R12、R3、R2 和 R1 寄存器。 */
    *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器。 */

    /*
     * 采用了一种保存方法，要求每个任务维护
     * 自己的执行返回（EXC_RETURN）值。
     */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_EXC_RETURN;

    pxTopOfStack -= 8; /* R11、R10、R9、R8、R7、R6、R5 和 R4 寄存器。 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0;

    /*
     * 实现任务的函数不得退出或试图返回其调用者，因为无处可返回。
     * 若任务需要退出，应改为调用 vTaskDelete( NULL )。
     *
     * 若定义了 configASSERT()，则人为触发 assert()，然后停在此处，
     * 以便应用程序编写者捕获该错误。
     */
    configASSERT( uxCriticalNesting == ~0UL );
    portDISABLE_INTERRUPTS();

    while( ulDummy == 0 )
    {
        /*
         * 调度器启动后，本文件会调用 prvTaskExitError()，以消除
         * 有关函数已定义但从未调用的编译器警告。ulDummy 纯粹用于
         * 消除有关该函数调用之后出现代码的其他警告——将 ulDummy
         * 声明为 volatile 会使编译器认为该函数可能返回，从而不会对
         * 其后的代码输出“不可达代码”警告。
         */
    }
}
/*-----------------------------------------------------------*/

void vPortSVCHandler( void )
{
    __asm volatile (
        "   ldr r3, =pxCurrentTCB           \n" /* 恢复上下文。 */
        "   ldr r1, [r3]                    \n" /* 获取 pxCurrentTCB 的地址。 */
        "   ldr r0, [r1]                    \n" /* pxCurrentTCB 中的第一项是任务栈顶。 */
        "   ldmia r0!, {r4-r11, r14}        \n" /* 弹出异常进入时不会自动保存的寄存器以及临界区嵌套计数。 */
        "   msr psp, r0                     \n" /* 恢复任务栈指针。 */
        "   isb                             \n"
        "   mov r0, #0                      \n"
        "   msr basepri, r0                 \n"
        "   bx r14                          \n"
        "                                   \n"
        "   .ltorg                          \n"
        );
}
/*-----------------------------------------------------------*/

static void prvPortStartFirstTask( void )
{
    /*
     * 启动第一个任务。这还会清除表示 FPU 正在使用的位，以防
     * 调度器启动前使用过 FPU——否则会在 SVC 栈中为延迟保存
     * FPU 寄存器无谓地预留空间。
     */
    __asm volatile (
        " ldr r0, =0xE000ED08   \n" /* 使用 NVIC 偏移寄存器定位栈。 */
        " ldr r0, [r0]          \n"
        " ldr r0, [r0]          \n"
        " msr msp, r0           \n" /* 将 MSP 恢复为栈的起始位置。 */
        " mov r0, #0            \n" /* 清除表示 FPU 正在使用的位，参见上面的注释。 */
        " msr control, r0       \n"
        " cpsie i               \n" /* 全局使能中断。 */
        " cpsie f               \n"
        " dsb                   \n"
        " isb                   \n"
        " svc 0                 \n" /* 通过系统调用启动第一个任务。 */
        " nop                   \n"
        " .ltorg                \n"
        );
}
/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
BaseType_t xPortStartScheduler( void )
{
    /*
     * 应用程序可以采用以下方式之一安装 FreeRTOS 中断处理程序：
     * 1. 直接路由——分别为 SVCall 和 PendSV 中断安装
     *    vPortSVCHandler 和 xPortPendSVHandler 函数。
     * 2. 间接路由——为 SVCall 和 PendSV 中断安装独立的处理程序，
     *    并将程序控制流从这些处理程序转发到 vPortSVCHandler 和
     *    xPortPendSVHandler 函数。
     *
     * 使用间接路由的应用程序必须在其 FreeRTOSConfig.h 中将
     * configCHECK_HANDLER_INSTALLATION 设置为 0。在
     * configCHECK_HANDLER_INSTALLATION 为 1 时于此处校验的直接
     * 路由应尽可能优先使用。
     */
    #if ( configCHECK_HANDLER_INSTALLATION == 1 )
    {
        const portISR_t * const pxVectorTable = portSCB_VTOR_REG;

        /*
         * 校验应用程序已正确安装 SVCall 和 PendSV 中断的 FreeRTOS
         * 处理程序。此处不检查 SysTick 处理程序的安装情况，因为应用程序
         * 可能通过重写弱函数 vPortSetupTimerInterrupt() 使用 SysTick
         * 以外的定时器驱动 RTOS 时钟节拍。
         *
         * 此处的断言失败表明 FreeRTOS 处理程序安装不正确。有关安装
         * FreeRTOS 处理程序的帮助，请参阅
         * https://www.freertos.org/Why-FreeRTOS/FAQs。
         *
         * 对于中断向量表地址可配置的系统，若 VTOR 未正确设置为指向
         * 应用程序的向量表，此处也可能出现断言失败甚至系统故障。
         */
        configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == vPortSVCHandler );
        configASSERT( pxVectorTable[ portVECTOR_INDEX_PENDSV ] == xPortPendSVHandler );
    }
    #endif /* configCHECK_HANDLER_INSTALLATION（检查处理程序安装） */

    #if ( configASSERT_DEFINED == 1 )
    {
        volatile uint8_t ucOriginalPriority;
        volatile uint32_t ulImplementedPrioBits = 0;
        volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定可以调用 ISR 安全 FreeRTOS API 函数的最高优先级。
         * ISR 安全函数是指以“FromISR”结尾的函数。FreeRTOS 同时提供
         * 独立的任务级和 ISR API 函数，以确保中断入口尽可能快速简单。
         *
         * 保存即将被覆盖的中断优先级值。
         */
        ucOriginalPriority = *pucFirstUserPriorityRegister;

        /*
         * 确定可用的优先级位数。首先向所有可能的位写入 1。
         */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

        /* 读回该值，查看有多少位保持置位。 */
        ucMaxPriorityValue = *pucFirstUserPriorityRegister;

        /* 对最大系统调用优先级使用相同的掩码。 */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /*
         * 检查在考虑硬件支持的优先级位数后，最大系统调用优先级
         * 不为零。优先级 0 无效，因为将 BASEPRI 寄存器设置为 0
         * 会解除所有中断的屏蔽，而优先级为 0 的中断无法使用
         * BASEPRI 屏蔽。
         * 参见 https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        configASSERT( ucMaxSysCallPriority );

        /*
         * 检查 configMAX_SYSCALL_INTERRUPT_PRIORITY 中
         * 硬件未实现的位是否为零。
         */
        configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY & ( ~ucMaxPriorityValue ) ) == 0U );

        /*
         * 根据读回的位数计算可接受的最大优先级分组值。
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
             * 不含子优先级。因此最低有效位始终用于子优先级，
             * 共有 128 个抢占优先级和 2 个子优先级。
             *
             * 这在某些情况下可能引起混淆——例如，若
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 设置为 5，则优先级 5 和 4
             * 的中断都会在临界区中被屏蔽，因为它们处于相同的抢占优先级。
             * 这可能令人困惑，因为 4 的优先级高于（数值上小于）
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，按理不应被屏蔽。
             * 反之，若将 configMAX_SYSCALL_INTERRUPT_PRIORITY 设置为 4，
             * 这种混淆就不会发生，行为保持一致。
             *
             * 下面的断言确保 configMAX_SYSCALL_INTERRUPT_PRIORITY 中的
             * 子优先级位为 0，以避免上述混淆。
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
         * 将先前被覆盖的中断优先级寄存器恢复为原值。
         */
        *pucFirstUserPriorityRegister = ucOriginalPriority;
    }
    #endif /* configASSERT_DEFINED（已定义 configASSERT） */

    /*
     * 将 PendSV 和 SysTick 设为最低优先级中断，并将 SVCall
     * 设为最高优先级。
     */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
    portNVIC_SHPR2_REG = 0;

    /*
     * 启动产生时钟节拍 ISR 的定时器。此处中断已经处于禁用状态。
     */
    vPortSetupTimerInterrupt();

    /* 初始化临界区嵌套计数，为第一个任务做好准备。 */
    uxCriticalNesting = 0;

    /* 确保 VFP 已使能——无论如何都应处于使能状态。 */
    vPortEnableVFP();

    /* 始终启用延迟保存。 */
    *( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;

    /* 启动第一个任务。 */
    prvPortStartFirstTask();

    /*
     * 任务此时已经开始执行，不应到达这里！调用任务退出错误函数，
     * 以避免在应用程序编写者通过定义 configTASK_RETURN_ADDRESS
     * 覆盖此功能时出现静态函数未被调用的编译器警告。调用
     * vTaskSwitchContext() 可防止链接时优化移除该符号。
     */
    vTaskSwitchContext();
    prvTaskExitError();

    /* 不应到达这里！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在无处可返回的移植层中未实现。
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
     * 这不是进入临界区函数的中断安全版本，因此若从中断上下文
     * 调用则触发 assert()。中断中只能使用以“FromISR”结尾的 API
     * 函数。仅当临界区嵌套计数为 1 时才断言，以防 assert 函数
     * 也使用临界区而导致递归调用。
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
        "   ldr r3, =pxCurrentTCB               \n" /* 获取当前 TCB 的位置。 */
        "   ldr r2, [r3]                        \n"
        "                                       \n"
        "   tst r14, #0x10                      \n" /* 任务是否使用 FPU 上下文？若是，则压入高 VFP 寄存器。 */
        "   it eq                               \n"
        "   vstmdbeq r0!, {s16-s31}             \n"
        "                                       \n"
        "   stmdb r0!, {r4-r11, r14}            \n" /* 保存内核寄存器。 */
        "   str r0, [r2]                        \n" /* 将新的栈顶保存到 TCB 的第一个成员。 */
        "                                       \n"
        "   stmdb sp!, {r0, r3}                 \n"
        "   mov r0, %0                          \n"
        "   cpsid i                             \n" /* ARM Cortex-M7 r0p1 勘误 837070 的变通方法。 */
        "   msr basepri, r0                     \n"
        "   dsb                                 \n"
        "   isb                                 \n"
        "   cpsie i                             \n" /* ARM Cortex-M7 r0p1 勘误 837070 的变通方法。 */
        "   bl vTaskSwitchContext               \n"
        "   mov r0, #0                          \n"
        "   msr basepri, r0                     \n"
        "   ldmia sp!, {r0, r3}                 \n"
        "                                       \n"
        "   ldr r1, [r3]                        \n" /* pxCurrentTCB 中的第一项是任务栈顶。 */
        "   ldr r0, [r1]                        \n"
        "                                       \n"
        "   ldmia r0!, {r4-r11, r14}            \n" /* 弹出内核寄存器。 */
        "                                       \n"
        "   tst r14, #0x10                      \n" /* 任务是否使用 FPU 上下文？若是，则同时弹出高 VFP 寄存器。 */
        "   it eq                               \n"
        "   vldmiaeq r0!, {s16-s31}             \n"
        "                                       \n"
        "   msr psp, r0                         \n"
        "   isb                                 \n"
        "                                       \n"
        #ifdef WORKAROUND_PMU_CM001 /* XMC4000 专用勘误变通方法。 */
            #if WORKAROUND_PMU_CM001 == 1
                "           push { r14 }                \n"
                "           pop { pc }                  \n"
            #endif
        #endif
        "                                       \n"
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
     * SysTick 以最低中断优先级运行，因此该中断执行时所有中断
     * 必定已解除屏蔽。所以无需保存并恢复中断屏蔽值，
     * 其值是已知的。
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
         * 进入临界区，但不使用 taskENTER_CRITICAL()
         * 方法，因为该方法会屏蔽本应退出睡眠模式的中断。
         */
        __asm volatile ( "cpsid i" ::: "memory" );
        __asm volatile ( "dsb" );
        __asm volatile ( "isb" );

        /*
         * 如果存在待处理的上下文切换，或有任务正在等待调度器
         * 解除挂起，则放弃进入低功耗模式。
         */
        if( eTaskConfirmSleepModeStatus() == eAbortSleep )
        {
            /*
             * 重新使能中断——参见上文 cpsid 指令周围的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
        else
        {
            /*
             * 暂时停止 SysTick。停止期间的时间会尽可能计入补偿，但使用
             * tickless 模式不可避免地会使内核维护的时间相对于日历时间
             * 产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /*
             * 使用 SysTick 当前值寄存器确定距离下一个时钟节拍中断
             * 剩余的 SysTick 递减次数。若当前值寄存器为零，则实际
             * 剩余 ulTimerCountsForOneTick 次递减而非零，因为 SysTick
             * 在从 1 递减到 0 时请求中断。
             */
            ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;

            if( ulSysTickDecrementsLeft == 0 )
            {
                ulSysTickDecrementsLeft = ulTimerCountsForOneTick;
            }

            /*
             * 计算等待 xExpectedIdleTime 个时钟节拍周期所需的重装载值。
             * 使用 -1 是因为本代码通常在一个节拍周期进行到一半时执行。
             * 但若此时 SysTick 中断处于挂起状态，则清除该中断，抑制第一个
             * 节拍，并修正重装载值以反映第二个节拍周期已经开始。
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
             * 清除 SysTick 计数标志并将计数值重新置零。
             */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

            /* 重新启动 SysTick。 */
            portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

            /*
             * 睡眠直到有事件发生。configPRE_SLEEP_PROCESSING() 可将
             * 其参数置为 0，表示其实现中包含自己的等待中断或等待事件
             * 指令，因此不应再次执行 wfi。不过，原始的预期空闲时间
             * 变量必须保持不变，因此这里复制一份。
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
             * 重新使能中断，使将 MCU 从睡眠模式唤醒的中断能够立即执行。
             * 参见上文 cpsid 指令周围的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 再次禁用中断，因为时钟即将停止，而时钟停止期间执行的中断
             * 会增大 RTOS 维护时间与日历时间之间的偏差。
             */
            __asm volatile ( "cpsid i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 禁用 SysTick 时钟，但不读取 portNVIC_SYSTICK_CTRL_REG 寄存器，
             * 以确保 portNVIC_SYSTICK_COUNT_FLAG_BIT 在置位时不被清除。
             * 同样，SysTick 停止期间的时间会尽可能计入补偿，但使用 tickless
             * 模式不可避免地会使内核维护的时间相对于日历时间产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /* 判断 SysTick 是否已经计数到零。 */
            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                uint32_t ulCalculatedLoadValue;

                /*
                 * 时钟节拍中断结束了睡眠（或当前处于挂起状态），
                 * 并且新的节拍周期已经开始。用新节拍周期的剩余时间
                 * 重新设置 portNVIC_SYSTICK_LOAD_REG。
                 */
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

                /*
                 * 不允许使用过小的值，也不允许使用因睡眠后钩子执行过久
                 * 或 SysTick 当前值寄存器为零而导致下溢的值。
                 */
                if( ( ulCalculatedLoadValue <= ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
                {
                    ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
                }

                portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

                /*
                 * 由于待处理的节拍将在本函数退出后立即处理，时钟节拍
                 * 维护的 tick 值向前推进的量比等待时间少一。
                 */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /* 睡眠是被时钟节拍中断以外的事件结束的。 */

                /*
                 * 使用 SysTick 当前值寄存器确定距离预期空闲时间结束
                 * 剩余的 SysTick 递减次数。
                 */
                ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;
                #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG != portNVIC_SYSTICK_CLK_BIT )
                {
                    /*
                     * 若 SysTick 未使用内核时钟，当前值寄存器此时可能仍为零。
                     * 这种情况下，SysTick 尚未从重装载寄存器加载，预期空闲时间
                     * 内实际剩余 ulReloadValue 次递减，而不是零。
                     */
                    if( ulSysTickDecrementsLeft == 0 )
                    {
                        ulSysTickDecrementsLeft = ulReloadValue;
                    }
                }
                #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG（SysTick 时钟配置） */

                /*
                 * 计算睡眠持续时间并舍入为完整的时钟节拍周期
                 * （而不是按部分节拍计算的 ulReload 值）。
                 */
                ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - ulSysTickDecrementsLeft;

                /*
                 * 处理器等待期间经过了多少个完整时钟节拍周期？
                 */
                ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

                /*
                 * 重装载值设置为单个节拍周期剩余的比例。
                 */
                portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
            }

            /*
             * 重新启动 SysTick，使其再次从 portNVIC_SYSTICK_LOAD_REG 运行，
             * 然后将 portNVIC_SYSTICK_LOAD_REG 恢复为标准值。若 SysTick
             * 未使用内核时钟，则临时将其配置为使用内核时钟。此配置会强制
             * SysTick 立即从 portNVIC_SYSTICK_LOAD_REG 加载，而不是等到
             * 另一个时钟的下一个周期。这样 portNVIC_SYSTICK_LOAD_REG 就能
             * 立即接收标准值。
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
                 * 如上所述，临时使用内核时钟的目的已经达到。
                 * 恢复使用另一个时钟。
                 */
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT;

                if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
                {
                    /*
                     * 部分节拍周期已经结束。确保 SysTick 只对其计数一次。
                     */
                    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
                }

                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            }
            #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG（SysTick 时钟配置） */

            /* 推进 tick 计数，以补偿已经流逝的时钟节拍周期。 */
            vTaskStepTick( ulCompleteTickPeriods );

            /* 在中断使能状态下退出。 */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
    }

#endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */
/*-----------------------------------------------------------*/

/*
 * 初始化 SysTick 定时器，以所需频率产生时钟节拍中断。
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
    #endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */

    /* 停止并清除 SysTick。 */
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* 配置 SysTick 以请求的速率产生中断。 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}
/*-----------------------------------------------------------*/

/* 这是一个裸函数。 */
static void vPortEnableVFP( void )
{
    __asm volatile
    (
        "   ldr.w r0, =0xE000ED88       \n" /* FPU 使能位位于 CPACR 中。 */
        "   ldr r1, [r0]                \n"
        "                               \n"
        "   orr r1, r1, #( 0xf << 20 )  \n" /* 使能 CP10 和 CP11 协处理器，然后写回。 */
        "   str r1, [r0]                \n"
        "   bx r14                      \n"
        "   .ltorg                      \n"
    );
}
/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

    void vPortValidateInterruptPriority( void )
    {
        uint32_t ulCurrentInterrupt;
        uint8_t ucCurrentPriority;

        /* 获取当前正在执行的中断编号。 */
        __asm volatile ( "mrs %0, ipsr" : "=r" ( ulCurrentInterrupt )::"memory" );

        /* 该中断编号是否为用户定义中断？ */
        if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
        {
            /* 查找该中断的优先级。 */
            ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

            /*
             * 若某个中断的 ISR 所分配的优先级高于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，却调用了 ISR 安全的
             * FreeRTOS API 函数，则下面的断言将失败。ISR 安全的 FreeRTOS
             * API 函数*只能*从优先级等于或低于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
             *
             * 数值较小的中断优先级编号表示逻辑上较高的中断优先级，
             * 因此中断的优先级必须设置为等于或数值*大于*
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
             *
             * 使用 FreeRTOS API 的中断不得保持默认优先级 0，因为那是
             * 可能的最高优先级，必然高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，
             * 因此也必然无效。
             *
             * FreeRTOS 同时提供独立的任务级和 ISR API 函数，以确保
             * 中断入口尽可能快速简单。
             *
             * 以下链接提供详细信息：
             * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
             * https://www.freertos.org/Why-FreeRTOS/FAQs
             */
            configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
        }

        /*
         * 优先级分组：中断控制器（NVIC）允许将定义每个中断优先级的位
         * 拆分为定义抢占优先级的位和定义子优先级的位。为简单起见，
         * 所有位都必须定义为抢占优先级位。若不符合该要求
         * （即某些位表示子优先级），下面的断言将失败。
         *
         * 若应用程序仅使用 CMSIS 库配置中断，则可在启动调度器前
         * 调用 NVIC_SetPriorityGrouping( 0 )，在所有 Cortex-M 器件上
         * 获得正确设置。但请注意，某些厂商专用外设库假定优先级分组
         * 设置为非零值，此时使用零值将导致不可预测的行为。
         */
        configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
    }

#endif /* configASSERT_DEFINED（已定义 configASSERT） */
