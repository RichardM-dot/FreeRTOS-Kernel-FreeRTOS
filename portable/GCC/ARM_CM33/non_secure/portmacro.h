/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
 * ------------------------------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置针对给定硬件和编译器正确地配置 FreeRTOS。
 *
 * 不应修改这些设置。
 * ------------------------------------------------------------------------------
 */

/**
 * 架构相关定义。
 */
#define portARCH_NAME                    "Cortex-M33"
#define portHAS_ARMV8M_MAIN_EXTENSION    1
#define portARMV8M_MINOR_VERSION         0
#define portDONT_DISCARD                 __attribute__( ( used ) )
#define portVALIDATED_FOR_SMP            0
/*-----------------------------------------------------------*/

/* ARMv8-M 通用移植层配置。 */
#include "portmacrocommon.h"
/*-----------------------------------------------------------*/

#ifndef configENABLE_MVE
    #define configENABLE_MVE    0
#elif ( configENABLE_MVE != 0 )
    #error configENABLE_MVE must be left undefined, or defined to 0 for the Cortex-M33.
#endif
/*-----------------------------------------------------------*/

/**
 * @brief 临界区管理。
 */
#define portDISABLE_INTERRUPTS()    ulSetInterruptMask()
#define portENABLE_INTERRUPTS()     vClearInterruptMask( 0 )
/*-----------------------------------------------------------*/

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* PORTMACRO_H（头文件保护宏） */
