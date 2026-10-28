/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2024-2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 把
 * 所有 API 函数重定义为使用 MPU 包装器的版本。仅当从应用程序文件
 * 包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* 调度器相关头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* MPU 相关头文件。 */
#include "mpu_wrappers.h"
#include "mpu_syscall_numbers.h"

/* 移植层汇编相关头文件。 */
#include "portasm.h"

#if ( configENABLE_TRUSTZONE == 1 )
    /* 安全组件包含文件。 */
    #include "secure_context.h"
    #include "secure_init.h"
#endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/**
 * FreeRTOS Cortex-M33 移植层可配置为仅在安全侧运行，即处理器以安全状态启动，
 * 且永不跳转到非安全侧。要在安全侧运行 FreeRTOS，必须禁用移植层的 TrustZone
 * 支持。以下是有效的配置组合：
 *
 * 1. FreeRTOS 运行在安全侧：
 *    configRUN_FREERTOS_SECURE_ONLY = 1 且 configENABLE_TRUSTZONE = 0
 *
 * 2. FreeRTOS 运行在非安全侧，并支持调用安全侧函数：
 *    configRUN_FREERTOS_SECURE_ONLY = 0 且 configENABLE_TRUSTZONE = 1
 *
 * 3. FreeRTOS 仅运行在非安全侧，即不支持调用安全侧函数：
 *    configRUN_FREERTOS_SECURE_ONLY = 0 且 configENABLE_TRUSTZONE = 0
 */
#if ( ( configRUN_FREERTOS_SECURE_ONLY == 1 ) && ( configENABLE_TRUSTZONE == 1 ) )
    #error TrustZone needs to be disabled in order to run FreeRTOS on the Secure Side.
#endif

/**
 * Cortex-M23 没有非安全 PSPLIM。仅当 FreeRTOS 运行在安全侧时，
 * 才应在 Cortex-M23 上使用 PSPLIM。
 */
#if ( ( portHAS_ARMV8M_MAIN_EXTENSION == 0 ) && ( configRUN_FREERTOS_SECURE_ONLY == 0 ) )
    #define portUSE_PSPLIM_REGISTER    0
#else
    #define portUSE_PSPLIM_REGISTER    1
#endif
/*-----------------------------------------------------------*/

/**
 * @brief 所有中断服务程序（ISR）的原型。
 */
typedef void ( * portISR_t )( void );
/*-----------------------------------------------------------*/

/**
 * @brief 操作 NVIC 所需的常量。
 */
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT       ( 1UL << 25UL )
#define portNVIC_PEND_SYSTICK_SET_BIT         ( 1UL << 26UL )
#define portMIN_INTERRUPT_PRIORITY            ( 255UL )
#define portNVIC_PENDSV_PRI                   ( portMIN_INTERRUPT_PRIORITY << 16UL )
#define portNVIC_SYSTICK_PRI                  ( portMIN_INTERRUPT_PRIORITY << 24UL )
/*-----------------------------------------------------------*/

/**
 * @brief 操作 SCB 所需的常量。
 */
#define portSCB_VTOR_REG                      ( *( ( portISR_t ** ) 0xe000ed08 ) )
#define portSCB_SYS_HANDLER_CTRL_STATE_REG    ( *( ( volatile uint32_t * ) 0xe000ed24 ) )
#define portSCB_MEM_FAULT_ENABLE_BIT          ( 1UL << 16UL )
#define portSCB_USG_FAULT_ENABLE_BIT          ( 1UL << 18UL )
/*-----------------------------------------------------------*/

/**
 * @brief 用于检查 FreeRTOS 中断处理程序安装情况的常量。
 */
#define portVECTOR_INDEX_SVC       ( 11 )
#define portVECTOR_INDEX_PENDSV    ( 14 )
/*-----------------------------------------------------------*/

/**
 * @brief 校验中断优先级有效性所需的常量。
 */
#define portNVIC_SHPR2_REG                 ( *( ( volatile uint32_t * ) 0xE000ED1C ) )
#define portFIRST_USER_INTERRUPT_NUMBER    ( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16    ( 0xE000E3F0 )
#define portAIRCR_REG                      ( *( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portTOP_BIT_OF_BYTE                ( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS              ( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK            ( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT                 ( 8UL )
/*-----------------------------------------------------------*/

/**
 * @brief 系统调用进入和退出期间使用的常量。
 */
#define portPSR_STACK_PADDING_MASK              ( 1UL << 9UL )
#define portEXC_RETURN_STACK_FRAME_TYPE_MASK    ( 1UL << 4UL )
/*-----------------------------------------------------------*/

/**
 * @brief 操作 FPU 所需的常量。
 */
#define portCPACR               ( ( volatile uint32_t * ) 0xe000ed88 ) /* 协处理器访问控制寄存器。 */
#define portCPACR_CP10_VALUE    ( 3UL )
#define portCPACR_CP11_VALUE    portCPACR_CP10_VALUE
#define portCPACR_CP10_POS      ( 20UL )
#define portCPACR_CP11_POS      ( 22UL )

#define portFPCCR               ( ( volatile uint32_t * ) 0xe000ef34 ) /* 浮点上下文控制寄存器。 */
#define portFPCCR_ASPEN_POS     ( 31UL )
#define portFPCCR_ASPEN_MASK    ( 1UL << portFPCCR_ASPEN_POS )
#define portFPCCR_LSPEN_POS     ( 30UL )
#define portFPCCR_LSPEN_MASK    ( 1UL << portFPCCR_LSPEN_POS )
/*-----------------------------------------------------------*/

/**
 * @brief 在 SVC 处理程序内部时，栈中参数偏移量。
 */
#define portOFFSET_TO_LR     ( 5 )
#define portOFFSET_TO_PC     ( 6 )
#define portOFFSET_TO_PSR    ( 7 )
/*-----------------------------------------------------------*/

/**
 * @brief 操作 MPU 所需的常量。
 */
#define portMPU_TYPE_REG                        ( *( ( volatile uint32_t * ) 0xe000ed90 ) )
#define portMPU_CTRL_REG                        ( *( ( volatile uint32_t * ) 0xe000ed94 ) )
#define portMPU_RNR_REG                         ( *( ( volatile uint32_t * ) 0xe000ed98 ) )

#define portMPU_RBAR_REG                        ( *( ( volatile uint32_t * ) 0xe000ed9c ) )
#define portMPU_RLAR_REG                        ( *( ( volatile uint32_t * ) 0xe000eda0 ) )

#define portMPU_RBAR_A1_REG                     ( *( ( volatile uint32_t * ) 0xe000eda4 ) )
#define portMPU_RLAR_A1_REG                     ( *( ( volatile uint32_t * ) 0xe000eda8 ) )

#define portMPU_RBAR_A2_REG                     ( *( ( volatile uint32_t * ) 0xe000edac ) )
#define portMPU_RLAR_A2_REG                     ( *( ( volatile uint32_t * ) 0xe000edb0 ) )

#define portMPU_RBAR_A3_REG                     ( *( ( volatile uint32_t * ) 0xe000edb4 ) )
#define portMPU_RLAR_A3_REG                     ( *( ( volatile uint32_t * ) 0xe000edb8 ) )

#define portMPU_MAIR0_REG                       ( *( ( volatile uint32_t * ) 0xe000edc0 ) )
#define portMPU_MAIR1_REG                       ( *( ( volatile uint32_t * ) 0xe000edc4 ) )

#define portMPU_RBAR_ADDRESS_MASK               ( 0xffffffe0 ) /* 必须按 32 字节对齐。 */
#define portMPU_RLAR_ADDRESS_MASK               ( 0xffffffe0 ) /* 必须按 32 字节对齐。 */

#define portMPU_RBAR_ACCESS_PERMISSIONS_MASK    ( 3UL << 1UL )

#define portMPU_MAIR_ATTR0_POS                  ( 0UL )
#define portMPU_MAIR_ATTR0_MASK                 ( 0x000000ff )

#define portMPU_MAIR_ATTR1_POS                  ( 8UL )
#define portMPU_MAIR_ATTR1_MASK                 ( 0x0000ff00 )

#define portMPU_MAIR_ATTR2_POS                  ( 16UL )
#define portMPU_MAIR_ATTR2_MASK                 ( 0x00ff0000 )

#define portMPU_MAIR_ATTR3_POS                  ( 24UL )
#define portMPU_MAIR_ATTR3_MASK                 ( 0xff000000 )

#define portMPU_MAIR_ATTR4_POS                  ( 0UL )
#define portMPU_MAIR_ATTR4_MASK                 ( 0x000000ff )

#define portMPU_MAIR_ATTR5_POS                  ( 8UL )
#define portMPU_MAIR_ATTR5_MASK                 ( 0x0000ff00 )

#define portMPU_MAIR_ATTR6_POS                  ( 16UL )
#define portMPU_MAIR_ATTR6_MASK                 ( 0x00ff0000 )

#define portMPU_MAIR_ATTR7_POS                  ( 24UL )
#define portMPU_MAIR_ATTR7_MASK                 ( 0xff000000 )

#define portMPU_RLAR_ATTR_INDEX0                ( 0UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX1                ( 1UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX2                ( 2UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX3                ( 3UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX4                ( 4UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX5                ( 5UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX6                ( 6UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX7                ( 7UL << 1UL )

#define portMPU_RLAR_REGION_ENABLE              ( 1UL )

#if ( portARMV8M_MINOR_VERSION >= 1 )

    /*
     * 使能所选内存区域的特权执行禁止（PXN）MPU 属性。
     */
    #define portMPU_RLAR_PRIVILEGED_EXECUTE_NEVER    ( 1UL << 4UL )
#endif /* portARMV8M_MINOR_VERSION >= 1（ARMv8-M 次版本号大于等于 1） */

/* 使能对未映射区域的特权访问。 */
#define portMPU_PRIV_BACKGROUND_ENABLE_BIT    ( 1UL << 2UL )

/* 使能 MPU。 */
#define portMPU_ENABLE_BIT                    ( 1UL << 0UL )

/*
 * 从 RBAR（区域基地址寄存器）值中编码的
 * 提取 MPU 区域的起始地址。
 */
#define portEXTRACT_FIRST_ADDRESS_FROM_RBAR( rbar ) \
    ( ( rbar ) & portMPU_RBAR_ADDRESS_MASK )

/*
 * 从 RLAR（区域限值地址寄存器）值中提取所编码的 MPU 区域末地址。
 */
#define portEXTRACT_LAST_ADDRESS_FROM_RLAR( rlar ) \
    ( ( ( rlar ) & portMPU_RLAR_ADDRESS_MASK ) | ~portMPU_RLAR_ADDRESS_MASK )

/* 地址 addr 是否位于 [start, end] 地址范围内？ */
#define portIS_ADDRESS_WITHIN_RANGE( addr, start, end ) \
    ( ( ( addr ) >= ( start ) ) && ( ( addr ) <= ( end ) ) )

/* 可用权限是否满足访问请求？ */
#define portIS_AUTHORIZED( accessRequest, permissions ) \
    ( ( ( permissions ) & ( accessRequest ) ) == accessRequest )

/* uint32_t 类型可容纳的最大值。 */
#define portUINT32_MAX    ( ~( ( uint32_t ) 0 ) )

/* 检查 a 与 b 相加是否会溢出。 */
#define portADD_UINT32_WILL_OVERFLOW( a, b )    ( ( a ) > ( portUINT32_MAX - ( b ) ) )
/*-----------------------------------------------------------*/

/**
 * @brief 最大的 24 位数。
 *
 * 由于 SysTick 是 24 位计数器，因此需要该常量。
 */
