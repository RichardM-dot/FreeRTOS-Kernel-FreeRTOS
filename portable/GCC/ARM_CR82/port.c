/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2025-2026 Arm Limited and/or its affiliates
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
#include <stdlib.h>
#include <string.h>

/*
 * 定义 MPU_WRAPPERS_INCLUDED_FROM_API_FILE 可防止 task.h 将所有 API 函数
 * 重定义为使用 MPU 包装函数。仅当从应用文件包含 task.h 时才应这样做。
 */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* MPU 头文件包含。 */
#include "mpu_wrappers.h"
#include "mpu_syscall_numbers.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#ifndef configINTERRUPT_CONTROLLER_BASE_ADDRESS
    #error configINTERRUPT_CONTROLLER_BASE_ADDRESS must be defined.  Refer to Cortex-A equivalent: /* 参见：https://www.freertos.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors */
#endif

#ifndef configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET
    #error configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET must be defined.  Refer to Cortex-A equivalent: /* 参见：https://www.freertos.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors */
#endif

#ifndef configUNIQUE_INTERRUPT_PRIORITIES
    #error configUNIQUE_INTERRUPT_PRIORITIES must be defined.  Refer to Cortex-A equivalent: /* 参见：https://www.freertos.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors */
#endif

#ifndef configSETUP_TICK_INTERRUPT
    #error configSETUP_TICK_INTERRUPT() must be defined.  Refer to Cortex-A equivalent: /* 参见：https://www.freertos.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors */
#endif /* configSETUP_TICK_INTERRUPT 条件编译结束 */

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
    #error configMAX_API_CALL_INTERRUPT_PRIORITY must be defined.  Refer to Cortex-A equivalent: /* 参见：https://www.freertos.org/Using-FreeRTOS-on-Cortex-A-Embedded-Processors */
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY == 0
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must not be set to 0"
#endif

#if configMAX_API_CALL_INTERRUPT_PRIORITY > configUNIQUE_INTERRUPT_PRIORITIES
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must be less than or equal to configUNIQUE_INTERRUPT_PRIORITIES as the lower the numeric priority value the higher the logical interrupt priority"
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error "configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 different priorities as tasks that share a priority will time slice."
    #endif
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

/* 安全扩展相关定义。 */
#if configMAX_API_CALL_INTERRUPT_PRIORITY <= ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )
    #error "configMAX_API_CALL_INTERRUPT_PRIORITY must be greater than ( configUNIQUE_INTERRUPT_PRIORITIES / 2 )"
#endif

#ifndef configCLEAR_TICK_INTERRUPT
    #error configCLEAR_TICK_INTERRUPT must be defined in FreeRTOSConfig.h to clear which ever interrupt was used to generate the tick interrupt.
#endif

#if configNUMBER_OF_CORES < 1
    #error configNUMBER_OF_CORES must be set to 1 or greater.  If the application is not using multiple cores then set configNUMBER_OF_CORES to 1.
#endif /* configNUMBER_OF_CORES < 1 条件编译结束 */

#ifndef configENABLE_MPU
    #error configENABLE_MPU must be defined in FreeRTOSConfig.h. Set configENABLE_MPU to 1 to enable the MPU or 0 to disable the MPU.
#endif /* configENABLE_MPU == 1 条件编译结束 */

#if ( ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 != 0) )
    #error Arm Cortex-R82 port supports only MPU Wrapper V2.
#endif /* ( configENABLE_MPU == 1 ) && ( configUSE_MPU_WRAPPERS_V1 != 0 ) 条件编译结束 */

/*
 * 临界区嵌套计数达到该值时，临界区退出。
 */
#define portNO_CRITICAL_NESTING    ( ( size_t ) 0 )

/* 解除屏蔽所有中断优先级的宏。 */
#define portCLEAR_INTERRUPT_PRIORITIES_MASK()    __asm volatile ( "SVC %0" : : "i" ( portSVC_UNMASK_ALL_INTERRUPTS ) : "memory" )

/* 从 EL1 解除屏蔽所有中断优先级的宏。 */
#define portCLEAR_INTERRUPT_PRIORITIES_MASK_FROM_EL1()  \
{                                                       \
    __asm volatile (                                    \
    " MSR DAIFSET, # 2                 \n"              \
    " DSB SY                           \n"              \
    " ISB SY                           \n"              \
    " MOV X0, %0                       \n"              \
    " MSR ICC_PMR_EL1, X0              \n"              \
    " DSB SY                           \n"              \
    " ISB SY                           \n"              \
    " MSR DAIFCLR, # 2                 \n"              \
    " DSB SY                           \n"              \
    " ISB SY                           \n"              \
    :                                                   \
    : "i" ( portUNMASK_VALUE )                          \
    );                                                  \
}

/*
 * 任务创建时没有浮点上下文，但可在创建后为其分配浮点上下文。
 * 任务上下文中的变量用于保存该状态：若任务没有 FPU 上下文，
 * 则保存 portNO_FLOATING_POINT_CONTEXT；否则保存其他值。
 */
#define portNO_FLOATING_POINT_CONTEXT    ( ( StackType_t ) 0 )

/* 初始化任务初始上下文所需的常量。 */
#define portSP_ELx                       ( ( StackType_t ) 0x01 )
#define portSP_EL0                       ( ( StackType_t ) 0x00 )
#define portEL1                          ( ( StackType_t ) 0x04 )
#define portEL0                          ( ( StackType_t ) 0x00 )

#define portINITIAL_PSTATE_EL0           ( portEL0 | portSP_EL0 )
#define portINITIAL_PSTATE_EL1           ( portEL1 | portSP_EL0 )

/*
 * portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 在确保二进制小数点为零时使用。
 */
#define portBINARY_POINT_BITS            ( ( uint8_t ) 0x03 )

/* 屏蔽 APSR 中除模式位以外的所有位。 */
#define portAPSR_MODE_BITS_MASK          ( 0x0C )

/* DAIF 位中的 I 位。 */
#define portDAIF_I                       ( 0x80 )

#define portMAX_8_BIT_VALUE              ( ( uint8_t ) 0xff )
#define portBIT_0_SET                    ( ( uint8_t ) 0x01 )

/*
 * 保存 FPU 寄存器所需的栈空间。
 * 共有 32 个 128 位寄存器加 2 个 64 位状态寄存器。
 */
#define portFPU_REGISTER_WORDS           ( ( 32 * 2 ) + 2 )

/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 设置存储器保护单元（MPU）。
     */
    PRIVILEGED_FUNCTION void vSetupMPU( void );

    /**
     * @brief 使能存储器保护单元（MPU）。
     */
    PRIVILEGED_FUNCTION void vEnableMPU( void );

    /**
     * @brief 从中断服务程序（ISR）调用，返回当前代码所运行的核心 ID。
     *
     * @return uint8_t 核心 ID。
     */
    PRIVILEGED_FUNCTION uint8_t ucPortGetCoreIDFromIsr( void );

    /**
     * @brief 检查调用任务是否具有特权。
     *
     * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
     */
    PRIVILEGED_FUNCTION BaseType_t xPortIsTaskPrivileged( void );

    /**
     * @brief 从保护区域基地址寄存器（PRBAR_EL1）的值中提取 MPU 区域的访问权限。
     *
     * @param ullPrbarEl1Value 该 MPU 区域的 PRBAR_EL1 值。
     *
     * @return uint32_t 访问权限。
     */
    PRIVILEGED_FUNCTION static uint32_t prvGetRegionAccessPermissions( uint64_t ullPrbarEl1Value );

    /**
     * @brief 执行进入系统调用所需的操作。
     *
     * @param pullPrivilegedOnlyTaskStack 触发 SVC 时任务的私有栈指针。
     * @param ucSystemCallNumber 系统调用编号。
     */
    PRIVILEGED_FUNCTION void vSystemCallEnter( uint64_t * pullPrivilegedOnlyTaskStack,
                                               uint8_t ucSystemCallNumber );

    /**
     * @brief 触发 SVC，以退出系统调用。
     */
    PRIVILEGED_FUNCTION __attribute__( ( naked ) ) void vRequestSystemCallExit( void );

    /**
     * @brief 设置任务栈，使其从 SVC 返回后重新使用该任务栈。
     *
     * @param ullSystemCallReturnValue 系统调用的实际返回值。
     */
    PRIVILEGED_FUNCTION void vSystemCallExit( uint64_t ullSystemCallReturnValue );

#endif /* configENABLE_MPU == 1 条件编译结束 */

/*
 * 启动第一个任务。该函数必须以汇编代码编写，因此实现在 portASM.s 中。
 */
extern void vPortRestoreTaskContext( void );
extern void vGIC_EnableIRQ( uint32_t ulInterruptID );
extern void vGIC_SetPriority( uint32_t ulInterruptID, uint32_t ulPriority );
extern void vGIC_PowerUpRedistributor( void );
extern void vGIC_EnableCPUInterface( void );

/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES == 1 )

    PRIVILEGED_DATA volatile uint64_t ullCriticalNesting = 0ULL;

    /*
     * 作为任务上下文的一部分保存。若 ullPortTaskHasFPUContext 非零，
     * 则必须为该任务保存和恢复浮点上下文。
     */
    PRIVILEGED_DATA uint64_t ullPortTaskHasFPUContext = pdFALSE;

    /* 置 1 表示从中断服务程序（ISR）中请求一次上下文切换。 */
    PRIVILEGED_DATA uint64_t ullPortYieldRequired = pdFALSE;

    /*
     * 统计中断嵌套深度。仅当嵌套深度为 0 时才会执行上下文切换。
     */
    PRIVILEGED_DATA uint64_t ullPortInterruptNesting = 0;

