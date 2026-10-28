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

    .extern ulTopOfSystemStack
    .extern ulInterruptNesting

/*-----------------------------------------------------------*/

.macro portFREERTOS_INTERRUPT_ENTRY

    /* 保存通用寄存器。 */
    pusha

    /*
     * 若 ulInterruptNesting 为零，则需要保存任务的其余上下文，
     * 并且可能需要进行栈切换。
     */
    movl    ulInterruptNesting, %eax
    test    %eax, %eax
    jne     2f

    /* 中断未嵌套，因此保存任务其余上下文。 */
    .if configSUPPORT_FPU == 1

        /*
         * 若任务已分配用于保存 FPU 上下文的缓冲区，则现在保存 FPU 上下文。
         */
        movl    pucPortTaskFPUContextBuffer, %eax
        test    %eax, %eax
        je      1f
        fnsave  ( %eax ) /* 将 FLOP 上下文保存到 ucTempFPUBuffer 数组。 */
        fwait

        1:
        /* 保存 FPU 上下文的地址（如有）。 */
        push    pucPortTaskFPUContextBuffer

    .endif /* configSUPPORT_FPU（支持 FPU） */

    /* 查找 TCB。 */
    movl    pxCurrentTCB, %eax

    /* 栈位置是 TCB 中的第一项。 */
    movl    %esp, (%eax)

    /* 切换栈。 */
    movl    ulTopOfSystemStack, %esp
    movl    %esp, %ebp

    2:
    /* 递增嵌套计数。 */
    add     $1, ulInterruptNesting

.endm
/*-----------------------------------------------------------*/

.macro portINTERRUPT_EPILOGUE

    cli
    sub     $1, ulInterruptNesting

    /* 若嵌套已回退到零。 */
    movl    ulInterruptNesting, %eax
    test    %eax, %eax
    jne     2f

    /* 若请求了让出，则现在选择新的 TCB。 */
    movl    ulPortYieldPending, %eax
    test    %eax, %eax
    je      1f
    movl    $0, ulPortYieldPending
    call    vTaskSwitchContext

    1:
    /* 栈位置是 TCB 中的第一项。 */
    movl    pxCurrentTCB, %eax
    movl    (%eax), %esp

    .if configSUPPORT_FPU == 1

        /* 恢复任务 FPU 上下文缓冲区的地址。 */
        pop     pucPortTaskFPUContextBuffer

        /*
         * 若任务已分配用于保存其 FPU 上下文的缓冲区，则现在恢复该上下文。
         */
        movl    pucPortTaskFPUContextBuffer, %eax
        test    %eax, %eax
        je      1f
        frstor  ( %eax )
        1:
    .endif

    2:
    popa

.endm
/*-----------------------------------------------------------*/

.macro portFREERTOS_INTERRUPT_EXIT

    portINTERRUPT_EPILOGUE
    /* EOI（中断结束）。 */
    movl    $0x00, (0xFEE000B0)
    iret

.endm