#define portMAX_24_BIT_NUMBER       ( 0xffffffUL )

/**
 * @brief 用于估算在 tickless 空闲计算期间 SysTick 计数器停止时
 * 本应发生的 SysTick 计数次数的修正系数。
 */
#define portMISSED_COUNTS_FACTOR    ( 94UL )
/*-----------------------------------------------------------*/

/**
 * @brief 建立初始栈所需的常量。
 */
#define portINITIAL_XPSR    ( 0x01000000 )

#if ( configRUN_FREERTOS_SECURE_ONLY == 1 )

    /**
     * @brief 初始 EXC_RETURN 值。
     *
     *     FF         FF         FF         FD
     * 1111 1111  1111 1111  1111 1111  1111 1101
     *
     * Bit[6] - 1 --> 异常从安全状态进入。
     * Bit[5] - 1 --> 不跳过附加状态上下文的压栈。
     * Bit[4] - 1 --> PE 未在栈上为 FP 上下文分配空间。
     * Bit[3] - 1 --> 返回线程模式。
     * Bit[2] - 1 --> 从进程栈恢复寄存器。
     * Bit[1] - 0 --> 保留位，0。
     * Bit[0] - 1 --> 异常进入安全状态。
     */
    #define portINITIAL_EXC_RETURN    ( 0xfffffffd )
#else

    /**
     * @brief 初始 EXC_RETURN 值。
     *
     *     FF         FF         FF         BC
     * 1111 1111  1111 1111  1111 1111  1011 1100
     *
     * Bit[6] - 0 --> 异常从非安全状态进入。
     * Bit[5] - 1 --> 不跳过附加状态上下文的压栈。
     * Bit[4] - 1 --> PE 未在栈上为 FP 上下文分配空间。
     * Bit[3] - 1 --> 返回线程模式。
     * Bit[2] - 1 --> 从进程栈恢复寄存器。
     * Bit[1] - 0 --> 保留位，0。
     * Bit[0] - 0 --> 异常进入非安全状态。
     */
    #define portINITIAL_EXC_RETURN    ( 0xffffffbc )
#endif /* configRUN_FREERTOS_SECURE_ONLY（仅在安全侧运行 FreeRTOS） */

/**
 * @brief CONTROL 寄存器的特权位掩码。
 *
 * CONTROL 寄存器中的 Bit[0] 指示特权状态：
 *  Bit[0] = 0 ==> 任务处于特权状态。
 *  Bit[0] = 1 ==> 任务处于非特权状态。
 */
#define portCONTROL_PRIVILEGED_MASK         ( 1UL << 0UL )

/**
 * @brief CONTROL 寄存器的初始值。
 */
#define portINITIAL_CONTROL_UNPRIVILEGED    ( 0x3 )
#define portINITIAL_CONTROL_PRIVILEGED      ( 0x2 )

/**
 * @brief 允许用户覆盖默认的 SysTick 时钟频率。若由用户定义，
 * 该符号必须等于配置寄存器中 CLK 位为 0 时的 SysTick 时钟频率。
 */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ             ( configCPU_CLOCK_HZ )
    /* 确保 SysTick 与内核使用相同频率的时钟。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( portNVIC_SYSTICK_CLK_BIT )
#else
    /* 选择 SysTick 不与内核同频的时钟选项。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( 0 )
#endif

/**
 * @brief 允许用户覆盖初始 LR 中预加载的 prvTaskExitError() 地址，
 * 以防该地址干扰调试器中的栈回溯。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/**
 * @brief 若定义 portPRELOAD_REGISTERS，则创建任务时会为寄存器赋予初始值。
 * 这有助于调试，但会增加代码体积。
 */
#define portPRELOAD_REGISTERS    1

/**
 * @brief 任务创建时没有安全上下文，必须在发起任何安全调用之前
 * 调用 portALLOCATE_SECURE_CONTEXT() 为自身分配安全上下文。
 */
#define portNO_SECURE_CONTEXT    0

/**
 * @brief 校验和配置 PACBTI 安全特性实现所需的常量。
 */
#if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )

    #define portID_ISAR5_REG       ( *( ( volatile uint32_t * ) 0xe000ed74 ) )

    #define portCONTROL_UPAC_EN    ( 1UL << 7UL )
    #define portCONTROL_PAC_EN     ( 1UL << 6UL )
    #define portCONTROL_UBTI_EN    ( 1UL << 5UL )
    #define portCONTROL_BTI_EN     ( 1UL << 4UL )

#endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（启用 PAC 或 BTI） */
/*-----------------------------------------------------------*/

/**
 * @brief 用于捕获试图从其实现函数返回的任务。
 */
static void prvTaskExitError( void );

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 从区域基地址寄存器（RBAR）值中提取 MPU 区域的访问权限。
     *
     * @param ulRBARValue MPU 区域的 RBAR 值。
     *
     * @return uint32_t 访问权限。
     */
    static uint32_t prvGetRegionAccessPermissions( uint32_t ulRBARValue ) PRIVILEGED_FUNCTION;
#endif /* configENABLE_MPU == 1 && configUSE_MPU_WRAPPERS_V1 == 0（启用 MPU 且使用 V2 包装器） */

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 初始化内存保护单元（MPU）。
     */
    static void prvSetupMPU( void ) PRIVILEGED_FUNCTION;
#endif /* configENABLE_MPU（使能 MPU） */

#if ( configENABLE_FPU == 1 )

    /**
     * @brief 配置浮点单元（FPU）。
     */
    static void prvSetupFPU( void ) PRIVILEGED_FUNCTION;
#endif /* configENABLE_FPU（使能浮点单元） */

#if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )

    /**
     * @brief 配置 PACBTI 特性。
     *
     * 该函数按用户配置设置指针认证（Pointer Authentication）和分支目标识别
     * （Branch Target Identification）安全特性，并相应返回特殊用途 CONTROL
     * 寄存器的值，可选择是否更新 CONTROL 寄存器。目前只有 Cortex-M85
     * （基于 ARMv8.1-M 架构）目标支持 PACBTI 安全特性。
     *
     * @param xWriteControlRegister 用于控制是否更新特殊用途的 CONTROL 寄存器。
     *
     * @return 按所配置 PACBTI 选项得到的 CONTROL 寄存器值。
     */
    #if ( configNUMBER_OF_CORES == 1 )
        static uint32_t prvConfigurePACBTI( BaseType_t xWriteControlRegister );
    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
        uint32_t vConfigurePACBTI( BaseType_t xWriteControlRegister );
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

#endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（启用 PAC 或 BTI） */

/**
 * @brief 初始化定时器以产生时钟节拍中断。
 *
 * 本文件中的实现为弱定义，以便应用程序编写者
 * 更换用于产生时钟节拍中断的定时器。
 */
