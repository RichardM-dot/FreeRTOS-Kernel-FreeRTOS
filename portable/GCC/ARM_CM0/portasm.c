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

/* 标准头文件。 */
#include <stdint.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可确保 PRIVILEGED_FUNCTION
 * 被正确定义，并将特权函数放入正确的段中。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* 移植层汇编相关头文件。 */
#include "portasm.h"

/* 系统调用编号相关头文件。 */
#include "mpu_syscall_numbers.h"

/*
 * MPU_WRAPPERS_INCLUDED_FROM_API_FILE 只需在为头文件
 * 时才定义。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configENABLE_MPU == 1 )

    void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .extern pxCurrentTCB                            \n"
            " .syntax unified                                 \n"
            "                                                 \n"
            " program_mpu_first_task:                         \n"
            "                                                 \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* r3 置为 &pxCurrentTCB。 */
            "    ldr r0, [r3]                                 \n" /* r0 置为 pxCurrentTCB。 */
            "    adds r0, #4                                  \n" /* r0 = TCB 中的第二项，即 xMPUSettings。 */
            "                                                 \n"
            "    dmb                                          \n" /* 在禁用 MPU 前完成未完成的传输。 */
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 置为 1。 */
            "    bics r2, r3                                  \n" /* r2 = r2 & ~r3，即清除 r2 中的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 禁用 MPU。 */
            "                                                 \n"
            "    ldr r1, =0xe000ed9c                          \n" /* r1 = 0xe000ed9c [RBAR 的地址]。 */
            "    ldr r2, =0xe000eda0                          \n" /* r2 = 0xe000eda0 [RASR 的地址]。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第一组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第二组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第三组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第四组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第五组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 置为 1。 */
            "    orrs r2, r3                                  \n" /* r2 = r2 | r3，即设置 r2 中的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 使能 MPU。 */
            "    dsb                                          \n" /* 继续执行前强制完成内存写入。 */
            "                                                 \n"
            " restore_context_first_task:                     \n"
            "    ldr r2, =pxCurrentTCB                        \n" /* r2 置为 &pxCurrentTCB。 */
            "    ldr r0, [r2]                                 \n" /* r0 置为 pxCurrentTCB。 */
            "    ldr r1, [r0]                                 \n" /* r1 = TCB 中保存上下文的位置。 */
            "                                                 \n"
            " restore_special_regs_first_task:                \n"
            "    subs r1, #12                                 \n"
            "    ldmia r1!, {r2-r4}                           \n" /* r2 = 原始 PSP，r3 = CONTROL，r4 = LR。 */
            "    subs r1, #12                                 \n"
            "    msr psp, r2                                  \n"
            "    msr control, r3                              \n"
            "    mov lr, r4                                   \n"
            "                                                 \n"
            " restore_general_regs_first_task:                \n"
            "    subs r1, #32                                 \n"
            "    ldmia r1!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的一半。 */
            "    stmia r2!, {r4-r7}                           \n" /* 将一半硬件保存的上下文复制到任务栈。 */
            "    ldmia r1!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的另一半。 */
            "    stmia r2!, {r4-r7}                           \n" /* 将另一半硬件保存的上下文复制到任务栈。 */
            "    subs r1, #48                                 \n"
            "    ldmia r1!, {r4-r7}                           \n" /* 恢复 r8-r11。 */
            "    mov r8, r4                                   \n" /* r8 置为 r4。 */
            "    mov r9, r5                                   \n" /* r9 置为 r5。 */
            "    mov r10, r6                                  \n" /* r10 置为 r6。 */
            "    mov r11, r7                                  \n" /* r11 置为 r7。 */
            "    subs r1, #32                                 \n"
            "    ldmia r1!, {r4-r7}                           \n" /* 恢复 r4-r7。 */
            "    subs r1, #16                                 \n"
            "                                                 \n"
            " restore_context_done_first_task:                \n"
            "    str r1, [r0]                                 \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
            "    bx lr                                        \n"
            "                                                 \n"
            " .align 4                                        \n"
            ::"i" ( portSVC_START_SCHEDULER ) : "memory"
        );
    }

