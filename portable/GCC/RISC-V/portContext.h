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

#ifndef PORTCONTEXT_H
#define PORTCONTEXT_H

#ifndef configENABLE_FPU
    #define configENABLE_FPU 0
#endif

#ifndef configENABLE_VPU
    #define configENABLE_VPU 0
#endif

#if __riscv_xlen == 64
    #define portWORD_SIZE    8
    #define store_x          sd
    #define load_x           ld
#elif __riscv_xlen == 32
    #define store_x          sw
    #define load_x           lw
    #define portWORD_SIZE    4
#else
    #error Assembler did not define __riscv_xlen
#endif

#include "freertos_risc_v_chip_specific_extensions.h"

/*
 * 默认只保存标准核心寄存器。任何附加寄存器都必须由
 * portasmSAVE_ADDITIONAL_REGISTERS 和 portasmRESTORE_ADDITIONAL_REGISTERS
 * 宏保存——这两个宏可在芯片专用的 freertos_risc_v_chip_specific_extensions.h
 * 版本中定义。参见 portASM.S 文件顶部的说明。
 */
#ifdef __riscv_32e
    #define portCONTEXT_SIZE               ( 15 * portWORD_SIZE )
    #define portCRITICAL_NESTING_OFFSET    14
#else
    #define portCONTEXT_SIZE               ( 31 * portWORD_SIZE )
    #define portCRITICAL_NESTING_OFFSET    30
#endif

#if ( configENABLE_FPU == 1 )
    /*
     * mstatus 中的位 [14:13] 编码 FPU 状态，取值如下：
     * 1. 值：0，含义：关闭。
     * 2. 值：1，含义：初始。
     * 3. 值：2，含义：干净。
     * 4. 值：3，含义：脏。
     */
    #define MSTATUS_FS_MASK                 0x6000
    #define MSTATUS_FS_INITIAL              0x2000
    #define MSTATUS_FS_CLEAN                0x4000
    #define MSTATUS_FS_DIRTY                0x6000
    #define MSTATUS_FS_OFFSET               13

    #ifdef __riscv_fdiv
        #if __riscv_flen == 32
            #define load_f                  flw
            #define store_f                 fsw
        #elif __riscv_flen == 64
            #define load_f                  fld
            #define store_f                 fsd
        #else
            #error Assembler did not define __riscv_flen
        #endif

        #define portFPU_REG_SIZE                ( __riscv_flen / 8 )
        #define portFPU_REG_COUNT               33 /* 32 个浮点寄存器加 1 个 CSR。 */
        #define portFPU_REG_OFFSET( regIndex )  ( ( 2 * portWORD_SIZE ) + ( regIndex * portFPU_REG_SIZE ) )
        #define portFPU_CONTEXT_SIZE            ( portFPU_REG_SIZE * portFPU_REG_COUNT )
    #else
        #error configENABLE_FPU must not be set to 1 if the hardware does not have FPU
    #endif
#endif

#if ( configENABLE_VPU == 1 )
    /*
     * mstatus 中的位 [10:9] 编码 VPU 状态，取值如下：
     * 1. 值：0，含义：关闭。
     * 2. 值：1，含义：初始。
     * 3. 值：2，含义：干净。
     * 4. 值：3，含义：脏。
     */
    #define MSTATUS_VS_MASK                 0x600
    #define MSTATUS_VS_INITIAL              0x200
    #define MSTATUS_VS_CLEAN                0x400
    #define MSTATUS_VS_DIRTY                0x600
    #define MSTATUS_VS_OFFSET               9

    #ifndef __riscv_vector
        #error configENABLE_VPU must not be set to 1 if the hardware does not have VPU
    #endif
#endif
/*-----------------------------------------------------------*/

.extern pxCurrentTCB
.extern xISRStackTop
.extern xCriticalNesting
.extern pxCriticalNesting
/*-----------------------------------------------------------*/

    .macro portcontexSAVE_FPU_CONTEXT