void vPortSetupTimerInterrupt( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 检查当前执行上下文是否为中断上下文。
 *
 * @return 若当前执行上下文为中断上下文则返回 pdTRUE，否则返回 pdFALSE。
 */
BaseType_t xPortIsInsideInterrupt( void );

/**
 * @brief 让出 CPU。
 */
void vPortYield( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 进入临界区。
 */
void vPortEnterCritical( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 退出临界区。
 */
void vPortExitCritical( void ) PRIVILEGED_FUNCTION;

/**
 * @brief SysTick 处理程序。
 */
void SysTick_Handler( void ) PRIVILEGED_FUNCTION;

/**
 * @brief SVC 处理程序的 C 语言部分。
 */
portDONT_DISCARD void vPortSVCHandler_C( uint32_t * pulCallerStackAddress ) PRIVILEGED_FUNCTION;

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 建立系统调用栈，以便从 SVC 返回后使用系统调用栈。
     *
     * @param pulTaskStack 触发 SVC 时的当前 SP。
     * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
     * @param ucSystemCallNumber 系统调用的调用编号。
     */
    void vSystemCallEnter( uint32_t * pulTaskStack,
                           uint32_t ulLR,
                           uint8_t ucSystemCallNumber ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 触发 SVC 以退出系统调用。
     */
    void vRequestSystemCallExit( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 建立任务栈，以便从 SVC 返回后重新使用任务栈。
     *
     * @param pulSystemCallStack 触发 SVC 时的当前 SP。
     * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
     */
    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 检查调用任务是否具有特权。
     *
     * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
     */
    BaseType_t xPortIsTaskPrivileged( void ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1（已启用 MPU） */

#if ( configNUMBER_OF_CORES > 1 )

    /**
     * @brief 由平台/应用定义的函数，用于唤醒从核。
     *
     * @return 若从核成功唤醒则返回 pdTRUE，否则返回 pdFALSE。
     */
    extern BaseType_t configWAKE_SECONDARY_CORES( void );

#endif /* 条件：#if ( configNUMBER_OF_CORES > 1 ) */

/**
 * @brief 配置中断优先级。
 */
void vPortConfigureInterruptPriorities( void ) PRIVILEGED_FUNCTION;
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 调度器启动时，该变量被置为 pdTRUE。
     */
    PRIVILEGED_DATA static BaseType_t xSchedulerRunning = pdFALSE;

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/**
 * @brief 每个任务通过临界区嵌套计数变量维护自身的中断状态。
 */
#if ( configNUMBER_OF_CORES == 1 )
    PRIVILEGED_DATA static volatile uint32_t ulCriticalNesting = 0UL;
#else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
    PRIVILEGED_DATA volatile uint32_t ulCriticalNestings[ configNUMBER_OF_CORES ] = { 0 };
    /* 用于检查从核是否就绪的标志。 */
    PRIVILEGED_DATA volatile uint8_t ucSecondaryCoresReadyFlags[ configNUMBER_OF_CORES - 1 ] = { 0 };
    /* 用于指示主核已完成初始化的标志。 */
    PRIVILEGED_DATA volatile uint8_t ucPrimaryCoreInitDoneFlag = 0;
 #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */

#if ( configENABLE_TRUSTZONE == 1 )

    /**
     * @brief 作为任务上下文的一部分保存，用于指示该任务在安全侧使用的上下文。
     */
    PRIVILEGED_DATA portDONT_DISCARD volatile SecureContextHandle_t xSecureContext = portNO_SECURE_CONTEXT;
#endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

/**
 * @brief 由 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 宏使用，确保不在
 * 优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用 FreeRTOS API 函数。
 */
#if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )

    static uint8_t ucMaxSysCallPriority = 0;
    static uint32_t ulMaxPRIGROUPValue = 0;
    static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * ) portNVIC_IP_REGISTERS_OFFSET_16;

#endif /* #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )（已定义 configASSERT 且具有 ARMv8-M 主扩展） */

#if ( configUSE_TICKLESS_IDLE == 1 )

    /**
     * @brief 构成一个时钟节拍周期的 SysTick 递增次数。
     */
    PRIVILEGED_DATA static uint32_t ulTimerCountsForOneTick = 0;

    /**
     * @brief 可抑制的最大时钟节拍周期数受 SysTick 定时器 24 位分辨率的限制。
     */
    PRIVILEGED_DATA static uint32_t xMaximumPossibleSuppressedTicks = 0;

    /**
     * @brief 补偿 SysTick 停止期间流逝的 CPU 周期（仅用于低功耗功能）。
     */
    PRIVILEGED_DATA static uint32_t ulStoppedTimerCompensation = 0;
#endif /* 条件：configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE == 1 )

    __attribute__( ( weak ) ) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
    {
        uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickDecrementsLeft;
        TickType_t xModifiableIdleTime;

        /* 确保 SysTick 重装载值不会使计数器溢出。 */
        if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
        {
            xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
        }

        /*
         * 进入临界区，但不使用 taskENTER_CRITICAL()
         * 方法，因为该方法会屏蔽本应退出睡眠模式的中断。
         */
        __asm volatile ( "cpsid i" ::: "memory" );
        __asm volatile ( "dsb" );
        __asm volatile ( "isb" );

        /*
         * 如果存在待处理的上下文切换，或有任务正在等待调度器
         * 解除挂起，则放弃进入低功耗模式。
         */
        if( eTaskConfirmSleepModeStatus() == eAbortSleep )
        {
            /*
             * 重新使能中断——参见上文 cpsid 指令周围的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
        else
        {
            /*
             * 暂时停止 SysTick。停止期间的时间会尽可能计入补偿，但使用
             * tickless 模式不可避免地会使内核维护的时间相对于日历时间
             * 产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /*
             * 使用 SysTick 当前值寄存器确定距离下一个时钟节拍中断
             * 剩余的 SysTick 递减次数。若当前值寄存器为零，则实际
             * 剩余 ulTimerCountsForOneTick 次递减而非零，因为 SysTick
             * 在从 1 递减到 0 时请求中断。
             */
            ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;

            if( ulSysTickDecrementsLeft == 0 )
            {
                ulSysTickDecrementsLeft = ulTimerCountsForOneTick;
            }

            /*
             * 计算等待 xExpectedIdleTime 个时钟节拍周期所需的重装载值。
             * 使用 -1 是因为本代码通常在一个节拍周期进行到一半时执行。
             * 但若此时 SysTick 中断处于挂起状态，则清除该中断，抑制第一个
             * 节拍，并修正重装载值以反映第二个节拍周期已经开始。
             * 预期空闲时间始终至少为两个节拍。
             */
            ulReloadValue = ulSysTickDecrementsLeft + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );

            if( ( portNVIC_INT_CTRL_REG & portNVIC_PEND_SYSTICK_SET_BIT ) != 0 )
            {
                portNVIC_INT_CTRL_REG = portNVIC_PEND_SYSTICK_CLEAR_BIT;
                ulReloadValue -= ulTimerCountsForOneTick;
            }

            if( ulReloadValue > ulStoppedTimerCompensation )
            {
                ulReloadValue -= ulStoppedTimerCompensation;
            }

            /* 设置新的重装载值。 */
            portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

            /*
             * 清除 SysTick 计数标志并将计数值重新置零。
             */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

            /* 重新启动 SysTick。 */
            portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

            /*
             * 睡眠直到有事件发生。configPRE_SLEEP_PROCESSING() 可将
             * 其参数置为 0，表示其实现中包含自己的等待中断或等待事件
             * 指令，因此不应再次执行 wfi。不过，原始的预期空闲时间
             * 变量必须保持不变，因此这里复制一份。
             */
            xModifiableIdleTime = xExpectedIdleTime;
            configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

            if( xModifiableIdleTime > 0 )
            {
                __asm volatile ( "dsb" ::: "memory" );
                __asm volatile ( "wfi" );
                __asm volatile ( "isb" );
            }

            configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

            /*
             * 重新使能中断，使将 MCU 从睡眠模式唤醒的中断能够立即执行。
             * 参见上文 cpsid 指令周围的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 再次禁用中断，因为时钟即将停止，而时钟停止期间执行的中断
             * 会增大 RTOS 维护时间与日历时间之间的偏差。
             */
            __asm volatile ( "cpsid i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 禁用 SysTick 时钟，但不读取 portNVIC_SYSTICK_CTRL_REG 寄存器，
             * 以确保 portNVIC_SYSTICK_COUNT_FLAG_BIT 在置位时不被清除。
             * 同样，SysTick 停止期间的时间会尽可能计入补偿，但使用 tickless
             * 模式不可避免地会使内核维护的时间相对于日历时间产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /* 判断 SysTick 是否已经计数到零。 */
            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                uint32_t ulCalculatedLoadValue;

                /*
                 * 时钟节拍中断结束了睡眠（或当前处于挂起状态），
                 * 并且新的节拍周期已经开始。用新节拍周期的剩余时间
                 * 重新设置 portNVIC_SYSTICK_LOAD_REG。
                 */
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

                /*
                 * 不允许使用过小的值，也不允许使用因睡眠后钩子执行过久
                 * 或 SysTick 当前值寄存器为零而导致下溢的值。
                 */
                if( ( ulCalculatedLoadValue <= ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
                {
                    ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
                }

                portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

                /*
                 * 由于待处理的节拍将在本函数退出后立即处理，时钟节拍
                 * 维护的 tick 值向前推进的量比等待时间少一。
                 */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /* 睡眠是被时钟节拍中断以外的事件结束的。 */

                /*
                 * 使用 SysTick 当前值寄存器确定距离预期空闲时间结束
                 * 剩余的 SysTick 递减次数。
                 */
                ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;
                #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG != portNVIC_SYSTICK_CLK_BIT )
                {
                    /*
                     * 若 SysTick 未使用内核时钟，当前值寄存器此时可能仍为零。
                     * 这种情况下，SysTick 尚未从重装载寄存器加载，预期空闲时间
                     * 内实际剩余 ulReloadValue 次递减，而不是零。
                     */
                    if( ulSysTickDecrementsLeft == 0 )
                    {
                        ulSysTickDecrementsLeft = ulReloadValue;
                    }
                }
                #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG（SysTick 时钟配置） */

                /*
                 * 计算睡眠持续时间并舍入为完整的时钟节拍周期
                 * （而不是按部分节拍计算的 ulReload 值）。
                 */
                ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - ulSysTickDecrementsLeft;

                /*
                 * 处理器等待期间经过了多少个完整时钟节拍周期？
                 */
                ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

                /*
                 * 重装载值设置为单个节拍周期剩余的比例。
                 */
                portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
            }

            /*
             * 重新启动 SysTick，使其再次从 portNVIC_SYSTICK_LOAD_REG 运行，
             * 然后将 portNVIC_SYSTICK_LOAD_REG 恢复为标准值。若 SysTick
             * 未使用内核时钟，则临时将其配置为使用内核时钟。此配置会强制
             * SysTick 立即从 portNVIC_SYSTICK_LOAD_REG 加载，而不是等到
             * 另一个时钟的下一个周期。这样 portNVIC_SYSTICK_LOAD_REG 就能
             * 立即接收标准值。
             */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
            portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG == portNVIC_SYSTICK_CLK_BIT )
            {
                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
            }
            #else
            {
                /*
                 * 如上所述，临时使用内核时钟的目的已经达到。
                 * 恢复使用另一个时钟。
                 */
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT;

                if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
                {
                    /*
                     * 部分节拍周期已经结束。确保 SysTick 只对其计数一次。
                     */
                    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
                }

                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            }
            #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG（SysTick 时钟配置） */

            /* 推进 tick 计数，以补偿已经流逝的时钟节拍周期。 */
            vTaskStepTick( ulCompleteTickPeriods );

            /* 在中断使能状态下退出。 */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
    }

#endif /* 条件：configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

__attribute__( ( weak ) ) void vPortSetupTimerInterrupt( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    /* 计算配置时钟节拍中断所需的常量。 */
    #if ( configUSE_TICKLESS_IDLE == 1 )
    {
        ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
        xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
        ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
    }
    #endif /* 条件：configUSE_TICKLESS_IDLE */

    /*
     * 停止并复位 SysTick。
     *
     * 7.0.0 之前的 QEMU 版本存在一个缺陷：若未先选择有效的时钟源
     * 就使能 SysTick，将导致错误。如果在同一时刻把时钟源从零时钟周期
     * 切换为非零时钟周期并使能 SysTick，就会触发该缺陷。
     * 因此这里先配置 CLKSOURCE 位，再设置 ENABLE 位。
     * 该变通方法可避免 7.0.0 之前 QEMU 版本中的缺陷。
     */
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* 配置 SysTick 以请求的速率产生中断。 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0UL;

    /*
     * 实现任务的函数不得退出或试图返回其调用者，因为无处可返回。
     * 若任务需要退出，应改为调用 vTaskDelete( NULL )。若定义了
     * configASSERT()，则人为触发 assert()，然后停在此处，
     * 以便应用程序编写者捕获该错误。
     */
    #if ( configNUMBER_OF_CORES == 1 )
        configASSERT( ulCriticalNesting == ~0UL );
    #else /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
        configASSERT( ulCriticalNestings[ portGET_CORE_ID() ] == ~0UL );
    #endif /* 条件：#if ( configNUMBER_OF_CORES == 1 ) */
    portDISABLE_INTERRUPTS();

    while( ulDummy == 0 )
    {
        /*
         * 调度器启动后，本文件会调用 prvTaskExitError()，以消除
         * 有关函数已定义但从未调用的编译器警告。ulDummy 纯粹用于
         * 消除有关该函数调用之后出现代码的其他警告——将 ulDummy
         * 声明为 volatile 会使编译器认为该函数可能返回，从而不会对
         * 其后的代码输出“不可达代码”警告。
         */
    }
}
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    static uint32_t prvGetRegionAccessPermissions( uint32_t ulRBARValue ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        uint32_t ulAccessPermissions = 0;

        if( ( ulRBARValue & portMPU_RBAR_ACCESS_PERMISSIONS_MASK ) == portMPU_REGION_READ_ONLY )
        {
            ulAccessPermissions = tskMPU_READ_PERMISSION;
        }

        if( ( ulRBARValue & portMPU_RBAR_ACCESS_PERMISSIONS_MASK ) == portMPU_REGION_READ_WRITE )
        {
            ulAccessPermissions = ( tskMPU_READ_PERMISSION | tskMPU_WRITE_PERMISSION );
        }

        return ulAccessPermissions;
    }

#endif /* configENABLE_MPU == 1 && configUSE_MPU_WRAPPERS_V1 == 0（启用 MPU 且使用 V2 包装器） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    static void prvSetupMPU( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;
            extern uint32_t * __unprivileged_flash_start__;
            extern uint32_t * __unprivileged_flash_end__;
            extern uint32_t * __privileged_sram_start__;
            extern uint32_t * __privileged_sram_end__;
        #else /* __ARMCC_VERSION 定义条件 */
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];
            extern uint32_t __unprivileged_flash_start__[];
            extern uint32_t __unprivileged_flash_end__[];
            extern uint32_t __privileged_sram_start__[];
            extern uint32_t __privileged_sram_end__[];
        #endif /* __ARMCC_VERSION 定义条件 */

        /* 允许的区域数量只能是 8 或 16。 */
        configASSERT( ( configTOTAL_MPU_REGIONS == 8 ) || ( configTOTAL_MPU_REGIONS == 16 ) );

        /* MAIR0 - 索引 0。 */
        portMPU_MAIR0_REG |= ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_ATTR0_POS ) & portMPU_MAIR_ATTR0_MASK );
        /* MAIR0 - 索引 1。 */
        portMPU_MAIR0_REG |= ( ( portMPU_DEVICE_MEMORY_nGnRE << portMPU_MAIR_ATTR1_POS ) & portMPU_MAIR_ATTR1_MASK );

        /*
         * 将特权 Flash 设置为只读，使特权任务可以读取但无法修改。
         */
        portMPU_RNR_REG = portPRIVILEGED_FLASH_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __privileged_functions_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_PRIVILEGED_READ_ONLY );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __privileged_functions_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /*
         * 将非特权 Flash 设置为特权和
         * 非特权任务均可只读。所有任务均可读取，但任何人都不能修改。
         */
        portMPU_RNR_REG = portUNPRIVILEGED_FLASH_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __unprivileged_flash_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_READ_ONLY );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __unprivileged_flash_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /*
         * 将非特权系统调用（syscalls）Flash 区域设置为特权任务和非特权任务均只读。
         * 所有任务都可读取，但任何任务都不能修改。
         */
        portMPU_RNR_REG = portUNPRIVILEGED_SYSCALLS_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __syscalls_flash_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_READ_ONLY );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __syscalls_flash_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /* 设置包含内核数据的 RAM 仅供特权访问。 */
        portMPU_RNR_REG = portPRIVILEGED_RAM_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __privileged_sram_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_PRIVILEGED_READ_WRITE ) |
                           ( portMPU_REGION_EXECUTE_NEVER );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __privileged_sram_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /* 使能存储器故障（MemManage）异常。 */
        portSCB_SYS_HANDLER_CTRL_STATE_REG |= portSCB_MEM_FAULT_ENABLE_BIT;

        /*
         * 使能 MPU 并启用特权背景访问，即未映射区域具有特权访问权限。
         */
        portMPU_CTRL_REG |= ( portMPU_PRIV_BACKGROUND_ENABLE_BIT | portMPU_ENABLE_BIT );
    }

