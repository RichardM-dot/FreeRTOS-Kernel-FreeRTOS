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
 * pvPortMalloc() 与 vPortFree() 的示例实现，允许释放已分配的块，
 * 但不会将相邻的空闲块合并为一个更大的块（因此会造成内存碎片）。
 * heap_4.c 提供了可将相邻块合并为更大块的等效实现。
 *
 * 替代实现参见 heap_1.c、heap_3.c 和 heap_4.c，
 * 更多信息参见 https://www.FreeRTOS.org 的内存管理页面。
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

/* 堆起始地址进行字节对齐时可能会损失几个字节。 */
#define configADJUSTED_HEAP_SIZE    ( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/* 假定字节为 8 位！ */
#define heapBITS_PER_BYTE           ( ( size_t ) 8 )

/* size_t 类型能容纳的最大值。 */
#define heapSIZE_MAX                ( ~( ( size_t ) 0 ) )

/* 检查 a 与 b 相乘是否会溢出。 */
#define heapMULTIPLY_WILL_OVERFLOW( a, b )    ( ( ( a ) > 0 ) && ( ( b ) > ( heapSIZE_MAX / ( a ) ) ) )

/* 检查 a 与 b 相加是否会溢出。 */
#define heapADD_WILL_OVERFLOW( a, b )         ( ( a ) > ( heapSIZE_MAX - ( b ) ) )

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

/*-----------------------------------------------------------*/

/* 为堆分配内存。 */
#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )

/*
 * 应用程序编写者已自行定义 RTOS 堆所用的数组——
 * 通常是为了将其放置在特定的段或地址。
 */
    extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
    PRIVILEGED_DATA static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* 配置项 configAPPLICATION_ALLOCATED_HEAP */


/*
 * 定义链表结构体，用于按块大小顺序链接空闲块。
 */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /* 链表中的下一个空闲块。 */
    size_t xBlockSize;                     /* 空闲块的大小。 */
} BlockLink_t;


static const size_t xHeapStructSize = ( ( sizeof( BlockLink_t ) + ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK ) );
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize * 2 ) )

/* 创建两个链表节点，用于标记链表的起始和结束。 */
PRIVILEGED_DATA static BlockLink_t xStart, xEnd;

/*
 * 记录剩余空闲字节数，但不反映碎片情况。
 */
PRIVILEGED_DATA static size_t xFreeBytesRemaining = configADJUSTED_HEAP_SIZE;

/* 指示堆是否已初始化。 */
PRIVILEGED_DATA static BaseType_t xHeapHasBeenInitialised = pdFALSE;

/*-----------------------------------------------------------*/

/*
 * 在首次使用前初始化堆结构。
 */
static void prvHeapInit( void ) PRIVILEGED_FUNCTION;

/*-----------------------------------------------------------*/

/* 静态函数以宏的形式定义，以尽量减小函数调用深度。 */

/*
 * 将块插入空闲块链表——该链表按块大小排序。
 * 小块位于链表头部，大块位于链表尾部。
 */
#define prvInsertBlockIntoFreeList( pxBlockToInsert )                                                                               \
    {                                                                                                                               \
        BlockLink_t * pxIterator;                                                                                                   \
        size_t xBlockSize;                                                                                                          \
                                                                                                                                    \
        xBlockSize = pxBlockToInsert->xBlockSize;                                                                                   \
                                                                                                                                    \
        /* Iterate through the list until a block is found that has a larger size */                                                \
        /* than the block we are inserting. */                                                                                      \
        for( pxIterator = &xStart; pxIterator->pxNextFreeBlock->xBlockSize < xBlockSize; pxIterator = pxIterator->pxNextFreeBlock ) \
        {                                                                                                                           \
            /* There is nothing to do here - just iterate to the correct position. */                                               \
        }                                                                                                                           \
                                                                                                                                    \
        /* Update the list to include the block being inserted in the correct */                                                    \
        /* position. */                                                                                                             \
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;                                                             \
        pxIterator->pxNextFreeBlock = pxBlockToInsert;                                                                              \
    }
/*-----------------------------------------------------------*/

