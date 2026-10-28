/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2024 Arm Limited and/or its affiliates
 * <open-source-office@arm.com>
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
    #if ( configENABLE_PAC == 1 )
        #define SECURE_CONTEXT_OFFSET   -36
    #else
        #define SECURE_CONTEXT_OFFSET   -20
    #endif
#else
    #define SECURE_CONTEXT_OFFSET       0
#endif

#if ( configENABLE_MPU == 1 )

    void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .syntax unified                                 \n"
            "                                                 \n"
            " program_mpu_first_task:                         \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r0, [r3]                                 \n" /* r0 置为 pxCurrentTCB。 */
            "                                                 \n"
            "    dmb                                          \n" /* 在禁用 MPU 前完成未完成的传输。 */
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    bic r2, #1                                   \n" /* r2 = r2 & ~1，即清除 r2 的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 禁用 MPU。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。此时 r0 指向 TCB 中的 MAIR0。 */
            "    ldr r1, [r0]                                 \n" /* r1 = *r0，即 r1 = MAIR0。 */
            "    ldr r2, =0xe000edc0                          \n" /* r2 = 0xe000edc0 [MAIR0 的地址]。 */
            "    str r1, [r2]                                 \n" /* 写入 MAIR0。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。此时 r0 指向 TCB 中的第一个 RBAR。 */
            "    ldr r1, =0xe000ed98                          \n" /* r1 = 0xe000ed98 [RNR 的地址]。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "                                                 \n"
            "    movs r3, #4                                  \n" /* r3 = 4（区域编号）。 */
            "    str r3, [r1]                                 \n" /* 写入 RNR = 4。 */
            "    ldmia r0!, {r4-r11}                          \n" /* 从 TCB 中读取 4 组 RBAR/RLAR 寄存器值。 */
            "    stmia r2, {r4-r11}                           \n" /* 使用别名寄存器写入 4 组 RBAR/RLAR 寄存器。 */
            "                                                 \n"
        #if ( configTOTAL_MPU_REGIONS == 16 )
            "    movs r3, #8                                  \n" /* r3 = 8（区域编号）。 */
            "    str r3, [r1]                                 \n" /* 写入 RNR = 8。 */
            "    ldmia r0!, {r4-r11}                          \n" /* 从 TCB 中读取 4 组 RBAR/RLAR 寄存器值。 */
            "    stmia r2, {r4-r11}                           \n" /* 使用别名寄存器写入 4 组 RBAR/RLAR 寄存器。 */
            "    movs r3, #12                                 \n" /* r3 = 12（区域编号）。 */
            "    str r3, [r1]                                 \n" /* 写入 RNR = 12。 */
            "    ldmia r0!, {r4-r11}                          \n" /* 从 TCB 中读取 4 组 RBAR/RLAR 寄存器值。 */
            "    stmia r2, {r4-r11}                           \n" /* 使用别名寄存器写入 4 组 RBAR/RLAR 寄存器。 */
        #endif /* configTOTAL_MPU_REGIONS == 16（MPU 区域总数为 16） */
            "                                                 \n"
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    orr r2, #1                                   \n" /* r2 = r1 | 1，即置位 r2 的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 使能 MPU。 */
            "    dsb                                          \n" /* 继续执行前强制完成内存写入。 */
            "                                                 \n"
            " restore_context_first_task:                     \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r1, [r3]                                 \n" /* r1 置为 pxCurrentTCB。 */
            "    ldr r2, [r1]                                 \n" /* r2 = TCB 中保存上下文的地址。 */
            "                                                 \n"
            " restore_special_regs_first_task:                \n"
        #if ( configENABLE_PAC == 1 )
            "   ldmdb r2!, {r3-r6}                            \n" /* 从任务上下文中读取该任务专用的 PAC 密钥。 */
            "   msr  PAC_KEY_P_0, r3                          \n" /* 将任务专用的 PAC 密钥写入 PAC 密钥寄存器。 */
            "   msr  PAC_KEY_P_1, r4                          \n"
            "   msr  PAC_KEY_P_2, r5                          \n"
            "   msr  PAC_KEY_P_3, r6                          \n"
            "   clrm {r3-r6}                                  \n" /* 清除 r3-r6。 */
        #endif /* configENABLE_PAC（使能 PAC） */
            "    ldmdb r2!, {r0, r3-r5, lr}                   \n" /* r0 = xSecureContext，r3 = 原始 PSP，r4 = PSPLIM，r5 = CONTROL，LR 已恢复。 */
            "    msr psp, r3                                  \n"
            "    msr psplim, r4                               \n"
            "    msr control, r5                              \n"
            "    ldr r4, =xSecureContext                      \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "    str r0, [r4]                                 \n" /* 恢复 xSecureContext。 */
            "                                                 \n"
            " restore_general_regs_first_task:                \n"
            "    ldmdb r2!, {r4-r11}                          \n" /* r4-r11 中包含硬件保存的上下文。 */
            "    stmia r3!, {r4-r11}                          \n" /* 将硬件保存的上下文复制到任务栈上。 */
            "    ldmdb r2!, {r4-r11}                          \n" /* r4-r11 已恢复。 */
            "                                                 \n"
            " restore_context_done_first_task:                \n"
            "    str r2, [r1]                                 \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
            "    mov r0, #0                                   \n"
            "    msr basepri, r0                              \n" /* 确保第一个任务启动时中断已使能。 */
            "    bx lr                                        \n"
        );
    }

