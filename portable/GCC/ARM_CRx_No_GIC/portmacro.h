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
#define portMAX_DELAY    ( TickType_t ) 0xffffffffUL

/*
 * 32 位架构上使用 32 位时钟节拍类型，因此读取节拍计数
 * 无需用临界区保护。
 */
#define portTICK_TYPE_IS_ATOMIC    1

/*-----------------------------------------------------------*/

/* 硬件相关定义。 */
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT    8

/*-----------------------------------------------------------*/

/* 任务相关工具宏。 */

/* 在可能引发上下文切换的中断服务程序（ISR）末尾调用。 */
#define portEND_SWITCHING_ISR( xSwitchRequired )      \
    {                                                 \
        extern volatile uint32_t ulPortYieldRequired; \
                                                      \
        if( xSwitchRequired != pdFALSE )              \
        {                                             \
            ulPortYieldRequired = pdTRUE;             \
        }                                             \
    }

#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )

void vPortYield( void );

#define portYIELD()     vPortYield();

/*-----------------------------------------------------------*/

/*
 * 临界区管理。
 */

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern void vPortEnableInterrupts( void );
extern void vPortDisableInterrupts( void );
extern uint32_t ulPortSetInterruptMaskFromISR( void );

/*
 * 在没有优先级屏蔽寄存器的情况下，这些函数和宏
 * 通过全局使能/禁止中断来实现临界区。
 */
#define portENTER_CRITICAL()                    vPortEnterCritical();
#define portEXIT_CRITICAL()                     vPortExitCritical();
#define portENABLE_INTERRUPTS()                 vPortEnableInterrupts();
#define portDISABLE_INTERRUPTS()                vPortDisableInterrupts();
#define portSET_INTERRUPT_MASK_FROM_ISR()       ulPortSetInterruptMaskFromISR();
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )  do { if( x == 0 ) portENABLE_INTERRUPTS(); } while( 0 )

/*-----------------------------------------------------------*/

/*
 * FreeRTOS.org 网站上所述的任务函数宏。本移植层并不需要它们，
 * 但保留下来以便使用这些宏的通用示例代码仍可编译。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/* 低功耗无节拍空闲（Tickless Idle）功能。 */
#ifndef portSUPPRESS_TICKS_AND_SLEEP
    extern void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );
    #define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime )    vPortSuppressTicksAndSleep( xExpectedIdleTime )
#endif

/*
 * FreeRTOS 时钟节拍处理程序的原型。必须将该处理程序安装为产生
 * RTOS 节拍的外设所对应的中断处理程序。
 */
void FreeRTOS_Tick_Handler( void );

/**
 * @brief 返回 32 位变量中前导零的个数。
 *
 * @param[in] ulBitmap 要统计前导零个数的 32 位数。
 *
 * @return ulBitmap 中前导零的个数。
 */
UBaseType_t ulPortCountLeadingZeros( UBaseType_t ulBitmap );

/*
 * 若 configUSE_TASK_FPU_SUPPORT 设为 1（或未定义），任务创建时
 * 不带 FPU 上下文，必须先调用 vPortTaskUsesFPU() 获得 FPU 上下文，
 * 才能使用任何 FPU 指令。若 configUSE_TASK_FPU_SUPPORT 设为 2，
 * 则所有任务默认都具有 FPU 上下文。
 */
#if ( configUSE_TASK_FPU_SUPPORT != 2 )
    void vPortTaskUsesFPU( void );
#else
    /*
     * 每个任务都已具有 FPU 上下文，因此将该函数定义为
     * 空操作。
     */
    #define vPortTaskUsesFPU()
#endif
#define portTASK_USES_FLOATING_POINT()    vPortTaskUsesFPU()

#define portLOWEST_INTERRUPT_PRIORITY           ( ( ( uint32_t ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define portLOWEST_USABLE_INTERRUPT_PRIORITY    ( portLOWEST_INTERRUPT_PRIORITY - 1UL )

/* 体系结构相关的优化。 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

    /* 在位图中存储、清除和获取就绪优先级。 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )  uxTopPriority = ( 31UL - ulPortCountLeadingZeros( ( uxTopReadyPriority ) ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

#define portNOP()               __asm volatile ( "NOP" )
#define portINLINE              __inline
#define portMEMORY_BARRIER()    __asm volatile ( "" ::: "memory" )

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：PORTMACRO_H */