addi sp, sp, -( portFPU_CONTEXT_SIZE )
/* 保存 FPU 寄存器。 */
store_f f0,  portFPU_REG_OFFSET( 0  )( sp )
store_f f1,  portFPU_REG_OFFSET( 1  )( sp )
store_f f2,  portFPU_REG_OFFSET( 2  )( sp )
store_f f3,  portFPU_REG_OFFSET( 3  )( sp )
store_f f4,  portFPU_REG_OFFSET( 4  )( sp )
store_f f5,  portFPU_REG_OFFSET( 5  )( sp )
store_f f6,  portFPU_REG_OFFSET( 6  )( sp )
store_f f7,  portFPU_REG_OFFSET( 7  )( sp )
store_f f8,  portFPU_REG_OFFSET( 8  )( sp )
store_f f9,  portFPU_REG_OFFSET( 9  )( sp )
store_f f10, portFPU_REG_OFFSET( 10 )( sp )
store_f f11, portFPU_REG_OFFSET( 11 )( sp )
store_f f12, portFPU_REG_OFFSET( 12 )( sp )
store_f f13, portFPU_REG_OFFSET( 13 )( sp )
store_f f14, portFPU_REG_OFFSET( 14 )( sp )
store_f f15, portFPU_REG_OFFSET( 15 )( sp )
store_f f16, portFPU_REG_OFFSET( 16 )( sp )
store_f f17, portFPU_REG_OFFSET( 17 )( sp )
store_f f18, portFPU_REG_OFFSET( 18 )( sp )
store_f f19, portFPU_REG_OFFSET( 19 )( sp )
store_f f20, portFPU_REG_OFFSET( 20 )( sp )
store_f f21, portFPU_REG_OFFSET( 21 )( sp )
store_f f22, portFPU_REG_OFFSET( 22 )( sp )
store_f f23, portFPU_REG_OFFSET( 23 )( sp )
store_f f24, portFPU_REG_OFFSET( 24 )( sp )
store_f f25, portFPU_REG_OFFSET( 25 )( sp )
store_f f26, portFPU_REG_OFFSET( 26 )( sp )
store_f f27, portFPU_REG_OFFSET( 27 )( sp )
store_f f28, portFPU_REG_OFFSET( 28 )( sp )
store_f f29, portFPU_REG_OFFSET( 29 )( sp )
store_f f30, portFPU_REG_OFFSET( 30 )( sp )
store_f f31, portFPU_REG_OFFSET( 31 )( sp )
csrr t0, fcsr
store_x t0,  portFPU_REG_OFFSET( 32 )( sp )
    .endm
/*-----------------------------------------------------------*/

    .macro portcontextRESTORE_FPU_CONTEXT
/* 恢复 FPU 寄存器。 */
load_f f0,  portFPU_REG_OFFSET( 0  )( sp )
load_f f1,  portFPU_REG_OFFSET( 1  )( sp )
load_f f2,  portFPU_REG_OFFSET( 2  )( sp )
load_f f3,  portFPU_REG_OFFSET( 3  )( sp )
load_f f4,  portFPU_REG_OFFSET( 4  )( sp )
load_f f5,  portFPU_REG_OFFSET( 5  )( sp )
load_f f6,  portFPU_REG_OFFSET( 6  )( sp )
load_f f7,  portFPU_REG_OFFSET( 7  )( sp )
load_f f8,  portFPU_REG_OFFSET( 8  )( sp )
load_f f9,  portFPU_REG_OFFSET( 9  )( sp )
load_f f10, portFPU_REG_OFFSET( 10 )( sp )
load_f f11, portFPU_REG_OFFSET( 11 )( sp )
load_f f12, portFPU_REG_OFFSET( 12 )( sp )
load_f f13, portFPU_REG_OFFSET( 13 )( sp )
load_f f14, portFPU_REG_OFFSET( 14 )( sp )
load_f f15, portFPU_REG_OFFSET( 15 )( sp )
load_f f16, portFPU_REG_OFFSET( 16 )( sp )
load_f f17, portFPU_REG_OFFSET( 17 )( sp )
load_f f18, portFPU_REG_OFFSET( 18 )( sp )
load_f f19, portFPU_REG_OFFSET( 19 )( sp )
load_f f20, portFPU_REG_OFFSET( 20 )( sp )
load_f f21, portFPU_REG_OFFSET( 21 )( sp )
load_f f22, portFPU_REG_OFFSET( 22 )( sp )
load_f f23, portFPU_REG_OFFSET( 23 )( sp )
load_f f24, portFPU_REG_OFFSET( 24 )( sp )
load_f f25, portFPU_REG_OFFSET( 25 )( sp )
load_f f26, portFPU_REG_OFFSET( 26 )( sp )
load_f f27, portFPU_REG_OFFSET( 27 )( sp )
load_f f28, portFPU_REG_OFFSET( 28 )( sp )
load_f f29, portFPU_REG_OFFSET( 29 )( sp )
load_f f30, portFPU_REG_OFFSET( 30 )( sp )
load_f f31, portFPU_REG_OFFSET( 31 )( sp )
load_x t0,  portFPU_REG_OFFSET( 32 )( sp )
csrw fcsr, t0
addi sp, sp, ( portFPU_CONTEXT_SIZE )
    .endm
