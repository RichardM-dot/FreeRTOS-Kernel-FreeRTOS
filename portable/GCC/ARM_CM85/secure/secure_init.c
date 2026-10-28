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

/* 标准头文件包含。 */
#include <stdint.h>

/* 安全初始化头文件包含。 */
#include "secure_init.h"

/* 安全移植宏。 */
#include "secure_port_macros.h"

/**
 * @brief 操作 SCB 所需的常量。
 */
#define secureinitSCB_AIRCR                 ( ( volatile uint32_t * ) 0xe000ed0c )  /* 应用中断与复位控制寄存器。 */
#define secureinitSCB_AIRCR_VECTKEY_POS     ( 16UL )
#define secureinitSCB_AIRCR_VECTKEY_MASK    ( 0xFFFFUL << secureinitSCB_AIRCR_VECTKEY_POS )
#define secureinitSCB_AIRCR_PRIS_POS        ( 14UL )
#define secureinitSCB_AIRCR_PRIS_MASK       ( 1UL << secureinitSCB_AIRCR_PRIS_POS )

/**
 * @brief 操作 FPU 所需的常量。
 */
#define secureinitFPCCR                     ( ( volatile uint32_t * ) 0xe000ef34 )  /* 浮点上下文控制寄存器。 */
#define secureinitFPCCR_LSPENS_POS          ( 29UL )
#define secureinitFPCCR_LSPENS_MASK         ( 1UL << secureinitFPCCR_LSPENS_POS )
#define secureinitFPCCR_TS_POS              ( 26UL )
#define secureinitFPCCR_TS_MASK             ( 1UL << secureinitFPCCR_TS_POS )

#define secureinitNSACR                     ( ( volatile uint32_t * ) 0xe000ed8c )  /* 非安全访问控制寄存器。 */
#define secureinitNSACR_CP10_POS            ( 10UL )
#define secureinitNSACR_CP10_MASK           ( 1UL << secureinitNSACR_CP10_POS )
#define secureinitNSACR_CP11_POS            ( 11UL )
#define secureinitNSACR_CP11_MASK           ( 1UL << secureinitNSACR_CP11_POS )
/*-----------------------------------------------------------*/

secureportNON_SECURE_CALLABLE void SecureInit_DePrioritizeNSExceptions( void )
{
    uint32_t ulIPSR;

    /* 读取中断程序状态寄存器（IPSR）的值。 */
    secureportREAD_IPSR( ulIPSR );

    /*
     * 如果处理器运行在 Thread 模式，则不执行任何操作。处理器运行在 Thread 模式时，IPSR 为零。
     */
    if( ulIPSR != 0 )
    {
        *( secureinitSCB_AIRCR ) = ( *( secureinitSCB_AIRCR ) & ~( secureinitSCB_AIRCR_VECTKEY_MASK | secureinitSCB_AIRCR_PRIS_MASK ) ) |
                                   ( ( 0x05FAUL << secureinitSCB_AIRCR_VECTKEY_POS ) & secureinitSCB_AIRCR_VECTKEY_MASK ) |
                                   ( ( 0x1UL << secureinitSCB_AIRCR_PRIS_POS ) & secureinitSCB_AIRCR_PRIS_MASK );
    }
}
/*-----------------------------------------------------------*/

secureportNON_SECURE_CALLABLE void SecureInit_EnableNSFPUAccess( void )
{
    uint32_t ulIPSR;

    /* 读取中断程序状态寄存器（IPSR）的值。 */
    secureportREAD_IPSR( ulIPSR );

    /*
     * 如果处理器运行在 Thread 模式，则不执行任何操作。处理器运行在 Thread 模式时，IPSR 为零。
     */
    if( ulIPSR != 0 )
    {
        /*
         * CP10 = 1 ==> 允许非安全访问浮点单元。CP11 应编程为与 CP10 相同的值。
         */
        *( secureinitNSACR ) |= ( secureinitNSACR_CP10_MASK | secureinitNSACR_CP11_MASK );

        /*
         * LSPENS = 0 ==> LSPEN 可从非安全状态写入。这确保可以在 port.c 文件中启用或禁用惰性压栈（lazy stacking）。
         */
        *( secureinitFPCCR ) &= ~( secureinitFPCCR_LSPENS_MASK );

        /*
         * TS = 1 ==> 将 FP 寄存器视为安全，即在异常进入时，被调用者保存的 FP 寄存器（S16-S31）也会压入栈中，并在异常返回时恢复。
         */
        *( secureinitFPCCR ) |= ( secureinitFPCCR_TS_MASK );
    }
}
/*-----------------------------------------------------------*/
