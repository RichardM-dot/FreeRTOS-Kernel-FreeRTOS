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
 * SH2A 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

/* 标准 C 头文件包含。 */
#include "limits.h"

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 库头文件包含。 */
#include "string.h"

/* 硬件相关定义。 */
#if ( configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H == 1 )

    #include "platform.h"

#else /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

    #include "iodefine.h"

#endif /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */
/*-----------------------------------------------------------*/

/*
 * 任务应以中断使能且处于超级用户模式的状态启动，因此 PSW 中 U 和 I 置位，
 * PM 和 IPL 清零。
 */
#define portINITIAL_PSW    ( ( StackType_t ) 0x00030000 )

/*
 * 外设时钟在提供给 CMT 之前先被此值分频。
 */
#if ( configUSE_TICKLESS_IDLE == 0 )
    /* 若未使用 tickless 空闲模式，则分频系数可以固定。 */
    #define portCLOCK_DIVISOR    8UL
#elif ( configPERIPHERAL_CLOCK_HZ >= 12000000 )
    #define portCLOCK_DIVISOR    512UL
#elif ( configPERIPHERAL_CLOCK_HZ >= 6000000 )
    #define portCLOCK_DIVISOR    128UL
#elif ( configPERIPHERAL_CLOCK_HZ >= 1000000 )
    #define portCLOCK_DIVISOR    32UL
#else
    #define portCLOCK_DIVISOR    8UL
#endif

/*
 * 这些宏允许在 xTaskIncrementTick() 调用周围添加临界区，该函数仅在中断中以内核
 * 优先级（即已知优先级）调用。因此相比调用全局 SET/CLEAR_INTERRUPT_MASK 宏，
 * 这些本地宏是轻微优化，因为全局宏需要先读取旧 IPL 并存入局部变量。
 */
#define portDISABLE_INTERRUPTS_FROM_KERNEL_ISR()    __asm volatile ( "MVTIPL    %0" ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) )
#define portENABLE_INTERRUPTS_FROM_KERNEL_ISR()     __asm volatile ( "MVTIPL    %0" ::"i" ( configKERNEL_INTERRUPT_PRIORITY ) )

/*
 * 分别用于加锁和解锁对某些系统寄存器访问的密钥。
 */
#define portUNLOCK_KEY    0xA50B
#define portLOCK_KEY      0xA500

/*-----------------------------------------------------------*/

/*
 * 启动第一个任务执行的函数 - 用汇编代码编写，因为需要直接访问寄存器。
 */
static void prvStartFirstTask( void ) __attribute__( ( naked ) );

/*
 * 软件中断处理程序。执行实际的上下文切换（保存和恢复寄存器）。用汇编代码编写，
 * 因为需要直接访问寄存器。
 */
#if ( configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H == 1 )

    R_BSP_PRAGMA_INTERRUPT( vPortSoftwareInterruptISR, VECT( ICU, SWINT ) )
    R_BSP_ATTRIB_INTERRUPT void vPortSoftwareInterruptISR( void ) __attribute__( ( naked ) );

#else /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

    void vPortSoftwareInterruptISR( void ) __attribute__( ( naked ) );

#endif /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

/*
 * 时钟节拍 ISR 处理程序。使用的外设由应用程序通过钩子/回调函数配置。
 */
#if ( configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H == 1 )

    R_BSP_PRAGMA_INTERRUPT( vPortTickISR, _VECT( configTICK_VECTOR ) )
    R_BSP_ATTRIB_INTERRUPT void vPortTickISR( void ); /* 不要添加 __attribute__( ( interrupt ) )。 */

#else /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

    void vPortTickISR( void ) __attribute__( ( interrupt ) );

#endif /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

/*
 * 使用 CMT 设置用于 RTOS 时钟节拍（tick）的周期性 ISR。
 * 应用程序编写者可以定义 configSETUP_TICK_INTERRUPT()（在 FreeRTOSConfig.h 中），
 * 以便用自己的时钟节拍中断配置替代 prvSetupTimerInterrupt()。
 */
static void prvSetupTimerInterrupt( void );
#ifndef configSETUP_TICK_INTERRUPT

