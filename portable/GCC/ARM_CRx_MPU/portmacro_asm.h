/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef PORTMACRO_ASM_H
#define PORTMACRO_ASM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOSConfig.h"

#ifndef configTOTAL_MPU_REGIONS
    #error "Set configTOTAL_MPU_REGIONS to the number of MPU regions in FreeRTOSConfig.h"
#elif( configTOTAL_MPU_REGIONS == 12 )
    #define portMPU_TOTAL_REGIONS ( 12UL )
#elif( configTOTAL_MPU_REGIONS == 16 )
    #define portMPU_TOTAL_REGIONS ( 16UL )
#else
    #error "Set configTOTAL_MPU_REGIONS to the number of MPU regions in FreeRTOSConfig.h"
#endif /* configTOTAL_MPU_REGIONS 条件编译结束 */

/*
 * 应用可通过将 configENABLE_FPU 设为 0 来禁用浮点单元（FPU）支持。
 * TCB 中保存的浮点上下文包含 32 个浮点寄存器（D0-D31）和 FPSCR 寄存器。
 * 因此禁用 FPU 可使每个任务的 RAM 占用减少
 * ( 32 + 1 ) * 4 = 132 字节。
 *
 * 禁用前请务必注意：某些标准库 API 会尝试使用浮点寄存器进行优化。
 * 若禁用 FPU 支持，使用此类 API 可能导致内存损坏。
 */
#ifndef configENABLE_FPU
    #define configENABLE_FPU    1
#endif /* configENABLE_FPU 条件编译结束 */

#define portENABLE_FPU configENABLE_FPU

/*
 * 在 ArmV7-R 架构中，处理器的运行模式由当前程序状态寄存器（CPSR）
 * 的模式位 [4:0] 设置。唯一的非特权模式是用户模式。
 *
 * 有关处理器模式的更多信息，请参见：
 * https://developer.arm.com/documentation/ddi0406/cb/System-Level-Architecture/The-System-Level-Programmers--Model/ARM-processor-modes-and-ARM-core-registers/ARM-processor-modes?lang=en
 */

/**
 * @brief 各种处理器模式对应的 CPSR 位。
 *
 * @ingroup Port Privilege
 */
#define USER_MODE   0x10U
#define FIQ_MODE    0x11U
#define IRQ_MODE    0x12U
#define SVC_MODE    0x13U
#define MON_MODE    0x16U
#define ABT_MODE    0x17U
#define HYP_MODE    0x1AU
#define UND_MODE    0x1BU
#define SYS_MODE    0x1FU

/**
 * @brief 用于标记某个 FreeRTOS 任务具有特权的标志。
 *
 * @ingroup Port Privilege
 */
#define portTASK_IS_PRIVILEGED_FLAG   ( 1UL << 1UL )

/**
 * @brief 各种调度器操作使用的 SVC 编号。
 *
 * @ingroup Scheduler
 *
 * @note 这些值不得在 mpu_syscall_numbers.h 中使用。
 */
#define portSVC_YIELD                 0x0100U
#define portSVC_SYSTEM_CALL_EXIT      0x0104U

/**
 * @brief 操作 MPU 所需的宏。
 *
 * 有关 MPU 的更多信息，请参见 Arm 文档：
 * https://developer.arm.com/documentation/ddi0363/g/System-Control/Register-descriptions/c6--MPU-memory-region-programming-registers
 */

/*
 * MPU 子区域禁用设置。该信息编码在 MPU 区域大小与使能寄存器中。
 */
#define portMPU_SUBREGION_0_DISABLE   ( 0x1UL << 8UL )
#define portMPU_SUBREGION_1_DISABLE   ( 0x1UL << 9UL )
#define portMPU_SUBREGION_2_DISABLE   ( 0x1UL << 10UL )
#define portMPU_SUBREGION_3_DISABLE   ( 0x1UL << 11UL )
#define portMPU_SUBREGION_4_DISABLE   ( 0x1UL << 12UL )
#define portMPU_SUBREGION_5_DISABLE   ( 0x1UL << 13UL )
#define portMPU_SUBREGION_6_DISABLE   ( 0x1UL << 14UL )
#define portMPU_SUBREGION_7_DISABLE   ( 0x1UL << 15UL )