/*-----------------------------------------------------------*/

    .macro portcontexSAVE_VPU_CONTEXT
/* 取消为 mstatus 和 epc 预留的空间。 */
add sp, sp, ( 2 * portWORD_SIZE )

csrr t0, vlenb /* t0 = vlenb。vlenb 是每个向量寄存器的字节长度。 */
slli t0, t0, 3 /* t0 = vlenb * 8。此时 t0 保存存储 8 个向量寄存器所需的空间。 */
neg  t0, t0

/* 按 8 个一组保存向量寄存器。 */
add     sp, sp, t0
vs8r.v  v24, (sp)   /* 保存 v24-v31。 */
add     sp, sp, t0
vs8r.v  v16, (sp)   /* 保存 v16-v23。 */
add     sp, sp, t0
vs8r.v  v8, (sp)    /* 保存 v8-v15。 */
add     sp, sp, t0
vs8r.v  v0, (sp)    /* 保存 v0-v7。 */

/* 保存 VPU 的 CSR。 */
addi    sp, sp, -( 4 * portWORD_SIZE )
csrr    t0, vstart
store_x t0, 0 * portWORD_SIZE( sp )
csrr    t0, vcsr
store_x t0, 1 * portWORD_SIZE( sp )
csrr    t0, vl
store_x t0, 2 * portWORD_SIZE( sp )
csrr    t0, vtype
store_x t0, 3 * portWORD_SIZE( sp )

/* 重新为 mstatus 和 epc 预留空间。 */
add sp, sp, -( 2 * portWORD_SIZE )
    .endm
/*-----------------------------------------------------------*/

    .macro portcontextRESTORE_VPU_CONTEXT
/* 取消为 mstatus 和 epc 预留的空间。 */
add sp, sp, ( 2 * portWORD_SIZE )

/* 恢复 VPU 的 CSR。 */
load_x  t0, 0  * portWORD_SIZE( sp )
csrw    vstart, t0
load_x  t0, 1 * portWORD_SIZE( sp )
csrw    vcsr, t0
load_x  t0, 2 * portWORD_SIZE( sp )
load_x  t1, 3 * portWORD_SIZE( sp )
vsetvl  x0, t0, t1 /* vlen 和 vtype 只能通过 vset*vl* 指令更新。 */
addi    sp, sp, ( 4 * portWORD_SIZE )

csrr t0, vlenb /* t0 = vlenb。vlenb 是每个向量寄存器的字节长度。 */
slli t0, t0, 3 /* t0 = vlenb * 8。此时 t0 保存存储 8 个向量寄存器所需的空间。 */

/* 恢复向量寄存器。 */
vl8r.v  v0, (sp)    /* 恢复 v0-v7。 */
add     sp, sp, t0
vl8r.v  v8, (sp)    /* 恢复 v8-v15。 */
add     sp, sp, t0
vl8r.v  v16, (sp)   /* 恢复 v16-v23。 */
add     sp, sp, t0
vl8r.v  v24, (sp)   /* 恢复 v23-v31。 */
add     sp, sp, t0

/* 重新为 mstatus 和 epc 预留空间。 */
add sp, sp, -( 2 * portWORD_SIZE )
    .endm
/*-----------------------------------------------------------*/

   .macro portcontextSAVE_CONTEXT_INTERNAL