/*
 * 用户未提供自己的时钟节拍中断配置，因此使用本文件中的定义（使用间隔定时器）。
 */
    #define configSETUP_TICK_INTERRUPT()    prvSetupTimerInterrupt()
#endif /* configSETUP_TICK_INTERRUPT（宏） */

/*
 * 在睡眠模式寄存器配置完成后调用，prvSleep() 执行睡眠前和睡眠后宏，并实际调用
 * wait 指令。
 */
#if configUSE_TICKLESS_IDLE == 1
    static void prvSleep( TickType_t xExpectedIdleTime );
#endif /* configUSE_TICKLESS_IDLE（宏） */

/*-----------------------------------------------------------*/

/* 用于上下文保存和恢复代码。 */
extern void * pxCurrentTCB;

/* 计算一个 tick 周期由多少个时钟增量组成。 */
static const uint32_t ulMatchValueForOneTick = ( ( configPERIPHERAL_CLOCK_HZ / portCLOCK_DIVISOR ) / configTICK_RATE_HZ );

#if configUSE_TICKLESS_IDLE == 1

/*
 * 保存可被抑制的最大 tick 数 - 基本上就是可以向未来产生中断的最远距离。
 * 初始化时设置。该值为比较匹配寄存器可保存的最大可能值除以 ulMatchValueForOneTick。
 */
    static const TickType_t xMaximumPossibleSuppressedTicks = USHRT_MAX / ( ( configPERIPHERAL_CLOCK_HZ / portCLOCK_DIVISOR ) / configTICK_RATE_HZ );

/*
 * 由时钟节拍中断设置的标志，使睡眠处理能够知道退出睡眠模式是由于 tick 中断
 * 还是其他原因产生的中断。
 */
    static volatile uint32_t ulTickFlag = pdFALSE;

/*
 * 每次重新编程时 CMT 计数器都会暂时停止。以下常量按计数器停止期间通常会丢失的
 * CMT 计数值对 CMT 计数器匹配值进行偏移，以补偿损失的时间。分频后的 CMT 时钟
 * 与 CPU 时钟之间的巨大差异意味着 ulStoppedTimerCompensation 很可能等于零 -
 * 并被优化掉。
 */
    static const uint32_t ulStoppedTimerCompensation = 100UL / ( configCPU_CLOCK_HZ / ( configPERIPHERAL_CLOCK_HZ / portCLOCK_DIVISOR ) );