/* 默认 MPU 区域。 */
#define portFIRST_CONFIGURABLE_REGION ( 0 )
#define portLAST_CONFIGURABLE_REGION  ( portMPU_TOTAL_REGIONS - 5UL )
#define portSTACK_REGION              ( portMPU_TOTAL_REGIONS - 4UL )
#define portUNPRIVILEGED_FLASH_REGION ( portMPU_TOTAL_REGIONS - 3UL )
#define portPRIVILEGED_FLASH_REGION   ( portMPU_TOTAL_REGIONS - 2UL )
#define portPRIVILEGED_RAM_REGION     ( portMPU_TOTAL_REGIONS - 1UL )
#define portNUM_CONFIGURABLE_REGIONS \
    ( ( portLAST_CONFIGURABLE_REGION - portFIRST_CONFIGURABLE_REGION ) + 1UL )
/* 加一，为栈区域留出空间。 */
#define portTOTAL_NUM_REGIONS_IN_TCB        ( portNUM_CONFIGURABLE_REGIONS + 1UL )

/*
 * MPU 区域大小。该信息编码在 MPU 区域大小与使能寄存器中。
 */
#define portMPU_REGION_SIZE_32B             ( 0x04UL << 1UL )
#define portMPU_REGION_SIZE_64B             ( 0x05UL << 1UL )
#define portMPU_REGION_SIZE_128B            ( 0x06UL << 1UL )
#define portMPU_REGION_SIZE_256B            ( 0x07UL << 1UL )
#define portMPU_REGION_SIZE_512B            ( 0x08UL << 1UL )
#define portMPU_REGION_SIZE_1KB             ( 0x09UL << 1UL )
#define portMPU_REGION_SIZE_2KB             ( 0x0AUL << 1UL )
#define portMPU_REGION_SIZE_4KB             ( 0x0BUL << 1UL )
#define portMPU_REGION_SIZE_8KB             ( 0x0CUL << 1UL )
#define portMPU_REGION_SIZE_16KB            ( 0x0DUL << 1UL )
#define portMPU_REGION_SIZE_32KB            ( 0x0EUL << 1UL )
#define portMPU_REGION_SIZE_64KB            ( 0x0FUL << 1UL )
#define portMPU_REGION_SIZE_128KB           ( 0x10UL << 1UL )
#define portMPU_REGION_SIZE_256KB           ( 0x11UL << 1UL )
#define portMPU_REGION_SIZE_512KB           ( 0x12UL << 1UL )
#define portMPU_REGION_SIZE_1MB             ( 0x13UL << 1UL )
#define portMPU_REGION_SIZE_2MB             ( 0x14UL << 1UL )
#define portMPU_REGION_SIZE_4MB             ( 0x15UL << 1UL )
#define portMPU_REGION_SIZE_8MB             ( 0x16UL << 1UL )
#define portMPU_REGION_SIZE_16MB            ( 0x17UL << 1UL )
#define portMPU_REGION_SIZE_32MB            ( 0x18UL << 1UL )
#define portMPU_REGION_SIZE_64MB            ( 0x19UL << 1UL )
#define portMPU_REGION_SIZE_128MB           ( 0x1AUL << 1UL )
#define portMPU_REGION_SIZE_256MB           ( 0x1BUL << 1UL )
#define portMPU_REGION_SIZE_512MB           ( 0x1CUL << 1UL )
#define portMPU_REGION_SIZE_1GB             ( 0x1DUL << 1UL )
#define portMPU_REGION_SIZE_2GB             ( 0x1EUL << 1UL )
#define portMPU_REGION_SIZE_4GB             ( 0x1FUL << 1UL )

/*
 * MPU 存储器类型。该信息编码在 MPU 区域访问控制寄存器的
 * TEX、S、C 和 B 位中。
 */
