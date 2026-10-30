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
 * ============================================================================
 * 【中文导读】list.c —— 内核链表（一切调度的数据结构基础，先读这个！）
 * ----------------------------------------------------------------------------
 * FreeRTOS 的就绪/延时/等待队列全是这文件里的双向环形链表（xList）。
 * 三个核心概念：链表根 List_t、节点 ListItem_t、节点“宿主”pvOwner
 * （调度器链表中通常是 TCB）。
 * vListInsert() 按 xItemValue 排序插入（O(n)）；vListInsertEnd() 和
 * uxListRemove() 为 O(1)。
 * 只有 5 个核心函数，约 200 行原始代码，是理解 tasks.c 的前置必修课。
 *
 * 【前置参考】Knowledge-Base/数据结构与算法/circular_singly_linked_list_with_a_sentinel_node.c
 * 是一份与本文件完全同构的教学实现：同为"哨兵 + 双向循环链表"（文件名虽写
 * singly，实际节点同时有 pxPrevious/pxNext，是双向的），建议对照阅读。
 * 下面各函数的注释都会反复与它对照：结构对照、四步接线的顺序陷阱、
 * 按值插入的两种写法、删除后是否清空指针的取舍。
 * ============================================================================
 */


#include <stdlib.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止内核 API 被“重命名”成
 * MPU 包装器版本。
 *
 * 机制（重命名真正发生在 include/mpu_wrappers.h:42-293）：
 *   portUSING_MPU_WRAPPERS == 1 时，FreeRTOS.h 会经 portable.h 连带包含
 *   mpu_wrappers.h，里面把 vTaskDelay、xTaskCreate 等一大批 API
 *   #define 成 MPU_xxx 版本——但仅当本宏未定义时才生效。
 *
 *   内核实现文件（tasks.c/queue.c/list.c...）在包含 FreeRTOS.h 前
 *   临时定义它 → 看到的是真实函数，内核内部互相调用不绕包装器；
 *   应用文件不定义 → 同样的头文件把 API 映射成 MPU_xxx，每次调用
 *   都先经包装器检查权限，再转调真身。
 *
 * 两个容易搞错的细节：
 *   1) 本文件包含的是 FreeRTOS.h 和 list.h（并没有包含 task.h），
 *      重命名来自 FreeRTOS.h → portable.h → mpu_wrappers.h 这条链；
 *   2) list API 本身不在重命名清单里，list.c 也不调用任何被重命名的
 *      API，所以此宏在本文件中并无实际生效点——它只是内核实现文件的
 *      统一防御性惯例（防止将来 include 其他内核头时踩坑）。
 *   权威依据：mpu_wrappers.h:39-40 的注释原文。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "list.h"

/*
 * 本文件在包含头文件前临时定义该宏，包含后立即取消定义，避免影响后续代码。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*
 * -----------------------------------------------------------
 * 公共链表 API（文档见 list.h）
 * ----------------------------------------------------------
 */