#endif /* 若 configUSE_TICKLESS_IDLE == 1 */

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /* 偏移以落在 8 字节边界上。 */
    pxTopOfStack--;

    /* 未包含 R0，因为它是栈指针。 */
    *pxTopOfStack = 0x00;
    pxTopOfStack--;
    *pxTopOfStack = 0x00;
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_PSW;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode;

    /*
     * 调试时，将每个寄存器设为已知值会很有帮助。否则，为节省代码空间，可以只设置
     * 需要设置的寄存器。
     */
    #ifdef USE_FULL_REGISTER_INITIALISATION
    {
        pxTopOfStack--;
        *pxTopOfStack = 0x12345678; /* r15 寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = 0xaaaabbbb;
        pxTopOfStack--;
        *pxTopOfStack = 0xdddddddd;
        pxTopOfStack--;
        *pxTopOfStack = 0xcccccccc;
        pxTopOfStack--;
        *pxTopOfStack = 0xbbbbbbbb;
        pxTopOfStack--;
        *pxTopOfStack = 0xaaaaaaaa;
        pxTopOfStack--;
        *pxTopOfStack = 0x99999999;
        pxTopOfStack--;
        *pxTopOfStack = 0x88888888;
        pxTopOfStack--;
        *pxTopOfStack = 0x77777777;
        pxTopOfStack--;
        *pxTopOfStack = 0x66666666;
        pxTopOfStack--;
        *pxTopOfStack = 0x55555555;
        pxTopOfStack--;
        *pxTopOfStack = 0x44444444;
        pxTopOfStack--;
        *pxTopOfStack = 0x33333333;
        pxTopOfStack--;
        *pxTopOfStack = 0x22222222;
        pxTopOfStack--;
    }
    #else /* 若定义了 USE_FULL_REGISTER_INITIALISATION */
    {
        /*
         * 为任务首次开始执行时将从栈中弹出的寄存器留出空间。
         */
        pxTopOfStack -= 15;
    }
    #endif /* 若定义了 USE_FULL_REGISTER_INITIALISATION */

    *pxTopOfStack = ( StackType_t ) pvParameters; /* R1 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = 0x12345678;                   /* 累加器。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x87654321;                   /* 累加器。 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /* 使用 pxCurrentTCB，以免其被优化掉。 */
    if( pxCurrentTCB != NULL )
    {
        /*
         * 调用应用程序函数来设置将产生时钟节拍中断的定时器。这样应用程序可以决定使用
         * 哪个外设。若使用 tickless 模式，则不应覆盖本文件中定义的默认实现（使用 CMT0）。
         */
        configSETUP_TICK_INTERRUPT();

        /* 使能软件中断。 */
        _IEN( _ICU_SWINT ) = 1;

        /* 确保软件中断已清除。 */
        _IR( _ICU_SWINT ) = 0;

        /* 确保软件中断被设置为内核优先级。 */
        _IPR( _ICU_SWINT ) = configKERNEL_INTERRUPT_PRIORITY;

        /* 启动第一个任务。 */
        prvStartFirstTask();
    }

    /*
     * 任务已在运行，执行不应到达此处！
     * 此处调用 prvSetupTimerInterrupt() 是为了防止在应用程序编写者提供了自己的时钟节拍
     * 中断配置例程（并定义了 configSETUP_TICK_INTERRUPT() 以便用自己的例程替代
     * prvSetupTimerInterrupt()）的情况下，编译器输出关于静态声明函数未被引用的警告。
     */
    prvSetupTimerInterrupt();

    /* 不应执行到此处。 */
    return pdFAIL;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在无需返回的移植层中未实现。
     * 人为强制触发断言。
     */
    configASSERT( pxCurrentTCB == NULL );
}
/*-----------------------------------------------------------*/

static void prvStartFirstTask( void )
{
    __asm volatile
    (

        /*
         * 启动调度器时无需将任何内容移到中断栈，因为该函数并非从中断中调用。
         * 只需确保当前栈为用户栈。
         */
        "SETPSW     U                       \n" \


        /*
         * 获取 pxCurrentTCB 当前所指向任务的相关栈位置。
         */
        "MOV.L      #_pxCurrentTCB, R15     \n" \
        "MOV.L      [R15], R15              \n" \
        "MOV.L      [R15], R0               \n" \


        /*
         * 从 pxCurrentTCB 所指向任务的栈中恢复寄存器。
         */
        "POP        R15                     \n" \

        /* 累加器低 32 位。 */
        "MVTACLO    R15                     \n" \
        "POP        R15                     \n" \

        /* 累加器高 32 位。 */
        "MVTACHI    R15                     \n" \

        /* R1 至 R15 - 未包含 R0，因为它是 SP。 */
        "POPM       R1-R15                  \n" \

        /* 这将弹出其余寄存器。 */
        "RTE                                \n" \
        "NOP                                \n" \
        "NOP                                \n"
    );
}
/*-----------------------------------------------------------*/

