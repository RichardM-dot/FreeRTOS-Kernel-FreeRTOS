/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2025 Arm Limited and/or its affiliates
 * <open-source-office@arm.com>
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
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 把
 * 所有 API 函数重定义为使用 MPU 包装器的版本。仅当从应用程序文件
 * 包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* 调度器相关头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* MPU 相关头文件。 */
#include "mpu_wrappers.h"
#include "mpu_syscall_numbers.h"

/* 移植层汇编相关头文件。 */
#include "portasm.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*-----------------------------------------------------------*/

/**
 * @brief 所有中断服务程序（ISR）的原型。
 */
typedef void ( * portISR_t )( void );

/*-----------------------------------------------------------*/

/**
 * @brief 操作 NVIC 所需的常量。
 */
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SHPR2_REG                    ( *( ( volatile uint32_t * ) 0xe000ed1c ) )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT       ( 1UL << 25UL )
#define portNVIC_PEND_SYSTICK_SET_BIT         ( 1UL << 26UL )
#define portMIN_INTERRUPT_PRIORITY            ( 255UL )
#define portNVIC_PENDSV_PRI                   ( portMIN_INTERRUPT_PRIORITY << 16UL )
#define portNVIC_SYSTICK_PRI                  ( portMIN_INTERRUPT_PRIORITY << 24UL )

/*-----------------------------------------------------------*/

/**
 * @brief 操作 SCB 所需的常量。
 */
#define portSCB_VTOR_REG                      ( *( ( portISR_t ** ) 0xe000ed08 ) )
#define portSCB_SYS_HANDLER_CTRL_STATE_REG    ( *( ( volatile uint32_t * ) 0xe000ed24 ) )
#define portSCB_MEM_FAULT_ENABLE_BIT          ( 1UL << 16UL )

/*-----------------------------------------------------------*/

/**
 * @brief 用于检查 FreeRTOS 中断处理程序安装情况的常量。
 */
#define portVECTOR_INDEX_SVC       ( 11 )
#define portVECTOR_INDEX_PENDSV    ( 14 )

/*-----------------------------------------------------------*/

/**
 * @brief 系统调用进入和退出期间使用的常量。
 */
#define portPSR_STACK_PADDING_MASK              ( 1UL << 9UL )
#define portEXC_RETURN_STACK_FRAME_TYPE_MASK    ( 1UL << 4UL )

/*-----------------------------------------------------------*/

/**
 * @brief 在 SVC 处理程序内部时，栈中参数偏移量。
 */
#define portOFFSET_TO_LR     ( 5 )
#define portOFFSET_TO_PC     ( 6 )
#define portOFFSET_TO_PSR    ( 7 )

/*-----------------------------------------------------------*/

/**
 * @brief 操作 MPU 所需的常量。
 */
#define portMPU_TYPE_REG                            ( *( ( volatile uint32_t * ) 0xe000ed90 ) )
#define portMPU_CTRL_REG                            ( *( ( volatile uint32_t * ) 0xe000ed94 ) )

#define portMPU_RBAR_REG                            ( *( ( volatile uint32_t * ) 0xe000ed9c ) )
#define portMPU_RASR_REG                            ( *( ( volatile uint32_t * ) 0xe000eda0 ) )

/* MPU 区域属性与大小寄存器（RASR）位掩码。 */
#define portMPU_RASR_AP_BITMASK                     ( 0x7UL << 24UL )
#define portMPU_RASR_S_C_B_BITMASK                  ( 0x7UL )
#define portMPU_RASR_S_C_B_LOCATION                 ( 16UL )
#define portMPU_RASR_SIZE_BITMASK                   ( 0x1FUL << 1UL )
#define portMPU_RASR_REGION_ENABLE_BITMASK          ( 0x1UL )

/* MPU 区域基地址寄存器（RBAR）位掩码。 */
#define portMPU_RBAR_ADDRESS_BITMASK                ( 0xFFFFFF00UL )
#define portMPU_RBAR_REGION_NUMBER_VALID_BITMASK    ( 0x1UL << 4UL )
#define portMPU_RBAR_REGION_NUMBER_BITMASK          ( 0x0000000FUL )

/* MPU 控制寄存器（MPU_CTRL）位掩码。 */
#define portMPU_CTRL_ENABLE_BITMASK                 ( 0x1UL )
#define portMPU_CTRL_PRIV_BACKGROUND_ENABLE_BITMASK ( 0x1UL << 2UL ) /* PRIVDEFENA 位。 */

/* portMPU_TYPE 寄存器的期望值。 */
#define portEXPECTED_MPU_TYPE_VALUE                 ( 0x8UL << 8UL ) /* 8 个统一的数据区域（DREGION）。 */

/*
 * 从 RBAR（区域基地址寄存器）值中编码的
 * 提取 MPU 区域的起始地址。
 */
#define portEXTRACT_FIRST_ADDRESS_FROM_RBAR( rbar ) \
    ( ( rbar ) & portMPU_RBAR_ADDRESS_BITMASK )

/*
 * 从 RASR（区域属性与大小寄存器）值中编码的
 * 提取 MPU 区域的大小。
 */
#define portEXTRACT_REGION_SIZE_FROM_RASR( rasr ) \
    ( 1 << ( ( ( ( rasr ) & portMPU_RASR_SIZE_BITMASK ) >> 1 )+ 1 ) )

/* 地址 addr 是否位于 [start, end] 地址范围内？ */
#define portIS_ADDRESS_WITHIN_RANGE( addr, start, end ) \
    ( ( ( addr ) >= ( start ) ) && ( ( addr ) <= ( end ) ) )

/* 可用权限是否满足访问请求？ */
#define portIS_AUTHORIZED( accessRequest, permissions ) \
    ( ( ( permissions ) & ( accessRequest ) ) == accessRequest )

/* uint32_t 类型可容纳的最大值。 */
#define portUINT32_MAX    ( ~( ( uint32_t ) 0 ) )