#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

#if ( configENABLE_FPU == 1 )

    static void prvSetupFPU( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        #if ( configENABLE_TRUSTZONE == 1 )
        {
            /* 使能对 FPU 的非安全访问。 */
            SecureInit_EnableNSFPUAccess();
        }
        #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

        /*
         * CP10 = 11 ==> 对 FPU 的完全访问，即特权代码和非特权代码均可访问 FPU。
         * CP11 应编程为与 CP10 相同的值。
         */
        *( portCPACR ) |= ( ( portCPACR_CP10_VALUE << portCPACR_CP10_POS ) |
                            ( portCPACR_CP11_VALUE << portCPACR_CP11_POS )
                            );

        /*
         * ASPEN = 1 ==> 硬件应在异常进入时自动保存浮点上下文，并在异常返回时恢复。
         * LSPEN = 1 ==> 使能 FP 状态的惰性上下文保存。
         */
        *( portFPCCR ) |= ( portFPCCR_ASPEN_MASK | portFPCCR_LSPEN_MASK );
    }

#endif /* configENABLE_FPU（使能浮点单元） */
/*-----------------------------------------------------------*/

void vPortYield( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    /* 设置 PendSV 以请求一次上下文切换。 */
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;

    /*
     * 通常不需要屏障指令，但使用它们可确保代码完全符合架构规定行为。
     */
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" );
}
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES == 1 )
    void vPortEnterCritical( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        portDISABLE_INTERRUPTS();
        ulCriticalNesting++;
        /*
         * 通常不需要屏障指令，但使用它们可确保代码完全符合架构规定行为。
         */
        __asm volatile ( "dsb" ::: "memory" );
        __asm volatile ( "isb" );
    }
    /*-----------------------------------------------------------*/

    void vPortExitCritical( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        configASSERT( ulCriticalNesting );
        ulCriticalNesting--;

        if( ulCriticalNesting == 0 )
        {
            portENABLE_INTERRUPTS();
        }
    }
#endif /* configNUMBER_OF_CORES == 1（单核配置） */
/*-----------------------------------------------------------*/

void SysTick_Handler( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    uint32_t ulPreviousMask;

    ulPreviousMask = portSET_INTERRUPT_MASK_FROM_ISR();
    #if ( configNUMBER_OF_CORES > 1 )
        UBaseType_t uxSavedInterruptStatus = portENTER_CRITICAL_FROM_ISR();
    #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */

    traceISR_ENTER();
    {
        /* 递增 RTOS 时钟节拍。 */
        if( xTaskIncrementTick() != pdFALSE )
        {
            traceISR_EXIT_TO_SCHEDULER();
            /* 挂起一次上下文切换。 */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
        else
        {
            traceISR_EXIT();
        }
    }
    #if ( configNUMBER_OF_CORES > 1 )
        portEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
    #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */

    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulPreviousMask );
}
/*-----------------------------------------------------------*/

