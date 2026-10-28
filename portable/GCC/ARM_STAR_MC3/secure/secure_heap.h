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

#ifndef __SECURE_HEAP_H__
#define __SECURE_HEAP_H__

/* 标准头文件包含。 */
#include <stdlib.h>

/**
 * @brief 从堆中分配存储器。
 *
 * @param[in] xWantedSize 要分配的存储器大小。
 *
 * @return 若分配成功则返回指向存储器区域的指针，否则返回 NULL。
 */
void * pvPortMalloc( size_t xWantedSize );

/**
 * @brief 释放先前分配的存储器。
 *
 * @param[in] pv 指向要释放的存储器的指针。
 */
void vPortFree( void * pv );

/**
 * @brief 获取空闲堆大小。
 *
 * @return 空闲堆大小。
 */
size_t xPortGetFreeHeapSize( void );

/**
 * @brief 获取历史最小空闲堆大小。
 *
 * @return 历史最小空闲堆大小。
 */
size_t xPortGetMinimumEverFreeHeapSize( void );

#endif /* 头文件保护：__SECURE_HEAP_H__ */
