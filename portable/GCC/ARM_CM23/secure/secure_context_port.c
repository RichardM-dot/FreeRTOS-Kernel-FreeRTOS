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

/* 安全上下文头文件包含。 */
#include "secure_context.h"

/* 安全移植层宏。 */
#include "secure_port_macros.h"

#if ( configENABLE_FPU == 1 )
    #error Cortex-M23 does not have a Floating Point Unit (FPU) and therefore configENABLE_FPU must be set to 0.
#endif

void SecureContext_LoadContextAsm( SecureContext_t * pxSecureContext ) __attribute__( ( naked ) );
void SecureContext_SaveContextAsm( SecureContext_t * pxSecureContext ) __attribute__( ( naked ) );

void SecureContext_LoadContextAsm( SecureContext_t * pxSecureContext )
{
    /* pxSecureContext 的值位于 r0 中。 */
    __asm volatile
    (
        " .syntax unified                   \n"
        "                                   \n"
        " mrs r1, ipsr                      \n" /* r1 = IPSR（读取中断程序状态寄存器）。 */
        " cbz r1, load_ctx_therad_mode      \n" /* 若处理器运行在线程模式则不做任何操作。 */
        " ldmia r0!, {r1, r2}               \n" /* r1 = pxSecureContext->pucCurrentStackPointer，r2 = pxSecureContext->pucStackLimit（读取上下文栈指针与栈限制）。 */
        "                                   \n"
        #if ( configENABLE_MPU == 1 )
            " ldmia r1!, {r3}               \n" /* 从任务的栈中读取 CONTROL 寄存器值。r3 = CONTROL。 */
            " msr control, r3               \n" /* CONTROL = r3（恢复 CONTROL 寄存器）。 */
        #endif /* configENABLE_MPU 条件编译分支 */
        "                                   \n"
        " msr psplim, r2                    \n" /* PSPLIM = r2（设置进程栈限制）。 */
        " msr psp, r1                       \n" /* PSP = r1（设置进程栈指针）。 */
        "                                   \n"
        " load_ctx_therad_mode:             \n"
        "    bx lr                          \n"
        "                                   \n"
        ::: "r0", "r1", "r2"
    );
}
/*-----------------------------------------------------------*/

void SecureContext_SaveContextAsm( SecureContext_t * pxSecureContext )
{
    /* pxSecureContext 的值位于 r0 中。 */
    __asm volatile
    (
        " .syntax unified                   \n"
        "                                   \n"
        " mrs r1, ipsr                      \n" /* r1 = IPSR（读取中断程序状态寄存器）。 */
        " cbz r1, save_ctx_therad_mode      \n" /* 若处理器运行在线程模式则不做任何操作。 */
        " mrs r1, psp                       \n" /* r1 = PSP（读取进程栈指针）。 */
        "                                   \n"
        #if ( configENABLE_MPU == 1 )
            " mrs r2, control               \n" /* r2 = CONTROL（读取 CONTROL 寄存器）。 */
            " subs r1, r1, #4               \n" /* 在栈上为 CONTROL 值腾出空间。 */
            " str r1, [r0]                  \n" /* 将栈顶保存到上下文中。pxSecureContext->pucCurrentStackPointer = r1。 */
            " stmia r1!, {r2}               \n" /* 将 CONTROL 值存储到栈上。 */
        #else /* configENABLE_MPU 条件编译分支 */
            " str r1, [r0]                  \n" /* 将栈顶保存到上下文中。pxSecureContext->pucCurrentStackPointer = r1。 */
        #endif /* configENABLE_MPU 条件编译结束 */
        "                                   \n"
        " movs r1, %0                       \n" /* r1 = securecontextNO_STACK（无栈标记）。 */
        " msr psplim, r1                    \n" /* PSPLIM = securecontextNO_STACK（清除进程栈限制）。 */
        " msr psp, r1                       \n" /* PSP = securecontextNO_STACK，即在下一个任务上下文加载之前，线程模式没有栈。 */
        "                                   \n"
        " save_ctx_therad_mode:             \n"
        "   bx lr                           \n"
        "                                   \n"
        ::"i" ( securecontextNO_STACK ) : "r1", "memory"
    );
}
/*-----------------------------------------------------------*/
