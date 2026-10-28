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

#ifndef __SECURE_PORT_MACROS_H__
#define __SECURE_PORT_MACROS_H__

/**
 * @brief 字节对齐要求。
 */
#define secureportBYTE_ALIGNMENT         8
#define secureportBYTE_ALIGNMENT_MASK    ( 0x0007 )

/**
 * @brief 将函数声明为非安全可调用（non-secure callable）的宏。
 */
#if defined( __IAR_SYSTEMS_ICC__ )
    #define secureportNON_SECURE_CALLABLE    __cmse_nonsecure_entry __root
#else
    #define secureportNON_SECURE_CALLABLE    __attribute__( ( cmse_nonsecure_entry ) ) __attribute__( ( used ) )
#endif

/**
 * @brief 设置安全侧 PRIMASK 值。
 */
#define secureportSET_SECURE_PRIMASK( ulPrimaskValue ) \
    __asm volatile ( "msr primask, %0" : : "r" ( ulPrimaskValue ) : "memory" )

/**
 * @brief 设置非安全侧 PRIMASK 值。
 */
#define secureportSET_NON_SECURE_PRIMASK( ulPrimaskValue ) \
    __asm volatile ( "msr primask_ns, %0" : : "r" ( ulPrimaskValue ) : "memory" )

/**
 * @brief 将 PSP 的值读入指定变量。
 */
#define secureportREAD_PSP( pucOutCurrentStackPointer ) \
    __asm volatile ( "mrs %0, psp"  : "=r" ( pucOutCurrentStackPointer ) )

/**
 * @brief 将 PSP 设置为给定值。
 */
#define secureportSET_PSP( pucCurrentStackPointer ) \
    __asm volatile ( "msr psp, %0" : : "r" ( pucCurrentStackPointer ) )

/**
 * @brief 将 PSPLIM 的值读入指定变量。
 */
#define secureportREAD_PSPLIM( pucOutStackLimit ) \
    __asm volatile ( "mrs %0, psplim"  : "=r" ( pucOutStackLimit ) )

/**
 * @brief 将 PSPLIM 设置为给定值。
 */
#define secureportSET_PSPLIM( pucStackLimit ) \
    __asm volatile ( "msr psplim, %0" : : "r" ( pucStackLimit ) )

/**
 * @brief 将非安全侧 MSP 设置为给定值。
 */
#define secureportSET_MSP_NS( pucMainStackPointer ) \
    __asm volatile ( "msr msp_ns, %0" : : "r" ( pucMainStackPointer ) )

/**
 * @brief 将 CONTROL 寄存器设置为给定值。
 */
#define secureportSET_CONTROL( ulControl ) \
    __asm volatile ( "msr control, %0" : : "r" ( ulControl ) : "memory" )

/**
 * @brief 将中断程序状态寄存器（IPSR）的值读入指定变量。
 */
#define secureportREAD_IPSR( ulIPSR ) \
    __asm volatile ( "mrs %0, ipsr"  : "=r" ( ulIPSR ) )

/**
 * @brief 使能中断时使用的 PRIMASK 值。
 */
#define secureportPRIMASK_ENABLE_INTERRUPTS_VAL     0

/**
 * @brief 禁止中断时使用的 PRIMASK 值。
 */
#define secureportPRIMASK_DISABLE_INTERRUPTS_VAL    1

/**
 * @brief 禁止安全侧中断。
 */
#define secureportDISABLE_SECURE_INTERRUPTS()        secureportSET_SECURE_PRIMASK( secureportPRIMASK_DISABLE_INTERRUPTS_VAL )

/**
 * @brief 禁止非安全侧中断。
 *
 * 这会实际禁止上下文切换。
 */
#define secureportDISABLE_NON_SECURE_INTERRUPTS()    secureportSET_NON_SECURE_PRIMASK( secureportPRIMASK_DISABLE_INTERRUPTS_VAL )

/**
 * @brief 使能非安全侧中断。
 */
#define secureportENABLE_NON_SECURE_INTERRUPTS()     secureportSET_NON_SECURE_PRIMASK( secureportPRIMASK_ENABLE_INTERRUPTS_VAL )

/**
 * @brief 断言定义。
 */
#define secureportASSERT( x )                      \
    if( ( x ) == 0 )                               \
    {                                              \
        secureportDISABLE_SECURE_INTERRUPTS();     \
        secureportDISABLE_NON_SECURE_INTERRUPTS(); \
        for( ; ; ) {; }                            \
    }

#endif /* __SECURE_PORT_MACROS_H__ 头文件保护结束 */