void vPortSoftwareInterruptISR( void )
{
    __asm volatile
    (
        /* 重新使能中断。 */
        "SETPSW     I                           \n" \


        /*
         * 将中断发生时自动压入中断栈的数据从中断栈移到用户栈。
         *
         * 保存 R15 后再覆盖它。
         */
        "PUSH.L     R15                         \n" \

        /* 读取用户栈指针。 */
        "MVFC       USP, R15                    \n" \

        /* 将地址下移到要移动的数据处。 */
        "SUB        #12, R15                    \n" \
        "MVTC       R15, USP                    \n" \

        /* 复制数据，依次为 R15、PC、PSW。 */
        "MOV.L      [ R0 ], [ R15 ]             \n" \
        "MOV.L      4[ R0 ], 4[ R15 ]           \n" \
        "MOV.L      8[ R0 ], 8[ R15 ]           \n" \

        /* 将中断栈指针移到其新的正确位置。 */
        "ADD        #12, R0                     \n" \

        /* 其余所有寄存器直接保存到用户栈。 */
        "SETPSW     U                           \n" \

        /* 保存其余的通用寄存器（R15 已保存）。 */
        "PUSHM      R1-R14                      \n" \

        /* 保存累加器。 */
        "MVFACHI    R15                         \n" \
        "PUSH.L     R15                         \n" \

        /* 中间字。 */
        "MVFACMI    R15                         \n" \

        /* 由于要恢复到低序字，因此左移。 */
        "SHLL       #16, R15                    \n" \
        "PUSH.L     R15                         \n" \

        /* 将栈指针保存到 TCB。 */
        "MOV.L      #_pxCurrentTCB, R15         \n" \
        "MOV.L      [ R15 ], R15                \n" \
        "MOV.L      R0, [ R15 ]                 \n" \


        /*
         * 在访问内核结构期间，确保中断屏蔽设置为系统调用优先级。
         */
        "MVTIPL     %0                          \n" \

        /* 选择下一个要运行的任务。 */
        "BSR.A      _vTaskSwitchContext         \n" \

        /* 不再需要访问数据结构，重置中断屏蔽。 */
        "MVTIPL     %1                          \n" \


        /*
         * 从 TCB 加载现已选为运行态任务的栈指针。
         */
        "MOV.L      #_pxCurrentTCB,R15          \n" \
        "MOV.L      [ R15 ], R15                \n" \
        "MOV.L      [ R15 ], R0                 \n" \


        /*
         * 恢复新任务的上下文。PSW（程序状态字）和 PC 将由 RTE 指令弹出。
         */
        "POP        R15                         \n" \
        "MVTACLO    R15                         \n" \
        "POP        R15                         \n" \
        "MVTACHI    R15                         \n" \
        "POPM       R1-R15                      \n" \
        "RTE                                    \n" \
        "NOP                                    \n" \
        "NOP                                      "
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ), "i" ( configKERNEL_INTERRUPT_PRIORITY )
    );
}
/*-----------------------------------------------------------*/

void vPortTickISR( void )
{
    /* 重新使能中断。 */
    __asm volatile ( "SETPSW I" );

    /*
     * 递增 tick，并执行新 tick 值所需的任何处理。首先确保 IPL 处于最大系统调用值。
     */
    portDISABLE_INTERRUPTS_FROM_KERNEL_ISR();
    {
        if( xTaskIncrementTick() != pdFALSE )
        {
            taskYIELD();
        }
    }
    portENABLE_INTERRUPTS_FROM_KERNEL_ISR();

    #if configUSE_TICKLESS_IDLE == 1
    {
        /* CPU 因时钟节拍而唤醒。 */
        ulTickFlag = pdTRUE;

        /*
         * 若这是退出 tickless 模式后的第一个 tick，则需要重新设置 CMT 比较匹配值。
         */
        CMT0.CMCOR = ( uint16_t ) ulMatchValueForOneTick;
    }
    #endif
}
/*-----------------------------------------------------------*/

