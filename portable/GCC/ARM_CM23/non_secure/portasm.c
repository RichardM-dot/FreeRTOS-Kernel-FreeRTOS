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

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可确保 PRIVILEGED_FUNCTION 被正确定义，
 * 并将特权级函数放置在正确的段中。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* portasm 头文件包含。 */
#include "portasm.h"

/* 系统调用编号头文件包含。 */
#include "mpu_syscall_numbers.h"

/*
 * MPU_WRAPPERS_INCLUDED_FROM_API_FILE 只需为头文件定义。
 */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configENABLE_FPU == 1 )
    #error Cortex-M23 does not have a Floating Point Unit (FPU) and therefore configENABLE_FPU must be set to 0.
#endif

#if ( configENABLE_MPU == 1 )
    #define SECURE_CONTEXT_OFFSET   -20
#else
    #define SECURE_CONTEXT_OFFSET   0
#endif

#if ( configENABLE_MPU == 1 )

    void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
    {
        __asm volatile
        (
            " .syntax unified                                 \n"
            "                                                 \n"
            " program_mpu_first_task:                         \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r0, [r3]                                 \n" /* r0 = pxCurrentTCB（加载 pxCurrentTCB 指针）。 */
            "                                                 \n"
            "    dmb                                          \n" /* 在禁用 MPU 之前完成未完成的传输。 */
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 = 1（设置立即数）。 */
            "    bics r2, r3                                  \n" /* r2 = r2 & ~r3，即清除 r2 中的位 0。 */
            "    str r2, [r1]                                 \n" /* 禁用 MPU。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。r0 现在指向 TCB 中的 MAIR0。 */
            "    ldr r1, [r0]                                 \n" /* r1 = *r0，即 r1 = MAIR0。 */
            "    ldr r2, =0xe000edc0                          \n" /* r2 = 0xe000edc0 [MAIR0 的地址]。 */
            "    str r1, [r2]                                 \n" /* 写入 MAIR0。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。r0 现在指向 TCB 中的第一个 RBAR。 */
            "    ldr r1, =0xe000ed98                          \n" /* r1 = 0xe000ed98 [RNR 的地址]。 */
            "                                                 \n"
            "    movs r3, #4                                  \n" /* r3 = 4（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 4。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第一组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第一组 RBAR/RLAR 寄存器。 */
            "    movs r3, #5                                  \n" /* r3 = 5（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 5。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第二组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第二组 RBAR/RLAR 寄存器。 */
            "    movs r3, #6                                  \n" /* r3 = 6（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 6。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第三组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第三组 RBAR/RLAR 寄存器。 */
            "    movs r3, #7                                  \n" /* r3 = 6（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 7。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第四组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第四组 RBAR/RLAR 寄存器。 */
            "                                                 \n"
            "   ldr r1, =0xe000ed94                           \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "   ldr r2, [r1]                                  \n" /* 读取 MPU_CTRL 的值。 */
            "   movs r3, #1                                   \n" /* r3 = 1（设置立即数）。 */
            "   orrs r2, r3                                   \n" /* r2 = r2 | r3，即设置 r2 中的位 0。 */
            "   str r2, [r1]                                  \n" /* 使能 MPU。 */
            "   dsb                                           \n" /* 在继续之前强制完成内存写入。 */
            "                                                 \n"
            " restore_context_first_task:                     \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r1, [r3]                                 \n" /* r1 = pxCurrentTCB（加载 pxCurrentTCB 指针）。 */
            "    ldr r2, [r1]                                 \n" /* r2 = TCB 中保存上下文的位置。 */
            "                                                 \n"
            " restore_special_regs_first_task:                \n"
            "    subs r2, #20                                 \n"
            "    ldmia r2!, {r0, r3-r6}                       \n" /* r0 = xSecureContext，r3 = 原始 PSP，r4 = PSPLIM，r5 = CONTROL，r6 = LR。 */
            "    subs r2, #20                                 \n"
            "    msr psp, r3                                  \n"
            "    msr control, r5                              \n"
            "    mov lr, r6                                   \n"
            "    ldr r4, =xSecureContext                      \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "    str r0, [r4]                                 \n" /* 恢复 xSecureContext。 */
            "                                                 \n"
            " restore_general_regs_first_task:                \n"
            "    subs r2, #32                                 \n"
            "    ldmia r2!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的一半。 */
            "    stmia r3!, {r4-r7}                           \n" /* 将硬件保存上下文的一半复制到任务栈。 */
            "    ldmia r2!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的另一半。 */
            "    stmia r3!, {r4-r7}                           \n" /* 将硬件保存上下文的另一半复制到任务栈。 */
            "    subs r2, #48                                 \n"
            "    ldmia r2!, {r4-r7}                           \n" /* 恢复 r8-r11。 */
            "    mov r8, r4                                   \n" /* r8 = r4（寄存器传值）。 */
            "    mov r9, r5                                   \n" /* r9 = r5（寄存器传值）。 */
            "    mov r10, r6                                  \n" /* r10 = r6（寄存器传值）。 */
            "    mov r11, r7                                  \n" /* r11 = r7（寄存器传值）。 */
            "    subs r2, #32                                 \n"
            "    ldmia r2!, {r4-r7}                           \n" /* 恢复 r4-r7。 */
            "    subs r2, #16                                 \n"
            "                                                 \n"
            " restore_context_done_first_task:                \n"
            "    str r2, [r1]                                 \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
            "    bx lr                                        \n"
        );
    }

