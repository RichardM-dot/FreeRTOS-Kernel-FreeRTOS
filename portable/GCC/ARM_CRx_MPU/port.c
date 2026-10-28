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

/* 标准头文件包含。 */
#include <stdint.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有 API 函数
 * 重定义为使用 MPU 包装函数。仅当从应用文件包含 task.h 时才应这样做。
 */
#ifndef MPU_WRAPPERS_INCLUDED_FROM_API_FILE
    #define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#endif /* MPU_WRAPPERS_INCLUDED_FROM_API_FILE 条件编译结束 */

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "mpu_syscall_numbers.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* uint32_t 类型可容纳的最大值。 */
#define portUINT32_MAX    ( ~( ( uint32_t ) 0 ) )

/* 检查 a 与 b 相加是否会导致溢出。 */
#define portADD_UINT32_WILL_OVERFLOW( a, b )    ( ( a ) > ( portUINT32_MAX - ( b ) ) )
/* ----------------------------------------------------------------------------------- */

/**
 * @brief 用于跟踪临界区嵌套计数的变量。
 *
 * @ingroup Critical Sections
 *
 * 该变量作为任务上下文的一部分保存，必须初始化为非零值，以确保调度器
 * 启动前中断不会被意外使能。由于它是任务上下文的一部分，启动第一个
 * 任务时会被置为 0。
 */
PRIVILEGED_DATA volatile UBaseType_t ulCriticalNesting = 0xFFFF;

/**
 * @brief 置 1 表示从中断服务程序（ISR）中请求一次上下文切换。
 *
 * @ingroup Interrupt Management
 */
PRIVILEGED_DATA volatile UBaseType_t ulPortYieldRequired = pdFALSE;

/**
 * @brief 中断嵌套深度，用于统计需要退出的中断层数。
 *
 * @ingroup Interrupt Management
 */
PRIVILEGED_DATA volatile UBaseType_t ulPortInterruptNesting = 0UL;

/**
 * @brief 用于跟踪调度器是否已经启动的变量。
 *
 * @ingroup Scheduler
 *
 * 这是 tasks.c 中 xSchedulerRunning 的移植层专用版本。
 */
PRIVILEGED_DATA static BaseType_t prvPortSchedulerRunning = pdFALSE;

/* -------------------------- 私有函数声明 -------------------------- */

/**
 * @brief 判断给定的 MPU 区域设置是否授权对给定缓冲区执行所请求的访问。
 *
 * @ingroup Task Context
 * @ingroup MPU Control
 *
 * @param xTaskMPURegion MPU 区域设置。
 * @param ulBufferStart 给定缓冲区的起始地址。
 * @param ulBufferLength 给定缓冲区的长度。
 * @param ulAccessRequested 请求的访问类型。
 *
 * @return 若 MPU 区域设置授权对给定缓冲区执行所请求的访问则返回 pdTRUE，
 * 否则返回 pdFALSE。
 */
PRIVILEGED_FUNCTION static BaseType_t prvMPURegionAuthorizesBuffer( const xMPU_REGION_REGISTERS * xTaskMPURegion,
                                                                    const uint32_t ulBufferStart,
                                                                    const uint32_t ulBufferLength,
                                                                    const uint32_t ulAccessRequested );

/**
 * @brief 计算给定 MPU 区域大小对应的最小区域大小编码。
 *
 * @ingroup MPU Control
 *
 * @param ulActualMPURegionSize MPU 区域大小（字节）。
 *
 * @return 给定 MPU 区域大小对应的最小区域大小编码。
 */
PRIVILEGED_FUNCTION static uint32_t prvGetMPURegionSizeEncoding( uint32_t ulActualMPURegionSize );

/**
 * @brief 设置 MPU。
 *
 * @ingroup MPU Control
 */
PRIVILEGED_FUNCTION static void prvSetupMPU( void );

/* -------------------------- 导出函数声明 -------------------------- */

/**
 * @brief 进入临界区。
 *
 * @ingroup Critical Section
 */
PRIVILEGED_FUNCTION void vPortEnterCritical( void );

/**
 * @brief 退出临界区。
 *
 * @ingroup Critical Section
 */
