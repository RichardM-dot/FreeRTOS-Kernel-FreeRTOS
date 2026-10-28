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

/* 安全上下文相关头文件。 */
#include "secure_context.h"

/* 安全堆相关头文件。 */
#include "secure_heap.h"

/* 安全移植层宏。 */
#include "secure_port_macros.h"

/**
 * @brief 特权任务的 CONTROL 值。
 *
 * Bit[0] - 0 --> 线程模式为特权级。
 * Bit[1] - 1 --> 线程模式使用 PSP。
 */
#define securecontextCONTROL_VALUE_PRIVILEGED      0x02

/**
 * @brief 非特权任务的 CONTROL 值。
 *
 * Bit[0] - 1 --> 线程模式为非特权级。
 * Bit[1] - 1 --> 线程模式使用 PSP。
 */
#define securecontextCONTROL_VALUE_UNPRIVILEGED    0x03

/**
 * @brief 栈密封值的大小（以字节为单位）。
 */
#define securecontextSTACK_SEAL_SIZE               8

/**
 * @brief ARM 推荐的栈密封值。
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
 * @brief 从安全上下文池（xSecureContexts）中为任务获取空闲的安全上下文。
 *
 * 本函数确保每个任务只分配一个安全上下文。
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

/* 这些函数在汇编中实现。 */
extern void SecureContext_LoadContextAsm( SecureContext_t * pxSecureContext );
extern void SecureContext_SaveContextAsm( SecureContext_t * pxSecureContext );
/*-----------------------------------------------------------*/

static uint32_t ulGetSecureContext( void * pvTaskHandle )
{
    /* 从无效索引开始。 */
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
             * 一个任务只能有一个安全上下文。不要为同一任务
             * 分配第二个上下文。
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
     * 如果处理器在线程模式下运行，则不做任何操作。处理器在线程模式
     * 下运行时 IPSR 为零。
     */
    if( ( ulIPSR != 0 ) && ( ulSecureContextsInitialized == 0 ) )
    {
        /* 确保安全上下文只初始化一次。 */
        ulSecureContextsInitialized = 1;

        /* 在加载任务上下文之前，线程模式没有栈。 */
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
        #else /* configENABLE_MPU（使能 MPU） */
        {
            /* 将线程模式配置为使用 PSP 且为特权级。 */
            secureportSET_CONTROL( securecontextCONTROL_VALUE_PRIVILEGED );
        }
        #endif /* configENABLE_MPU（使能 MPU） */
    }
}
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )
    secureportNON_SECURE_CALLABLE SecureContextHandle_t SecureContext_AllocateContext( uint32_t ulSecureStackSize,
                                                                                       uint32_t ulIsTaskPrivileged,
                                                                                       void * pvTaskHandle )
#else /* configENABLE_MPU（使能 MPU） */
    secureportNON_SECURE_CALLABLE SecureContextHandle_t SecureContext_AllocateContext( uint32_t ulSecureStackSize,
                                                                                       void * pvTaskHandle )