#else /* configENABLE_MPU 条件编译结束 */

    void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
    {
        __asm volatile
        (
            "   .syntax unified                                 \n"
            "                                                   \n"
            "   ldr  r2, =pxCurrentTCB                          \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr  r3, [r2]                                   \n" /* 读取 pxCurrentTCB。 */
            "   ldr  r0, [r3]                                   \n" /* 从 TCB 读取栈顶——pxCurrentTCB 的第一项是任务栈顶。 */
            "                                                   \n"
            "   ldm  r0!, {r1-r3}                               \n" /* 从栈读取——r1 = xSecureContext，r2 = PSPLIM，r3 = EXC_RETURN。 */
            "   ldr  r4, =xSecureContext                        \n"
            "   str  r1, [r4]                                   \n" /* 将此任务的 xSecureContext 设置为其对应的值。 */
            "   movs r1, #2                                     \n" /* r1 = 2（设置立即数）。 */
            "   msr  CONTROL, r1                                \n" /* 切换为在线程模式下使用 PSP。 */
            "   adds r0, #32                                    \n" /* 丢弃 r0 之前的所有内容。 */
            "   msr  psp, r0                                    \n" /* 这现在是任务要使用的新栈顶。 */
            "   isb                                             \n"
            "   bx   r3                                         \n" /* 最后跳转到 EXC_RETURN。 */
        );
    }

#endif /* configENABLE_MPU 条件编译结束 */
/*-----------------------------------------------------------*/

BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked ))（裸函数属性） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* r0 = CONTROL（读取 CONTROL 寄存器）。 */
        "   movs r1, #1                                     \n" /* r1 = 1（设置立即数）。 */
        "   tst r0, r1                                      \n" /* 执行 r0 & r1（按位与）并更新条件标志。 */
        "   beq running_privileged                          \n" /* 如果前一按位与操作的结果为 0，则跳转。 */
        "   movs r0, #0                                     \n" /* CONTROL[0]!=0。返回 false 表示处理器不处于特权级。 */
        "   bx lr                                           \n" /* 返回。 */
        " running_privileged:                               \n"
        "   movs r0, #1                                     \n" /* CONTROL[0]==0。返回 true 表示处理器处于特权级。 */
        "   bx lr                                           \n" /* 返回。 */
        ::: "r0", "r1", "memory"
    );
}
/*-----------------------------------------------------------*/

void vRaisePrivilege( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* 读取 CONTROL 寄存器。 */
        "   movs r1, #1                                     \n" /* r1 = 1（设置立即数）。 */
        "   bics r0, r1                                     \n" /* 清除位 0。 */
        "   msr control, r0                                 \n" /* 写回新的 CONTROL 值。 */
        "   bx lr                                           \n" /* 返回调用者。 */
        ::: "r0", "r1", "memory"
    );
}
/*-----------------------------------------------------------*/

