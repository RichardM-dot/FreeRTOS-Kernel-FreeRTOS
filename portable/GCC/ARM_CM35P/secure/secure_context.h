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

#ifndef __SECURE_CONTEXT_H__
#define __SECURE_CONTEXT_H__

/* 标准头文件。 */
#include <stdint.h>

/* FreeRTOS 相关头文件。 */
#include "FreeRTOSConfig.h"

/**
 * @brief 未加载安全上下文时的 PSP 值。
 */
#define securecontextNO_STACK              0x0

/**
 * @brief 无效的上下文 ID。
 */
#define securecontextINVALID_CONTEXT_ID    0UL
/*-----------------------------------------------------------*/

/**
 * @brief 表示安全上下文的结构体。
 *
 * @note 由于栈向下增长，pucStackStart 是最高地址，而 pucStackLimit 是
 * 所分配内存的首地址。
 */
typedef struct SecureContext
{
    uint8_t * pucCurrentStackPointer; /**< 栈指针（PSP）的当前值。 */
    uint8_t * pucStackLimit;          /**< 栈内存的最后位置（PSPLIM）。 */
    uint8_t * pucStackStart;          /**< 栈内存的起始位置。 */
    void * pvTaskHandle;              /**< 此上下文所关联任务的句柄。 */
} SecureContext_t;
/*-----------------------------------------------------------*/

/**
 * @brief 安全上下文的不透明句柄。
 */
typedef uint32_t SecureContextHandle_t;
/*-----------------------------------------------------------*/

/**
 * @brief 初始化安全上下文管理系统。
 *
 * PSP 被设置为 NULL，因此任务必须先分配并加载上下文，才能在线程模式下
 * 调用任何安全侧函数。
 *
 * @note 本函数必须在处理程序模式下调用；在线程模式下调用时不执行任何操作。
 */
void SecureContext_Init( void );

/**
 * @brief 在安全侧分配上下文。
 *
 * @note 本函数必须在处理程序模式下调用；在线程模式下调用时不执行任何操作。
 *
 * @param[in] ulSecureStackSize 在安全侧分配的栈大小。
 * @param[in] ulIsTaskPrivileged 调用任务具有特权时为 1，否则为 0。
 *
 * @return 成功分配上下文时返回不透明上下文句柄，否则返回 NULL。
 */
#if ( configENABLE_MPU == 1 )
    SecureContextHandle_t SecureContext_AllocateContext( uint32_t ulSecureStackSize,
                                                         uint32_t ulIsTaskPrivileged,
                                                         void * pvTaskHandle );
#else /* configENABLE_MPU（使能 MPU） */
    SecureContextHandle_t SecureContext_AllocateContext( uint32_t ulSecureStackSize,
                                                         void * pvTaskHandle );
#endif /* configENABLE_MPU（使能 MPU） */

/**
 * @brief 释放给定的上下文。
 *
 * @note 本函数必须在处理程序模式下调用；在线程模式下调用时不执行任何操作。
 *
 * @param[in] xSecureContextHandle 与待释放上下文对应的上下文句柄。
 */
void SecureContext_FreeContext( SecureContextHandle_t xSecureContextHandle,
                                void * pvTaskHandle );

/**
 * @brief 加载给定的上下文。
 *
 * @note 本函数必须在处理程序模式下调用；在线程模式下调用时不执行任何操作。
 *
 * @param[in] xSecureContextHandle 与待加载上下文对应的上下文句柄。
 */
void SecureContext_LoadContext( SecureContextHandle_t xSecureContextHandle,
                                void * pvTaskHandle );

/**
 * @brief 保存给定的上下文。
 *
 * @note 本函数必须在处理程序模式下调用；在线程模式下调用时不执行任何操作。
 *
 * @param[in] xSecureContextHandle 与待保存上下文对应的上下文句柄。
 */
void SecureContext_SaveContext( SecureContextHandle_t xSecureContextHandle,
                                void * pvTaskHandle );

#endif /* __SECURE_CONTEXT_H__（头文件保护宏） */