#else /* configENABLE_MPU（使能 MPU） */

    void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .extern pxCurrentTCB                            \n"
            " .syntax unified                                 \n"
            "                                                 \n"
            " ldr  r2, =pxCurrentTCB                          \n" /* r2 置为 &pxCurrentTCB。 */
            " ldr  r1, [r2]                                   \n" /* r1 置为 pxCurrentTCB。 */
            " ldr  r0, [r1]                                   \n" /* 从 TCB 读取任务栈顶——pxCurrentTCB 中的第一项就是任务栈顶。 */
            "                                                 \n"
            " ldm  r0!, {r2}                                  \n" /* 从栈中读取——r2 = EXC_RETURN。 */
            " movs r1, #2                                     \n" /* r1 置为 2。 */
            " msr  CONTROL, r1                                \n" /* 在线程模式下切换为使用 PSP。 */
            " adds r0, #32                                    \n" /* 丢弃 r0 之前的所有内容。 */
            " msr  psp, r0                                    \n" /* 这现在是任务要使用的新栈顶。 */
            " isb                                             \n"
            " bx   r2                                         \n" /* 最后，跳转到 EXC_RETURN。 */
            "                                                 \n"
            " .align 4                                        \n"
        );
    }

#endif /* configENABLE_MPU（使能 MPU） */

/*-----------------------------------------------------------*/

BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        " .syntax unified                                 \n"
        "                                                 \n"
        " mrs r0, control                                 \n" /* r0 置为 CONTROL。 */
        " movs r1, #1                                     \n" /* r1 置为 1。 */
        " tst r0, r1                                      \n" /* 执行 r0 & r1（按位与）并更新条件标志。 */
        " beq running_privileged                          \n" /* 若前一步 AND 操作的结果为 0，则跳转。 */
        " movs r0, #0                                     \n" /* CONTROL[0]!=0。返回 false 表示处理器当前不处于特权状态。 */
        " bx lr                                           \n" /* 返回。 */
        " running_privileged:                             \n"
        "    movs r0, #1                                  \n" /* CONTROL[0]==0。返回 true 表示处理器当前处于特权状态。 */
        "    bx lr                                        \n" /* 返回。 */
        "                                                 \n"
        " .align 4                                        \n"
        ::: "r0", "r1", "memory"
    );
}

/*-----------------------------------------------------------*/

void vRaisePrivilege( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        " .syntax unified                                 \n"
        "                                                 \n"
        " mrs  r0, control                                \n" /* 读取 CONTROL 寄存器。 */
        " movs r1, #1                                     \n" /* r1 置为 1。 */
        " bics r0, r1                                     \n" /* 清除 bit 0。 */
        " msr  control, r0                                \n" /* 写回新的 CONTROL 值。 */
        " bx lr                                           \n" /* 返回调用者。 */
        ::: "r0", "r1", "memory"
    );
}

/*-----------------------------------------------------------*/

void vResetPrivilege( void ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        " .syntax unified                                 \n"
        "                                                 \n"
        " mrs r0, control                                 \n" /* r0 置为 CONTROL。 */
        " movs r1, #1                                     \n" /* r1 置为 1。 */
        " orrs r0, r1                                     \n" /* r0 置为 r0 | r1。 */
        " msr control, r0                                 \n" /* CONTROL 置为 r0。 */
        " bx lr                                           \n" /* 返回调用者。 */
        ::: "r0", "r1", "memory"
    );
}

/*-----------------------------------------------------------*/

void vStartFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    /*
     * 不要像 CM3/4 器件那样复位 MSP 栈。原因是向量表偏移寄存器
     * （VTOR）在 CM0+ 架构中是可选的，因此并非所有器件都提供该寄存器。
     */
    __asm volatile
    (
        " .syntax unified                                 \n"
        " cpsie i                                         \n" /* 全局使能中断。 */
        " dsb                                             \n"
        " isb                                             \n"
        " svc %0                                          \n" /* 通过系统调用启动第一个任务。 */
        " nop                                             \n"
        "                                                 \n"
        " .align 4                                        \n"
        ::"i" ( portSVC_START_SCHEDULER ) : "memory"
    );
}

