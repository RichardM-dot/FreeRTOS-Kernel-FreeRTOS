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
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 重新定义所有 API 函数
 * 以使用 MPU 包装器。仅当从应用程序文件中包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* 调度器相关头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* MPU 相关头文件包含。 */
#include "mpu_wrappers.h"
#include "mpu_syscall_numbers.h"

/* portasm 相关头文件包含。 */
#include "portasm.h"

#if ( configENABLE_TRUSTZONE == 1 )
    /* 安全组件相关头文件包含。 */
    #include "secure_context.h"
    #include "secure_init.h"
#endif /* configENABLE_TRUSTZONE（条件编译结束） */

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/**
 * FreeRTOS Cortex-M33 移植层可配置为仅在安全侧运行，即处理器以安全状态启动，
 * 且永不跳转到非安全侧。若要在安全侧运行 FreeRTOS，必须禁止移植层中的
 * TrustZone 支持。以下是有效的配置设置：
 *
 * 1. 在安全侧运行 FreeRTOS：
 *    configRUN_FREERTOS_SECURE_ONLY = 1 且 configENABLE_TRUSTZONE = 0
 *
 * 2. 在非安全侧运行 FreeRTOS，并支持安全侧函数调用：
 *    configRUN_FREERTOS_SECURE_ONLY = 0 且 configENABLE_TRUSTZONE = 1
 *
 * 3. 仅在非安全侧运行 FreeRTOS，即不支持安全侧函数调用：
 *    configRUN_FREERTOS_SECURE_ONLY = 0 且 configENABLE_TRUSTZONE = 0
 */
#if ( ( configRUN_FREERTOS_SECURE_ONLY == 1 ) && ( configENABLE_TRUSTZONE == 1 ) )
    #error TrustZone needs to be disabled in order to run FreeRTOS on the Secure Side.
#endif

/**
 * Cortex-M23 没有非安全侧 PSPLIM。只有在安全侧运行 FreeRTOS 时，
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
 * @brief 用于检查中断优先级有效性的常量。
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
 * @brief SVC 处理程序内部时，栈中参数的偏移量。
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
     * 为所选内存区域使能特权级执行永不（PXN）MPU 属性。
     */
    #define portMPU_RLAR_PRIVILEGED_EXECUTE_NEVER    ( 1UL << 4UL )
#endif /* portARMV8M_MINOR_VERSION >= 1（条件编译结束） */

/* 使能对未映射区域的特权访问。 */
#define portMPU_PRIV_BACKGROUND_ENABLE_BIT    ( 1UL << 2UL )

/* 使能 MPU。 */
#define portMPU_ENABLE_BIT                    ( 1UL << 0UL )

/*
 * 提取编码在 RBAR（区域基地址寄存器）值中的 MPU 区域首地址。
 */
#define portEXTRACT_FIRST_ADDRESS_FROM_RBAR( rbar ) \
    ( ( rbar ) & portMPU_RBAR_ADDRESS_MASK )

/*
 * 提取编码在 RLAR（区域限地址寄存器）值中的 MPU 区域末地址。
 */
#define portEXTRACT_LAST_ADDRESS_FROM_RLAR( rlar ) \
    ( ( ( rlar ) & portMPU_RLAR_ADDRESS_MASK ) | ~portMPU_RLAR_ADDRESS_MASK )

/* addr 是否位于 [start, end] 地址范围内？ */
#define portIS_ADDRESS_WITHIN_RANGE( addr, start, end ) \
    ( ( ( addr ) >= ( start ) ) && ( ( addr ) <= ( end ) ) )

/* 可用权限是否满足该访问请求？ */
#define portIS_AUTHORIZED( accessRequest, permissions ) \
    ( ( ( permissions ) & ( accessRequest ) ) == accessRequest )

/* uint32_t 类型能容纳的最大值。 */
#define portUINT32_MAX    ( ~( ( uint32_t ) 0 ) )

/* 检查 a 与 b 相加是否会溢出。 */
#define portADD_UINT32_WILL_OVERFLOW( a, b )    ( ( a ) > ( portUINT32_MAX - ( b ) ) )
/*-----------------------------------------------------------*/

/**
 * @brief 最大 24 位数。
 *
 * 需要该值是因为 SysTick 是 24 位计数器。
 */
#define portMAX_24_BIT_NUMBER       ( 0xffffffUL )

/**
 * @brief 用于估算在无节拍空闲计算期间 SysTick 计数器停止时
 * 本应发生的 SysTick 计数的修正系数。
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
     * 位[6] - 1 --> 该异常取自安全状态。
     * 位[5] - 1 --> 不跳过附加状态上下文的压栈。
     * 位[4] - 1 --> PE 未在栈上为 FP 上下文分配空间。
     * 位[3] - 1 --> 返回到线程模式。
     * 位[2] - 1 --> 从进程栈恢复寄存器。
     * 位[1] - 0 --> 保留，0。
     * 位[0] - 1 --> 该异常进入安全状态。
     */
    #define portINITIAL_EXC_RETURN    ( 0xfffffffd )
#else

    /**
     * @brief 初始 EXC_RETURN 值。
     *
     *     FF         FF         FF         BC
     * 1111 1111  1111 1111  1111 1111  1011 1100
     *
     * 位[6] - 0 --> 该异常取自非安全状态。
     * 位[5] - 1 --> 不跳过附加状态上下文的压栈。
     * 位[4] - 1 --> PE 未在栈上为 FP 上下文分配空间。
     * 位[3] - 1 --> 返回到线程模式。
     * 位[2] - 1 --> 从进程栈恢复寄存器。
     * 位[1] - 0 --> 保留，0。
     * 位[0] - 0 --> 该异常进入非安全状态。
     */
    #define portINITIAL_EXC_RETURN    ( 0xffffffbc )
#endif /* configRUN_FREERTOS_SECURE_ONLY（条件编译结束） */

/**
 * @brief CONTROL 寄存器的特权位掩码。
 *
 * CONTROL 寄存器中的位[0]表示特权级：
 * 位[0] = 0 ==> 任务为特权级。
 * 位[0] = 1 ==> 任务为非特权级。
 */
#define portCONTROL_PRIVILEGED_MASK         ( 1UL << 0UL )

/**
 * @brief CONTROL 寄存器的初始值。
 */
#define portINITIAL_CONTROL_UNPRIVILEGED    ( 0x3 )
#define portINITIAL_CONTROL_PRIVILEGED      ( 0x2 )

/**
 * @brief 允许用户覆盖默认的 SysTick 时钟频率。若由用户定义，当配置寄存器中
 * 的 CLK 位为 0 时，该符号必须等于 SysTick 时钟频率。
 */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ             ( configCPU_CLOCK_HZ )
    /* 确保 SysTick 与内核工作在同一频率。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( portNVIC_SYSTICK_CLK_BIT )
#else
    /* 选择 SysTick 不与内核同频的时钟选项。 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( 0 )
#endif

/**
 * @brief 允许用户覆盖初始 LR 的预加载值（即 prvTaskExitError() 的地址），
 * 以防其干扰调试器中的栈回溯。
 */
#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS    configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS    prvTaskExitError
#endif

/**
 * @brief 若定义了 portPRELOAD_REGISTERS，则创建任务时会为寄存器赋予初始值。
 * 这有助于调试，但会增加代码大小。
 */
#define portPRELOAD_REGISTERS    1

/**
 * @brief 任务创建时没有安全上下文，必须先调用 portALLOCATE_SECURE_CONTEXT()
 * 为自身分配安全上下文，然后才能进行任何安全调用。
 */
#define portNO_SECURE_CONTEXT    0

/**
 * @brief 用于检查和配置 PACBTI 安全特性实现情况的常量。
 */
#if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )

    #define portID_ISAR5_REG       ( *( ( volatile uint32_t * ) 0xe000ed74 ) )

    #define portCONTROL_UPAC_EN    ( 1UL << 7UL )
    #define portCONTROL_PAC_EN     ( 1UL << 6UL )
    #define portCONTROL_UBTI_EN    ( 1UL << 5UL )
    #define portCONTROL_BTI_EN     ( 1UL << 4UL )

#endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（条件编译结束） */
/*-----------------------------------------------------------*/

/**
 * @brief 用于捕获试图从其实现函数返回的任务。
 */
