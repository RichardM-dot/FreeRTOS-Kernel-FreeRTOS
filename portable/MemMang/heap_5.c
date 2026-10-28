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
 * pvPortMalloc() 的示例实现，允许堆跨越多个不连续块定义，
 * 并在释放时合并（coalescence）相邻的内存块。
 *
 * 替代实现参见 heap_1.c、heap_2.c、heap_3.c 和 heap_4.c，
 * 更多信息参见 https://www.FreeRTOS.org 的内存管理页面。
 *
 * 使用说明：
 *
 * vPortDefineHeapRegions() ***必须*** 在 pvPortMalloc() 之前调用。
 * 创建任何任务对象（任务、队列、事件组等）时都会调用 pvPortMalloc()，
 * 因此 vPortDefineHeapRegions() ***必须*** 在定义任何其他对象之前调用。
 *
 * vPortDefineHeapRegions() 接受单个参数。该参数是
 * HeapRegion_t 结构体数组。HeapRegion_t 在 portable.h 中定义如下：
 *
 * typedef struct HeapRegion
 * {
 *  uint8_t *pucStartAddress; << 将成为堆一部分的内存块的起始地址。
 *  size_t xSizeInBytes;      << 内存块的大小。
 * } HeapRegion_t;
 *
 * 该数组使用 NULL 且大小为零的区域定义来终止，
 * 数组中定义的内存区域 ***必须*** 按地址从低到高排列。
 * 以下为该函数用法的有效示例。
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 *  { ( uint8_t * ) 0x80000000UL, 0x10000 }, << 定义起始地址为 0x80000000、大小为 0x10000 字节的块
 *  { ( uint8_t * ) 0x90000000UL, 0xa0000 }, << 定义起始地址为 0x90000000、大小为 0xa0000 字节的块
 *  { NULL, 0 }                << 终止数组。
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << 将数组传入 vPortDefineHeapRegions()。
 *
 * 注意 0x80000000 是较低地址，因此首先出现在数组中。
 */
#include <stdlib.h>
#include <string.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有 API 函数
 * 重定义为使用 MPU 包装器。只有在应用程序文件中包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
    #define configHEAP_CLEAR_MEMORY_ON_FREE    0
#endif

/* 块大小不得过小。 */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* 假定字节为 8 位！ */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* size_t 类型能容纳的最大值。 */
#define heapSIZE_MAX              ( ~( ( size_t ) 0 ) )

/* 检查 a 与 b 相乘是否会溢出。 */
#define heapMULTIPLY_WILL_OVERFLOW( a, b )     ( ( ( a ) > 0 ) && ( ( b ) > ( heapSIZE_MAX / ( a ) ) ) )

/* 检查 a 与 b 相加是否会溢出。 */
#define heapADD_WILL_OVERFLOW( a, b )          ( ( a ) > ( heapSIZE_MAX - ( b ) ) )

/* 检查减法运算 ( a - b ) 是否会发生下溢。 */
#define heapSUBTRACT_WILL_UNDERFLOW( a, b )    ( ( a ) < ( b ) )

/*
 * BlockLink_t 结构体 xBlockSize 成员的最高位（MSB）用于跟踪块的分配状态。
 * 当 BlockLink_t 结构体 xBlockSize 成员的最高位被置位时，该块属于应用程序；
 * 当该位空闲（为 0）时，该块仍属于空闲堆空间。
 */
#define heapBLOCK_ALLOCATED_BITMASK    ( ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 ) )
#define heapBLOCK_SIZE_IS_VALID( xBlockSize )    ( ( ( xBlockSize ) & heapBLOCK_ALLOCATED_BITMASK ) == 0 )
#define heapBLOCK_IS_ALLOCATED( pxBlock )        ( ( ( pxBlock->xBlockSize ) & heapBLOCK_ALLOCATED_BITMASK ) != 0 )
#define heapALLOCATE_BLOCK( pxBlock )            ( ( pxBlock->xBlockSize ) |= heapBLOCK_ALLOCATED_BITMASK )
#define heapFREE_BLOCK( pxBlock )                ( ( pxBlock->xBlockSize ) &= ~heapBLOCK_ALLOCATED_BITMASK )