PRIVILEGED_FUNCTION void vPortExitCritical( void );

/* ----------------------------------------------------------------------------------- */

/**
 * @brief 设置 FreeRTOS 任务的初始上下文。
 *
 * @ingroup Task Context
 *
 * @param pxTopOfStack 栈顶。
 * @param pxCode 任务函数。
 * @param pvParameters 传递给任务函数的参数。
 * @param xRunPrivileged 标记任务是否具有特权。
 * @param xMPUSettings 任务的 MPU 设置。
 *
 * @return 应从中恢复任务上下文的位置。
 */
/* 特权函数 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters,
                                     BaseType_t xRunPrivileged,
                                     xMPU_SETTINGS * xMPUSettings )
{
    /*
     * 设置任务的初始上下文。上下文的布置与 portRESTORE_CONTEXT() 宏的预期完全一致。
     */
    UBaseType_t ulIndex = CONTEXT_SIZE - 1U;

    xSYSTEM_CALL_STACK_INFO * xSysCallInfo = NULL;

    if( xRunPrivileged == pdTRUE )
    {
        xMPUSettings->ulTaskFlags |= portTASK_IS_PRIVILEGED_FLAG;
        /* 当前程序状态寄存器（CPSR）。 */
        xMPUSettings->ulContext[ ulIndex ] = SYS_MODE;
    }
    else
    {
        xMPUSettings->ulTaskFlags &= ( ~portTASK_IS_PRIVILEGED_FLAG );
        /* 当前程序状态寄存器（CPSR）。 */
        xMPUSettings->ulContext[ ulIndex ] = USER_MODE;
    }

    if( ( ( uint32_t ) pxCode & portTHUMB_MODE_ADDRESS ) != 0x0UL )
    {
        /*
         * 任务将使处理器以 THUMB 状态启动，因此置位 CPSR 中的 Thumb 状态位。
         */
        xMPUSettings->ulContext[ ulIndex ] |= portTHUMB_MODE_BIT;
    }

    ulIndex--;

    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) pxCode; /* PC 程序计数器 */
    ulIndex--;

    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) portTASK_RETURN_ADDRESS; /* LR 链接寄存器 */
    ulIndex--;

    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) pxTopOfStack; /* SP 栈指针 */
    ulIndex--;

    /* 通用寄存器。 */
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x12121212; /* R12 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x11111111; /* R11 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x10101010; /* R10 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x09090909; /* R9 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x08080808; /* R8 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x07070707; /* R7 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x06060606; /* R6 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x05050505; /* R5 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x04040404; /* R4 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x03030303; /* R3 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x02020202; /* R2 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x01010101; /* R1 通用寄存器 */
    ulIndex--;
    xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) pvParameters; /* R0 通用寄存器 */
    ulIndex--;

    #if( portENABLE_FPU == 1 )
    {
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000015; /* S31 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1500000; /* S30 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000014; /* S29 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1400000; /* S28 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000013; /* S27 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1300000; /* S26 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000012; /* S25 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1200000; /* S24 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000011; /* S23 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1100000; /* S22 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000010; /* S21 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1000000; /* S20 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000009; /* S19 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD9000000; /* S18 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000008; /* S17 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD8000000; /* S16 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000007; /* S15 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD7000000; /* S14 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000006; /* S13 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD6000000; /* S12 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000005; /* S11 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD5000000; /* S10 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000004; /* S9 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD4000000; /* S8 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000003; /* S7 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD3000000; /* S6 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000002; /* S5 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD2000000; /* S4 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000001; /* S3 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD1000000; /* S2 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000000; /* S1 浮点寄存器 */
        ulIndex--;
        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0xD0000000; /* S0 浮点寄存器 */
        ulIndex--;

        xMPUSettings->ulContext[ ulIndex ] = ( StackType_t ) 0x00000000; /* FPSR 浮点状态寄存器 */
        ulIndex--;
    }
    #endif /* portENABLE_FPU 条件编译结束 */

    /* 任务将以临界区嵌套计数 0 启动。 */
    xMPUSettings->ulContext[ ulIndex ] = portNO_CRITICAL_NESTING;

    /* 确保系统调用栈按双字对齐。 */
    xSysCallInfo = &( xMPUSettings->xSystemCallStackInfo );
    xSysCallInfo->pulSystemCallStackPointer = &( xSysCallInfo->ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE - 1U ] );
    xSysCallInfo->pulSystemCallStackPointer = ( uint32_t * ) ( ( ( uint32_t ) ( xSysCallInfo->pulSystemCallStackPointer ) ) &
                                                               ( ( uint32_t ) ( ~( portBYTE_ALIGNMENT_MASK ) ) ) );

    /* 仅在系统调用执行期间该值非 NULL。 */
    xSysCallInfo->pulTaskStackPointer = NULL;

    /* 将系统调用的返回地址设置为 vPortSystemCallExit。 */
    xSysCallInfo->pulSystemCallExitAddress = ( uint32_t * ) ( &vPortSystemCallExit );

    /* 返回应从中恢复该任务上下文的位置。 */
    return &( xMPUSettings->ulContext[ ulIndex ] );
}