/*-----------------------------------------------------------*/

uint32_t ulSetInterruptMask( void ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        " .syntax unified                                 \n"
        "                                                 \n"
        " mrs r0, PRIMASK                                 \n"
        " cpsid i                                         \n"
        " bx lr                                           \n"
        ::: "memory"
    );
}

/*-----------------------------------------------------------*/

void vClearInterruptMask( __attribute__( ( unused ) ) uint32_t ulMask ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        " .syntax unified                                 \n"
        "                                                 \n"
        " msr PRIMASK, r0                                 \n"
        " bx lr                                           \n"
        ::: "memory"
    );
}

/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .extern pxCurrentTCB                            \n"
            " .syntax unified                                 \n"
            "                                                 \n"
            " ldr r2, =pxCurrentTCB                           \n" /* r2 置为 &( pxCurrentTCB )。 */
            " ldr r0, [r2]                                    \n" /* r0 置为 pxCurrentTCB。 */
            " ldr r1, [r0]                                    \n" /* r1 = TCB 中应保存上下文的位置。 */
            " mrs r2, psp                                     \n" /* r2 置为 PSP。 */
            "                                                 \n"
            " save_general_regs:                              \n"
            "    stmia r1!, {r4-r7}                           \n" /* 保存 r4-r7。 */
            "    mov r4, r8                                   \n" /* r4 置为 r8。 */
            "    mov r5, r9                                   \n" /* r5 置为 r9。 */
            "    mov r6, r10                                  \n" /* r6 置为 r10。 */
            "    mov r7, r11                                  \n" /* r7 置为 r11。 */
            "    stmia r1!, {r4-r7}                           \n" /* 保存 r8-r11。 */
            "    ldmia r2!, {r4-r7}                           \n" /* 将一半硬件保存的上下文复制到 r4-r7。 */
            "    stmia r1!, {r4-r7}                           \n" /* 保存硬件保存的上下文。 */
            "    ldmia r2!, {r4-r7}                           \n" /* 将另一半硬件保存的上下文复制到 r4-r7。 */
            "    stmia r1!, {r4-r7}                           \n" /* 保存硬件保存的上下文。 */
            "                                                 \n"
            " save_special_regs:                              \n"
            "    mrs r2, psp                                  \n" /* r2 置为 PSP。 */
            "    mrs r3, control                              \n" /* r3 置为 CONTROL。 */
            "    mov r4, lr                                   \n" /* r4 置为 LR。 */
            "    stmia r1!, {r2-r4}                           \n" /* 保存原始 PSP（硬件已保存上下文之后）、CONTROL 和 LR。 */
            "    str r1, [r0]                                 \n" /* 将恢复上下文的位置作为 TCB 的第一个成员保存。 */
            "                                                 \n"
            " select_next_task:                               \n"
            "    cpsid i                                      \n"
            "    bl vTaskSwitchContext                        \n"
            "    cpsie i                                      \n"
            "                                                 \n"
            " program_mpu:                                    \n"
            "                                                 \n"
            "    ldr r2, =pxCurrentTCB                        \n" /* r2 置为 &( pxCurrentTCB )。 */
            "    ldr r0, [r2]                                 \n" /* r0 置为 pxCurrentTCB。 */
            "    adds r0, #4                                  \n" /* r0 = TCB 中的第二项，即 xMPUSettings。 */
            "                                                 \n"
            "    dmb                                          \n" /* 在禁用 MPU 前完成未完成的传输。 */
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 置为 1。 */
            "    bics r2, r3                                  \n" /* r2 = r2 & ~r3，即清除 r2 中的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 禁用 MPU。 */
            "                                                 \n"
            "    ldr r1, =0xe000ed9c                          \n" /* r1 = 0xe000ed9c [RBAR 的地址]。 */
            "    ldr r2, =0xe000eda0                          \n" /* r2 = 0xe000eda0 [RASR 的地址]。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第一组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第二组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第三组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第四组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldmia r0!, {r3-r4}                           \n" /* 从 TCB 读取第五组 RBAR/RASR 寄存器。 */
            "    str r3, [r1]                                 \n" /* 写入 RBAR。 */
            "    str r4, [r2]                                 \n" /* 写入 RASR。 */
            "                                                 \n"
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 置为 1。 */
            "    orrs r2, r3                                  \n" /* r2 = r2 | r3，即设置 r2 中的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 使能 MPU。 */
            "    dsb                                          \n" /* 继续执行前强制完成内存写入。 */
            "                                                 \n"
            " restore_context:                                \n"
            "    ldr r2, =pxCurrentTCB                        \n" /* r2 置为 &pxCurrentTCB。 */
            "    ldr r0, [r2]                                 \n" /* r0 置为 pxCurrentTCB。 */
            "    ldr r1, [r0]                                 \n" /* r1 = TCB 中保存上下文的位置。 */
            "                                                 \n"
            " restore_special_regs:                           \n"
            "    subs r1, #12                                 \n"
            "    ldmia r1!, {r2-r4}                           \n" /* r2 = 原始 PSP，r3 = CONTROL，r4 = LR。 */
            "    subs r1, #12                                 \n"
            "    msr psp, r2                                  \n"
            "    msr control, r3                              \n"
            "    mov lr, r4                                   \n"
            "                                                 \n"
            " restore_general_regs:                           \n"
            "    subs r1, #32                                 \n"
            "    ldmia r1!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的一半。 */
            "    stmia r2!, {r4-r7}                           \n" /* 将一半硬件保存的上下文复制到任务栈。 */
            "    ldmia r1!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的另一半。 */
            "    stmia r2!, {r4-r7}                           \n" /* 将另一半硬件保存的上下文复制到任务栈。 */
            "    subs r1, #48                                 \n"
            "    ldmia r1!, {r4-r7}                           \n" /* 恢复 r8-r11。 */
            "    mov r8, r4                                   \n" /* r8 置为 r4。 */
            "    mov r9, r5                                   \n" /* r9 置为 r5。 */
            "    mov r10, r6                                  \n" /* r10 置为 r6。 */
            "    mov r11, r7                                  \n" /* r11 置为 r7。 */
            "    subs r1, #32                                 \n"
            "    ldmia r1!, {r4-r7}                           \n" /* 恢复 r4-r7。 */
            "    subs r1, #16                                 \n"
            "                                                 \n"
            " restore_context_done:                           \n"
            "    str r1, [r0]                                 \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
            "    bx lr                                        \n"
            "                                                 \n"
            " .align 4                                        \n"
        );
    }