uint32_t ulPortGetIPL( void )
{
    __asm volatile
    (
        "MVFC   PSW, R1         \n" \
        "SHLR   #24, R1         \n" \
        "RTS                      "
    );

    /* 这永远不会执行，但可避免编译器报怨。 */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortSetIPL( uint32_t ulNewIPL )
{
    __asm volatile
    (
        "PUSH   R5              \n" \
        "MVFC   PSW, R5         \n" \
        "SHLL   #24, R1         \n" \
        "AND    #-0F000001H, R5 \n" \
        "OR     R1, R5          \n" \
        "MVTC   R5, PSW         \n" \
        "POP    R5              \n" \
        "RTS                      "
    );
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
    /* 解锁。 */
    SYSTEM.PRCR.WORD = portUNLOCK_KEY;

    /* 使能 CMT0。 */
    MSTP( CMT0 ) = 0;

    /* 再次加锁。 */
    SYSTEM.PRCR.WORD = portLOCK_KEY;

    /* 比较匹配时产生中断。 */
    CMT0.CMCR.BIT.CMIE = 1;

    /* 设置比较匹配值。 */
    CMT0.CMCOR = ( uint16_t ) ulMatchValueForOneTick;

    /* 对 PCLK 分频。 */
    #if portCLOCK_DIVISOR == 512
    {
        CMT0.CMCR.BIT.CKS = 3;
    }
    #elif portCLOCK_DIVISOR == 128
    {
        CMT0.CMCR.BIT.CKS = 2;
    }
    #elif portCLOCK_DIVISOR == 32
    {
        CMT0.CMCR.BIT.CKS = 1;
    }
    #elif portCLOCK_DIVISOR == 8
    {
        CMT0.CMCR.BIT.CKS = 0;
    }
    #else /* 若 portCLOCK_DIVISOR == 512 */
    {
        #error Invalid portCLOCK_DIVISOR setting
    }
    #endif /* 若 portCLOCK_DIVISOR == 512 */

    /* 使能中断…… */
    _IEN( _CMT0_CMI0 ) = 1;

    /* ……并将其优先级设置为应用程序定义的内核优先级。 */
    _IPR( _CMT0_CMI0 ) = configKERNEL_INTERRUPT_PRIORITY;

    /* 启动定时器。 */
    CMT.CMSTR0.BIT.STR0 = 1;
}
/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1

    static void prvSleep( TickType_t xExpectedIdleTime )
    {
        /* 允许应用程序定义一些睡眠前处理。 */
        configPRE_SLEEP_PROCESSING( xExpectedIdleTime );

        /*
         * configPRE_SLEEP_PROCESSING() 将 xExpectedIdleTime 置为 0，表示应用程序定义
         * 的代码已执行 WAIT 指令。
         */
        if( xExpectedIdleTime > 0 )
        {
            __asm volatile ( "WAIT" );
        }

        /* 允许应用程序定义一些睡眠后处理。 */
        configPOST_SLEEP_PROCESSING( xExpectedIdleTime );
    }

#endif /* configUSE_TICKLESS_IDLE（宏） */
/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1

    void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
    {
        uint32_t ulMatchValue, ulCompleteTickPeriods, ulCurrentCount;
        eSleepModeStatus eSleepAction;

        /* 此函数在调度器挂起的情况下被调用。 */

        /* 确保 CMT 重载值不会使计数器溢出。 */
        if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
        {
            xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
        }

        /*
         * 计算等待 xExpectedIdleTime 个 tick 周期所需的重载值。
         */
        ulMatchValue = ulMatchValueForOneTick * xExpectedIdleTime;

        if( ulMatchValue > ulStoppedTimerCompensation )
        {
            /*
             * 补偿 CMT 将短暂停止的事实。
             */
            ulMatchValue -= ulStoppedTimerCompensation;
        }

        /*
         * 暂时停止 CMT。CMT 停止的时间已尽可能计入，但使用 tickless 模式不可避免地
         * 会导致内核维护的时间相对于日历时间出现微小漂移。
         */
        CMT.CMSTR0.BIT.STR0 = 0;

        while( CMT.CMSTR0.BIT.STR0 == 1 )
        {
            /* 此处无需执行任何操作。 */
        }

        /*
         * 使用全局中断位构成临界区，因为 i 位会被 WAIT 指令自动复位。
         */
        __asm volatile ( "CLRPSW i" );

        /*
         * 睡眠前将 tick 标志置为 false。若退出睡眠模式时为 true，则说明很可能是因为在
         * 整个 xExpectedIdleTime 期间抑制了 tick 而退出睡眠模式。
         */
        ulTickFlag = pdFALSE;

        /*
         * 若有上下文切换挂起，则放弃进入低功耗模式，因为该上下文切换可能是由需要处理
         * 的外部中断挂起的。
         */
        eSleepAction = eTaskConfirmSleepModeStatus();

        if( eSleepAction == eAbortSleep )
        {
            /* 重启 tick。 */
            CMT.CMSTR0.BIT.STR0 = 1;
            __asm volatile ( "SETPSW i" );
        }
        else if( eSleepAction == eNoTasksWaitingTimeout )
        {
            /* 关闭保护。 */
            SYSTEM.PRCR.WORD = portUNLOCK_KEY;

            /* 准备进入所有时钟停止的软件待机模式。 */
            SYSTEM.SBYCR.BIT.SSBY = 1;

            /* 开启保护。 */
            SYSTEM.PRCR.WORD = portLOCK_KEY;

            /*
             * 睡眠直到有事件发生。调用 prvSleep() 会自动复位 PSW 中的 i 位。
             */
            prvSleep( xExpectedIdleTime );

            /* 重启 CMT。 */
            CMT.CMSTR0.BIT.STR0 = 1;
        }
        else
        {
            /* 关闭保护。 */
            SYSTEM.PRCR.WORD = portUNLOCK_KEY;

            /* 准备进入深度睡眠模式。 */
            SYSTEM.MSTPCRC.BIT.DSLPE = 1;
            SYSTEM.MSTPCRA.BIT.MSTPA28 = 1;
            SYSTEM.SBYCR.BIT.SSBY = 0;

            /* 开启保护。 */
            SYSTEM.PRCR.WORD = portLOCK_KEY;

            /*
             * 调整匹配值，以考虑当前时间片已部分完成。
             */
            ulMatchValue -= ( uint32_t ) CMT0.CMCNT;
            CMT0.CMCOR = ( uint16_t ) ulMatchValue;

            /* 重启 CMT，向上计数到新的匹配值。 */
            CMT0.CMCNT = 0;
            CMT.CMSTR0.BIT.STR0 = 1;

            /*
             * 睡眠直到有事件发生。调用 prvSleep() 会自动复位 PSW 中的 i 位。
             */
            prvSleep( xExpectedIdleTime );

            /*
             * 停止 CMT。同样，SysTick 停止的时间已尽可能计入，但使用 tickless 模式不可避免地
             * 会导致内核维护的时间相对于日历时间出现微小漂移。
             */
            CMT.CMSTR0.BIT.STR0 = 0;

            while( CMT.CMSTR0.BIT.STR0 == 1 )
            {
                /* 此处无需执行任何操作。 */
            }

            ulCurrentCount = ( uint32_t ) CMT0.CMCNT;

            if( ulTickFlag != pdFALSE )
            {
                /*
                 * 时钟节拍中断已经执行，但由于此函数在调度器挂起的情况下被调用，实际的 tick
                 * 处理要等到此函数退出后才进行。用本 tick 周期剩余的时间重置匹配值。
                 */
                ulMatchValue = ulMatchValueForOneTick - ulCurrentCount;
                CMT0.CMCOR = ( uint16_t ) ulMatchValue;

                /*
                 * 时钟节拍中断处理程序已在内核中挂起 tick 处理。由于挂起的 tick 将在此函数退出后
                 * 立即处理，tick 维护的值会向前步进比睡眠时间少一的值。tick 的实际步进出现在
                 * 此函数的后面。
                 */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /*
                 * 睡眠是被时钟节拍中断以外的其他事件结束的。
                 * 处理器睡眠期间经过了多少个完整的 tick 周期？
                 */
                ulCompleteTickPeriods = ulCurrentCount / ulMatchValueForOneTick;

                /*
                 * 匹配值设置为单个 tick 周期剩余的比例。
                 */
                ulMatchValue = ulCurrentCount - ( ulCompleteTickPeriods * ulMatchValueForOneTick );
                CMT0.CMCOR = ( uint16_t ) ulMatchValue;
            }

            /*
             * 重启 CMT，使其运行到匹配值。下次 CMT 中断执行时，匹配值将被设置为恰好产生
             * 一个 tick 周期所需的值。
             */
            CMT0.CMCNT = 0;
            CMT.CMSTR0.BIT.STR0 = 1;

            /*
             * 将 tick 向前推进 CPU 处于低功耗状态的 tick 周期数。
             */
            vTaskStepTick( ulCompleteTickPeriods );
        }
    }

#endif /* configUSE_TICKLESS_IDLE（宏） */
