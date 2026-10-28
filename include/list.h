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
 * 调度器使用的链表实现。虽然它针对调度器的需求做了大量定制，
 * 但应用程序代码同样可以使用。
 *
 * List_t 只能存储指向 ListItem_t 的指针。每个 ListItem_t 都包含一个
 * 数值（xItemValue）。大多数情况下，链表按项值升序排列。
 *
 * 链表创建时已包含一个链表项。该项的值是可存储的最大值，
 * 因此它始终位于链表末尾，并充当标记。链表成员 pxHead 始终
 * 指向该标记——即使它位于链表尾部。这是因为尾部包含一个
 * 回绕指针，指向链表真正的头部。
 *
 * 除数值外，每个链表项还包含指向链表中下一项的指针（pxNext）、
 * 指向其所属链表的指针（pxContainer），以及指回包含该项的对象的
 * 指针。后两个指针用于提高链表操作效率。包含链表项的对象与
 * 链表项本身之间实际上形成了双向链接。
 *
 *
 * \page ListIntroduction 链表实现
 * \ingroup FreeRTOSIntro
 */


#ifndef LIST_H
#define LIST_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before list.h"
#endif

/*
 * 链表结构成员会在中断中被修改，因此按理应声明为 volatile。
 * 不过，它们只以功能上原子的方式被修改（在临界区内，或在调度器
 * 挂起时），并且要么以引用方式传入函数，要么通过 volatile 变量索引。
 * 因此，在迄今为止测试的所有用例中，都可以省略 volatile 限定符，
 * 从而在不影响功能行为的前提下获得适度的性能提升。已检查 IAR、ARM
 * 和 GCC 编译器在各自选项设置为最高优化级别时生成的汇编指令，
 * 确认其符合预期。话虽如此，随着编译器技术的发展，特别是在使用
 * 激进的跨模块优化时（该用例尚未经过大规模验证），可能仍需要
 * volatile 限定符才能正确优化。预计如果编译器在链表结构成员未加
 * volatile 限定符且启用激进跨模块优化的情况下删除了必要代码，
 * 将导致调度器出现彻底而明显的故障。如果遇到这种情况，可在
 * FreeRTOSConfig.h 中把 configLIST_VOLATILE 定义为 volatile
 * （参照本注释块底部的示例），从而在链表结构的相应位置插入
 * volatile 限定符。如果未定义 configLIST_VOLATILE，则下面的
 * 预处理指令会将 configLIST_VOLATILE 完全 #define 为空。
 *
 * 要使用 volatile 链表结构成员，请在 FreeRTOSConfig.h 中添加以下行
 * （不含引号）：
 * "#define configLIST_VOLATILE volatile"
 */
#ifndef configLIST_VOLATILE
    #define configLIST_VOLATILE
#endif /* 配置项 configLIST_VOLATILE */

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

/*
 * 可用于在链表结构中放置已知值，然后在应用程序执行期间
 * 检查这些已知值是否被破坏的宏。这些宏可能发现链表数据结构
 * 被内存覆写的情况。它们无法发现由 FreeRTOS 配置或使用不当
 * 导致的数据错误。
 */
#if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 0 )
    /* 定义这些宏为空操作。 */
    #define listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE
    #define listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE
    #define listFIRST_LIST_INTEGRITY_CHECK_VALUE
    #define listSECOND_LIST_INTEGRITY_CHECK_VALUE
    #define listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem )
    #define listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem )
    #define listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList )
    #define listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList )
    #define listTEST_LIST_ITEM_INTEGRITY( pxItem )
    #define listTEST_LIST_INTEGRITY( pxList )
#else /* 条件不成立：if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 0 ) */
    /* 定义向链表结构中添加新成员的宏。 */
    #define listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE     TickType_t xListItemIntegrityValue1;
    #define listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE    TickType_t xListItemIntegrityValue2;
    #define listFIRST_LIST_INTEGRITY_CHECK_VALUE          TickType_t xListIntegrityValue1;
    #define listSECOND_LIST_INTEGRITY_CHECK_VALUE         TickType_t xListIntegrityValue2;

/* 定义将新增结构成员设置为已知值的宏。 */
    #define listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem )     ( pxItem )->xListItemIntegrityValue1 = pdINTEGRITY_CHECK_VALUE
    #define listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem )    ( pxItem )->xListItemIntegrityValue2 = pdINTEGRITY_CHECK_VALUE
    #define listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList )              ( pxList )->xListIntegrityValue1 = pdINTEGRITY_CHECK_VALUE
    #define listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList )              ( pxList )->xListIntegrityValue2 = pdINTEGRITY_CHECK_VALUE