/* 检查 a 与 b 相加是否会导致溢出。 */
#define portADD_UINT32_WILL_OVERFLOW( a, b )    ( ( a ) > ( portUINT32_MAX - ( b ) ) )

/*-----------------------------------------------------------*/

/**
 * @brief 最大的 24 位数。
 *
 * 由于 SysTick 是 24 位计数器，因此需要该常量。
 */
#define portMAX_24_BIT_NUMBER       ( 0xffffffUL )

/**
 * @brief 用于估算在 tickless 空闲计算期间 SysTick 计数器停止时
 * 本应发生的 SysTick 计数次数的修正系数。
 */
#define portMISSED_COUNTS_FACTOR    ( 94UL )

/*-----------------------------------------------------------*/

/**
 * @brief 建立初始栈所需的常量。
 */
#define portINITIAL_XPSR    ( 0x01000000 )

/**
 * @brief 初始 EXC_RETURN 值。
 *
 *     FF         FF         FF         FD
 * 1111 1111  1111 1111  1111 1111  1111 1101
 *
 * Bit[3] - 1 --> 返回 Thread 模式。
 * Bit[2] - 1 --> 从进程栈恢复寄存器。
 * Bit[1] - 0 --> 保留位，取 0。
 * Bit[0] - 0 --> 保留位，取 1。
 */
#define portINITIAL_EXC_RETURN    ( 0xfffffffdUL )

/**
 * @brief CONTROL 寄存器的特权位掩码。
 *
 * CONTROL 寄存器中的 Bit[0] 指示特权状态：
 *  Bit[0] = 0 ==> 任务处于特权状态。
 *  Bit[0] = 1 ==> 任务处于非特权状态。
 */
#define portCONTROL_PRIVILEGED_MASK         ( 1UL << 0UL )

/**
 * @brief CONTROL 寄存器的初始值。
 */
#define portINITIAL_CONTROL_UNPRIVILEGED    ( 0x3 )
#define portINITIAL_CONTROL_PRIVILEGED      ( 0x2 )

/**
 * @brief 允许用户覆盖默认的 SysTick 时钟频率。若由用户定义，
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

/**
 * @brief 允许用户覆盖初始 LR 中预加载的 prvTaskExitError() 地址，
 * 以防该地址干扰调试器中的栈回溯。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/**
 * @brief 若定义 portPRELOAD_REGISTERS，则创建任务时会为寄存器赋予初始值。
 * 这有助于调试，但会增加代码体积。
 */
#define portPRELOAD_REGISTERS    1

/*-----------------------------------------------------------*/

/**
 * @brief 用于捕获试图从其实现函数返回的任务。
 */