static void prvTaskExitError( void );

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 从区域基地址寄存器（RBAR）值中提取 MPU 区域的访问权限。
     *
     * @param ulRBARValue 该 MPU 区域的 RBAR 值。
     *
     * @return uint32_t 访问权限。
     */
    static uint32_t prvGetRegionAccessPermissions( uint32_t ulRBARValue ) PRIVILEGED_FUNCTION;
#endif /* configENABLE_MPU == 1 && configUSE_MPU_WRAPPERS_V1 == 0（条件编译结束） */

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 配置内存保护单元（MPU）。
     */
    static void prvSetupMPU( void ) PRIVILEGED_FUNCTION;
#endif /* configENABLE_MPU（条件编译结束） */

#if ( configENABLE_FPU == 1 )

    /**
     * @brief 配置浮点单元（FPU）。
     */
    static void prvSetupFPU( void ) PRIVILEGED_FUNCTION;
#endif /* configENABLE_FPU（条件编译结束） */

#if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )

    /**
     * @brief 配置 PACBTI 特性。
     *
     * 该函数根据用户配置配置指针认证（PAC）和分支目标识别（BTI）安全特性，
     * 并相应返回专用 CONTROL 寄存器的值，可选地更新 CONTROL 寄存器的值。
     * 目前仅 Cortex-M85（基于 ARMv8.1-M 架构）目标支持 PACBTI 安全特性。
     *
     * @param xWriteControlRegister 用于控制是否更新专用 CONTROL 寄存器。
     *
     * @return 按所配置 PACBTI 选项得到的 CONTROL 寄存器值。
     */
    #if ( configNUMBER_OF_CORES == 1 )
        static uint32_t prvConfigurePACBTI( BaseType_t xWriteControlRegister );
    #else /* #if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */
        uint32_t vConfigurePACBTI( BaseType_t xWriteControlRegister );
    #endif /* #if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */

#endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（条件编译结束） */

/**
 * @brief 设置定时器以产生节拍中断。
 *
 * 本文件中的实现为弱定义，以便应用程序编写者更改用于产生节拍中断的定时器。
 */
void vPortSetupTimerInterrupt( void ) PRIVILEGED_FUNCTION;

/**
 * @brief 检查当前执行上下文是否为中断上下文。
 *
 * @return 若当前执行上下文为中断上下文则返回 pdTRUE，否则返回 pdFALSE。
 */
BaseType_t xPortIsInsideInterrupt( void );

/**
 * @brief 让出处理器。
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
     * @brief 设置系统调用栈，以便从 SVC 返回后使用系统调用栈。
     *
     * @param pulTaskStack 触发 SVC 时的当前 SP。
     * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
     * @param ucSystemCallNumber 该系统调用的系统调用编号。
     */
    void vSystemCallEnter( uint32_t * pulTaskStack,
                           uint32_t ulLR,
                           uint8_t ucSystemCallNumber ) PRIVILEGED_FUNCTION;

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 发出 SVC 以退出系统调用。
     */
    void vRequestSystemCallExit( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 设置任务栈，以便从 SVC 返回后再次使用任务栈。
     *
     * @param pulSystemCallStack 触发 SVC 时的当前 SP。
     * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
     */
    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) PRIVILEGED_FUNCTION;

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 检查调用任务是否处于特权级。
     *
     * @return 若调用任务为特权级则返回 pdTRUE，否则返回 pdFALSE。
     */
    BaseType_t xPortIsTaskPrivileged( void ) PRIVILEGED_FUNCTION;

#endif /* configENABLE_MPU == 1（条件编译结束） */

#if ( configNUMBER_OF_CORES > 1 )

    /**
     * @brief 由平台/应用程序定义、用于唤醒从核的函数。
     *
     * @return 若从核被成功唤醒则返回 pdTRUE，否则返回 pdFALSE。
     */
    extern BaseType_t configWAKE_SECONDARY_CORES( void );

#endif /* #if ( configNUMBER_OF_CORES > 1 )（条件编译结束） */

/**
 * @brief 配置中断优先级。
 */
void vPortConfigureInterruptPriorities( void ) PRIVILEGED_FUNCTION;
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    /**
     * @brief 调度器启动时，该变量被设置为 pdTRUE。
     */
    PRIVILEGED_DATA static BaseType_t xSchedulerRunning = pdFALSE;

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */

/**
 * @brief 每个任务在临界嵌套变量中维护自己的中断状态。
 */
#if ( configNUMBER_OF_CORES == 1 )
    PRIVILEGED_DATA static volatile uint32_t ulCriticalNesting = 0UL;
#else /* #if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */
    PRIVILEGED_DATA volatile uint32_t ulCriticalNestings[ configNUMBER_OF_CORES ] = { 0 };
    /* 用于检查从核是否就绪的标志。 */
    PRIVILEGED_DATA volatile uint8_t ucSecondaryCoresReadyFlags[ configNUMBER_OF_CORES - 1 ] = { 0 };
    /* 用于指示主核已完成初始化的标志。 */
    PRIVILEGED_DATA volatile uint8_t ucPrimaryCoreInitDoneFlag = 0;
 #endif /* #if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */

#if ( configENABLE_TRUSTZONE == 1 )

    /**
     * @brief 作为任务上下文的一部分保存，用于指示任务在安全侧使用的上下文。
     */
    PRIVILEGED_DATA portDONT_DISCARD volatile SecureContextHandle_t xSecureContext = portNO_SECURE_CONTEXT;
#endif /* configENABLE_TRUSTZONE（条件编译结束） */

/**
 * @brief portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 宏使用该变量，确保不会从
 * 优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用 FreeRTOS API 函数。
 */
#if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )

    static uint8_t ucMaxSysCallPriority = 0;
    static uint32_t ulMaxPRIGROUPValue = 0;
    static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * ) portNVIC_IP_REGISTERS_OFFSET_16;

#endif /* #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )（条件编译结束） */