/*
 * 定义在某个结构成员不等于其预期值时触发断言的宏。
 */
    #define listTEST_LIST_ITEM_INTEGRITY( pxItem )                      configASSERT( ( ( pxItem )->xListItemIntegrityValue1 == pdINTEGRITY_CHECK_VALUE ) && ( ( pxItem )->xListItemIntegrityValue2 == pdINTEGRITY_CHECK_VALUE ) )
    #define listTEST_LIST_INTEGRITY( pxList )                           configASSERT( ( ( pxList )->xListIntegrityValue1 == pdINTEGRITY_CHECK_VALUE ) && ( ( pxList )->xListIntegrityValue2 == pdINTEGRITY_CHECK_VALUE ) )
#endif /* 配置项 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES */


/*
 * 链表唯一可包含的对象类型的定义。
 */
struct xLIST;
struct xLIST_ITEM
{
    listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE           /* 当 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 为 1 时设置为已知值。 */
    configLIST_VOLATILE TickType_t xItemValue;          /* 链表项的值。大多数情况下用于按升序对链表排序。 */
    struct xLIST_ITEM * configLIST_VOLATILE pxNext;     /* 指向链表中下一个 ListItem_t 的指针。 */
    struct xLIST_ITEM * configLIST_VOLATILE pxPrevious; /* 指向链表中上一个 ListItem_t 的指针。 */
    void * pvOwner;                                     /* 指向包含该链表项的对象（通常是 TCB）的指针。因此，包含链表项的对象与该链表项之间形成双向链接。 */
    struct xLIST * configLIST_VOLATILE pxContainer;     /* 指向该链表项所在链表的指针（如果有）。 */
    listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE          /* 当 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 为 1 时设置为已知值。 */
};
typedef struct xLIST_ITEM ListItem_t;

#if ( configUSE_MINI_LIST_ITEM == 1 )
    struct xMINI_LIST_ITEM
    {
        listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE /* 当 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 为 1 时设置为已知值。 */
        configLIST_VOLATILE TickType_t xItemValue;
        struct xLIST_ITEM * configLIST_VOLATILE pxNext;
        struct xLIST_ITEM * configLIST_VOLATILE pxPrevious;
    };
    typedef struct xMINI_LIST_ITEM MiniListItem_t;
#else
    typedef struct xLIST_ITEM      MiniListItem_t;
#endif

/*
 * 调度器使用的队列类型的定义。
 */
typedef struct xLIST
{
    listFIRST_LIST_INTEGRITY_CHECK_VALUE      /* 当 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 为 1 时设置为已知值。 */
    configLIST_VOLATILE UBaseType_t uxNumberOfItems;
    ListItem_t * configLIST_VOLATILE pxIndex; /* 用于遍历链表。指向 listGET_OWNER_OF_NEXT_ENTRY() 上次返回的项。 */
    MiniListItem_t xListEnd;                  /* 包含最大可能项值的链表项，意味着它始终位于链表末尾，因此用作标记。 */
    listSECOND_LIST_INTEGRITY_CHECK_VALUE     /* 当 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 为 1 时设置为已知值。 */
} List_t;

/*
 * 用于设置链表项所有者的访问宏。链表项的所有者是
 * 包含该链表项的对象（通常是 TCB）。
 *
 * \page listSET_LIST_ITEM_OWNER listSET_LIST_ITEM_OWNER
 * \ingroup LinkedList
 */
#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner )    ( ( pxListItem )->pvOwner = ( void * ) ( pxOwner ) )

/*
 * 用于获取链表项所有者的访问宏。链表项的所有者是
 * 包含该链表项的对象（通常是 TCB）。
 *
 * \page listGET_LIST_ITEM_OWNER listGET_LIST_ITEM_OWNER
 * \ingroup LinkedList
 */
#define listGET_LIST_ITEM_OWNER( pxListItem )             ( ( pxListItem )->pvOwner )

/*
 * 用于设置链表项值的访问宏。大多数情况下，该值
 * 用于按升序对链表排序。
 *
 * \page listSET_LIST_ITEM_VALUE listSET_LIST_ITEM_VALUE
 * \ingroup LinkedList
 */