addi sp, sp, -portCONTEXT_SIZE
store_x x1,  2  * portWORD_SIZE( sp )
store_x x5,  3  * portWORD_SIZE( sp )
store_x x6,  4  * portWORD_SIZE( sp )
store_x x7,  5  * portWORD_SIZE( sp )
store_x x8,  6  * portWORD_SIZE( sp )
store_x x9,  7  * portWORD_SIZE( sp )
store_x x10, 8  * portWORD_SIZE( sp )
store_x x11, 9  * portWORD_SIZE( sp )
store_x x12, 10 * portWORD_SIZE( sp )
store_x x13, 11 * portWORD_SIZE( sp )
store_x x14, 12 * portWORD_SIZE( sp )
store_x x15, 13 * portWORD_SIZE( sp )
#ifndef __riscv_32e
    store_x x16, 14 * portWORD_SIZE( sp )
    store_x x17, 15 * portWORD_SIZE( sp )
    store_x x18, 16 * portWORD_SIZE( sp )
    store_x x19, 17 * portWORD_SIZE( sp )
    store_x x20, 18 * portWORD_SIZE( sp )
    store_x x21, 19 * portWORD_SIZE( sp )
    store_x x22, 20 * portWORD_SIZE( sp )
    store_x x23, 21 * portWORD_SIZE( sp )
    store_x x24, 22 * portWORD_SIZE( sp )
    store_x x25, 23 * portWORD_SIZE( sp )
    store_x x26, 24 * portWORD_SIZE( sp )
    store_x x27, 25 * portWORD_SIZE( sp )
    store_x x28, 26 * portWORD_SIZE( sp )
    store_x x29, 27 * portWORD_SIZE( sp )
    store_x x30, 28 * portWORD_SIZE( sp )
    store_x x31, 29 * portWORD_SIZE( sp )
#endif /* ifndef __riscv_32e 条件结束 */

load_x t0, xCriticalNesting                                   /* 将 xCriticalNesting 的值加载到 t0。 */
store_x t0, portCRITICAL_NESTING_OFFSET * portWORD_SIZE( sp ) /* 将临界区嵌套值保存到栈中。 */

#if( configENABLE_FPU == 1 )
    csrr t0, mstatus
    srl t1, t0, MSTATUS_FS_OFFSET
    andi t1, t1, 3
    addi t2, x0, 3
    bne t1, t2, 1f /* 若 FPU 状态不是脏，则不保存 FPU 寄存器。 */

    portcontexSAVE_FPU_CONTEXT
1:
#endif

#if( configENABLE_VPU == 1 )
    csrr t0, mstatus
    srl t1, t0, MSTATUS_VS_OFFSET
    andi t1, t1, 3
    addi t2, x0, 3
    bne t1, t2, 2f /* 若 VPU 状态不是脏，则不保存 VPU 寄存器。 */

    portcontexSAVE_VPU_CONTEXT
2:
#endif

csrr t0, mstatus
store_x t0, 1 * portWORD_SIZE( sp )

portasmSAVE_ADDITIONAL_REGISTERS /* 由 freertos_risc_v_chip_specific_extensions.h 定义，用于保存 RISC-V 实现特有的所有寄存器。 */

#if( configENABLE_FPU == 1 )
    /* 若 FPU 此前为脏且已保存 FPU 寄存器，则将 FPU 标记为干净。 */
    srl t1, t0, MSTATUS_FS_OFFSET
    andi t1, t1, 3
    addi t2, x0, 3
    bne t1, t2, 3f

    li t1, ~MSTATUS_FS_MASK
    and t0, t0, t1
    li t1, MSTATUS_FS_CLEAN
    or t0, t0, t1
    csrw mstatus, t0
3:
#endif

#if( configENABLE_VPU == 1 )
    /* 若 VPU 此前为脏且已保存 VPU 寄存器，则将 VPU 标记为干净。 */
    srl t1, t0, MSTATUS_VS_OFFSET
    andi t1, t1, 3
    addi t2, x0, 3
    bne t1, t2, 4f

    li t1, ~MSTATUS_VS_MASK
    and t0, t0, t1
    li t1, MSTATUS_VS_CLEAN
    or t0, t0, t1
    csrw mstatus, t0
4:
#endif

load_x t0, pxCurrentTCB          /* 加载 pxCurrentTCB。 */
store_x sp, 0 ( t0 )             /* 将 sp 写入 TCB 的第一个成员。 */

   .endm
/*-----------------------------------------------------------*/

   .macro portcontextSAVE_EXCEPTION_CONTEXT
portcontextSAVE_CONTEXT_INTERNAL
csrr a0, mcause
csrr a1, mepc
addi a1, a1, 4          /* 同步异常，因此将异常返回地址更新为产生异常的指令之后的下一条指令。 */
store_x a1, 0 ( sp )    /* 保存更新后的异常返回地址。 */
load_x sp, xISRStackTop /* 切换到 ISR 栈。 */
   .endm
/*-----------------------------------------------------------*/

   .macro portcontextSAVE_INTERRUPT_CONTEXT