void vPortSVCHandler_C( uint32_t * pulCallerStackAddress ) /* PRIVILEGED_FUNCTION portDONT_DISCARD（特权函数，禁止链接器丢弃） */
{
    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 ) )
        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;
        #else
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];
        #endif /* __ARMCC_VERSION 定义条件 */
    #endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 1（使用 V1 MPU 包装器） */

    #if ( configENABLE_MPU == 1 )
        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
        #else
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
        #endif /* __ARMCC_VERSION 定义条件 */
    #endif /* configENABLE_MPU == 1（已启用 MPU） */

    uint32_t ulPC;

    #if ( configENABLE_TRUSTZONE == 1 )
        uint32_t ulR0, ulR1;
        extern TaskHandle_t pxCurrentTCB;
        #if ( configENABLE_MPU == 1 )
            uint32_t ulControl, ulIsTaskPrivileged;
        #endif /* configENABLE_MPU（使能 MPU） */
    #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */
    uint8_t ucSVCNumber;

    /*
     * 寄存器按以下顺序保存在栈中：R0、R1、R2、R3、
     * R12、LR、PC、xPSR。
     */
    ulPC = pulCallerStackAddress[ portOFFSET_TO_PC ];
    ucSVCNumber = ( ( uint8_t * ) ulPC )[ -2 ];

    switch( ucSVCNumber )
    {
    #if ( configENABLE_TRUSTZONE == 1 )
        case portSVC_ALLOCATE_SECURE_CONTEXT:

            /*
             * R0 包含作为参数传给 vPortAllocateSecureContext 函数的栈大小。
             */
            ulR0 = pulCallerStackAddress[ 0 ];

            #if ( configENABLE_MPU == 1 )
            {
                /* 读取 CONTROL 寄存器的值。 */
                __asm volatile ( "mrs %0, control"  : "=r" ( ulControl ) );

                /*
                 * 若 CONTROL 寄存器的 bit[0] 为 0，则触发 SVC 的任务为特权级。
                 */
                ulIsTaskPrivileged = ( ( ulControl & portCONTROL_PRIVILEGED_MASK ) == 0 );

                /* 为安全任务分配并加载上下文。 */
                xSecureContext = SecureContext_AllocateContext( ulR0, ulIsTaskPrivileged, pxCurrentTCB );
            }
            #else /* if ( configENABLE_MPU == 1 )（启用 MPU） */
            {
                /* 为安全任务分配并加载上下文。 */
                xSecureContext = SecureContext_AllocateContext( ulR0, pxCurrentTCB );
            }
            #endif /* configENABLE_MPU（使能 MPU） */

            configASSERT( xSecureContext != securecontextINVALID_CONTEXT_ID );
            SecureContext_LoadContext( xSecureContext, pxCurrentTCB );
            break;

        case portSVC_FREE_SECURE_CONTEXT:
            #if ( configENABLE_MPU == 1 )
            if( ( ulPC >= ( uint32_t ) __privileged_functions_start__ ) &&
                ( ulPC <= ( uint32_t ) __privileged_functions_end__ ) )
            {
            #endif /* configENABLE_MPU（使能 MPU） */
                /*
                 * R0 包含待释放的 TCB，R1 包含待释放的安全上下文句柄。
                 */
                ulR0 = pulCallerStackAddress[ 0 ];
                ulR1 = pulCallerStackAddress[ 1 ];

                /* 释放安全上下文。 */
                SecureContext_FreeContext( ( SecureContextHandle_t ) ulR1, ( void * ) ulR0 );
            #if ( configENABLE_MPU == 1 )
            }
            #endif /* configENABLE_MPU（使能 MPU） */
            break;
    #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

        case portSVC_START_SCHEDULER:
            #if ( configENABLE_MPU == 1 )
            if( ( ulPC >= ( uint32_t ) __privileged_functions_start__ ) &&
                ( ulPC <= ( uint32_t ) __privileged_functions_end__ ) )
            {
            #endif /* configENABLE_MPU（使能 MPU） */
                #if ( configENABLE_TRUSTZONE == 1 )
                {
                    /*
                     * 降低非安全异常的优先级，使非安全 PendSV 以最低优先级运行。
                     */
                    SecureInit_DePrioritizeNSExceptions();

                    /* 初始化安全上下文管理系统。 */
                    SecureContext_Init();
                }
                #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

                #if ( configENABLE_FPU == 1 )
                {
                    /* 配置浮点单元（FPU）。 */
                    prvSetupFPU();
                }
                #endif /* configENABLE_FPU（使能浮点单元） */

                /*
                 * 设置第一个任务的上下文，使第一个任务开始执行。
                 */
                vRestoreContextOfFirstTask();
            #if ( configENABLE_MPU == 1 )
            }
            #endif /* configENABLE_MPU（使能 MPU） */
            break;

    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 ) )
        case portSVC_RAISE_PRIVILEGE:

            /*
             * 仅当 SVC 是从任一系统调用触发时，才提升特权。
             */
            if( ( ulPC >= ( uint32_t ) __syscalls_flash_start__ ) &&
                ( ulPC <= ( uint32_t ) __syscalls_flash_end__ ) )
            {
                vRaisePrivilege();
            }
            break;
    #endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 1（使用 V1 MPU 包装器） */

    #if ( configENABLE_MPU == 1 )
        case portSVC_YIELD:
            vPortYield();
            break;
    #endif /* configENABLE_MPU == 1（已启用 MPU） */

        default:
            /* 错误的 SVC 调用。 */
            configASSERT( pdFALSE );
    }
}
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vSystemCallEnter( uint32_t * pulTaskStack,
                           uint32_t ulLR,
                           uint8_t ucSystemCallNumber ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        extern TaskHandle_t pxCurrentTCB;
        extern UBaseType_t uxSystemCallImplementations[ NUM_SYSTEM_CALLS ];
        xMPU_SETTINGS * pxMpuSettings;
        uint32_t * pulSystemCallStack;
        uint32_t ulHardwareSavedExceptionFrameSize, ulSystemCallLocation, i;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;
        #else
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];
        #endif /* __ARMCC_VERSION 定义条件 */

        ulSystemCallLocation = pulTaskStack[ portOFFSET_TO_PC ];
        pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );

        /*
         * 检查：
         * 1. SVC 是从系统调用代码段触发的（即应用程序没有直接触发 SVC）。
         * 2. pxMpuSettings->xSystemCallStackInfo.pulTaskStack 必须为 NULL，
         *    因为仅在系统调用执行期间（即系统调用进入和退出之间）
         *    该值才非 NULL。
         * 3. 系统调用对应的内核 API 未被 FreeRTOSConfig.h 中的配置禁用。
         * 4. 无需检查 ucSystemCallNumber 是否在有效范围内，
         *    因为汇编 SVC 处理程序在调用本函数前已经检查过。
         */
        if( ( ulSystemCallLocation >= ( uint32_t ) __syscalls_flash_start__ ) &&
            ( ulSystemCallLocation <= ( uint32_t ) __syscalls_flash_end__ ) &&
            ( pxMpuSettings->xSystemCallStackInfo.pulTaskStack == NULL ) &&
            ( uxSystemCallImplementations[ ucSystemCallNumber ] != ( UBaseType_t ) 0 ) )
        {
            pulSystemCallStack = pxMpuSettings->xSystemCallStackInfo.pulSystemCallStack;

            /*
             * 异常进入时硬件保存的栈帧大小：
             * - 无 FPU：基本帧（R0-R3、R12、LR、PC 和 xPSR）= 8 字。
             * - 有 FPU（惰性压栈）：基本帧 + S0–S15 + FPSCR + 保留字 = 26 字。
             */
            #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            {
                if( ( ulLR & portEXC_RETURN_STACK_FRAME_TYPE_MASK ) == 0UL )
                {
                    /* 扩展帧，即使用了 FPU。 */
                    ulHardwareSavedExceptionFrameSize = 26;
                    __asm volatile (
                        " vpush {s0}         \n" /* 触发惰性压栈。 */
                        " vpop  {s0}         \n" /* 抵消上述指令的影响。 */
                        ::: "memory"
                        );
                }
                else
                {
                    /* 标准帧，即未使用 FPU。 */
                    ulHardwareSavedExceptionFrameSize = 8;
                }
            }
            #else /* if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )（启用 FPU 或 MVE） */
            {
                ulHardwareSavedExceptionFrameSize = 8;
            }
            #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */

            /* 在系统调用栈上为栈帧分配空间。 */
            pulSystemCallStack = pulSystemCallStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulSystemCallStack[ i ] = pulTaskStack[ i ];
            }

            /*
             * 保存触发 SVC 之前的链接寄存器值。
             * 其中包含系统调用入口点的调用者地址（即 MPU_<API> 的调用者）。
             * 退出系统调用时需要恢复该值。
             */
            pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry = pulTaskStack[ portOFFSET_TO_LR ];

            /*
             * 保存触发 SVC 之前的 PSPLIM 寄存器值。
             * 从系统调用退出时需要恢复该值。
             */
            #if ( portUSE_PSPLIM_REGISTER == 1 )
            {
                __asm volatile ( "mrs %0, psplim" : "=r" ( pxMpuSettings->xSystemCallStackInfo.ulStackLimitRegisterAtSystemCallEntry ) );
            }
            #endif

            /* 在线程模式下使用 pulSystemCallStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulSystemCallStack ) );
            #if ( portUSE_PSPLIM_REGISTER == 1 )
            {
                __asm volatile ( "msr psplim, %0" : : "r" ( pxMpuSettings->xSystemCallStackInfo.pulSystemCallStackLimit ) );
            }
            #endif

            /* 从本处理程序返回后开始执行系统调用。 */
            pulSystemCallStack[ portOFFSET_TO_PC ] = uxSystemCallImplementations[ ucSystemCallNumber ];

            /*
             * 请求在系统调用完成后退出系统调用。
             */
            pulSystemCallStack[ portOFFSET_TO_LR ] = ( uint32_t ) vRequestSystemCallExit;

            /*
             * 记住退出系统调用时应复制栈帧的位置。
             */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = pulTaskStack + ulHardwareSavedExceptionFrameSize;

            /*
             * 记录硬件是否使用填充强制栈指针双字对齐。
             */
            if( ( pulTaskStack[ portOFFSET_TO_PSR ] & portPSR_STACK_PADDING_MASK ) == portPSR_STACK_PADDING_MASK )
            {
                pxMpuSettings->ulTaskFlags |= portSTACK_FRAME_HAS_PADDING_FLAG;
            }
            else
            {
                pxMpuSettings->ulTaskFlags &= ( ~portSTACK_FRAME_HAS_PADDING_FLAG );
            }

            /*
             * 在 pxPortInitialiseStack 中已确保系统调用栈
             * 双字对齐，因此无需填充。
             * 清除已保存 xPSR 的 bit[9]。
             */
            pulSystemCallStack[ portOFFSET_TO_PSR ] &= ( ~portPSR_STACK_PADDING_MASK );

            /* 在系统调用执行期间提升特权。 */
            __asm volatile (
                " mrs r0, control     \n" /* 获取当前 CONTROL 值。 */
                " movs r1, #1         \n" /* r1 置为 1。 */
                " bics r0, r1         \n" /* 清除 nPRIV 位。 */
                " msr control, r0     \n" /* 写回新的 CONTROL 值。 */
                ::: "r0", "r1", "memory"
                );
        }
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vRequestSystemCallExit( void ) /* __attribute__( ( naked ) ) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile ( "svc %0 \n" ::"i" ( portSVC_SYSTEM_CALL_EXIT ) : "memory" );
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        extern TaskHandle_t pxCurrentTCB;
        xMPU_SETTINGS * pxMpuSettings;
        uint32_t * pulTaskStack;
        uint32_t ulHardwareSavedExceptionFrameSize, ulSystemCallLocation, i;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
        #else
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
        #endif /* __ARMCC_VERSION 定义条件 */

        ulSystemCallLocation = pulSystemCallStack[ portOFFSET_TO_PC ];
        pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );

        /*
         * 检查：
         * 1. SVC 是从特权代码触发的（即应用程序没有直接触发 SVC）。
         *    此 SVC 仅由位于特权代码段的 vRequestSystemCallExit 触发。
         * 2. pxMpuSettings->xSystemCallStackInfo.pulTaskStack 必须非 NULL——
         *    这表明此前已进入系统调用，应用程序没有在未进入系统调用
         *    的情况下尝试退出。
         */
        if( ( ulSystemCallLocation >= ( uint32_t ) __privileged_functions_start__ ) &&
            ( ulSystemCallLocation <= ( uint32_t ) __privileged_functions_end__ ) &&
            ( pxMpuSettings->xSystemCallStackInfo.pulTaskStack != NULL ) )
        {
            pulTaskStack = pxMpuSettings->xSystemCallStackInfo.pulTaskStack;

            /*
             * 异常进入时硬件保存的栈帧大小：
             * - 无 FPU：基本帧（R0-R3、R12、LR、PC 和 xPSR）= 8 字。
             * - 有 FPU（惰性压栈）：基本帧 + S0–S15 + FPSCR + 保留字 = 26 字。
             */
            #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )
            {
                if( ( ulLR & portEXC_RETURN_STACK_FRAME_TYPE_MASK ) == 0UL )
                {
                    /* 扩展帧，即使用了 FPU。 */
                    ulHardwareSavedExceptionFrameSize = 26;
                    __asm volatile (
                        " vpush {s0}         \n" /* 触发惰性压栈。 */
                        " vpop  {s0}         \n" /* 抵消上述指令的影响。 */
                        ::: "memory"
                        );
                }
                else
                {
                    /* 标准帧，即未使用 FPU。 */
                    ulHardwareSavedExceptionFrameSize = 8;
                }
            }
            #else /* if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )（启用 FPU 或 MVE） */
            {
                ulHardwareSavedExceptionFrameSize = 8;
            }
            #endif /* configENABLE_FPU || configENABLE_MVE（启用 FPU 或 MVE） */

            /* 在任务栈上为栈帧分配空间。 */
            pulTaskStack = pulTaskStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulTaskStack[ i ] = pulSystemCallStack[ i ];
            }

            /* 在线程模式下使用 pulTaskStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulTaskStack ) );

            /*
             * 返回系统调用入口点的调用者（即 MPU_<API> 的调用者）。
             */
            pulTaskStack[ portOFFSET_TO_PC ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;
            /* 确保 LR 具有有效值。 */
            pulTaskStack[ portOFFSET_TO_LR ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;

            /*
             * 将 PSPLIM 寄存器恢复为系统调用入口时的值。
             */
            #if ( portUSE_PSPLIM_REGISTER == 1 )
            {
                __asm volatile ( "msr psplim, %0" : : "r" ( pxMpuSettings->xSystemCallStackInfo.ulStackLimitRegisterAtSystemCallEntry ) );
            }
            #endif

            /*
             * 若硬件使用了填充来强制栈指针双字对齐，
             * 则置位已保存 xPSR 的 bit[9]，否则清除该位。
             */
            if( ( pxMpuSettings->ulTaskFlags & portSTACK_FRAME_HAS_PADDING_FLAG ) == portSTACK_FRAME_HAS_PADDING_FLAG )
            {
                pulTaskStack[ portOFFSET_TO_PSR ] |= portPSR_STACK_PADDING_MASK;
            }
            else
            {
                pulTaskStack[ portOFFSET_TO_PSR ] &= ( ~portPSR_STACK_PADDING_MASK );
            }

            /* 仅在系统调用期间该值才非 NULL。 */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = NULL;

            /* 返回线程模式前降低特权。 */
            __asm volatile (
                " mrs r0, control     \n" /* 获取当前 CONTROL 值。 */
                " movs r1, #1         \n" /* r1 置为 1。 */
                " orrs r0, r1         \n" /* 设置 nPRIV 位。 */
                " msr control, r0     \n" /* 写回新的 CONTROL 值。 */
                ::: "r0", "r1", "memory"
                );
        }
    }

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    BaseType_t xPortIsTaskPrivileged( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        BaseType_t xTaskIsPrivileged = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xTaskIsPrivileged = pdTRUE;
        }

        return xTaskIsPrivileged;
    }

#endif /* configENABLE_MPU == 1（已启用 MPU） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         StackType_t * pxEndOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters,
                                         BaseType_t xRunPrivileged,
                                         xMPU_SETTINGS * xMPUSettings ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        uint32_t ulIndex = 0;
        uint32_t ulControl = 0x0;

        xMPUSettings->ulContext[ ulIndex ] = 0x04040404; /* r4 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x05050505; /* r5 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x06060606; /* r6 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x07070707; /* r7 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x08080808; /* r8 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x09090909; /* r9 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x10101010; /* r10 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x11111111; /* r11 寄存器。 */
        ulIndex++;

        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) pvParameters; /* r0 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x01010101; /* r1 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x02020202; /* r2 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x03030303; /* r3 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x12121212; /* r12 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) portTASK_RETURN_ADDRESS; /* LR 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) pxCode; /* PC 寄存器。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = portINITIAL_XPSR; /* xPSR 寄存器。 */
        ulIndex++;

        #if ( configENABLE_TRUSTZONE == 1 )
        {
            xMPUSettings->ulContext[ ulIndex ] = portNO_SECURE_CONTEXT; /* xSecureContext（安全上下文）。 */
            ulIndex++;
        }
        #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) ( pxTopOfStack - 8 ); /* PSP 指向硬件保存的栈帧。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) pxEndOfStack; /* PSPLIM（进程栈限）。 */
        ulIndex++;

        #if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
        {
            /*
             * 在将 CONTROL 寄存器的值压入任务 TCB 之前，检查 PACBTI 安全特性配置。
             */
            #if ( configNUMBER_OF_CORES == 1 )
                ulControl = prvConfigurePACBTI( pdFALSE );
            #else /* configNUMBER_OF_CORES > 1（多核配置） */
                ulControl = vConfigurePACBTI( pdFALSE );
            #endif /* configNUMBER_OF_CORES 结束 */
        }
        #endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（启用 PAC 或 BTI） */

        if( xRunPrivileged == pdTRUE )
        {
            xMPUSettings->ulTaskFlags |= portTASK_IS_PRIVILEGED_FLAG;
            xMPUSettings->ulContext[ ulIndex ] = ( ulControl | ( uint32_t ) portINITIAL_CONTROL_PRIVILEGED ); /* CONTROL 寄存器。 */
            ulIndex++;
        }
        else
        {
            xMPUSettings->ulTaskFlags &= ( ~portTASK_IS_PRIVILEGED_FLAG );
            xMPUSettings->ulContext[ ulIndex ] = ( ulControl | ( uint32_t ) portINITIAL_CONTROL_UNPRIVILEGED ); /* CONTROL 寄存器。 */
            ulIndex++;
        }

        xMPUSettings->ulContext[ ulIndex ] = portINITIAL_EXC_RETURN; /* LR（EXC_RETURN 值）。 */
        ulIndex++;

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
        {
            /* 确保系统调用栈双字对齐。 */
            xMPUSettings->xSystemCallStackInfo.pulSystemCallStack = &( xMPUSettings->xSystemCallStackInfo.ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE - 1 ] );
            xMPUSettings->xSystemCallStackInfo.pulSystemCallStack = ( uint32_t * ) ( ( uint32_t ) ( xMPUSettings->xSystemCallStackInfo.pulSystemCallStack ) &
                                                                                     ( uint32_t ) ( ~( portBYTE_ALIGNMENT_MASK ) ) );

            xMPUSettings->xSystemCallStackInfo.pulSystemCallStackLimit = &( xMPUSettings->xSystemCallStackInfo.ulSystemCallStackBuffer[ 0 ] );
            xMPUSettings->xSystemCallStackInfo.pulSystemCallStackLimit = ( uint32_t * ) ( ( ( uint32_t ) ( xMPUSettings->xSystemCallStackInfo.pulSystemCallStackLimit ) +
                                                                                            ( uint32_t ) ( portBYTE_ALIGNMENT - 1 ) ) &
                                                                                          ( uint32_t ) ( ~( portBYTE_ALIGNMENT_MASK ) ) );

            /* 仅在系统调用期间该值才非 NULL。 */
            xMPUSettings->xSystemCallStackInfo.pulTaskStack = NULL;
        }
        #endif /* configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

        #if ( configENABLE_PAC == 1 )
        {
            uint32_t ulTaskPacKey[ 4 ], i;

            vApplicationGenerateTaskRandomPacKey( &( ulTaskPacKey[ 0 ] ) );

            for( i = 0; i < 4; i++ )
            {
                xMPUSettings->ulContext[ ulIndex ] = ulTaskPacKey[ i ];
                ulIndex++;
            }
        }
        #endif /* configENABLE_PAC（使能 PAC） */

        return &( xMPUSettings->ulContext[ ulIndex ] );
    }