void * pvPortMalloc( size_t xWantedSize )
{
    BlockLink_t * pxBlock;
    BlockLink_t * pxPreviousBlock;
    BlockLink_t * pxNewBlockLink;
    void * pvReturn = NULL;
    size_t xAdditionalRequiredSize;
    size_t xAllocatedBlockSize = 0;

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
         * 如果这是首次调用 malloc，则需要初始化堆，
         * 以建立空闲块链表。
         */
        if( xHeapHasBeenInitialised == pdFALSE )
        {
            prvHeapInit();
            xHeapHasBeenInitialised = pdTRUE;
        }

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
                 * 块按大小顺序存储——从链表起始处（最小块）开始遍历，
                 * 直到找到大小足够的块。
                 */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* 如果遍历到结束标记，则说明未找到大小足够的块。 */
                if( pxBlock != &xEnd )
                {
                    /*
                     * 返回内存空间——跳过其起始处的 BlockLink_t 结构体。
                     */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    /*
                     * 该块将被返回使用，因此必须从空闲块链表中移除。
                     */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* 如果块大于所需大小，可将其拆分为两个块。 */
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /*
                         * 该块将被拆分为两个。按请求的字节数在其后创建新块。
                         * 使用 void 强制转换可避免编译器产生字节对齐警告。
                         */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

                        /*
                         * 计算从单个块拆分出的两个块的大小。
                         */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /*
                         * 将新块插入空闲块链表。
                         * 空闲块链表按大小排序，必须迭代查找
                         * 新块的正确插入位置。
                         */
                        prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    xAllocatedBlockSize = pxBlock->xBlockSize;

                    /*
                     * 该块将被返回——它已分配并归应用程序所有，
                     * 且没有“下一个”块。
                     */
                    heapALLOCATE_BLOCK( pxBlock );
                    pxBlock->pxNextFreeBlock = NULL;
                }
            }
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
    }
    #endif

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

        /*
         * 此处看似意外的强制转换是为了避免某些编译器产生字节对齐警告。
         */
        pxLink = ( void * ) puc;

        configASSERT( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /*
                 * 该块将被归还给堆——它不再处于已分配状态。
                 */
                heapFREE_BLOCK( pxLink );
                #if ( configHEAP_CLEAR_MEMORY_ON_FREE == 1 )
                {
                    ( void ) memset( puc + xHeapStructSize, 0, pxLink->xBlockSize - xHeapStructSize );
                }
                #endif

                vTaskSuspendAll();
                {
                    /* 将该块加入空闲块链表。 */
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                }
                ( void ) xTaskResumeAll();
            }
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* 此函数仅用于消除链接器警告。 */
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

static void prvHeapInit( void ) /* 特权函数 */
{
    BlockLink_t * pxFirstFreeBlock;
    uint8_t * pucAlignedHeap;

    /* 确保堆起始于正确对齐的边界。 */
    pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE ) & ucHeap[ portBYTE_ALIGNMENT - 1 ] ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

    /*
     * xStart 用于保存指向空闲块链表首项的指针。
     * 使用 void 强制转换可避免编译器警告。
     */
    xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart.xBlockSize = ( size_t ) 0;

    /* xEnd 用于标记空闲块链表的末尾。 */
    xEnd.xBlockSize = configADJUSTED_HEAP_SIZE;
    xEnd.pxNextFreeBlock = NULL;

    /*
     * 初始时只有一个空闲块，其大小占据整个堆空间。
     */
    pxFirstFreeBlock = ( BlockLink_t * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = configADJUSTED_HEAP_SIZE;
    pxFirstFreeBlock->pxNextFreeBlock = &xEnd;
}
/*-----------------------------------------------------------*/

/*
 * 重置本文件中的状态。该状态通常在启动时初始化。
 * 应用程序必须在重启调度器之前调用此函数。
 */
void vPortHeapResetState( void )
{
    xFreeBytesRemaining = configADJUSTED_HEAP_SIZE;

    xHeapHasBeenInitialised = pdFALSE;
}
/*-----------------------------------------------------------*/