/* ----------------------------------------------------------------------------------- */

/**
 * @brief 将 FreeRTOS 任务的 MPU 设置保存到其 TCB 中。
 *
 * @ingroup Task Context
 * @ingroup MPU Control
 *
 * @param xMPUSettings TCB 中的 MPU 设置。
 * @param xRegions 任务请求的更新后的 MPU 设置。
 * @param pxBottomOfStack 任务栈的基地址。
 * @param ulStackDepth 任务栈的长度。
 */
/* 特权函数 */
void vPortStoreTaskMPUSettings( xMPU_SETTINGS * xMPUSettings,
                                const struct xMEMORY_REGION * const xRegions,
                                StackType_t * pxBottomOfStack,
                                uint32_t ulStackDepth )
{
    #if defined( __ARMCC_VERSION )
        /*
         * 当这些变量在代码中定义而不是从链接脚本导出时的声明。
         */
        extern uint32_t * __SRAM_segment_start__;
        extern uint32_t * __SRAM_segment_end__;
    #else
        /* 当这些变量从链接脚本导出时的声明。 */
        extern uint32_t __SRAM_segment_start__[];
        extern uint32_t __SRAM_segment_end__[];
    #endif /* if defined( __ARMCC_VERSION ) 条件结束 */

    uint32_t ulIndex = 0x0;
    uint32_t ulRegionLength;
    uint32_t ulRegionLengthEncoded;
    uint32_t ulRegionLengthDecoded;

    if( xRegions == NULL )
    {
        /* 未指定任何 MPU 区域，因此允许访问整个 RAM。 */
        ulRegionLength = ( uint32_t ) __SRAM_segment_end__ - ( uint32_t ) __SRAM_segment_start__;
        ulRegionLengthEncoded = prvGetMPURegionSizeEncoding( ulRegionLength );
        ulRegionLength |= portMPU_REGION_ENABLE;

        /*
         * 调用该函数前，TCB 中的 MPU 设置已被清零。
         * 因此无需显式清零 xMPUSettings 中未使用的 MPU 区域。
         */
        ulIndex = portSTACK_REGION;

        xMPUSettings->xRegion[ ulIndex ].ulRegionBaseAddress = ( uint32_t ) __SRAM_segment_start__;
        xMPUSettings->xRegion[ ulIndex ].ulRegionSize = ( ulRegionLengthEncoded |
                                                          portMPU_REGION_ENABLE );
        xMPUSettings->xRegion[ ulIndex ].ulRegionAttribute = ( portMPU_REGION_PRIV_RW_USER_RW_NOEXEC |
                                                               portMPU_REGION_NORMAL_OIWTNOWA_SHARED );
    }
    else
    {
        for( ulIndex = 0UL; ulIndex < portNUM_CONFIGURABLE_REGIONS; ulIndex++ )
        {
            /* 若提供了长度，则表示该区域正在使用。 */
            if( ( xRegions[ ulIndex ] ).ulLengthInBytes > 0UL )
            {
                ulRegionLength = xRegions[ ulIndex ].ulLengthInBytes;
                ulRegionLengthEncoded = prvGetMPURegionSizeEncoding( ulRegionLength );

                /*
                 * MPU 区域基地址必须按区域大小边界对齐。
                 */
                ulRegionLengthDecoded = 2UL << ( ulRegionLengthEncoded >> 1UL );
                configASSERT( ( ( ( uint32_t ) xRegions[ ulIndex ].pvBaseAddress ) % ( ulRegionLengthDecoded ) ) == 0UL );

                xMPUSettings->xRegion[ ulIndex ].ulRegionBaseAddress = ( uint32_t ) xRegions[ ulIndex ].pvBaseAddress;
                xMPUSettings->xRegion[ ulIndex ].ulRegionSize = ( ulRegionLengthEncoded |
                                                                  portMPU_REGION_ENABLE );
                xMPUSettings->xRegion[ ulIndex ].ulRegionAttribute = xRegions[ ulIndex ].ulParameters;
            }
            else
            {
                xMPUSettings->xRegion[ ulIndex ].ulRegionBaseAddress = 0x0UL;
                xMPUSettings->xRegion[ ulIndex ].ulRegionSize = 0x0UL;
                xMPUSettings->xRegion[ ulIndex ].ulRegionAttribute = 0x0UL;
            }
        }

        /*
         * 该函数在任务创建时自动调用——此时栈区域参数有效。在其他任何时候，
         * 栈参数都无效，并假定栈区域已经配置完成。
         */
        if( ulStackDepth != 0x0UL )
        {
            ulRegionLengthEncoded = prvGetMPURegionSizeEncoding( ulStackDepth * ( uint32_t ) sizeof( StackType_t ) );

            /*
             * MPU 区域基地址必须按区域大小边界对齐。
             */
            ulRegionLengthDecoded = 2UL << ( ulRegionLengthEncoded >> 1UL );
            configASSERT( ( ( uint32_t ) pxBottomOfStack % ( ulRegionLengthDecoded ) ) == 0U );

            ulIndex = portSTACK_REGION;
            xMPUSettings->xRegion[ ulIndex ].ulRegionBaseAddress = ( uint32_t ) pxBottomOfStack;
            xMPUSettings->xRegion[ ulIndex ].ulRegionSize = ( ulRegionLengthEncoded |
                                                              portMPU_REGION_ENABLE );
            xMPUSettings->xRegion[ ulIndex ].ulRegionAttribute = ( portMPU_REGION_PRIV_RW_USER_RW_NOEXEC |
                                                                   portMPU_REGION_NORMAL_OIWTNOWA_SHARED );
        }
    }
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
BaseType_t xPortIsTaskPrivileged( void )
{
    BaseType_t xTaskIsPrivileged = pdFALSE;

    /* 调用任务的 MPU 设置。 */
    const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL );

    if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
    {
        xTaskIsPrivileged = pdTRUE;
    }

    return xTaskIsPrivileged;
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
BaseType_t xPortStartScheduler( void )
{
    /* 启动产生时钟节拍 ISR 的定时器。 */
    configSETUP_TICK_INTERRUPT();

    /* 配置所有任务共用的 MPU 区域。 */
    prvSetupMPU();

    prvPortSchedulerRunning = pdTRUE;

    /* 加载第一个任务的上下文。 */
    vPortStartFirstTask();

    /*
     * 仅当以非特权模式调用 vTaskStartScheduler()，或二进制小数点寄存器
     * 未设置为最低可能值时才会到达此处。引用 prvTaskExitError() 是为了
     * 防止在用户自定义退出地址时编译器报告“已定义但未引用”警告。
     */
    ( void ) prvTaskExitError();
    return pdFALSE;
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
static uint32_t prvGetMPURegionSizeEncoding( uint32_t ulActualMPURegionSize )
{
    uint32_t ulRegionSize, ulReturnValue = 4U;

    /* 32 字节是 Cortex-R4 和 Cortex-R5 CPU 的最小有效区域。 */
    for( ulRegionSize = 0x20UL; ulReturnValue < 0x1FUL; ( ulRegionSize <<= 1UL ) )
    {
        if( ulActualMPURegionSize <= ulRegionSize )
        {
            break;
        }
        else
        {
            ulReturnValue++;
        }
    }

    /*
     * 返回前将编码右移一位，使其能够直接写入属性寄存器的正确位位置。
     */
    return ulReturnValue << 1UL;
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
static void prvSetupMPU( void )
{
#if defined( __ARMCC_VERSION )
    /* 当这些变量在代码中定义时的声明。 */
    /* 用于 FLASH 的段。 */
    extern uint32_t * __FLASH_segment_start__;
    extern uint32_t * __FLASH_segment_end__;
    extern uint32_t * __privileged_functions_start__;
    extern uint32_t * __privileged_functions_end__;

    /* 用于 RAM 的段。 */
    extern uint32_t * __SRAM_segment_start__;
    extern uint32_t * __SRAM_segment_end__;
    extern uint32_t * __privileged_data_start__;
    extern uint32_t * __privileged_data_end__;
#else
    /* 当这些变量从链接脚本导出时的声明。 */
    /* 用于 FLASH 的段。 */
    extern uint32_t __FLASH_segment_start__[];
    extern uint32_t __FLASH_segment_end__[];
    extern uint32_t __privileged_functions_start__[];
    extern uint32_t __privileged_functions_end__[];

    /* 用于 RAM 的段。 */
    extern uint32_t __SRAM_segment_start__[];
    extern uint32_t __SRAM_segment_end__[];
    extern uint32_t __privileged_data_start__[];
    extern uint32_t __privileged_data_end__[];
#endif /* if defined( __ARMCC_VERSION ) 条件结束 */

    uint32_t ulRegionLength;
    uint32_t ulRegionLengthEncoded;

    /* 在配置 MPU 之前先禁用它。 */
    vMPUDisable();

    /* 整个 Flash：特权级 RX，非特权级 RX。 */
    ulRegionLength = ( uint32_t ) __FLASH_segment_end__ - ( uint32_t ) __FLASH_segment_start__;
    ulRegionLengthEncoded = prvGetMPURegionSizeEncoding( ulRegionLength );
    vMPUSetRegion( portUNPRIVILEGED_FLASH_REGION,
                   ( uint32_t ) __FLASH_segment_start__,
                   ( ulRegionLengthEncoded | portMPU_REGION_ENABLE ),
                   ( portMPU_REGION_PRIV_RO_USER_RO_EXEC |
                     portMPU_REGION_NORMAL_OIWTNOWA_SHARED ) );

    /* 特权函数：特权级 RX，非特权级无访问权限。 */
    ulRegionLength = ( uint32_t ) __privileged_functions_end__ - ( uint32_t ) __privileged_functions_start__;
    ulRegionLengthEncoded = prvGetMPURegionSizeEncoding( ulRegionLength );
    vMPUSetRegion( portPRIVILEGED_FLASH_REGION,
                   ( uint32_t ) __privileged_functions_start__,
                   ( ulRegionLengthEncoded | portMPU_REGION_ENABLE ),
                   ( portMPU_REGION_PRIV_RO_USER_NA_EXEC |
                     portMPU_REGION_NORMAL_OIWTNOWA_SHARED ) );

    /* 特权数据：特权级 RW，非特权级无访问权限。 */
    ulRegionLength = ( uint32_t ) __privileged_data_end__ - ( uint32_t ) __privileged_data_start__;
    ulRegionLengthEncoded = prvGetMPURegionSizeEncoding( ulRegionLength );
    vMPUSetRegion( portPRIVILEGED_RAM_REGION,
                   ( uint32_t ) __privileged_data_start__,
                   ( ulRegionLengthEncoded | portMPU_REGION_ENABLE ),
                   ( portMPU_REGION_PRIV_RW_USER_NA_NOEXEC |
                     portMPU_REGION_PRIV_RW_USER_NA_NOEXEC ) );

    /*
     * 使能 MPU 背景区域——它允许特权运行模式访问未映射的存储器区域
     * 而不会产生错误。
     */
    vMPUEnableBackgroundRegion();

    /* 设置默认区域后，使能 MPU。 */
    vMPUEnable();
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
static BaseType_t prvMPURegionAuthorizesBuffer( const xMPU_REGION_REGISTERS * xTaskMPURegion,
                                                const uint32_t ulBufferStart,
                                                const uint32_t ulBufferLength,
                                                const uint32_t ulAccessRequested )
{
    BaseType_t xAccessGranted = pdFALSE;
    uint32_t ulBufferEnd;
    uint32_t ulMPURegionLength;
    uint32_t ulMPURegionStart;
    uint32_t ulMPURegionEnd;
    uint32_t ulMPURegionAccessPermissions;

    if( portADD_UINT32_WILL_OVERFLOW( ulBufferStart, ( ulBufferLength - 1UL ) ) == pdFALSE )
    {
        ulBufferEnd = ulBufferStart + ulBufferLength - 1UL;
        ulMPURegionLength = 2UL << ( xTaskMPURegion->ulRegionSize >> 1UL );
        ulMPURegionStart = xTaskMPURegion->ulRegionBaseAddress;
        ulMPURegionEnd = xTaskMPURegion->ulRegionBaseAddress + ulMPURegionLength - 1UL;

        if( ( ulBufferStart >= ulMPURegionStart ) &&
            ( ulBufferEnd <= ulMPURegionEnd ) &&
            ( ulBufferStart <= ulBufferEnd ) )
        {
            ulMPURegionAccessPermissions = xTaskMPURegion->ulRegionAttribute & portMPU_REGION_AP_BITMASK;

            if( ulAccessRequested == tskMPU_READ_PERMISSION ) /* 只读。 */
            {
                if( ( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RW_USER_RO ) ||
                    ( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RO_USER_RO ) ||
                    ( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RW_USER_RW ) )

                {
                    xAccessGranted = pdTRUE;
                }
            }
            else if( ( ulAccessRequested & tskMPU_WRITE_PERMISSION ) != 0UL ) /* 写或读写。 */
            {
                if( ulMPURegionAccessPermissions == portMPU_REGION_PRIV_RW_USER_RW )
                {
                    xAccessGranted = pdTRUE;
                }
            }
        }
    }

    return xAccessGranted;
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
BaseType_t xPortIsAuthorizedToAccessBuffer( const void * pvBuffer,
                                            uint32_t ulBufferLength,
                                            uint32_t ulAccessRequested )
{
    BaseType_t xAccessGranted = pdFALSE;
    uint32_t ulRegionIndex;
    xMPU_SETTINGS * xTaskMPUSettings = NULL;

    if( prvPortSchedulerRunning == pdFALSE )
    {
        /*
         * 调度器启动前授予对所有存储器的访问权限。这是必要的，因为此时
         * 还没有任务在运行，因此无法使用任何任务的权限。
         */
        xAccessGranted = pdTRUE;
    }
    else
    {
        /* 调用任务的 MPU 设置。 */
        xTaskMPUSettings = xTaskGetMPUSettings( NULL );

        if( ( xTaskMPUSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            /* 特权任务可访问所有存储器。 */
            xAccessGranted = pdTRUE;
        }
        else
        {
            for( ulRegionIndex = 0x0UL; ulRegionIndex < portTOTAL_NUM_REGIONS_IN_TCB; ulRegionIndex++ )
            {
                xAccessGranted = prvMPURegionAuthorizesBuffer( &( xTaskMPUSettings->xRegion[ ulRegionIndex ] ),
                                                               ( uint32_t ) pvBuffer,
                                                               ulBufferLength,
                                                               ulAccessRequested );

                if( xAccessGranted == pdTRUE )
                {
                    break;
                }
            }
        }
    }

    return xAccessGranted;
}

/* ----------------------------------------------------------------------------------- */

#if( configENABLE_ACCESS_CONTROL_LIST == 1 )

/* 特权函数 */
BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject )
{
    uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
    BaseType_t xAccessGranted = pdFALSE;
    const xMPU_SETTINGS * xTaskMpuSettings;

    if( prvPortSchedulerRunning == pdFALSE )
    {
        /*
         * 调度器启动前授予对所有内核对象的访问权限。这是必要的，因为
         * 此时还没有任务在运行，因此无法使用任何任务的权限。
         */
        xAccessGranted = pdTRUE;
    }
    else
    {
        /* 调用任务的 MPU 设置。 */
        xTaskMpuSettings = xTaskGetMPUSettings( NULL );

        ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject
                                          / portACL_ENTRY_SIZE_BITS );
        ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject
                                        % portACL_ENTRY_SIZE_BITS );

        if( ( xTaskMpuSettings->ulTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xAccessGranted = pdTRUE;
        }
        else
        {
            if( ( ( xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] ) &
                  ( 1U << ulAccessControlListEntryBit ) ) != 0UL )
            {
                xAccessGranted = pdTRUE;
            }
        }
    }

    return xAccessGranted;
}

#else

/* 特权函数 */
BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject )
{
    ( void ) lInternalIndexOfKernelObject;

    /*
     * 若未使用访问控制列表功能，则所有任务都可访问所有内核对象。
     */
    return pdTRUE;
}

#endif /* configENABLE_ACCESS_CONTROL_LIST == 1 条件编译结束 */

/* ----------------------------------------------------------------------------------- */

#if( configENABLE_ACCESS_CONTROL_LIST == 1 )

/* 特权函数 */
void vPortGrantAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                     int32_t lInternalIndexOfKernelObject )
{
    uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
    xMPU_SETTINGS * xTaskMpuSettings;

    ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject
                                      / portACL_ENTRY_SIZE_BITS );
    ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject
                                    % portACL_ENTRY_SIZE_BITS );

    xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

    xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] |= ( 1U << ulAccessControlListEntryBit );
}

