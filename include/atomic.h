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

/**
 * @file atomic.h
 * @brief FreeRTOS 原子操作支持。
 *
 * 本文件通过全局禁用中断来实现原子函数。
 * 各编译器目录下可提供使用架构特定原子指令的实现。
 *
 * 原子接口可在所有 FreeRTOS 移植层的 FreeRTOS 任务中使用。
 * 在支持嵌套中断的 FreeRTOS 移植层（即 portHAS_NESTED_INTERRUPTS
 * 设为 1）中，也可在中断服务程序（ISR）中使用。在不支持嵌套中断的
 * 移植层（即 portHAS_NESTED_INTERRUPTS 设为 0）中，不得在 ISR 中
 * 使用原子接口，因为这些移植层的 ISR 不能被中断，因而无需在 ISR
 * 中使用原子操作。
 */

#ifndef ATOMIC_H
#define ATOMIC_H

#ifndef INC_FREERTOS_H
    #error "include FreeRTOS.h must appear in source files before include atomic.h"
#endif

/* 标准头文件包含。 */
#include <stdint.h>

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

/*
 * 移植层特定定义——进入/退出临界区。
 * 参考模板——./lib/FreeRTOS/portable/Compiler/Arch/portmacro.h
 *
 * 每次调用 ATOMIC_EXIT_CRITICAL() 都必须与
 * ATOMIC_ENTER_CRITICAL() 紧密配对。
 */
#if ( portHAS_NESTED_INTERRUPTS == 1 )

/* 此移植层支持嵌套中断方案。 */
    #define ATOMIC_ENTER_CRITICAL() \
    UBaseType_t uxCriticalSectionType = portSET_INTERRUPT_MASK_FROM_ISR()

    #define ATOMIC_EXIT_CRITICAL() \
    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxCriticalSectionType )

#else

/* 此移植层不支持嵌套中断方案。 */
    #define ATOMIC_ENTER_CRITICAL()    portENTER_CRITICAL()
    #define ATOMIC_EXIT_CRITICAL()     portEXIT_CRITICAL()

#endif /* 配置项 portSET_INTERRUPT_MASK_FROM_ISR() */

/*
 * 移植层特定定义——“始终内联”。
 * 内联是编译器相关的，并且可能因优化级别而未必真正内联。
 * 此外，内联被视为原子操作的性能优化手段。因此，如果
 * portmacro.h 未提供 portFORCE_INLINE，则将其定义为空，
 * 而不是产生错误。
 */
#ifndef portFORCE_INLINE
    #define portFORCE_INLINE
#endif

#define ATOMIC_COMPARE_AND_SWAP_SUCCESS    0x1U     /* 比较并交换成功，已完成交换。 */
#define ATOMIC_COMPARE_AND_SWAP_FAILURE    0x0U     /* 比较并交换失败，未进行交换。 */

/* ----------------------------- 交换与 CAS ------------------------------ */

/**
 * 原子比较并交换
 *
 * @brief 对指定值执行原子比较并交换操作。
 *
 * @param[in, out] pulDestination  指向内存位置的指针，将从该位置
 *                               加载并检查值。
 * @param[in] ulExchange         如果条件满足，则将此值写入内存。
 * @param[in] ulComparand        交换条件。
 *
 * @return 值为 1 或 0 的无符号整数。1 表示已交换，0 表示未交换。
 *
 * @note 仅当之前的 *pulDestination 值等于 ulComparand 时，
 *       此函数才会将 *pulDestination 与 ulExchange 交换。
 */
static portFORCE_INLINE uint32_t Atomic_CompareAndSwap_u32( uint32_t volatile * pulDestination,
                                                            uint32_t ulExchange,
                                                            uint32_t ulComparand )
{
    uint32_t ulReturnValue;

    ATOMIC_ENTER_CRITICAL();
    {
        if( *pulDestination == ulComparand )
        {
            *pulDestination = ulExchange;
            ulReturnValue = ATOMIC_COMPARE_AND_SWAP_SUCCESS;
        }
        else
        {
            ulReturnValue = ATOMIC_COMPARE_AND_SWAP_FAILURE;
        }
    }
    ATOMIC_EXIT_CRITICAL();

    return ulReturnValue;
}
/*-----------------------------------------------------------*/

/**
 * 原子交换（指针）
 *
 * @brief 以原子方式将 *ppvDestination 所指向的地址设置为
 *        *pvExchange 的值。
 *
 * @param[in, out] ppvDestination  指向内存位置的指针，将从该位置
 *                                 加载指针值并写回。
 * @param[in] pvExchange           要写入 *ppvDestination 的指针值。
 *
 * @return *ppvDestination 的初始值。
 */
static portFORCE_INLINE void * Atomic_SwapPointers_p32( void * volatile * ppvDestination,
                                                        void * pvExchange )
{
    void * pReturnValue;

    ATOMIC_ENTER_CRITICAL();
    {
        pReturnValue = *ppvDestination;
        *ppvDestination = pvExchange;
    }
    ATOMIC_EXIT_CRITICAL();

    return pReturnValue;
}
/*-----------------------------------------------------------*/

/**
 * 原子比较并交换（指针）
 *
 * @brief 对指定的指针值执行原子比较并交换操作。
 *
 * @param[in, out] ppvDestination  指向内存位置的指针，将从该位置
 *                                 加载并检查指针值。
 * @param[in] pvExchange          如果条件满足，则将此值写入内存。
 * @param[in] pvComparand         交换条件。
 *
 * @return 值为 1 或 0 的无符号整数。1 表示已交换，0 表示未交换。
 *
 * @note 仅当之前的 *ppvDestination 值等于 pvComparand 时，
 *       此函数才会将 *ppvDestination 与 pvExchange 交换。
 */
