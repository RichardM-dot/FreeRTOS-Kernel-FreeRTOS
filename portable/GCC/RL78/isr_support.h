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

/* 调度器使用的变量 */
.extern _pxCurrentTCB
   .extern _usCriticalNesting

/*
 * portSAVE_CONTEXT 宏
 * 将通用寄存器、CS 和 ES（仅远存储器模式）寄存器、usCriticalNesting 值
 * 以及活动任务的栈指针保存到任务栈中
 */
   .macro portSAVE_CONTEXT

SEL RB0

/* 将 AX 寄存器保存到栈。 */
PUSH AX
PUSH HL
/* 保存 CS 寄存器。 */
MOV A, CS
XCH A, X
/* 保存 ES 寄存器。 */
MOV A, ES
PUSH AX
/* 保存寄存器组 0 中其余的通用寄存器。 */
PUSH DE
PUSH BC
/* 保存其他寄存器组 - 仅在 GCC 移植层中需要。 */
SEL RB1
PUSH AX
PUSH BC
PUSH DE
PUSH HL
SEL RB2
PUSH AX
PUSH BC
PUSH DE
PUSH HL
/* 寄存器组 3 中的寄存器仅供 ISR 使用，因此无需保存。 */
SEL RB0
/* 保存 usCriticalNesting 值。 */
MOVW AX, !_usCriticalNesting
PUSH AX
/* 保存栈指针。 */
MOVW AX, !_pxCurrentTCB
MOVW HL, AX
MOVW AX, SP
MOVW[ HL ], AX
/* 切换栈指针。 */
movw sp, # _stack /* 设置栈指针 */

   .endm


/*
 * portRESTORE_CONTEXT 宏
 * 恢复任务栈指针，然后使用该指针从任务栈中恢复所选任务的 usCriticalNesting、
 * 通用寄存器以及 CS 和 ES（仅远存储器模式）
 */
   .macro portRESTORE_CONTEXT MACRO
SEL RB0
/* 恢复栈指针。 */
MOVW AX, !_pxCurrentTCB
MOVW HL, AX
MOVW AX, [ HL ]
MOVW SP, AX
/* 恢复 usCriticalNesting 值。 */
POP AX
MOVW    !_usCriticalNesting, AX

/*
 * 恢复备用寄存器组 - 仅在 GCC 移植层中需要。寄存器组 3 专供中断使用，
 * 因此不保存也不恢复。
 */
SEL RB2
POP HL
POP DE
POP BC
POP AX
SEL RB1
POP HL
POP DE
POP BC
POP AX
SEL RB0
/* 恢复必要的通用寄存器。 */
POP BC
POP DE
/* 恢复 ES 寄存器。 */
POP AX
MOV ES, A
/* 恢复 CS 寄存器。 */
XCH A, X
MOV CS, A
/* 恢复通用寄存器 HL。 */
POP HL
/* 恢复 AX。 */
POP AX

   .endm
