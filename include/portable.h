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

/*
 * -----------------------------------------------------------
 * 移植层 API。每个函数都必须针对每个移植层分别定义。
 * ----------------------------------------------------------
 */

#ifndef PORTABLE_H
#define PORTABLE_H

/*
 * 每个 FreeRTOS 移植层都有唯一的 portmacro.h 头文件。最初使用
 * 预处理器定义来确保预处理器找到所用移植层对应的正确 portmacro.h
 * 文件。该方案已被弃用，改为设置编译器的包含路径，使其找到正确的
 * portmacro.h 文件——这样就不再需要该常量，并允许 portmacro.h 文件
 * 位于与所用移植层相关的任意位置。纯粹出于向后兼容的考虑，旧方法
 * 仍然有效，但为明确新项目不应使用旧方法，对移植层特定常量的支持
 * 已移至 deprecated_definitions.h 头文件。
 */
#include "deprecated_definitions.h"

/*
 * 如果未定义 portENTER_CRITICAL，则包含 deprecated_definitions.h
 * 不会引入 portmacro.h 头文件——此时应在此处包含。在这种情况下，
 * 必须在编译器的包含路径中设置正确 portmacro.h 头文件的路径。
 */
#ifndef portENTER_CRITICAL
    #include "portmacro.h"
#endif

#if portBYTE_ALIGNMENT == 32
    #define portBYTE_ALIGNMENT_MASK    ( 0x001f )
#elif portBYTE_ALIGNMENT == 16
    #define portBYTE_ALIGNMENT_MASK    ( 0x000f )
#elif portBYTE_ALIGNMENT == 8
    #define portBYTE_ALIGNMENT_MASK    ( 0x0007 )
#elif portBYTE_ALIGNMENT == 4
    #define portBYTE_ALIGNMENT_MASK    ( 0x0003 )
#elif portBYTE_ALIGNMENT == 2
    #define portBYTE_ALIGNMENT_MASK    ( 0x0001 )
#elif portBYTE_ALIGNMENT == 1
    #define portBYTE_ALIGNMENT_MASK    ( 0x0000 )
#else /* 条件不成立：if portBYTE_ALIGNMENT == 32 */
    #error "Invalid portBYTE_ALIGNMENT definition"
#endif /* 条件编译结束：if portBYTE_ALIGNMENT == 32 */

#ifndef portUSING_MPU_WRAPPERS
    #define portUSING_MPU_WRAPPERS    0
#endif

#ifndef portNUM_CONFIGURABLE_REGIONS
    #define portNUM_CONFIGURABLE_REGIONS    1
#endif

#ifndef portHAS_STACK_OVERFLOW_CHECKING
    #define portHAS_STACK_OVERFLOW_CHECKING    0
#endif

#ifndef portARCH_NAME
    #define portARCH_NAME    NULL
#endif

#ifndef portBASE_TYPE_ENTER_CRITICAL
    #define portBASE_TYPE_ENTER_CRITICAL()    taskENTER_CRITICAL()
#endif

#ifndef portBASE_TYPE_EXIT_CRITICAL
    #define portBASE_TYPE_EXIT_CRITICAL()    taskEXIT_CRITICAL()
#endif

#ifndef configSTACK_DEPTH_TYPE
    #define configSTACK_DEPTH_TYPE    StackType_t
#endif

#ifndef configSTACK_ALLOCATION_FROM_SEPARATE_HEAP
    /* 为向后兼容，默认为 0。 */
    #define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP    0
#endif

#include "mpu_wrappers.h"

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

/*
 * 设置新任务的栈，使其准备好接受调度器控制。寄存器必须按照
 * 移植层期望的顺序压入栈中。
 */
#if ( portUSING_MPU_WRAPPERS == 1 )
    #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             StackType_t * pxEndOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters,
                                             BaseType_t xRunPrivileged,
                                             xMPU_SETTINGS * xMPUSettings ) PRIVILEGED_FUNCTION;
    #else
        StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters,
                                             BaseType_t xRunPrivileged,
                                             xMPU_SETTINGS * xMPUSettings ) PRIVILEGED_FUNCTION;
    #endif /* 条件编译结束：if ( portHAS_STACK_OVERFLOW_CHECKING == 1 ) */
#else /* 条件不成立：if ( portUSING_MPU_WRAPPERS == 1 ) */
    #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             StackType_t * pxEndOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters ) PRIVILEGED_FUNCTION;
    #else
        StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters ) PRIVILEGED_FUNCTION;
    #endif
#endif /* 条件编译结束：if ( portUSING_MPU_WRAPPERS == 1 ) */

/*
 * 由 heap_5.c 使用，用于定义共同构成 FreeRTOS 总堆空间的
 * 各个内存区域的起始地址和大小。
 */
typedef struct HeapRegion
{
    uint8_t * pucStartAddress;
    size_t xSizeInBytes;
} HeapRegion_t;