#else /* configENABLE_MPU（使能 MPU） */

    void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            "   .syntax unified                                 \n"
            "                                                   \n"
            "   ldr  r2, =pxCurrentTCB                          \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "   ldr  r3, [r2]                                   \n" /* 读取 pxCurrentTCB。 */
            "   ldr  r0, [r3]                                   \n" /* 从 TCB 读取任务栈顶——pxCurrentTCB 中的第一项就是任务栈顶。 */
            "                                                   \n"
        #if ( configENABLE_PAC == 1 )
            "   ldmia r0!, {r1-r4}                              \n" /* 从栈中读取任务专用的 PAC 密钥。 */
            "   msr  PAC_KEY_P_3, r1                            \n" /* 将任务专用的 PAC 密钥写入 PAC 密钥寄存器。 */
            "   msr  PAC_KEY_P_2, r2                            \n"
            "   msr  PAC_KEY_P_1, r3                            \n"
            "   msr  PAC_KEY_P_0, r4                            \n"
            "   clrm {r1-r4}                                    \n" /* 清除 r1-r4。 */
        #endif /* configENABLE_PAC（使能 PAC） */
            "                                                   \n"
            "   ldm  r0!, {r1-r3}                               \n" /* 从栈中读取：r1 = xSecureContext，r2 = PSPLIM，r3 = EXC_RETURN。 */
            "   ldr  r4, =xSecureContext                        \n"
            "   str  r1, [r4]                                   \n" /* 将 xSecureContext 设置为该任务对应的值。 */
            "   msr  psplim, r2                                 \n" /* 设置该任务的 PSPLIM 值。 */
            "   mrs  r1, control                                \n" /* 获取 CONTROL 寄存器的当前值。 */
            "   orrs r1, r1, #2                                 \n" /* r1 = r1 | 0x2 —— 置位 bit 1，以使用进程栈指针（PSP）。 */
            "   msr control, r1                                 \n" /* 写回新的 CONTROL 寄存器值。 */
            "   adds r0, #32                                    \n" /* 丢弃 r0 之前的所有内容。 */
            "   msr  psp, r0                                    \n" /* 这现在是任务要使用的新栈顶。 */
            "   isb                                             \n"
            "   mov  r0, #0                                     \n"
            "   msr  basepri, r0                                \n" /* 确保第一个任务启动时中断已使能。 */
            "   bx   r3                                         \n" /* 最后，跳转到 EXC_RETURN。 */
        );
    }