#if ( configUSE_TICKLESS_IDLE == 1 )

    /**
     * @brief 构成一个节拍周期的 SysTick 增量数。
     */
    PRIVILEGED_DATA static uint32_t ulTimerCountsForOneTick = 0;

    /**
     * @brief 可抑制的最大节拍周期数受 SysTick 定时器 24 位分辨率的限制。
     */
    PRIVILEGED_DATA static uint32_t xMaximumPossibleSuppressedTicks = 0;

    /**
     * @brief 补偿 SysTick 停止期间流逝的 CPU 周期（仅低功耗功能）。
     */
    PRIVILEGED_DATA static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE（条件编译结束） */
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
         * 进入临界区，但不要使用 taskENTER_CRITICAL()，因为该方法会屏蔽
         * 本应退出睡眠模式的中断。
         */
        __asm volatile ( "cpsid i" ::: "memory" );
        __asm volatile ( "dsb" );
        __asm volatile ( "isb" );

        /*
         * 若有上下文切换挂起，或有任务正在等待调度器解除挂起，则放弃进入低功耗模式。
         */
        if( eTaskConfirmSleepModeStatus() == eAbortSleep )
        {
            /*
             * 重新使能中断，参见上面 cpsid 指令上方的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
        else
        {
            /*
             * 暂时停止 SysTick。SysTick 停止的时间会尽可能被补偿，但使用无节拍模式
             * 不可避免地会使内核维护的时间相对日历时间产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /*
             * 使用 SysTick 当前值寄存器确定距离下一个节拍中断剩余的 SysTick 递减次数。
             * 若当前值寄存器为零，则实际剩余 ulTimerCountsForOneTick 次递减，而不是零，
             * 因为 SysTick 在从 1 递减到 0 时请求中断。
             */
            ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;

            if( ulSysTickDecrementsLeft == 0 )
            {
                ulSysTickDecrementsLeft = ulTimerCountsForOneTick;
            }

            /*
             * 计算等待 xExpectedIdleTime 个节拍周期所需的重装载值。使用 -1 是因为该代码
             * 通常在第一个节拍周期进行到一半时执行。但若 SysTick IRQ 当前处于挂起状态，
             * 则清除该 IRQ，抑制第一个节拍，并修正重装载值以反映第二个节拍周期已经开始。
             * 预期的空闲时间始终至少为两个节拍。
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
             * 清除 SysTick 计数标志，并将计数值重新设置为零。
             */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

            /* 重新启动 SysTick。 */
            portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

            /*
             * 休眠直到有事件发生。configPRE_SLEEP_PROCESSING() 可将其参数设置为 0，
             * 以表示其实现中包含自身的等待中断或等待事件指令，因此不应再执行 wfi。
             * 但是，原始预期空闲时间变量必须保持不变，因此这里使用其副本。
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
             * 重新使能中断，以便让使 MCU 退出睡眠模式的中断立即执行。参见上面
             * cpsid 指令上方的注释。
             */
            __asm volatile ( "cpsie i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 再次禁止中断，因为时钟即将停止，而时钟停止期间执行的中断会加大
             * RTOS 维护时间与日历时间之间的偏差。
             */
            __asm volatile ( "cpsid i" ::: "memory" );
            __asm volatile ( "dsb" );
            __asm volatile ( "isb" );

            /*
             * 禁止 SysTick 时钟时不要读取 portNVIC_SYSTICK_CTRL_REG 寄存器，以确保
             * portNVIC_SYSTICK_COUNT_FLAG_BIT 若已置位则不会被清除。同样，SysTick
             * 停止的时间会尽可能被补偿，但使用无节拍模式不可避免地会使内核维护的
             * 时间相对日历时间产生微小漂移。
             */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /* 确定 SysTick 是否已经计数到零。 */
            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                uint32_t ulCalculatedLoadValue;

                /*
                 * 节拍中断结束了睡眠（或当前处于挂起状态），并且新的节拍周期已经开始。
                 * 将 portNVIC_SYSTICK_LOAD_REG 重置为新节拍周期的剩余值。
                 */
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

                /*
                 * 不允许过小的值，也不允许因睡眠后钩子执行耗时过长或 SysTick
                 * 当前值寄存器为零而导致下溢的值。
                 */
                if( ( ulCalculatedLoadValue <= ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
                {
                    ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
                }

                portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

                /*
                 * 由于挂起的节拍将在本函数退出后立即得到处理，内核维护的节拍值应
                 * 向前推进等待时间减一。
                 */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /* 除节拍中断之外的其他事件结束了睡眠。 */

                /*
                 * 使用 SysTick 当前值寄存器确定距离预期空闲时间结束剩余的
                 * SysTick 递减次数。
                 */
                ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;
                #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG != portNVIC_SYSTICK_CLK_BIT )
                {
                    /*
                     * 若 SysTick 未使用内核时钟，此处当前值寄存器可能仍为零。这种情况下，
                     * SysTick 未从重装载寄存器加载，预期空闲时间内还剩 ulReloadValue 次递减，
                     * 而不是零。
                     */
                    if( ulSysTickDecrementsLeft == 0 )
                    {
                        ulSysTickDecrementsLeft = ulReloadValue;
                    }
                }
                #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG（条件编译结束） */

                /*
                 * 计算睡眠持续时间，并舍入为完整的节拍周期（而不是按部分节拍
                 * 计算的 ulReload 值）。
                 */
                ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - ulSysTickDecrementsLeft;

                /*
                 * 处理器等待期间经过了多少个完整节拍周期？
                 */
                ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

                /*
                 * 重装载值被设置为单个节拍周期剩余的部分。
                 */
                portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
            }

            /*
             * 重新启动 SysTick，使其再次从 portNVIC_SYSTICK_LOAD_REG 运行，然后将
             * portNVIC_SYSTICK_LOAD_REG 恢复为标准值。若 SysTick 未使用内核时钟，
             * 则临时将其配置为使用内核时钟。该配置强制 SysTick 立即从
             * portNVIC_SYSTICK_LOAD_REG 加载，而不是等到另一个时钟的下一个周期。
             * 这样 portNVIC_SYSTICK_LOAD_REG 就能立即接收标准值。
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
                 * 临时使用内核时钟已完成其使命（如上所述）。恢复使用另一个时钟。
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
            #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG（条件编译结束） */

            /* 推进节拍，以补偿已经过的节拍周期。 */
            vTaskStepTick( ulCompleteTickPeriods );

            /* 在中断使能状态下退出。 */
            __asm volatile ( "cpsie i" ::: "memory" );
        }
    }

#endif /* configUSE_TICKLESS_IDLE（条件编译结束） */
/*-----------------------------------------------------------*/

__attribute__( ( weak ) ) void vPortSetupTimerInterrupt( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /* 计算配置节拍中断所需的常量。 */
    #if ( configUSE_TICKLESS_IDLE == 1 )
    {
        ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
        xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
        ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
    }
    #endif /* configUSE_TICKLESS_IDLE（条件编译结束） */

    /*
     * 停止并复位 SysTick。
     *
     * 版本低于 7.0.0 的 QEMU 存在一个缺陷：若在未先选择有效时钟源的情况下
     * 使能 SysTick，就会触发错误。若在使能 SysTick 的同时，将时钟源从
     * 零时钟周期的时钟切换到非零时钟周期的时钟，就会触发该缺陷。因此，
     * 此处先配置 CLKSOURCE 位，再设置 ENABLE 位。该变通方法可避免
     * 版本低于 7.0.0 的 QEMU 中的该缺陷。
     */
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* 配置 SysTick 以按请求的速率产生中断。 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0UL;

    /*
     * 实现任务的函数不得退出或试图返回到调用者，因为没有可返回的位置。若任务
     * 想要退出，应改为调用 vTaskDelete( NULL )。若定义了 configASSERT()，则人为
     * 触发 assert()，然后在此停止，以便应用程序编写者捕获该错误。
     */
    #if ( configNUMBER_OF_CORES == 1 )
        configASSERT( ulCriticalNesting == ~0UL );
    #else /* #if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */
        configASSERT( ulCriticalNestings[ portGET_CORE_ID() ] == ~0UL );
    #endif /* #if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */
    portDISABLE_INTERRUPTS();

    while( ulDummy == 0 )
    {
        /*
         * 本文件在调度器启动后调用 prvTaskExitError()，以消除“函数已定义但从未调用”
         * 的编译警告。ulDummy 纯粹用于抑制有关该函数之后出现代码的其他警告：
         * 将 ulDummy 声明为 volatile 可使编译器认为该函数可能返回，从而不会对其
         * 后面的代码发出“不可达代码”警告。
         */
    }
}
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    static uint32_t prvGetRegionAccessPermissions( uint32_t ulRBARValue ) /* PRIVILEGED_FUNCTION（特权函数） */
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

#endif /* configENABLE_MPU == 1 && configUSE_MPU_WRAPPERS_V1 == 0（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    static void prvSetupMPU( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非从链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;
            extern uint32_t * __unprivileged_flash_start__;
            extern uint32_t * __unprivileged_flash_end__;
            extern uint32_t * __privileged_sram_start__;
            extern uint32_t * __privileged_sram_end__;
        #else /* if defined( __ARMCC_VERSION )（条件编译结束） */
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];
            extern uint32_t __unprivileged_flash_start__[];
            extern uint32_t __unprivileged_flash_end__[];
            extern uint32_t __privileged_sram_start__[];
            extern uint32_t __privileged_sram_end__[];
        #endif /* defined( __ARMCC_VERSION )（条件编译结束） */

        /* 仅允许 8 或 16 个区域。 */
        configASSERT( ( configTOTAL_MPU_REGIONS == 8 ) || ( configTOTAL_MPU_REGIONS == 16 ) );

        /* MAIR0 - 索引 0。 */
        portMPU_MAIR0_REG |= ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_ATTR0_POS ) & portMPU_MAIR_ATTR0_MASK );
        /* MAIR0 - 索引 1。 */
        portMPU_MAIR0_REG |= ( ( portMPU_DEVICE_MEMORY_nGnRE << portMPU_MAIR_ATTR1_POS ) & portMPU_MAIR_ATTR1_MASK );

        /*
         * 将特权级 Flash 设置为只读，使特权任务可以读取但不能修改。
         */
        portMPU_RNR_REG = portPRIVILEGED_FLASH_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __privileged_functions_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_PRIVILEGED_READ_ONLY );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __privileged_functions_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /*
         * 将非特权级 Flash 设置为特权级和非特权级任务均可只读。
         * 所有任务都可读取，但任何人都不能修改。
         */
        portMPU_RNR_REG = portUNPRIVILEGED_FLASH_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __unprivileged_flash_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_READ_ONLY );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __unprivileged_flash_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /*
         * 将非特权级系统调用 Flash 设置为特权级和非特权级任务均可只读。
         * 所有任务都可读取，但任何人都不能修改。
         */
        portMPU_RNR_REG = portUNPRIVILEGED_SYSCALLS_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __syscalls_flash_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_READ_ONLY );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __syscalls_flash_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /* 将包含内核数据的 RAM 设置为仅特权级可访问。 */
        portMPU_RNR_REG = portPRIVILEGED_RAM_REGION;
        portMPU_RBAR_REG = ( ( ( uint32_t ) __privileged_sram_start__ ) & portMPU_RBAR_ADDRESS_MASK ) |
                           ( portMPU_REGION_NON_SHAREABLE ) |
                           ( portMPU_REGION_PRIVILEGED_READ_WRITE ) |
                           ( portMPU_REGION_EXECUTE_NEVER );
        portMPU_RLAR_REG = ( ( ( uint32_t ) __privileged_sram_end__ ) & portMPU_RLAR_ADDRESS_MASK ) |
                           ( portMPU_RLAR_ATTR_INDEX0 ) |
                           ( portMPU_RLAR_REGION_ENABLE );

        /* 使能内存故障异常。 */
        portSCB_SYS_HANDLER_CTRL_STATE_REG |= portSCB_MEM_FAULT_ENABLE_BIT;

        /*
         * 使能 MPU，并启用特权级后台访问，即未映射区域具有特权级访问权限。
         */
        portMPU_CTRL_REG |= ( portMPU_PRIV_BACKGROUND_ENABLE_BIT | portMPU_ENABLE_BIT );
    }

