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
 * FreeRTOS 内核的 RISC-V 移植层分为两部分：所有当前支持的 RISC-V 芯片
 * （RISC-V ISA 实现）共用的代码，以及针对特定 RISC-V 芯片定制的代码：
 *
 * + FreeRTOS\Source\portable\GCC\RISC-V\portASM.S 包含所有当前支持的
 *   RISC-V 芯片共用的代码。所有 RISC-V 目标芯片都构建同一个 portASM.S
 *   文件，因此该文件只有一个。
 *
 * + 名为 freertos_risc_v_chip_specific_extensions.h 的头文件包含针对特定
 *   RISC-V 芯片定制 FreeRTOS 内核 RISC-V 移植层的代码。由于存在多种
 *   RISC-V 芯片实现，因此也有多个 freertos_risc_v_chip_specific_extensions.h
 *   文件。
 *
 * !!!注意!!!
 * 务必包含所用芯片对应的正确 freertos_risc_v_chip_specific_extensions.h
 * 头文件。该文件通过汇编器（而非编译器！）的包含路径选择。例如，若所用芯片
 * 包含核心本地中断器（CLINT）且不包含任何芯片专用寄存器扩展，则将以下路径
 * 添加到汇编器的包含路径：
 * FreeRTOS\Source\portable\GCC\RISC-V\chip_specific_extensions\RV32I_CLINT_no_extensions
 */


#ifndef __FREERTOS_RISC_V_EXTENSIONS_H__
#define __FREERTOS_RISC_V_EXTENSIONS_H__

#define portasmHAS_SIFIVE_CLINT           1
#define portasmHAS_MTIME                  1
#define portasmADDITIONAL_CONTEXT_SIZE    0

.macro portasmSAVE_ADDITIONAL_REGISTERS
/* 没有需要保存的附加寄存器，因此该宏为空操作。 */
   .endm

   .macro portasmRESTORE_ADDITIONAL_REGISTERS
/* 没有需要恢复的附加寄存器，因此该宏为空操作。 */
   .endm

#endif /* 头文件保护：__FREERTOS_RISC_V_EXTENSIONS_H__ */