void vResetPrivilege( void ) /* __attribute__ (( naked ))（裸函数属性） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* r0 = CONTROL（读取 CONTROL 寄存器）。 */
        "   movs r1, #1                                     \n" /* r1 = 1（设置立即数）。 */
        "   orrs r0, r1                                     \n" /* r0 = r0 | r1（按位或）。 */
        "   msr control, r0                                 \n" /* CONTROL = r0（写回 CONTROL 寄存器）。 */
        "   bx lr                                           \n" /* 返回调用者。 */
        ::: "r0", "r1", "memory"
    );
}
/*-----------------------------------------------------------*/

void vStartFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   ldr r0, =0xe000ed08                             \n" /* 使用 NVIC 偏移寄存器定位栈。 */
        "   ldr r0, [r0]                                    \n" /* 读取 VTOR 寄存器，它给出向量表的地址。 */
        "   ldr r0, [r0]                                    \n" /* 向量表的第一项是栈指针。 */
        "   msr msp, r0                                     \n" /* 将 MSP 重新设置为栈的起始位置。 */
        "   cpsie i                                         \n" /* 全局使能中断。 */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   svc %0                                          \n" /* 通过系统调用启动第一个任务。 */
        "   nop                                             \n"
        ::"i" ( portSVC_START_SCHEDULER ) : "memory"
    );
}
/*-----------------------------------------------------------*/

uint32_t ulSetInterruptMask( void ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, PRIMASK                                 \n"
        "   cpsid i                                         \n"
        "   bx lr                                           \n"
        ::: "memory"
    );
}
/*-----------------------------------------------------------*/