static void prvTaskExitError( void );

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 初始化内存保护单元（MPU）。
     */
    static void prvSetupMPU( void ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU（使能 MPU） */

/**
 * @brief 初始化定时器以产生时钟节拍中断。
 *
 * 本文件中的实现为弱定义，以便应用程序编写者
 * 更换用于产生时钟节拍中断的定时器。
 */
void vPortSetupTimerInterrupt( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 检查当前执行上下文是否为中断上下文。
 *
 * @return 若当前执行上下文为中断上下文则返回 pdTRUE，否则返回 pdFALSE。
 */
BaseType_t xPortIsInsideInterrupt( void );

/**
 * @brief 让出 CPU。
 */
void vPortYield( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 进入临界区。
 */
void vPortEnterCritical( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 退出临界区。
 */
void vPortExitCritical( void ) PRIVILEGED_FUNCTION;

/**
 * @brief SysTick 处理程序。
 */
void SysTick_Handler( void ) PRIVILEGED_FUNCTION;

/**
 * @brief SVC 处理程序的 C 语言部分。
 */
portDONT_DISCARD void vPortSVCHandler_C( uint32_t * pulCallerStackAddress ) PRIVILEGED_FUNCTION;

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 建立系统调用栈，以便从 SVC 返回后使用系统调用栈。
     *
     * @param pulTaskStack 触发 SVC 时的当前 SP。
     * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
     * @param ucSystemCallNumber 系统调用的调用编号。
     */
    void vSystemCallEnter( uint32_t * pulTaskStack,
                           uint32_t ulLR,
                           uint8_t ucSystemCallNumber ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 触发 SVC 以退出系统调用。
     */
    void vRequestSystemCallExit( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 建立任务栈，以便从 SVC 返回后重新使用任务栈。
     *
     * @param pulSystemCallStack 触发 SVC 时的当前 SP。
     * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
     */
    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 检查调用任务是否具有特权。
     *
     * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
     */
    BaseType_t xPortIsTaskPrivileged( void ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1（已启用 MPU） */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 调度器启动时，该变量被置为 pdTRUE。
     */
    PRIVILEGED_DATA static BaseType_t xSchedulerRunning = pdFALSE;

#endif

/**
 * @brief 每个任务通过临界区嵌套计数变量维护自身的中断状态。
 */
PRIVILEGED_DATA static volatile uint32_t ulCriticalNesting = 0xaaaaaaaaUL;

#if ( configUSE_TICKLESS_IDLE == 1 )

    /**
     * @brief 构成一个时钟节拍周期的 SysTick 递增次数。
     */
    PRIVILEGED_DATA static uint32_t ulTimerCountsForOneTick = 0;

    /**
     * @brief 可抑制的最大时钟节拍周期数受 SysTick 定时器 24 位分辨率的限制。
     */
    PRIVILEGED_DATA static uint32_t xMaximumPossibleSuppressedTicks = 0;

    /**
     * @brief 补偿 SysTick 停止期间流逝的 CPU 周期（仅用于低功耗功能）。
     */
    PRIVILEGED_DATA static uint32_t ulStoppedTimerCompensation = 0;

#endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */

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

__attribute__( ( weak ) ) void vPortSetupTimerInterrupt( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /* 计算配置时钟节拍中断所需的常量。 */
    #if ( configUSE_TICKLESS_IDLE == 1 )
    {
        ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
        xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
        ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
    }
    #endif /* configUSE_TICKLESS_IDLE（低功耗 tickless 空闲） */

    /*
     * 停止并复位 SysTick。
     *
     * 7.0.0 之前的 QEMU 版本存在一个缺陷：若未先选择有效的时钟源
     * 就使能 SysTick，将导致错误。如果在同一时刻把时钟源从零时钟周期
     * 切换为非零时钟周期并使能 SysTick，就会触发该缺陷。
     * 因此这里先配置 CLKSOURCE 位，再设置 ENABLE 位。
     * 该变通方法可避免 7.0.0 之前 QEMU 版本中的缺陷。
     */
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* 配置 SysTick 以请求的速率产生中断。 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
}

/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0UL;

    /*
     * 实现任务的函数不得退出或试图返回其调用者，因为无处可返回。
     * 若任务需要退出，应改为调用 vTaskDelete( NULL )。若定义了
     * configASSERT()，则人为触发 assert()，然后停在此处，
     * 以便应用程序编写者捕获该错误。
     */
    configASSERT( ulCriticalNesting == ~0UL );
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

#if ( configENABLE_MPU == 1 )

    static uint32_t prvGetMPURegionSizeSetting( uint32_t ulActualSizeInBytes )
    {
        uint32_t ulRegionSize, ulReturnValue = 7UL;

        /*
         * 256 是最小的区域大小，31 是 ulReturnValue 的最大有效值。
         */
        for( ulRegionSize = 256UL; ulReturnValue < 31UL; ( ulRegionSize <<= 1UL ) )
        {
            if( ulActualSizeInBytes <= ulRegionSize )
            {
                break;
            }
            else
            {
                ulReturnValue++;
            }
        }

        /*
         * 返回前将编码左移一位，使其可以直接写入属性寄存器的正确位位置。
         */
        return( ulReturnValue << 1UL );
    }

#endif /* configENABLE_MPU（使能 MPU） */

/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    static void prvSetupMPU( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
            extern uint32_t * __FLASH_segment_start__;
            extern uint32_t * __FLASH_segment_end__;
            extern uint32_t * __privileged_sram_start__;
            extern uint32_t * __privileged_sram_end__;

        #else /* __ARMCC_VERSION 定义条件 */

            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
            extern uint32_t __FLASH_segment_start__[];
            extern uint32_t __FLASH_segment_end__[];
            extern uint32_t __privileged_sram_start__[];
            extern uint32_t __privileged_sram_end__[];

        #endif /* __ARMCC_VERSION 定义条件 */

        /* 确保 MPU 存在。 */
        configASSERT( portMPU_TYPE_REG == portEXPECTED_MPU_TYPE_VALUE );

        /* 检查 MPU 是否存在。 */
        if( portMPU_TYPE_REG == portEXPECTED_MPU_TYPE_VALUE )
        {
            /*
             * 将特权 Flash 设置为只读，使特权任务可以读取但无法修改。
             */
            portMPU_RBAR_REG = ( ( ( uint32_t ) __privileged_functions_start__ ) | /* 基地址。 */
                                 ( portMPU_RBAR_REGION_NUMBER_VALID_BITMASK ) |
                                 ( portPRIVILEGED_FLASH_REGION ) );

            portMPU_RASR_REG = ( ( portMPU_REGION_PRIV_RO_UNPRIV_NA ) |
                                 ( ( configS_C_B_FLASH & portMPU_RASR_S_C_B_BITMASK ) << portMPU_RASR_S_C_B_LOCATION ) |
                                 ( prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_functions_end__ - ( uint32_t ) __privileged_functions_start__ ) ) |
                                 ( portMPU_RASR_REGION_ENABLE_BITMASK ) );

            /*
             * 将非特权 Flash 设置为特权和
             * 非特权任务均可只读。所有任务均可读取，但任何人都不能修改。
             */
            portMPU_RBAR_REG = ( ( ( uint32_t ) __FLASH_segment_start__ ) | /* 基地址。 */
                                 ( portMPU_RBAR_REGION_NUMBER_VALID_BITMASK ) |
                                 ( portUNPRIVILEGED_FLASH_REGION ) );

            portMPU_RASR_REG = ( ( portMPU_REGION_PRIV_RO_UNPRIV_RO ) |
                                 ( ( configS_C_B_FLASH & portMPU_RASR_S_C_B_BITMASK ) << portMPU_RASR_S_C_B_LOCATION ) |
                                 ( prvGetMPURegionSizeSetting( ( uint32_t ) __FLASH_segment_end__ - ( uint32_t ) __FLASH_segment_start__ ) ) |
                                 ( portMPU_RASR_REGION_ENABLE_BITMASK ) );

            /* 设置包含内核数据的 RAM 仅供特权访问。 */
            portMPU_RBAR_REG = ( ( uint32_t ) __privileged_sram_start__ ) | /* 基地址。 */
                                 ( portMPU_RBAR_REGION_NUMBER_VALID_BITMASK ) |
                                 ( portPRIVILEGED_RAM_REGION );

            portMPU_RASR_REG = ( ( portMPU_REGION_PRIV_RW_UNPRIV_NA ) |
                                 ( portMPU_REGION_EXECUTE_NEVER ) |
                                 ( ( configS_C_B_SRAM & portMPU_RASR_S_C_B_BITMASK ) << portMPU_RASR_S_C_B_LOCATION ) |
                                 prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_sram_end__ - ( uint32_t ) __privileged_sram_start__ ) |
                                 ( portMPU_RASR_REGION_ENABLE_BITMASK ) );

            /*
             * 使能 MPU 并启用特权背景访问，即未映射区域具有特权访问权限。
             */
            portMPU_CTRL_REG |= ( portMPU_CTRL_PRIV_BACKGROUND_ENABLE_BITMASK |
                                  portMPU_CTRL_ENABLE_BITMASK );
        }
    }

#endif /* configENABLE_MPU（使能 MPU） */

/*-----------------------------------------------------------*/

void vPortYield( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /* 设置 PendSV 以请求一次上下文切换。 */
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;

    /*
     * 通常不需要屏障指令，但使用它们可确保代码完全符合架构规定行为。
     */
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" );
}

/*-----------------------------------------------------------*/

void vPortEnterCritical( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    portDISABLE_INTERRUPTS();
    ulCriticalNesting++;

    /*
     * 通常不需要屏障指令，但使用它们可确保代码完全符合架构规定行为。
     */
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" );
}

/*-----------------------------------------------------------*/

void vPortExitCritical( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    configASSERT( ulCriticalNesting );
    ulCriticalNesting--;

    if( ulCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}

/*-----------------------------------------------------------*/

void SysTick_Handler( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    uint32_t ulPreviousMask;

    ulPreviousMask = portSET_INTERRUPT_MASK_FROM_ISR();

    traceISR_ENTER();
    {
        /* 递增 RTOS 时钟节拍。 */
        if( xTaskIncrementTick() != pdFALSE )
        {
            traceISR_EXIT_TO_SCHEDULER();
            /* 挂起一次上下文切换。 */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
        else
        {
            traceISR_EXIT();
        }
    }

    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulPreviousMask );
}

/*-----------------------------------------------------------*/

void vPortSVCHandler_C( uint32_t * pulCallerStackAddress ) /* PRIVILEGED_FUNCTION portDONT_DISCARD（特权函数，禁止链接器丢弃） */
{
    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 ) )

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;

        #else

            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];

        #endif /* __ARMCC_VERSION 定义条件 */

    #endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 1（使用 V1 MPU 包装器） */

    uint32_t ulPC;
    uint8_t ucSVCNumber;

    /*
     * 寄存器按以下顺序保存在栈中：R0、R1、R2、R3、
     * R12、LR、PC、xPSR。
     */
    ulPC = pulCallerStackAddress[ portOFFSET_TO_PC ];
    ucSVCNumber = ( ( uint8_t * ) ulPC )[ -2 ];

    switch( ucSVCNumber )
    {
        case portSVC_START_SCHEDULER:
            /*
             * 设置第一个任务的上下文，使第一个任务开始执行。
             */
            vRestoreContextOfFirstTask();
            break;

    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 ) )

        case portSVC_RAISE_PRIVILEGE:
            /*
             * 仅当 SVC 是从任一系统调用触发时，才提升特权。
             */
            if( ( ulPC >= ( uint32_t ) __syscalls_flash_start__ ) &&
                ( ulPC <= ( uint32_t ) __syscalls_flash_end__ ) )
            {
                vRaisePrivilege();
            }
            break;

    #endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 1（使用 V1 MPU 包装器） */

    #if ( configENABLE_MPU == 1 )

        case portSVC_YIELD:
            vPortYield();
            break;

    #endif /* configENABLE_MPU == 1（已启用 MPU） */

        default:
            /* 错误的 SVC 调用。 */
            configASSERT( pdFALSE );
    }
}

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vSystemCallEnter( uint32_t * pulTaskStack,
                           uint32_t ulLR,
                           uint8_t ucSystemCallNumber ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        extern TaskHandle_t pxCurrentTCB;
        extern UBaseType_t uxSystemCallImplementations[ NUM_SYSTEM_CALLS ];
        xMPU_SETTINGS * pxMpuSettings;
        uint32_t * pulSystemCallStack;
        uint32_t ulSystemCallLocation, i;
        /*
         * 异常进入时硬件保存的栈帧大小：
         * 基本帧（R0-R3、R12、LR、PC 和 xPSR）为 8 个字。
         */
        const uint32_t ulHardwareSavedExceptionFrameSize = 8;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;

        #else

            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];

        #endif /* __ARMCC_VERSION 定义条件 */

        ulSystemCallLocation = pulTaskStack[ portOFFSET_TO_PC ];
        pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );

        /*
         * 检查：
         * 1. SVC 是从系统调用代码段触发的（即应用程序没有直接触发 SVC）。
         * 2. pxMpuSettings->xSystemCallStackInfo.pulTaskStack 必须为 NULL，
         *    因为仅在系统调用执行期间（即系统调用进入和退出之间）
         *    该值才非 NULL。
         * 3. 系统调用对应的内核 API 未被 FreeRTOSConfig.h 中的配置禁用。
         * 4. 无需检查 ucSystemCallNumber 是否在有效范围内，
         *    因为汇编 SVC 处理程序在调用本函数前已经检查过。
         */
        if( ( ulSystemCallLocation >= ( uint32_t ) __syscalls_flash_start__ ) &&
            ( ulSystemCallLocation <= ( uint32_t ) __syscalls_flash_end__ ) &&
            ( pxMpuSettings->xSystemCallStackInfo.pulTaskStack == NULL ) &&
            ( uxSystemCallImplementations[ ucSystemCallNumber ] != ( UBaseType_t ) 0 ) )
        {
            pulSystemCallStack = pxMpuSettings->xSystemCallStackInfo.pulSystemCallStack;

            /* 在系统调用栈上为栈帧分配空间。 */
            pulSystemCallStack = pulSystemCallStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulSystemCallStack[ i ] = pulTaskStack[ i ];
            }

            /*
             * 保存触发 SVC 之前的链接寄存器值。
             * 其中包含系统调用入口点的调用者地址（即 MPU_<API> 的调用者）。
             * 退出系统调用时需要恢复该值。
             */
            pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry = pulTaskStack[ portOFFSET_TO_LR ];

            /* 在线程模式下使用 pulSystemCallStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulSystemCallStack ) );

            /* 从本处理程序返回后开始执行系统调用。 */
            pulSystemCallStack[ portOFFSET_TO_PC ] = uxSystemCallImplementations[ ucSystemCallNumber ];

            /*
             * 请求在系统调用完成后退出系统调用。
             */
            pulSystemCallStack[ portOFFSET_TO_LR ] = ( uint32_t ) vRequestSystemCallExit;

            /*
             * 记住退出系统调用时应复制栈帧的位置。
             */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = pulTaskStack + ulHardwareSavedExceptionFrameSize;

            /*
             * 记录硬件是否使用填充强制栈指针双字对齐。
             */
            if( ( pulTaskStack[ portOFFSET_TO_PSR ] & portPSR_STACK_PADDING_MASK ) == portPSR_STACK_PADDING_MASK )
            {
                pxMpuSettings->ulTaskFlags |= portSTACK_FRAME_HAS_PADDING_FLAG;
            }
            else
            {
                pxMpuSettings->ulTaskFlags &= ( ~portSTACK_FRAME_HAS_PADDING_FLAG );
            }

            /*
             * 在 pxPortInitialiseStack 中已确保系统调用栈
             * 双字对齐，因此无需填充。
             * 清除已保存 xPSR 的 bit[9]。
             */
            pulSystemCallStack[ portOFFSET_TO_PSR ] &= ( ~portPSR_STACK_PADDING_MASK );

            /* 在系统调用执行期间提升特权。 */
            __asm volatile
            (
                " .syntax unified     \n"
                " mrs r0, control     \n" /* 获取当前 CONTROL 值。 */
                " movs r1, #1         \n" /* r1 = 1，即立即数 1。 */
                " bics r0, r1         \n" /* 清除 nPRIV 位。 */
                " msr control, r0     \n" /* 写回新的 CONTROL 值。 */
                ::: "r0", "r1", "memory"
            );
        }
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vRequestSystemCallExit( void ) /* __attribute__( ( naked ) ) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile ( "svc %0 \n" ::"i" ( portSVC_SYSTEM_CALL_EXIT ) : "memory" );
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        extern TaskHandle_t pxCurrentTCB;
        xMPU_SETTINGS * pxMpuSettings;
        uint32_t * pulTaskStack;
        uint32_t ulSystemCallLocation, i;
        /*
         * 异常进入时硬件保存的栈帧大小：
         * 基本帧（R0-R3、R12、LR、PC 和 xPSR）为 8 个字。
         */
        const uint32_t ulHardwareSavedExceptionFrameSize = 8;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;

        #else

            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];

        #endif /* __ARMCC_VERSION 定义条件 */

        ulSystemCallLocation = pulSystemCallStack[ portOFFSET_TO_PC ];
        pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );

        /*
         * 检查：
         * 1. SVC 是从特权代码触发的（即应用程序没有直接触发 SVC）。
         *    此 SVC 仅由位于特权代码段的 vRequestSystemCallExit 触发。
         * 2. pxMpuSettings->xSystemCallStackInfo.pulTaskStack 必须非 NULL——
         *    这表明此前已进入系统调用，应用程序没有在未进入系统调用
         *    的情况下尝试退出。
         */
        if( ( ulSystemCallLocation >= ( uint32_t ) __privileged_functions_start__ ) &&
            ( ulSystemCallLocation <= ( uint32_t ) __privileged_functions_end__ ) &&
            ( pxMpuSettings->xSystemCallStackInfo.pulTaskStack != NULL ) )
        {
            pulTaskStack = pxMpuSettings->xSystemCallStackInfo.pulTaskStack;

            /* 在任务栈上为栈帧分配空间。 */
            pulTaskStack = pulTaskStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulTaskStack[ i ] = pulSystemCallStack[ i ];
            }

            /* 在线程模式下使用 pulTaskStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulTaskStack ) );

            /*
             * 返回系统调用入口点的调用者（即 MPU_<API> 的调用者）。
             */
            pulTaskStack[ portOFFSET_TO_PC ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;

            /* 确保 LR 具有有效值。 */
            pulTaskStack[ portOFFSET_TO_LR ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;

            /*
             * 若硬件使用了填充来强制栈指针双字对齐，
             * 则置位已保存 xPSR 的 bit[9]，否则清除该位。
             */
            if( ( pxMpuSettings->ulTaskFlags & portSTACK_FRAME_HAS_PADDING_FLAG ) == portSTACK_FRAME_HAS_PADDING_FLAG )
            {
                pulTaskStack[ portOFFSET_TO_PSR ] |= portPSR_STACK_PADDING_MASK;
            }
            else
            {
                pulTaskStack[ portOFFSET_TO_PSR ] &= ( ~portPSR_STACK_PADDING_MASK );
            }

            /* 仅在系统调用期间该值才非 NULL。 */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = NULL;

            /* 返回线程模式前降低特权。 */
            __asm volatile
            (
                " .syntax unified     \n"
                " mrs r0, control     \n" /* 获取当前 CONTROL 值。 */
                " movs r1, #1         \n" /* r1 = 1，即立即数 1。 */
                " orrs r0, r1         \n" /* 设置 nPRIV 位。 */
                " msr control, r0     \n" /* 写回新的 CONTROL 值。 */
                ::: "r0", "r1", "memory"
            );
        }
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    BaseType_t xPortIsTaskPrivileged( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        BaseType_t xTaskIsPrivileged = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xTaskIsPrivileged = pdTRUE;
        }

        return xTaskIsPrivileged;
    }

