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

#ifndef PORTMACRO_H
#define PORTMACRO_H

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

/*
 * -----------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置使 FreeRTOS 针对给定硬件和编译器正确配置。
 *
 * 不应修改这些设置。
 * -----------------------------------------------------------
 */

/* 类型定义。 */
#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE     long

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;

typedef uint32_t         TickType_t;
#define portMAX_DELAY    ( ( TickType_t ) 0xffffffffUL )

/*-----------------------------------------------------------*/

/* 硬件相关细节。 */
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT    32

/*-----------------------------------------------------------*/

/* 任务工具。 */

/*
 * 中断优先级（向量 16 到 255）通过 vector/16 确定。
 * 商四舍五入到最接近的整数，1 为最低优先级，15 为最高优先级。
 * 因此下面两个中断处于最低优先级。*注意 1* 若更改让出向量，
 * 还必须更改紧随其后的 portYIELD_INTERRUPT 定义中的对应值。
 */
#define portAPIC_TIMER_INT_VECTOR       ( 0x21 )
#define portAPIC_YIELD_INT_VECTOR       ( 0x20 )

/* 构造用于让出 CPU 的中断指令。 */
#define portYIELD_INTERRUPT             "int $0x20"

/* APIC 寄存器地址。 */
#define portAPIC_EOI                    ( *( ( volatile uint32_t * ) 0xFEE000B0UL ) )

/* APIC 位定义。 */
#define portAPIC_ENABLE_BIT             ( 1UL << 8UL )
#define portAPIC_TIMER_PERIODIC         ( 1UL << 17UL )
#define portAPIC_DISABLE                ( 1UL << 16UL )
#define portAPIC_NMI                    ( 4 << 8 )
#define portAPIC_DIV_16                 ( 0x03 )

/* 定义本地 API 寄存器地址。 */
#define portAPIC_ID_REGISTER            ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x20UL ) ) )
#define portAPIC_SPURIOUS_INT           ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0xF0UL ) ) )
#define portAPIC_LVT_TIMER              ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x320UL ) ) )
#define portAPIC_TIMER_INITIAL_COUNT    ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x380UL ) ) )
#define portAPIC_TIMER_CURRENT_COUNT    ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x390UL ) ) )
#define portAPIC_TASK_PRIORITY          ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x80UL ) ) )
#define portAPIC_LVT_ERROR              ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x370UL ) ) )
#define portAPIC_ERROR_STATUS           ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x280UL ) ) )
#define portAPIC_LDR                    ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0xD0UL ) ) )
#define portAPIC_TMRDIV                 ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x3E0UL ) ) )
#define portAPIC_LVT_PERF               ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x340UL ) ) )
#define portAPIC_LVT_LINT0              ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x350UL ) ) )
#define portAPIC_LVT_LINT1              ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0x360UL ) ) )

/*
 * 如果处于临界区内则不要让出 CPU，而是将让出请求挂起，
 * 待退出临界区时再执行。
 */
#define portYIELD()                                  \
    {                                                \
        extern volatile uint32_t ulCriticalNesting;  \
        extern volatile uint32_t ulPortYieldPending; \
        if( ulCriticalNesting != 0 )                 \
        {                                            \
            ulPortYieldPending = pdTRUE;             \
        }                                            \
        else                                         \
        {                                            \
            __asm volatile ( portYIELD_INTERRUPT );  \
        }                                            \
    }

/*
 * 可在中断中引发上下文切换的 ISR 结束处调用；
 * 若 xSwitchRequired 不为 false，则挂起一次让出 CPU 请求。
 */
#define portEND_SWITCHING_ISR( xSwitchRequired )     \
    {                                                \
        extern volatile uint32_t ulPortYieldPending; \
        if( xSwitchRequired != pdFALSE )             \
        {                                            \
            ulPortYieldPending = 1;                  \
        }                                            \
    }

/* 与 portEND_SWITCHING_ISR() 相同——可任选一个名称使用。 */
#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )

/*
 * -----------------------------------------------------------
 * 临界区控制
 * ----------------------------------------------------------
 */

/* 供中断中使用的临界区。 */
#define portSET_INTERRUPT_MASK_FROM_ISR()         ulPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vPortClearInterruptMask( x )

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern uint32_t ulPortSetInterruptMask( void );
extern void vPortClearInterruptMask( uint32_t ulNewMaskValue );

/*
 * 这些宏不会全局禁用/使能中断。它们屏蔽优先级低于 configMAX_API_CALL_INTERRUPT_PRIORITY 的中断。
 */
#define portENTER_CRITICAL()        vPortEnterCritical()
#define portEXIT_CRITICAL()         vPortExitCritical()
#define portDISABLE_INTERRUPTS()    __asm volatile ( "cli" )
#define portENABLE_INTERRUPTS()     __asm volatile ( "sti" )

/*-----------------------------------------------------------*/

/*
 * FreeRTOS.org 网站上所述的任务函数宏。本移植层并不需要这些宏，
 * 但为兼容使用这些宏的通用演示代码而包含它们。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/* 架构相关优化。 */
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

/* 用位图保存/清除就绪优先级。 */
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) \
    __asm volatile ( "bsr %1, %0\n\t"                                    \
                     : "=r" ( uxTopPriority ) : "rm" ( uxReadyPriorities ) : "cc" )

    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION（使用移植层优化的任务选择） */

#define portNOP()    __asm volatile ( "NOP" )

/*
 * -----------------------------------------------------------
 * 杂项
 * ----------------------------------------------------------
 */