void vListInitialise( List_t * const pxList ) 
// List_t 见 list.h（教学版对应 List 结构体）。const 只修饰指针本身：
//   List_t * const pxList 是“常量指针”——pxList 在函数内不能再指向别处，
//   但 pxList->xxx 的内容可以改；
//   别与 const List_t * pxList（指向常量的指针：能改指向、不能改内容）混淆。
// 内核所有 API 的句柄参数统一用这种写法，防止实现代码不小心把句柄指到别处。
{
    traceENTER_vListInitialise( pxList ); // 进入函数的追踪钩子；默认空操作，不参与链表逻辑。

    /*
     * xListEnd 是链表末尾哨兵。初始化时让 pxIndex 指向它，
     * 并让 xListEnd 的 pxNext / pxPrevious 指向自身，形成空链表环。
     */
    pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );

    listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( &( pxList->xListEnd ) );

    /*
     * 链表末尾项的值取链表中的最大可能值，以确保它始终位于链表末尾。
     */
    pxList->xListEnd.xItemValue = portMAX_DELAY;

    /*
     * 链表末尾项的前后指针都指向自身，这样就能判断链表是否为空。
     */
    // 空表 = 哨兵自指（环长为 1，只有哨兵自己），与教学版 list_init() 的三行自指完全同构。
    // 之后插入第一个实际节点 A 时，A 会插在“哨兵与它自己”之间（紧跟哨兵），环长从 1 变 2。
    pxList->xListEnd.pxNext = ( ListItem_t * ) &( pxList->xListEnd );
    pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );

    /*
     * 编译期选择（不是运行时判断）：
     *   == 1 时 xListEnd 为精简版 MiniListItem_t，没有 pvOwner/pxContainer 字段，省 RAM；
     *   == 0 时为完整 ListItem_t，多了这两个字段，下面才需要补齐。
     * 教学版没有这个区分（哨兵直接用 Node）；FreeRTOS 为了省内存把哨兵瘦身，
     * 代价就是这里多出一段条件编译。
     */
    #if ( configUSE_MINI_LIST_ITEM == 0 )
    {
        pxList->xListEnd.pvOwner = NULL;
        pxList->xListEnd.pxContainer = NULL;
        listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( &( pxList->xListEnd ) );
    }
    #endif

    // 节点计数器清零（对照教学版 list_init() 的 l->count = 0）。
    // 顺带说明 0U 的 U 后缀：0 字面量默认是 int（有符号），与 UBaseType_t 这类
    // 无符号类型混用会触发告警，内核统一给无符号常量加 U（MISRA 风格）。
    pxList->uxNumberOfItems = ( UBaseType_t ) 0U;

    /*
     * 若 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 设为 1，
     * 则向链表结构体首尾写入已知校验值（金丝雀）。
     * 关闭检查时，下面两个宏展开为空，不产生任何代码。
     */
    listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList );
    listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList );

    traceRETURN_vListInitialise(); // 函数返回追踪钩子；默认空操作，不参与链表逻辑。
}
/* ----------------------------------------------------------- */

void vListInitialiseItem( ListItem_t * const pxItem )
// 把传入的链表项初始化成“未挂到任何链表上”的状态。
// 上面提到的“调试标记”指下面的完整性校验魔数：仅当
// configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES=1 时才真正写值，否则宏展开为空。
// 教学版没有对应函数——教学版的 Node 没有 pxContainer 字段，
// pxContainer 是 FreeRTOS 比教学版多出的“我现在挂在哪条链上”反向指针。
{
    traceENTER_vListInitialiseItem( pxItem ); // 进入函数的追踪钩子；默认空操作。

    /* 确保该链表项不被记录为已挂在某个链表上。 */
    pxItem->pxContainer = NULL;
    /* 注意：这是赋值，不是判断。
     * 表示该节点当前不属于任何链表。
     * 以后插入链表时，vListInsert / vListInsertEnd（或内联宏 listINSERT_END）
     * 会把 pxContainer 改成所属链表的地址；
     * 从链表删除时（uxListRemove），又会把它改回 NULL。
     * 所以 pxContainer 相当于“这个节点现在挂在哪个链表上”的标志。
     */

    /*
     * 若 configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES 设为 1，
     * 则向链表项首尾写入已知校验值。
     * 关闭检查时这两个宏展开为空，什么都不做。
     */
    listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
    listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );

    traceRETURN_vListInitialiseItem(); /* 函数返回追踪钩子；默认空操作。 */
}
/* ----------------------------------------------------------- */