#endif /* configENABLE_MPU（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( configENABLE_FPU == 1 )

    static void prvSetupFPU( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        #if ( configENABLE_TRUSTZONE == 1 )
        {
            /* 使能对 FPU 的非安全访问。 */
            SecureInit_EnableNSFPUAccess();
        }
        #endif /* configENABLE_TRUSTZONE（条件编译结束） */

        /*
         * CP10 = 11 ==> 完全访问 FPU，即特权级和非特权级代码都应能够访问 FPU。
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

#endif /* configENABLE_FPU（条件编译结束） */
/*-----------------------------------------------------------*/

void vPortYield( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /* 设置 PendSV 以请求上下文切换。 */
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;

    /*
     * 通常不需要屏障指令，但可确保代码完全符合该架构规定的行为。
     */
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" );
}
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES == 1 )
    void vPortEnterCritical( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        portDISABLE_INTERRUPTS();
        ulCriticalNesting++;
        /*
         * 通常不需要屏障指令，但可确保代码完全符合该架构规定的行为。
         */
        __asm volatile ( "dsb" ::: "memory" );
        __asm volatile ( "isb" );
    }
    /*-----------------------------------------------------------*/

    void vPortExitCritical( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        configASSERT( ulCriticalNesting );
        ulCriticalNesting--;

        if( ulCriticalNesting == 0 )
        {
            portENABLE_INTERRUPTS();
        }
    }
#endif /* configNUMBER_OF_CORES == 1（条件编译结束） */
/*-----------------------------------------------------------*/

void SysTick_Handler( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    uint32_t ulPreviousMask;

    ulPreviousMask = portSET_INTERRUPT_MASK_FROM_ISR();
    #if ( configNUMBER_OF_CORES > 1 )
        UBaseType_t uxSavedInterruptStatus = portENTER_CRITICAL_FROM_ISR();
    #endif /* if ( configNUMBER_OF_CORES > 1 )（条件编译结束） */

    traceISR_ENTER();
    {
        /* 递增 RTOS 节拍。 */
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
    #endif /* if ( configNUMBER_OF_CORES > 1 )（条件编译结束） */

    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulPreviousMask );
}
/*-----------------------------------------------------------*/