#else /* configENABLE_MPU（使能 MPU） */

    void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .extern pxCurrentTCB                            \n"
            " .syntax unified                                 \n"
            "                                                 \n"
            " mrs r0, psp                                     \n" /* 读取 PSP 到 r0。 */
            " ldr r2, =pxCurrentTCB                           \n" /* r2 置为 &( pxCurrentTCB )。 */
            " ldr r1, [r2]                                    \n" /* r1 置为 pxCurrentTCB。 */
            " subs r0, r0, #36                                \n" /* 在栈上为 LR 和其余寄存器分配空间。 */
            " str r0, [r1]                                    \n" /* 将新的栈顶保存到 TCB。 */
            "                                                 \n"
            " mov r3, lr                                      \n" /* r3 置为 LR/EXC_RETURN。 */
            " stmia r0!, {r3-r7}                              \n" /* 保存到栈上——LR 以及不会自动保存的低寄存器。 */
            " mov r4, r8                                      \n" /* r4 置为 r8。 */
            " mov r5, r9                                      \n" /* r5 置为 r9。 */
            " mov r6, r10                                     \n" /* r6 置为 r10。 */
            " mov r7, r11                                     \n" /* r7 置为 r11。 */
            " stmia r0!, {r4-r7}                              \n" /* 保存不会自动保存的高寄存器。 */
            "                                                 \n"
            " cpsid i                                         \n"
            " bl vTaskSwitchContext                           \n"
            " cpsie i                                         \n"
            "                                                 \n"
            " ldr r2, =pxCurrentTCB                           \n" /* r2 置为 &( pxCurrentTCB )。 */
            " ldr r1, [r2]                                    \n" /* r1 置为 pxCurrentTCB。 */
            " ldr r0, [r1]                                    \n" /* pxCurrentTCB 中的第一项是任务栈顶。r0 现在指向栈顶。 */
            "                                                 \n"
            " adds r0, r0, #20                                \n" /* 切换到高寄存器。 */
            " ldmia r0!, {r4-r7}                              \n" /* 恢复不会自动恢复的高寄存器。 */
            " mov r8, r4                                      \n" /* r8 置为 r4。 */
            " mov r9, r5                                      \n" /* r9 置为 r5。 */
            " mov r10, r6                                     \n" /* r10 置为 r6。 */
            " mov r11, r7                                     \n" /* r11 置为 r7。 */
            " msr psp, r0                                     \n" /* 记住任务的新栈顶。 */
            " subs r0, r0, #36                                \n" /* 移动到已保存上下文的起始位置。 */
            " ldmia r0!, {r3-r7}                              \n" /* 从栈中读取——r3 = LR，并恢复 r4-r7。 */
            " bx r3                                           \n"
            "                                                 \n"
            " .align 4                                        \n"
        );
    }