#endif /* configENABLE_ACCESS_CONTROL_LIST == 1 条件编译结束 */

/* ----------------------------------------------------------------------------------- */

#if( configENABLE_ACCESS_CONTROL_LIST == 1 )

/* 特权函数 */
void vPortRevokeAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                      int32_t lInternalIndexOfKernelObject )
{
    uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
    xMPU_SETTINGS * xTaskMpuSettings;

    ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject
                                      / portACL_ENTRY_SIZE_BITS );
    ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject
                                    % portACL_ENTRY_SIZE_BITS );

    xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

    xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] &= ~( 1U << ulAccessControlListEntryBit );
}

#endif /* configENABLE_ACCESS_CONTROL_LIST == 1 条件编译结束 */

/* ----------------------------------------------------------------------------------- */

void prvTaskExitError( void )
{
    /*
     * 实现任务的函数不得退出，也不得试图返回调用者，因为不存在可返回的位置。
     * 若任务需要退出，应改为调用 vTaskDelete( NULL )。
     *
     * 若定义了 configASSERT()，则人为触发 assert()，
     * 然后在此处停止，以便应用开发者捕获该错误。
     */
    configASSERT( ulPortInterruptNesting == ~0UL );

    for( ;; )
    {
    }
}

/* ----------------------------------------------------------------------------------- */

