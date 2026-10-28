/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT AND BSD-3-Clause
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

/* 本文件已针对 Doxygen 自动文档生成做好准备。 */

/*!
 * ! \file *********************************************************************
 *
 * \brief AVR32 UC3 的 FreeRTOS 移植层源文件。
 *
 * - 编译器：          GNU GCC for AVR32
 * - 支持的器件：      可使用所有 AVR32 器件。
 * - 应用笔记：
 *
 * \author               Atmel Corporation（现为 Microchip）：
 *                                        https://www.microchip.com \n
 *                       支持与常见问题：https://www.microchip.com/support/
 *
 * ***************************************************************************
 */

/*
 * Copyright (c) 2007, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 标准头文件包含。 */
#include <sys/cpu.h>
#include <sys/usart.h>
#include <malloc.h>

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* AVR32 UC3 头文件包含。 */
#include <avr32/io.h>
#include "gpio.h"
#if ( configTICK_USE_TC == 1 )
    #include "tc.h"
#endif


/* 配置任务上下文所需的常量。 */
#define portINITIAL_SR             ( ( StackType_t ) 0x00400000 ) /* AVR32 模式配置位：[M2:M0]=001 I1M=0 I0M=0，GM=0 */
#define portINSTRUCTION_SIZE       ( ( StackType_t ) 0 )

/* 每个任务维护各自的临界区嵌套计数变量。 */
#define portNO_CRITICAL_NESTING    ( ( uint32_t ) 0 )
volatile uint32_t ulCriticalNesting = 9999UL;

#if ( configTICK_USE_TC == 0 )
    static void prvScheduleNextTick( void );
#else
    static void prvClearTcInt( void );
#endif

/* 配置定时器以产生时钟节拍（tick）中断。 */
static void prvSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/

/*
 * 在 main 函数之前、启动过程中调用的底层初始化例程。
 * 本版本用于替换 Newlib 提供的默认版本。
 * Newlib 的 _init_startup 仅调用 init_exceptions，但 Newlib 的异常
 * 向量与当前 FreeRTOS 移植层中的 SCALL 管理不兼容。此外此处还增加了
 * 更多底层初始化。
 */
void _init_startup( void )
{
    /* 导入异常向量基地址。 */
    extern void _evba;

    #if configHEAP_INIT
        extern void __heap_start__;
        extern void __heap_end__;
        BaseType_t * pxMem;
    #endif

    /* 将异常向量基地址装入相应的系统寄存器。 */
    Set_system_register( AVR32_EVBA, ( int ) &_evba );

    /* 使能异常。 */
    ENABLE_ALL_EXCEPTIONS();

    /* 初始化中断处理。 */
    INTC_init_interrupts();

    #if configHEAP_INIT
        /* 初始化 malloc 使用的堆。 */
        for( pxMem = &__heap_start__; pxMem < ( BaseType_t * ) &__heap_end__; )
        {
            *pxMem++ = 0xA5A5A5A5;
        }
    #endif

    /* 将所用 CPU 时钟频率提供给 Newlib，以便其正常工作。 */
    set_cpu_hz( configCPU_CLOCK_HZ );

    /* 仅当调试跟踪被激活时才存在此代码段。 */
    #if configDBG
    {
        static const gpio_map_t DBG_USART_GPIO_MAP =
        {
            { configDBG_USART_RX_PIN, configDBG_USART_RX_FUNCTION },
            { configDBG_USART_TX_PIN, configDBG_USART_TX_FUNCTION }
        };

        /* 用配置的参数初始化用于调试跟踪的 USART。 */
        set_usart_base( ( void * ) configDBG_USART );
        gpio_enable_module( DBG_USART_GPIO_MAP,
                            sizeof( DBG_USART_GPIO_MAP ) / sizeof( DBG_USART_GPIO_MAP[ 0 ] ) );
        usart_init( configDBG_USART_BAUDRATE );
    }
    #endif /* 若 configDBG */
}
/*-----------------------------------------------------------*/