#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* r0 置为 CONTROL。 */
        "   tst r0, #1                                      \n" /* 执行 r0 & 1（按位与）并更新条件标志。 */
        "   ite ne                                          \n"
        "   movne r0, #0                                    \n" /* CONTROL[0]!=0。返回 false 表示处理器当前不处于特权状态。 */
        "   moveq r0, #1                                    \n" /* CONTROL[0]==0。返回 true 表示处理器当前处于特权状态。 */
        "   bx lr                                           \n" /* 返回。 */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vRaisePrivilege( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* 读取 CONTROL 寄存器。 */
        "   bic r0, #1                                      \n" /* 清除 bit 0。 */
        "   msr control, r0                                 \n" /* 写回新的 CONTROL 值。 */
        "   bx lr                                           \n" /* 返回调用者。 */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vResetPrivilege( void ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* r0 置为 CONTROL。 */
        "   orr r0, #1                                      \n" /* r0 = r0 | 1（置位 bit 0）。 */
        "   msr control, r0                                 \n" /* CONTROL 置为 r0。 */
        "   bx lr                                           \n" /* 返回调用者。 */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vStartFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   ldr r0, =0xe000ed08                             \n" /* 使用 NVIC 偏移寄存器定位栈。 */
        "   ldr r0, [r0]                                    \n" /* 读取 VTOR 寄存器，该寄存器给出向量表的地址。 */
        "   ldr r0, [r0]                                    \n" /* 向量表中的第一项是栈指针。 */
        "   msr msp, r0                                     \n" /* 将 MSP 重新设置为栈的起始地址。 */
        "   cpsie i                                         \n" /* 全局使能中断。 */
        "   cpsie f                                         \n"
        "   dsb                                             \n"
        "   isb                                             \n"
        "   svc %0                                          \n" /* 通过系统调用启动第一个任务。 */
        "   nop                                             \n"
        ::"i" ( portSVC_START_SCHEDULER ) : "memory"
    );
}
/*-----------------------------------------------------------*/

uint32_t ulSetInterruptMask( void ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, basepri                                 \n" /* r0 = basepri，返回原始的 basepri 值。 */
        "   mov r1, %0                                      \n" /* r1 = configMAX_SYSCALL_INTERRUPT_PRIORITY（系统调用可屏蔽的最高中断优先级）。 */
        "   msr basepri, r1                                 \n" /* 关闭优先级低于或等于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断。 */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   bx lr                                           \n" /* 返回。 */
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) : "memory"
    );
}
/*-----------------------------------------------------------*/

