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

/* 安全上下文包含文件。 */
#include "secure_context.h"

/* 安全移植层宏。 */
#include "secure_port_macros.h"

void SecureContext_LoadContextAsm( SecureContext_t * pxSecureContext ) __attribute__( ( naked ) );
void SecureContext_SaveContextAsm( SecureContext_t * pxSecureContext ) __attribute__( ( naked ) );

void SecureContext_LoadContextAsm( SecureContext_t * pxSecureContext )
{
    /* pxSecureContext 的值位于 r0 中。 */
    __asm volatile
    (
        " .syntax unified                   \n"
        "                                   \n"
        " mrs r1, ipsr                      \n" /* r1 = IPSR（读取 IPSR 值）。 */
        " cbz r1, load_ctx_therad_mode      \n" /* 若处理器运行在线程模式（Thread Mode），则不执行任何操作。 */
        " ldmia r0!, {r1, r2}               \n" /* r1 = pxSecureContext->pucCurrentStackPointer，r2 = pxSecureContext->pucStackLimit（读取上下文栈指针与栈限）。 */
        "                                   \n"
        #if ( configENABLE_MPU == 1 )
            " ldmia r1!, {r3}               \n" /* 从任务栈中读取 CONTROL 寄存器的值。r3 = CONTROL。 */
            " msr control, r3               \n" /* CONTROL = r3（写 CONTROL）。 */
        #endif /* configENABLE_MPU（使能 MPU） */
        "                                   \n"
        " msr psplim, r2                    \n" /* PSPLIM = r2（写 PSPLIM）。 */
        " msr psp, r1                       \n" /* PSP = r1（写 PSP）。 */
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
        " mrs r1, ipsr                      \n" /* r1 = IPSR（读取 IPSR 值）。 */
        " cbz r1, save_ctx_therad_mode      \n" /* 若处理器运行在线程模式（Thread Mode），则不执行任何操作。 */
        " mrs r1, psp                       \n" /* r1 = PSP（读取 PSP 值）。 */
        "                                   \n"
        #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            " vstmdb r1!, {s0}              \n" /* 触发 FPU 寄存器的延迟压栈。 */
            " vldmia r1!, {s0}              \n" /* 抵消上一条语句的效果。 */
        #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */
        "                                   \n"
        #if ( configENABLE_MPU == 1 )
            " mrs r2, control               \n" /* r2 = CONTROL（读取 CONTROL）。 */
            " stmdb r1!, {r2}               \n" /* 将 CONTROL 值保存到栈上。 */
        #endif /* configENABLE_MPU（使能 MPU） */
        "                                   \n"
        " str r1, [r0]                      \n" /* 将栈顶保存到上下文中。pxSecureContext->pucCurrentStackPointer = r1。 */
        " movs r1, %0                       \n" /* r1 = securecontextNO_STACK（置为无栈标记）。 */
        " msr psplim, r1                    \n" /* PSPLIM = securecontextNO_STACK（写入无栈标记）。 */
        " msr psp, r1                       \n" /* PSP = securecontextNO_STACK，即在下一个任务的上下文加载之前，线程模式没有栈。 */
        "                                   \n"
        " save_ctx_therad_mode:             \n"
        "    bx lr                          \n"
        "                                   \n"
        ::"i" ( securecontextNO_STACK ) : "r1", "memory"
    );
}
/*-----------------------------------------------------------*/