/*
 * 将 configENABLE_HEAP_PROTECTOR 设为 1 可启用堆块指针保护：
 * 使用应用程序提供的金丝雀值，在发生堆缓冲区溢出时捕获堆损坏。
 */
#if ( configENABLE_HEAP_PROTECTOR == 1 )

/*
 * 用于将 BlockLink_t 指针加载/存储到内存的宏。通过将指针与随机金丝雀值
 * 进行异或，堆溢出将导致随机不可预测的指针值，
 * 从而被 heapVALIDATE_BLOCK_POINTER 断言捕获。
 */
    #define heapPROTECT_BLOCK_POINTER( pxBlock )    ( ( BlockLink_t * ) ( ( ( portPOINTER_SIZE_TYPE ) ( pxBlock ) ) ^ xHeapCanary ) )

/*
 * 断言堆块指针位于堆边界内。
 * 将 configVALIDATE_HEAP_BLOCK_POINTER 设为 1 可启用 heap_5 上的
 * 自定义堆块指针保护。
 */
    #ifndef configVALIDATE_HEAP_BLOCK_POINTER
        #define heapVALIDATE_BLOCK_POINTER( pxBlock )                           \
            configASSERT( ( pucHeapHighAddress != NULL ) &&                     \
                          ( pucHeapLowAddress != NULL ) &&                      \
                          ( ( uint8_t * ) ( pxBlock ) >= pucHeapLowAddress ) && \
                          ( ( uint8_t * ) ( pxBlock ) < pucHeapHighAddress ) )
    #else /* 条件块结束：ifndef configVALIDATE_HEAP_BLOCK_POINTER */
        #define heapVALIDATE_BLOCK_POINTER( pxBlock )                           \
            configVALIDATE_HEAP_BLOCK_POINTER( pxBlock )
    #endif /* 配置项 configVALIDATE_HEAP_BLOCK_POINTER */

#else /* 条件不成立：if ( configENABLE_HEAP_PROTECTOR == 1 ) */

    #define heapPROTECT_BLOCK_POINTER( pxBlock )    ( pxBlock )

    #define heapVALIDATE_BLOCK_POINTER( pxBlock )

#endif /* 配置项 configENABLE_HEAP_PROTECTOR */

/*-----------------------------------------------------------*/

/*
 * 定义链表结构体，用于按内存地址顺序链接空闲块。
 */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /**< 链表中的下一个空闲块。 */
    size_t xBlockSize;                     /**< 空闲块的大小。 */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * 将正在释放的内存块插入空闲内存块链表的正确位置。
 * 如果内存块彼此相邻，被释放的块将与其前一个块和/或后一个块合并。
 */
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert ) PRIVILEGED_FUNCTION;
void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions ) PRIVILEGED_FUNCTION;

#if ( configENABLE_HEAP_PROTECTOR == 1 )

/**
 * @brief 应用程序提供的函数，用于获取用作金丝雀值的随机值。
 *
 * @param pxHeapCanary [out] 输出参数，用于返回金丝雀值。
 */
    extern void vApplicationGetRandomHeapCanary( portPOINTER_SIZE_TYPE * pxHeapCanary );
#endif /* 配置项 configENABLE_HEAP_PROTECTOR */

/*-----------------------------------------------------------*/

/*
 * 放置在每个已分配内存块起始处的结构体大小必须正确进行字节对齐。
 */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* 创建两个链表节点，用于标记链表的起始和结束。 */
PRIVILEGED_DATA static BlockLink_t xStart;
PRIVILEGED_DATA static BlockLink_t * pxEnd = NULL;

/*
 * 记录分配和释放内存的调用次数以及剩余空闲字节数，
 * 但不反映碎片情况。
 */