#endif /* configENABLE_MPU（使能 MPU） */

/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .syntax unified                \n"
            " .extern vPortSVCHandler_C      \n"
            " .extern vSystemCallEnter       \n"
            " .extern vSystemCallExit        \n"
            " .extern pxCurrentTCB           \n"
            "                                \n"
            " movs r0, #4                    \n"
            " mov r1, lr                     \n"
            " tst r0, r1                     \n"
            " beq stack_on_msp               \n"
            "                                \n"
            " stack_on_psp:                  \n"
            "     mrs r0, psp                \n"
            "     b route_svc                \n"
            "                                \n"
            " stack_on_msp:                  \n"
            "     mrs r0, msp                \n"
            "     b route_svc                \n"
            "                                \n"
            " route_svc:                     \n"
            "     ldr r3, [r0, #24]          \n"
            "     subs r3, #2                \n"
            "     ldrb r2, [r3, #0]          \n"
            "     ldr r3, =%0                \n"
            "     cmp r2, r3                 \n"
            "     blt system_call_enter      \n"
            "     ldr r3, =%1                \n"
            "     cmp r2, r3                 \n"
            "     beq system_call_exit       \n"
            "     ldr r3, =vPortSVCHandler_C \n"
            "     bx r3                      \n"
            "                                \n"
            " system_call_enter:             \n"
            "    push {lr}                   \n"
            "    bl vSystemCallEnter         \n"
            "    pop {pc}                    \n"
            "                                \n"
            " system_call_exit:              \n"
            "    push {lr}                   \n"
            "    bl vSystemCallExit          \n"
            "    pop {pc}                    \n"
            "                                \n"
            " .align 4                       \n"
            "                                \n"
            : /* 无输出。 */
            : "i" ( NUM_SYSTEM_CALLS ), "i" ( portSVC_SYSTEM_CALL_EXIT )
            : "r0", "r1", "r2", "r3", "memory"
        );
    }

#else /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

    void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .syntax unified                \n"
            " .extern vPortSVCHandler_C      \n"
            "                                \n"
            " movs r0, #4                    \n"
            " mov r1, lr                     \n"
            " tst r0, r1                     \n"
            " beq stacking_used_msp          \n"
            "                                \n"
            " stacking_used_psp:             \n"
            "    mrs r0, psp                 \n"
            "    ldr r3, =vPortSVCHandler_C  \n"
            "    bx r3                       \n"
            "                                \n"
            " stacking_used_msp:             \n"
            "    mrs r0, msp                 \n"
            "    ldr r3, =vPortSVCHandler_C  \n"
            "    bx r3                       \n"
            "                                \n"
            " .align 4                       \n"
        );
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/*-----------------------------------------------------------*/