#else /* configENABLE_MPU（使能 MPU） */

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         StackType_t * pxEndOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        /*
         * 模拟上下文切换中断会创建的栈帧。
         */
        #if ( portPRELOAD_REGISTERS == 0 )
        {
            pxTopOfStack--; /* 补偿 MCU 进出中断时使用栈的方式所加的偏移量。 */
            *pxTopOfStack = portINITIAL_XPSR; /* xPSR 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxCode; /* PC 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR 寄存器。 */
            pxTopOfStack -= 5; /* R12、R3、R2 和 R1 寄存器。 */
            *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器。 */
            pxTopOfStack -= 9; /* R11..R4 及 EXC_RETURN。 */
            *pxTopOfStack = portINITIAL_EXC_RETURN;
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxEndOfStack; /* 用于存放该任务 PSPLIM 值的槽位。 */

            #if ( configENABLE_TRUSTZONE == 1 )
            {
                pxTopOfStack--;
                *pxTopOfStack = portNO_SECURE_CONTEXT; /* 用于存放该任务 xSecureContext 值的槽位。 */
            }
            #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */
        }
        #else /* portPRELOAD_REGISTERS（预装载寄存器） */
        {
            pxTopOfStack--; /* 补偿 MCU 进出中断时使用栈的方式所加的偏移量。 */
            *pxTopOfStack = portINITIAL_XPSR; /* xPSR 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxCode; /* PC 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x12121212UL; /* R12 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x03030303UL; /* R3 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x02020202UL; /* R2 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x01010101UL; /* R1 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x11111111UL; /* R11 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x10101010UL; /* R10 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x09090909UL; /* R09 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x08080808UL; /* R08 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x07070707UL; /* R07 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x06060606UL; /* R06 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x05050505UL; /* R05 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x04040404UL; /* R04 寄存器。 */
            pxTopOfStack--;
            *pxTopOfStack = portINITIAL_EXC_RETURN; /* EXC_RETURN 值。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxEndOfStack; /* 用于存放该任务 PSPLIM 值的槽位。 */

            #if ( configENABLE_TRUSTZONE == 1 )
            {
                pxTopOfStack--;
                *pxTopOfStack = portNO_SECURE_CONTEXT; /* 用于存放该任务 xSecureContext 值的槽位。 */
            }
            #endif /* configENABLE_TRUSTZONE（使能 TrustZone） */
        }
        #endif /* portPRELOAD_REGISTERS（预装载寄存器） */

        #if ( configENABLE_PAC == 1 )
        {
            uint32_t ulTaskPacKey[ 4 ], i;

            vApplicationGenerateTaskRandomPacKey( &( ulTaskPacKey[ 0 ] ) );

            for( i = 0; i < 4; i++ )
            {
                pxTopOfStack--;
                *pxTopOfStack = ulTaskPacKey[ i ];
            }
        }
        #endif /* configENABLE_PAC（使能 PAC） */

        return pxTopOfStack;
    }

#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    /*
     * 应用程序可以采用以下方式之一安装 FreeRTOS 中断处理程序：
     * 1. 直接路由——分别为 SVCall 和 PendSV 中断安装
     *    SVC_Handler 和 PendSV_Handler 函数。
     * 2. 间接路由——为 SVCall 和 PendSV 中断安装独立的处理程序，
     *    并将程序控制流从这些处理程序转发到 SVC_Handler 和
     *    PendSV_Handler 函数。
     *
     * 使用间接路由的应用程序必须在其 FreeRTOSConfig.h 中将
     * configCHECK_HANDLER_INSTALLATION 设置为 0。在
     * configCHECK_HANDLER_INSTALLATION 为 1 时于此处校验的直接
     * 路由应尽可能优先使用。
     */
    #if ( configCHECK_HANDLER_INSTALLATION == 1 )
    {
        const portISR_t * const pxVectorTable = portSCB_VTOR_REG;

        /*
         * 校验应用程序已正确安装 SVCall 和 PendSV 中断的 FreeRTOS
         * 处理程序。此处不检查 SysTick 处理程序的安装情况，因为应用程序
         * 可能通过重写弱函数 vPortSetupTimerInterrupt() 使用 SysTick
         * 以外的定时器驱动 RTOS 时钟节拍。
         *
         * 此处的断言失败表明 FreeRTOS 处理程序安装不正确。有关安装
         * FreeRTOS 处理程序的帮助，请参阅
         * https://www.freertos.org/Why-FreeRTOS/FAQs。
         *
         * 对于中断向量表地址可配置的系统，若 VTOR 未正确设置为指向
         * 应用程序的向量表，此处也可能出现断言失败甚至系统故障。
         */
        configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == SVC_Handler );
        configASSERT( pxVectorTable[ portVECTOR_INDEX_PENDSV ] == PendSV_Handler );
    }
    #endif /* configCHECK_HANDLER_INSTALLATION（检查处理程序安装） */

    vPortConfigureInterruptPriorities();

    #if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
    {
        /*
         * 在启动第一个任务之前，根据 PACBTI 安全特性配置设置 CONTROL 寄存器的值。
         */
        #if ( configNUMBER_OF_CORES == 1 )
            ( void ) prvConfigurePACBTI( pdTRUE );
        #else /* configNUMBER_OF_CORES > 1（多核配置） */
            ( void ) vConfigurePACBTI( pdTRUE );
        #endif /* configNUMBER_OF_CORES 结束 */
    }
    #endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（启用 PAC 或 BTI） */

    #if ( configENABLE_MPU == 1 )
    {
        /* 初始化内存保护单元（MPU）。 */
        prvSetupMPU();
    }
    #endif /* configENABLE_MPU（使能 MPU） */

    #if ( configNUMBER_OF_CORES > 1 )
        /*
         * 启动产生时钟节拍 ISR 的定时器。此处中断已经处于禁用状态。
         */
        vPortSetupTimerInterrupt();
        /* 初始化所有内核的临界区嵌套计数。 */
        for ( uint8_t ucCoreID = 0; ucCoreID < configNUMBER_OF_CORES; ucCoreID++ )
        {
            ulCriticalNestings[ ucCoreID ] = 0;
        }
        /* 通知主核已完成所有必要的初始化。 */
        ucPrimaryCoreInitDoneFlag = 1;
        /* 唤醒从核。 */
        BaseType_t xWakeResult = configWAKE_SECONDARY_CORES();
        configASSERT( xWakeResult == pdTRUE );

        /*
         * 在此处阻塞主核，直到所有从核就绪；仅当 ucSecondaryCoresReadyFlags 的
         * 所有元素都已置位时才会继续。
         */
        while( 1 )
        {
            BaseType_t xAllCoresReady = pdTRUE;
            for( uint8_t ucCoreID = 0; ucCoreID < ( configNUMBER_OF_CORES - 1 ); ucCoreID++ )
            {
                if( ucSecondaryCoresReadyFlags[ ucCoreID ] != pdTRUE )
                {
                    xAllCoresReady = pdFALSE;
                    break;
                }
                }

            if ( xAllCoresReady == pdTRUE )
            {
                break;
            }
        }
    #else /* 条件：if ( configNUMBER_OF_CORES > 1 ) */
        /* 启动产生 tick 中断的定时器。 */
        vPortSetupTimerInterrupt();
        /* 初始化临界区嵌套计数，为第一个任务做好准备。 */
        ulCriticalNesting = 0;
    #endif /* 条件：if ( configNUMBER_OF_CORES > 1 ) */

    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )
    {
        xSchedulerRunning = pdTRUE;
    }
    #endif /* ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )（启用 MPU 且使用 V2 包装器） */

    /* 启动第一个任务。 */
    vStartFirstTask();

    /*
     * 任务此时已经开始执行，不应到达这里。调用任务退出错误函数，
     * 以避免在应用程序编写者通过定义 configTASK_RETURN_ADDRESS
     * 覆盖此功能时出现静态函数未被调用的编译器警告。调用
     * vTaskSwitchContext() 可防止链接时优化移除该符号。
     */
    #if ( configNUMBER_OF_CORES > 1 )
        vTaskSwitchContext( portGET_CORE_ID() );
    #else
        vTaskSwitchContext();
    #endif
    prvTaskExitError();

    /* 不应到达这里。 */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    /*
     * 在无处可返回的移植层中未实现。
     * 人为触发断言。
     */
    #if ( configNUMBER_OF_CORES == 1 )
        configASSERT( ulCriticalNesting == 1000UL );
    #else /* 条件：if ( configNUMBER_OF_CORES == 1 ) */
        configASSERT( ulCriticalNestings[ portGET_CORE_ID() ] == 1000UL );
    #endif /* 条件：if ( configNUMBER_OF_CORES == 1 ) */
}
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    void vPortStoreTaskMPUSettings( xMPU_SETTINGS * xMPUSettings,
                                    const struct xMEMORY_REGION * const xRegions,
                                    StackType_t * pxBottomOfStack,
                                    configSTACK_DEPTH_TYPE uxStackDepth )
    {
        uint32_t ulRegionStartAddress, ulRegionEndAddress, ulRegionNumber;
        int32_t lIndex = 0;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非由链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_sram_start__;
            extern uint32_t * __privileged_sram_end__;
        #else
            /* 当这些变量由链接脚本导出时的声明。 */
            extern uint32_t __privileged_sram_start__[];
            extern uint32_t __privileged_sram_end__[];
        #endif /* __ARMCC_VERSION 定义条件 */

        /* 配置 MAIR0。 */
        xMPUSettings->ulMAIR0 = ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_ATTR0_POS ) & portMPU_MAIR_ATTR0_MASK );
        xMPUSettings->ulMAIR0 |= ( ( portMPU_DEVICE_MEMORY_nGnRE << portMPU_MAIR_ATTR1_POS ) & portMPU_MAIR_ATTR1_MASK );

        /*
         * 任务创建时会自动调用该函数，此时栈区域参数有效。其他情况下栈参数
         * 无效，并假定栈区域已经配置完毕。
         */
        if( uxStackDepth > 0 )
        {
            ulRegionStartAddress = ( uint32_t ) pxBottomOfStack;
            ulRegionEndAddress = ( uint32_t ) pxBottomOfStack + ( uxStackDepth * ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t ) ) - 1;

            /*
             * 若栈位于特权 SRAM 内，则不要使用单独的 MPU 区域保护它。这是因为
             * 特权 SRAM 已由某个 MPU 区域保护，而 ARMv8-M 不允许 MPU 区域重叠。
             */
            if( ( ulRegionStartAddress >= ( uint32_t ) __privileged_sram_start__ ) &&
                ( ulRegionEndAddress <= ( uint32_t ) __privileged_sram_end__ ) )
            {
                xMPUSettings->xRegionsSettings[ 0 ].ulRBAR = 0;
                xMPUSettings->xRegionsSettings[ 0 ].ulRLAR = 0;
            }
            else
            {
                /* 定义允许访问栈的区域。 */
                ulRegionStartAddress &= portMPU_RBAR_ADDRESS_MASK;
                ulRegionEndAddress &= portMPU_RLAR_ADDRESS_MASK;

                xMPUSettings->xRegionsSettings[ 0 ].ulRBAR = ( ulRegionStartAddress ) |
                                                             ( portMPU_REGION_NON_SHAREABLE ) |
                                                             ( portMPU_REGION_READ_WRITE ) |
                                                             ( portMPU_REGION_EXECUTE_NEVER );

                xMPUSettings->xRegionsSettings[ 0 ].ulRLAR = ( ulRegionEndAddress ) |
                                                             ( portMPU_RLAR_ATTR_INDEX0 ) |
                                                             ( portMPU_RLAR_REGION_ENABLE );
            }
        }

        /* 用户提供的可配置区域。 */
        for( ulRegionNumber = 1; ulRegionNumber <= portNUM_CONFIGURABLE_REGIONS; ulRegionNumber++ )
        {
            /*
             * 若 xRegions 为 NULL，即任务未指定任何 MPU 区域，则 else 分支会
             * 确保所有可配置 MPU 区域都被置为无效。
             */
            if( ( xRegions != NULL ) && ( xRegions[ lIndex ].ulLengthInBytes > 0UL ) )
            {
                /*
                 * 将 xRegions 中的通用区域定义转换为 ARMv8 专用的 MPU 设置，
                 * 然后存入 xMPUSettings。
                 */
                ulRegionStartAddress = ( ( uint32_t ) xRegions[ lIndex ].pvBaseAddress ) & portMPU_RBAR_ADDRESS_MASK;
                ulRegionEndAddress = ( uint32_t ) xRegions[ lIndex ].pvBaseAddress + xRegions[ lIndex ].ulLengthInBytes - 1;
                ulRegionEndAddress &= portMPU_RLAR_ADDRESS_MASK;

                /* 起始地址。 */
                xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRBAR = ( ulRegionStartAddress ) |
                                                                          ( portMPU_REGION_NON_SHAREABLE );

                /* 只读/读写（RO/RW）。 */
                if( ( xRegions[ lIndex ].ulParameters & tskMPU_REGION_READ_ONLY ) != 0 )
                {
                    xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRBAR |= ( portMPU_REGION_READ_ONLY );
                }
                else
                {
                    xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRBAR |= ( portMPU_REGION_READ_WRITE );
                }

                /* XN（执行禁止）。 */
                if( ( xRegions[ lIndex ].ulParameters & tskMPU_REGION_EXECUTE_NEVER ) != 0 )
                {
                    xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRBAR |= ( portMPU_REGION_EXECUTE_NEVER );
                }

                /* 结束地址。 */
                xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR = ( ulRegionEndAddress ) |
                                                                          ( portMPU_RLAR_REGION_ENABLE );

                /* PXN（特权执行禁止）。 */
                #if ( portARMV8M_MINOR_VERSION >= 1 )
                {
                    if( ( xRegions[ lIndex ].ulParameters & tskMPU_REGION_PRIVILEGED_EXECUTE_NEVER ) != 0 )
                    {
                        xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR |= ( portMPU_RLAR_PRIVILEGED_EXECUTE_NEVER );
                    }
                }
                #endif /* portARMV8M_MINOR_VERSION >= 1（ARMv8-M 次版本号大于等于 1） */

                /* 普通内存/设备内存。 */
                if( ( xRegions[ lIndex ].ulParameters & tskMPU_REGION_DEVICE_MEMORY ) != 0 )
                {
                    /* MAIR0 中的 Attr1 配置为设备内存。 */
                    xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR |= portMPU_RLAR_ATTR_INDEX1;
                }
                else
                {
                    /* MAIR0 中的 Attr0 配置为普通内存。 */
                    xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR |= portMPU_RLAR_ATTR_INDEX0;
                }
            }
            else
            {
                /* 使该区域失效。 */
                xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRBAR = 0UL;
                xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR = 0UL;
            }

            lIndex++;
        }
    }

