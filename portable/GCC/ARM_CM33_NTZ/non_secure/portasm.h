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

#ifndef __PORT_ASM_H__
#define __PORT_ASM_H__

/* 调度器相关头文件包含。 */
#include "FreeRTOS.h"

/* MPU 包装器相关头文件包含。 */
#include "mpu_wrappers.h"

/**
 * @brief 恢复第一个任务的上下文，使第一个任务开始执行。
 */
void vRestoreContextOfFirstTask( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief 检查处理器是否处于特权级。
 *
 * @return 若处理器已处于特权级则返回 1，否则返回 0。
 */
BaseType_t xIsPrivileged( void ) __attribute__( ( naked ) );

/**
 * @brief 通过清除 CONTROL 寄存器的位 0 来提升特权级。
 *
 * @note 这是一个特权函数，只应从内核代码中调用。
 *
 * CONTROL 寄存器的位 0 定义线程模式的特权级。
 * 位[0] = 0 --> 处理器以特权级运行
 * 位[0] = 1 --> 处理器以非特权级运行。
 */
void vRaisePrivilege( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief 通过设置 CONTROL 寄存器的位 0 来降低特权级。
 *
 * CONTROL 寄存器的位 0 定义线程模式的特权级。
 * 位[0] = 0 --> 处理器以特权级运行
 * 位[0] = 1 --> 处理器以非特权级运行。
 */
void vResetPrivilege( void ) __attribute__( ( naked ) );

/**
 * @brief 启动第一个任务。
 */
void vStartFirstTask( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief 禁止中断。
 */
uint32_t ulSetInterruptMask( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief 使能中断。
 */
void vClearInterruptMask( uint32_t ulMask ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief PendSV 异常处理程序。
 */
void PendSV_Handler( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief SVC 处理程序。
 */
void SVC_Handler( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/**
 * @brief 为调用任务分配安全上下文。
 *
 * @param[in] ulSecureStackSize 为调用任务在安全侧分配的栈大小。
 */
void vPortAllocateSecureContext( uint32_t ulSecureStackSize ) __attribute__( ( naked ) );

/**
 * @brief 释放任务的安全上下文。
 *
 * @param[in] pulTCB 指向该任务任务控制块（TCB）的指针。
 */
void vPortFreeSecureContext( uint32_t * pulTCB ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

#endif /* __PORT_ASM_H__（头文件保护宏） */