void vClearInterruptMask( __attribute__( ( unused ) ) uint32_t ulMask ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   msr basepri, r0                                 \n" /* basepri = ulMask（写入屏蔽值）。 */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   bx lr                                           \n" /* 返回。 */
        ::: "memory"
    );
}
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .syntax unified                                 \n"
            " .extern SecureContext_SaveContext               \n"
            " .extern SecureContext_LoadContext               \n"
            "                                                 \n"
            " ldr r3, =xSecureContext                         \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            " ldr r0, [r3]                                    \n" /* 读取 xSecureContext —— 其值必须放入 r0，因为后续将作为参数使用。 */
            " ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            " ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB —— 其值必须放入 r1，因为后续将作为参数使用。 */
            " ldr r2, [r1]                                    \n" /* r2 = TCB 中用于保存上下文的位置。 */
            "                                                 \n"
            " cbz r0, save_ns_context                         \n" /* 没有需要保存的安全上下文。 */
            " save_s_context:                                 \n"
            "    push {r0-r2, lr}                             \n"
            "    bl SecureContext_SaveContext                 \n" /* 参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "    pop {r0-r2, lr}                              \n"
            "                                                 \n"
            " save_ns_context:                                \n"
            "    mov r3, lr                                   \n" /* r3 = LR（EXC_RETURN，异常返回值）。 */
            "    lsls r3, r3, #25                             \n" /* r3 = r3 << 25。若使用安全栈保存栈帧，EXC_RETURN 的 bit[6] 为 1；若使用非安全栈，则为 0。 */
            "    bmi save_special_regs                        \n" /* r3 < 0 ==> EXC_RETURN 的 bit[6] 为 1 ==> 使用安全栈保存了栈帧。 */
            "                                                 \n"
            " save_general_regs:                              \n"
            "    mrs r3, psp                                  \n"
        #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            "    add r3, r3, #0x20                            \n" /* 将 r3 移动到保存 s0 的位置。 */
            "    tst lr, #0x10                                \n"
            "    ittt eq                                      \n"
            "    vstmiaeq r2!, {s16-s31}                      \n" /* 保存 s16-s31。 */
            "    vldmiaeq r3, {s0-s16}                        \n" /* 将硬件保存的 FP 上下文复制到 s0-s16。 */
            "    vstmiaeq r2!, {s0-s16}                       \n" /* 保存硬件保存的 FP 上下文。 */
            "    sub r3, r3, #0x20                            \n" /* 将 r3 重新设置为硬件保存上下文的位置。 */
        #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */
            "    stmia r2!, {r4-r11}                          \n" /* 保存 r4-r11。 */
            "    ldmia r3, {r4-r11}                           \n" /* 将硬件保存的上下文复制到 r4-r11。 */
            "    stmia r2!, {r4-r11}                          \n" /* 保存硬件保存的上下文。 */
            "                                                 \n"
            " save_special_regs:                              \n"
            "    mrs r3, psp                                  \n" /* r3 = PSP（读取进程栈指针）。 */
            "    mrs r4, psplim                               \n" /* r4 = PSPLIM（读取进程栈限）。 */
            "    mrs r5, control                              \n" /* r5 = CONTROL（读取控制寄存器）。 */
            "    stmia r2!, {r0, r3-r5, lr}                   \n" /* 保存 xSecureContext、原始 PSP（硬件保存上下文之后的值）、PSPLIM、CONTROL 和 LR。 */
        #if ( configENABLE_PAC == 1 )
            "   mrs  r3, PAC_KEY_P_0                          \n" /* 从 PAC 密钥寄存器中读取任务专用的 PAC 密钥。 */
            "   mrs  r4, PAC_KEY_P_1                          \n"
            "   mrs  r5, PAC_KEY_P_2                          \n"
            "   mrs  r6, PAC_KEY_P_3                          \n"
            "   stmia r2!, {r3-r6}                            \n" /* 将任务专用的 PAC 密钥保存到任务上下文中。 */
            "   clrm {r3-r6}                                  \n" /* 清除 r3-r6。 */
        #endif /* configENABLE_PAC（使能 PAC） */
            "    str r2, [r1]                                 \n"      /* 将恢复上下文的位置作为 TCB 的第一个成员保存。 */
            "                                                 \n"
            " select_next_task:                               \n"
            "    mov r0, %0                                   \n" /* r0 = configMAX_SYSCALL_INTERRUPT_PRIORITY（系统调用可屏蔽的最高中断优先级） */
            "    msr basepri, r0                              \n" /* 关闭优先级低于或等于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断。 */
            "    dsb                                          \n"
            "    isb                                          \n"
            "    bl vTaskSwitchContext                        \n"
            "    mov r0, #0                                   \n" /* r0 = 0（清零）。 */
            "    msr basepri, r0                              \n" /* 使能中断。 */
            "                                                 \n"
            " program_mpu:                                    \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r0, [r3]                                 \n" /* r0 置为 pxCurrentTCB。 */
            "                                                 \n"
            "    dmb                                          \n" /* 在禁用 MPU 前完成未完成的传输。 */
            "    ldr r1, =0xe000ed94                          \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "    ldr r2, [r1]                                 \n" /* 读取 MPU_CTRL 的值。 */
            "    bic r2, #1                                   \n" /* r2 = r2 & ~1，即清除 r2 的 bit 0。 */
            "    str r2, [r1]                                 \n" /* 禁用 MPU。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。此时 r0 指向 TCB 中的 MAIR0。 */
            "    ldr r1, [r0]                                 \n" /* r1 = *r0，即 r1 = MAIR0。 */
            "    ldr r2, =0xe000edc0                          \n" /* r2 = 0xe000edc0 [MAIR0 的地址]。 */
            "    str r1, [r2]                                 \n" /* 写入 MAIR0。 */
            "                                                 \n"
            "    adds r0, #4                                  \n" /* r0 = r0 + 4。此时 r0 指向 TCB 中的第一个 RBAR。 */
            "    ldr r1, =0xe000ed98                          \n" /* r1 = 0xe000ed98 [RNR 的地址]。 */
            "    ldr r2, =0xe000ed9c                          \n" /* r2 = 0xe000ed9c [RBAR 的地址]。 */
            "                                                 \n"
            "    movs r3, #4                                  \n" /* r3 = 4（区域编号）。 */
            "    str r3, [r1]                                 \n" /* 写入 RNR = 4。 */
            "    ldmia r0!, {r4-r11}                          \n" /* 从 TCB 中读取 4 组 RBAR/RLAR 寄存器值。 */
            "    stmia r2, {r4-r11}                           \n" /* 使用别名寄存器写入 4 组 RBAR/RLAR 寄存器。 */
            "                                                 \n"
        #if ( configTOTAL_MPU_REGIONS == 16 )
            "    movs r3, #8                                  \n" /* r3 = 8（区域编号）。 */
            "    str r3, [r1]                                 \n" /* 写入 RNR = 8。 */
            "    ldmia r0!, {r4-r11}                          \n" /* 从 TCB 中读取 4 组 RBAR/RLAR 寄存器值。 */
            "    stmia r2, {r4-r11}                           \n" /* 使用别名寄存器写入 4 组 RBAR/RLAR 寄存器。 */
            "    movs r3, #12                                 \n" /* r3 = 12（区域编号）。 */
            "    str r3, [r1]                                 \n" /* 写入 RNR = 12。 */
            "    ldmia r0!, {r4-r11}                          \n" /* 从 TCB 中读取 4 组 RBAR/RLAR 寄存器值。 */
            "    stmia r2, {r4-r11}                           \n" /* 使用别名寄存器写入 4 组 RBAR/RLAR 寄存器。 */
        #endif /* configTOTAL_MPU_REGIONS == 16（MPU 区域总数为 16） */
            "                                                 \n"
            "   ldr r1, =0xe000ed94                           \n" /* r1 = 0xe000ed94 [MPU_CTRL 的地址]。 */
            "   ldr r2, [r1]                                  \n" /* 读取 MPU_CTRL 的值。 */
            "   orr r2, #1                                    \n" /* r2 = r2 | 1，即置位 r2 的 bit 0。 */
            "   str r2, [r1]                                  \n" /* 使能 MPU。 */
            "   dsb                                           \n" /* 继续执行前强制完成内存写入。 */
            "                                                 \n"
            " restore_context:                                \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r1, [r3]                                 \n" /* r1 置为 pxCurrentTCB。 */
            "    ldr r2, [r1]                                 \n" /* r2 = TCB 中保存上下文的地址。 */
            "                                                 \n"
            " restore_special_regs:                           \n"
        #if ( configENABLE_PAC == 1 )
            "   ldmdb r2!, {r3-r6}                            \n" /* 从任务上下文中读取该任务专用的 PAC 密钥。 */
            "   msr  PAC_KEY_P_0, r3                          \n" /* 将任务专用的 PAC 密钥写入 PAC 密钥寄存器。 */
            "   msr  PAC_KEY_P_1, r4                          \n"
            "   msr  PAC_KEY_P_2, r5                          \n"
            "   msr  PAC_KEY_P_3, r6                          \n"
            "   clrm {r3-r6}                                  \n" /* 清除 r3-r6。 */
        #endif /* configENABLE_PAC（使能 PAC） */
            "    ldmdb r2!, {r0, r3-r5, lr}                   \n" /* r0 = xSecureContext，r3 = 原始 PSP，r4 = PSPLIM，r5 = CONTROL，LR 已恢复。 */
            "    msr psp, r3                                  \n"
            "    msr psplim, r4                               \n"
            "    msr control, r5                              \n"
            "    ldr r4, =xSecureContext                      \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "    str r0, [r4]                                 \n" /* 恢复 xSecureContext。 */
            "    cbz r0, restore_ns_context                   \n" /* 没有需要恢复的安全上下文。 */
            "                                                 \n"
            " restore_s_context:                              \n"
            "    push {r1-r3, lr}                             \n"
            "    bl SecureContext_LoadContext                 \n" /* 参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "    pop {r1-r3, lr}                              \n"
            "                                                 \n"
            " restore_ns_context:                             \n"
            "    mov r0, lr                                   \n" /* r0 = LR（EXC_RETURN，异常返回值）。 */
            "    lsls r0, r0, #25                             \n" /* r0 = r0 << 25。若使用安全栈保存栈帧，EXC_RETURN 的 bit[6] 为 1；若使用非安全栈，则为 0。 */
            "    bmi restore_context_done                     \n" /* r0 < 0 ==> EXC_RETURN 的 bit[6] 为 1 ==> 使用安全栈保存了栈帧。 */
            "                                                 \n"
            " restore_general_regs:                           \n"
            "    ldmdb r2!, {r4-r11}                          \n" /* r4-r11 中包含硬件保存的上下文。 */
            "    stmia r3!, {r4-r11}                          \n" /* 将硬件保存的上下文复制到任务栈上。 */
            "    ldmdb r2!, {r4-r11}                          \n" /* r4-r11 已恢复。 */
        #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            "    tst lr, #0x10                                \n"
            "    ittt eq                                      \n"
            "    vldmdbeq r2!, {s0-s16}                       \n" /* s0-s16 中包含硬件保存的 FP 上下文。 */
            "    vstmiaeq r3!, {s0-s16}                       \n" /* 将硬件保存的 FP 上下文复制到任务栈上。 */
            "    vldmdbeq r2!, {s16-s31}                      \n" /* 恢复 s16-s31。 */
        #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */
            "                                                 \n"
            " restore_context_done:                           \n"
            "    str r2, [r1]                                 \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
            "    bx lr                                        \n"
            ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
        );
    }