#endif /* configENABLE_MPU == 1（已启用 MPU） */

/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters,
                                         BaseType_t xRunPrivileged,
                                         xMPU_SETTINGS * xMPUSettings ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        xMPUSettings->ulContext[ 0 ] = 0x04040404; /* r4 寄存器。 */
        xMPUSettings->ulContext[ 1 ] = 0x05050505; /* r5 寄存器。 */
        xMPUSettings->ulContext[ 2 ] = 0x06060606; /* r6 寄存器。 */
        xMPUSettings->ulContext[ 3 ] = 0x07070707; /* r7 寄存器。 */
        xMPUSettings->ulContext[ 4 ] = 0x08080808; /* r8 寄存器。 */
        xMPUSettings->ulContext[ 5 ] = 0x09090909; /* r9 寄存器。 */
        xMPUSettings->ulContext[ 6 ] = 0x10101010; /* r10 寄存器。 */
        xMPUSettings->ulContext[ 7 ] = 0x11111111; /* r11 寄存器。 */

        xMPUSettings->ulContext[ 8 ] = ( uint32_t ) pvParameters;            /* r0 寄存器。 */
        xMPUSettings->ulContext[ 9 ] = 0x01010101;                           /* r1 寄存器。 */
        xMPUSettings->ulContext[ 10 ] = 0x02020202;                           /* r2 寄存器。 */
        xMPUSettings->ulContext[ 11 ] = 0x03030303;                           /* r3 寄存器。 */
        xMPUSettings->ulContext[ 12 ] = 0x12121212;                           /* r12 寄存器。 */
        xMPUSettings->ulContext[ 13 ] = ( uint32_t ) portTASK_RETURN_ADDRESS; /* LR 寄存器。 */
        xMPUSettings->ulContext[ 14 ] = ( uint32_t ) pxCode;                  /* PC 寄存器。 */
        xMPUSettings->ulContext[ 15 ] = portINITIAL_XPSR;                     /* xPSR 寄存器。 */

        xMPUSettings->ulContext[ 16 ] = ( uint32_t ) ( pxTopOfStack - 8 ); /* PSP 指向硬件保存的栈帧。 */
        if( xRunPrivileged == pdTRUE )
        {
            xMPUSettings->ulTaskFlags |= portTASK_IS_PRIVILEGED_FLAG;
            xMPUSettings->ulContext[ 17 ] = ( uint32_t ) portINITIAL_CONTROL_PRIVILEGED; /* CONTROL 寄存器。 */
        }
        else
        {
            xMPUSettings->ulTaskFlags &= ( ~portTASK_IS_PRIVILEGED_FLAG );
            xMPUSettings->ulContext[ 17 ] = ( uint32_t ) portINITIAL_CONTROL_UNPRIVILEGED; /* CONTROL 寄存器。 */
        }
        xMPUSettings->ulContext[ 18 ] = portINITIAL_EXC_RETURN; /* LR（EXC_RETURN 值）。 */

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
        {
            /* 确保系统调用栈双字对齐。 */
            xMPUSettings->xSystemCallStackInfo.pulSystemCallStack = &( xMPUSettings->xSystemCallStackInfo.ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE - 1 ] );
            xMPUSettings->xSystemCallStackInfo.pulSystemCallStack = ( uint32_t * ) ( ( uint32_t ) ( xMPUSettings->xSystemCallStackInfo.pulSystemCallStack ) &
                                                                                     ( uint32_t ) ( ~( portBYTE_ALIGNMENT_MASK ) ) );

            /* 仅在系统调用期间该值才非 NULL。 */
            xMPUSettings->xSystemCallStackInfo.pulTaskStack = NULL;
        }
        #endif /* configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

        return &( xMPUSettings->ulContext[ 19 ] );
    }

