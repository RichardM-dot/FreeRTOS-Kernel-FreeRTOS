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

#ifndef __SECURE_INIT_H__
#define __SECURE_INIT_H__

/**
 * @brief 降低非安全异常的优先级。
 *
 * 这是为了确保非安全 PendSV 以最低优先级运行。上下文切换在非安全 PendSV 处理程序中完成。
 *
 * @note 此函数必须在 Handler 模式下调用。如果在 Thread 模式下调用，则不执行任何操作。
 */
void SecureInit_DePrioritizeNSExceptions( void );

/**
 * @brief 将浮点单元（FPU）设置为允许非安全访问。
 *
 * 同时设置 FPCCR.TS=1，以确保浮点寄存器的内容不会泄漏到非安全侧。
 *
 * @note 此函数必须在 Handler 模式下调用。如果在 Thread 模式下调用，则不执行任何操作。
 */
void SecureInit_EnableNSFPUAccess( void );

#endif /* __SECURE_INIT_H__（头文件保护宏） */