void vPortSVCHandler_C( uint32_t * pulCallerStackAddress ) /* PRIVILEGED_FUNCTION portDONT_DISCARD（特权函数，禁止丢弃） */
{
    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 ) )
        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而非从链接脚本导出时的声明。
             */
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];
        #endif /* defined( __ARMCC_VERSION )（条件编译结束） */
    #endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 )（条件编译结束） */

    #if ( configENABLE_MPU == 1 )
        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而非从链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
        #endif /* defined( __ARMCC_VERSION )（条件编译结束） */
    #endif /* configENABLE_MPU == 1（条件编译结束） */

    uint32_t ulPC;

    #if ( configENABLE_TRUSTZONE == 1 )
        uint32_t ulR0, ulR1;
        extern TaskHandle_t pxCurrentTCB;
        #if ( configENABLE_MPU == 1 )
            uint32_t ulControl, ulIsTaskPrivileged;
        #endif /* configENABLE_MPU（条件编译结束） */
    #endif /* configENABLE_TRUSTZONE（条件编译结束） */
    uint8_t ucSVCNumber;

    /*
     * 寄存器按以下顺序保存在栈上：R0、R1、R2、R3、R12、LR、PC、xPSR。
     */
    ulPC = pulCallerStackAddress[ portOFFSET_TO_PC ];
    ucSVCNumber = ( ( uint8_t * ) ulPC )[ -2 ];

    switch( ucSVCNumber )
    {
    #if ( configENABLE_TRUSTZONE == 1 )
        case portSVC_ALLOCATE_SECURE_CONTEXT:

            /*
             * R0 包含作为参数传递给 vPortAllocateSecureContext 函数的栈大小。
             */
            ulR0 = pulCallerStackAddress[ 0 ];

            #if ( configENABLE_MPU == 1 )
            {
                /* 读取 CONTROL 寄存器的值。 */
                __asm volatile ( "mrs %0, control"  : "=r" ( ulControl ) );

                /*
                 * 若 CONTROL 寄存器中的位[0]为 0，则触发 SVC 的任务为特权级。
                 */
                ulIsTaskPrivileged = ( ( ulControl & portCONTROL_PRIVILEGED_MASK ) == 0 );

                /* 为安全任务分配并加载上下文。 */
                xSecureContext = SecureContext_AllocateContext( ulR0, ulIsTaskPrivileged, pxCurrentTCB );
            }
            #else /* if ( configENABLE_MPU == 1 )（条件编译结束） */
            {
                /* 为安全任务分配并加载上下文。 */
                xSecureContext = SecureContext_AllocateContext( ulR0, pxCurrentTCB );
            }
            #endif /* configENABLE_MPU（条件编译结束） */

            configASSERT( xSecureContext != securecontextINVALID_CONTEXT_ID );
            SecureContext_LoadContext( xSecureContext, pxCurrentTCB );
            break;

        case portSVC_FREE_SECURE_CONTEXT:
            #if ( configENABLE_MPU == 1 )
            if( ( ulPC >= ( uint32_t ) __privileged_functions_start__ ) &&
                ( ulPC <= ( uint32_t ) __privileged_functions_end__ ) )
            {
            #endif /* configENABLE_MPU（条件编译结束） */
                /*
                 * R0 包含正在释放的 TCB，R1 包含待释放的安全上下文句柄。
                 */
                ulR0 = pulCallerStackAddress[ 0 ];
                ulR1 = pulCallerStackAddress[ 1 ];

                /* 释放安全上下文。 */
                SecureContext_FreeContext( ( SecureContextHandle_t ) ulR1, ( void * ) ulR0 );
            #if ( configENABLE_MPU == 1 )
            }
            #endif /* configENABLE_MPU（条件编译结束） */
            break;
    #endif /* configENABLE_TRUSTZONE（条件编译结束） */

        case portSVC_START_SCHEDULER:
            #if ( configENABLE_MPU == 1 )
            if( ( ulPC >= ( uint32_t ) __privileged_functions_start__ ) &&
                ( ulPC <= ( uint32_t ) __privileged_functions_end__ ) )
            {
            #endif /* configENABLE_MPU（条件编译结束） */
                #if ( configENABLE_TRUSTZONE == 1 )
                {
                    /*
                     * 降低非安全异常的优先级，使非安全侧 PendSV 以最低优先级运行。
                     */
                    SecureInit_DePrioritizeNSExceptions();

                    /* 初始化安全上下文管理系统。 */
                    SecureContext_Init();
                }
                #endif /* configENABLE_TRUSTZONE（条件编译结束） */

                #if ( configENABLE_FPU == 1 )
                {
                    /* 配置浮点单元（FPU）。 */
                    prvSetupFPU();
                }
                #endif /* configENABLE_FPU（条件编译结束） */

                /*
                 * 设置第一个任务的上下文，使第一个任务开始执行。
                 */
                vRestoreContextOfFirstTask();
            #if ( configENABLE_MPU == 1 )
            }
            #endif /* configENABLE_MPU（条件编译结束） */
            break;

    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 ) )
        case portSVC_RAISE_PRIVILEGE:

            /*
             * 仅当 SVC 是由任一系统调用触发时，才提升特权级。
             */
            if( ( ulPC >= ( uint32_t ) __syscalls_flash_start__ ) &&
                ( ulPC <= ( uint32_t ) __syscalls_flash_end__ ) )
            {
                vRaisePrivilege();
            }
            break;
    #endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 1 )（条件编译结束） */

    #if ( configENABLE_MPU == 1 )
        case portSVC_YIELD:
            vPortYield();
            break;
    #endif /* configENABLE_MPU == 1（条件编译结束） */

        default:
            /* 不正确的 SVC 调用。 */
            configASSERT( pdFALSE );
    }
}
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vSystemCallEnter( uint32_t * pulTaskStack,
                           uint32_t ulLR,
                           uint8_t ucSystemCallNumber ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        extern TaskHandle_t pxCurrentTCB;
        extern UBaseType_t uxSystemCallImplementations[ NUM_SYSTEM_CALLS ];
        xMPU_SETTINGS * pxMpuSettings;
        uint32_t * pulSystemCallStack;
        uint32_t ulHardwareSavedExceptionFrameSize, ulSystemCallLocation, i;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非从链接脚本导出时的声明。
             */
            extern uint32_t * __syscalls_flash_start__;
            extern uint32_t * __syscalls_flash_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint32_t __syscalls_flash_start__[];
            extern uint32_t __syscalls_flash_end__[];
        #endif /* #if defined( __ARMCC_VERSION )（条件编译结束） */

        ulSystemCallLocation = pulTaskStack[ portOFFSET_TO_PC ];
        pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );

        /*
         * 检查：
         * 1. SVC 是从系统调用段触发的（即应用程序没有直接触发 SVC）。
         * 2. pxMpuSettings->xSystemCallStackInfo.pulTaskStack 必须为 NULL，因为仅在
         *    系统调用执行期间（即系统调用进入与退出之间）它才非 NULL。
         * 3. 系统调用不是针对 FreeRTOSConfig.h 配置禁用的内核 API。
         * 4. 无需检查 ucSystemCallNumber 是否在范围内，因为汇编 SVC 处理程序
         *    在调用本函数之前已进行检查。
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
            #else /* if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )（条件编译结束） */
            {
                ulHardwareSavedExceptionFrameSize = 8;
            }
            #endif /* configENABLE_FPU || configENABLE_MVE（条件编译结束） */

            /* 在系统调用栈上为栈帧留出空间。 */
            pulSystemCallStack = pulSystemCallStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulSystemCallStack[ i ] = pulTaskStack[ i ];
            }

            /*
             * 保存触发 SVC 之前的链接寄存器值。它包含系统调用入口点调用者
             * （即 MPU_<API> 的调用者）的地址。退出系统调用时需要恢复该值。
             */
            pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry = pulTaskStack[ portOFFSET_TO_LR ];

            /*
             * 保存触发 SVC 之前的 PSPLIM 寄存器值。退出系统调用时需要恢复该值。
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

            /* 从该处理程序返回后开始执行系统调用。 */
            pulSystemCallStack[ portOFFSET_TO_PC ] = uxSystemCallImplementations[ ucSystemCallNumber ];

            /*
             * 在系统调用完成时发出退出系统调用的请求。
             */
            pulSystemCallStack[ portOFFSET_TO_LR ] = ( uint32_t ) vRequestSystemCallExit;

            /*
             * 记录退出系统调用时应复制栈帧的位置。
             */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = pulTaskStack + ulHardwareSavedExceptionFrameSize;

            /*
             * 记录硬件是否使用填充来强制栈指针双字对齐。
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
             * 在 pxPortInitialiseStack 中已确保系统调用栈双字对齐，因此无需填充。
             * 清除已压栈 xPSR 的位[9]。
             */
            pulSystemCallStack[ portOFFSET_TO_PSR ] &= ( ~portPSR_STACK_PADDING_MASK );

            /* 在系统调用期间提升特权级。 */
            __asm volatile (
                " mrs r0, control     \n" /* 获取当前 control 值。 */
                " movs r1, #1         \n" /* r1 = 1（置位掩码）。 */
                " bics r0, r1         \n" /* 清除 nPRIV 位。 */
                " msr control, r0     \n" /* 写回新的 control 值。 */
                ::: "r0", "r1", "memory"
                );
        }
    }

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vRequestSystemCallExit( void ) /* __attribute__( ( naked ) ) PRIVILEGED_FUNCTION（裸函数、特权函数） */
    {
        __asm volatile ( "svc %0 \n" ::"i" ( portSVC_SYSTEM_CALL_EXIT ) : "memory" );
    }

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        extern TaskHandle_t pxCurrentTCB;
        xMPU_SETTINGS * pxMpuSettings;
        uint32_t * pulTaskStack;
        uint32_t ulHardwareSavedExceptionFrameSize, ulSystemCallLocation, i;

        #if defined( __ARMCC_VERSION )

            /*
             * 当这些变量在代码中定义而非从链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_functions_start__;
            extern uint32_t * __privileged_functions_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint32_t __privileged_functions_start__[];
            extern uint32_t __privileged_functions_end__[];
        #endif /* #if defined( __ARMCC_VERSION )（条件编译结束） */

        ulSystemCallLocation = pulSystemCallStack[ portOFFSET_TO_PC ];
        pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );

        /*
         * 检查：
         * 1. SVC 是从特权级代码触发的（即应用程序没有直接触发 SVC）。该 SVC
         *    仅从位于特权级代码段中的 vRequestSystemCallExit 触发。
         * 2. pxMpuSettings->xSystemCallStackInfo.pulTaskStack 必须非 NULL，这表明
         *    此前已进入系统调用，应用程序不会在未进入系统调用的情况下尝试退出。
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
            #else /* if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )（条件编译结束） */
            {
                ulHardwareSavedExceptionFrameSize = 8;
            }
            #endif /* configENABLE_FPU || configENABLE_MVE（条件编译结束） */

            /* 在任务栈上为栈帧留出空间。 */
            pulTaskStack = pulTaskStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulTaskStack[ i ] = pulSystemCallStack[ i ];
            }

            /* 在线程模式下使用 pulTaskStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulTaskStack ) );

            /*
             * 返回到系统调用入口点的调用者（即 MPU_<API> 的调用者）。
             */
            pulTaskStack[ portOFFSET_TO_PC ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;
            /* 确保 LR 具有有效值。 */
            pulTaskStack[ portOFFSET_TO_LR ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;

            /*
             * 将 PSPLIM 寄存器恢复为系统调用进入时的值。
             */
            #if ( portUSE_PSPLIM_REGISTER == 1 )
            {
                __asm volatile ( "msr psplim, %0" : : "r" ( pxMpuSettings->xSystemCallStackInfo.ulStackLimitRegisterAtSystemCallEntry ) );
            }
            #endif

            /*
             * 若硬件使用填充来强制栈指针双字对齐，则置位已压栈 xPSR 的位[9]，
             * 否则清除该位。
             */
            if( ( pxMpuSettings->ulTaskFlags & portSTACK_FRAME_HAS_PADDING_FLAG ) == portSTACK_FRAME_HAS_PADDING_FLAG )
            {
                pulTaskStack[ portOFFSET_TO_PSR ] |= portPSR_STACK_PADDING_MASK;
            }
            else
            {
                pulTaskStack[ portOFFSET_TO_PSR ] &= ( ~portPSR_STACK_PADDING_MASK );
            }

            /* 仅在系统调用期间非 NULL。 */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = NULL;

            /* 在返回线程模式之前降低特权级。 */
            __asm volatile (
                " mrs r0, control     \n" /* 获取当前 control 值。 */
                " movs r1, #1         \n" /* r1 = 1（置位掩码）。 */
                " orrs r0, r1         \n" /* 设置 nPRIV 位。 */
                " msr control, r0     \n" /* 写回新的 control 值。 */
                ::: "r0", "r1", "memory"
                );
        }
    }

#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    BaseType_t xPortIsTaskPrivileged( void ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        BaseType_t xTaskIsPrivileged = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xTaskIsPrivileged = pdTRUE;
        }

        return xTaskIsPrivileged;
    }