#endif /* configENABLE_MPU（使能 MPU） */
{
    uint8_t * pucStackMemory = NULL;
    uint8_t * pucStackLimit;
    uint32_t ulIPSR, ulSecureContextIndex;
    SecureContextHandle_t xSecureContextHandle = securecontextINVALID_CONTEXT_ID;

    #if ( configENABLE_MPU == 1 )
        uint32_t * pulCurrentStackPointer = NULL;
    #endif /* configENABLE_MPU（使能 MPU） */

    /*
     * 读取中断程序状态寄存器（IPSR）和进程栈限制
     * 寄存器（PSPLIM）的值。
     */
    secureportREAD_IPSR( ulIPSR );
    secureportREAD_PSPLIM( pucStackLimit );

    /*
     * 如果处理器在线程模式下运行，则不做任何操作。处理器在线程模式
     * 下运行时 IPSR 为零。
     * 此外，如果已经加载了安全上下文，也不做任何操作。未加载安全
     * 上下文时，PSPLIM 被设置为 securecontextNO_STACK。
     */
    if( ( ulIPSR != 0 ) && ( pucStackLimit == securecontextNO_STACK ) )
    {
        /* 获取一个空闲的安全上下文。 */
        ulSecureContextIndex = ulGetSecureContext( pvTaskHandle );

        /* 是否成功获取了空闲上下文？ */
        if( ulSecureContextIndex < secureconfigMAX_SECURE_CONTEXTS )
        {
            /* 如果可能，分配栈空间。 */
            if( ( ulSecureStackSize < securecontextSTACK_SEAL_SIZE ) ||
                ( ulSecureStackSize > ( UINT32_MAX - securecontextSTACK_SEAL_SIZE ) ) )
            {
                /*
                 * 拒绝过小的栈（CONTROL 字会在分配之前被写入，
                 * 从而破坏安全堆），也拒绝加上密封值后
                 * 会溢出的尺寸。
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
                 * 由于栈向下增长，起始点将是最后一个
                 * 位置。注意该位置紧邻最后分配的栈字节
                 * （不包括密封值的空间），因为硬件在写入
                 * 之前会先递减栈指针，即如果栈指针为 0x2，
                 * 压栈操作会先将栈指针递减到 0x1，然后在 0x1 处写入。
                 */
                xSecureContexts[ ulSecureContextIndex ].pucStackStart = pucStackMemory + ulSecureStackSize;

                /* 对创建的安全进程栈进行密封。 */
                *( uint32_t * ) ( pucStackMemory + ulSecureStackSize ) = securecontextSTACK_SEAL_VALUE;
                *( uint32_t * ) ( pucStackMemory + ulSecureStackSize + 4 ) = securecontextSTACK_SEAL_VALUE;

                /*
                 * 栈不能越过此位置。该值在上下文切换时
                 * 被编程到 PSPLIM 寄存器中。
                 */
                xSecureContexts[ ulSecureContextIndex ].pucStackLimit = pucStackMemory;

                xSecureContexts[ ulSecureContextIndex ].pvTaskHandle = pvTaskHandle;

                #if ( configENABLE_MPU == 1 )
                {
                    /*
                     * 在栈上保存任务正确的 CONTROL 值。
                     * 该值在上下文切换时被编程到
                     * CONTROL 寄存器中。
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
                     * 保存当前栈指针。该值在上下文切换时
                     * 被编程到 PSP 寄存器中。
                     */
                    xSecureContexts[ ulSecureContextIndex ].pucCurrentStackPointer = ( uint8_t * ) pulCurrentStackPointer;
                }
                #else /* configENABLE_MPU（使能 MPU） */
                {
                    /*
                     * 当前 SP 被设置为栈的起始位置。该值
                     * 在上下文切换时被编程到 PSP 寄存器中。
                     */
                    xSecureContexts[ ulSecureContextIndex ].pucCurrentStackPointer = xSecureContexts[ ulSecureContextIndex ].pucStackStart;
                }
                #endif /* configENABLE_MPU（使能 MPU） */

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
     * 读取中断程序状态寄存器（IPSR）和进程栈限制
     * 寄存器（PSPLIM）的值。
     */
    secureportREAD_IPSR( ulIPSR );
    secureportREAD_PSPLIM( pucStackLimit );

    /*
     * 如果处理器在线程模式下运行，则不做任何操作。处理器在线程模式
     * 下运行时 IPSR 为零。
     */
    if( ulIPSR != 0 )
    {
        /* 仅在传入有效上下文句柄时才释放。 */
        if( ( xSecureContextHandle > 0UL ) && ( xSecureContextHandle <= secureconfigMAX_SECURE_CONTEXTS ) )
        {
            ulSecureContextIndex = xSecureContextHandle - 1UL;

            /*
             * 确保正在删除的安全上下文与任务关联，
             * 并且不是当前已加载的上下文。释放其栈当前已加载到
             * PSPLIM 的上下文，会使正在运行的任务引用已释放的安全内存
             * （释放后使用）。
             * pvTaskHandle 由非安全侧提供，不可信，
             * 因此仅将其用作额外的归属匹配，而不作为
             * 权限依据。
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
         * 确保未加载任何安全上下文，且任务正在加载其
         * 自己的上下文。
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
         * 确保任务上下文已加载，且任务正在保存其
         * 自己的上下文。
         */
        if( ( xSecureContexts[ ulSecureContextIndex ].pucStackLimit == pucStackLimit ) &&
            ( xSecureContexts[ ulSecureContextIndex ].pvTaskHandle == pvTaskHandle ) )
        {
            SecureContext_SaveContextAsm( &( xSecureContexts[ ulSecureContextIndex ] ) );
        }
    }
}
/*-----------------------------------------------------------*/