/*
 * malloc、realloc 和 free 应分别通过 pvPortMalloc、pvPortRealloc 和
 * vPortFree 调用。
 * 后一组函数在任务挂起的区内调用前一组函数，因此后一组函数是任务安全的。
 * __malloc_lock 和 __malloc_unlock 使用相同机制，使前一组函数也保持任务安全，
 * 因为它们可能被 Newlib 函数直接调用。
 * 然而，所有这些函数都是中断不安全的，因此不得在中断中调用，因为
 * __malloc_lock 和 __malloc_unlock 不调用 portENTER_CRITICAL 和
 * portEXIT_CRITICAL，以免在内存分配管理期间禁止中断，因为这一过程可能
 * 非常耗时。
 */

/*
 * Newlib 在进入 malloc / realloc / free 时调用的加锁例程，用于保证代码段的
 * 安全性，因为内存分配管理使用全局数据。
 * 详见前述说明。
 */
void __malloc_lock( struct _reent * ptr )
{
    vTaskSuspendAll();
}

/*
 * Newlib 在退出 malloc / realloc / free 时调用的解锁例程，用于保证代码段的
 * 安全性，因为内存分配管理使用全局数据。
 * 详见前述说明。
 */
void __malloc_unlock( struct _reent * ptr )
{
    xTaskResumeAll();
}
/*-----------------------------------------------------------*/

/* 因 FreeRTOS 中没有此函数而添加。 */
void * pvPortRealloc( void * pv,
                      size_t xWantedSize )
{
    void * pvReturn;

    vTaskSuspendAll();
    {
        pvReturn = realloc( pv, xWantedSize );
    }
    xTaskResumeAll();

    return pvReturn;
}
/*-----------------------------------------------------------*/

/*
 * 协作式调度器需要一个普通的 IRQ 服务程序，仅递增系统时钟节拍（tick）。
 */

/*
 * 抢占式调度器定义为 "naked"，因为进入时已作为上下文切换的一部分保存了完整上下文。
 */
__attribute__( ( __naked__ ) ) static void vTick( void )
{
    /* 保存被中断任务的上下文。 */
    portSAVE_CONTEXT_OS_INT();

    #if ( configTICK_USE_TC == 1 )
        /* 清除中断标志。 */
        prvClearTcInt();
    #else

        /*
         * 从当前时刻起，在 (configCPU_CLOCK_HZ/configTICK_RATE_HZ) 个时钟周期后
         * 调度 COUNT&COMPARE 匹配中断。
         */
        prvScheduleNextTick();
    #endif

    /*
     * 由于 FreeRTOS 不应在嵌套中断下运行，所有 OS 调用都置于临界区中。
     */
    portENTER_CRITICAL();
    xTaskIncrementTick();
    portEXIT_CRITICAL();

    /* 恢复“被选中任务”的上下文。 */
    portRESTORE_CONTEXT_OS_INT();
}
/*-----------------------------------------------------------*/

__attribute__( ( __naked__ ) ) void SCALLYield( void )
{
    /* 保存被中断任务的上下文。 */
    portSAVE_CONTEXT_SCALL();
    vTaskSwitchContext();
    portRESTORE_CONTEXT_SCALL();
}
/*-----------------------------------------------------------*/

/*
 * GCC 编译器生成的代码在不同优化级别下对栈的使用方式不同，因此中断标志
 * 无法总是保存到栈上。改为将临界区嵌套层数存入变量，再作为栈上下文的一部分保存。
 */
__attribute__( ( __noinline__ ) ) void vPortEnterCritical( void )
{
    /* 禁止中断 */
    portDISABLE_INTERRUPTS();

    /*
     * 中断现已禁止，可以直接访问 ulCriticalNesting。递增 ulCriticalNesting，
     * 以记录 portENTER_CRITICAL() 已被调用的次数。
     */
    ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

__attribute__( ( __noinline__ ) ) void vPortExitCritical( void )
{
    if( ulCriticalNesting > portNO_CRITICAL_NESTING )
    {
        ulCriticalNesting--;

        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /* 使能所有中断/异常。 */
            portENABLE_INTERRUPTS();
        }
    }
}
/*-----------------------------------------------------------*/