void vClearInterruptMask( __attribute__( ( unused ) ) uint32_t ulMask ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   msr PRIMASK, r0                                 \n"
        "   bx lr                                           \n"
        ::: "memory"
    );
}
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
    {
        __asm volatile
        (
            " .syntax unified                                 \n"
            " .extern SecureContext_SaveContext               \n"
            " .extern SecureContext_LoadContext               \n"
            "                                                 \n"
            " ldr r3, =xSecureContext                         \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            " ldr r0, [r3]                                    \n" /* 读取 xSecureContext——xSecureContext 的值必须放入 r0，因为后续将其用作参数。 */
            " ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            " ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB——pxCurrentTCB 的值必须放入 r1，因为后续将其用作参数。 */
            " ldr r2, [r1]                                    \n" /* r2 = TCB 中应保存上下文的位置。 */
            "                                                 \n"
            " cbz r0, save_ns_context                         \n" /* 没有需要保存的安全上下文。 */
            " save_s_context:                                 \n"
            "    push {r0-r2, lr}                             \n"
            "    bl SecureContext_SaveContext                 \n" /* 参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "    pop {r0-r3}                                  \n" /* LR 现在位于 r3 中。 */
            "    mov lr, r3                                   \n" /* 恢复 LR。 */
            "                                                 \n"
            " save_ns_context:                                \n"
            "    mov r3, lr                                   \n" /* r3 = LR（EXC_RETURN）（读取异常返回信息）。 */
            "    lsls r3, r3, #25                             \n" /* r3 = r3 << 25。如果使用安全栈保存栈帧，则 EXC_RETURN 的位[6] 为 1；如果使用非安全栈，则为 0。 */
            "    bmi save_special_regs                        \n" /* r3 < 0 ==> EXC_RETURN 中的位[6] 为 1 ==> 使用安全栈保存了栈帧。 */
            "                                                 \n"
            " save_general_regs:                              \n"
            "    mrs r3, psp                                  \n"
            "    stmia r2!, {r4-r7}                           \n" /* 存储 r4-r7。 */
            "    mov r4, r8                                   \n" /* r4 = r8（寄存器传值）。 */
            "    mov r5, r9                                   \n" /* r5 = r9（寄存器传值）。 */
            "    mov r6, r10                                  \n" /* r6 = r10（寄存器传值）。 */
            "    mov r7, r11                                  \n" /* r7 = r11（寄存器传值）。 */
            "    stmia r2!, {r4-r7}                           \n" /* 存储 r8-r11。 */
            "    ldmia r3!, {r4-r7}                           \n" /* 将硬件保存上下文的一半复制到 r4-r7。 */
            "    stmia r2!, {r4-r7}                           \n" /* 存储硬件保存的上下文。 */
            "    ldmia r3!, {r4-r7}                           \n" /* 将硬件保存上下文的另一半复制到 r4-r7。 */
            "    stmia r2!, {r4-r7}                           \n" /* 存储硬件保存的上下文。 */
            "                                                 \n"
            " save_special_regs:                              \n"
            "    mrs r3, psp                                  \n" /* r3 = PSP（读取进程栈指针）。 */
            "    movs r4, #0                                  \n" /* r4 = 0。0 存储在 PSPLIM 槽位中。 */
            "    mrs r5, control                              \n" /* r5 = CONTROL（读取 CONTROL 寄存器）。 */
            "    mov r6, lr                                   \n" /* r6 = LR（读取链接寄存器）。 */
            "    stmia r2!, {r0, r3-r6}                       \n" /* 存储 xSecureContext、原始 PSP（硬件保存上下文之后）、PSPLIM、CONTROL 和 LR。 */
            "    str r2, [r1]                                 \n" /* 将恢复上下文的位置作为 TCB 的第一个成员保存。 */
            "                                                 \n"
            " select_next_task:                               \n"
            "    cpsid i                                      \n"
            "    bl vTaskSwitchContext                        \n"
            "    cpsie i                                      \n"
            "                                                 \n"
            " program_mpu:                                    \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r0, [r3]                                 \n" /* r0 = pxCurrentTCB（加载 pxCurrentTCB 指针）。 */
            "                                                 \n"
            "    dmb                                          \n" /* 在禁用 MPU 之前完成未完成的传输。 */
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 = 1（设置立即数）。 */
            "    bics r2, r3                                  \n" /* r2 = r2 & ~r3，即清除 r2 中的位 0。 */
            "    str r2, [r1]                                 \n" /* 禁用 MPU。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。r0 现在指向 TCB 中的 MAIR0。 */
            "    ldr r1, [r0]                                 \n" /* r1 = *r0，即 r1 = MAIR0。 */
            "    ldr r2, =0xe000edc0                          \n" /* r2 = 0xe000edc0 [MAIR0 的地址]。 */
            "    str r1, [r2]                                 \n" /* 写入 MAIR0。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。r0 现在指向 TCB 中的第一个 RBAR。 */
            "    ldr r1, =0xe000ed98                          \n" /* r1 = 0xe000ed98 [RNR 的地址]。 */
            "                                                 \n"
            "    movs r3, #4                                  \n" /* r3 = 4（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 4。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第一组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第一组 RBAR/RLAR 寄存器。 */
            "    movs r3, #5                                  \n" /* r3 = 5（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 5。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第二组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第二组 RBAR/RLAR 寄存器。 */
            "    movs r3, #6                                  \n" /* r3 = 6（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 6。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第三组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第三组 RBAR/RLAR 寄存器。 */
            "    movs r3, #7                                  \n" /* r3 = 6（设置立即数）。 */
            "    str r3, [r1]                                 \n" /* 设置 RNR = 7。 */
            "    ldmia r0!, {r4-r5}                           \n" /* 从 TCB 读取第四组 RBAR/RLAR 寄存器。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "    stmia r2!, {r4-r5}                           \n" /* 写入第四组 RBAR/RLAR 寄存器。 */
            "                                                 \n"
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    movs r3, #1                                  \n" /* r3 = 1（设置立即数）。 */
            "    orrs r2, r3                                  \n" /* r2 = r2 | r3，即设置 r2 中的位 0。 */
            "    str r2, [r1]                                 \n" /* 使能 MPU。 */
            "    dsb                                          \n" /* 在继续之前强制完成内存写入。 */
            "                                                 \n"
            " restore_context:                                \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r1, [r3]                                 \n" /* r1 = pxCurrentTCB（加载 pxCurrentTCB 指针）。 */
            "    ldr r2, [r1]                                 \n" /* r2 = TCB 中保存上下文的位置。 */
            "                                                 \n"
            " restore_special_regs:                           \n"
            "    subs r2, #20                                 \n"
            "    ldmia r2!, {r0, r3-r6}                       \n" /* r0 = xSecureContext，r3 = 原始 PSP，r4 = PSPLIM，r5 = CONTROL，r6 = LR。 */
            "    subs r2, #20                                 \n"
            "    msr psp, r3                                  \n"
            "    msr control, r5                              \n"
            "    mov lr, r6                                   \n"
            "    ldr r4, =xSecureContext                      \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "    str r0, [r4]                                 \n" /* 恢复 xSecureContext。 */
            "    cbz r0, restore_ns_context                   \n" /* 没有需要恢复的安全上下文。 */
            "                                                 \n"
            " restore_s_context:                              \n"
            "    push {r1-r3, lr}                             \n"
            "    bl SecureContext_LoadContext                 \n" /* 参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "    pop {r1-r4}                                  \n" /* LR 现在位于 r4 中。 */
            "    mov lr, r4                                   \n"
            "                                                 \n"
            " restore_ns_context:                             \n"
            "    mov r0, lr                                   \n" /* r0 = LR（EXC_RETURN）（读取异常返回信息）。 */
            "    lsls r0, r0, #25                             \n" /* r0 = r0 << 25。如果使用安全栈保存栈帧，则 EXC_RETURN 的位[6] 为 1；如果使用非安全栈，则为 0。 */
            "    bmi restore_context_done                     \n" /* r0 < 0 ==> EXC_RETURN 中的位[6] 为 1 ==> 使用安全栈保存了栈帧。 */
            "                                                 \n"
            " restore_general_regs:                           \n"
            "    subs r2, #32                                 \n"
            "    ldmia r2!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的一半。 */
            "    stmia r3!, {r4-r7}                           \n" /* 将硬件保存上下文的一半复制到任务栈。 */
            "    ldmia r2!, {r4-r7}                           \n" /* r4-r7 包含硬件保存上下文的另一半。 */
            "    stmia r3!, {r4-r7}                           \n" /* 将硬件保存上下文的另一半复制到任务栈。 */
            "    subs r2, #48                                 \n"
            "    ldmia r2!, {r4-r7}                           \n" /* 恢复 r8-r11。 */
            "    mov r8, r4                                   \n" /* r8 = r4（寄存器传值）。 */
            "    mov r9, r5                                   \n" /* r9 = r5（寄存器传值）。 */
            "    mov r10, r6                                  \n" /* r10 = r6（寄存器传值）。 */
            "    mov r11, r7                                  \n" /* r11 = r7（寄存器传值）。 */
            "    subs r2, #32                                 \n"
            "    ldmia r2!, {r4-r7}                           \n" /* 恢复 r4-r7。 */
            "    subs r2, #16                                 \n"
            "                                                 \n"
            " restore_context_done:                           \n"
            "    str r2, [r1]                                 \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
            "    bx lr                                        \n"
        );
    }