#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    BaseType_t xPortIsAuthorizedToAccessBuffer( const void * pvBuffer,
                                                uint32_t ulBufferLength,
                                                uint32_t ulAccessRequested ) /* PRIVILEGED_FUNCTION 特权函数标记 */

    {
        uint32_t i, ulBufferStartAddress, ulBufferEndAddress;
        BaseType_t xAccessGranted = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( xSchedulerRunning == pdFALSE )
        {
            /*
             * 在调度器启动前授予对所有内核对象的访问权限。这是必要的，
             * 因为此时尚无任务运行，无法使用任何任务的权限。
             */
            xAccessGranted = pdTRUE;
        }
        else if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xAccessGranted = pdTRUE;
        }
        else
        {
            if( portADD_UINT32_WILL_OVERFLOW( ( ( uint32_t ) pvBuffer ), ( ulBufferLength - 1UL ) ) == pdFALSE )
            {
                ulBufferStartAddress = ( uint32_t ) pvBuffer;
                ulBufferEndAddress = ( ( ( uint32_t ) pvBuffer ) + ulBufferLength - 1UL );

                for( i = 0; i < portTOTAL_NUM_REGIONS; i++ )
                {
                    /* 该 MPU 区域是否已使能？ */
                    if( ( xTaskMpuSettings->xRegionsSettings[ i ].ulRLAR & portMPU_RLAR_REGION_ENABLE ) == portMPU_RLAR_REGION_ENABLE )
                    {
                        if( portIS_ADDRESS_WITHIN_RANGE( ulBufferStartAddress,
                                                         portEXTRACT_FIRST_ADDRESS_FROM_RBAR( xTaskMpuSettings->xRegionsSettings[ i ].ulRBAR ),
                                                         portEXTRACT_LAST_ADDRESS_FROM_RLAR( xTaskMpuSettings->xRegionsSettings[ i ].ulRLAR ) ) &&
                            portIS_ADDRESS_WITHIN_RANGE( ulBufferEndAddress,
                                                         portEXTRACT_FIRST_ADDRESS_FROM_RBAR( xTaskMpuSettings->xRegionsSettings[ i ].ulRBAR ),
                                                         portEXTRACT_LAST_ADDRESS_FROM_RLAR( xTaskMpuSettings->xRegionsSettings[ i ].ulRLAR ) ) &&
                            portIS_AUTHORIZED( ulAccessRequested,
                                               prvGetRegionAccessPermissions( xTaskMpuSettings->xRegionsSettings[ i ].ulRBAR ) ) )
                        {
                            xAccessGranted = pdTRUE;
                            break;
                        }
                    }
                }
            }
        }

        return xAccessGranted;
    }

#endif /* #if ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（启用 MPU 且使用 V2 包装器） */
/*-----------------------------------------------------------*/

BaseType_t xPortIsInsideInterrupt( void )
{
    uint32_t ulCurrentInterrupt;
    BaseType_t xReturn;

    /*
     * 获取当前正在执行的中断编号。中断程序状态寄存器（IPSR）保存
     * 当前正在执行的异常编号；处于线程模式时为零。
     */
    __asm volatile ( "mrs %0, ipsr" : "=r" ( ulCurrentInterrupt )::"memory" );

    if( ulCurrentInterrupt == 0 )
    {
        xReturn = pdFALSE;
    }
    else
    {
        xReturn = pdTRUE;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

#if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )

    void vPortValidateInterruptPriority( void )
    {
        uint32_t ulCurrentInterrupt;
        uint8_t ucCurrentPriority;

        /* 获取当前正在执行的中断编号。 */
        __asm volatile ( "mrs %0, ipsr" : "=r" ( ulCurrentInterrupt )::"memory" );

        /* 该中断编号是否为用户定义中断？ */
        if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
        {
            /* 查找该中断的优先级。 */
            ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

            /*
             * 若某个中断的 ISR 所分配的优先级高于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，却调用了 ISR 安全的
             * FreeRTOS API 函数，则下面的断言将失败。ISR 安全的 FreeRTOS
             * API 函数*只能*从优先级等于或低于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
             *
             * 数值较小的中断优先级编号表示逻辑上较高的中断优先级，
             * 因此中断的优先级必须设置为等于或数值*大于*
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
             *
             * 使用 FreeRTOS API 的中断不得保持默认优先级 0，因为那是
             * 可能的最高优先级，必然高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，
             * 因此也必然无效。
             *
             * FreeRTOS 同时提供独立的任务级和 ISR API 函数，以确保
             * 中断入口尽可能快速简单。
             *
             * 以下链接提供详细信息：
             * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
             * https://www.freertos.org/Why-FreeRTOS/FAQs
             */
            configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
        }

        /*
         * 优先级分组：中断控制器（NVIC）允许将定义每个中断优先级的位
         * 拆分为定义抢占优先级的位和定义子优先级的位。为简单起见，
         * 所有位都必须定义为抢占优先级位。若不符合该要求
         * （即某些位表示子优先级），下面的断言将失败。
         *
         * 若应用程序仅使用 CMSIS 库配置中断，则可在启动调度器前
         * 调用 NVIC_SetPriorityGrouping( 0 )，在所有 Cortex-M 器件上
         * 获得正确设置。但请注意，某些厂商专用外设库假定优先级分组
         * 设置为非零值，此时使用零值将导致不可预测的行为。
         */
        configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
    }

#endif /* #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )（已定义 configASSERT 且具有 ARMv8-M 主扩展） */
/*-----------------------------------------------------------*/

void vPortConfigureInterruptPriorities( void ) /* PRIVILEGED_FUNCTION 特权函数标记 */
{
    #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )
    {
        volatile uint32_t ulImplementedPrioBits = 0;
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定可调用 ISR 安全 FreeRTOS API 函数的最大优先级。ISR 安全函数
         * 是以“FromISR”结尾的函数。FreeRTOS 分别维护线程和 ISR 两套 API，
         * 以确保中断进入尽可能快速、简单。
         *
         * 首先确定可用的优先级位数。向 SVCall 的优先级设置写入所有可能的位。
         */
        portNVIC_SHPR2_REG = 0xFF000000;

        /* 读回该值，查看有多少位保持置位。 */
        ucMaxPriorityValue = ( uint8_t ) ( ( portNVIC_SHPR2_REG & 0xFF000000 ) >> 24 );

        /* 对最大系统调用优先级使用相同的掩码。 */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /*
         * 检查在考虑硬件支持的优先级位数后，最大系统调用优先级
         * 不为零。优先级 0 无效，因为将 BASEPRI 寄存器设置为 0
         * 会解除所有中断的屏蔽，而优先级为 0 的中断无法使用
         * BASEPRI 屏蔽。
         * 参见 https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        configASSERT( ucMaxSysCallPriority );

        /*
         * 检查 configMAX_SYSCALL_INTERRUPT_PRIORITY 中
         * 硬件未实现的位是否为零。
         */
        configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY & ( uint8_t ) ( ~( uint32_t ) ucMaxPriorityValue ) ) == 0U );

        /*
         * 根据读回的位数计算可接受的最大优先级分组值。
         */
        while( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
        {
            ulImplementedPrioBits++;
            ucMaxPriorityValue <<= ( uint8_t ) 0x01;
        }

        if( ulImplementedPrioBits == 8 )
        {
            /*
             * 当硬件实现 8 个优先级位时，软件无法将 PRIGROUP 配置为
             * 不含子优先级。因此最低有效位始终用于子优先级，
             * 共有 128 个抢占优先级和 2 个子优先级。
             *
             * 这在某些情况下可能引起混淆——例如，若
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 设置为 5，则优先级 5 和 4
             * 的中断都会在临界区中被屏蔽，因为它们处于相同的抢占优先级。
             * 这可能令人困惑，因为 4 的优先级高于（数值上小于）
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，按理不应被屏蔽。
             * 反之，若将 configMAX_SYSCALL_INTERRUPT_PRIORITY 设置为 4，
             * 这种混淆就不会发生，行为保持一致。
             *
             * 下面的断言确保 configMAX_SYSCALL_INTERRUPT_PRIORITY 中的
             * 子优先级位为 0，以避免上述混淆。
             */
            configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY & 0x1U ) == 0U );
            ulMaxPRIGROUPValue = 0;
        }
        else
        {
            ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS - ulImplementedPrioBits;
        }

        /*
         * 将优先级分组值移回其在 AIRCR 寄存器中的位置。
         */
        ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
        ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;
    }
    #endif /* #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )（已定义 configASSERT 且具有 ARMv8-M 主扩展） */

    /*
     * 将 PendSV 和 SysTick 设为最低优先级中断，并将 SVCall
     * 设为最高优先级。
     */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
    portNVIC_SHPR2_REG = 0;
}
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

    void vPortGrantAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                         int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
        xMPU_SETTINGS * xTaskMpuSettings;

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

        xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

        xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] |= ( 1U << ulAccessControlListEntryBit );
    }

