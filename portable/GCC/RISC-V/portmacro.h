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
#if __riscv_xlen == 64
    #define portSTACK_TYPE           uint64_t
    #define portBASE_TYPE            int64_t
    #define portUBASE_TYPE           uint64_t
    #define portMAX_DELAY            ( TickType_t ) 0xffffffffffffffffUL
    #define portPOINTER_SIZE_TYPE    uint64_t
#elif __riscv_xlen == 32
    #define portSTACK_TYPE           uint32_t
    #define portBASE_TYPE            int32_t
    #define portUBASE_TYPE           uint32_t
    #define portMAX_DELAY            ( TickType_t ) 0xffffffffUL
#else /* if __riscv_xlen == 64 条件分支 */
    #error "Assembler did not define __riscv_xlen"
#endif /* if __riscv_xlen == 64 条件结束 */

typedef portSTACK_TYPE   StackType_t;
typedef portBASE_TYPE    BaseType_t;
typedef portUBASE_TYPE   UBaseType_t;
typedef portUBASE_TYPE   TickType_t;

/* 旧版类型定义。 */
#define portCHAR                   char
#define portFLOAT                  float
#define portDOUBLE                 double
#define portLONG                   long
#define portSHORT                  short

/*
 * 32 位架构上使用 32 位时钟节拍类型，因此读取节拍计数
 * 无需用临界区保护。
 */
#define portTICK_TYPE_IS_ATOMIC    1
/*-----------------------------------------------------------*/

/* 体系结构相关定义。 */
#define portSTACK_GROWTH          ( -1 )
#define portTICK_PERIOD_MS        ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#ifdef __riscv_32e
    #define portBYTE_ALIGNMENT    8     /* RV32E 使用 RISC-V EABI，栈对齐要求更低 */
#else
    #define portBYTE_ALIGNMENT    16
#endif
/*-----------------------------------------------------------*/

/* 调度器相关工具宏。 */
extern void vTaskSwitchContext( void );
#define portYIELD()                __asm volatile ( "ecall" );
#define portEND_SWITCHING_ISR( xSwitchRequired ) \
    do                                           \
    {                                            \
        if( xSwitchRequired != pdFALSE )         \
        {                                        \
            traceISR_EXIT_TO_SCHEDULER();        \
            vTaskSwitchContext();                \
        }                                        \
        else                                     \
        {                                        \
            traceISR_EXIT();                     \
        }                                        \
    } while( 0 )
#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )
/*-----------------------------------------------------------*/

/* 临界区管理。 */
#define portCRITICAL_NESTING_IN_TCB    0

#define portDISABLE_INTERRUPTS()                                   __asm volatile ( "csrc mstatus, 8" )
#define portENABLE_INTERRUPTS()                                    __asm volatile ( "csrs mstatus, 8" )

extern size_t xCriticalNesting;
#define portENTER_CRITICAL()      \
    {                             \
        portDISABLE_INTERRUPTS(); \
        xCriticalNesting++;       \
    }

#define portEXIT_CRITICAL()          \
    {                                \
        xCriticalNesting--;          \
        if( xCriticalNesting == 0 )  \
        {                            \
            portENABLE_INTERRUPTS(); \
        }                            \
    }

/*-----------------------------------------------------------*/

/* 体系结构相关的优化。 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#endif

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1 )

/* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error "configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice."
    #endif

/* 在位图中存储/清除就绪优先级。 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

/*-----------------------------------------------------------*/

    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )    uxTopPriority = ( 31UL - __builtin_clz( uxReadyPriorities ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */


/*-----------------------------------------------------------*/

/*
 * FreeRTOS.org 网站上所述的任务函数宏。使用本移植层并不需要它们，
 * 定义它们是为了让所有移植层通用的示例文件仍可构建。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

/*-----------------------------------------------------------*/

#define portNOP()    __asm volatile ( " nop " )
#define portINLINE              __inline

#ifndef portFORCE_INLINE
    #define portFORCE_INLINE    inline __attribute__( ( always_inline ) )
#endif

#define portMEMORY_BARRIER()    __asm volatile ( "" ::: "memory" )
/*-----------------------------------------------------------*/

/*
 * configCLINT_BASE_ADDRESS 是旧版定义，已被 configMTIME_BASE_ADDRESS
 * 和 configMTIMECMP_BASE_ADDRESS 取代。为保持向后兼容，若发现旧版定义，
 * 则据此推导新定义。
 */
#if defined( configCLINT_BASE_ADDRESS ) && !defined( configMTIME_BASE_ADDRESS ) && ( configCLINT_BASE_ADDRESS == 0 )

/*
 * 旧版用法将 configCLINT_BASE_ADDRESS 定义为 0，表示不存在 CLINT。
 * 现在的等效做法是将 MTIME 和 MTIMECMP 地址都设为 0。
 */
    #define configMTIME_BASE_ADDRESS       ( 0 )
    #define configMTIMECMP_BASE_ADDRESS    ( 0 )
#elif defined( configCLINT_BASE_ADDRESS ) && !defined( configMTIME_BASE_ADDRESS )

/*
 * 旧版用法将 configCLINT_BASE_ADDRESS 设为 CLINT 的基地址。
 * 现在的等效做法是根据 CLINT 地址推导 MTIME 和 MTIMECMP 地址。
 */
    #define configMTIME_BASE_ADDRESS       ( ( configCLINT_BASE_ADDRESS ) + 0xBFF8UL )
    #define configMTIMECMP_BASE_ADDRESS    ( ( configCLINT_BASE_ADDRESS ) + 0x4000UL )
#elif !defined( configMTIME_BASE_ADDRESS ) || !defined( configMTIMECMP_BASE_ADDRESS )
    #error "configMTIME_BASE_ADDRESS and configMTIMECMP_BASE_ADDRESS must be defined in FreeRTOSConfig.h.  Set them to zero if there is no MTIME (machine time) clock.  See www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html"
#endif /* configCLINT_BASE_ADDRESS 旧版兼容条件结束 */

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：PORTMACRO_H */