static portFORCE_INLINE uint32_t Atomic_CompareAndSwapPointers_p32( void * volatile * ppvDestination,
                                                                    void * pvExchange,
                                                                    void * pvComparand )
{
    uint32_t ulReturnValue = ATOMIC_COMPARE_AND_SWAP_FAILURE;

    ATOMIC_ENTER_CRITICAL();
    {
        if( *ppvDestination == pvComparand )
        {
            *ppvDestination = pvExchange;
            ulReturnValue = ATOMIC_COMPARE_AND_SWAP_SUCCESS;
        }
    }
    ATOMIC_EXIT_CRITICAL();

    return ulReturnValue;
}


/* ----------------------------- 算术运算 ------------------------------ */

/**
 * 原子加法
 *
 * @brief 以原子方式将 count 加到指定指针所指向的值上。
 *
 * @param[in,out] pulAddend  指向内存位置的指针，将从该位置加载值
 *                         并写回。
 * @param[in] ulCount      要加到 *pulAddend 的值。
 *
 * @return *pulAddend 的先前值。
 */
static portFORCE_INLINE uint32_t Atomic_Add_u32( uint32_t volatile * pulAddend,
                                                 uint32_t ulCount )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulAddend;
        *pulAddend += ulCount;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}
/*-----------------------------------------------------------*/

/**
 * 原子减法
 *
 * @brief 以原子方式从指定指针所指向的值中减去 count。
 *
 * @param[in,out] pulAddend  指向内存位置的指针，将从该位置加载值
 *                         并写回。
 * @param[in] ulCount      要从 *pulAddend 中减去的值。
 *
 * @return *pulAddend 的先前值。
 */
static portFORCE_INLINE uint32_t Atomic_Subtract_u32( uint32_t volatile * pulAddend,
                                                      uint32_t ulCount )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulAddend;
        *pulAddend -= ulCount;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}
/*-----------------------------------------------------------*/

/**
 * 原子自增
 *
 * @brief 以原子方式将指定指针所指向的值加 1。
 *
 * @param[in,out] pulAddend  指向内存位置的指针，将从该位置加载值
 *                         并写回。
 *
 * @return 自增前的 *pulAddend 值。
 */
static portFORCE_INLINE uint32_t Atomic_Increment_u32( uint32_t volatile * pulAddend )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulAddend;
        *pulAddend += 1;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}
/*-----------------------------------------------------------*/

/**
 * 原子自减
 *
 * @brief 以原子方式将指定指针所指向的值减 1。
 *
 * @param[in,out] pulAddend  指向内存位置的指针，将从该位置加载值
 *                         并写回。
 *
 * @return 自减前的 *pulAddend 值。
 */
static portFORCE_INLINE uint32_t Atomic_Decrement_u32( uint32_t volatile * pulAddend )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulAddend;
        *pulAddend -= 1;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}

/* ----------------------------- 按位逻辑运算 ------------------------------ */

/**
 * 原子 OR
 *
 * @brief 对指定值执行原子 OR 操作。
 *
 * @param [in, out] pulDestination  指向内存位置的指针，将从该位置
 *                               加载值并写回。
 * @param [in] ulValue           要与 *pulDestination 进行 OR 运算的值。
 *
 * @return *pulDestination 的原始值。
 */
static portFORCE_INLINE uint32_t Atomic_OR_u32( uint32_t volatile * pulDestination,
                                                uint32_t ulValue )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulDestination;
        *pulDestination |= ulValue;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}
/*-----------------------------------------------------------*/

/**
 * 原子 AND
 *
 * @brief 对指定值执行原子 AND 操作。
 *
 * @param [in, out] pulDestination  指向内存位置的指针，将从该位置
 *                               加载值并写回。
 * @param [in] ulValue           要与 *pulDestination 进行 AND 运算的值。
 *
 * @return *pulDestination 的原始值。
 */
static portFORCE_INLINE uint32_t Atomic_AND_u32( uint32_t volatile * pulDestination,
                                                 uint32_t ulValue )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulDestination;
        *pulDestination &= ulValue;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}
/*-----------------------------------------------------------*/

/**
 * 原子 NAND
 *
 * @brief 对指定值执行原子 NAND 操作。
 *
 * @param [in, out] pulDestination  指向内存位置的指针，将从该位置
 *                               加载值并写回。
 * @param [in] ulValue           要与 *pulDestination 进行 NAND 运算的值。
 *
 * @return *pulDestination 的原始值。
 */
static portFORCE_INLINE uint32_t Atomic_NAND_u32( uint32_t volatile * pulDestination,
                                                  uint32_t ulValue )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulDestination;
        *pulDestination = ~( ulCurrent & ulValue );
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}
/*-----------------------------------------------------------*/

/**
 * 原子 XOR
 *
 * @brief 对指定值执行原子 XOR 操作。
 *
 * @param [in, out] pulDestination  指向内存位置的指针，将从该位置
 *                               加载值并写回。
 * @param [in] ulValue           要与 *pulDestination 进行 XOR 运算的值。
 *
 * @return *pulDestination 的原始值。
 */
static portFORCE_INLINE uint32_t Atomic_XOR_u32( uint32_t volatile * pulDestination,
                                                 uint32_t ulValue )
{
    uint32_t ulCurrent;

    ATOMIC_ENTER_CRITICAL();
    {
        ulCurrent = *pulDestination;
        *pulDestination ^= ulValue;
    }
    ATOMIC_EXIT_CRITICAL();

    return ulCurrent;
}

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：ATOMIC_H */