#endif /* configENABLE_MPU == 1、configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1 */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

    void vPortRevokeAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                          int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION 特权函数标记 */
    {
        uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
        xMPU_SETTINGS * xTaskMpuSettings;

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

        xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

        xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] &= ~( 1U << ulAccessControlListEntryBit );
    }

#endif /* configENABLE_MPU == 1、configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1 */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION 特权函数标记 */
        {
            uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
            BaseType_t xAccessGranted = pdFALSE;
            const xMPU_SETTINGS * xTaskMpuSettings;

            if( xSchedulerRunning == pdFALSE )
            {
                /*
                 * 在调度器启动前授予对所有内核对象的访问权限。这是必要的，
                 * 因为此时尚无任务运行，无法使用任何任务的权限。
                 */
                xAccessGranted = pdTRUE;
            }
            else
            {
                xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

                ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
                ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

                if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
                {
                    xAccessGranted = pdTRUE;
                }
                else
                {
                    if( ( xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] & ( 1U << ulAccessControlListEntryBit ) ) != 0 )
                    {
                        xAccessGranted = pdTRUE;
                    }
                }
            }

            return xAccessGranted;
        }

    #else /* configENABLE_ACCESS_CONTROL_LIST == 1（启用访问控制列表） */

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION 特权函数标记 */
        {
            ( void ) lInternalIndexOfKernelObject;

            /*
             * 若不使用访问控制列表功能，所有任务均可访问所有内核对象。
             */
            return pdTRUE;
        }

    #endif /* configENABLE_ACCESS_CONTROL_LIST == 1（启用访问控制列表） */

#endif /* configENABLE_MPU == 1 且 configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
    #if ( configNUMBER_OF_CORES == 1 )
        static uint32_t prvConfigurePACBTI( BaseType_t xWriteControlRegister )
    #else /* configNUMBER_OF_CORES > 1（多核配置） */
        uint32_t vConfigurePACBTI( BaseType_t xWriteControlRegister )
    #endif /* configNUMBER_OF_CORES 结束 */
        {
            uint32_t ulControl = 0x0;

            /* 确认已实现 PACBTI。 */
            configASSERT( portID_ISAR5_REG != 0x0 );

            /* 使能用法故障（UsageFault）异常。 */
            portSCB_SYS_HANDLER_CTRL_STATE_REG |= portSCB_USG_FAULT_ENABLE_BIT;

            #if ( configENABLE_PAC == 1 )
            {
                ulControl |= ( portCONTROL_UPAC_EN | portCONTROL_PAC_EN );
            }
            #endif

            #if ( configENABLE_BTI == 1 )
            {
                ulControl |= ( portCONTROL_UBTI_EN | portCONTROL_BTI_EN );
            }
            #endif

            if( xWriteControlRegister == pdTRUE )
            {
                __asm volatile ( "msr control, %0" : : "r" ( ulControl ) );
            }

            return ulControl;
        }

#endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（启用 PAC 或 BTI） */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    /* 哪个内核持有该锁？ */
    PRIVILEGED_DATA volatile uint32_t ulOwnedByCore[ portMAX_CORE_COUNT ];
    /* 内核持有的锁计数。 */
    PRIVILEGED_DATA volatile uint32_t ulRecursionCountByLock[ eLockCount ];
    /* 索引 0 用于 ISR 锁，索引 1 用于任务锁。 */
    PRIVILEGED_DATA volatile uint32_t ulGateWord[ eLockCount ];

    __attribute__((weak)) void vInterruptCore( uint8_t ucCoreID )
    {
        /* 默认弱符号桩函数——平台专用实现可以覆盖。 */
        ( void ) ucCoreID;
    }

/*-----------------------------------------------------------*/

    static inline void prvSpinUnlock( volatile uint32_t * ulLock )
    {
        /* 保守解锁：为广泛的硬件/FVP 保留原有屏障。 */
        __asm volatile (
            "dmb sy         \n"
            "mov r1, #0     \n"
            "str r1, [%0]   \n"
            "sev            \n"
            "dsb            \n"
            "isb            \n"
            :
            : "r" ( ulLock )
            : "memory", "r1"
        );
    }

/*-----------------------------------------------------------*/

    static inline uint32_t prvSpinTrylock( volatile uint32_t * ulLock )
    {
        /*
         * 保守的 ldrex/strex 尝试加锁：
         * - 若忙则立即返回 1，并清除独占状态（CLREX）。
         * - 仅在观察到空闲且 strex 虚假失败时重试。
         * - 成功后执行 DMB，以保持预期的获取语义。
         */
        uint32_t ulVal;
        uint32_t ulStatus;

        __asm volatile (
            " ldrex %0, [%1]   \n"
            : "=r" ( ulVal )
            : "r" ( ulLock )
            : "memory"
        );

        if( ulVal != 0U )
        {
            __asm volatile ("clrex" ::: "memory");
            return 1U;
        }

        __asm volatile (
            " strex %0, %2, [%1]   \n"
            : "=&r" ( ulStatus )
            : "r" ( ulLock ), "r" (1U)
            : "memory"
        );

        if( ulStatus != 0U )
        {
            return 1U;
        }
        __asm volatile ( "dmb" ::: "memory" );
        return 0U;
    }


/*-----------------------------------------------------------*/

    /* 读取内核间共享的 32 位值。 */
    static inline uint32_t prvGet32( volatile uint32_t * x )
    {
        __asm( "dsb" );
        return *x;
    }

/*-----------------------------------------------------------*/

    /* 写入内核间共享的 32 位值。 */
    static inline void prvSet32( volatile uint32_t * x,
                                 uint32_t value )
    {
        *x = value;
        __asm( "dsb" );
    }

/*-----------------------------------------------------------*/

    void vPortRecursiveLock( uint8_t ucCoreID,
                             ePortRTOSLock eLockNum,
                             BaseType_t uxAcquire )
    {
        /* 校验内核 ID 和锁编号。 */
        configASSERT( ucCoreID < portMAX_CORE_COUNT );
        configASSERT( eLockNum < eLockCount );

        uint32_t ulLockBit = 1u << eLockNum;

        /* 获取锁。 */
        if( uxAcquire )
        {
            /* 检查自旋锁是否可用。 */
            /* 若自旋锁不可用，检查该内核是否持有该锁。 */
            /* 若该内核持有该锁，则将其锁计数加一。 */
            /* 若该内核不持有该锁，则等待自旋锁。 */
            if( prvSpinTrylock( &ulGateWord[ eLockNum ] ) != 0 )
            {
                /* 检查该内核是否持有自旋锁。 */
                if( prvGet32( &ulOwnedByCore[ ucCoreID ] ) & ulLockBit )
                {
                    configASSERT( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) != portUINT32_MAX );
                    prvSet32( &ulRecursionCountByLock[ eLockNum ], ( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) + 1 ) );
                    return;
                }

                /* 将门控字预加载到缓存中。 */
                uint32_t dummy = ulGateWord[ eLockNum ];
                dummy++;

                while( prvSpinTrylock( &ulGateWord[ eLockNum ] ) != 0 )
                {
                    __asm volatile ( "wfe" );
                }
            }

            /* 添加屏障，确保在继续之前已取得锁。 */
            __asm volatile( "dmb sy" ::: "memory" );

            /* 断言：自旋锁空闲且成功获取时，锁计数为 0。 */
            configASSERT( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) == 0 );

            /* 将锁计数设置为 1。 */
            prvSet32( &ulRecursionCountByLock[ eLockNum ], 1 );
            /* 设置 ulOwnedByCore（持有锁的内核）。 */
            prvSet32( &ulOwnedByCore[ ucCoreID ], ( prvGet32( &ulOwnedByCore[ ucCoreID ] ) | ulLockBit ) );
        }
        /* 释放锁。 */
        else
        {
            /* 断言：该锁尚未处于空闲状态。 */
            configASSERT( ( prvGet32( &ulOwnedByCore[ ucCoreID ] ) & ulLockBit ) != 0 );
            configASSERT( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) != 0 );

            /* 将 ulRecursionCountByLock 减 1。 */
            prvSet32( &ulRecursionCountByLock[ eLockNum ], ( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) - 1 ) );

            if( !prvGet32( &ulRecursionCountByLock[ eLockNum ] ) )
            {
                prvSet32( &ulOwnedByCore[ ucCoreID ], ( prvGet32( &ulOwnedByCore[ ucCoreID ] ) & ~ulLockBit ) );
                prvSpinUnlock( &ulGateWord[ eLockNum ] );
                /* 添加屏障，确保在继续之前锁状态已生效。 */
                __asm volatile( "dmb sy" ::: "memory" );
            }
        }
    }

/*-----------------------------------------------------------*/

    uint8_t ucPortGetCoreID( void )
    {
        return *(volatile uint8_t *)(configCORE_ID_REGISTER);
    }

/*-----------------------------------------------------------*/

#endif /* if( configNUMBER_OF_CORES > 1 )（多核配置） */