#define portNUM_VECTORS     256
#define portMAX_PRIORITY    15
typedef void ( * ISR_Handler_t ) ( void );

/*
 * 任何使用浮点单元的任务都必须先调用 vPortTaskUsesFPU()，
 * 然后才能执行浮点指令。
 */
#ifndef configSUPPORT_FPU
    #define configSUPPORT_FPU    0
#endif

#if configSUPPORT_FPU == 1
    void vPortTaskUsesFPU( void );
    #define portTASK_USES_FLOATING_POINT()    vPortTaskUsesFPU()
#endif

/*
 * 参见下面 configUSE_COMMON_INTERRUPT_ENTRY_POINT 定义处的注释。
 */
BaseType_t xPortRegisterCInterruptHandler( ISR_Handler_t pxHandler,
                                           uint32_t ulVectorNumber );
BaseType_t xPortInstallInterruptHandler( ISR_Handler_t pxHandler,
                                         uint32_t ulVectorNumber );

#ifndef configAPIC_BASE

/*
 * configAPIC_BASE_ADDRESS 设置本地 APIC 的基地址。
 * 若该地址不是常量，可在 FreeRTOSConfig.h 中覆盖。
 */
    #define configAPIC_BASE    0xFEE00000UL
#endif

#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION

/*
 * FreeRTOS 调度算法负责选择进入运行态的任务。
 * configUSE_PORT_OPTIMISED_TASK_SELECTION 用于设置选择方式。
 *
 * 若 configUSE_PORT_OPTIMISED_TASK_SELECTION 设置为 0，则使用
 * C 语言编写的通用算法选择进入运行态的任务。这是最慢的方法，
 * 但该算法不限制可用的 RTOS 任务唯一优先级的最大数量。
 *
 * 若 configUSE_PORT_OPTIMISED_TASK_SELECTION 设置为 1，则使用
 * 单条汇编指令选择进入运行态的任务。这是最快的方法，
 * 但将 RTOS 任务唯一优先级的最大数量限制为 32
 * （同一任务优先级可以分配给任意数量的 RTOS 任务）。
 */
    #warning configUSE_PORT_OPTIMISED_TASK_SELECTION was not defined in FreeRTOSConfig.h and has been defaulted to 1
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#endif

#ifndef configUSE_COMMON_INTERRUPT_ENTRY_POINT

/*
 * 实现中断处理程序有两种方式：
 *
 *  1) 作为标准 C 函数——
 *
 *  仅当 configUSE_COMMON_INTERRUPT_ENTRY_POINT 设置为 1 时
 *  才能使用此方法。C 函数通过
 *  xPortRegisterCInterruptHandler() 安装。
 *
 *  这是两种方法中最简单的一种，但中断入口时间略长。
 *
 *  2) 使用汇编存根，将处理程序包裹在 FreeRTOS 的
 *     portFREERTOS_INTERRUPT_ENTRY 和 portFREERTOS_INTERRUPT_EXIT 宏中。
 *
 *  此方法始终可用。它比方法 1 稍复杂，
 *  但中断入口时间更短。
 */
    #warning configUSE_COMMON_INTERRUPT_ENTRY_POINT was not defined in FreeRTOSConfig.h and has been defaulted to 1.
    #define configUSE_COMMON_INTERRUPT_ENTRY_POINT    1
#endif

#ifndef configISR_STACK_SIZE

/*
 * 中断入口代码会将当前使用的栈切换到专用系统栈。
 *
 * configISR_STACK_SIZE 定义系统栈上可存储的 32 位值的数量，
 * 必须足够大，以容纳可能嵌套的中断栈帧。
 */

    #error configISR_STACK_SIZE was not defined in FreeRTOSConfig.h.
#endif

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY

/*
 * 中断安全的 FreeRTOS 函数（以 "FromISR" 结尾的函数）不得
 * 从优先级高于 configMAX_API_CALL_INTERRUPT_PRIORITY 所设值的中断中调用。
 */
    #warning configMAX_API_CALL_INTERRUPT_PRIORITY was not defined in FreeRTOSConfig.h and has been defaulted to 10
    #define configMAX_API_CALL_INTERRUPT_PRIORITY    10
#endif

#ifndef configSUPPORT_FPU
    #warning configSUPPORT_FPU was not defined in FreeRTOSConfig.h and has been defaulted to 0
    #define configSUPPORT_FPU    0
#endif

/*
 * 写入任务优先级寄存器的值，用于将中断屏蔽提升到可调用 FreeRTOS API 的最高水平。
 */
#define portAPIC_PRIORITY_SHIFT        ( 4UL )
#define portAPIC_MAX_SUB_PRIORITY      ( 0x0fUL )
#define portMAX_API_CALL_PRIORITY      ( ( configMAX_API_CALL_INTERRUPT_PRIORITY << portAPIC_PRIORITY_SHIFT ) | portAPIC_MAX_SUB_PRIORITY )

/*
 * 若从中断优先级高于系统调用最大中断优先级的中断中
 * 调用中断安全 FreeRTOS 函数，则触发断言。
 */
#define portAPIC_PROCESSOR_PRIORITY    ( *( ( volatile uint32_t * ) ( configAPIC_BASE + 0xA0UL ) ) )
#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()    configASSERT( ( portAPIC_PROCESSOR_PRIORITY ) <= ( portMAX_API_CALL_PRIORITY ) )

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* PORTMACRO_H（移植宏头文件） */
