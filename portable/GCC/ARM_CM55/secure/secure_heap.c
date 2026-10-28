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

/* 标准头文件包含。 */
#include <stdint.h>

/* 配置包含文件。 */
#include "FreeRTOSConfig.h"

/* 安全上下文堆包含文件。 */
#include "secure_heap.h"

/* 安全移植层宏。 */
#include "secure_port_macros.h"

/**
 * @brief 堆总大小。
 */
#ifndef secureconfigTOTAL_HEAP_SIZE
    #define secureconfigTOTAL_HEAP_SIZE    ( ( ( size_t ) ( 10 * 1024 ) ) )
#endif

/* 默认不使用测试标记。 */
#ifndef mtCOVERAGE_TEST_MARKER
    #define mtCOVERAGE_TEST_MARKER()
#endif

/* 默认不启用跟踪。 */
#ifndef traceMALLOC
    #define traceMALLOC( pvReturn, xWantedSize )
#endif

/* 默认不启用跟踪。 */
#ifndef traceFREE
    #define traceFREE( pv, xBlockSize )
#endif

/* 块大小不得过小。 */
#define secureheapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* 假定字节为 8 位！ */
#define secureheapBITS_PER_BYTE         ( ( size_t ) 8 )

/* size_t 类型能容纳的最大值。 */
#define secureheapSIZE_MAX              ( ~( ( size_t ) 0 ) )

/* 检查 a 与 b 相加是否会溢出。 */
#define secureheapADD_WILL_OVERFLOW( a, b )    ( ( a ) > ( secureheapSIZE_MAX - ( b ) ) )

/*
 * BlockLink_t 结构体 xBlockSize 成员的最高位（MSB）用于跟踪块的分配状态。
 * 当 BlockLink_t 结构体 xBlockSize 成员的最高位被置位时，该块属于应用程序；
 * 当该位空闲（为 0）时，该块仍属于空闲堆空间。
 */
#define secureheapBLOCK_ALLOCATED_BITMASK    ( ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * secureheapBITS_PER_BYTE ) - 1 ) )
#define secureheapBLOCK_SIZE_IS_VALID( xBlockSize )    ( ( ( xBlockSize ) & secureheapBLOCK_ALLOCATED_BITMASK ) == 0 )
#define secureheapBLOCK_IS_ALLOCATED( pxBlock )        ( ( ( pxBlock->xBlockSize ) & secureheapBLOCK_ALLOCATED_BITMASK ) != 0 )
#define secureheapALLOCATE_BLOCK( pxBlock )            ( ( pxBlock->xBlockSize ) |= secureheapBLOCK_ALLOCATED_BITMASK )
#define secureheapFREE_BLOCK( pxBlock )                ( ( pxBlock->xBlockSize ) &= ~secureheapBLOCK_ALLOCATED_BITMASK )
/*-----------------------------------------------------------*/

/* 为堆分配内存。 */
#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )

/*
 * 应用程序编写者已自行定义 RTOS 堆所用的数组——
 * 通常是为了将其放置在特定的段或地址。
 */
    extern uint8_t ucHeap[ secureconfigTOTAL_HEAP_SIZE ];
#else /* 配置项 configAPPLICATION_ALLOCATED_HEAP */
    static uint8_t ucHeap[ secureconfigTOTAL_HEAP_SIZE ];
#endif /* 配置项 configAPPLICATION_ALLOCATED_HEAP */

/**
 * @brief 链表结构体。
 *
 * 用于按内存地址顺序链接空闲内存块。
 */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /**< 链表中的下一个空闲块。 */
    size_t xBlockSize;                     /**< 空闲块的大小。 */
} BlockLink_t;
/*-----------------------------------------------------------*/

/**
 * @brief 首次调用 pvPortMalloc() 时自动调用，用于建立所需的堆结构。
 */
static void prvHeapInit( void );

/**
 * @brief 将一个正在释放的内存块插入空闲内存块链表中的正确位置。
 *
 * 若内存块彼此相邻，被释放的块会与其前面和/或后面的块合并。
 *
 * @param[in] pxBlockToInsert 正在释放的内存块。
 */
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert );
/*-----------------------------------------------------------*/

/**
 * @brief 放置在每个已分配内存块起始处的结构体大小必须正确按字节对齐。
 */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( secureportBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) secureportBYTE_ALIGNMENT_MASK );

/**
 * @brief 创建两个链表节点，用于标记链表的起始和结束。
 */
static BlockLink_t xStart;
static BlockLink_t * pxEnd = NULL;

/**
 * @brief 记录剩余空闲字节数，但不反映碎片情况。
 */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/*-----------------------------------------------------------*/

