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

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进） */

/* 系统头文件包含。 */
#include <tc1782.h>
#include <machine/intrinsics.h>

/*
 * -----------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置使 FreeRTOS 针对给定的硬件和编译器正确配置。
 *
 * 这些设置不应被修改。
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

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffffffffUL

/*
 * 32 位体系结构上使用 32 位 tick 类型，因此读取 tick 计数无需用临界区保护。
 */
    #define portTICK_TYPE_IS_ATOMIC    1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*---------------------------------------------------------------------------*/

/* 体系结构相关定义。 */
#define portSTACK_GROWTH                         ( -1 )
#define portTICK_PERIOD_MS                       ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT                       4
#define portNOP()    __asm volatile ( " nop " )
#define portCRITICAL_NESTING_IN_TCB              1
#define portRESTORE_FIRST_TASK_PRIORITY_LEVEL    1


/*---------------------------------------------------------------------------*/

typedef struct MPU_SETTINGS
{
    uint32_t ulNotUsed;
} xMPU_SETTINGS;

/* 将恢复上下文宏中的该指令定义为空。 */
#define portPRIVILEGE_BIT    0x0UL

#define portCCPN_MASK        ( 0x000000FFUL )

extern void vTaskEnterCritical( void );
extern void vTaskExitCritical( void );
#define portENTER_CRITICAL()    vTaskEnterCritical()
#define portEXIT_CRITICAL()     vTaskExitCritical()
/*---------------------------------------------------------------------------*/

/* CSA 操作。 */
#define portCSA_TO_ADDRESS( pCSA )        ( ( uint32_t * ) ( ( ( ( pCSA ) & 0x000F0000 ) << 12 ) | ( ( ( pCSA ) & 0x0000FFFF ) << 6 ) ) )
#define portADDRESS_TO_CSA( pAddress )    ( ( uint32_t ) ( ( ( ( ( uint32_t ) ( pAddress ) ) & 0xF0000000 ) >> 12 ) | ( ( ( uint32_t ) ( pAddress ) & 0x003FFFC0 ) >> 6 ) ) )
/*---------------------------------------------------------------------------*/

#define portYIELD()                       _syscall( 0 )
/* 从函数返回时，移植层的恢复操作由平台隐式完成：原 PSW 会被自动替换。 */
#define portSYSCALL_TASK_YIELD        0
#define portSYSCALL_RAISE_PRIORITY    1
/*---------------------------------------------------------------------------*/

/* 临界区管理。 */

/* 将 ICR.CCPN 设置为 configMAX_SYSCALL_INTERRUPT_PRIORITY。 */
#define portDISABLE_INTERRUPTS()                                                                      \
    {                                                                                                 \
        uint32_t ulICR;                                                                               \
        _disable();                                                                                   \
        ulICR = __MFCR( $ICR );                        /* Get current ICR value. */                   \
        ulICR &= ~portCCPN_MASK;                       /* Clear down mask bits. */                    \
        ulICR |= configMAX_SYSCALL_INTERRUPT_PRIORITY; /* Set mask bits to required priority mask. */ \
        _mtcr( $ICR, ulICR );                          /* Write back updated ICR. */                  \
        _isync();                                                                                     \
        _enable();                                                                                    \
    }

/* 清除 ICR.CCPN，以允许所有中断优先级。 */
#define portENABLE_INTERRUPTS()                                \
    {                                                          \
        uint32_t ulICR;                                        \
        _disable();                                            \
        ulICR = __MFCR( $ICR );  /* Get current ICR value. */  \
        ulICR &= ~portCCPN_MASK; /* Clear down mask bits. */   \
        _mtcr( $ICR, ulICR );    /* Write back updated ICR. */ \
        _isync();                                              \
        _enable();                                             \
    }

/* 将 ICR.CCPN 设置为 uxSavedMaskValue。 */
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedMaskValue )                          \
    {                                                                                  \
        uint32_t ulICR;                                                                \
        _disable();                                                                    \
        ulICR = __MFCR( $ICR );    /* Get current ICR value. */                        \
        ulICR &= ~portCCPN_MASK;   /* Clear down mask bits. */                         \
        ulICR |= uxSavedMaskValue; /* Set mask bits to previously saved mask value. */ \
        _mtcr( $ICR, ulICR );      /* Write back updated ICR. */                       \
        _isync();                                                                      \
        _enable();                                                                     \
    }


/* 将 ICR.CCPN 设置为 configMAX_SYSCALL_INTERRUPT_PRIORITY */
extern uint32_t uxPortSetInterruptMaskFromISR( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()                 uxPortSetInterruptMaskFromISR()

/* 挂起一个优先级 1 中断，由它处理上下文切换。 */
#define portYIELD_FROM_ISR( xHigherPriorityTaskWoken )    do { if( xHigherPriorityTaskWoken != pdFALSE ) { CPU_SRC0.bits.SETR = 1; _isync(); } } while( 0 )

/*---------------------------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
/*---------------------------------------------------------------------------*/

/*
 * 移植层专用的清理宏，用于释放已被删除任务所占用的 CSA。
 */
void vPortReclaimCSA( uint32_t * pxTCB );
#define portCLEAN_UP_TCB( pxTCB )    vPortReclaimCSA( ( uint32_t * ) ( pxTCB ) )

/* *INDENT-OFF*（关闭自动缩进） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进） */

#endif /* PORTMACRO_H（头文件保护宏） */