PRIVILEGED_DATA static size_t xFreeBytesRemaining = ( size_t ) 0U;
PRIVILEGED_DATA static size_t xMinimumEverFreeBytesRemaining = ( size_t ) 0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulAllocations = ( size_t ) 0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulFrees = ( size_t ) 0U;

#if ( configENABLE_HEAP_PROTECTOR == 1 )

/* 用于保护内部堆指针的金丝雀值。 */
    PRIVILEGED_DATA static portPOINTER_SIZE_TYPE xHeapCanary;

/* 用于堆块边界检查的最高和最低堆地址。 */
    PRIVILEGED_DATA static uint8_t * pucHeapHighAddress = NULL;
    PRIVILEGED_DATA static uint8_t * pucHeapLowAddress = NULL;

#endif /* 配置项 configENABLE_HEAP_PROTECTOR */

/*-----------------------------------------------------------*/

void * pvPortMalloc( size_t xWantedSize )
{
    BlockLink_t * pxBlock;
    BlockLink_t * pxPreviousBlock;
    BlockLink_t * pxNewBlockLink;
    void * pvReturn = NULL;
    size_t xAdditionalRequiredSize;
    size_t xAllocatedBlockSize = 0;

    /*
     * 首次调用 pvPortMalloc() 之前必须先初始化堆。
     */
    configASSERT( pxEnd );

    if( xWantedSize > 0 )
    {
        /*
         * 需要增大请求大小，使其除请求的字节数外
         * 还能容纳一个 BlockLink_t 结构体。
         */
        if( heapADD_WILL_OVERFLOW( xWantedSize, xHeapStructSize ) == 0 )
        {
            xWantedSize += xHeapStructSize;

            /*
             * 确保块始终按要求的字节数对齐。
             */
            if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
            {
                /* 需要进行字节对齐。 */
                xAdditionalRequiredSize = portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK );

                if( heapADD_WILL_OVERFLOW( xWantedSize, xAdditionalRequiredSize ) == 0 )
                {
                    xWantedSize += xAdditionalRequiredSize;
                }
                else
                {
                    xWantedSize = 0;
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            xWantedSize = 0;
        }
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    vTaskSuspendAll();
    {
        /*
         * 检查试图分配的块大小是否过大而导致最高位被置位。
         * BlockLink_t 结构体块大小成员的最高位用于确定块的归属——
         * 应用程序还是内核，因此该位必须保持为 0。
         */
        if( heapBLOCK_SIZE_IS_VALID( xWantedSize ) != 0 )
        {
            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
            {
                /*
                 * 从链表起始处（最低地址）开始遍历，
                 * 直到找到大小足够的块。
                 */
                pxPreviousBlock = &xStart;
                pxBlock = heapPROTECT_BLOCK_POINTER( xStart.pxNextFreeBlock );
                heapVALIDATE_BLOCK_POINTER( pxBlock );

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != heapPROTECT_BLOCK_POINTER( NULL ) ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = heapPROTECT_BLOCK_POINTER( pxBlock->pxNextFreeBlock );

                    /*
                     * pxEnd 是空闲链表的结束标记。它位于
                     * pucHeapHighAddress，不属于可用堆，因此在
                     * 堆块指针校验时必须排除。
                     */
                    if( pxBlock != pxEnd )
                    {
                        heapVALIDATE_BLOCK_POINTER( pxBlock );
                    }
                }

                /*
                 * 如果到达结束标记，则说明未找到大小足够的块。
                 */
                if( pxBlock != pxEnd )
                {
                    /*
                     * 返回所指向的内存空间——跳过其起始处的
                     * BlockLink_t 结构体。
                     */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) heapPROTECT_BLOCK_POINTER( pxPreviousBlock->pxNextFreeBlock ) ) + xHeapStructSize );
                    heapVALIDATE_BLOCK_POINTER( pvReturn );

                    /*
                     * 该块将被返回使用，因此必须从空闲块链表中
                     * 移除。
                     */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /*
                     * 如果块大于所需大小，可将其拆分为
                     * 两个块。
                     */
                    configASSERT( heapSUBTRACT_WILL_UNDERFLOW( pxBlock->xBlockSize, xWantedSize ) == 0 );

                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /*
                         * 该块将被拆分为两个。按请求的字节数在其后创建
                         * 新块。使用 void 强制转换可避免编译器
                         * 产生字节对齐警告。
                         */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
                        configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

                        /*
                         * 计算从单个块拆分出的两个块的大小。
                         */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* 将新块插入空闲块链表。 */
                        pxNewBlockLink->pxNextFreeBlock = pxPreviousBlock->pxNextFreeBlock;
                        pxPreviousBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( pxNewBlockLink );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
                    {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xAllocatedBlockSize = pxBlock->xBlockSize;

                    /*
                     * 该块将被返回——它已分配并归应用程序所有，
                     * 且没有“下一个”块。
                     */
                    heapALLOCATE_BLOCK( pxBlock );
                    pxBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( NULL );
                    xNumberOfSuccessfulAllocations++;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xAllocatedBlockSize );

        /* 在未使用跟踪宏时避免编译器警告。 */
        ( void ) xAllocatedBlockSize;
    }
    ( void ) xTaskResumeAll();

    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
        if( pvReturn == NULL )
        {
            vApplicationMallocFailedHook();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif /* 条件块结束：if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void * pv )
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;

    if( pv != NULL )
    {
        /*
         * 被释放的内存之前紧邻一个 BlockLink_t 结构体。
         */
        puc -= xHeapStructSize;

        /* 此处强制转换是为了避免编译器发出警告。 */
        pxLink = ( void * ) puc;

        heapVALIDATE_BLOCK_POINTER( pxLink );
        configASSERT( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == heapPROTECT_BLOCK_POINTER( NULL ) );

        if( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == heapPROTECT_BLOCK_POINTER( NULL ) )
            {
                /*
                 * 该块将被归还给堆——它不再处于
                 * 已分配状态。
                 */
                heapFREE_BLOCK( pxLink );
                #if ( configHEAP_CLEAR_MEMORY_ON_FREE == 1 )
                {
                    /*
                     * 检查下溢，因为当堆块中的 xBlockSize
                     * 被覆盖时可能发生下溢。
                     */
                    if( heapSUBTRACT_WILL_UNDERFLOW( pxLink->xBlockSize, xHeapStructSize ) == 0 )
                    {
                        ( void ) memset( puc + xHeapStructSize, 0, pxLink->xBlockSize - xHeapStructSize );
                    }
                }
                #endif

                vTaskSuspendAll();
                {
                    /* 将该块加入空闲块链表。 */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                    xNumberOfSuccessfulFrees++;
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void xPortResetHeapMinimumEverFreeHeapSize( void )
{
    xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void * pvPortCalloc( size_t xNum,
                     size_t xSize )
{
    void * pv = NULL;

    if( heapMULTIPLY_WILL_OVERFLOW( xNum, xSize ) == 0 )
    {
        pv = pvPortMalloc( xNum * xSize );

        if( pv != NULL )
        {
            ( void ) memset( pv, 0, xNum * xSize );
        }
    }

    return pv;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert ) /* 特权函数 */
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /*
     * 遍历链表，直到找到地址高于待插入块的块。
     */
    for( pxIterator = &xStart; heapPROTECT_BLOCK_POINTER( pxIterator->pxNextFreeBlock ) < pxBlockToInsert; pxIterator = heapPROTECT_BLOCK_POINTER( pxIterator->pxNextFreeBlock ) )
    {
        /* 此处无需执行操作，只需迭代到正确位置。 */
    }

    if( pxIterator != &xStart )
    {
        heapVALIDATE_BLOCK_POINTER( pxIterator );
    }

    /*
     * 待插入块与它将被插入其后的那个块
     * 是否构成连续的内存块？
     */
    puc = ( uint8_t * ) pxIterator;

    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /*
     * 待插入块与它将被插入其前的那个块
     * 是否构成连续的内存块？
     */
    puc = ( uint8_t * ) pxBlockToInsert;

    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) heapPROTECT_BLOCK_POINTER( pxIterator->pxNextFreeBlock ) )
    {
        if( heapPROTECT_BLOCK_POINTER( pxIterator->pxNextFreeBlock ) != pxEnd )
        {
            /* 将两个块合并为一个大块。 */
            pxBlockToInsert->xBlockSize += heapPROTECT_BLOCK_POINTER( pxIterator->pxNextFreeBlock )->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( pxIterator->pxNextFreeBlock )->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( pxEnd );
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /*
     * 如果待插入块填补了一个空隙，即同时与前后的块合并，
     * 那么它的 pxNextFreeBlock 指针已经设置，不应在此处再次设置，
     * 否则会指向自身。
     */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( pxBlockToInsert );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions ) /* 特权函数 */
{
    BlockLink_t * pxFirstFreeBlockInRegion = NULL;
    BlockLink_t * pxPreviousFreeBlock;
    portPOINTER_SIZE_TYPE xAlignedHeap;
    size_t xTotalRegionSize, xTotalHeapSize = 0;
    BaseType_t xDefinedRegions = 0;
    portPOINTER_SIZE_TYPE xAddress;
    const HeapRegion_t * pxHeapRegion;

    /* 只能调用一次！ */
    configASSERT( pxEnd == NULL );

    #if ( configENABLE_HEAP_PROTECTOR == 1 )
    {
        vApplicationGetRandomHeapCanary( &( xHeapCanary ) );
    }
    #endif

    pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

    while( pxHeapRegion->xSizeInBytes > 0 )
    {
        xTotalRegionSize = pxHeapRegion->xSizeInBytes;

        /* 确保堆区域起始于正确对齐的边界。 */
        xAddress = ( portPOINTER_SIZE_TYPE ) pxHeapRegion->pucStartAddress;

        if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
        {
            xAddress += ( portBYTE_ALIGNMENT - 1 );
            xAddress &= ~( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK;

            /* 根据对齐损失调整大小。 */
            xTotalRegionSize -= ( size_t ) ( xAddress - ( portPOINTER_SIZE_TYPE ) pxHeapRegion->pucStartAddress );
        }

        xAlignedHeap = xAddress;

        /* 如果 xStart 尚未设置，则设置它。 */
        if( xDefinedRegions == 0 )
        {
            /*
             * xStart 用于保存指向空闲块链表首项的指针。
             * 使用 void 强制转换可避免编译器警告。
             */
            xStart.pxNextFreeBlock = ( BlockLink_t * ) heapPROTECT_BLOCK_POINTER( xAlignedHeap );
            xStart.xBlockSize = ( size_t ) 0;
        }
        else
        {
            /*
             * 只有在已有一个区域添加到堆中时才会执行到这里。
             */
            configASSERT( pxEnd != heapPROTECT_BLOCK_POINTER( NULL ) );

            /* 检查传入块的起始地址是否递增。 */
            configASSERT( ( size_t ) xAddress > ( size_t ) pxEnd );
        }

        #if ( configENABLE_HEAP_PROTECTOR == 1 )
        {
            if( ( pucHeapLowAddress == NULL ) ||
                ( ( uint8_t * ) xAlignedHeap < pucHeapLowAddress ) )
            {
                pucHeapLowAddress = ( uint8_t * ) xAlignedHeap;
            }
        }
        #endif /* 配置项 configENABLE_HEAP_PROTECTOR */

        /*
         * 记住上一个区域中结束标记的位置（如果存在）。
         */
        pxPreviousFreeBlock = pxEnd;

        /*
         * pxEnd 用于标记空闲块链表的末尾，
         * 并被插入到区域空间的末尾。
         */
        xAddress = xAlignedHeap + ( portPOINTER_SIZE_TYPE ) xTotalRegionSize;
        xAddress -= ( portPOINTER_SIZE_TYPE ) xHeapStructSize;
        xAddress &= ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK );
        pxEnd = ( BlockLink_t * ) xAddress;
        pxEnd->xBlockSize = 0;
        pxEnd->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( NULL );

        /*
         * 初始时该区域只有一个空闲块，其大小占据
         * 整个堆区域减去空闲块结构体占用的空间。
         */
        pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
        pxFirstFreeBlockInRegion->xBlockSize = ( size_t ) ( xAddress - ( portPOINTER_SIZE_TYPE ) pxFirstFreeBlockInRegion );
        pxFirstFreeBlockInRegion->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( pxEnd );

        /*
         * 如果这不是构成整个堆空间的第一个区域，
         * 则将上一个区域链接到本区域。
         */
        if( pxPreviousFreeBlock != NULL )
        {
            pxPreviousFreeBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER( pxFirstFreeBlockInRegion );
        }

        xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

        #if ( configENABLE_HEAP_PROTECTOR == 1 )
        {
            if( ( pucHeapHighAddress == NULL ) ||
                ( ( ( ( uint8_t * ) pxFirstFreeBlockInRegion ) + pxFirstFreeBlockInRegion->xBlockSize ) > pucHeapHighAddress ) )
            {
                pucHeapHighAddress = ( ( uint8_t * ) pxFirstFreeBlockInRegion ) + pxFirstFreeBlockInRegion->xBlockSize;
            }
        }
        #endif

        /* 移动到下一个 HeapRegion_t 结构体。 */
        xDefinedRegions++;
        pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
    }

    xMinimumEverFreeBytesRemaining = xTotalHeapSize;
    xFreeBytesRemaining = xTotalHeapSize;

    /* 在访问前检查确实已定义内容。 */
    configASSERT( xTotalHeapSize );
}
/*-----------------------------------------------------------*/

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
    BlockLink_t * pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = SIZE_MAX;

    vTaskSuspendAll();
    {
        pxBlock = heapPROTECT_BLOCK_POINTER( xStart.pxNextFreeBlock );

        /*
         * 如果堆尚未初始化，pxBlock 将为 NULL。
         * 首次分配时会自动初始化堆。
         */
        if( pxBlock != NULL )
        {
            while( pxBlock != pxEnd )
            {
                /*
                 * 递增块计数，并记录目前遇到的最大块。
                 */
                xBlocks++;

                if( pxBlock->xBlockSize > xMaxSize )
                {
                    xMaxSize = pxBlock->xBlockSize;
                }

                /*
                 * heap_5 在每个区域的末尾都有一个大小为零的块——
                 * 该块仅用于链接到下一个堆区域，
                 * 并不是真正的块。
                 */
                if( pxBlock->xBlockSize != 0 )
                {
                    if( pxBlock->xBlockSize < xMinSize )
                    {
                        xMinSize = pxBlock->xBlockSize;
                    }
                }

                /*
                 * 沿链移动到下一个块，直到到达最后一个块。
                 */
                pxBlock = heapPROTECT_BLOCK_POINTER( pxBlock->pxNextFreeBlock );
            }
        }
    }
    ( void ) xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。该状态通常在启动时初始化。
 * 应用程序必须在重启调度器之前调用此函数。
 */
void vPortHeapResetState( void )
{
    pxEnd = NULL;

    xFreeBytesRemaining = ( size_t ) 0U;
    xMinimumEverFreeBytesRemaining = ( size_t ) 0U;
    xNumberOfSuccessfulAllocations = ( size_t ) 0U;
    xNumberOfSuccessfulFrees = ( size_t ) 0U;

    #if ( configENABLE_HEAP_PROTECTOR == 1 )
        pucHeapHighAddress = NULL;
        pucHeapLowAddress = NULL;
    #endif /* 条件块结束：#if ( configENABLE_HEAP_PROTECTOR == 1 ) */
}
/*-----------------------------------------------------------*/