static void prvHeapInit( void )
{
    BlockLink_t * pxFirstFreeBlock;
    uint8_t * pucAlignedHeap;
    size_t uxAddress;
    size_t xTotalHeapSize = secureconfigTOTAL_HEAP_SIZE;

    /* 确保堆起始于正确对齐的边界。 */
    uxAddress = ( size_t ) ucHeap;

    if( ( uxAddress & secureportBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxAddress += ( secureportBYTE_ALIGNMENT - 1 );
        uxAddress &= ~( ( size_t ) secureportBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
    }

    pucAlignedHeap = ( uint8_t * ) uxAddress;

    /*
     * xStart 用于保存指向空闲块链表首项的指针。
     * 使用 void 强制转换可避免编译器警告。
     */
    xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart.xBlockSize = ( size_t ) 0;

    /*
     * pxEnd 用于标记空闲块链表的末尾，
     * 并被插入到堆空间的末尾。
     */
    uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~( ( size_t ) secureportBYTE_ALIGNMENT_MASK );
    pxEnd = ( void * ) uxAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /*
     * 初始时只有一个空闲块，其大小占据整个堆空间，
     * 减去 pxEnd 占用的空间。
     */
    pxFirstFreeBlock = ( void * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* 仅存在一个块——它覆盖整个可用堆空间。 */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert )
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /*
     * 遍历链表，直到找到地址高于待插入块的块。
     */
    for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* 此处无需执行操作，只需迭代到正确位置。 */
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

    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
    {
        if( pxIterator->pxNextFreeBlock != pxEnd )
        {
            /* 将两个块合并为一个大块。 */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd;
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
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
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

    /*
     * 如果这是首次调用 malloc，则需要初始化堆，
     * 以建立空闲块链表。
     */
    if( pxEnd == NULL )
    {
        prvHeapInit();
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    if( xWantedSize > 0 )
    {
        /*
         * 需要增大请求大小，使其除请求的字节数外
         * 还能容纳一个 BlockLink_t 结构体。
         */
        if( secureheapADD_WILL_OVERFLOW( xWantedSize, xHeapStructSize ) == 0 )
        {
            xWantedSize += xHeapStructSize;

            /*
             * 确保块始终按要求的字节数对齐。
             */
            if( ( xWantedSize & secureportBYTE_ALIGNMENT_MASK ) != 0x00 )
            {
                /* 需要进行字节对齐。 */
                xAdditionalRequiredSize = secureportBYTE_ALIGNMENT - ( xWantedSize & secureportBYTE_ALIGNMENT_MASK );

                if( secureheapADD_WILL_OVERFLOW( xWantedSize, xAdditionalRequiredSize ) == 0 )
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

    /*
     * 检查请求的块大小不至于大到使最高位被置位。
     * BlockLink_t 结构体中块大小成员的最高位用于判断块的归属——
     * 应用程序还是内核，因此该位必须为空闲（0）。
     */
    if( secureheapBLOCK_SIZE_IS_VALID( xWantedSize ) != 0 )
    {
        if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
        {
            /*
             * 从链表起始处（最低地址）开始遍历，
             * 直到找到大小足够的块。
             */
            pxPreviousBlock = &xStart;
            pxBlock = xStart.pxNextFreeBlock;

            while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
            {
                pxPreviousBlock = pxBlock;
                pxBlock = pxBlock->pxNextFreeBlock;
            }

            /*
             * 若到达结束标记，则说明未找到大小合适的内存块。
             */
            if( pxBlock != pxEnd )
            {
                /*
                 * 返回所指向的内存空间——跳过其起始处的
                 * BlockLink_t 结构体。
                 */
                pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                /*
                 * 该块将被返回使用，因此必须从空闲块链表中
                 * 移除。
                 */
                pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                /*
                 * 如果块大于所需大小，可将其拆分为
                 * 两个块。
                 */
                if( ( pxBlock->xBlockSize - xWantedSize ) > secureheapMINIMUM_BLOCK_SIZE )
                {
                    /*
                     * 该块将被拆分为两个。按请求的字节数在其后创建
                     * 新块。使用 void 强制转换可避免编译器
                     * 产生字节对齐警告。
                     */
                    pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
                    secureportASSERT( ( ( ( size_t ) pxNewBlockLink ) & secureportBYTE_ALIGNMENT_MASK ) == 0 );

                    /*
                     * 计算从单个块拆分出的两个块的大小。
                     */
                    pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                    pxBlock->xBlockSize = xWantedSize;

                    /* 将新块插入空闲块链表。 */
                    pxNewBlockLink->pxNextFreeBlock = pxPreviousBlock->pxNextFreeBlock;
                    pxPreviousBlock->pxNextFreeBlock = pxNewBlockLink;
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
                 * 该内存块正在被归还——它已分配且归应用程序所有，没有“next”块。
                 */
                secureheapALLOCATE_BLOCK( pxBlock );
                pxBlock->pxNextFreeBlock = NULL;
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

    #if ( secureconfigUSE_MALLOC_FAILED_HOOK == 1 )
    {
        if( pvReturn == NULL )
        {
            extern void vApplicationMallocFailedHook( void );
            vApplicationMallocFailedHook();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif /* if ( secureconfigUSE_MALLOC_FAILED_HOOK == 1 )（启用内存分配失败钩子） */

    secureportASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) secureportBYTE_ALIGNMENT_MASK ) == 0 );
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

        /* 检查该内存块确实已分配。 */
        secureportASSERT( secureheapBLOCK_IS_ALLOCATED( pxLink ) != 0 );
        secureportASSERT( pxLink->pxNextFreeBlock == NULL );

        if( secureheapBLOCK_IS_ALLOCATED( pxLink ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /*
                 * 该块将被归还给堆——它不再处于
                 * 已分配状态。
                 */
                secureheapFREE_BLOCK( pxLink );

                secureportDISABLE_NON_SECURE_INTERRUPTS();
                {
                    /* 将该块加入空闲块链表。 */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                }
                secureportENABLE_NON_SECURE_INTERRUPTS();
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
