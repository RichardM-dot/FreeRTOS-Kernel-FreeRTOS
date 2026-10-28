/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright (c) 2026 Arm Technology (China) Co., Ltd.All Rights Reserved.
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
 * 本文件中的设置使 FreeRTOS 针对给定硬件和编译器正确配置。
 *
 * 不应修改这些设置。
 * ------------------------------------------------------------------------------
 */

#ifndef configENABLE_MVE
    #error configENABLE_MVE must be defined in FreeRTOSConfig.h. Set configENABLE_MVE to 1 to enable the MVE or 0 to disable the MVE.
#endif /* configENABLE_MVE 条件编译结束 */
/*-----------------------------------------------------------*/

/**
 * 体系结构相关定义。
 */
#define portARCH_NAME                    "STAR-MC3"
#define portHAS_ARMV8M_MAIN_EXTENSION    1
#define portARMV8M_MINOR_VERSION         1
#define portDONT_DISCARD                 __attribute__( ( used ) )
#define portVALIDATED_FOR_SMP            0
/*-----------------------------------------------------------*/

/* ARMv8-M 通用移植层配置。 */
#include "portmacrocommon.h"
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

#endif /* 头文件保护：PORTMACRO_H */