#else /* configENABLE_MPU 条件编译结束 */

    void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
    {
        __asm volatile
        (
            "   .syntax unified                                 \n"
            "   .extern SecureContext_SaveContext               \n"
            "   .extern SecureContext_LoadContext               \n"
            "                                                   \n"
            "   ldr r3, =xSecureContext                         \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "   ldr r0, [r3]                                    \n" /* 读取 xSecureContext——xSecureContext 的值必须放入 r0，因为后续将其用作参数。 */
            "   ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB——pxCurrentTCB 的值必须放入 r1，因为后续将其用作参数。 */
            "   mrs r2, psp                                     \n" /* 读取 PSP 到 r2。 */
            "                                                   \n"
            "   cbz r0, save_ns_context                         \n" /* 没有需要保存的安全上下文。 */
            "   push {r0-r2, r14}                               \n"
            "   bl SecureContext_SaveContext                    \n" /* 参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "   pop {r0-r3}                                     \n" /* LR 现在位于 r3 中。 */
            "   mov lr, r3                                      \n" /* LR = r3（恢复链接寄存器）。 */
            "   lsls r1, r3, #25                                \n" /* r1 = r3 << 25。如果使用安全栈保存栈帧，则 EXC_RETURN 的位[6] 为 1；如果使用非安全栈，则为 0。 */
            "   bpl save_ns_context                             \n" /* bpl——若为正或零则跳转。若 r1 >= 0 ==> EXC_RETURN 中的位[6] 为 0，即使用了非安全栈。 */
            "   ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB。 */
            "   subs r2, r2, #12                                \n" /* 在栈上为 xSecureContext、PSPLIM 和 LR 腾出空间。 */
            "   str r2, [r1]                                    \n" /* 将新的栈顶保存到 TCB。 */
            "   movs r1, #0                                     \n" /* r1 = 0。0 存储在 PSPLIM 槽位中。 */
            "   mov r3, lr                                      \n" /* r3 = LR/EXC_RETURN（读取链接寄存器或异常返回信息）。 */
            "   stmia r2!, {r0, r1, r3}                         \n" /* 将 xSecureContext、PSPLIM 和 LR 存储到栈上。 */
            "   b select_next_task                              \n"
            "                                                   \n"
            " save_ns_context:                                  \n"
            "   ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB。 */
            "   subs r2, r2, #44                                \n" /* 在栈上为 xSecureContext、PSPLIM、LR 及剩余寄存器腾出空间。 */
            "   str r2, [r1]                                    \n" /* 将新的栈顶保存到 TCB。 */
            "   movs r1, #0                                     \n" /* r1 = 0。0 存储在 PSPLIM 槽位中。 */
            "   mov r3, lr                                      \n" /* r3 = LR/EXC_RETURN（读取链接寄存器或异常返回信息）。 */
            "   stmia r2!, {r0, r1, r3-r7}                      \n" /* 存储 xSecureContext、PSPLIM、LR 以及不会自动保存的低寄存器。 */
            "   mov r4, r8                                      \n" /* r4 = r8（寄存器传值）。 */
            "   mov r5, r9                                      \n" /* r5 = r9（寄存器传值）。 */
            "   mov r6, r10                                     \n" /* r6 = r10（寄存器传值）。 */
            "   mov r7, r11                                     \n" /* r7 = r11（寄存器传值）。 */
            "   stmia r2!, {r4-r7}                              \n" /* 存储不会自动保存的高寄存器。 */
            "                                                   \n"
            " select_next_task:                                 \n"
            "   cpsid i                                         \n"
            "   bl vTaskSwitchContext                           \n"
            "   cpsie i                                         \n"
            "                                                   \n"
            "   ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB。 */
            "   ldr r2, [r1]                                    \n" /* pxCurrentTCB 的第一项是任务栈顶。r2 现在指向栈顶。 */
            "                                                   \n"
            "   ldmia r2!, {r0, r1, r4}                         \n" /* 从栈读取——r0 = xSecureContext，r1 = PSPLIM，r4 = LR。 */
            "   mov lr, r4                                      \n" /* LR = r4（恢复链接寄存器）。 */
            "   ldr r3, =xSecureContext                         \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "   str r0, [r3]                                    \n" /* 恢复任务的 xSecureContext。 */
            "   cbz r0, restore_ns_context                      \n" /* 如果任务没有安全上下文，则恢复非安全上下文。 */
            "   ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB。 */
            "   push {r2, r4}                                   \n"
            "   bl SecureContext_LoadContext                    \n" /* 恢复安全上下文。参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "   pop {r2, r4}                                    \n"
            "   mov lr, r4                                      \n" /* LR = r4（恢复链接寄存器）。 */
            "   lsls r1, r4, #25                                \n" /* r1 = r4 << 25。如果使用安全栈保存栈帧，则 EXC_RETURN 的位[6] 为 1；如果使用非安全栈，则为 0。 */
            "   bpl restore_ns_context                          \n" /* bpl——若为正或零则跳转。若 r1 >= 0 ==> EXC_RETURN 中的位[6] 为 0，即使用了非安全栈。 */
            "   msr psp, r2                                     \n" /* 记住任务的新栈顶。 */
            "   bx lr                                           \n"
            "                                                   \n"
            " restore_ns_context:                               \n"
            "   adds r2, r2, #16                                \n" /* 切换到高寄存器。 */
            "   ldmia r2!, {r4-r7}                              \n" /* 恢复不会自动恢复的高寄存器。 */
            "   mov r8, r4                                      \n" /* r8 = r4（寄存器传值）。 */
            "   mov r9, r5                                      \n" /* r9 = r5（寄存器传值）。 */
            "   mov r10, r6                                     \n" /* r10 = r6（寄存器传值）。 */
            "   mov r11, r7                                     \n" /* r11 = r7（寄存器传值）。 */
            "   msr psp, r2                                     \n" /* 记住任务的新栈顶。 */
            "   subs r2, r2, #32                                \n" /* 返回低寄存器。 */
            "   ldmia r2!, {r4-r7}                              \n" /* 恢复不会自动恢复的低寄存器。 */
            "   bx lr                                           \n"
        );
    }