/*
 * 初始化任务栈，使其与调用过 portSAVE_CONTEXT 后的状态完全一致。
 *
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 设置任务的初始栈。栈内容按 portRESTORE_CONTEXT() 宏的预期精确设置。
     */

    /* 任务启动时，将期望在 R12 中找到函数参数。 */
    pxTopOfStack--;
    *pxTopOfStack-- = ( StackType_t ) 0x08080808;                    /* R8 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x09090909;                    /* R9 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x0A0A0A0A;                    /* R10 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x0B0B0B0B;                    /* R11 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) pvParameters;                  /* R12 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0xDEADBEEF;                    /* R14/LR 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) pxCode + portINSTRUCTION_SIZE; /* R15/PC 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) portINITIAL_SR;                /* SR 状态寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0xFF0000FF;                    /* R0 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x01010101;                    /* R1 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x02020202;                    /* R2 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x03030303;                    /* R3 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x04040404;                    /* R4 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x05050505;                    /* R5 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x06060606;                    /* R6 寄存器 */
    *pxTopOfStack-- = ( StackType_t ) 0x07070707;                    /* R7 寄存器 */
    *pxTopOfStack = ( StackType_t ) portNO_CRITICAL_NESTING;         /* ulCriticalNesting（临界区嵌套计数） */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /*
     * 启动产生时钟节拍 ISR 的定时器。此处中断已被禁止。
     */
    prvSetupTimerInterrupt();

    /* 启动第一个任务。 */
    portRESTORE_CONTEXT();

    /* 不应执行到此处！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * AVR32 移植层不太可能需要此函数，因为没有可供返回之处。
     */
}
/*-----------------------------------------------------------*/

/*
 * 从当前时刻起，在 (configCPU_CLOCK_HZ/configTICK_RATE_HZ) 个时钟周期后
 * 调度 COUNT&COMPARE 匹配中断。
 */
#if ( configTICK_USE_TC == 0 )
    static void prvScheduleFirstTick( void )
    {
        uint32_t lCycles;

        lCycles = Get_system_register( AVR32_COUNT );
        lCycles += ( configCPU_CLOCK_HZ / configTICK_RATE_HZ );

        /* 如果 lCycles 最终为 0，则将其置为 1，以免 COMPARE 及异常 */
        /* 生成功能被禁用。 */
        if( 0 == lCycles )
        {
            lCycles++;
        }

        Set_system_register( AVR32_COMPARE, lCycles );
    }

    __attribute__( ( __noinline__ ) ) static void prvScheduleNextTick( void )
    {
        uint32_t lCycles, lCount;

        lCycles = Get_system_register( AVR32_COMPARE );
        lCycles += ( configCPU_CLOCK_HZ / configTICK_RATE_HZ );

        /* 如果 lCycles 最终为 0，则将其置为 1，以免 COMPARE 及异常 */
        /* 生成功能被禁用。 */
        if( 0 == lCycles )
        {
            lCycles++;
        }

        lCount = Get_system_register( AVR32_COUNT );

        if( lCycles < lCount )
        { /* 错过了一个时钟节拍（tick），为下一个节拍做补偿。 */
            lCycles += ( configCPU_CLOCK_HZ / configTICK_RATE_HZ );
        }

        Set_system_register( AVR32_COMPARE, lCycles );
    }
#else /* 若 ( configTICK_USE_TC == 0 ) */
    __attribute__( ( __noinline__ ) ) static void prvClearTcInt( void )
    {
        AVR32_TC.channel[ configTICK_TC_CHANNEL ].sr;
    }
#endif /* 若 ( configTICK_USE_TC == 0 ) */
/*-----------------------------------------------------------*/