#define listSET_LIST_ITEM_VALUE( pxListItem, xValue )     ( ( pxListItem )->xItemValue = ( xValue ) )

/*
 * 用于获取链表项值的访问宏。该值可以表示任意内容——
 * 例如任务的优先级，或任务应解除阻塞的时间。
 *
 * \page listGET_LIST_ITEM_VALUE listGET_LIST_ITEM_VALUE
 * \ingroup LinkedList
 */
#define listGET_LIST_ITEM_VALUE( pxListItem )             ( ( pxListItem )->xItemValue )

/*
 * 用于获取给定链表头部项值的访问宏。
 *
 * \page listGET_ITEM_VALUE_OF_HEAD_ENTRY listGET_ITEM_VALUE_OF_HEAD_ENTRY
 * \ingroup LinkedList
 */
#define listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxList )        ( ( ( pxList )->xListEnd ).pxNext->xItemValue )

/*
 * 返回链表头部的链表项。
 *
 * \page listGET_HEAD_ENTRY listGET_HEAD_ENTRY
 * \ingroup LinkedList
 */
#define listGET_HEAD_ENTRY( pxList )                      ( ( ( pxList )->xListEnd ).pxNext )

/*
 * 返回下一个链表项。
 *
 * \page listGET_NEXT listGET_NEXT
 * \ingroup LinkedList
 */
#define listGET_NEXT( pxListItem )                        ( ( pxListItem )->pxNext )

/*
 * 返回标记链表末尾的链表项。
 *
 * \page listGET_END_MARKER listGET_END_MARKER
 * \ingroup LinkedList
 */
#define listGET_END_MARKER( pxList )                      ( ( ListItem_t const * ) ( &( ( pxList )->xListEnd ) ) )

/*
 * 用于判断链表是否包含任何项的访问宏。仅当链表为空时，
 * 该宏的值才为 true。
 *
 * \page listLIST_IS_EMPTY listLIST_IS_EMPTY
 * \ingroup LinkedList
 */
#define listLIST_IS_EMPTY( pxList )                       ( ( ( pxList )->uxNumberOfItems == ( UBaseType_t ) 0 ) ? pdTRUE : pdFALSE )

/*
 * 返回链表中项数的访问宏。
 */
#define listCURRENT_LIST_LENGTH( pxList )                 ( ( pxList )->uxNumberOfItems )

/*
 * 获取链表中下一项所有者的访问函数。
 *
 * 链表成员 pxIndex 用于遍历链表。调用
 * listGET_OWNER_OF_NEXT_ENTRY 会将 pxIndex 递增到链表中的下一项，
 * 并返回该项的 pxOwner 参数。因此，通过多次调用此函数，
 * 可以遍历链表中包含的每一项。
 *
 * 链表项的 pxOwner 参数是指向拥有该链表项的对象的指针。
 * 在调度器中，这通常是任务控制块。pxOwner 参数实际上在链表项
 * 与其所有者之间建立了双向链接。
 *
 * @param pxTCB pxTCB 被设置为下一个链表项所有者的地址。
 * @param pxList 要返回下一项所有者的链表。
 *
 * \page listGET_OWNER_OF_NEXT_ENTRY listGET_OWNER_OF_NEXT_ENTRY
 * \ingroup LinkedList
 */
#if ( configNUMBER_OF_CORES == 1 )
    #define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )                                       \
    do {                                                                                       \
        List_t * const pxConstList = ( pxList );                                               \
        /* Increment the index to the next item and return the item, ensuring */               \
        /* we don't return the marker used at the end of the list.  */                         \
        ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;                           \
        if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) ) \
        {                                                                                      \
            ( pxConstList )->pxIndex = ( pxConstList )->xListEnd.pxNext;                       \
        }                                                                                      \
        ( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;                                         \
    } while( 0 )
#else /* 条件不成立：#if ( configNUMBER_OF_CORES == 1 ) */

/*
 * SMP 下不需要此函数。FreeRTOS SMP 调度器不使用
 * pxIndex，它应始终指向 xListEnd。此处不定义该宏，
 * 以避免更新 pxIndex。
 */
#endif /* 条件编译结束：#if ( configNUMBER_OF_CORES == 1 ) */