void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem )
// 把新节点 pxNewListItem 插入链表 pxList 的“末尾”。
// 这里的“末尾”不是按 xItemValue 排序的末尾，而是插到 pxIndex 之前，
// 使轮转遍历时新节点最后被轮到。
{
    // 从链表 pxList 里取出它当前的 pxIndex 指针，保存到局部变量 pxIndex 中。
    ListItem_t * const pxIndex = pxList->pxIndex;

    // 进入函数后尽早调用的追踪钩子；默认空操作，不参与链表逻辑。
    traceENTER_vListInsertEnd( pxList, pxNewListItem );

    /*
     * 开启完整性检查时，下面两个宏展开为 configASSERT，分别检查
     * 链表结构体和节点首尾魔数是否被写坏；关闭检查时展开为空。
     */
    listTEST_LIST_INTEGRITY( pxList );
    listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );

    /*
     * 将新链表项插入 pxList，但不进行排序，而是让新项成为
     * listGET_OWNER_OF_NEXT_ENTRY() 轮转遍历时最后取到的项。
     */
    pxNewListItem->pxNext = pxIndex;
    pxNewListItem->pxPrevious = pxIndex->pxPrevious;

    /* 仅在判定覆盖（decision coverage）测试中使用。 */
    mtCOVERAGE_TEST_DELAY();

    // 后两步接线：让旧邻居反过来指向新节点，插入完成。
    // ★ 顺序关键：上一行里 pxIndex->pxPrevious 必须还是“旧前驱”，
    //   所以“改旧前驱的 pxNext”必须先于“改 pxIndex->pxPrevious”执行；
    //   顺序颠倒的话，上一行会变成 pxNewListItem->pxNext = pxNewListItem，链表自环。
    //   教学版 list_insert_before() 文件末尾的四步拆解大图逐步演示了这个陷阱，
    //   那里的 pos 对应本函数的 pxIndex，n 对应 pxNewListItem。
    pxIndex->pxPrevious->pxNext = pxNewListItem;
    pxIndex->pxPrevious = pxNewListItem;

    /* 记录该项当前所在的链表。 */
    pxNewListItem->pxContainer = pxList;

    // 节点计数 +1（教学版把 count++ 放在 list_insert_end 包装函数里；
    // FreeRTOS 没有包装层，直接内联在每个 API 的尾部）。
    ( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems + 1U );

    traceRETURN_vListInsertEnd();
}
/* ----------------------------------------------------------- */

