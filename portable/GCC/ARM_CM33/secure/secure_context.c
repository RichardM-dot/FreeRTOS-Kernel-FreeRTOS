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

/* 安全上下文相关头文件包含。 */
#include "secure_context.h"

/* 安全堆相关头文件包含。 */
#include "secure_heap.h"

/* 安全移植层宏。 */
#include "secure_port_macros.h"

/**
 * @brief 特权任务的 CONTROL 值。
 *
 * 位[0] - 0 --> 线程模式为特权级。
 * 位[1] - 1 --> 线程模式使用 PSP。
 */
#define securecontextCONTROL_VALUE_PRIVILEGED      0x02

/**
 * @brief 非特权任务的 CONTROL 值。
 *
 * 位[0] - 1 --> 线程模式为非特权级。
 * 位[1] - 1 --> 线程模式使用 PSP。
 */
#define securecontextCONTROL_VALUE_UNPRIVILEGED    0x03

/**
 * @brief 栈封印值的大小（字节）。
 */
#define securecontextSTACK_SEAL_SIZE               8

/**
 * @brief ARM 推荐的栈封印值。
 */
#define securecontextSTACK_SEAL_VALUE              0xFEF5EDA5

/**
 * @brief 安全上下文的最大数量。
 */
#ifndef secureconfigMAX_SECURE_CONTEXTS
    #define secureconfigMAX_SECURE_CONTEXTS    8UL
#endif
/*-----------------------------------------------------------*/

/**
 * @brief 预分配的安全上下文数组。
 */
SecureContext_t xSecureContexts[ secureconfigMAX_SECURE_CONTEXTS ];
/*-----------------------------------------------------------*/

/**
 * @brief 从安全上下文池（xSecureContexts）中为任务获取一个空闲的安全上下文。
 *
 * 该函数确保一个任务只分配一个安全上下文。
 *
 * @param[in] pvTaskHandle 为其分配安全上下文的任务句柄。
 *
 * @return 空闲安全上下文在 xSecureContexts 数组中的索引。
 */
static uint32_t ulGetSecureContext( void * pvTaskHandle );

/**
 * @brief 将安全上下文归还到安全上下文池（xSecureContexts）。
 *
 * @param[in] ulSecureContextIndex 上下文在 xSecureContexts 数组中的索引。
 */
static void vReturnSecureContext( uint32_t ulSecureContextIndex );

/* 这些函数由汇编实现。 */
extern void SecureContext_LoadContextAsm( SecureContext_t * pxSecureContext );
extern void SecureContext_SaveContextAsm( SecureContext_t * pxSecureContext );
/*-----------------------------------------------------------*/

static uint32_t ulGetSecureContext( void * pvTaskHandle )
{
    /* 以无效索引开始。 */
    uint32_t i, ulSecureContextIndex = secureconfigMAX_SECURE_CONTEXTS;

    for( i = 0; i < secureconfigMAX_SECURE_CONTEXTS; i++ )
    {
        if( ( xSecureContexts[ i ].pucCurrentStackPointer == NULL ) &&
            ( xSecureContexts[ i ].pucStackLimit == NULL ) &&
            ( xSecureContexts[ i ].pucStackStart == NULL ) &&
            ( xSecureContexts[ i ].pvTaskHandle == NULL ) &&
            ( ulSecureContextIndex == secureconfigMAX_SECURE_CONTEXTS ) )
        {
            ulSecureContextIndex = i;
        }
        else if( xSecureContexts[ i ].pvTaskHandle == pvTaskHandle )
        {
            /*
             * 一个任务只能有一个安全上下文。不要为同一任务分配第二个上下文。
             */
            ulSecureContextIndex = secureconfigMAX_SECURE_CONTEXTS;
            break;
        }
    }

    return ulSecureContextIndex;
}
/*-----------------------------------------------------------*/