#else /* configENABLE_MPU（使能 MPU） */

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        /*
         * 模拟上下文切换中断会创建的栈帧。
         */
        #if ( portPRELOAD_REGISTERS == 0 )
        {
            pxTopOfStack--;                                          /* 用于补偿 MCU 在中断进入/退出时使用栈的方式而增加的偏移量。 */
            *pxTopOfStack = portINITIAL_XPSR;                        /* xPSR 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxCode;                  /* PC 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR 寄存器。 */
            pxTopOfStack -= 5;                                       /* R12、R3、R2 和 R1 寄存器。 */
            *pxTopOfStack = ( StackType_t ) pvParameters;            /* R0 寄存器。 */
            pxTopOfStack -= 9;                                       /* R11..R4 及 EXC_RETURN。 */
            *pxTopOfStack = portINITIAL_EXC_RETURN;
        }
        #else /* portPRELOAD_REGISTERS（预装载寄存器） */
        {
            pxTopOfStack--;                                          /* 用于补偿 MCU 在中断进入/退出时使用栈的方式而增加的偏移量。 */
            *pxTopOfStack = portINITIAL_XPSR;                        /* xPSR 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxCode;                  /* PC 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x12121212UL;            /* R12 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x03030303UL;            /* R3 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x02020202UL;            /* R2 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x01010101UL;            /* R1 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pvParameters;            /* R0 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x11111111UL;            /* R11 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x10101010UL;            /* R10 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x09090909UL;            /* R09 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x08080808UL;            /* R08 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x07070707UL;            /* R07 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x06060606UL;            /* R06 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x05050505UL;            /* R05 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x04040404UL;            /* R04 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = portINITIAL_EXC_RETURN;                  /* EXC_RETURN 值。 */
        }
        #endif /* portPRELOAD_REGISTERS（预装载寄存器） */

        return pxTopOfStack;
    }