#else /* configNUMBER_OF_CORES == 1 条件分支 */
    PRIVILEGED_DATA volatile uint64_t ullCriticalNestings[ configNUMBER_OF_CORES ] = { 0 };

    /* 用于检查从核是否就绪的标志。 */
    PRIVILEGED_DATA volatile uint8_t ucSecondaryCoresReadyFlags[ configNUMBER_OF_CORES - 1 ] = { 0 };

    /* 用于通知主核已完成所有共享初始化的标志。 */
    PRIVILEGED_DATA volatile uint8_t ucPrimaryCoreInitDoneFlag = 0;

    /*
     * 作为任务上下文的一部分保存。若 ullPortTaskHasFPUContext 非零，
     * 则必须为该任务保存和恢复浮点上下文。
     */
    PRIVILEGED_DATA uint64_t ullPortTaskHasFPUContext[ configNUMBER_OF_CORES ] = { pdFALSE };

    /* 置 1 表示从中断服务程序（ISR）中请求一次上下文切换。 */
    PRIVILEGED_DATA uint64_t ullPortYieldRequired[ configNUMBER_OF_CORES ] = { pdFALSE };

    /*
     * 统计中断嵌套深度。仅当嵌套深度为 0 时才会执行上下文切换。
     */
    PRIVILEGED_DATA uint64_t ullPortInterruptNestings[ configNUMBER_OF_CORES ] = { 0 };

#endif /* configNUMBER_OF_CORES == 1 条件编译结束 */

#if ( configENABLE_MPU == 1 )
    /* 调度器启动时置为 pdTRUE。 */
    PRIVILEGED_DATA static BaseType_t xSchedulerRunning = pdFALSE;
#endif /* configENABLE_MPU == 1 条件编译结束 */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters,
                                         BaseType_t xRunPrivileged,
                                         xMPU_SETTINGS * xMPUSettings )
    {
        uint32_t ulIndex = 0;

        /*
         * 布局必须与 portRESTORE_CONTEXT 的弹出顺序（递减栈）一致：
         * 1) FPU 标志，2) 临界区嵌套值，3) 可选的 FPU 保存区，
         * 4) ELR（PC），5) SPSR（PSTATE），6) 按恢复顺序成对保存的通用寄存器。
         */

        /* 1) FPU 标志和 2) 临界区嵌套值。 */
        #if ( configUSE_TASK_FPU_SUPPORT == portTASK_NO_FPU_CONTEXT_BY_DEFAULT )
            xMPUSettings->ullContext[ ulIndex++ ] = portNO_FLOATING_POINT_CONTEXT; /* FPU 标志 */
            xMPUSettings->ullContext[ ulIndex++ ] = portNO_CRITICAL_NESTING;       /* 临界区嵌套值 */
        #elif ( configUSE_TASK_FPU_SUPPORT == portTASK_HAVE_FPU_CONTEXT_BY_DEFAULT )
            xMPUSettings->ullContext[ ulIndex++ ] = pdTRUE;                        /* FPU 标志 */
            xMPUSettings->ullContext[ ulIndex++ ] = portNO_CRITICAL_NESTING;       /* 临界区嵌套值 */
            #if ( configNUMBER_OF_CORES == 1 )
                ullPortTaskHasFPUContext = pdTRUE;
            #else
                ullPortTaskHasFPUContext[ portGET_CORE_ID() ] = pdTRUE;
            #endif
        #else
            #error "Invalid configUSE_TASK_FPU_SUPPORT setting - must be 1 or 2."
        #endif

        /* 3) 可选的 FPU 保存区，紧跟在标志和临界区嵌套值之后。 */
        #if ( configUSE_TASK_FPU_SUPPORT == portTASK_HAVE_FPU_CONTEXT_BY_DEFAULT )
            memset( &xMPUSettings->ullContext[ ulIndex ], 0x00, portFPU_REGISTER_WORDS * sizeof( StackType_t ) );
            ulIndex += portFPU_REGISTER_WORDS;
        #endif

        /* 4) ELR（PC）和 5) SPSR（PSTATE）。 */
        xMPUSettings->ullContext[ ulIndex++ ] = ( StackType_t ) pxCode; /* ELR（异常链接寄存器） */

        if( xRunPrivileged == pdTRUE )
        {
            xMPUSettings->ullContext[ ulIndex++ ] = portINITIAL_PSTATE_EL1;    /* SPSR（保存的程序状态寄存器） */
        }
        else
        {
            xMPUSettings->ullContext[ ulIndex++ ] = portINITIAL_PSTATE_EL0;    /* SPSR（保存的程序状态寄存器） */
        }

        /* 6) 按 restoreallgpregisters 期望的顺序排列的通用寄存器。 */
        xMPUSettings->ullContext[ ulIndex++ ] = ( StackType_t ) 0x00;         /* X30（LR 链接寄存器） */
        xMPUSettings->ullContext[ ulIndex++ ] = ( StackType_t ) 0x00;         /* XZR（占位） */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x2828282828282828ULL;        /* X28 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x2929292929292929ULL;        /* X29 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x2626262626262626ULL;        /* X26 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x2727272727272727ULL;        /* X27 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x2424242424242424ULL;        /* X24 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x2525252525252525ULL;        /* X25 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x2222222222222222ULL;        /* X22 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x2323232323232323ULL;        /* X23 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x2020202020202020ULL;        /* X20 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x2121212121212121ULL;        /* X21 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x1818181818181818ULL;        /* X18 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x1919191919191919ULL;        /* X19 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x1616161616161616ULL;        /* X16 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x1717171717171717ULL;        /* X17 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x1414141414141414ULL;        /* X14 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x1515151515151515ULL;        /* X15 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x1212121212121212ULL;        /* X12 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x1313131313131313ULL;        /* X13 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x1010101010101010ULL;        /* X10 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x1111111111111111ULL;        /* X11 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x0808080808080808ULL;        /* X8 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x0909090909090909ULL;        /* X9 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x0606060606060606ULL;        /* X6 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x0707070707070707ULL;        /* X7 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x0404040404040404ULL;        /* X4 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x0505050505050505ULL;        /* X5 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = 0x0202020202020202ULL;        /* X2 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x0303030303030303ULL;        /* X3 寄存器 */

        xMPUSettings->ullContext[ ulIndex++ ] = ( StackType_t ) pvParameters; /* X0 寄存器 */
        xMPUSettings->ullContext[ ulIndex++ ] = 0x0101010101010101ULL;        /* X1 寄存器 */

        if( xRunPrivileged == pdTRUE )
        {
            xMPUSettings->ullTaskFlags |= portTASK_IS_PRIVILEGED_FLAG;
        }
        else
        {
            xMPUSettings->ullTaskFlags &= ( ~portTASK_IS_PRIVILEGED_FLAG );
        }

        xMPUSettings->ullTaskUnprivilegedSP = ( ( uint64_t ) pxTopOfStack & portMPU_PRBAR_EL1_ADDRESS_MASK );

        return &( xMPUSettings->ullContext[ 0 ] );
    }

#else /* configENABLE_MPU == 1 条件分支 */

    /*
     * 详见头文件中的说明。
     */
    StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                         TaskFunction_t pxCode,
                                         void * pvParameters )
    {
        /*
         * 设置任务的初始栈。栈的布置与 portRESTORE_CONTEXT() 宏的预期完全一致。
         */

        /* 首先保存所有通用寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0101010101010101ULL;        /* R1 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0303030303030303ULL;        /* R3 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0202020202020202ULL;        /* R2 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0505050505050505ULL;        /* R5 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0404040404040404ULL;        /* R4 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0707070707070707ULL;        /* R7 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0606060606060606ULL;        /* R6 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0909090909090909ULL;        /* R9 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x0808080808080808ULL;        /* R8 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1111111111111111ULL;        /* R11 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1010101010101010ULL;        /* R10 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1313131313131313ULL;        /* R13 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1212121212121212ULL;        /* R12 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1515151515151515ULL;        /* R15 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1414141414141414ULL;        /* R14 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1717171717171717ULL;        /* R17 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1616161616161616ULL;        /* R16 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1919191919191919ULL;        /* R19 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x1818181818181818ULL;        /* R18 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2121212121212121ULL;        /* R21 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2020202020202020ULL;        /* R20 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2323232323232323ULL;        /* R23 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2222222222222222ULL;        /* R22 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2525252525252525ULL;        /* R25 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2424242424242424ULL;        /* R24 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2727272727272727ULL;        /* R27 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2626262626262626ULL;        /* R26 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2929292929292929ULL;        /* R29 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = 0x2828282828282828ULL;        /* R28 寄存器 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x00;         /* XZR 无实际作用，保留它是为了使寄存器个数为偶数。 */
        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) 0x00;         /* R30——过程调用链接寄存器。 */

        pxTopOfStack--;
        *pxTopOfStack = portINITIAL_PSTATE_EL0;

        pxTopOfStack--;
        *pxTopOfStack = ( StackType_t ) pxCode; /* 异常返回地址。 */

        #if ( configUSE_TASK_FPU_SUPPORT == portTASK_NO_FPU_CONTEXT_BY_DEFAULT )
        {
            /*
             * 任务将以临界区嵌套计数 0 启动，因为此时中断已使能。
             */
            pxTopOfStack--;
            *pxTopOfStack = portNO_CRITICAL_NESTING;

            /*
             * 任务启动时没有浮点上下文。使用浮点硬件的任务必须在执行
             * 任何浮点指令前调用 vPortTaskUsesFPU()。
             */
            pxTopOfStack--;
            *pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;
        }
        #elif ( configUSE_TASK_FPU_SUPPORT == portTASK_HAVE_FPU_CONTEXT_BY_DEFAULT )
        {
            /*
             * 任务启动时具有浮点上下文。为寄存器预留足够空间，
             * 并确保将其初始化为 0。
             */
            pxTopOfStack -= portFPU_REGISTER_WORDS;
            memset( pxTopOfStack, 0x00, portFPU_REGISTER_WORDS * sizeof( StackType_t ) );

            /*
             * 任务将以临界区嵌套计数 0 启动，因为此时中断已使能。
             */
            pxTopOfStack--;
            *pxTopOfStack = portNO_CRITICAL_NESTING;

            pxTopOfStack--;
            *pxTopOfStack = pdTRUE;
            #if ( configNUMBER_OF_CORES == 1 )
            ullPortTaskHasFPUContext = pdTRUE;
            #else
            ullPortTaskHasFPUContext[ portGET_CORE_ID() ] = pdTRUE;
            #endif
        }
        #else /* if ( configUSE_TASK_FPU_SUPPORT == portTASK_NO_FPU_CONTEXT_BY_DEFAULT ) 条件分支 */
        {
            #error "Invalid configUSE_TASK_FPU_SUPPORT setting - configUSE_TASK_FPU_SUPPORT must be set to 1, 2, or left undefined."
        }
        #endif /* if ( configUSE_TASK_FPU_SUPPORT == portTASK_NO_FPU_CONTEXT_BY_DEFAULT ) 条件结束 */

        return pxTopOfStack;

    }