/* 用于从 vPortGetHeapStats() 传出堆的相关信息。 */
typedef struct xHeapStats
{
    size_t xAvailableHeapSpaceInBytes;      /* 当前可用的堆总大小——这是所有空闲块之和，而不是可分配的最大块。 */
    size_t xSizeOfLargestFreeBlockInBytes;  /* 调用 vPortGetHeapStats() 时，堆内所有空闲块的最大大小（字节）。 */
    size_t xSizeOfSmallestFreeBlockInBytes; /* 调用 vPortGetHeapStats() 时，堆内所有空闲块的最小大小（字节）。 */
    size_t xNumberOfFreeBlocks;             /* 调用 vPortGetHeapStats() 时，堆内空闲内存块的数量。 */
    size_t xMinimumEverFreeBytesRemaining;  /* 自系统启动以来，堆内曾出现的最小空闲内存总量（所有空闲块之和）。 */
    size_t xNumberOfSuccessfulAllocations;  /* pvPortMalloc() 返回有效内存块的调用次数。 */
    size_t xNumberOfSuccessfulFrees;        /* vPortFree() 成功释放内存块的调用次数。 */
} HeapStats_t;

/*
 * 用于为 heap_5.c 定义多个堆区域。此函数必须在任何
 * pvPortMalloc() 调用之前调用——创建任务、队列、信号量、互斥量、
 * 软件定时器、事件组等都会导致调用 pvPortMalloc。
 *
 * pxHeapRegions 传入一个 HeapRegion_t 结构体数组，其中每个结构体
 * 定义一个可用作堆的内存区域。该数组以一个大小为 0 的
 * HeapRegions_t 结构体结束。起始地址最低的区域必须位于数组首位。
 */
void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions ) PRIVILEGED_FUNCTION;

/*
 * 返回一个填充了当前堆状态信息的 HeapStats_t 结构体。
 */
void vPortGetHeapStats( HeapStats_t * pxHeapStats );

/*
 * 映射到移植层所需的内存管理例程。
 */
void * pvPortMalloc( size_t xWantedSize ) PRIVILEGED_FUNCTION;
void * pvPortCalloc( size_t xNum,
                     size_t xSize ) PRIVILEGED_FUNCTION;
void vPortFree( void * pv ) PRIVILEGED_FUNCTION;
void vPortInitialiseBlocks( void ) PRIVILEGED_FUNCTION;
size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION;
size_t xPortGetMinimumEverFreeHeapSize( void ) PRIVILEGED_FUNCTION;
void xPortResetHeapMinimumEverFreeHeapSize( void ) PRIVILEGED_FUNCTION;

#if ( configSTACK_ALLOCATION_FROM_SEPARATE_HEAP == 1 )
    void * pvPortMallocStack( size_t xSize ) PRIVILEGED_FUNCTION;
    void vPortFreeStack( void * pv ) PRIVILEGED_FUNCTION;
#else
    #define pvPortMallocStack    pvPortMalloc
    #define vPortFreeStack       vPortFree
#endif

/*
 * 此函数重置堆模块的内部状态。应用程序必须在重新启动调度器
 * 之前调用它。
 */
void vPortHeapResetState( void ) PRIVILEGED_FUNCTION;

#if ( configUSE_MALLOC_FAILED_HOOK == 1 )

/**
 * task.h
 * @code{c}
 * void vApplicationMallocFailedHook( void )
 * @endcode
 *
 * 此钩子函数在内存分配失败时被调用。
 */
    void vApplicationMallocFailedHook( void );
#endif

/*
 * 为调度器接管控制做好硬件准备。这通常会设置时钟节拍中断，
 * 并按正确的节拍频率配置定时器。
 */
BaseType_t xPortStartScheduler( void ) PRIVILEGED_FUNCTION;

/*
 * 撤销 xPortStartScheduler() 所做的任何硬件/ISR 设置，使调度器
 * 停止运行后硬件恢复到原始状态。
 */
void vPortEndScheduler( void ) PRIVILEGED_FUNCTION;

/*
 * MPU 的结构和操作方法都包含在移植层内。
 *
 * 用 xRegions 中的内存区域信息填充 xMPUSettings 结构体。
 */
#if ( portUSING_MPU_WRAPPERS == 1 )
    struct xMEMORY_REGION;
    void vPortStoreTaskMPUSettings( xMPU_SETTINGS * xMPUSettings,
                                    const struct xMEMORY_REGION * const xRegions,
                                    StackType_t * pxBottomOfStack,
                                    configSTACK_DEPTH_TYPE uxStackDepth ) PRIVILEGED_FUNCTION;
#endif

/**
 * @brief 检查调用任务是否有权访问给定缓冲区。
 *
 * @param pvBuffer 调用任务想要访问的缓冲区。
 * @param ulBufferLength pvBuffer 的长度。
 * @param ulAccessRequested 调用任务请求的权限。
 *
 * @return 如果调用任务有权访问该缓冲区则返回 pdTRUE，
 *         否则返回 pdFALSE。
 */
#if ( portUSING_MPU_WRAPPERS == 1 )
    BaseType_t xPortIsAuthorizedToAccessBuffer( const void * pvBuffer,
                                                uint32_t ulBufferLength,
                                                uint32_t ulAccessRequested ) PRIVILEGED_FUNCTION;
#endif

/**
 * @brief 检查调用任务是否有权访问给定的内核对象。
 *
 * @param lInternalIndexOfKernelObject 内核对象句柄池中该内核对象的索引。
 *
 * @return 如果调用任务有权访问该内核对象则返回 pdTRUE，
 *         否则返回 pdFALSE。
 */
#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) PRIVILEGED_FUNCTION;

#endif

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：PORTABLE_H */