#endif /* configENABLE_MPU（使能 MPU） */

/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /*
     * 应用程序可以采用以下方式之一安装 FreeRTOS 中断处理程序：
     * 1. 直接路由——分别为 SVCall 和 PendSV 中断安装
     *    SVC_Handler 和 PendSV_Handler 函数。
     * 2. 间接路由——为 SVCall 和 PendSV 中断安装独立的处理程序，
     *    并将程序控制流从这些处理程序转发到 SVC_Handler 和
     *    PendSV_Handler 函数。
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
        configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == SVC_Handler );
        configASSERT( pxVectorTable[ portVECTOR_INDEX_PENDSV ] == PendSV_Handler );
    }
    #endif /* configCHECK_HANDLER_INSTALLATION（检查处理程序安装） */

    /*
     * 将 PendSV 和 SysTick 设为最低优先级中断，并将 SVCall
     * 设为最高优先级。
     */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
    portNVIC_SHPR2_REG = 0;

    #if ( configENABLE_MPU == 1 )
    {
        /* 初始化内存保护单元（MPU）。 */
        prvSetupMPU();
    }
    #endif /* configENABLE_MPU（使能 MPU） */

    /*
     * 启动产生时钟节拍 ISR 的定时器。此处中断已经处于禁用状态。
     */
    vPortSetupTimerInterrupt();

    /* 初始化临界区嵌套计数，为第一个任务做好准备。 */
    ulCriticalNesting = 0;

    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )
    {
        xSchedulerRunning = pdTRUE;
    }
    #endif

    /* 启动第一个任务。 */
    vStartFirstTask();

    /*
     * 任务此时已经开始执行，不应到达这里。调用任务退出错误函数，
     * 以避免在应用程序编写者通过定义 configTASK_RETURN_ADDRESS
     * 覆盖此功能时出现静态函数未被调用的编译器警告。调用
     * vTaskSwitchContext() 可防止链接时优化移除该符号。
     */
    vTaskSwitchContext();
    prvTaskExitError();

    /* 不应到达这里。 */
    return 0;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /*
     * 在无处可返回的移植层中未实现。
     * 人为触发断言。
     */
    configASSERT( ulCriticalNesting == 1000UL );
}