static void vReturnSecureContext( uint32_t ulSecureContextIndex )
{
    xSecureContexts[ ulSecureContextIndex ].pucCurrentStackPointer = NULL;
    xSecureContexts[ ulSecureContextIndex ].pucStackLimit = NULL;
    xSecureContexts[ ulSecureContextIndex ].pucStackStart = NULL;
    xSecureContexts[ ulSecureContextIndex ].pvTaskHandle = NULL;
}
/*-----------------------------------------------------------*/

secureportNON_SECURE_CALLABLE void SecureContext_Init( void )
{
    uint32_t ulIPSR, i;
    static uint32_t ulSecureContextsInitialized = 0;

    /* 读取中断程序状态寄存器（IPSR）的值。 */
    secureportREAD_IPSR( ulIPSR );

    /*
     * 若处理器运行在线程模式，则不做任何操作。处理器运行在线程模式时 IPSR 为零。
     */
    if( ( ulIPSR != 0 ) && ( ulSecureContextsInitialized == 0 ) )
    {
        /* 确保安全上下文仅初始化一次。 */
        ulSecureContextsInitialized = 1;

        /* 在加载任务的上下文之前，线程模式没有可用栈。 */
        secureportSET_PSPLIM( securecontextNO_STACK );
        secureportSET_PSP( securecontextNO_STACK );

        /* 初始化所有安全上下文。 */
        for( i = 0; i < secureconfigMAX_SECURE_CONTEXTS; i++ )
        {
            xSecureContexts[ i ].pucCurrentStackPointer = NULL;
            xSecureContexts[ i ].pucStackLimit = NULL;
            xSecureContexts[ i ].pucStackStart = NULL;
            xSecureContexts[ i ].pvTaskHandle = NULL;
        }

        #if ( configENABLE_MPU == 1 )
        {
            /* 将线程模式配置为使用 PSP 且为非特权级。 */
            secureportSET_CONTROL( securecontextCONTROL_VALUE_UNPRIVILEGED );
        }
        #else /* configENABLE_MPU（条件编译结束） */
        {
            /* 将线程模式配置为使用 PSP 且为特权级。 */
            secureportSET_CONTROL( securecontextCONTROL_VALUE_PRIVILEGED );
        }
        #endif /* configENABLE_MPU（条件编译结束） */
    }
}
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )
    secureportNON_SECURE_CALLABLE SecureContextHandle_t SecureContext_AllocateContext( uint32_t ulSecureStackSize,
                                                                                       uint32_t ulIsTaskPrivileged,
                                                                                       void * pvTaskHandle )
#else /* configENABLE_MPU（条件编译结束） */
    secureportNON_SECURE_CALLABLE SecureContextHandle_t SecureContext_AllocateContext( uint32_t ulSecureStackSize,
                                                                                       void * pvTaskHandle )