/* 配置定时器以产生时钟节拍（tick）中断。 */
static void prvSetupTimerInterrupt( void )
{
    #if ( configTICK_USE_TC == 1 )
        volatile avr32_tc_t * tc = &AVR32_TC;

        /* 波形生成选项。 */
        tc_waveform_opt_t waveform_opt =
        {
            .channel = configTICK_TC_CHANNEL,              /* 通道选择。 */

            .bswtrg  = TC_EVT_EFFECT_NOOP,                 /* 软件触发对 TIOB 的影响。 */
            .beevt   = TC_EVT_EFFECT_NOOP,                 /* 外部事件对 TIOB 的影响。 */
            .bcpc    = TC_EVT_EFFECT_NOOP,                 /* RC 比较对 TIOB 的影响。 */
            .bcpb    = TC_EVT_EFFECT_NOOP,                 /* RB 比较对 TIOB 的影响。 */

            .aswtrg  = TC_EVT_EFFECT_NOOP,                 /* 软件触发对 TIOA 的影响。 */
            .aeevt   = TC_EVT_EFFECT_NOOP,                 /* 外部事件对 TIOA 的影响。 */
            .acpc    = TC_EVT_EFFECT_NOOP,                 /* RC 比较对 TIOA 的影响：翻转。 */
            .acpa    = TC_EVT_EFFECT_NOOP,                 /* RA 比较对 TIOA 的影响：翻转（其他可选值为无、置位和清零）。 */

            .wavsel  = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER, /* 波形选择：向上模式，RC 比较时不自动触发。 */
            .enetrg  = FALSE,                              /* 使能外部事件触发。 */
            .eevt    = 0,                                  /* 外部事件选择。 */
            .eevtedg = TC_SEL_NO_EDGE,                     /* 外部事件边沿选择。 */
            .cpcdis  = FALSE,                              /* RC 比较时禁用计数器。 */
            .cpcstop = FALSE,                              /* RC 比较时停止计数器时钟。 */

            .burst   = FALSE,                              /* 突发信号选择。 */
            .clki    = FALSE,                              /* 时钟反相。 */
            .tcclks  = TC_CLOCK_SOURCE_TC2                 /* 内部源时钟 2。 */
        };

        tc_interrupt_t tc_interrupt =
        {
            .etrgs = 0,
            .ldrbs = 0,
            .ldras = 0,
            .cpcs  = 1,
            .cpbs  = 0,
            .cpas  = 0,
            .lovrs = 0,
            .covfs = 0,
        };
    #endif /* 若 ( configTICK_USE_TC == 1 ) */

    /* 禁止所有中断/异常。 */
    portDISABLE_INTERRUPTS();

    /*
     * 向中断控制器注册比较中断处理程序，并使能比较中断。
     */

    #if ( configTICK_USE_TC == 1 )
    {
        INTC_register_interrupt( &vTick, configTICK_TC_IRQ, INT0 );

        /* 初始化定时器/计数器。 */
        tc_init_waveform( tc, &waveform_opt );

        /*
         * 设置比较触发。
         * 注意 TC 计数器为 16 位，因此无法按秒计数！
         * 这就是将其配置为按毫秒计数的原因。
         */
        tc_write_rc( tc, configTICK_TC_CHANNEL, ( configPBA_CLOCK_HZ / 4 ) / configTICK_RATE_HZ );

        tc_configure_interrupts( tc, configTICK_TC_CHANNEL, &tc_interrupt );

        /* 启动定时器/计数器。 */
        tc_start( tc, configTICK_TC_CHANNEL );
    }
    #else /* 若 ( configTICK_USE_TC == 1 ) */
    {
        INTC_register_interrupt( &vTick, AVR32_CORE_COMPARE_IRQ, INT0 );
        prvScheduleFirstTick();
    }
    #endif /* 若 ( configTICK_USE_TC == 1 ) */
}
