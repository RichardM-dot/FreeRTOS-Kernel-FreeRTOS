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

#ifndef INC_PICOLIBC_FREERTOS_H
#define INC_PICOLIBC_FREERTOS_H

/*
 * 使用 picolibc TLS 支持为 __thread 变量分配空间，
 * 在线程创建时初始化它们，并在线程切换时设置 TLS 上下文。
 *
 * 更多信息请参见 picolibc TLS 文档：
 * https://github.com/picolibc/picolibc/blob/main/doc/tls.md。
 */

#include <picotls.h>

#define configUSE_C_RUNTIME_TLS_SUPPORT    1

#define configTLS_BLOCK_TYPE               void *

#define picolibcTLS_SIZE                   ( ( portPOINTER_SIZE_TYPE ) _tls_size() )
#define picolibcSTACK_ALIGNMENT_MASK       ( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK )

#if __PICOLIBC_MAJOR__ > 1 || __PICOLIBC_MINOR__ >= 8

/*
 * Picolibc 1.8 及更新版本通过 _tls_align() 内联函数
 * 提供显式的对齐值
 */
    #define picolibcTLS_ALIGNMENT_MASK    ( ( portPOINTER_SIZE_TYPE ) ( _tls_align() - 1 ) )
#else

/* 对于较早的 Picolibc 版本，使用移植层的通用对齐值 */
    #define picolibcTLS_ALIGNMENT_MASK    ( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK )
#endif

/*
 * 在栈末尾之外分配线程局部存储块。picolibcTLS_SIZE 宏返回
 * 应用程序所用 TLS 区域的总大小（字节）。
 * 计算栈顶地址。
 */
#if ( portSTACK_GROWTH < 0 )

    #define configINIT_TLS_BLOCK( xTLSBlock, pxTopOfStack )                                  \
    do {                                                                                     \
        xTLSBlock = ( void * ) ( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) -              \
                                   picolibcTLS_SIZE ) &                                      \
                                 ~picolibcTLS_ALIGNMENT_MASK );                              \
        pxTopOfStack = ( StackType_t * ) ( ( ( ( portPOINTER_SIZE_TYPE ) xTLSBlock ) - 1 ) & \
                                           ~picolibcSTACK_ALIGNMENT_MASK );                  \
        _init_tls( xTLSBlock );                                                              \
    } while( 0 )
#else /* 条件不成立：portSTACK_GROWTH */
    #define configINIT_TLS_BLOCK( xTLSBlock, pxTopOfStack )                                          \
    do {                                                                                             \
        xTLSBlock = ( void * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack +                          \
                                   picolibcTLS_ALIGNMENT_MASK ) & ~picolibcTLS_ALIGNMENT_MASK );     \
        pxTopOfStack = ( StackType_t * ) ( ( ( ( ( portPOINTER_SIZE_TYPE ) xTLSBlock ) +             \
                                               picolibcTLS_SIZE ) + picolibcSTACK_ALIGNMENT_MASK ) & \
                                           ~picolibcSTACK_ALIGNMENT_MASK );                          \
        _init_tls( xTLSBlock );                                                                      \
    } while( 0 )
#endif /* 条件编译结束：portSTACK_GROWTH */

#define configSET_TLS_BLOCK( xTLSBlock )    _set_tls( xTLSBlock )

#define configDEINIT_TLS_BLOCK( xTLSBlock )

#endif /* 头文件保护：INC_PICOLIBC_FREERTOS_H */