#endif /* configENABLE_MPU（条件编译结束） */
{
    uint8_t * pucStackMemory = NULL;
    uint8_t * pucStackLimit;
    uint32_t ulIPSR, ulSecureContextIndex;
    SecureContextHandle_t xSecureContextHandle = securecontextINVALID_CONTEXT_ID;

    #if ( configENABLE_MPU == 1 )
        uint32_t * pulCurrentStackPointer = NULL;
    #endif /* configENABLE_MPU（条件编译结束） */

    /*
     * 读取中断程序状态寄存器（IPSR）和进程栈限寄存器（PSPLIM）的值。
     */
    secureportREAD_IPSR( ulIPSR );
    secureportREAD_PSPLIM( pucStackLimit );

    /*
     * 若处理器运行在线程模式，则不做任何操作。处理器运行在线程模式时 IPSR 为零。
     * 若已加载安全上下文，也不做任何操作。未加载安全上下文时，
     * PSPLIM 被设置为 securecontextNO_STACK。
     */
    if( ( ulIPSR != 0 ) && ( pucStackLimit == securecontextNO_STACK ) )
    {
        /* 获取一个空闲的安全上下文。 */
        ulSecureContextIndex = ulGetSecureContext( pvTaskHandle );

        /* 是否成功获取到空闲上下文？ */
        if( ulSecureContextIndex < secureconfigMAX_SECURE_CONTEXTS )
        {
            /* 若可能，分配栈空间。 */
            if( ( ulSecureStackSize < securecontextSTACK_SEAL_SIZE ) ||
                ( ulSecureStackSize > ( UINT32_MAX - securecontextSTACK_SEAL_SIZE ) ) )
            {
                /*
                 * 拒绝过小的栈（CONTROL 字会在分配之前被写入，从而破坏安全堆），
                 * 以及加上封印值后会发生溢出的尺寸。
                 */
                pucStackMemory = NULL;
            }
            else
            {
                pucStackMemory = pvPortMalloc( ulSecureStackSize + securecontextSTACK_SEAL_SIZE );
            }

            if( pucStackMemory != NULL )
            {
                /*
                 * 由于栈向下增长，起始点为最后一个位置。注意该位置紧邻栈最后
                 * 分配的字节（不包括封印值占用的空间），因为硬件在写入之前
                 * 会先递减栈指针；即若栈指针为 0x2，压栈操作会把栈指针
                 * 递减到 0x1，然后写入 0x1。
                 */
                xSecureContexts[ ulSecureContextIndex ].pucStackStart = pucStackMemory + ulSecureStackSize;

                /* 对新创建的安全进程栈进行封印。 */
                *( uint32_t * ) ( pucStackMemory + ulSecureStackSize ) = securecontextSTACK_SEAL_VALUE;
                *( uint32_t * ) ( pucStackMemory + ulSecureStackSize + 4 ) = securecontextSTACK_SEAL_VALUE;

                /*
                 * 栈不能超出此位置。该值在上下文切换时写入 PSPLIM 寄存器。
                 */
                xSecureContexts[ ulSecureContextIndex ].pucStackLimit = pucStackMemory;

                xSecureContexts[ ulSecureContextIndex ].pvTaskHandle = pvTaskHandle;

                #if ( configENABLE_MPU == 1 )
                {
                    /*
                     * 将任务正确的 CONTROL 值保存到栈中。该值在上下文切换时
                     * 写入 CONTROL 寄存器。
                     */
                    pulCurrentStackPointer = ( uint32_t * ) xSecureContexts[ ulSecureContextIndex ].pucStackStart;
                    pulCurrentStackPointer--;

                    if( ulIsTaskPrivileged )
                    {
                        *( pulCurrentStackPointer ) = securecontextCONTROL_VALUE_PRIVILEGED;
                    }
                    else
                    {
                        *( pulCurrentStackPointer ) = securecontextCONTROL_VALUE_UNPRIVILEGED;
                    }

                    /*
                     * 保存当前栈指针。该值在上下文切换时写入 PSP 寄存器。
                     */
                    xSecureContexts[ ulSecureContextIndex ].pucCurrentStackPointer = ( uint8_t * ) pulCurrentStackPointer;
                }
                #else /* configENABLE_MPU（条件编译结束） */
                {
                    /*
                     * 将当前 SP 设置为栈的起始位置。该值在上下文切换时
                     * 写入 PSP 寄存器。
                     */
                    xSecureContexts[ ulSecureContextIndex ].pucCurrentStackPointer = xSecureContexts[ ulSecureContextIndex ].pucStackStart;
                }
                #endif /* configENABLE_MPU（条件编译结束） */

                /* 确保绝不将 0 作为有效的上下文句柄返回。 */
                xSecureContextHandle = ulSecureContextIndex + 1UL;
            }
        }
    }

    return xSecureContextHandle;
}
/*-----------------------------------------------------------*/