#define portMPU_REGION_STRONGLY_ORDERED_SHAREABLE  ( 0x00UL ) /* TEX=000，S=NA，C=0，B=0（存储器类型编码）。 */
#define portMPU_REGION_DEVICE_SHAREABLE            ( 0x01UL ) /* TEX=000，S=NA，C=0，B=1（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OIWTNOWA_NONSHARED   ( 0x02UL ) /* TEX=000，S=0，C=1，B=0（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OIWTNOWA_SHARED      ( 0x06UL ) /* TEX=000，S=1，C=1，B=0（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OIWBNOWA_NONSHARED   ( 0x03UL ) /* TEX=000，S=0，C=1，B=1（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OIWBNOWA_SHARED      ( 0x07UL ) /* TEX=000，S=1，C=1，B=1（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OINC_NONSHARED       ( 0x08UL ) /* TEX=001，S=0，C=0，B=0（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OINC_SHARED          ( 0x0CUL ) /* TEX=001，S=1，C=0，B=0（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OIWBWA_NONSHARED     ( 0x0BUL ) /* TEX=001，S=0，C=1，B=1（存储器类型编码）。 */
#define portMPU_REGION_NORMAL_OIWBWA_SHARED        ( 0x0FUL ) /* TEX=001，S=1，C=1，B=1（存储器类型编码）。 */
#define portMPU_REGION_DEVICE_NONSHAREABLE         ( 0x10UL ) /* TEX=010，S=NA，C=0，B=0（存储器类型编码）。 */

/*
 * MPU 访问权限。该信息编码在 MPU 区域访问控制寄存器的 XN 和 AP 位中。
 */
#define portMPU_REGION_AP_BITMASK                  ( 0x07UL << 8UL )
#define portMPU_REGION_XN_BITMASK                  ( 0x01UL << 12UL )

#define portMPU_REGION_PRIV_NA_USER_NA             ( 0x00UL << 8UL )
#define portMPU_REGION_PRIV_NA_USER_NA_EXEC        ( portMPU_REGION_PRIV_NA_USER_NA ) /* 特权级：X，非特权级：X。 */
#define portMPU_REGION_PRIV_NA_USER_NA_NOEXEC      ( portMPU_REGION_PRIV_NA_USER_NA | \
                                                     portMPU_REGION_XN_BITMASK ) /* 特权级：无访问权限，非特权级：无访问权限。 */

#define portMPU_REGION_PRIV_RW_USER_NA             ( 0x01UL << 8UL )
#define portMPU_REGION_PRIV_RW_USER_NA_EXEC        ( portMPU_REGION_PRIV_RW_USER_NA ) /* 特权级：RWX，非特权级：X。 */
#define portMPU_REGION_PRIV_RW_USER_NA_NOEXEC      ( portMPU_REGION_PRIV_RW_USER_NA | \
                                                     portMPU_REGION_XN_BITMASK ) /* 特权级：RW，非特权级：无访问权限。 */

#define portMPU_REGION_PRIV_RW_USER_RO             ( 0x02UL << 8UL )
#define portMPU_REGION_PRIV_RW_USER_RO_EXEC        ( portMPU_REGION_PRIV_RW_USER_RO ) /* 特权级：RWX，非特权级：RX。 */
#define portMPU_REGION_PRIV_RW_USER_RO_NOEXEC      ( portMPU_REGION_PRIV_RW_USER_RO | \
                                                     portMPU_REGION_XN_BITMASK ) /* 特权级：RW，非特权级：R。 */

#define portMPU_REGION_PRIV_RW_USER_RW             ( 0x03UL << 8UL )
#define portMPU_REGION_PRIV_RW_USER_RW_EXEC        ( portMPU_REGION_PRIV_RW_USER_RW ) /* 特权级：RWX，非特权级：RWX。 */
#define portMPU_REGION_PRIV_RW_USER_RW_NOEXEC      ( portMPU_REGION_PRIV_RW_USER_RW | \
                                                     portMPU_REGION_XN_BITMASK ) /* 特权级：RW，非特权级：RW。 */