#endif /* configENABLE_MPU == 1（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         StackType_t * pxEndOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters,
                                         BaseType_t xRunPrivileged,
                                         xMPU_SETTINGS * xMPUSettings ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        uint32_t ulIndex = 0;
        uint32_t ulControl = 0x0;

        xMPUSettings->ulContext[ ulIndex ] = 0x04040404; /* r4（保存 r4）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x05050505; /* r5（保存 r5）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x06060606; /* r6（保存 r6）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x07070707; /* r7（保存 r7）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x08080808; /* r8（保存 r8）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x09090909; /* r9（保存 r9）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x10101010; /* r10（保存 r10）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x11111111; /* r11（保存 r11）。 */
        ulIndex++;

        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) pvParameters; /* r0（保存 r0）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x01010101; /* r1（保存 r1）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x02020202; /* r2（保存 r2）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x03030303; /* r3（保存 r3）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = 0x12121212; /* r12（保存 r12）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) portTASK_RETURN_ADDRESS; /* LR（链接寄存器）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) pxCode; /* PC（程序计数器）。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = portINITIAL_XPSR; /* xPSR（程序状态寄存器）。 */
        ulIndex++;

        #if ( configENABLE_TRUSTZONE == 1 )
        {
            xMPUSettings->ulContext[ ulIndex ] = portNO_SECURE_CONTEXT; /* xSecureContext（安全上下文）。 */
            ulIndex++;
        }
        #endif /* configENABLE_TRUSTZONE（条件编译结束） */
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) ( pxTopOfStack - 8 ); /* 保存硬件栈帧的 PSP。 */
        ulIndex++;
        xMPUSettings->ulContext[ ulIndex ] = ( uint32_t ) pxEndOfStack; /* PSPLIM（进程栈限）。 */
        ulIndex++;

        #if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
        {
            /*
             * 在将 CONTROL 寄存器的值压入任务 TCB 之前，检查 PACBTI 安全特性的配置。
             */
            #if ( configNUMBER_OF_CORES == 1 )
                ulControl = prvConfigurePACBTI( pdFALSE );
            #else /* configNUMBER_OF_CORES > 1（条件编译结束） */
                ulControl = vConfigurePACBTI( pdFALSE );
            #endif /* configNUMBER_OF_CORES（条件编译结束） */
        }
        #endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（条件编译结束） */

        if( xRunPrivileged == pdTRUE )
        {
            xMPUSettings->ulTaskFlags |= portTASK_IS_PRIVILEGED_FLAG;
            xMPUSettings->ulContext[ ulIndex ] = ( ulControl | ( uint32_t ) portINITIAL_CONTROL_PRIVILEGED ); /* CONTROL（控制寄存器）。 */
            ulIndex++;
        }
        else
        {
            xMPUSettings->ulTaskFlags &= ( ~portTASK_IS_PRIVILEGED_FLAG );
            xMPUSettings->ulContext[ ulIndex ] = ( ulControl | ( uint32_t ) portINITIAL_CONTROL_UNPRIVILEGED ); /* CONTROL（控制寄存器）。 */
            ulIndex++;
        }

        xMPUSettings->ulContext[ ulIndex ] = portINITIAL_EXC_RETURN; /* LR（EXC_RETURN，异常返回信息）。 */
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

            /* 仅在系统调用期间非 NULL。 */
            xMPUSettings->xSystemCallStackInfo.pulTaskStack = NULL;
        }
        #endif /* configUSE_MPU_WRAPPERS_V1 == 0（条件编译结束） */

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
        #endif /* configENABLE_PAC（条件编译结束） */

        return &( xMPUSettings->ulContext[ ulIndex ] );
    }

#else /* configENABLE_MPU（条件编译结束） */

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         StackType_t * pxEndOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        /*
         * 模拟上下文切换中断所创建的栈帧。
         */
        #if ( portPRELOAD_REGISTERS == 0 )
        {
            pxTopOfStack--; /* 为补偿 MCU 在中断进入/退出时使用栈的方式而增加的偏移量。 */
            *pxTopOfStack = portINITIAL_XPSR; /* xPSR（程序状态寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxCode; /* PC（程序计数器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR（链接寄存器）。 */
            pxTopOfStack -= 5; /* R12、R3、R2 和 R1。 */
            *pxTopOfStack = ( StackType_t ) pvParameters; /* R0（参数/返回值寄存器）。 */
            pxTopOfStack -= 9; /* R11..R4、EXC_RETURN（被调用者保存寄存器及异常返回信息）。 */
            *pxTopOfStack = portINITIAL_EXC_RETURN;
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxEndOfStack; /* 用于保存该任务 PSPLIM 值的槽位。 */

            #if ( configENABLE_TRUSTZONE == 1 )
            {
                pxTopOfStack--;
                *pxTopOfStack = portNO_SECURE_CONTEXT; /* 用于保存该任务 xSecureContext 值的槽位。 */
            }
            #endif /* configENABLE_TRUSTZONE（条件编译结束） */
        }
        #else /* portPRELOAD_REGISTERS（条件编译结束） */
        {
            pxTopOfStack--; /* 为补偿 MCU 在中断进入/退出时使用栈的方式而增加的偏移量。 */
            *pxTopOfStack = portINITIAL_XPSR; /* xPSR（程序状态寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxCode; /* PC（程序计数器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR（链接寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x12121212UL; /* R12（内部过程调用临时寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x03030303UL; /* R3（参数寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x02020202UL; /* R2（参数寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x01010101UL; /* R1（参数寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pvParameters; /* R0（参数寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x11111111UL; /* R11（帧指针/被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x10101010UL; /* R10（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x09090909UL; /* R09（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x08080808UL; /* R08（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x07070707UL; /* R07（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x06060606UL; /* R06（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x05050505UL; /* R05（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) 0x04040404UL; /* R04（被调用者保存寄存器）。 */
            pxTopOfStack--;
            *pxTopOfStack = portINITIAL_EXC_RETURN; /* EXC_RETURN（异常返回信息）。 */
            pxTopOfStack--;
            *pxTopOfStack = ( StackType_t ) pxEndOfStack; /* 用于保存该任务 PSPLIM 值的槽位。 */

            #if ( configENABLE_TRUSTZONE == 1 )
            {
                pxTopOfStack--;
                *pxTopOfStack = portNO_SECURE_CONTEXT; /* 用于保存该任务 xSecureContext 值的槽位。 */
            }
            #endif /* configENABLE_TRUSTZONE（条件编译结束） */
        }
        #endif /* portPRELOAD_REGISTERS（条件编译结束） */

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
        #endif /* configENABLE_PAC（条件编译结束） */

        return pxTopOfStack;
    }