secureportNON_SECURE_CALLABLE void SecureContext_FreeContext( SecureContextHandle_t xSecureContextHandle,
                                                              void * pvTaskHandle )
{
    uint32_t ulIPSR, ulSecureContextIndex;
    uint8_t * pucStackLimit;

    /*
     * 读取中断程序状态寄存器（IPSR）和进程栈限寄存器（PSPLIM）的值。
     */
    secureportREAD_IPSR( ulIPSR );
    secureportREAD_PSPLIM( pucStackLimit );

    /*
     * 若处理器运行在线程模式，则不做任何操作。处理器运行在线程模式时 IPSR 为零。
     */
    if( ulIPSR != 0 )
    {
        /* 仅在传入有效上下文句柄时才释放。 */
        if( ( xSecureContextHandle > 0UL ) && ( xSecureContextHandle <= secureconfigMAX_SECURE_CONTEXTS ) )
        {
            ulSecureContextIndex = xSecureContextHandle - 1UL;

            /*
             * 确保待删除的安全上下文与该任务关联，且不是当前已加载的上下文。
             * 释放其栈当前已加载到 PSPLIM 的上下文，会使正在运行的任务
             * 引用已释放的安全内存（释放后使用）。pvTaskHandle 由非安全侧
             * 提供且不可信，因此仅作为额外的所有权匹配条件，而不作为权限依据。
             */
            if( ( xSecureContexts[ ulSecureContextIndex ].pvTaskHandle == pvTaskHandle ) &&
                ( xSecureContexts[ ulSecureContextIndex ].pucStackLimit != pucStackLimit ) )
            {
                /* 释放栈空间。 */
                vPortFree( xSecureContexts[ ulSecureContextIndex ].pucStackLimit );

                /* 将安全上下文归还到空闲安全上下文池。 */
                vReturnSecureContext( ulSecureContextIndex );
            }
        }
    }
}
/*-----------------------------------------------------------*/

secureportNON_SECURE_CALLABLE void SecureContext_LoadContext( SecureContextHandle_t xSecureContextHandle,
                                                              void * pvTaskHandle )
{
    uint8_t * pucStackLimit;
    uint32_t ulSecureContextIndex;

    if( ( xSecureContextHandle > 0UL ) && ( xSecureContextHandle <= secureconfigMAX_SECURE_CONTEXTS ) )
    {
        ulSecureContextIndex = xSecureContextHandle - 1UL;

        secureportREAD_PSPLIM( pucStackLimit );

        /*
         * 确保未加载任何安全上下文，且任务正在加载其自身的上下文。
         */
        if( ( pucStackLimit == securecontextNO_STACK ) &&
            ( xSecureContexts[ ulSecureContextIndex ].pvTaskHandle == pvTaskHandle ) )
        {
            SecureContext_LoadContextAsm( &( xSecureContexts[ ulSecureContextIndex ] ) );
        }
    }
}
/*-----------------------------------------------------------*/

secureportNON_SECURE_CALLABLE void SecureContext_SaveContext( SecureContextHandle_t xSecureContextHandle,
                                                              void * pvTaskHandle )
{
    uint8_t * pucStackLimit;
    uint32_t ulSecureContextIndex;

    if( ( xSecureContextHandle > 0UL ) && ( xSecureContextHandle <= secureconfigMAX_SECURE_CONTEXTS ) )
    {
        ulSecureContextIndex = xSecureContextHandle - 1UL;

        secureportREAD_PSPLIM( pucStackLimit );

        /*
         * 确保任务已加载其上下文，且任务正在保存其自身的上下文。
         */
        if( ( xSecureContexts[ ulSecureContextIndex ].pucStackLimit == pucStackLimit ) &&
            ( xSecureContexts[ ulSecureContextIndex ].pvTaskHandle == pvTaskHandle ) )
        {
            SecureContext_SaveContextAsm( &( xSecureContexts[ ulSecureContextIndex ] ) );
        }
    }
}
/*-----------------------------------------------------------*/