#endif /* configENABLE_MPU == 1 条件编译结束 */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 将任务的 MPU 设置保存到其 TCB 中。
     *
     * @ingroup Task Context
     * @ingroup MPU Control
     *
     * @param xMPUSettings TCB 中的 MPU 设置。
     * @param xRegions 任务请求的 MPU 区域。
     * @param pxBottomOfStack 任务栈的基地址。
     * @param xStackDepth 任务栈的长度。
     */
    void vPortStoreTaskMPUSettings( xMPU_SETTINGS * xMPUSettings,
                                    const struct xMEMORY_REGION * const xRegions,
                                    StackType_t * pxBottomOfStack,
                                    StackType_t xStackDepth )  /* 特权函数 */
    {
        uint64_t ullRegionStartAddress, ullRegionEndAddress;
        uint8_t  ucIndex = 0, ucRegionNumber;

        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而不是从链接脚本导出时的声明。
             */
            extern uint64_t * __privileged_sram_start__;
            extern uint64_t * __privileged_sram_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint64_t __privileged_sram_start__[];
            extern uint64_t __privileged_sram_end__[];
        #endif /* defined( __ARMCC_VERSION ) 条件编译结束 */

        /* 设置 MAIR_EL1。 */
        xMPUSettings->ullMairEl1 = ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_EL1_ATTR0_POS ) & portMPU_MAIR_EL1_ATTR0_MASK );
        xMPUSettings->ullMairEl1 |= ( ( portMPU_DEVICE_MEMORY_nGnRE << portMPU_MAIR_EL1_ATTR1_POS ) & portMPU_MAIR_EL1_ATTR1_MASK );

        /*
         * 该函数在任务创建时自动调用——此时栈区域参数有效。在其他任何时候，
         * 栈参数都无效，并假定栈区域已经配置完成。
         */
        if( xStackDepth > 0 )
        {
            ullRegionStartAddress = ( uint64_t ) pxBottomOfStack;
            ullRegionEndAddress = ( uint64_t ) pxBottomOfStack + ( xStackDepth * ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t ) ) - 1;

            /*
             * 若栈位于特权 SRAM 内，则不要使用单独的 MPU 区域保护它。
             * 这是必要的，因为该区域已由某个 MPU 区域保护，而 ARMv8-R
             * 不允许 MPU 区域重叠。
             */
            if( ( ullRegionStartAddress >= ( uint64_t ) __privileged_sram_start__ ) &&
                ( ullRegionEndAddress <= ( uint64_t ) __privileged_sram_end__ ) )
            {
                xMPUSettings->xRegionsSettings[ portSTACK_REGION_INDEX ].ullPrbarEl1 = 0;
                xMPUSettings->xRegionsSettings[ portSTACK_REGION_INDEX ].ullPrlarEl1 = 0;
            }
            else
            {
                /* 定义允许访问该栈的区域。 */
                ullRegionStartAddress &= portMPU_PRBAR_EL1_ADDRESS_MASK;
                ullRegionEndAddress &= portMPU_PRLAR_EL1_ADDRESS_MASK;

                xMPUSettings->xRegionsSettings[ portSTACK_REGION_INDEX ].ullPrbarEl1 = ( ullRegionStartAddress ) |
                                                                                 ( portMPU_REGION_INNER_SHAREABLE ) |
                                                                                 ( portMPU_REGION_READ_WRITE ) |
                                                                                 ( portMPU_REGION_EXECUTE_NEVER );

                xMPUSettings->xRegionsSettings[ portSTACK_REGION_INDEX ].ullPrlarEl1 = ( ullRegionEndAddress ) |
                                                                                 ( portMPU_PRLAR_EL1_ATTR_INDEX0 ) |
                                                                                 ( portMPU_PRLAR_EL1_REGION_ENABLE );
            }
        }

        /* 用户提供的可配置区域。 */
        for( ucRegionNumber = 1; ucRegionNumber <= portNUM_CONFIGURABLE_REGIONS; ucRegionNumber++ )
        {
            /*
             * 若 xRegions 为 NULL，即任务未指定任何 MPU 区域，则 else 分支
             * 会确保所有可配置 MPU 区域都无效。
             * 最小区域大小为 64 字节。
             */
            if( xRegions != NULL )
            {
                /*
                 * 仅当基地址非 NULL 时才配置该区域。
                 * 用户可能只使用可用 MPU 区域的一个子集。
                 * 该检查可防止配置未定义区域（即基地址为 NULL 的区域），
                 * 并避免触发下面的区域大小检查断言。
                 */
                if ( xRegions[ ucIndex ].pvBaseAddress != NULL )
                {
                    configASSERT( xRegions[ ucIndex ].ulLengthInBytes >= 64UL );

                    uint64_t ullPrbarEl1RegValue, ullPrlarEl1RegValue;

                    /*
                     * 将 xRegions 中包含的通用区域定义转换为 ARMv8-R 专用的
                     * MPU 设置，然后保存到 xMPUSettings 中。
                     */
                    ullRegionStartAddress = ( ( uint64_t ) xRegions[ ucIndex ].pvBaseAddress ) & portMPU_PRBAR_EL1_ADDRESS_MASK;
                    ullRegionEndAddress = ( uint64_t ) xRegions[ ucIndex ].pvBaseAddress + xRegions[ ucIndex ].ulLengthInBytes - 1;
                    ullRegionEndAddress &= portMPU_PRLAR_EL1_ADDRESS_MASK;

                    /* 检查是否与其他已配置的用户定义区域和栈区域重叠。 */
                    for( uint8_t ucUserRegionNumber = 0; ucUserRegionNumber < portNUM_CONFIGURABLE_REGIONS; ucUserRegionNumber++ )
                    {
                        /* 检查是否重叠。 */
                        if( ( portIS_ADDRESS_WITHIN_RANGE( ullRegionStartAddress,
                                                        ( xMPUSettings->xRegionsSettings[ ucUserRegionNumber ].ullPrbarEl1 & portMPU_PRBAR_EL1_ADDRESS_MASK ),
                                                        ( xMPUSettings->xRegionsSettings[ ucUserRegionNumber ].ullPrlarEl1 & portMPU_PRLAR_EL1_ADDRESS_MASK ) ) ||
                            ( portIS_ADDRESS_WITHIN_RANGE( ullRegionEndAddress,
                                                        ( xMPUSettings->xRegionsSettings[ ucUserRegionNumber ].ullPrbarEl1 & portMPU_PRBAR_EL1_ADDRESS_MASK ),
                                                        ( xMPUSettings->xRegionsSettings[ ucUserRegionNumber ].ullPrlarEl1 & portMPU_PRLAR_EL1_ADDRESS_MASK ) ) ) ) )
                        {
                            /* 检测到重叠——触发断言。 */
                            configASSERT( NULL );
                        }
                    }

                    /* 检查是否与 vSetupMPU 中已配置的内核区域重叠。 */
                    for (uint8_t ucProgrammedRegionIndex = 0; ucProgrammedRegionIndex < 4; ucProgrammedRegionIndex++)
                    {
                        __asm volatile ( "msr PRSELR_EL1, %0" : : "r" ( ( uint64_t ) ucProgrammedRegionIndex ) );

                        __asm volatile ( "mrs %0, PRBAR_EL1" : "=r" ( ullPrbarEl1RegValue ) );
                        ullPrbarEl1RegValue &= portMPU_PRBAR_EL1_ADDRESS_MASK;

                        __asm volatile ( "mrs %0, PRLAR_EL1" : "=r" ( ullPrlarEl1RegValue ) );
                        ullPrlarEl1RegValue &= portMPU_PRLAR_EL1_ADDRESS_MASK;

                        /* 检查是否重叠。 */
                        if( ( portIS_ADDRESS_WITHIN_RANGE( ullRegionStartAddress,
                                                        ullPrbarEl1RegValue,
                                                        ullPrlarEl1RegValue ) ) ||
                            ( portIS_ADDRESS_WITHIN_RANGE( ullRegionEndAddress,
                                                        ullPrbarEl1RegValue,
                                                        ullPrlarEl1RegValue ) ) )
                        {
                            /* 检测到重叠——触发断言。 */
                            configASSERT( NULL );
                        }
                    }

                    /* 起始地址。 */
                    xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 = ( ullRegionStartAddress );

                    /* 只读/读写。 */
                    if( ( xRegions[ ucIndex ].ulParameters & tskMPU_REGION_READ_ONLY ) != 0 )
                    {
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 |= ( portMPU_REGION_READ_ONLY );
                    }
                    else
                    {
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 |= ( portMPU_REGION_READ_WRITE );
                    }

                    /* XN（不可执行）位。 */
                    if( ( xRegions[ ucIndex ].ulParameters & tskMPU_REGION_EXECUTE_NEVER ) != 0 )
                    {
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 |= ( portMPU_REGION_EXECUTE_NEVER );
                    }

                    /* SH（共享性）位。 */
                    if( ( xRegions[ ucIndex ].ulParameters & tskMPU_REGION_INNER_SHAREABLE ) != 0 )
                    {
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 |= ( portMPU_REGION_INNER_SHAREABLE );
                    }
                    else if( ( xRegions[ ucIndex ].ulParameters & tskMPU_REGION_OUTER_SHAREABLE ) != 0 )
                    {
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 |= ( portMPU_REGION_OUTER_SHAREABLE );
                    }
                    else
                    {
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 |= ( portMPU_REGION_NON_SHAREABLE );
                    }

                    /* 结束地址。 */
                    xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrlarEl1 = ( ullRegionEndAddress ) |
                                                                                ( portMPU_PRLAR_EL1_REGION_ENABLE );

                    /* 普通存储器/设备存储器。 */
                    if( ( xRegions[ ucIndex ].ulParameters & tskMPU_REGION_DEVICE_MEMORY ) != 0 )
                    {
                        /* MAIR_EL1 中的 Attr1 被配置为设备存储器。 */
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrlarEl1 |= portMPU_PRLAR_EL1_ATTR_INDEX1;
                    }
                    else
                    {
                        /* MAIR_EL1 中的 Attr0 被配置为普通存储器。 */
                        xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrlarEl1 |= portMPU_PRLAR_EL1_ATTR_INDEX0;
                    }
                }
            }
            else
            {
                /* 使该区域无效。 */
                xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrbarEl1 = 0UL;
                xMPUSettings->xRegionsSettings[ ucRegionNumber ].ullPrlarEl1 = 0UL;
            }

            ucIndex++;
        }

    }
    /*-----------------------------------------------------------*/

    void vSetupMPU( void ) /* 特权函数 */
    {
        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而不是从链接脚本导出时的声明。
             */
            extern uint64_t * __privileged_functions_start__;
            extern uint64_t * __privileged_functions_end__;
            extern uint64_t * __syscalls_flash_start__;
            extern uint64_t * __syscalls_flash_end__;
            extern uint64_t * __unprivileged_flash_start__;
            extern uint64_t * __unprivileged_flash_end__;
            extern uint64_t * __privileged_sram_start__;
            extern uint64_t * __privileged_sram_end__;
        #else /* if defined( __ARMCC_VERSION ) 条件分支 */
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint64_t __privileged_functions_start__[];
            extern uint64_t __privileged_functions_end__[];
            extern uint64_t __syscalls_flash_start__[];
            extern uint64_t __syscalls_flash_end__[];
            extern uint64_t __unprivileged_flash_start__[];
            extern uint64_t __unprivileged_flash_end__[];
            extern uint64_t __privileged_sram_start__[];
            extern uint64_t __privileged_sram_end__[];
        #endif /* defined( __ARMCC_VERSION ) 条件编译结束 */

        /* 只允许 16 或 32 个区域。 */
        configASSERT( ( configTOTAL_MPU_REGIONS == 16 ) || ( configTOTAL_MPU_REGIONS == 32 ) );

        /* MAIR_EL1——索引 0。 */
        uint64_t ullMairEl1RegValue = ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_EL1_ATTR0_POS ) & portMPU_MAIR_EL1_ATTR0_MASK );
        /* MAIR_EL1——索引 1。 */
        ullMairEl1RegValue |= ( ( portMPU_DEVICE_MEMORY_nGnRE << portMPU_MAIR_EL1_ATTR1_POS ) & portMPU_MAIR_EL1_ATTR1_MASK );

        __asm volatile ( "msr MAIR_EL1, %0" : : "r" ( ullMairEl1RegValue ) );

        /*
         * 将特权 Flash 设置为只读，使特权任务可以读取但不能修改。
         */
        uint64_t ullPrselrEl1RegValue = portPRIVILEGED_FLASH_REGION;
        __asm volatile ( "msr PRSELR_EL1, %0" : : "r" ( ullPrselrEl1RegValue ) );

        uint64_t ullPrbarEl1RegValue = ( ( ( uint64_t ) __privileged_functions_start__ ) & portMPU_PRBAR_EL1_ADDRESS_MASK ) |
                                       ( portMPU_REGION_NON_SHAREABLE ) |
                                       ( portMPU_REGION_PRIVILEGED_READ_ONLY );
        __asm volatile ( "msr PRBAR_EL1, %0" : : "r" ( ullPrbarEl1RegValue ) );

        uint64_t ullPrlarEl1RegValue = ( ( ( uint64_t ) __privileged_functions_end__ ) & portMPU_PRLAR_EL1_ADDRESS_MASK ) |
                                       ( portMPU_PRLAR_EL1_ATTR_INDEX0 ) |
                                       ( portMPU_PRLAR_EL1_REGION_ENABLE );
        __asm volatile ( "msr PRLAR_EL1, %0" : : "r" ( ullPrlarEl1RegValue ) );

        /*
         * 将非特权 Flash 设置为特权任务和非特权任务都只读。
         * 所有任务都可读取，但任何任务都不能修改。
         */
        ullPrselrEl1RegValue = portUNPRIVILEGED_FLASH_REGION;
        __asm volatile ( "msr PRSELR_EL1, %0" : : "r" ( ullPrselrEl1RegValue ) );

        ullPrbarEl1RegValue = ( ( ( uint64_t ) __unprivileged_flash_start__ ) & portMPU_PRBAR_EL1_ADDRESS_MASK ) |
                              ( portMPU_REGION_NON_SHAREABLE ) |
                              ( portMPU_REGION_READ_ONLY );
        __asm volatile ( "msr PRBAR_EL1, %0" : : "r" ( ullPrbarEl1RegValue ) );

        ullPrlarEl1RegValue = ( ( ( uint64_t ) __unprivileged_flash_end__ ) & portMPU_PRLAR_EL1_ADDRESS_MASK ) |
                              ( portMPU_PRLAR_EL1_ATTR_INDEX0 ) |
                              ( portMPU_PRLAR_EL1_REGION_ENABLE );
        __asm volatile ( "msr PRLAR_EL1, %0" : : "r" ( ullPrlarEl1RegValue ) );

        /*
         * 将非特权系统调用 Flash 设置为特权任务和非特权任务都只读。
         * 所有任务都可读取，但任何任务都不能修改。
         */
        ullPrselrEl1RegValue = portUNPRIVILEGED_SYSCALLS_REGION;
        __asm volatile ( "msr PRSELR_EL1, %0" : : "r" ( ullPrselrEl1RegValue ) );

        ullPrbarEl1RegValue = ( ( ( uint64_t ) __syscalls_flash_start__ ) & portMPU_PRBAR_EL1_ADDRESS_MASK ) |
                              ( portMPU_REGION_NON_SHAREABLE ) |
                              ( portMPU_REGION_READ_ONLY );
        __asm volatile ( "msr PRBAR_EL1, %0" : : "r" ( ullPrbarEl1RegValue ) );

        ullPrlarEl1RegValue = ( ( ( uint64_t ) __syscalls_flash_end__ ) & portMPU_PRLAR_EL1_ADDRESS_MASK ) |
                              ( portMPU_PRLAR_EL1_ATTR_INDEX0 ) |
                              ( portMPU_PRLAR_EL1_REGION_ENABLE );
        __asm volatile ( "msr PRLAR_EL1, %0" : : "r" ( ullPrlarEl1RegValue ) );

        /* 将包含内核数据的 RAM 设置为仅特权访问。 */
        ullPrselrEl1RegValue = portPRIVILEGED_RAM_REGION;
        __asm volatile ( "msr PRSELR_EL1, %0" : : "r" ( ullPrselrEl1RegValue ) );

        ullPrbarEl1RegValue = ( ( ( uint64_t ) __privileged_sram_start__ ) & portMPU_PRBAR_EL1_ADDRESS_MASK ) |
                              ( portMPU_REGION_INNER_SHAREABLE ) |
                              ( portMPU_REGION_PRIVILEGED_READ_WRITE ) |
                              ( portMPU_REGION_EXECUTE_NEVER );
        __asm volatile ( "msr PRBAR_EL1, %0" : : "r" ( ullPrbarEl1RegValue ) );

        ullPrlarEl1RegValue = ( ( ( uint64_t ) __privileged_sram_end__ ) & portMPU_PRLAR_EL1_ADDRESS_MASK ) |
                              ( portMPU_PRLAR_EL1_ATTR_INDEX0 ) |
                              ( portMPU_PRLAR_EL1_REGION_ENABLE );
        __asm volatile ( "msr PRLAR_EL1, %0" : : "r" ( ullPrlarEl1RegValue ) );
    }
    /*-----------------------------------------------------------*/

    void vEnableMPU( void ) /* 特权函数 */
    {
        uint64_t ullSctlrEl1RegValue;

        __asm volatile ( "mrs %0, SCTLR_EL1" : "=r" ( ullSctlrEl1RegValue ) );
        /*
         * 使能 MPU，同时使能对背景区域的特权访问。
         */
        ullSctlrEl1RegValue |= ( portMPU_PRIV_BACKGROUND_ENABLE_BIT | portMPU_ENABLE_BIT );
        __asm volatile ( "msr SCTLR_EL1, %0" : : "r" ( ullSctlrEl1RegValue ) );

        /*
         * 确保对 SCTLR_EL1 的写入在返回前完成。
         */
        __asm volatile ( "isb" );
    }
    /*-----------------------------------------------------------*/

    BaseType_t xPortIsTaskPrivileged( void ) /* 特权函数 */
    {
        BaseType_t xTaskIsPrivileged = pdFALSE;

        #if ( configNUMBER_OF_CORES == 1 )
            extern TaskHandle_t pxCurrentTCB;
            xMPU_SETTINGS * pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );
        #else
            extern TaskHandle_t pxCurrentTCBs[ configNUMBER_OF_CORES ];
            xMPU_SETTINGS * pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCBs[ portGET_CORE_ID_FROM_ISR() ] );
        #endif

        if( ( pxMpuSettings->ullTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xTaskIsPrivileged = pdTRUE;
        }

        return xTaskIsPrivileged;
    }
    /*-----------------------------------------------------------*/

    static uint32_t prvGetRegionAccessPermissions( uint64_t ullPrbarEl1Value ) /* 特权函数 */
    {
        uint32_t ulAccessPermissions = 0;

        if( ( ullPrbarEl1Value & portMPU_PRBAR_EL1_ACCESS_PERMISSIONS_MASK ) == portMPU_REGION_READ_ONLY )
        {
            ulAccessPermissions = tskMPU_READ_PERMISSION;
        }

        if( ( ullPrbarEl1Value & portMPU_PRBAR_EL1_ACCESS_PERMISSIONS_MASK ) == portMPU_REGION_READ_WRITE )
        {
            ulAccessPermissions = ( tskMPU_READ_PERMISSION | tskMPU_WRITE_PERMISSION );
        }

        return ulAccessPermissions;
    }
    /*-----------------------------------------------------------*/

    BaseType_t xPortIsAuthorizedToAccessBuffer( const void * pvBuffer,
                                                uint32_t ulBufferLength,
                                                uint32_t ulAccessRequested ) /* 特权函数 */

    {
        uint32_t i;
        uint64_t ullBufferStartAddress, ullBufferEndAddress;
        BaseType_t xAccessGranted = pdFALSE;
        const xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

        if( xSchedulerRunning == pdFALSE )
        {
            /*
             * 调度器启动前授予对所有内核对象的访问权限。这是必要的，因为
             * 此时还没有任务在运行，因此无法使用任何任务的权限。
             */
            xAccessGranted = pdTRUE;
        }
        else if( ( xTaskMpuSettings->ullTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
        {
            xAccessGranted = pdTRUE;
        }
        else
        {
            if( portADD_UINT64_WILL_OVERFLOW( ( ( uint64_t ) pvBuffer ), ( ulBufferLength - 1UL ) ) == pdFALSE )
            {
                ullBufferStartAddress = ( uint64_t ) pvBuffer;
                ullBufferEndAddress = ( ( ( uint64_t ) pvBuffer ) + ulBufferLength - 1UL );

                for( i = 0; i < portTOTAL_NUM_REGIONS; i++ )
                {
                    /* 该 MPU 区域是否已使能？ */
                    if( ( xTaskMpuSettings->xRegionsSettings[ i ].ullPrlarEl1 & portMPU_PRLAR_EL1_REGION_ENABLE ) == portMPU_PRLAR_EL1_REGION_ENABLE )
                    {
                        if( portIS_ADDRESS_WITHIN_RANGE( ullBufferStartAddress,
                                                         portEXTRACT_FIRST_ADDRESS_FROM_PRBAR_EL1( xTaskMpuSettings->xRegionsSettings[ i ].ullPrbarEl1 ),
                                                         portEXTRACT_LAST_ADDRESS_FROM_PRLAR_EL1( xTaskMpuSettings->xRegionsSettings[ i ].ullPrlarEl1 ) ) &&
                            portIS_ADDRESS_WITHIN_RANGE( ullBufferEndAddress,
                                                         portEXTRACT_FIRST_ADDRESS_FROM_PRBAR_EL1( xTaskMpuSettings->xRegionsSettings[ i ].ullPrbarEl1 ),
                                                         portEXTRACT_LAST_ADDRESS_FROM_PRLAR_EL1( xTaskMpuSettings->xRegionsSettings[ i ].ullPrlarEl1 ) ) &&
                            portIS_AUTHORIZED( ulAccessRequested,
                                               prvGetRegionAccessPermissions( xTaskMpuSettings->xRegionsSettings[ i ].ullPrbarEl1 ) ) )
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
    /*-----------------------------------------------------------*/

    void vSystemCallEnter( uint64_t * pullPrivilegedOnlyTaskStack,
                           uint8_t ucSystemCallNumber ) /* 特权函数 */
    {
        #if ( configNUMBER_OF_CORES == 1 )
            extern TaskHandle_t pxCurrentTCB;
        #else
            extern TaskHandle_t pxCurrentTCBs[ configNUMBER_OF_CORES ];
        #endif
        extern UBaseType_t uxSystemCallImplementations[ NUM_SYSTEM_CALLS ];
        xMPU_SETTINGS * pxMpuSettings;
        uint64_t ullSystemCallLocation;  /* 触发 SVC 的地址。 */
        __asm volatile ( "MRS %0, ELR_EL1" : "=r" ( ullSystemCallLocation ) );

        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而不是从链接脚本导出时的声明。
             */
            extern uint64_t * __syscalls_flash_start__;
            extern uint64_t * __syscalls_flash_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint64_t __syscalls_flash_start__[];
            extern uint64_t __syscalls_flash_end__[];
        #endif /* if defined( __ARMCC_VERSION ) 条件编译结束 */

        #if ( configNUMBER_OF_CORES == 1 )
            pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );
        #else
            pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCBs[ portGET_CORE_ID_FROM_ISR() ] );
        #endif

        /*
         * 检查：
         * 1. SVC 是从系统调用段触发的（即应用未直接触发 SVC）。
         * 2. 无需检查 ucSystemCallNumber 是否在范围内，
         *    因为汇编 SVC 处理程序在调用该函数前已进行检查。
         */
        if( ( ullSystemCallLocation >= ( uint64_t ) __syscalls_flash_start__ ) &&
            ( ullSystemCallLocation <= ( uint64_t ) __syscalls_flash_end__ ) &&
            ( uxSystemCallImplementations[ ucSystemCallNumber ] != 0 ) )
        {
            /*
             * 在触发 SVC 之前保存链接寄存器的值。
             * 其中包含系统调用入口点调用者（即 MPU_<API> 的调用者）的地址。
             * 退出系统调用时必须恢复该值。
             */
            pxMpuSettings->xSystemCallInfo.ullLinkRegisterAtSystemCallEntry = pullPrivilegedOnlyTaskStack[ portOFFSET_TO_LR ];

            /* 在系统调用入口捕获用户模式 SP。 */
            uint64_t ullUserSpAtEntry;
            __asm volatile ( "MRS %0, SP_EL0" : "=r" ( ullUserSpAtEntry ) );
            pxMpuSettings->xSystemCallInfo.ullUserSPAtSystemCallEntry = ullUserSpAtEntry;

            /* 设置 MPU_<API> 的输入、系统调用栈和 SPSR。 */
            __asm volatile (
                "MOV X0, %0         \n"
                "MOV X1, %1         \n"
                "MOV X2, %2         \n"
                "MOV X3, %3         \n"
                "MSR ELR_EL1, %4    \n"
                "MSR SP_EL0, %5     \n"
                "MSR SPSR_EL1, %6   \n"
                :
                : "r" ( pullPrivilegedOnlyTaskStack[ portOFFSET_TO_X0 ] ),
                  "r" ( pullPrivilegedOnlyTaskStack[ portOFFSET_TO_X1 ] ),
                  "r" ( pullPrivilegedOnlyTaskStack[ portOFFSET_TO_X2 ] ),
                  "r" ( pullPrivilegedOnlyTaskStack[ portOFFSET_TO_X3 ] ),
                  "r" ( ( uint64_t ) uxSystemCallImplementations[ ucSystemCallNumber ] ),
                  "r" ( &( pxMpuSettings->ullContext[ MAX_CONTEXT_SIZE + configSYSTEM_CALL_STACK_SIZE ] ) ),
                  "r" ( portINITIAL_PSTATE_EL1 )
                : "memory", "x0", "x1", "x2", "x3"
            );
        }
    }
    /*-----------------------------------------------------------*/

    void vRequestSystemCallExit( void ) /* 特权函数（naked 属性） */
    {
        __asm volatile ( "svc %0 \n" ::"i" ( portSVC_SYSTEM_CALL_EXIT ) : "memory" );
    }
    /*-----------------------------------------------------------*/

    void vSystemCallExit( uint64_t ullSystemCallReturnValue ) /* 特权函数 */
    {
        #if ( configNUMBER_OF_CORES == 1 )
            extern TaskHandle_t pxCurrentTCB;
        #else
            extern TaskHandle_t pxCurrentTCBs[ configNUMBER_OF_CORES ];
        #endif
        xMPU_SETTINGS * pxMpuSettings;
        uint64_t ullSystemCallLocation; /* 触发 SVC 的地址。 */
        __asm volatile ( "MRS %0, ELR_EL1" : "=r" ( ullSystemCallLocation ) );

        #if defined( __ARMCC_VERSION )
            /*
             * 当这些变量在代码中定义而不是从链接脚本导出时的声明。
             */
            extern uint64_t * __privileged_functions_start__;
            extern uint64_t * __privileged_functions_end__;
        #else
            /* 当这些变量从链接脚本导出时的声明。 */
            extern uint64_t __privileged_functions_start__[];
            extern uint64_t __privileged_functions_end__[];
        #endif /* if defined( __ARMCC_VERSION ) 条件编译结束 */

        #if ( configNUMBER_OF_CORES == 1 )
            pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCB );
        #else
            pxMpuSettings = xTaskGetMPUSettings( pxCurrentTCBs[ portGET_CORE_ID_FROM_ISR() ] );
        #endif

        /*
         * 检查：
         * SVC 是从特权代码触发的（即应用未直接触发 SVC）。该 SVC 仅由
         * 特权代码段中的 vRequestSystemCallExit 触发。
         */
        if( ( ullSystemCallLocation >= ( uint64_t ) __privileged_functions_start__ ) &&
            ( ullSystemCallLocation <= ( uint64_t ) __privileged_functions_end__ ) )
        {
            __asm volatile (
                "MSR ELR_EL1, %0    \n" /* 返回到 MPU_<API> 调用者。 */
                "MSR SP_EL0, %1     \n" /* 恢复系统调用入口保存的用户 SP。 */
                "MSR SPSR_EL1, %3   \n" /* 确保返回 EL0。 */
                "MOV X0, %2         \n" /* 将系统调用返回值移动到 X0。 */
                :
                : "r" ( pxMpuSettings->xSystemCallInfo.ullLinkRegisterAtSystemCallEntry ),
                  "r" ( pxMpuSettings->xSystemCallInfo.ullUserSPAtSystemCallEntry ),
                  "r" ( ullSystemCallReturnValue ),
                  "r" ( ( uint64_t ) portINITIAL_PSTATE_EL0 )
                : "memory"
            );
        }
    }
    /*-----------------------------------------------------------*/

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        void vPortGrantAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                             int32_t lInternalIndexOfKernelObject ) /* 特权函数 */
        {
            uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
            xMPU_SETTINGS * xTaskMpuSettings;

            /*
             * 计算访问控制列表表项的索引以及该表项中的位位置。
             */
            ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
            ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

            xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

            /* 置位该内核对象对应的位，以授予访问权限。 */
            xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] |= ( 1U << ulAccessControlListEntryBit );
        }
        /*-----------------------------------------------------------*/

        void vPortRevokeAccessToKernelObject( TaskHandle_t xInternalTaskHandle,
                                              int32_t lInternalIndexOfKernelObject ) /* 特权函数 */
        {
            uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
            xMPU_SETTINGS * xTaskMpuSettings;

            /*
             * 计算访问控制列表表项的索引以及该表项中的位位置。
             */
            ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
            ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

            xTaskMpuSettings = xTaskGetMPUSettings( xInternalTaskHandle );

            /* 清除该内核对象对应的位，以撤销访问权限。 */
            xTaskMpuSettings->ulAccessControlList[ ulAccessControlListEntryIndex ] &= ~( 1U << ulAccessControlListEntryBit );
        }
        /*-----------------------------------------------------------*/

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* 特权函数 */
        {
            uint32_t ulAccessControlListEntryIndex, ulAccessControlListEntryBit;
            BaseType_t xAccessGranted = pdFALSE;
            const xMPU_SETTINGS * xTaskMpuSettings;

            if( xSchedulerRunning == pdFALSE )
            {
                /*
                 * 调度器启动前授予对所有内核对象的访问权限。这是必要的，因为
                 * 此时还没有任务在运行，因此无法使用任何任务的权限。
                 */
                xAccessGranted = pdTRUE;
            }
            else
            {
                xTaskMpuSettings = xTaskGetMPUSettings( NULL ); /* 调用任务的 MPU 设置。 */

                ulAccessControlListEntryIndex = ( ( uint32_t ) lInternalIndexOfKernelObject / portACL_ENTRY_SIZE_BITS );
                ulAccessControlListEntryBit = ( ( uint32_t ) lInternalIndexOfKernelObject % portACL_ENTRY_SIZE_BITS );

                if( ( xTaskMpuSettings->ullTaskFlags & portTASK_IS_PRIVILEGED_FLAG ) == portTASK_IS_PRIVILEGED_FLAG )
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
        /*-----------------------------------------------------------*/
    #else /* configENABLE_ACCESS_CONTROL_LIST == 1 条件分支 */

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* 特权函数 */
        {
            ( void ) lInternalIndexOfKernelObject;

            /*
             * 若未使用访问控制列表功能，则所有任务都可访问所有内核对象。
             */
            return pdTRUE;
        }
        /*-----------------------------------------------------------*/

    #endif /* configENABLE_ACCESS_CONTROL_LIST == 1 条件编译结束 */

#endif /* configENABLE_MPU == 1 条件编译结束 */

BaseType_t xPortStartScheduler( void )
{
    uint64_t ullAPSR;

    #if ( configASSERT_DEFINED == 1 )
    {
        volatile uint8_t ucOriginalPriority;
        volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( configINTERRUPT_CONTROLLER_BASE_ADDRESS + portINTERRUPT_PRIORITY_REGISTER_OFFSET );
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定 GIC 中实现了多少个优先级位。
         *
         * 保存即将被覆盖的中断优先级值。
         */
        ucOriginalPriority = *pucFirstUserPriorityRegister;

        /*
         * 确定可用的优先级位数。首先向所有可能的位写入。
         */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

        /* 读回该值，查看有多少位保持有效。 */
        ucMaxPriorityValue = *pucFirstUserPriorityRegister;

        /* 移位到最低有效位。 */
        while( ( ucMaxPriorityValue & portBIT_0_SET ) != portBIT_0_SET )
        {
            ucMaxPriorityValue >>= ( uint8_t ) 0x01;
        }

        /*
         * 检查 configUNIQUE_INTERRUPT_PRIORITIES 是否与读到的值匹配。
         */
        configASSERT( ucMaxPriorityValue >= portLOWEST_INTERRUPT_PRIORITY );


        /*
         * 将此前被覆盖的中断优先级寄存器恢复为原值。
         */
        *pucFirstUserPriorityRegister = ucOriginalPriority;
    }
    #endif /* configASSERT_DEFINED 条件编译结束 */

    __asm volatile ( "MRS %0, CurrentEL" : "=r" ( ullAPSR ) );
    ullAPSR &= portAPSR_MODE_BITS_MASK;

    configASSERT( ullAPSR == portEL1 );

    #if ( configENABLE_MPU == 1 )
    {
        /* 设置存储器保护单元（MPU）。 */
        vSetupMPU();
    }
    #endif /* configENABLE_MPU == 1 条件编译结束 */

    /*
     * 在 CPU 自身中关闭中断，以确保调度器启动期间不会执行时钟节拍。
     * 第一个任务开始执行时，CPU 会自动重新使能中断。
     */
    __asm volatile ( "MSR DAIFSET, #2\n"
                     "DSB SY\n"
                     "ISB SY\n" ::: "memory" );
    #if ( configNUMBER_OF_CORES > 1 )
        /* 启动产生时钟节拍 ISR 的定时器。 */
        configSETUP_TICK_INTERRUPT();
        ucPrimaryCoreInitDoneFlag = 1;
        __asm volatile ( "SEV                       \n"
                         "DSB SY                    \n"
                         "ISB SY                    \n"
                         ::: "memory" );
        /*
         * 在此等待主核，直到所有从核就绪；这只有在 ucSecondaryCoresReadyFlags
         * 的所有元素都置位时才会发生。
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
    #else /* if ( configNUMBER_OF_CORES > 1 ) 条件分支 */
        /* 启动产生时钟节拍 ISR 的定时器。 */
        configSETUP_TICK_INTERRUPT();
    #endif /* if ( configNUMBER_OF_CORES > 1 ) 条件编译结束 */

    #if ( configENABLE_MPU == 1 )
        xSchedulerRunning = pdTRUE;

        /*
         * 使能存储器保护单元（MPU）。
         * 只有在主核与从核握手完成后才使能 MPU，以防止不同核心上的
         * MPU 区域属性不一致，导致握手标志的值未及时更新。
         */
        vEnableMPU();
    #endif /* configENABLE_MPU == 1 条件编译结束 */

    /* 启动第一个任务。 */
    vPortRestoreTaskContext();

    return 0;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void ) /* 特权函数 */
{
    /*
     * 在没有可返回位置的移植层中提供的桩实现。
     * 人为触发 assert。
     */
    configASSERT( NULL );
}

/*-----------------------------------------------------------*/

#if  ( configNUMBER_OF_CORES == 1 )
    PRIVILEGED_FUNCTION void vPortEnterCritical( void )
    {
        /* 屏蔽中断至最大系统调用中断优先级。 */
        uxPortSetInterruptMask();

        /*
         * 现在中断已禁止，可直接访问 ullCriticalNesting。递增 ullCriticalNesting
         * 以统计 portENTER_CRITICAL() 的调用次数。
         */
        ullCriticalNesting++;

        /*
         * 这不是中断安全的进入临界区版本，因此若从中断上下文调用则触发 assert()。
         * 只有以“FromISR”结尾的 API 函数可在中断中使用。仅当临界区嵌套计数为 1
         * 时才断言，以防止 assert 函数本身使用临界区时出现递归调用。
         */
        if( ullCriticalNesting == 1ULL )
        {
            configASSERT( ullPortInterruptNesting == 0 );
        }
    }

/*-----------------------------------------------------------*/

    PRIVILEGED_FUNCTION void vPortExitCritical( void )
    {
        if( ullCriticalNesting > portNO_CRITICAL_NESTING )
        {
            /*
             * 退出临界区时递减嵌套计数。
             */
            ullCriticalNesting--;

            /*
             * 若嵌套计数已归零，则必须重新使能所有中断优先级。
             */
            if( ullCriticalNesting == portNO_CRITICAL_NESTING )
            {
                /*
                 * 临界区嵌套已归零，因此应取消屏蔽所有中断优先级。
                 */
                portCLEAR_INTERRUPT_PRIORITIES_MASK();
            }
        }
    }
#endif /* if ( configNUMBER_OF_CORES == 1 ) 条件编译结束 */

/*-----------------------------------------------------------*/

void FreeRTOS_Tick_Handler( void )
{
    /* 必须使用最低可能的优先级。 */
    uint64_t ullRunningInterruptPriority;
    __asm volatile ( "MRS %0, ICC_RPR_EL1" : "=r" ( ullRunningInterruptPriority ) );

    configASSERT( ullRunningInterruptPriority == ( portLOWEST_USABLE_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );

    /* 在此点之前不应使能中断。 */
    #if ( configASSERT_DEFINED == 1 )
    {
        uint64_t ullMaskBits;

        __asm volatile ( "MRS %0, DAIF" : "=r" ( ullMaskBits )::"memory" );
        configASSERT( ( ullMaskBits & portDAIF_I ) != 0 );
    }
    #endif /* configASSERT_DEFINED 条件编译结束 */

    /*
     * 在修改调度器结构前设置中断屏蔽。时钟节拍中断以最低优先级运行，
     * 因此中断不可能已被屏蔽，无需保存和恢复当前屏蔽值。更新 ICCPMR 时
     * 必须关闭 CPU 自身的中断。
     */
    UBaseType_t uxInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    #if ( configNUMBER_OF_CORES > 1 )
        UBaseType_t x = portENTER_CRITICAL_FROM_ISR();
    #endif /* if ( configNUMBER_OF_CORES > 1 ) 条件编译结束 */

    /* 递增 RTOS 时钟节拍。 */
    if( xTaskIncrementTick() != pdFALSE )
    {
        #if ( configNUMBER_OF_CORES == 1 )
            ullPortYieldRequired = pdTRUE;
        #else
            ullPortYieldRequired[ portGET_CORE_ID_FROM_ISR() ] = pdTRUE;
        #endif
    }
    #if ( configNUMBER_OF_CORES > 1 )
        portEXIT_CRITICAL_FROM_ISR(x);
    #endif /* if ( configNUMBER_OF_CORES > 1 ) 条件编译结束 */

    /* 确保所有中断优先级重新生效。 */
    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxInterruptStatus );

    /* 清除中断源后即可使能中断。 */
    configCLEAR_TICK_INTERRUPT();
}

/*-----------------------------------------------------------*/

#if ( configUSE_TASK_FPU_SUPPORT == portTASK_NO_FPU_CONTEXT_BY_DEFAULT )

    void vPortTaskUsesFPU( void )
    {
        /*
         * 任务正在声明其需要 FPU 上下文。设置 FPU 标志
         * （该标志作为任务上下文的一部分保存）。
         */
        #if  ( configNUMBER_OF_CORES == 1 )
            ullPortTaskHasFPUContext = pdTRUE;
        #else
            ullPortTaskHasFPUContext[ portGET_CORE_ID() ] = pdTRUE;
        #endif

        /*
         * 可以考虑在此初始化 FPSR——但在 AArch64 中可能并不需要。
         */
    }

#endif /* configUSE_TASK_FPU_SUPPORT 条件编译结束 */

/*-----------------------------------------------------------*/

void vPortClearInterruptMask( UBaseType_t uxNewMaskValue )
{
    if( uxNewMaskValue == portUNMASK_VALUE )
    {
        /* 解除屏蔽所有中断优先级。 */
        portCLEAR_INTERRUPT_PRIORITIES_MASK();
    }
    else
    {
        __asm volatile (
            "SVC %0 \n"
            :
            : "i" ( portSVC_UNMASK_INTERRUPTS )
            : "memory"
        );
    }
}

void vPortClearInterruptMaskFromISR( UBaseType_t uxNewMaskValue )
{
    __asm volatile (
        "MSR DAIFSET, #2        \n"
        "DSB SY                 \n"
        "ISB SY                 \n"
        "MSR ICC_PMR_EL1, %0    \n"
        "DSB SY                 \n"
        "ISB SY                 \n"
        "MSR DAIFCLR, #2        \n"
        "DSB SY                 \n"
        "ISB SY                 \n"
        :
        : "r" ( uxNewMaskValue )
    );
}
/*-----------------------------------------------------------*/

UBaseType_t uxPortSetInterruptMask( void )
{
    UBaseType_t ullPMRValue;

    /* 使用 SVC，因此可以从 EL0 任务中安全调用。 */
    __asm volatile (
        "svc %1        \n"
        "mov %0, x0    \n"
        : "=r" ( ullPMRValue )
        : "i" ( portSVC_MASK_ALL_INTERRUPTS )
        : "x0", "memory"
    );

    return ullPMRValue;
}

/* EL1/ISR 变体，避免从中断上下文触发 SVC。 */
UBaseType_t uxPortSetInterruptMaskFromISR( void )
{
    UBaseType_t ullPMRValue;

    __asm volatile ( "MRS %0, ICC_PMR_EL1" : "=r" ( ullPMRValue ) );

    if( ullPMRValue != ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) )
    {
        __asm volatile ( "MSR DAIFSET, #2        \n"
                         "DSB SY                 \n"
                         "ISB SY                 \n"
                         "MSR ICC_PMR_EL1, %0    \n"
                         "DSB SY                 \n"
                         "ISB SY                 \n"
                         "MSR DAIFCLR, #2        \n"
                         "DSB SY                 \n"
                         "ISB SY                 \n"
                         ::"r" ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) : "memory" );
    }

    return ullPMRValue;
}

/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

    void vPortValidateInterruptPriority( void )
    {
        /*
         * 若某中断服务程序（ISR）的优先级高于
         * configMAX_SYSCALL_INTERRUPT_PRIORITY，却调用了中断安全的 FreeRTOS API 函数，
         * 则下面的断言将失败。中断安全的 FreeRTOS API 函数*只能*从优先级等于或低于
         * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
         *
         * 数值越小表示逻辑优先级越高，因此中断优先级必须设置为等于或数值上*高于*
         * configMAX_SYSCALL_INTERRUPT_PRIORITY 的值。
         *
         * FreeRTOS 分别维护线程和 ISR API 函数，以确保中断进入尽可能快速、简单。
         */
        uint64_t ullRunningInterruptPriority;
        __asm volatile ( "MRS %0, ICC_RPR_EL1" : "=r" ( ullRunningInterruptPriority ) );
        configASSERT( ullRunningInterruptPriority >= ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );
    }

#endif /* configASSERT_DEFINED 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * 若应用提供了 vApplicationIRQHandler() 的实现，则该函数会被直接调用，
 * 中断进入时不保存 FPU 寄存器；此处提供的 vApplicationFPUSafeIRQHandler()
 * 弱实现仅用于消除链接错误——它实际上永远不会被调用，因此其实现包含
 * 一个必然失败的 configASSERT() 调用。
 *
 * 若应用提供了自己的 vApplicationFPUSafeIRQHandler() 实现，则 portASM.S 中
 * 提供的 vApplicationIRQHandler() 会在调用它之前保存 FPU 寄存器。
 *
 * 因此，若应用作者希望在中断进入时保存 FPU 寄存器，其中断处理程序必须
 * 命名为 vApplicationFPUSafeIRQHandler()；若不希望保存 FPU 寄存器，
 * 则其中断处理程序必须命名为 vApplicationIRQHandler()。
 */
 __attribute__( ( weak ) ) void vApplicationFPUSafeIRQHandler( uint32_t ulICCIAR )
{
    ( void ) ulICCIAR;
    configASSERT( ( volatile void * ) NULL );
}

/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES > 1 )

    /* 哪个核心拥有该锁？保存在特权、可共享 RAM 中。 */
    PRIVILEGED_DATA volatile uint64_t ullOwnedByCore[ portMAX_CORE_COUNT ];
    /* 某个核心拥有的锁计数。 */
    PRIVILEGED_DATA volatile uint64_t ullRecursionCountByLock[ eLockCount ];
    /* 索引 0 用于 ISR 锁，索引 1 用于任务锁。 */
    PRIVILEGED_DATA uint32_t ulGateWord[ eLockCount ];

    void vInterruptCore( uint32_t ulInterruptID, uint8_t ucCoreID )
    {
        uint64_t ulRegVal = 0;
        uint32_t ulCoreMask = ( 1UL << ucCoreID );
        ulRegVal |= ( (ulCoreMask & 0xFFFF) | ( ( ulInterruptID & 0xF ) << 24U ) );
        __asm volatile (
            "str  x0, [ sp, #-0x10 ]!       \n"
            "mov  x0, %0                    \n"
            "svc  %1                        \n"
            "ldr  x0, [ sp ], # 0x10        \n"
            :
            : "r" ( ulRegVal ), "i" ( portSVC_INTERRUPT_CORE )
            : "memory", "w1"
        );
    }

/*-----------------------------------------------------------*/

    static inline void prvSpinUnlock( uint32_t * ulLock )
    {
        /* 保守解锁：保留原有的屏障，以兼容广泛的硬件/FVP。 */
        __asm volatile (
            "dmb sy         \n"
            "mov w1, #0     \n"
            "str w1, [%x0]  \n"
            "sev            \n"
            "dsb sy         \n"
            "isb sy         \n"
            :
            : "r" ( ulLock )
            : "memory", "w1"
        );
    }

/*-----------------------------------------------------------*/

    static inline uint32_t prvSpinTrylock( uint32_t * ulLock )
    {
        /*
         * 保守的 LDXR/STXR 尝试加锁：
         * - 若忙则立即返回 1，并清除独占状态（CLREX）。
         * - 仅在观察到锁空闲时因伪失败重试 STXR。
         * - 成功后执行 DMB，以保持预期的获取语义。
         */
        register uint32_t ulRet;
        __asm volatile (
            "1:                     \n"
            "ldxr w1, [%x1]         \n"
            "cbnz w1, 2f            \n" /* 忙 -> 返回 1 */
            "mov  w2, #1            \n"
            "stxr w3, w2, [%x1]     \n" /* w3 = 状态 */
            "cbnz w3, 1b            \n" /* STXR 失败时重试 */
            "dmb  sy                \n" /* 成功后执行获取屏障 */
            "mov %w0, #0            \n" /* 成功 */
            "b    3f                \n"
            "2:                     \n"
            "clrex                  \n" /* 忙时清除监视器 */
            "mov %w0, #1            \n" /* 忙 */
            "3:                     \n"
            : "=r" ( ulRet )
            : "r" ( ulLock )
            : "memory", "w1", "w2", "w3"
        );

        return ulRet;
    }

/*-----------------------------------------------------------*/

    /* 读取核心间共享的 64 位值。 */
    static inline uint64_t prvGet64( volatile uint64_t * x )
    {
        __asm( "dsb sy" );
        return *x;
    }

/*-----------------------------------------------------------*/

    /* 写入核心间共享的 64 位值。 */
    static inline void prvSet64( volatile uint64_t * x,
                                 uint64_t value )
    {
        *x = value;
        __asm( "dsb sy" );
    }

/*-----------------------------------------------------------*/

    void vPortRecursiveLock( uint8_t ucCoreID,
                             ePortRTOSLock eLockNum,
                             BaseType_t uxAcquire )
    {
        /* 校验核心 ID 和锁编号。 */
        configASSERT( ucCoreID < portMAX_CORE_COUNT );
        configASSERT( eLockNum < eLockCount );

        uint32_t ulLockBit = 1u << eLockNum;

        /* 锁获取。 */
        if( uxAcquire )
        {
            /*
             * 检查自旋锁是否可用。
             * 若自旋锁不可用，检查该核心是否拥有该锁。
             * 若该核心拥有该锁，则将该核心的锁计数加一。
             * 若该核心不拥有该锁，则等待自旋锁。
             */
            if( prvSpinTrylock( &ulGateWord[ eLockNum ] ) != 0 )
            {
                /* 检查该核心是否拥有自旋锁。 */
                if( prvGet64( &ullOwnedByCore[ ucCoreID ] ) & ulLockBit )
                {
                    configASSERT( prvGet64( &ullRecursionCountByLock[ eLockNum ] ) != 255u );
                    prvSet64( &ullRecursionCountByLock[ eLockNum ], ( prvGet64( &ullRecursionCountByLock[ eLockNum ] ) + 1 ) );
                    return;
                }

                /* 将门控字预加载到缓存中。 */
                uint32_t dummy = ulGateWord[ eLockNum ];
                dummy++;

                while( prvSpinTrylock( &ulGateWord[ eLockNum ] ) != 0 )
                {
                   /*
                    * 遵循 Arm 推荐的睡眠方式。
                    * sevl 用于为等待循环做准备。
                    * 由于 sevl 已设置标志，第一次 wfe 会立即唤醒。
                    * 检查锁，若不可用则再次执行 wfe 进入睡眠。
                    * 这保证核心真正进入睡眠。
                    */
                    __asm volatile (
                        "sevl            \n"
                        "1: wfe          \n"
                        "ldr w2, [%x0]   \n"
                        "cbnz w2, 1b     \n"
                        :
                        : "r" ( &ulGateWord[ eLockNum ] )
                        : "memory", "w2"
                    );
                }
            }

            /* 添加屏障，确保继续执行前锁已获取。 */
            __asm__ __volatile__ ( "dmb sy" ::: "memory" );

            /* 断言自旋锁空闲并被获取时锁计数为 0。 */
            configASSERT( prvGet64( &ullRecursionCountByLock[ eLockNum ] ) == 0 );

            /* 将锁计数设为 1。 */
            prvSet64( &ullRecursionCountByLock[ eLockNum ], 1 );
            /* 设置 ullOwnedByCore。 */
            prvSet64( &ullOwnedByCore[ ucCoreID ], ( prvGet64( &ullOwnedByCore[ ucCoreID ] ) | ulLockBit ) );
        }
        /* 锁释放。 */
        else
        {
            /* 断言该锁尚未处于空闲状态。 */
            configASSERT( ( prvGet64( &ullOwnedByCore[ ucCoreID ] ) & ulLockBit ) != 0 );
            configASSERT( prvGet64( &ullRecursionCountByLock[ eLockNum ] ) != 0 );

            /* 将 ullRecursionCountByLock 减 1。 */
            prvSet64( &ullRecursionCountByLock[ eLockNum ], ( prvGet64( &ullRecursionCountByLock[ eLockNum ] ) - 1 ) );

            if( !prvGet64( &ullRecursionCountByLock[ eLockNum ] ) )
            {
                prvSet64( &ullOwnedByCore[ ucCoreID ], ( prvGet64( &ullOwnedByCore[ ucCoreID ] ) & ~ulLockBit ) );
                prvSpinUnlock( &ulGateWord[ eLockNum ] );
                /* 添加屏障，确保继续执行前锁状态已反映。 */
                __asm__ __volatile__ ( "dmb sy" ::: "memory" );
            }
        }
    }

/*-----------------------------------------------------------*/

    uint8_t ucPortGetCoreID( void )
    {
        /*
         * 使用 SVC 获取核心 ID，以便从 EL0 任务调用时是安全的。
         * ISR 和 EL1 代码应使用 ucPortGetCoreIDFromIsr()/portGET_CORE_ID_FROM_ISR()。
         */
        uint8_t ucCoreID;
        __asm volatile (
            "svc %1             \n"
            "mov %w0, w0        \n"
            : "=r" ( ucCoreID )
            : "i" ( portSVC_GET_CORE_ID )
            : "x0", "memory"
        );
        return ucCoreID;
    }

/*-----------------------------------------------------------*/

    uint8_t ucPortGetCoreIDFromIsr ( void ) /* 特权函数 */
    {
        uint64_t ullMpidrEl1;
        __asm volatile ( "MRS %0, MPIDR_EL1" : "=r" ( ullMpidrEl1 ) );

        return ( uint8_t ) ( ullMpidrEl1 & 0xff );
    }

/*------------------------------------------------------------*/
    void FreeRTOS_SGI_Handler( void )
    {
        /* 必须使用最低可能的优先级。 */
        uint64_t ullRunningInterruptPriority;
        __asm volatile ( "MRS %0, ICC_RPR_EL1" : "=r" ( ullRunningInterruptPriority ) );

        configASSERT( ullRunningInterruptPriority == ( portLOWEST_USABLE_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );
        /* 在此点之前不应使能中断。 */
        #if ( configASSERT_DEFINED == 1 )
        {
            uint64_t ullMaskBits;

            __asm volatile ( "mrs %0, DAIF" : "=r" ( ullMaskBits )::"memory" );
            configASSERT( ( ullMaskBits & portDAIF_I ) != 0 );
        }
        #endif /* configASSERT_DEFINED 条件编译结束 */

        /*
         * 在修改调度器结构前设置中断屏蔽。SGI 中断以最低优先级运行，
         * 因此中断不可能已被屏蔽，无需保存和恢复当前屏蔽值。更新 ICCPMR 时
         * 必须关闭 CPU 自身的中断。
         */
        UBaseType_t uxInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

        UBaseType_t uxSavedInterruptStatus = portENTER_CRITICAL_FROM_ISR();
        #if ( configNUMBER_OF_CORES == 1 )
            ullPortYieldRequired = pdTRUE;
        #else
            ullPortYieldRequired[ portGET_CORE_ID_FROM_ISR() ] = pdTRUE;
        #endif
        portEXIT_CRITICAL_FROM_ISR( uxSavedInterruptStatus );
        portCLEAR_INTERRUPT_MASK_FROM_ISR( uxInterruptStatus );
    }

/*-----------------------------------------------------------*/

#endif /* if( configNUMBER_OF_CORES > 1 ) 条件编译结束 */