void vListInsert( List_t * const pxList,ListItem_t * const pxNewListItem )
// 按 xItemValue 从小到大排序插入（对应教学版 list_insert_sorted()）。
// 差异在循环形状：教学版从第一个真实节点起、用 while、看“当前节点”的值；
// 这里从哨兵起、用 for、看“下一个节点”的值——起点的不同决定了终止条件
// 的写法不同（见循环处注释）。xValueOfInsertion == portMAX_DELAY 时必须
// 特殊处理，否则死循环；新节点排在已有相同值节点之后（两版都用 <=），保证公平。
{
    /* pxIterator 用来在链表里“走位”，最终停在“新节点应该插在它后面”的
     * 那个节点上。这里先不初始化，因为下面 if/else 两个分支都会给它赋值。 */
    ListItem_t * pxIterator;

    /* 先把要插入的排序值取出来存到局部变量。
     * const 表示函数内不再改它。
     * 后面 for 循环条件里反复用到这个值，用局部变量比每次写
     * pxNewListItem->xItemValue 更短，也避免重复解引用。 */
    const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    /* 函数入口追踪钩子；默认空操作，不参与链表逻辑。 */
    traceENTER_vListInsert( pxList, pxNewListItem );

    /*
     * 开启完整性检查时，下面两个宏展开为 configASSERT，分别检查
     * 链表结构体和节点首尾魔数是否被写坏；关闭检查时展开为空。
     */
    listTEST_LIST_INTEGRITY( pxList );
    listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );

    /*
     * 将新链表项按 xItemValue 顺序插入链表。
     *
     * 若链表中已存在相同 item 值的项，则新项应排在其后。
     * 这可确保存放在就绪链表中的 TCB（其 xItemValue 值均相同）能够公平分享 CPU。
     *
     * 但是，若 xValueOfInsertion 与尾部标记项 xListEnd 的值相同，
     * 下面的遍历循环将无法结束，因此需要先检查该值，必要时对算法稍作调整。
     */
    if( xValueOfInsertion == portMAX_DELAY )
    {
        /*
         * 特殊情况：要插入的值等于 portMAX_DELAY。
         *
         * xListEnd 的 xItemValue 在 vListInitialise() 里被设成了 portMAX_DELAY。
         * 如果走下面的 for 循环，条件
         * pxIterator->pxNext->xItemValue <= xValueOfInsertion
         * 在走到 xListEnd 时仍然成立（因为 xListEnd 的值也是 portMAX_DELAY），
         * 循环会永远绕下去，死循环。
         *
         * 所以直接让 pxIterator 指向 xListEnd 的前一个节点，
         * 也就是当前链表的最后一个实际节点。
         * 这样新节点会插到 xListEnd 之前，即链表末尾。
         *
         * 与教学版的取舍对照：教学版 list_insert_sorted() 的 while 条件是
         * “p != 哨兵 && 值不比 n 大”，靠那个身份判断兜住 n == 最大值的情形，
         * 所以它不需要特判；代价是每次迭代都多一次指针比较。
         * FreeRTOS 把“每次迭代一次身份比较”省掉了（哨兵的值本身就是终止器），
         * 付出的代价就是这里这一个 if 分支。内核热路径上前者更划算。
         */
        pxIterator = pxList->xListEnd.pxPrevious;
    }
    else
    {
        /*
         * *** 注意 ***********************************************************
         * 若应用程序在此处崩溃，常见原因如下所列。更多提示请参阅
         * https://www.freertos.org/Why-FreeRTOS/FAQs ，并确保定义了
         * configASSERT()！
         * https://www.FreeRTOS.org/a00110.html#configASSERT
         *
         *   1) 栈溢出
         *   2) 中断优先级设置错误（Cortex-M 上数值越大优先级越低）
         *   3) 在临界区内或调度器挂起时调用 API，或在中断中调用非 FromISR 版本
         *   4) 在队列/信号量未初始化、调度器未启动前就使用它们
         *   5) tick 中断优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY
         * ********************************************************************
         *
         * 这段注释是 FreeRTOS 官方给的“崩溃排查提示”。
         * 按值插入需要遍历链表，如果链表结构被破坏，就可能卡死在这里。
         */

        /*
         * for 循环：从哨兵 xListEnd 开始，向后找插入位置。
         *
         * 初始化：pxIterator = &( pxList->xListEnd );
         *   - xListEnd 是链表里的哨兵节点，不是实际数据节点；
         *   - 从它开始，pxIterator->pxNext 就是第一个实际节点；
         *   - 强制转换 ( ListItem_t * ) 是因为 xListEnd 可能是
         *     MiniListItem_t，取地址后转成 ListItem_t* 才能赋给 pxIterator。
         *
         * 条件：pxIterator->pxNext->xItemValue <= xValueOfInsertion
         *   - 注意看的是“下一个节点”的值，不是当前节点的值；
         *   - 只要下一个节点的值 <= 要插入的值，就继续往后走；
         *   - 这样循环结束时，pxIterator 停在“最后一个值 <= 插入值的节点”上，
         *     新节点就插在它后面；
         *   - 用 <= 而不是 <，保证相同值的新节点排在已有相同值节点之后，
         *     实现“先来先服务”的公平性。
         *
         * 迭代：pxIterator = pxIterator->pxNext;
         *   - 继续向后走。
         *
         * 循环体为空：这里不需要做任何事，只是“走到目标位置”。
         * 若代码卡在这里，请参阅上面的注意事项。
         *
         * 两种循环形状的等价性（对照教学版 list_insert_sorted）：
         *   教学版：p 从第一个真实节点出发，条件看 p 自己的值，
         *           “停在第一个值比 n 大的节点上，插它前面”；
         *           由于 p 可能一路走到哨兵，必须有 p != 哨兵 这个身份判断。
         *   本版本：pxIterator 从哨兵出发，条件看“下一格”的值，
         *           “停在下一格比 n 大的节点上，插它后面”；
         *           哨兵的值恒为 portMAX_DELAY（大于一切合法插入值），
         *           所以循环走到“下一格是哨兵”时必然不满足条件而停下，
         *           身份判断被值判断顺带取代——这正是哨兵值设为最大的回报。
         */
        for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd );
             pxIterator->pxNext->xItemValue <= xValueOfInsertion;
             pxIterator = pxIterator->pxNext )
        {
            /* 空循环体：只负责遍历到目标插入位置。 */
        }
    }

    /*
     * 四行“接线”：把新节点 pxNewListItem 插到 pxIterator 之后。
     *
     * 插入前：
     *     pxIterator <-> pxIterator->pxNext
     *
     * 插入后：
     *     pxIterator <-> pxNewListItem <-> pxIterator->pxNext
     *
     * 与 vListInsertEnd() 的对照（两者恰好是教学版一对镜像函数）：
     *   vListInsertEnd() = list_insert_before( pxIndex, n )     —— 插在 pxIndex 前面
     *   vListInsert()    = list_insert_after( pxIterator, n )   —— 插在 pxIterator 后面
     *
     * 教学版 list_insert_after() 的四行顺序是“先接好 n 自己的两根指针，
     * 再让邻居反指”；本函数四行是交错顺序（n 的 next → 新后继的 prev →
     * n 的 prev → pxIterator 的 next）。两种顺序都对，共同的安全前提是：
     * 后面还要用的旧值在被改写之前已经读掉——本函数里第 1 行读了
     * pxIterator->pxNext（旧后继），第 4 行才改写它，所以安全；若把第 4 行
     * 提前，第 1、2 行就会拿到新节点自己，形成自环（教学版文件末尾的
     * 四步拆解图用 insert_before 演示了同一个陷阱）。
     */

    /* 新节点的 next 指向 pxIterator 原来的后继。 */
    pxNewListItem->pxNext = pxIterator->pxNext;

    /* 原来后继的 previous 改为指向新节点。 */
    pxNewListItem->pxNext->pxPrevious = pxNewListItem;

    /* 新节点的 previous 指向 pxIterator。 */
    pxNewListItem->pxPrevious = pxIterator;

    /* pxIterator 的 next 改为指向新节点。 */
    pxIterator->pxNext = pxNewListItem;

    /*
     * 记录该项当前所在的链表，以便之后快速移除该项。
     * 删除时可以通过 pxContainer 找到所属链表，把节点数减一。
     */
    pxNewListItem->pxContainer = pxList;

    /*
     * 节点计数器加一。
     * 等价于：pxList->uxNumberOfItems++；
     * 显式 ( UBaseType_t ) 转换和 1U 后缀是为了类型匹配、避免警告，
     * 符合 FreeRTOS 编码风格。
     */
    ( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems + 1U );

    /* 函数返回追踪钩子；默认空操作。 */
    traceRETURN_vListInsert();
}
/* ----------------------------------------------------------- */


UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
// 从链表摘除一个节点（对应教学版 list_unlink()），返回剩余节点数。
// 注意参数只有“节点”没有“链表”——靠 pxContainer 反查，教学版做不到这点
// （它的 unlink 必须传 List *l，因为 Node 里没有反向指针）。
{
    /*
     * 链表项自身知道它属于哪个链表，因此可从链表项反查所属链表。
     *
     * pxContainer 是 vListInitialiseItem() 里被清成 NULL、
     * 在 listINSERT_END / vListInsert 里被设成所属链表地址的字段。
     * 删除时不需要传入链表指针，只传节点，就能通过它找到链表。
     *
     * const 表示 pxList 这个局部指针本身不再改指向，
     * 但它指向的 List_t 内容可改。
     */
    List_t * const pxList = pxItemToRemove->pxContainer;

    /* 函数入口追踪钩子；默认空操作，不参与链表逻辑。 */
    traceENTER_uxListRemove( pxItemToRemove );

    /*
     * 双向链表删除的核心“接线”：把被删节点从环里摘掉。
     *
     * 删除前：
     *     ... <-> P <-> X <-> N <-> ...
     *              ^
     *              pxItemToRemove 指向 X
     *
     * 删除后：
     *     ... <-> P <-> N <-> ...
     *
     * 第一行：让 X 的后继 N 的 previous 指向 X 的前驱 P。
     * 第二行：让 X 的前驱 P 的 next 指向 X 的后继 N。
     *
     * 两行执行完，P 和 N 直接互连，X 就被从环里摘出来了。
     * 注意：这里没有改 X 自己的 pxNext / pxPrevious，
     * 它们仍然指着 P 和 N，但已经没人指向 X，X 不在环里了。
     *
     * 与教学版 list_unlink() 的两处顺序差异（混搭两版会引入 bug）：
     *   教学版：① 先修游标 → ② 邻居互指摘除 → ③ 把 X 的前后指针置 NULL。
     *   本版本：① 邻居互指摘除 → ② 后面才用 pxItemToRemove->pxPrevious 修游标
     *          → ③ 只清 pxContainer，不清前后指针。
     *   本版本“摘除后再修游标”之所以成立，正因为不清前后指针——
     *   pxItemToRemove->pxPrevious 在摘除后依然有效；若配上教学版的
     *   “置空指针”，这里就会解引用 NULL。顺序和清不清空是绑定的设计决定。
     */

    /* 后继的 previous 指向被删节点的前驱：N->pxPrevious = P。 */
    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;

    /* 前驱的 next 指向被删节点的后继：P->pxNext = N。 */
    pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

    /* 仅在判定覆盖（decision coverage）测试中使用，正常编译时为空。 */
    mtCOVERAGE_TEST_DELAY();

    /*
     * 确保索引始终指向一个有效项。
     *
     * pxIndex 是链表遍历用的“当前索引位置”，
     * listGET_OWNER_OF_NEXT_ENTRY() 靠它轮转遍历。
     *
     * 如果 pxIndex 正好指向被删节点 X，删除后 X 已经不在环里，
     * pxIndex 就悬空了，必须调整。
     *
     * 调整方式：让 pxIndex 指向 X 的前驱 P（pxItemToRemove->pxPrevious）。
     *
     * 为什么指向前驱 P，而不是后继 N？
     *   listGET_OWNER_OF_NEXT_ENTRY() 的逻辑是：
     *       先 pxIndex = pxIndex->pxNext，再取 pxIndex 的 owner。
     *   如果 pxIndex 指向 P，下一次遍历会先走到 N，再取 N 的 owner，
     *   N 不会被跳过；
     *   如果 pxIndex 指向 N，下一次遍历会先走到 N 的后继，
     *   N 就被跳过了。
     *   所以指向前驱 P 才能保证下一次遍历从 N 开始，不漏节点。
     */
    if( pxList->pxIndex == pxItemToRemove )
    {
        pxList->pxIndex = pxItemToRemove->pxPrevious;
    }
    else
    {
        /* pxIndex 不指向被删节点，无需调整。
         * mtCOVERAGE_TEST_MARKER() 用于覆盖率测试，正常编译时为空。 */
        mtCOVERAGE_TEST_MARKER();
    }

    /*
     * 清空“所属链表”记录，表示该节点当前不属于任何链表。
     *
     * 注意：这里只清 pxContainer，不清 pxNext / pxPrevious。
     * 教学版把“置空前后指针”列为可选项（便于暴露悬空指针的误用）；
     * FreeRTOS 选择不清，理由有三：
     *   1) 节点常被马上重新插入（如任务离开就绪表又回来），
     *      插入函数反正会重设这两个指针，清了是白做两次内存写；
     *   2) pxContainer == NULL 本身就是“不在链表上”的权威标志，
     *      没有任何代码会在游离期间去读它的前后指针；
     *   3) 上面修游标那步刚用过 pxItemToRemove->pxPrevious，不能清。
     * 而 pxContainer 必须清：否则 listIS_CONTAINED_WITHIN() 等查询
     * 会误以为它还挂在链表上。
     */
    pxItemToRemove->pxContainer = NULL;

    /*
     * 节点计数器减一。
     * 等价于：pxList->uxNumberOfItems--；
     * 显式 ( UBaseType_t ) 转换和 1U 后缀是为了类型匹配、避免警告，
     * 符合 FreeRTOS 编码风格。
     */
    ( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems - 1U );

    /*
     * 函数返回追踪钩子，带参数：剩余节点数。
     * 用户可自定义，默认空操作。
     */
    traceRETURN_uxListRemove( pxList->uxNumberOfItems );

    /*
     * 返回删除后链表中剩余的节点数。
     * 调用者可以用它判断链表是否已空，例如：
     *     if( uxListRemove( pxItem ) == 0 ) { ...链表已空... }
     */
    return pxList->uxNumberOfItems;
}
/* ----------------------------------------------------------- */