/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    void vPortStoreTaskMPUSettings( xMPU_SETTINGS * xMPUSettings,
                                    const struct xMEMORY_REGION * const xRegions,
                                    StackType_t * pxBottomOfStack,
                                    configSTACK_DEPTH_TYPE uxStackDepth )
    {
        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __SRAM_segment_start__;
            extern uint32_t * __SRAM_segment_end__;
            extern uint32_t * __privileged_sram_start__;
            extern uint32_t * __privileged_sram_end__;

        #else
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __SRAM_segment_start__[];
            extern uint32_t __SRAM_segment_end__[];
            extern uint32_t __privileged_sram_start__[];
            extern uint32_t __privileged_sram_end__[];

        #endif /* __ARMCC_VERSION 定义条件 */

        int32_t lIndex;
        uint32_t ul;

        if( xRegions == NULL )
        {
            /* 未指定任何 MPU 区域，因此允许访问所有 RAM。 */
            xMPUSettings->xRegionsSettings[ 0 ].ulRBAR =
                ( ( ( uint32_t ) __SRAM_segment_start__ ) | /* 基地址。 */
                  ( portMPU_RBAR_REGION_NUMBER_VALID_BITMASK ) |
                  ( portSTACK_REGION ) );                   /* 区域编号。 */

            xMPUSettings->xRegionsSettings[ 0 ].ulRASR =
                ( ( portMPU_REGION_PRIV_RW_UNPRIV_RW ) |
                  ( portMPU_REGION_EXECUTE_NEVER ) |
                  ( ( configS_C_B_SRAM & portMPU_RASR_S_C_B_BITMASK ) << portMPU_RASR_S_C_B_LOCATION ) |
                  ( prvGetMPURegionSizeSetting( ( uint32_t ) __SRAM_segment_end__ - ( uint32_t ) __SRAM_segment_start__ ) ) |
                  ( portMPU_RASR_REGION_ENABLE_BITMASK ) );


            /* 使用户可配置区域失效。 */
            for( ul = 1UL; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ )
            {
                xMPUSettings->xRegionsSettings[ ul ].ulRBAR = ( ( ul - 1UL ) | portMPU_RBAR_REGION_NUMBER_VALID_BITMASK );
                xMPUSettings->xRegionsSettings[ ul ].ulRASR = 0UL;
            }
        }
        else
        {
            /*
             * 创建任务时会自动调用该函数——此时栈区域参数有效。其他
             * 时候栈参数无效，并假定栈区域已经配置好。
             */
            if( uxStackDepth > 0 )
            {
                /* 定义允许访问栈的区域。 */
                xMPUSettings->xRegionsSettings[ 0 ].ulRBAR =
                    ( ( ( uint32_t ) pxBottomOfStack ) |
                      ( portMPU_RBAR_REGION_NUMBER_VALID_BITMASK ) |
                      ( portSTACK_REGION ) ); /* 区域编号。 */

                xMPUSettings->xRegionsSettings[ 0 ].ulRASR =
                    ( ( portMPU_REGION_PRIV_RW_UNPRIV_RW ) |
                      ( portMPU_REGION_EXECUTE_NEVER ) |
                      ( prvGetMPURegionSizeSetting( uxStackDepth * ( uint32_t ) sizeof( StackType_t ) ) ) |
                      ( ( configS_C_B_SRAM & portMPU_RASR_S_C_B_BITMASK ) << portMPU_RASR_S_C_B_LOCATION ) |
                      ( portMPU_RASR_REGION_ENABLE_BITMASK ) );
            }

            lIndex = 0;

            for( ul = 1UL; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ )
            {
                if( ( xRegions[ lIndex ] ).ulLengthInBytes > 0UL )
                {
                    /*
                     * 将以 xRegions 中通用区域定义转换为 CM0+ 特定的 MPU 设置，
                     * 然后存入 xMPUSettings。
                     */
                    xMPUSettings->xRegionsSettings[ ul ].ulRBAR =
                        ( ( uint32_t ) xRegions[ lIndex ].pvBaseAddress ) |
                        ( portMPU_RBAR_REGION_NUMBER_VALID_BITMASK ) |
                        ( ul - 1UL ); /* 区域编号。 */

                    xMPUSettings->xRegionsSettings[ ul ].ulRASR =
                        ( prvGetMPURegionSizeSetting( xRegions[ lIndex ].ulLengthInBytes ) ) |
                        ( xRegions[ lIndex ].ulParameters ) |
                        ( portMPU_RASR_REGION_ENABLE_BITMASK );
                }
                else
                {
                    /* 使该区域失效。 */
                    xMPUSettings->xRegionsSettings[ ul ].ulRBAR = ( ( ul - 1UL ) | portMPU_RBAR_REGION_NUMBER_VALID_BITMASK );
                    xMPUSettings->xRegionsSettings[ ul ].ulRASR = 0UL;
                }

            lIndex++;
        }
    }
}