/*
 * 不返回值的 uxListRemove() 版本。以内联方式提供，
 * 作为 xTaskIncrementTick() 的轻微优化。
 *
 * 从链表中移除一个项。链表项包含指向其所在链表的指针，
 * 因此只需传入链表项即可。
 *
 * @param pxItemToRemove 要移除的项。该项将把自己从
 * 其 pxContainer 参数指向的链表中移除。
 *
 * @return 移除链表项后链表中剩余的项数。
 *
 * \page listREMOVE_ITEM listREMOVE_ITEM
 * \ingroup LinkedList
 */
#define listREMOVE_ITEM( pxItemToRemove ) \
    do {                                  \
        /* The list item knows which list it is in.  Obtain the list from the list \
         * item. */                                                                                 \
        List_t * const pxList = ( pxItemToRemove )->pxContainer;                                    \
                                                                                                    \
        ( pxItemToRemove )->pxNext->pxPrevious = ( pxItemToRemove )->pxPrevious;                    \
        ( pxItemToRemove )->pxPrevious->pxNext = ( pxItemToRemove )->pxNext;                        \
        /* Make sure the index is left pointing to a valid item. */                                 \
        if( pxList->pxIndex == ( pxItemToRemove ) )                                                 \
        {                                                                                           \
            pxList->pxIndex = ( pxItemToRemove )->pxPrevious;                                       \
        }                                                                                           \
                                                                                                    \
        ( pxItemToRemove )->pxContainer = NULL;                                                     \
        ( ( pxList )->uxNumberOfItems ) = ( UBaseType_t ) ( ( ( pxList )->uxNumberOfItems ) - 1U ); \
    } while( 0 )

/*
 * vListInsertEnd() 的内联版本，用于为 xTaskIncrementTick()
 * 提供轻微优化。
 *
 * 将链表项插入链表。插入位置使得在多次调用
 * listGET_OWNER_OF_NEXT_ENTRY 时，该项将是最后返回的项。
 *
 * 链表成员 pxIndex 用于遍历链表。调用
 * listGET_OWNER_OF_NEXT_ENTRY 会将 pxIndex 递增到链表中的下一项。
 * 使用 vListInsertEnd 将项放入链表，实际上是把该项放在
 * pxIndex 所指向的链表位置。这意味着在 pxIndex 参数再次指向
 * 被插入项之前，链表中所有其他项都会先由
 * listGET_OWNER_OF_NEXT_ENTRY 返回。
 *
 * @param pxList 要插入项的链表。
 *
 * @param pxNewListItem 要插入链表的链表项。
 *
 * \page listINSERT_END listINSERT_END
 * \ingroup LinkedList
 */
#define listINSERT_END( pxList, pxNewListItem )           \
    do {                                                  \
        ListItem_t * const pxIndex = ( pxList )->pxIndex; \
                                                          \
        /* Only effective when configASSERT() is also defined, these tests may catch \
         * the list data structures being overwritten in memory.  They will not catch \
         * data errors caused by incorrect configuration or use of FreeRTOS. */ \
        listTEST_LIST_INTEGRITY( ( pxList ) );                                  \
        listTEST_LIST_ITEM_INTEGRITY( ( pxNewListItem ) );                      \
                                                                                \
        /* Insert a new list item into ( pxList ), but rather than sort the list, \
         * makes the new list item the last item to be removed by a call to \
         * listGET_OWNER_OF_NEXT_ENTRY(). */                                                        \
        ( pxNewListItem )->pxNext = pxIndex;                                                        \
        ( pxNewListItem )->pxPrevious = pxIndex->pxPrevious;                                        \
                                                                                                    \
        pxIndex->pxPrevious->pxNext = ( pxNewListItem );                                            \
        pxIndex->pxPrevious = ( pxNewListItem );                                                    \
                                                                                                    \
        /* Remember which list the item is in. */                                                   \
        ( pxNewListItem )->pxContainer = ( pxList );                                                \
                                                                                                    \
        ( ( pxList )->uxNumberOfItems ) = ( UBaseType_t ) ( ( ( pxList )->uxNumberOfItems ) + 1U ); \
    } while( 0 )

/*
 * 获取链表中第一项所有者的访问函数。链表通常按项值升序排序。
 *
 * 此函数返回链表中第一项的 pxOwner 成员。
 * 链表项的 pxOwner 参数是指向拥有该链表项的对象的指针。
 * 在调度器中，这通常是任务控制块。pxOwner 参数实际上在链表项
 * 与其所有者之间建立了双向链接。
 *
 * @param pxList 要返回头部项所有者的链表。
 *
 * \page listGET_OWNER_OF_HEAD_ENTRY listGET_OWNER_OF_HEAD_ENTRY
 * \ingroup LinkedList
 */