#endif /* configENABLE_MPU 条件编译结束 */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
    {
        __asm volatile
        (
            ".syntax unified                \n"
            ".extern vPortSVCHandler_C      \n"
            ".extern vSystemCallEnter       \n"
            ".extern vSystemCallExit        \n"
            "                               \n"
            "movs r0, #4                    \n"
            "mov r1, lr                     \n"
            "tst r0, r1                     \n"
            "beq stack_on_msp               \n"
            "stack_on_psp:                  \n"
            "    mrs r0, psp                \n"
            "    b route_svc                \n"
            "stack_on_msp:                  \n"
            "    mrs r0, msp                \n"
            "    b route_svc                \n"
            "                               \n"
            "route_svc:                     \n"
            "    ldr r3, [r0, #24]          \n"
            "    subs r3, #2                \n"
            "    ldrb r2, [r3, #0]          \n"
            "    cmp r2, %0                 \n"
            "    blt system_call_enter      \n"
            "    cmp r2, %1                 \n"
            "    beq system_call_exit       \n"
            "    b vPortSVCHandler_C        \n"
            "                               \n"
            "system_call_enter:             \n"
            "    b vSystemCallEnter         \n"
            "system_call_exit:              \n"
            "    b vSystemCallExit          \n"
            "                               \n"
            : /* 无输出。 */
            : "i" ( NUM_SYSTEM_CALLS ), "i" ( portSVC_SYSTEM_CALL_EXIT )
            : "r0", "r1", "r2", "r3", "memory"
        );
    }