#else /* configENABLE_MPU（使能 MPU） */

    void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            " .syntax unified                                 \n"
            " .extern SecureContext_SaveContext               \n"
            " .extern SecureContext_LoadContext               \n"
            "                                                 \n"
            " ldr r3, =xSecureContext                         \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            " ldr r0, [r3]                                    \n" /* 读取 xSecureContext —— 其值必须放入 r0，因为后续将作为参数使用。 */
            " ldr r3, =pxCurrentTCB                           \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            " ldr r1, [r3]                                    \n" /* 读取 pxCurrentTCB —— 其值必须放入 r1，因为后续将作为参数使用。 */
            " mrs r2, psp                                     \n" /* 将 PSP 读入 r2。 */
            "                                                 \n"
            " cbz r0, save_ns_context                         \n" /* 没有需要保存的安全上下文。 */
            " save_s_context:                                 \n"
            "    push {r0-r2, lr}                             \n"
            "    bl SecureContext_SaveContext                 \n" /* 参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "    pop {r0-r2, lr}                              \n"
            "                                                 \n"
            " save_ns_context:                                \n"
            "    mov r3, lr                                   \n" /* r3 = LR（EXC_RETURN，异常返回值）。 */
            "    lsls r3, r3, #25                             \n" /* r3 = r3 << 25。若使用安全栈保存栈帧，EXC_RETURN 的 bit[6] 为 1；若使用非安全栈，则为 0。 */
            "    bmi save_special_regs                        \n" /* 若 r3 < 0 ==> EXC_RETURN 的 bit[6] 为 1 ==> 使用了安全栈。 */
            "                                                 \n"
            " save_general_regs:                              \n"
        #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            "    tst lr, #0x10                                \n" /* 测试 LR 的 bit[4]。若使用扩展栈帧，EXC_RETURN 的 bit[4] 为 0。 */
            "    it eq                                        \n"
            "    vstmdbeq r2!, {s16-s31}                      \n" /* 保存不会自动保存的额外 FP 上下文寄存器。 */
        #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */
            "   stmdb r2!, {r4-r11}                           \n" /* 保存不会自动保存的寄存器。 */
            "                                                 \n"
            " save_special_regs:                              \n"
            "    mrs r3, psplim                               \n" /* r3 = PSPLIM（读取进程栈限）。 */
            "    stmdb r2!, {r0, r3, lr}                      \n" /* 将 xSecureContext、PSPLIM 和 LR 保存到栈上。 */
        #if ( configENABLE_PAC == 1 )
            "    mrs  r3, PAC_KEY_P_3                         \n" /* 从 PAC 密钥寄存器中读取任务专用的 PAC 密钥。 */
            "    mrs  r4, PAC_KEY_P_2                         \n"
            "    mrs  r5, PAC_KEY_P_1                         \n"
            "    mrs  r6, PAC_KEY_P_0                         \n"
            "    stmdb r2!, {r3-r6}                           \n" /* 将任务专用的 PAC 密钥保存到栈上。 */
            "    clrm {r3-r6}                                 \n" /* 清除 r3-r6。 */
        #endif /* configENABLE_PAC（使能 PAC） */
            "                                                 \n"
            " str r2, [r1]                                    \n" /* 将新的栈顶保存到 TCB。 */
            "                                                 \n"
            " select_next_task:                               \n"
            "    mov r0, %0                                   \n" /* r0 = configMAX_SYSCALL_INTERRUPT_PRIORITY（系统调用可屏蔽的最高中断优先级） */
            "    msr basepri, r0                              \n" /* 关闭优先级低于或等于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断。 */
            "    dsb                                          \n"
            "    isb                                          \n"
            "    bl vTaskSwitchContext                        \n"
            "    mov r0, #0                                   \n" /* r0 = 0（清零）。 */
            "    msr basepri, r0                              \n" /* 使能中断。 */
            "                                                 \n"
            " restore_context:                                \n"
            "    ldr r3, =pxCurrentTCB                        \n" /* 读取 pxCurrentTCB 的地址，即 &( pxCurrentTCB )。 */
            "    ldr r1, [r3]                                 \n" /* 读取 pxCurrentTCB。 */
            "    ldr r2, [r1]                                 \n" /* pxCurrentTCB 的第一项是任务栈顶。此时 r2 指向栈顶。 */
            "                                                 \n"
            " restore_special_regs:                           \n"
        #if ( configENABLE_PAC == 1 )
            "    ldmia r2!, {r3-r6}                           \n" /* 从栈中读取任务专用的 PAC 密钥。 */
            "    msr  PAC_KEY_P_3, r3                         \n" /* 将任务专用的 PAC 密钥写入 PAC 密钥寄存器。 */
            "    msr  PAC_KEY_P_2, r4                         \n"
            "    msr  PAC_KEY_P_1, r5                         \n"
            "    msr  PAC_KEY_P_0, r6                         \n"
            "    clrm {r3-r6}                                 \n" /* 清除 r3-r6。 */
        #endif /* configENABLE_PAC（使能 PAC） */
            "    ldmia r2!, {r0, r3, lr}                      \n" /* 从栈中读取：r0 = xSecureContext，r3 = PSPLIM，LR 已恢复。 */
            "    msr psplim, r3                               \n" /* 恢复该任务的 PSPLIM 寄存器值。 */
            "    ldr r3, =xSecureContext                      \n" /* 读取 xSecureContext 的地址，即 &( xSecureContext )。 */
            "    str r0, [r3]                                 \n" /* 恢复该任务的 xSecureContext。 */
            "    cbz r0, restore_ns_context                   \n" /* 若该任务没有安全上下文，则恢复非安全上下文。 */
            "                                                 \n"
            " restore_s_context:                              \n"
            "    push {r1-r3, lr}                             \n"
            "    bl SecureContext_LoadContext                 \n" /* 恢复安全上下文。参数位于 r0 和 r1 中。r0 = xSecureContext，r1 = pxCurrentTCB。 */
            "    pop {r1-r3, lr}                              \n"
            "                                                 \n"
            " restore_ns_context:                             \n"
            "    mov r0, lr                                   \n" /* r0 = LR（EXC_RETURN，异常返回值）。 */
            "    lsls r0, r0, #25                             \n" /* r0 = r0 << 25。若使用安全栈保存栈帧，EXC_RETURN 的 bit[6] 为 1；若使用非安全栈，则为 0。 */
            "    bmi restore_context_done                     \n" /* r0 < 0 ==> EXC_RETURN 的 bit[6] 为 1 ==> 使用安全栈保存了栈帧。 */
            "                                                 \n"
            " restore_general_regs:                           \n"
            "    ldmia r2!, {r4-r11}                          \n" /* 恢复不会自动恢复的寄存器。 */
        #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            "   tst lr, #0x10                                 \n" /* 测试 LR 的 bit[4]。若使用扩展栈帧，EXC_RETURN 的 bit[4] 为 0。 */
            "   it eq                                         \n"
            "   vldmiaeq r2!, {s16-s31}                       \n" /* 恢复不会自动恢复的额外 FP 上下文寄存器。 */
        #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */
            "                                                 \n"
            " restore_context_done:                           \n"
            "    msr psp, r2                                  \n" /* 记住任务的新栈顶。 */
            "    bx lr                                        \n"
            ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
        );
    }