#define listGET_OWNER_OF_HEAD_ENTRY( pxList )            ( ( &( ( pxList )->xListEnd ) )->pxNext->pvOwner )

/*
 * 检查链表项是否位于链表中。链表项维护一个
 * “容器”指针，指向其所在链表。此宏所做的
 * 只是检查该容器指针与链表是否匹配。
 *
 * @param pxList 要检查链表项是否位于其中的链表。
 * @param pxListItem 要检查是否在链表中的链表项。
 * @return 如果链表项在链表中则返回 pdTRUE，否则返回 pdFALSE。
 */
#define listIS_CONTAINED_WITHIN( pxList, pxListItem )    ( ( ( pxListItem )->pxContainer == ( pxList ) ) ? ( pdTRUE ) : ( pdFALSE ) )

/*
 * 返回链表项所属（被引用）的链表。
 *
 * @param pxListItem 被查询的链表项。
 * @return 指向引用 pxListItem 的 List_t 对象的指针
 */
#define listLIST_ITEM_CONTAINER( pxListItem )            ( ( pxListItem )->pxContainer )

/*
 * 这提供了一种粗略判断链表是否已初始化的方法，因为
 * vListInitialise() 函数会将 pxList->xListEnd.xItemValue
 * 设置为 portMAX_DELAY。
 */
#define listLIST_IS_INITIALISED( pxList )                ( ( pxList )->xListEnd.xItemValue == portMAX_DELAY )

/*
 * 必须在链表使用前调用！此函数初始化链表结构的所有
 * 成员，并将 xListEnd 项插入链表，作为链表尾部的标记。
 *
 * @param pxList 指向正在初始化的链表。
 *
 * \page vListInitialise vListInitialise
 * \ingroup LinkedList
 */
void vListInitialise( List_t * const pxList ) PRIVILEGED_FUNCTION;

/*
 * 必须在链表项使用前调用。此函数将链表容器设置为空，
 * 使该项不会认为自己已位于某个链表中。
 *
 * @param pxItem 指向正在初始化的链表项。
 *
 * \page vListInitialiseItem vListInitialiseItem
 * \ingroup LinkedList
 */
void vListInitialiseItem( ListItem_t * const pxItem ) PRIVILEGED_FUNCTION;

/*
 * 将链表项插入链表。该项将按其项值确定的位置
 * 插入链表（按项值升序）。
 *
 * @param pxList 要插入项的链表。
 *
 * @param pxNewListItem 要放入链表的项。
 *
 * \page vListInsert vListInsert
 * \ingroup LinkedList
 */
void vListInsert( List_t * const pxList,
                  ListItem_t * const pxNewListItem ) PRIVILEGED_FUNCTION;

/*
 * 将链表项插入链表。插入位置使得在多次调用
 * listGET_OWNER_OF_NEXT_ENTRY 时，该项将是最后返回的项。
 *
 * 链表成员 pxIndex 用于遍历链表。调用
 * listGET_OWNER_OF_NEXT_ENTRY 会将 pxIndex 递增到链表中的下一项。
 * 使用 vListInsertEnd 将项放入链表，实际上是把该项放在
 * pxIndex 所指向的链表位置。这意味着在 pxIndex 参数再次指向
 * 被插入项之前，链表中所有其他项都会先由
 * listGET_OWNER_OF_NEXT_ENTRY 返回。
 *
 * @param pxList 要插入项的链表。
 *
 * @param pxNewListItem 要插入链表的链表项。
 *
 * \page vListInsertEnd vListInsertEnd
 * \ingroup LinkedList
 */
void vListInsertEnd( List_t * const pxList,
                     ListItem_t * const pxNewListItem ) PRIVILEGED_FUNCTION;

/*
 * 从链表中移除一个项。链表项包含指向其所在链表的指针，
 * 因此只需传入链表项即可。
 *
 * @param pxItemToRemove 要移除的项。该项将把自己从
 * 其 pxContainer 参数指向的链表中移除。
 *
 * @return 移除链表项后链表中剩余的项数。
 *
 * \page uxListRemove uxListRemove
 * \ingroup LinkedList
 */
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove ) PRIVILEGED_FUNCTION;

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：ifndef LIST_H */