#endif /* configENABLE_MPU（使能 MPU） */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    BaseType_t xPortIsAuthorizedToAccessBuffer( const void * pvBuffer,
                                                uint32_t ulBufferLength,
                                                uint32_t ulAccessRequested ) /* PRIVILEGED_FUNCTION（特权函数） */

    {
        uint32_t i, ulBufferStartAddress, ulBufferEndAddress;
        uint32_t ulRegionStart, ulRegionSize, ulRegionEnd;
        uint32_t ulMPURegionAccessPermissions;
        BaseType_t xAccessGranted = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( xSchedulerRunning == pdFALSE )
        {
            /*
             * 在调度器启动前授予对所有内核对象的访问权限。这是必要的，
             * 因为此时尚无任务运行，无法使用任何任务的权限。
             */
            xAccessGranted = pdTRUE;
        }
        else if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xAccessGranted = pdTRUE;
        }
        else
        {
            if( portADD_UINT32_WILL_OVERFLOW( ( ( uint32_t ) pvBuffer ), ( ulBufferLength - 1UL ) ) == pdFALSE )
            {
                ulBufferStartAddress = ( uint32_t ) pvBuffer;
                ulBufferEndAddress = ( ( ( uint32_t ) pvBuffer ) + ulBufferLength - 1UL );

                for( i = 0; i < portTOTAL_NUM_REGIONS; i++ )
                {
                    /* 该 MPU 区域是否已使能？ */
                    if( ( xTaskMpuSettings->xRegionsSettings[ i ].ulRASR &
                          portMPU_RASR_REGION_ENABLE_BITMASK ) == portMPU_RASR_REGION_ENABLE_BITMASK )
                    {
                        ulRegionStart = portEXTRACT_FIRST_ADDRESS_FROM_RBAR( xTaskMpuSettings->xRegionsSettings[ i ].ulRBAR );
                        ulRegionSize = portEXTRACT_REGION_SIZE_FROM_RASR( xTaskMpuSettings->xRegionsSettings[ i ].ulRASR );
                        ulRegionEnd = ulRegionStart + ulRegionSize;

                        if( portIS_ADDRESS_WITHIN_RANGE( ulBufferStartAddress,
                                                         ulRegionStart,
                                                         ulRegionEnd ) &&
                            portIS_ADDRESS_WITHIN_RANGE( ulBufferEndAddress,
                                                         ulRegionStart,
                                                         ulRegionEnd ) )
                        {
                            ulMPURegionAccessPermissions = xTaskMpuSettings->xRegionsSettings[ i ].ulRASR &
                                                           portMPU_RASR_AP_BITMASK;

                            if( ulAccessRequested == tskMPU_READ_PERMISSION ) /* 只读（RO）。 */
                            {
                                if( ( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RW_UNPRIV_RO ) ||
                                    ( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RO_UNPRIV_RO ) ||
                                    ( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RW_UNPRIV_RW ) )
                                {
                                    xAccessGranted = pdTRUE;
                                    break;
                                }
                            }
                            else if( ( ulAccessRequested & tskMPU_WRITE_PERMISSION ) != 0UL ) /* 写或读写（W 或 RW）。 */
                            {
                                if( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RW_UNPRIV_RW )
                                {
                                    xAccessGranted = pdTRUE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        return xAccessGranted;
    }

#endif /* configENABLE_MPU == 1、configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1 */

/*-----------------------------------------------------------*/

BaseType_t xPortIsInsideInterrupt( void )
{
    uint32_t ulCurrentInterrupt;
    BaseType_t xReturn;

    /*
     * 获取当前正在执行的中断编号。中断程序状态寄存器（IPSR）保存
     * 当前正在执行的异常编号；处于线程模式时为零。
     */
    __asm volatile ( "mrs %0, ipsr" : "=r" ( ulCurrentInterrupt )::"memory" );

    if( ulCurrentInterrupt == 0 )
    {
        xReturn = pdFALSE;
    }
    else
    {
        xReturn = pdTRUE;
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

    void vPortGrantAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                         int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
        xMPU_SETTINGS * xTaskMpuSettings;

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

        xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

        xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] |= ( 1U << ulAccessControlListEntryBit );
    }

#endif /* configENABLE_MPU == 1、configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1 */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

    void vPortRevokeAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                          int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
        xMPU_SETTINGS * xTaskMpuSettings;

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

        xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

        xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] &= ~( 1U << ulAccessControlListEntryBit );
    }

#endif /* configENABLE_MPU == 1、configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1 */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
        {
            uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
            BaseType_t xAccessGranted = pdFALSE;
            const xMPU_SETTINGS * xTaskMpuSettings;

            if( xSchedulerRunning == pdFALSE )
            {
                /*
                 * 在调度器启动前授予对所有内核对象的访问权限。这是必要的，
                 * 因为此时尚无任务运行，无法使用任何任务的权限。
                 */
                xAccessGranted = pdTRUE;
            }
            else
            {
                xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

                ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
                ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

                if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
                {
                    xAccessGranted = pdTRUE;
                }
                else
                {
                    if( ( xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] & ( 1U << ulAccessControlListEntryBit ) ) != 0 )
                    {
                        xAccessGranted = pdTRUE;
                    }
                }
            }

            return xAccessGranted;
        }

    #else /* configENABLE_ACCESS_CONTROL_LIST == 1（启用访问控制列表） */

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
        {
            ( void ) lInternalIndexOfKernelObject;

            /*
             * 若不使用访问控制列表功能，所有任务均可访问所有内核对象。
             */
            return pdTRUE;
        }

    #endif /* configENABLE_ACCESS_CONTROL_LIST == 1（启用访问控制列表） */

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/*-----------------------------------------------------------*/