#endif /* configENABLE_MPU（条件编译结束） */
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /*
     * 应用程序可以通过以下方式之一安装 FreeRTOS 中断处理程序：
     * 1. 直接路由 - 分别为 SVCall 和 PendSV 中断安装 SVC_Handler 和 PendSV_Handler 函数。
     * 2. 间接路由 - 为 SVCall 和 PendSV 中断安装单独的处理程序，并将程序控制
     *    从这些处理程序路由到 SVC_Handler 和 PendSV_Handler 函数。
     *
     * 使用间接路由的应用程序必须在其 FreeRTOSConfig.h 中将
     * configCHECK_HANDLER_INSTALLATION 设置为 0。当
     * configCHECK_HANDLER_INSTALLATION 为 1 时，此处会校验直接路由；
     * 在可能的情况下应优先使用直接路由。
     */
    #if ( configCHECK_HANDLER_INSTALLATION == 1 )
    {
        const portISR_t * const pxVectorTable = portSCB_VTOR_REG;

        /*
         * 校验应用程序已正确安装 SVCall 和 PendSV 中断的 FreeRTOS 处理程序。
         * 我们不检查 SysTick 处理程序的安装情况，因为应用程序可能通过覆盖弱函数
         * vPortSetupTimerInterrupt() 使用 SysTick 定时器之外的定时器来驱动 RTOS 节拍。
         *
         * 此处的断言失败表示 FreeRTOS 处理程序安装不正确。有关安装 FreeRTOS
         * 处理程序的帮助，请参见 https://www.freertos.org/Why-FreeRTOS/FAQs。
         *
         * 对于中断向量表地址可配置的系统，若 VTOR 未正确设置指向应用程序的
         * 向量表，此处也可能遇到断言失败甚至系统故障。
         */
        configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == SVC_Handler );
        configASSERT( pxVectorTable[ portVECTOR_INDEX_PENDSV ] == PendSV_Handler );
    }
    #endif /* configCHECK_HANDLER_INSTALLATION（条件编译结束） */

    vPortConfigureInterruptPriorities();

    #if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
    {
        /*
         * 在启动第一个任务之前，根据 PACBTI 安全特性配置设置 CONTROL 寄存器的值。
         */
        #if ( configNUMBER_OF_CORES == 1 )
            ( void ) prvConfigurePACBTI( pdTRUE );
        #else /* configNUMBER_OF_CORES > 1（条件编译结束） */
            ( void ) vConfigurePACBTI( pdTRUE );
        #endif /* configNUMBER_OF_CORES（条件编译结束） */
    }
    #endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（条件编译结束） */

    #if ( configENABLE_MPU == 1 )
    {
        /* 配置内存保护单元（MPU）。 */
        prvSetupMPU();
    }
    #endif /* configENABLE_MPU（条件编译结束） */

    #if ( configNUMBER_OF_CORES > 1 )
        /*
         * 启动产生节拍 ISR 的定时器。此处中断已处于禁止状态。
         */
        vPortSetupTimerInterrupt();
        /* 初始化所有内核的临界嵌套计数。 */
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
         * 在此处保持主核，直到所有从核就绪；仅当 ucSecondaryCoresReadyFlags
         * 的所有元素都已置位时才会满足该条件。
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
    #else /* if ( configNUMBER_OF_CORES > 1 )（条件编译结束） */
        /* 启动产生节拍 ISR 的定时器。 */
        vPortSetupTimerInterrupt();
        /* 初始化临界嵌套计数，为第一个任务做好准备。 */
        ulCriticalNesting = 0;
    #endif /* if ( configNUMBER_OF_CORES > 1 )（条件编译结束） */

    #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )
    {
        xSchedulerRunning = pdTRUE;
    }
    #endif /* ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )（条件编译结束） */

    /* 启动第一个任务。 */
    vStartFirstTask();

    /*
     * 不应到达此处，因为任务此时已开始执行。调用任务退出错误函数，以避免
     * 应用程序编写者通过定义 configTASK_RETURN_ADDRESS 覆盖该功能时，
     * 编译器对未调用的静态函数发出警告。调用 vTaskSwitchContext()，
     * 以免链接时优化移除该符号。
     */
    #if ( configNUMBER_OF_CORES > 1 )
        vTaskSwitchContext( portGET_CORE_ID() );
    #else
        vTaskSwitchContext();
    #endif
    prvTaskExitError();

    /* 不应到达此处。 */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    /*
     * 在无可返回位置的移植层中未实现。人为触发断言。
     */
    #if ( configNUMBER_OF_CORES == 1 )
        configASSERT( ulCriticalNesting == 1000UL );
    #else /* if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */
        configASSERT( ulCriticalNestings[ portGET_CORE_ID() ] == 1000UL );
    #endif /* if ( configNUMBER_OF_CORES == 1 )（条件编译结束） */
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
             * 当这些变量在代码中定义而非从链接脚本导出时的声明。
             */
            extern uint32_t * __privileged_sram_start__;
            extern uint32_t * __privileged_sram_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint32_t __privileged_sram_start__[];
            extern uint32_t __privileged_sram_end__[];
        #endif /* defined( __ARMCC_VERSION )（条件编译结束） */

        /* 配置 MAIR0。 */
        xMPUSettings->ulMAIR0 = ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_ATTR0_POS ) & portMPU_MAIR_ATTR0_MASK );
        xMPUSettings->ulMAIR0 |= ( ( portMPU_DEVICE_MEMORY_nGnRE << portMPU_MAIR_ATTR1_POS ) & portMPU_MAIR_ATTR1_MASK );

        /*
         * 当任务创建时会自动调用该函数，此时栈区域参数有效。在其他所有时候，
         * 栈参数无效，并假定栈区域已经配置好。
         */
        if( uxStackDepth > 0 )
        {
            ulRegionStartAddress = ( uint32_t ) pxBottomOfStack;
            ulRegionEndAddress = ( uint32_t ) pxBottomOfStack + ( uxStackDepth * ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t ) ) - 1;

            /*
             * 若栈位于特权级 SRAM 内，则不要使用单独的 MPU 区域保护它。这是必要的，
             * 因为特权级 SRAM 已使用一个 MPU 区域保护，而 ARMv8-M 不允许 MPU 区域重叠。
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
             * 若 xRegions 为 NULL，即任务未指定任何 MPU 区域，则 else 分支确保
             * 所有可配置 MPU 区域都失效。
             */
            if( ( xRegions != NULL ) && ( xRegions[ lIndex ].ulLengthInBytes > 0UL ) )
            {
                /*
                 * 将 xRegions 中包含的通用区域定义转换为 ARMv8 专用的 MPU 设置，
                 * 然后存入 xMPUSettings。
                 */
                ulRegionStartAddress = ( ( uint32_t ) xRegions[ lIndex ].pvBaseAddress ) & portMPU_RBAR_ADDRESS_MASK;
                ulRegionEndAddress = ( uint32_t ) xRegions[ lIndex ].pvBaseAddress + xRegions[ lIndex ].ulLengthInBytes - 1;
                ulRegionEndAddress &= portMPU_RLAR_ADDRESS_MASK;

                /* 起始地址。 */
                xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRBAR = ( ulRegionStartAddress ) |
                                                                          ( portMPU_REGION_NON_SHAREABLE );

                /* 只读/读写。 */
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

                /* PXN（特权级执行禁止）。 */
                #if ( portARMV8M_MINOR_VERSION >= 1 )
                {
                    if( ( xRegions[ lIndex ].ulParameters & tskMPU_REGION_PRIVILEGED_EXECUTE_NEVER ) != 0 )
                    {
                        xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR |= ( portMPU_RLAR_PRIVILEGED_EXECUTE_NEVER );
                    }
                }
                #endif /* portARMV8M_MINOR_VERSION >= 1（条件编译结束） */

                /* 普通内存/设备内存。 */
                if( ( xRegions[ lIndex ].ulParameters & tskMPU_REGION_DEVICE_MEMORY ) != 0 )
                {
                    /* MAIR0 中的 Attr1 被配置为设备内存。 */
                    xMPUSettings->xRegionsSettings[ ulRegionNumber ].ulRLAR |= portMPU_RLAR_ATTR_INDEX1;
                }
                else
                {
                    /* MAIR0 中的 Attr0 被配置为普通内存。 */
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

#endif /* configENABLE_MPU（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    BaseType_t xPortIsAuthorizedToAccessBuffer( const void * pvBuffer,
                                                uint32_t ulBufferLength,
                                                uint32_t ulAccessRequested ) /* PRIVILEGED_FUNCTION（特权函数） */

    {
        uint32_t i, ulBufferStartAddress, ulBufferEndAddress;
        BaseType_t xAccessGranted = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( xSchedulerRunning == pdFALSE )
        {
            /*
             * 在调度器启动之前授予对所有内核对象的访问权限。这是必要的，因为此时
             * 没有任务在运行，因此无法使用任何任务的权限。
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
                    /* 该 MPU 区域是否使能？ */
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

#endif /* #if ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 )（条件编译结束） */
/*-----------------------------------------------------------*/

BaseType_t xPortIsInsideInterrupt( void )
{
    uint32_t ulCurrentInterrupt;
    BaseType_t xReturn;

    /*
     * 获取当前正在执行的中断编号。中断程序状态寄存器（IPSR）保存当前正在执行的
     * 异常编号，线程模式下为零。
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

        /* 该中断编号是否为用户定义的中断？ */
        if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
        {
            /* 查询该中断的优先级。 */
            ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

            /*
             * 以下断言会在以下情况失败：为某个中断分配了高于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的优先级，而该中断的服务程序（ISR）
             * 调用了中断安全的 FreeRTOS API 函数。中断安全的 FreeRTOS API 函数必须*仅*
             * 从优先级等于或低于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
             *
             * 数值上较小的中断优先级编号表示逻辑上较高的中断优先级，因此中断的优先级
             * 必须设置为等于或数值上*大于* configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
             *
             * 使用 FreeRTOS API 的中断不得保持在默认的零优先级，因为零是可能的最高
             * 优先级，必然高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，因此也必然无效。
             *
             * FreeRTOS 维护独立的线程和 ISR API 函数，以确保中断进入尽可能快速和简单。
             *
             * 以下链接提供详细信息：
             * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
             * https://www.freertos.org/Why-FreeRTOS/FAQs
             */
            configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
        }

        /*
         * 优先级分组：中断控制器（NVIC）允许将定义每个中断优先级的位划分为
         * 定义中断抢占优先级的位和定义中断子优先级的位。为简单起见，所有位
         * 都必须定义为抢占优先级位。若不是这样（即某些位表示子优先级），
         * 以下断言将失败。
         *
         * 若应用程序仅使用 CMSIS 库进行中断配置，则在所有 Cortex-M 器件上，
         * 都可以在启动调度器之前调用 NVIC_SetPriorityGrouping( 0 ); 来获得正确设置。
         * 但请注意，某些厂商特定的外设库假定优先级分组设置为非零值，此时使用
         * 零值会导致不可预测的行为。
         */
        configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
    }

#endif /* #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )（条件编译结束） */
/*-----------------------------------------------------------*/

void vPortConfigureInterruptPriorities( void ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )
    {
        volatile uint32_t ulImplementedPrioBits = 0;
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定可从中调用中断安全 FreeRTOS API 函数的最高优先级。中断安全函数
         * 是指以 "FromISR" 结尾的函数。FreeRTOS 维护独立的线程和 ISR API 函数，
         * 以确保中断进入尽可能快速和简单。
         *
         * 首先确定可用的优先级位数。向 SVCall 优先级设置中的所有可能位写入 1。
         */
        portNVIC_SHPR2_REG = 0xFF000000;

        /* 读回该值，以查看有多少位被保留。 */
        ucMaxPriorityValue = ( uint8_t ) ( ( portNVIC_SHPR2_REG & 0xFF000000 ) >> 24 );

        /* 对最大系统调用优先级使用相同的掩码。 */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /*
         * 在考虑硬件支持的优先级位数之后，检查最大系统调用优先级非零。
         * 优先级 0 无效，因为将 BASEPRI 寄存器设置为 0 会解除所有中断的屏蔽，
         * 而优先级为 0 的中断无法使用 BASEPRI 屏蔽。
         * 参见 https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        configASSERT( ucMaxSysCallPriority );

        /*
         * 检查 configMAX_SYSCALL_INTERRUPT_PRIORITY 中硬件未实现的位为零。
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
             * 当硬件实现 8 个优先级位时，软件无法将 PRIGROUP 配置为没有子优先级。
             * 因此，最低有效位始终用于子优先级，从而有 128 个抢占优先级和
             * 2 个子优先级。
             *
             * 这在某些情况下可能引起混淆——例如，若 configMAX_SYSCALL_INTERRUPT_PRIORITY
             * 设置为 5，则优先级为 5 和 4 的中断都会在临界区中被屏蔽，因为它们处于
             * 相同的抢占优先级。这看起来可能令人困惑，因为 4 的优先级高于（数值上低于）
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，本不应被屏蔽。相反，若将
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 设置为 4，则不会出现这种混淆，
             * 且行为保持不变。
             *
             * 以下断言确保 configMAX_SYSCALL_INTERRUPT_PRIORITY 中的子优先级位为 0，
             * 以避免上述混淆。
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
    #endif /* #if ( ( configASSERT_DEFINED == 1 ) && ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) )（条件编译结束） */

    /*
     * 将 PendSV 和 SysTick 设为最低优先级中断，并将 SVCall 设为最高优先级。
     */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
    portNVIC_SHPR2_REG = 0;
}
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

    void vPortGrantAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                         int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
        xMPU_SETTINGS * xTaskMpuSettings;

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

        xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

        xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] |= ( 1U << ulAccessControlListEntryBit );
    }

#endif /* #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

    void vPortRevokeAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                          int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
    {
        uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
        xMPU_SETTINGS * xTaskMpuSettings;

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

        xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

        xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] &= ~( 1U << ulAccessControlListEntryBit );
    }