portcontextSAVE_CONTEXT_INTERNAL
csrr a0, mcause
csrr a1, mepc
store_x a1, 0 ( sp )    /* 异步中断，因此保存未修改的异常返回地址。 */
load_x sp, xISRStackTop /* 切换到 ISR 栈。 */
   .endm
/*-----------------------------------------------------------*/

   .macro portcontextRESTORE_CONTEXT
load_x t1, pxCurrentTCB /* 加载 pxCurrentTCB。 */
load_x sp, 0 ( t1 )     /* 从 TCB 的第一个成员读取 sp。 */

/* 将下一条要执行的任务指令地址加载到 mepc。 */
load_x t0, 0 ( sp )
csrw mepc, t0

/* 由 freertos_risc_v_chip_specific_extensions.h 定义，用于恢复 RISC-V 实现特有的所有寄存器。 */
portasmRESTORE_ADDITIONAL_REGISTERS

/*
 * 恢复 mstatus 寄存器。此处必须使用 t3（而不是 t0），因为 t3 不会被
 * portcontextRESTORE_VPU_CONTEXT 和 portcontextRESTORE_FPU_CONTEXT 破坏。
 */
load_x t3, 1 * portWORD_SIZE( sp )
csrw mstatus, t3

#if( configENABLE_VPU == 1 )
    srl t1, t3, MSTATUS_VS_OFFSET
    andi t1, t1, 3
    addi t2, x0, 3
    bne t1, t2, 5f /* 若 VPU 状态不是脏，则不恢复 VPU 寄存器。 */

    portcontextRESTORE_VPU_CONTEXT
5:
#endif /* ifdef portasmSTORE_VPU_CONTEXT 条件结束 */

#if( configENABLE_FPU == 1 )
    srl t1, t3, MSTATUS_FS_OFFSET
    andi t1, t1, 3
    addi t2, x0, 3
    bne t1, t2, 6f /* 若 FPU 状态不是脏，则不恢复 FPU 寄存器。 */

    portcontextRESTORE_FPU_CONTEXT
6:
#endif /* ifdef portasmSTORE_FPU_CONTEXT 条件结束 */

load_x t0, portCRITICAL_NESTING_OFFSET * portWORD_SIZE( sp ) /* 从任务栈中获取该任务的 xCriticalNesting 值。 */
load_x t1, pxCriticalNesting                                 /* 将 xCriticalNesting 的地址加载到 t1。 */
store_x t0, 0 ( t1 )                                         /* 恢复该任务的临界区嵌套值。 */

load_x x1,  2  * portWORD_SIZE( sp )
load_x x5,  3  * portWORD_SIZE( sp )
load_x x6,  4  * portWORD_SIZE( sp )
load_x x7,  5  * portWORD_SIZE( sp )
load_x x8,  6  * portWORD_SIZE( sp )
load_x x9,  7  * portWORD_SIZE( sp )
load_x x10, 8  * portWORD_SIZE( sp )
load_x x11, 9  * portWORD_SIZE( sp )
load_x x12, 10 * portWORD_SIZE( sp )
load_x x13, 11 * portWORD_SIZE( sp )
load_x x14, 12 * portWORD_SIZE( sp )
load_x x15, 13 * portWORD_SIZE( sp )
#ifndef __riscv_32e
    load_x x16, 14 * portWORD_SIZE( sp )
    load_x x17, 15 * portWORD_SIZE( sp )
    load_x x18, 16 * portWORD_SIZE( sp )
    load_x x19, 17 * portWORD_SIZE( sp )
    load_x x20, 18 * portWORD_SIZE( sp )
    load_x x21, 19 * portWORD_SIZE( sp )
    load_x x22, 20 * portWORD_SIZE( sp )
    load_x x23, 21 * portWORD_SIZE( sp )
    load_x x24, 22 * portWORD_SIZE( sp )
    load_x x25, 23 * portWORD_SIZE( sp )
    load_x x26, 24 * portWORD_SIZE( sp )
    load_x x27, 25 * portWORD_SIZE( sp )
    load_x x28, 26 * portWORD_SIZE( sp )
    load_x x29, 27 * portWORD_SIZE( sp )
    load_x x30, 28 * portWORD_SIZE( sp )
    load_x x31, 29 * portWORD_SIZE( sp )
#endif /* ifndef __riscv_32e 条件结束 */
addi sp, sp, portCONTEXT_SIZE

mret
   .endm
/*-----------------------------------------------------------*/

#endif /* 头文件保护：PORTCONTEXT_H */