void vPortEndScheduler( void )
{
    prvPortSchedulerRunning = pdFALSE;

    /* 本移植层未实现。人为触发 assert。 */
    configASSERT( prvPortSchedulerRunning == pdTRUE );
}

/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();

    /*
     * 现在中断已禁止，可直接访问 ulCriticalNesting。递增 ulCriticalNesting
     * 以统计 portENTER_CRITICAL() 的调用次数。
     */
    ulCriticalNesting++;

    /*
     * 这不是中断安全的进入临界区版本，因此若从中断上下文调用则触发 assert()。
     * 只有以“FromISR”结尾的 API 函数可在中断中使用。仅当临界区嵌套计数为 1
     * 时才断言，以防止 assert 函数本身使用临界区时出现递归调用。
     */
    if( ulCriticalNesting == 1 )
    {
        configASSERT( ulPortInterruptNesting == 0 );
    }
}
/* ----------------------------------------------------------------------------------- */

/* 特权函数 */
void vPortExitCritical( void )
{
    if( ulCriticalNesting > portNO_CRITICAL_NESTING )
    {
        /*
         * 退出临界区时递减嵌套计数。
         */
        ulCriticalNesting--;

        /*
         * 若嵌套计数已归零，则必须重新使能所有中断优先级。
         */
        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /*
             * 临界区嵌套已归零，因此应取消屏蔽所有中断优先级。
             */
            portENABLE_INTERRUPTS();
        }
    }
}
/* ----------------------------------------------------------------------------------- */