#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            ".syntax unified                \n"
            ".extern vPortSVCHandler_C      \n"
            ".extern vSystemCallEnter       \n"
            ".extern vSystemCallExit        \n"
            "                               \n"
            "tst lr, #4                     \n"
            "ite eq                         \n"
            "mrseq r0, msp                  \n"
            "mrsne r0, psp                  \n"
            "                               \n"
            "ldr r1, [r0, #24]              \n"
            "ldrb r2, [r1, #-2]             \n"
            "cmp r2, %0                     \n"
            "blt syscall_enter              \n"
            "cmp r2, %1                     \n"
            "beq syscall_exit               \n"
            "b vPortSVCHandler_C            \n"
            "                               \n"
            "syscall_enter:                 \n"
            "    mov r1, lr                 \n"
            "    b vSystemCallEnter         \n"
            "                               \n"
            "syscall_exit:                  \n"
            "    mov r1, lr                 \n"
            "    b vSystemCallExit          \n"
            "                               \n"
            : /* 无输出。 */
            : "i" ( NUM_SYSTEM_CALLS ), "i" ( portSVC_SYSTEM_CALL_EXIT )
            : "r0", "r1", "r2", "memory"
        );
    }

#else /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

    void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            "   .syntax unified                                 \n"
            "                                                   \n"
            "   tst lr, #4                                      \n"
            "   ite eq                                          \n"
            "   mrseq r0, msp                                   \n"
            "   mrsne r0, psp                                   \n"
            "   ldr r1, =vPortSVCHandler_C                      \n"
            "   bx r1                                           \n"
        );
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */
/*-----------------------------------------------------------*/

void vPortAllocateSecureContext( uint32_t ulSecureStackSize ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   svc %0                                          \n" /* 安全上下文在系统调用（supervisor call）中分配。 */
        "   bx lr                                           \n" /* 返回。 */
        ::"i" ( portSVC_ALLOCATE_SECURE_CONTEXT ) : "memory"
    );
}
/*-----------------------------------------------------------*/

void vPortFreeSecureContext( uint32_t * pulTCB ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   ldr r2, [r0]                                    \n" /* TCB 的第一项是所保存上下文的位置。 */
        "   ldr r1, [r2, %0]                                \n" /* 从任务上下文中读取 xSecureContext。 */
        "   cmp r1, #0                                      \n" /* 若任务的 xSecureContext 不为 NULL，则触发 SVC。 */
        "   it ne                                           \n"
        "   svcne %1                                        \n" /* 安全上下文在系统调用（supervisor call）中释放。 */
        "   bx lr                                           \n" /* 返回。 */
        ::"i" ( SECURE_CONTEXT_OFFSET ), "i" ( portSVC_FREE_SECURE_CONTEXT ) : "memory"
    );
}
/*-----------------------------------------------------------*/