#else /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) 条件编译结束 */

    void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
    {
        __asm volatile
        (
            "   .syntax unified                                 \n"
            "                                                   \n"
            "   movs r0, #4                                     \n"
            "   mov r1, lr                                      \n"
            "   tst r0, r1                                      \n"
            "   beq stacking_used_msp                           \n"
            "   mrs r0, psp                                     \n"
            "   ldr r2, =vPortSVCHandler_C                      \n"
            "   bx r2                                           \n"
            " stacking_used_msp:                                \n"
            "   mrs r0, msp                                     \n"
            "   ldr r2, =vPortSVCHandler_C                      \n"
            "   bx r2                                           \n"
        );
    }

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) 条件编译结束 */
/*-----------------------------------------------------------*/

void vPortAllocateSecureContext( uint32_t ulSecureStackSize ) /* __attribute__ (( naked ))（裸函数属性） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   svc %0                                          \n" /* 安全上下文在超级调用（SVC）中分配。 */
        "   bx lr                                           \n" /* 返回。 */
        ::"i" ( portSVC_ALLOCATE_SECURE_CONTEXT ) : "memory"
    );
}
/*-----------------------------------------------------------*/

void vPortFreeSecureContext( uint32_t * pulTCB ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数与特权级函数标记） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   ldr r2, [r0]                                    \n" /* TCB 的第一项是存储的上下文位置。 */
        "   adds r2, r2, %0                                 \n" /* r2 = r2 + SECURE_CONTEXT_OFFSET（调整安全上下文偏移）。 */
        "   ldr r1, [r2]                                    \n" /* 从任务的上下文中读取 xSecureContext。 */
        "   cmp r1, #0                                      \n" /* 如果任务的 xSecureContext 非 NULL，则触发 SVC。 */
        "   bne free_secure_context                         \n" /* 如果 r1 != 0 则跳转。 */
        "   bx lr                                           \n" /* 没有安全上下文（xSecureContext 为 NULL）。 */
        " free_secure_context:                              \n"
        "   svc %1                                          \n" /* 安全上下文在超级调用（SVC）中释放。 */
        "   bx lr                                           \n" /* 返回。 */
        ::"i" ( SECURE_CONTEXT_OFFSET ), "i" ( portSVC_FREE_SECURE_CONTEXT ) : "memory"
    );
}
/*-----------------------------------------------------------*/
