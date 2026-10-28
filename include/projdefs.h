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

#ifndef PROJDEFS_H
#define PROJDEFS_H

/*
 * 定义任务函数必须遵循的原型。在此文件中定义，
 * 以确保在包含 portable.h 之前该类型已知。
 */
typedef void (* TaskFunction_t)( void * arg );

/*
 * 将毫秒时间转换为时钟节拍（tick）时间。如果此处的定义不适合
 * 应用，可在 FreeRTOSConfig.h 中用同名宏覆盖。
 */
#ifndef pdMS_TO_TICKS
    #define pdMS_TO_TICKS( xTimeInMs )    ( ( TickType_t ) ( ( ( uint64_t ) ( xTimeInMs ) * ( uint64_t ) configTICK_RATE_HZ ) / ( uint64_t ) 1000U ) )
#endif

/*
 * 将时钟节拍（tick）时间转换为毫秒时间。如果此处的定义不适合
 * 应用，可在 FreeRTOSConfig.h 中用同名宏覆盖。
 */
#ifndef pdTICKS_TO_MS
    #define pdTICKS_TO_MS( xTimeInTicks )    ( ( TickType_t ) ( ( ( uint64_t ) ( xTimeInTicks ) * ( uint64_t ) 1000U ) / ( uint64_t ) configTICK_RATE_HZ ) )
#endif

#define pdFALSE                                  ( ( BaseType_t ) 0 )
#define pdTRUE                                   ( ( BaseType_t ) 1 )
#define pdFALSE_SIGNED                           ( ( BaseType_t ) 0 )
#define pdTRUE_SIGNED                            ( ( BaseType_t ) 1 )
#define pdFALSE_UNSIGNED                         ( ( UBaseType_t ) 0 )
#define pdTRUE_UNSIGNED                          ( ( UBaseType_t ) 1 )

#define pdPASS                                   ( pdTRUE )
#define pdFAIL                                   ( pdFALSE )
#define errQUEUE_EMPTY                           ( ( BaseType_t ) 0 )
#define errQUEUE_FULL                            ( ( BaseType_t ) 0 )

/* FreeRTOS 错误定义。 */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY    ( -1 )
#define errQUEUE_BLOCKED                         ( -4 )
#define errQUEUE_YIELD                           ( -5 )

/* 用于基本数据损坏检查的宏。 */
#ifndef configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES
    #define configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES    0
#endif

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a5a5aUL
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a5a5a5a5a5a5aULL
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif

/*
 * 以下 errno 值由 FreeRTOS+ 组件使用，FreeRTOS 本身不使用。
 */
#define pdFREERTOS_ERRNO_NONE             0   /* 无错误 */
#define pdFREERTOS_ERRNO_ENOENT           2   /* 没有此文件或目录 */
#define pdFREERTOS_ERRNO_EINTR            4   /* 系统调用被中断 */
#define pdFREERTOS_ERRNO_EIO              5   /* I/O 错误 */
#define pdFREERTOS_ERRNO_ENXIO            6   /* 没有此设备或地址 */
#define pdFREERTOS_ERRNO_EBADF            9   /* 文件编号无效 */
#define pdFREERTOS_ERRNO_EAGAIN           11  /* 没有更多进程 */
#define pdFREERTOS_ERRNO_EWOULDBLOCK      11  /* 操作将阻塞 */
#define pdFREERTOS_ERRNO_ENOMEM           12  /* 内存不足 */
#define pdFREERTOS_ERRNO_EACCES           13  /* 权限不足 */
#define pdFREERTOS_ERRNO_EFAULT           14  /* 地址无效 */
#define pdFREERTOS_ERRNO_EBUSY            16  /* 挂载设备忙 */
#define pdFREERTOS_ERRNO_EEXIST           17  /* 文件已存在 */
#define pdFREERTOS_ERRNO_EXDEV            18  /* 跨设备链接 */
#define pdFREERTOS_ERRNO_ENODEV           19  /* 没有此设备 */
#define pdFREERTOS_ERRNO_ENOTDIR          20  /* 不是目录 */
#define pdFREERTOS_ERRNO_EISDIR           21  /* 是目录 */
#define pdFREERTOS_ERRNO_EINVAL           22  /* 无效参数 */
#define pdFREERTOS_ERRNO_ENOSPC           28  /* 设备上没有剩余空间 */
#define pdFREERTOS_ERRNO_ESPIPE           29  /* 非法定位（seek） */
#define pdFREERTOS_ERRNO_EROFS            30  /* 只读文件系统 */
#define pdFREERTOS_ERRNO_EUNATCH          42  /* 协议驱动程序未挂接 */
#define pdFREERTOS_ERRNO_EBADE            50  /* 无效交换 */
#define pdFREERTOS_ERRNO_EFTYPE           79  /* 文件类型或格式不合适 */
#define pdFREERTOS_ERRNO_ENMFILE          89  /* 没有更多文件 */
#define pdFREERTOS_ERRNO_ENOTEMPTY        90  /* 目录非空 */
#define pdFREERTOS_ERRNO_ENAMETOOLONG     91  /* 文件名或路径名过长 */
#define pdFREERTOS_ERRNO_EOPNOTSUPP       95  /* 传输端点不支持该操作 */
#define pdFREERTOS_ERRNO_EAFNOSUPPORT     97  /* 协议不支持该地址族 */
#define pdFREERTOS_ERRNO_ENOBUFS          105 /* 无可用缓冲区空间 */
#define pdFREERTOS_ERRNO_ENOPROTOOPT      109 /* 协议不可用 */
#define pdFREERTOS_ERRNO_EADDRINUSE       112 /* 地址已在使用 */
#define pdFREERTOS_ERRNO_ETIMEDOUT        116 /* 连接超时 */
#define pdFREERTOS_ERRNO_EINPROGRESS      119 /* 连接已在进行中 */
#define pdFREERTOS_ERRNO_EALREADY         120 /* 套接字已连接 */
#define pdFREERTOS_ERRNO_EADDRNOTAVAIL    125 /* 地址不可用 */
#define pdFREERTOS_ERRNO_EISCONN          127 /* 套接字已处于连接状态 */
#define pdFREERTOS_ERRNO_ENOTCONN         128 /* 套接字未连接 */
#define pdFREERTOS_ERRNO_ENOMEDIUM        135 /* 未插入介质 */
#define pdFREERTOS_ERRNO_EILSEQ           138 /* 遇到无效的 UTF-16 序列。 */
#define pdFREERTOS_ERRNO_ECANCELED        140 /* 操作已取消。 */

/*
 * 以下字节序值由 FreeRTOS+ 组件使用，FreeRTOS 本身不使用。
 */
#define pdFREERTOS_LITTLE_ENDIAN          0
#define pdFREERTOS_BIG_ENDIAN             1

/* 为通用命名重新定义字节序值。 */
#define pdLITTLE_ENDIAN                   pdFREERTOS_LITTLE_ENDIAN
#define pdBIG_ENDIAN                      pdFREERTOS_BIG_ENDIAN


#endif /* 头文件保护：PROJDEFS_H */
