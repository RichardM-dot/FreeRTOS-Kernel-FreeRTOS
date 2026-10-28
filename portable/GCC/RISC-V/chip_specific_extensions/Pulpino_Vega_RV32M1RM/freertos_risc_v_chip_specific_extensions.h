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

/*
 * 本 freertos_risc_v_chip_specific_extensions.h 适用于 Pulpino Ri5cy 设备，
 * 已在 Vega 板 RV32M1RM 上开发和测试。
 */

#ifndef __FREERTOS_RISC_V_EXTENSIONS_H__
#define __FREERTOS_RISC_V_EXTENSIONS_H__

#define portasmHAS_MTIME                  0

/* 定义 Pulpino RI5KY 上附加寄存器的常量。 */
#define lpstart0                          0x7b0
#define lpend0                            0x7b1
#define lpcount0                          0x7b2
#define lpstart1                          0x7b4
#define lpend1                            0x7b5
#define lpcount1                          0x7b6

/* 按照上面的 #define，需要保存和恢复六个附加寄存器。 */
#define portasmADDITIONAL_CONTEXT_SIZE    6 /* 在 32 位内核上必须为偶数。 */

/* 保存 Pulpino 上的附加寄存器。 */
.macro portasmSAVE_ADDITIONAL_REGISTERS
addi sp, sp, -( portasmADDITIONAL_CONTEXT_SIZE * portWORD_SIZE ) /* 为附加寄存器腾出空间。 */
csrr t0, lpstart0                                                /* 将附加寄存器加载到可访问的临时寄存器中。 */
csrr t1, lpend0
csrr t2, lpcount0
csrr t3, lpstart1
csrr t4, lpend1
csrr t5, lpcount1
sw t0, 1 * portWORD_SIZE( sp )
sw t1, 2 * portWORD_SIZE( sp )
sw t2, 3 * portWORD_SIZE( sp )
sw t3, 4 * portWORD_SIZE( sp )
sw t4, 5 * portWORD_SIZE( sp )
sw t5, 6 * portWORD_SIZE( sp )
   .endm

/* 恢复 Pulpino 上的附加寄存器。 */
   .macro portasmRESTORE_ADDITIONAL_REGISTERS
lw t0, 1 * portWORD_SIZE( sp ) /* 将附加寄存器加载到可访问的临时寄存器中。 */
lw t1, 2 * portWORD_SIZE( sp )
lw t2, 3 * portWORD_SIZE( sp )
lw t3, 4 * portWORD_SIZE( sp )
lw t4, 5 * portWORD_SIZE( sp )
lw t5, 6 * portWORD_SIZE( sp )
csrw lpstart0, t0
csrw lpend0, t1
csrw lpcount0, t2
csrw lpstart1, t3
csrw lpend1, t4
csrw lpcount1, t5
addi sp, sp, ( portasmADDITIONAL_CONTEXT_SIZE * portWORD_SIZE ) /* 释放为附加寄存器添加的空间。 */
   .endm

#endif /* 头文件保护：__FREERTOS_RISC_V_EXTENSIONS_H__ */