#define portMPU_REGION_PRIV_RO_USER_NA             ( 0x05UL << 8UL )
#define portMPU_REGION_PRIV_RO_USER_NA_EXEC        ( portMPU_REGION_PRIV_RO_USER_NA ) /* 特权级：RX，非特权级：X。 */
#define portMPU_REGION_PRIV_RO_USER_NA_NOEXEC      ( portMPU_REGION_PRIV_RO_USER_NA | \
                                                     portMPU_REGION_XN_BITMASK ) /* 特权级：R，非特权级：无访问权限。 */

#define portMPU_REGION_PRIV_RO_USER_RO             ( 0x06UL << 8UL )
#define portMPU_REGION_PRIV_RO_USER_RO_EXEC        ( portMPU_REGION_PRIV_RO_USER_RO ) /* 特权级：RX，非特权级：RX。 */
#define portMPU_REGION_PRIV_RO_USER_RO_NOEXEC      ( portMPU_REGION_PRIV_RO_USER_RO | \
                                                     portMPU_REGION_XN_BITMASK ) /* 特权级：R，非特权级：R。 */

/* MPU 区域管理。 */
#define portMPU_REGION_EXECUTE_NEVER               ( 0x01UL << 12UL )
#define portMPU_REGION_ENABLE                      ( 0x01UL )

/**
 * @brief 任务上下文的字（word）数。
 *
 * TCB 中会分配该大小的数组，任务被换出时其上下文保存在此。
 *
 * 有关浮点单元（FPU）的信息：
 * https://developer.arm.com/documentation/den0042/a/Floating-Point
 *
 * Cortex-R4-F FPU 实现的相关信息：
 * https://developer.arm.com/documentation/ddi0363/e/fpu-programmer-s-model
 *
 * Cortex-R5-F FPU 实现的相关信息：
 * https://developer.arm.com/documentation/ddi0460/d/FPU-Programmers-Model
 *
 * ArmV7-R CPSR 的相关信息：
 * https://developer.arm.com/documentation/ddi0406/cb/Application-Level-Architecture/Application-Level-Programmers--Model/The-Application-Program-Status-Register--APSR-?lang=en
 *
 * 通用寄存器（GPR）的相关信息：
 * https://developer.arm.com/documentation/ddi0406/cb/System-Level-Architecture/The-System-Level-Programmers--Model/ARM-processor-modes-and-ARM-core-registers/ARM-core-registers?lang=en
 */

#if( portENABLE_FPU == 1 )
    /*
     * 任务上下文布局（含 FPU）：
     * +-------------------+-------+----------+--------+----------+----------+----------+------+
     * | ulCriticalNesting | FPSCR |  S0-S31  | R0-R12 | SP (R13) | LR (R14) | PC (R15) | CPSR |
     * +-------------------+-------+----------+--------+----------+----------+----------+------+
     *
     * <------------------><------><---------><--------><---------><--------><----------><----->
     *           1            1        32         13         1         1          1        1
     */
    #define CONTEXT_SIZE 51U
#else
    /*
     * 任务上下文布局（不含 FPU）：
     * +-------------------+--------+----------+----------+----------+------+
     * | ulCriticalNesting | R0-R12 | SP (R13) | LR (R14) | PC (R15) | CPSR |
     * +-------------------+--------+----------+----------+----------+------+
     *
     * <------------------><--------><---------><--------><----------><----->
     *           1             13         1         1          1        1
     */
    #define CONTEXT_SIZE 18U
#endif /* CONTEXT_SIZE 条件编译结束 */

/**
 * @brief xSystemCallStackInfo 相对 TCB 起始位置的偏移量。
 */
#define portSYSTEM_CALL_INFO_OFFSET             \
    ( ( 1U /* pxTopOfStack. */ +                \
        ( portTOTAL_NUM_REGIONS_IN_TCB * 3U ) + \
        1U /* ulTaskFlags. */                   \
      ) * 4U )

#ifdef __cplusplus
} /* extern C 块结束 */
#endif

#endif /* 头文件保护：PORTMACRO_ASM_H */