#endif /* #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
        {
            uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
            BaseType_t xAccessGranted = pdFALSE;
            const xMPU_SETTINGS * xTaskMpuSettings;

            if( xSchedulerRunning == pdFALSE )
            {
                /*
                 * 在调度器启动之前授予对所有内核对象的访问权限。这是必要的，因为此时
                 * 没有任务在运行，因此无法使用任何任务的权限。
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

    #else /* #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )（条件编译结束） */

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
        {
            ( void ) lInternalIndexOfKernelObject;

            /*
             * 若不使用访问控制列表特性，则所有任务都可以访问所有内核对象。
             */
            return pdTRUE;
        }

    #endif /* #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )（条件编译结束） */

#endif /* #if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 == 0 ) )（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
    #if ( configNUMBER_OF_CORES == 1 )
        static uint32_t prvConfigurePACBTI( BaseType_t xWriteControlRegister )
    #else /* configNUMBER_OF_CORES > 1（条件编译结束） */
        uint32_t vConfigurePACBTI( BaseType_t xWriteControlRegister )
    #endif /* configNUMBER_OF_CORES（条件编译结束） */
        {
            uint32_t ulControl = 0x0;

            /* 确保已实现 PACBTI。 */
            configASSERT( portID_ISAR5_REG != 0x0 );

            /* 使能 UsageFault 异常。 */
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

#endif /* configENABLE_PAC == 1 || configENABLE_BTI == 1（条件编译结束） */
/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    /* 哪个内核拥有该锁？ */
    PRIVILEGED_DATA volatile uint32_t ulOwnedByCore[ portMAX_CORE_COUNT ];
    /* 内核持有的锁计数。 */
    PRIVILEGED_DATA volatile uint32_t ulRecursionCountByLock[ eLockCount ];
    /* 索引 0 用于 ISR 锁，索引 1 用于任务锁。 */
    PRIVILEGED_DATA volatile uint32_t ulGateWord[ eLockCount ];

    __attribute__((weak)) void vInterruptCore( uint8_t ucCoreID )
    {
        /* 默认弱桩函数，平台特定的实现可以覆盖。 */
        ( void ) ucCoreID;
    }

/*-----------------------------------------------------------*/

    static inline void prvSpinUnlock( volatile uint32_t * ulLock )
    {
        /* 保守解锁：为广泛的硬件/FVP 保留原有的屏障。 */
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
         * - 若忙，立即返回 1，并清除独占状态（CLREX）。
         * - 仅在观察到空闲且 strex 虚假失败时重试。
         * - 成功时执行 DMB，以保留预期的获取语义。
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

    /* 读取内核之间共享的 32 位值。 */
    static inline uint32_t prvGet32( volatile uint32_t * x )
    {
        __asm( "dsb" );
        return *x;
    }

/*-----------------------------------------------------------*/

    /* 写入内核之间共享的 32 位值。 */
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
            /* 若自旋锁不可用，检查该内核是否拥有该锁。 */
            /* 若该内核拥有该锁，则等待并将该内核的锁计数递增。 */
            /* 若该内核不拥有该锁，则等待自旋锁。 */
            if( prvSpinTrylock( &ulGateWord[ eLockNum ] ) != 0 )
            {
                /* 检查该内核是否拥有自旋锁。 */
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

            /* 断言自旋锁空闲并被获取时锁计数为 0。 */
            configASSERT( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) == 0 );

            /* 将锁计数设置为 1。 */
            prvSet32( &ulRecursionCountByLock[ eLockNum ], 1 );
            /* 设置 ulOwnedByCore。 */
            prvSet32( &ulOwnedByCore[ ucCoreID ], ( prvGet32( &ulOwnedByCore[ ucCoreID ] ) | ulLockBit ) );
        }
        /* 释放锁。 */
        else
        {
            /* 断言该锁尚未处于空闲状态。 */
            configASSERT( ( prvGet32( &ulOwnedByCore[ ucCoreID ] ) & ulLockBit ) != 0 );
            configASSERT( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) != 0 );

            /* 将 ulRecursionCountByLock 减 1。 */
            prvSet32( &ulRecursionCountByLock[ eLockNum ], ( prvGet32( &ulRecursionCountByLock[ eLockNum ] ) - 1 ) );

            if( !prvGet32( &ulRecursionCountByLock[ eLockNum ] ) )
            {
                prvSet32( &ulOwnedByCore[ ucCoreID ], ( prvGet32( &ulOwnedByCore[ ucCoreID ] ) & ~ulLockBit ) );
                prvSpinUnlock( &ulGateWord[ eLockNum ] );
                /* 添加屏障，确保在继续之前锁状态已反映。 */
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

#endif /* if( configNUMBER_OF_CORES > 1 )（条件编译结束） */
