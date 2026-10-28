/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2025 Arm Limited and/or its affiliates
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

/*
 * -----------------------------------------------------------
 * ARM CM4 MPU 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
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
#include "mpu_syscall_numbers.h"

#ifndef __ARM_FP
    #error This port can only be used when the project options are configured to enable hardware floating point support.
#endif

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ    configCPU_CLOCK_HZ
    /* 确保 SysTick 与内核使用相同频率的时钟。 */
    #define portNVIC_SYSTICK_CLK      ( 1UL << 2UL )
#else

/*
 * 不修改 SysTick 的时钟方式，以防其与内核时钟不一致。
 */
    #define portNVIC_SYSTICK_CLK    ( 0 )
#endif

/*
 * 使用 MPU 包装器版本 2 时不支持非特权临界区。将该选项默认设为 0，并拒绝显式设置为 1。
 */
#ifndef configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS
    #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
        #define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS    0
    #else
        #warning "configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS is not defined. We recommend defining it to 0 in FreeRTOSConfig.h for better security."
        #define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS    1
    #endif
#else
    #if ( ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1 ) )
        #error configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS is not supported with MPU wrappers version 2 ( configUSE_MPU_WRAPPERS_V1 == 0 ).
    #endif
#endif

/* 所有中断服务程序（ISR）的原型。 */
typedef void ( * portISR_t )( void );

/* 访问和操作 NVIC 所需的常量。 */
#define portNVIC_SYSTICK_CTRL_REG                 ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG                 ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG        ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                        ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SHPR2_REG                        ( *( ( volatile uint32_t * ) 0xe000ed1c ) )
#define portNVIC_SYS_CTRL_STATE_REG               ( *( ( volatile uint32_t * ) 0xe000ed24 ) )
#define portNVIC_MEM_FAULT_ENABLE                 ( 1UL << 16UL )

/*
 * 用于检测 Cortex-M7 r0p0 和 r0p1 内核，并确保 errata 837070
 * 规避措施生效的常量。
 */
#define portCPUID                                 ( *( ( volatile uint32_t * ) 0xE000ed00 ) )
#define portCORTEX_M7_r0p1_ID                     ( 0x410FC271UL )
#define portCORTEX_M7_r0p0_ID                     ( 0x410FC270UL )

/* 访问和操作 MPU 所需的常量。 */
#define portMPU_TYPE_REG                          ( *( ( volatile uint32_t * ) 0xe000ed90 ) )
#define portMPU_REGION_BASE_ADDRESS_REG           ( *( ( volatile uint32_t * ) 0xe000ed9C ) )
#define portMPU_REGION_ATTRIBUTE_REG              ( *( ( volatile uint32_t * ) 0xe000edA0 ) )
#define portMPU_CTRL_REG                          ( *( ( volatile uint32_t * ) 0xe000ed94 ) )
#define portEXPECTED_MPU_TYPE_VALUE               ( configTOTAL_MPU_REGIONS << 8UL )
#define portMPU_ENABLE                            ( 0x01UL )
#define portMPU_BACKGROUND_ENABLE                 ( 1UL << 2UL )
#define portPRIVILEGED_EXECUTION_START_ADDRESS    ( 0UL )
#define portMPU_REGION_VALID                      ( 0x10UL )
#define portMPU_REGION_ENABLE                     ( 0x01UL )
#define portPERIPHERALS_START_ADDRESS             0x40000000UL
#define portPERIPHERALS_END_ADDRESS               0x5FFFFFFFUL

/*
 * 访问和操作 SysTick 及其他 FreeRTOS 中断所需的常量。
 */
#define portNVIC_SYSTICK_INT                      ( 0x00000002UL )
#define portNVIC_SYSTICK_ENABLE                   ( 0x00000001UL )
#define portMIN_INTERRUPT_PRIORITY                ( 255UL )
#define portNVIC_PENDSV_PRI                       ( ( ( uint32_t ) portMIN_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI                      ( ( ( uint32_t ) portMIN_INTERRUPT_PRIORITY ) << 24UL )
#define portNVIC_SVC_PRI                          ( ( ( uint32_t ) configMAX_SYSCALL_INTERRUPT_PRIORITY - 1UL ) << 24UL )

/* 操作 VFP 所需的常量。 */
#define portFPCCR                                 ( ( volatile uint32_t * ) 0xe000ef34UL ) /* 浮点上下文控制寄存器。 */
#define portASPEN_AND_LSPEN_BITS                  ( 0x3UL << 30UL )

/* 建立初始栈所需的常量。 */
#define portINITIAL_XPSR                          ( 0x01000000UL )
#define portINITIAL_EXC_RETURN                    ( 0xfffffffdUL )
#define portINITIAL_CONTROL_IF_UNPRIVILEGED       ( 0x03 )
#define portINITIAL_CONTROL_IF_PRIVILEGED         ( 0x02 )

/* 用于检查 FreeRTOS 中断处理程序安装情况的常量。 */
#define portSCB_VTOR_REG                          ( *( ( portISR_t ** ) 0xE000ED08 ) )
#define portVECTOR_INDEX_SVC                      ( 11 )
#define portVECTOR_INDEX_PENDSV                   ( 14 )

/* 用于检查中断优先级有效性的常量。 */
#define portFIRST_USER_INTERRUPT_NUMBER           ( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16           ( 0xE000E3F0 )
#define portAIRCR_REG                             ( *( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE                       ( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE                       ( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS                     ( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK                   ( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT                        ( 8UL )

/* 系统调用进入和退出期间使用的常量。 */
#define portPSR_STACK_PADDING_MASK                ( 1UL << 9UL )
#define portEXC_RETURN_STACK_FRAME_TYPE_MASK      ( 1UL << 4UL )

/* 在 SVC 处理程序中时，栈中参数的偏移量。 */
#define portOFFSET_TO_LR                          ( 5 )
#define portOFFSET_TO_PC                          ( 6 )
#define portOFFSET_TO_PSR                         ( 7 )


/*
 * 为严格符合 Cortex-M 规范，任务起始地址的 bit0 应清零，
 * 因为该地址在退出 ISR 时被加载到 PC。
 */
#define portSTART_ADDRESS_MASK    ( ( StackType_t ) 0xfffffffeUL )

/* addr 是否位于 [start, end] 地址范围内？ */
#define portIS_ADDRESS_WITHIN_RANGE( addr, start, end ) \
    ( ( ( addr ) >= ( start ) ) && ( ( addr ) <= ( end ) ) )

/* 可用权限是否满足访问请求？ */
#define portIS_AUTHORIZED( accessRequest, permissions ) \
    ( ( ( permissions ) & ( accessRequest ) ) == accessRequest )

/* uint32_t 类型可容纳的最大值。 */
#define portUINT32_MAX    ( ~( ( uint32_t ) 0 ) )

/* 检查 a 与 b 相加是否会导致溢出。 */
#define portADD_UINT32_WILL_OVERFLOW( a, b )    ( ( a ) > ( portUINT32_MAX - ( b ) ) )
/*-----------------------------------------------------------*/

/*
 * 配置所有任务共用的若干标准 MPU 区域。
 */
static void prvSetupMPU( void ) PRIVILEGED_FUNCTION;

/*
 * 返回给定字节数能够容纳的最小 MPU 区域大小。区域大小以应写入该区域属性寄存器的值返回。
 */
static uint32_t prvGetMPURegionSizeSetting( uint32_t ulActualSizeInBytes ) PRIVILEGED_FUNCTION;

/*
 * 配置定时器以产生时钟节拍中断。本文件中的实现为弱符号，
 * 便于应用开发者更换产生节拍中断的定时器。
 */
void vPortSetupTimerInterrupt( void );

/*
 * 标准 FreeRTOS 异常处理程序。
 */
void xPortPendSVHandler( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;
void xPortSysTickHandler( void ) PRIVILEGED_FUNCTION;
void vPortSVCHandler( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/*
 * 通过恢复第一个待运行任务的上下文来启动调度器。
 */
static void prvRestoreContextOfFirstTask( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

/*
 * SVC 处理程序的 C 部分。为便于编码和维护，SVC 处理程序分为汇编入口和 C 包装函数两部分。
 */
void vSVCHandler_C( uint32_t * pulRegisters ) __attribute__( ( noinline ) ) PRIVILEGED_FUNCTION;

/*
 * 使能 VFP 的函数。
 */
static void vPortEnableVFP( void ) __attribute__( ( naked ) );

/**
 * @brief 检查处理器当前是否处于特权状态。
 *
 * @return 若处理器已处于特权状态则返回 1，否则返回 0。
 */
BaseType_t xIsPrivileged( void ) __attribute__( ( naked ) );

/**
 * @brief 通过设置 CONTROL 寄存器的 bit 0 降低特权级别。
 *
 * CONTROL 寄存器的 bit 0 定义线程模式的特权级别。
 *  Bit[0] = 0 --> 处理器以特权状态运行
 *  Bit[0] = 1 --> 处理器以非特权状态运行。
 */
void vResetPrivilege( void ) __attribute__( ( naked ) );

/**
 * @brief 将任务设置为非特权。
 */
void vPortSwitchToUserMode( void );

/**
 * @brief 进入临界区。
 */
#if ( configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1 )
    void vPortEnterCritical( void ) FREERTOS_SYSTEM_CALL;
#else
    void vPortEnterCritical( void ) PRIVILEGED_FUNCTION;
#endif

/**
 * @brief 退出临界区。
 */
#if ( configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1 )
    void vPortExitCritical( void ) FREERTOS_SYSTEM_CALL;
#else
    void vPortExitCritical( void ) PRIVILEGED_FUNCTION;
#endif

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

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

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

/**
 * @brief 触发 SVC 以退出系统调用。
 */
    void vRequestSystemCallExit( void ) __attribute__( ( naked ) ) PRIVILEGED_FUNCTION;

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

/**
 * @brief 建立任务栈，以便从 SVC 返回后重新使用任务栈。
 *
 * @param pulSystemCallStack 触发 SVC 时的当前 SP。
 * @param ulLR SVC 处理程序中链接寄存器（EXC_RETURN）的值。
 */
    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) PRIVILEGED_FUNCTION;

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

/**
 * @brief 检查调用任务是否具有特权。
 *
 * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
 */
BaseType_t xPortIsTaskPrivileged( void ) PRIVILEGED_FUNCTION;
/*-----------------------------------------------------------*/

/*
 * 每个任务在临界嵌套变量中维护各自的中断状态。注意该变量不作为任务上下文的一部分保存，因为上下文切换只能在 uxCriticalNesting 为零时发生。
 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

/*
 * 调度器启动时该变量被设置为 pdTRUE。
 */
    PRIVILEGED_DATA static BaseType_t xSchedulerRunning = pdFALSE;

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

/*
 * 供 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 宏使用，确保
 * 不在优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中
 * 调用 FreeRTOS API 函数。
 */
#if ( configASSERT_DEFINED == 1 )
    static uint8_t ucMaxSysCallPriority = 0;
    static uint32_t ulMaxPRIGROUPValue = 0;
    static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * const ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif /* configASSERT_DEFINED 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * 详见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters,
                                     BaseType_t xRunPrivileged,
                                     xMPU_SETTINGS * xMPUSettings )
{
    if( xRunPrivileged == pdTRUE )
    {
        xMPUSettings->ulTaskFlags |= portTASK_IS_PRIVILEGED_FLAG;
        xMPUSettings->ulContext[ 0 ] = portINITIAL_CONTROL_IF_PRIVILEGED;
    }
    else
    {
        xMPUSettings->ulTaskFlags &= ( ~( portTASK_IS_PRIVILEGED_FLAG ) );
        xMPUSettings->ulContext[ 0 ] = portINITIAL_CONTROL_IF_UNPRIVILEGED;
    }

    xMPUSettings->ulContext[ 1 ] = 0x04040404;                                        /* r4 寄存器。 */
    xMPUSettings->ulContext[ 2 ] = 0x05050505;                                        /* r5 寄存器。 */
    xMPUSettings->ulContext[ 3 ] = 0x06060606;                                        /* r6 寄存器。 */
    xMPUSettings->ulContext[ 4 ] = 0x07070707;                                        /* r7 寄存器。 */
    xMPUSettings->ulContext[ 5 ] = 0x08080808;                                        /* r8 寄存器。 */
    xMPUSettings->ulContext[ 6 ] = 0x09090909;                                        /* r9 寄存器。 */
    xMPUSettings->ulContext[ 7 ] = 0x10101010;                                        /* r10 寄存器。 */
    xMPUSettings->ulContext[ 8 ] = 0x11111111;                                        /* r11 寄存器。 */
    xMPUSettings->ulContext[ 9 ] = portINITIAL_EXC_RETURN;                            /* EXC_RETURN 值。 */

    xMPUSettings->ulContext[ 10 ] = ( uint32_t ) ( pxTopOfStack - 8 );                /* PSP 指向硬件保存的栈帧。 */
    xMPUSettings->ulContext[ 11 ] = ( uint32_t ) pvParameters;                        /* r0 寄存器。 */
    xMPUSettings->ulContext[ 12 ] = 0x01010101;                                       /* r1 寄存器。 */
    xMPUSettings->ulContext[ 13 ] = 0x02020202;                                       /* r2 寄存器。 */
    xMPUSettings->ulContext[ 14 ] = 0x03030303;                                       /* r3 寄存器。 */
    xMPUSettings->ulContext[ 15 ] = 0x12121212;                                       /* r12 寄存器。 */
    xMPUSettings->ulContext[ 16 ] = 0;                                                /* LR 寄存器。 */
    xMPUSettings->ulContext[ 17 ] = ( ( uint32_t ) pxCode ) & portSTART_ADDRESS_MASK; /* PC 寄存器。 */
    xMPUSettings->ulContext[ 18 ] = portINITIAL_XPSR;                                 /* xPSR 寄存器。 */

    #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
    {
        /* 确保系统调用栈双字对齐。 */
        xMPUSettings->xSystemCallStackInfo.pulSystemCallStack = &( xMPUSettings->xSystemCallStackInfo.ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE - 1 ] );
        xMPUSettings->xSystemCallStackInfo.pulSystemCallStack = ( uint32_t * ) ( ( uint32_t ) ( xMPUSettings->xSystemCallStackInfo.pulSystemCallStack ) &
                                                                                 ( uint32_t ) ( ~( portBYTE_ALIGNMENT_MASK ) ) );

        /* 仅在系统调用期间该值才非 NULL。 */
        xMPUSettings->xSystemCallStackInfo.pulTaskStack = NULL;
    }
    #endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

    return &( xMPUSettings->ulContext[ 19 ] );
}
/*-----------------------------------------------------------*/

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

    void vPortSVCHandler( void ) /* __attribute__( ( naked ) ) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile
        (
            ".syntax unified                \n"
            ".extern vSVCHandler_C          \n"
            ".extern vSystemCallEnter       \n"
            ".extern vSystemCallExit        \n"
            "                               \n"
            "tst lr, #4                     \n"
            "ite eq                         \n"
            "mrseq r0, msp                  \n"
            "mrsne r0, psp                  \n"
            "                               \n"
            "ldr r1, [r0, #24]              \n"
            "ldrb r2, [r1, #-2]             \n"
            "cmp r2, %0                     \n"
            "blt syscall_enter              \n"
            "cmp r2, %1                     \n"
            "beq syscall_exit               \n"
            "b vSVCHandler_C                \n"
            "                               \n"
            "syscall_enter:                 \n"
            "    mov r1, lr                 \n"
            "    b vSystemCallEnter         \n"
            "                               \n"
            "syscall_exit:                  \n"
            "    mov r1, lr                 \n"
            "    b vSystemCallExit          \n"
            "                               \n"
            : /* 无输出。 */
            : "i" ( NUM_SYSTEM_CALLS ), "i" ( portSVC_SYSTEM_CALL_EXIT )
            : "r0", "r1", "r2", "memory"
        );
    }

#else /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

    void vPortSVCHandler( void )
    {
        /* 假定此前使用的是 PSP。 */
        __asm volatile
        (
            #ifndef USE_PROCESS_STACK /* 若 main() 使用进程栈，则不需要此代码。 */
                "   tst lr, #4                      \n"
                "   ite eq                          \n"
                "   mrseq r0, msp                   \n"
                "   mrsne r0, psp                   \n"
            #else
                "   mrs r0, psp                     \n"
            #endif
            "   b %0                            \n"
            ::"i" ( vSVCHandler_C ) : "r0", "memory"
        );
    }

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */
/*-----------------------------------------------------------*/

void vSVCHandler_C( uint32_t * pulParam ) /* PRIVILEGED_FUNCTION（特权函数） */
{
    uint8_t ucSVCNumber;
    uint32_t ulPC;

    #if ( ( configUSE_MPU_WRAPPERS_V1 == 1 ) && ( configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY == 1 ) )
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
    #endif /* configUSE_MPU_WRAPPERS_V1 == 1 且 configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY == 1（使用 V1 包装器且仅允许内核发起系统调用） */

    /*
     * 栈中包含：r0、r1、r2、r3、r12、LR、PC 和 xPSR。第一个参数（r0）为 pulParam[ 0 ]。
     */
    ulPC = pulParam[ portOFFSET_TO_PC ];
    ucSVCNumber = ( ( uint8_t * ) ulPC )[ -2 ];

    switch( ucSVCNumber )
    {
        case portSVC_START_SCHEDULER:
            prvRestoreContextOfFirstTask();
            break;

        case portSVC_YIELD:
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;

            /*
             * 通常不需要屏障指令，
             * 但需确保代码完全符合
             * 该体系结构规定的行为。
             */
            __asm volatile ( "dsb" ::: "memory" );
            __asm volatile ( "isb" );

            break;

    #if ( configUSE_MPU_WRAPPERS_V1 == 1 )
        #if ( configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY == 1 )
            case portSVC_RAISE_PRIVILEGE: /*
             * 仅当 SVC 由任一系统调用发起时，
             * 才提升特权。
             */

                if( ( ulPC >= ( uint32_t ) __syscalls_flash_start__ ) &&
                    ( ulPC <= ( uint32_t ) __syscalls_flash_end__ ) )
                {
                    __asm volatile
                    (
                        "   mrs r1, control     \n" /* 获取当前 CONTROL 值。 */
                        "   bic r1, #1          \n" /* 设置特权位。 */
                        "   msr control, r1     \n" /* 写回新的 CONTROL 值。 */
                        ::: "r1", "memory"
                    );
                }

                break;
        #else /* configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY == 1（仅允许内核发起系统调用） */
            case portSVC_RAISE_PRIVILEGE:
                __asm volatile
                (
                    "   mrs r1, control     \n" /* 获取当前 CONTROL 值。 */
                    "   bic r1, #1          \n" /* 设置特权位。 */
                    "   msr control, r1     \n" /* 写回新的 CONTROL 值。 */
                    ::: "r1", "memory"
                );
                break;
        #endif /* configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY == 1（仅允许内核发起系统调用） */
    #endif /* configUSE_MPU_WRAPPERS_V1 == 1（使用 V1 包装器） */

        default: /* 未知的 SVC 调用。 */
            break;
    }
}
/*-----------------------------------------------------------*/

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

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
             * - 无 FPU：基本帧（R0-R3、R12、LR、PC 和 xPSR）= 8 个字。
             * - 有 FPU（惰性压栈）：基本帧 + S0–S15 + FPSCR + 保留字 = 26 个字。
             */
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

            /* 在系统调用栈上为栈帧分配空间。 */
            pulSystemCallStack = pulSystemCallStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulSystemCallStack[ i ] = pulTaskStack[ i ];
            }

            /* 在线程模式下使用 pulSystemCallStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulSystemCallStack ) );

            /* 在系统调用执行期间提升特权。 */
            __asm volatile (
                " mrs r1, control     \n" /* 获取当前 CONTROL 值。 */
                " bic r1, #1          \n" /* 清除 nPRIV 位。 */
                " msr control, r1     \n" /* 写回新的 CONTROL 值。 */
                ::: "r1", "memory"
                );

            /*
             * 记住退出系统调用时应复制栈帧的位置。
             */
            pxMpuSettings->xSystemCallStackInfo.pulTaskStack = pulTaskStack + ulHardwareSavedExceptionFrameSize;

            /*
             * 保存触发 SVC 之前的链接寄存器值。
             * 其中包含系统调用入口点的调用者地址（即 MPU_<API> 的调用者）。
             * 退出系统调用时需要恢复该值。
             */
            pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry = pulTaskStack[ portOFFSET_TO_LR ];

            /* 从本处理程序返回后开始执行系统调用。 */
            pulSystemCallStack[ portOFFSET_TO_PC ] = uxSystemCallImplementations[ ucSystemCallNumber ];
            /*
             * 请求在系统调用完成后退出系统调用。
             */
            pulSystemCallStack[ portOFFSET_TO_LR ] = ( uint32_t ) vRequestSystemCallExit;

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
        }
    }

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */
/*-----------------------------------------------------------*/

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

    void vRequestSystemCallExit( void ) /* __attribute__( ( naked ) ) PRIVILEGED_FUNCTION（裸函数，特权函数） */
    {
        __asm volatile ( "svc %0 \n" ::"i" ( portSVC_SYSTEM_CALL_EXIT ) : "memory" );
    }

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */
/*-----------------------------------------------------------*/

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

    void vSystemCallExit( uint32_t * pulSystemCallStack,
                          uint32_t ulLR ) /* PRIVILEGED_FUNCTION（特权函数） */
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
             * - 无 FPU：基本帧（R0-R3、R12、LR、PC 和 xPSR）= 8 个字。
             * - 有 FPU（惰性压栈）：基本帧 + S0–S15 + FPSCR + 保留字 = 26 个字。
             */
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

            /* 在任务栈上为栈帧分配空间。 */
            pulTaskStack = pulTaskStack - ulHardwareSavedExceptionFrameSize;

            /* 复制栈帧。 */
            for( i = 0; i < ulHardwareSavedExceptionFrameSize; i++ )
            {
                pulTaskStack[ i ] = pulSystemCallStack[ i ];
            }

            /* 在线程模式下使用 pulTaskStack。 */
            __asm volatile ( "msr psp, %0" : : "r" ( pulTaskStack ) );

            /* 返回线程模式前降低特权。 */
            __asm volatile (
                " mrs r1, control     \n" /* 获取当前 CONTROL 值。 */
                " orr r1, #1          \n" /* 设置 nPRIV 位。 */
                " msr control, r1     \n" /* 写回新的 CONTROL 值。 */
                ::: "r1", "memory"
                );

            /*
             * 返回系统调用入口点的调用者（即 MPU_<API> 的调用者）。
             */
            pulTaskStack[ portOFFSET_TO_PC ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;
            /* 确保 LR 具有有效值。 */
            pulTaskStack[ portOFFSET_TO_LR ] = pxMpuSettings->xSystemCallStackInfo.ulLinkRegisterAtSystemCallEntry;

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
        }
    }

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */
/*-----------------------------------------------------------*/

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
/*-----------------------------------------------------------*/

static void prvRestoreContextOfFirstTask( void )
{
    __asm volatile
    (
        " ldr r0, =0xE000ED08                   \n" /* 使用 NVIC 偏移寄存器定位栈。 */
        " ldr r0, [r0]                          \n"
        " ldr r0, [r0]                          \n"
        " msr msp, r0                           \n" /* 将 MSP 重新设置为栈起始地址。 */
        "                                       \n"
        /* ------------ 配置 MPU。------------ */
        " ldr r3, =pxCurrentTCB                 \n" /* r3 = &pxCurrentTCB（当前 TCB 的地址）。 */
        " ldr r2, [r3]                          \n" /* r2 = pxCurrentTCB（当前任务的 TCB）。 */
        " add r2, r2, #4                        \n" /* r2 = TCB 中的第二项，即 xMPUSettings。 */
        "                                       \n"
        " dmb                                   \n" /* 在禁用 MPU 前完成未完成的传输。 */
        " ldr r0, =0xe000ed94                   \n" /* MPU_CTRL 寄存器。 */
        " ldr r3, [r0]                          \n" /* 读取 MPU_CTRL 的值。 */
        " bic r3, #1                            \n" /* r3 = r3 & ~1，即清除 r3 的 bit 0。 */
        " str r3, [r0]                          \n" /* 禁用 MPU。 */
        "                                       \n"
        " ldr r0, =0xe000ed9c                   \n" /* 区域基地址寄存器（RBAR）。 */
        " ldmia r2!, {r4-r11}                   \n" /* 读取 4 组 MPU 寄存器 [MPU 区域 #0 - 3]。 */
        " stmia r0, {r4-r11}                    \n" /* 写入 4 组 MPU 寄存器 [MPU 区域 #0 - 3]。 */
        "                                       \n"
        #if ( configTOTAL_MPU_REGIONS == 16 )
            " ldmia r2!, {r4-r11}                   \n" /* 读取 4 组 MPU 寄存器 [MPU 区域 #4 - 8]。 */
            " stmia r0, {r4-r11}                    \n" /* 写入 4 组 MPU 寄存器 [MPU 区域 #4 - 8]。 */
            " ldmia r2!, {r4-r11}                   \n" /* 读取 4 组 MPU 寄存器 [MPU 区域 #9 - 12]。 */
            " stmia r0, {r4-r11}                    \n" /* 写入 4 组 MPU 寄存器 [MPU 区域 #9 - 12]。 */
        #endif /* configTOTAL_MPU_REGIONS == 16（即 16 个区域）。 */
        "                                       \n"
        " ldr r0, =0xe000ed94                   \n" /* MPU_CTRL 寄存器。 */
        " ldr r3, [r0]                          \n" /* 读取 MPU_CTRL 的值。 */
        " orr r3, #1                            \n" /* r3 = r3 | 1，即设置 r3 的 bit 0。 */
        " str r3, [r0]                          \n" /* 使能 MPU。 */
        " dsb                                   \n" /* 继续执行前强制完成内存写入。 */
        "                                       \n"
        /* ---------- 恢复上下文。---------- */
        " ldr r3, =pxCurrentTCB                 \n" /* r3 = &pxCurrentTCB（当前 TCB 的地址）。 */
        " ldr r2, [r3]                          \n" /* r2 = pxCurrentTCB（当前任务的 TCB）。 */
        " ldr r1, [r2]                          \n" /* r1 = TCB 中保存上下文的位置。 */
        "                                       \n"
        " ldmdb r1!, {r0, r4-r11}               \n" /* r0 包含硬件保存上下文之后的 PSP。r4-r11 包含硬件保存的上下文。 */
        " msr psp, r0                           \n"
        " stmia r0, {r4-r11}                    \n" /* 将硬件保存的上下文复制到任务栈上。 */
        " ldmdb r1!, {r3-r11, lr}               \n" /* r3 包含 CONTROL 寄存器。r4-r11 和 LR 已恢复。 */
        " msr control, r3                       \n"
        " str r1, [r2]                          \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
        "                                       \n"
        " mov r0, #0                            \n"
        " msr basepri, r0                       \n"
        " bx lr                                 \n"
        "                                       \n"
        " .ltorg                                \n" /* 汇编当前文字池（literal pool），以避免链接时优化（LTO）出现偏移越界错误。 */
    );
}
/*-----------------------------------------------------------*/

/*
 * 详见头文件中的说明。
 */
BaseType_t xPortStartScheduler( void )
{
    /*
     * Errata 837070 的规避措施只能在内核为 Cortex-M7 r0p0 和 r0p1 时启用。
     */
    #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
        configASSERT( ( portCPUID == portCORTEX_M7_r0p1_ID ) || ( portCPUID == portCORTEX_M7_r0p0_ID ) );
    #else

        /*
         * 在 Cortex-M7 r0p0 或 r0p1 内核上使用此移植层时，请在 FreeRTOSConfig.h
         * 中将 configENABLE_ERRATA_837070_WORKAROUND 定义为 1。
         */
        configASSERT( portCPUID != portCORTEX_M7_r0p1_ID );
        configASSERT( portCPUID != portCORTEX_M7_r0p0_ID );
    #endif

    /*
     * 应用可通过以下方式之一安装 FreeRTOS 中断处理程序：
     * 1. 直接路由——分别为 SVCall 和 PendSV 中断安装 vPortSVCHandler
     *    和 xPortPendSVHandler 函数。
     * 2. 间接路由——为 SVCall 和 PendSV 中断安装独立的处理程序，
     *    并将程序控制从这些处理程序转发到 vPortSVCHandler 和
     *    xPortPendSVHandler 函数。
     *
     * 使用间接路由的应用必须在 FreeRTOSConfig.h 中将
     * configCHECK_HANDLER_INSTALLATION 设为 0。当
     * configCHECK_HANDLER_INSTALLATION 为 1 时，此处校验直接路由；
     * 条件允许时应优先采用直接路由。
     */
    #if ( configCHECK_HANDLER_INSTALLATION == 1 )
    {
        const portISR_t * const pxVectorTable = portSCB_VTOR_REG;

        /*
         * 校验应用已正确安装 SVCall 和 PendSV 中断的 FreeRTOS
         * 处理程序。此处不检查 SysTick 处理程序的安装情况，因为应用可能
         * 通过重写弱函数 vPortSetupTimerInterrupt()，改用 SysTick 之外的
         * 定时器驱动 RTOS 时钟节拍。
         *
         * 此处断言失败表明 FreeRTOS 处理程序安装不正确。有关安装
         * FreeRTOS 处理程序的帮助，请参见
         * https://www.freertos.org/Why-FreeRTOS/FAQs。
         *
         * 对于中断向量表地址可配置的系统，若 VTOR 未正确指向
         * 应用的向量表，此处也可能出现断言失败甚至系统故障。
         */
        configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == vPortSVCHandler );
        configASSERT( pxVectorTable[ portVECTOR_INDEX_PENDSV ] == xPortPendSVHandler );
    }
    #endif /* configCHECK_HANDLER_INSTALLATION 条件编译结束 */

    #if ( configASSERT_DEFINED == 1 )
    {
        volatile uint8_t ucOriginalPriority;
        volatile uint32_t ulImplementedPrioBits = 0;
        volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
        volatile uint8_t ucMaxPriorityValue;

        /*
         * 确定可从中调用 ISR 安全 FreeRTOS API 函数的最高优先级。
         * ISR 安全函数是以“FromISR”结尾的函数。FreeRTOS 维护独立的线程和
         * ISR API 函数，以确保中断进入尽可能快速、简单。
         *
         * 保存即将被改写的原中断优先级值。
         */
        ucOriginalPriority = *pucFirstUserPriorityRegister;

        /*
         * 确定可用的优先级位数。先向所有可能的位写入 1。
         */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

        /* 回读该值，查看哪些位保持为 1。 */
        ucMaxPriorityValue = *pucFirstUserPriorityRegister;

        /* 对最大系统调用优先级施加相同的掩码。 */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /*
         * 检查在考虑硬件支持的优先级位数后，最大系统调用优先级非零。
         * 优先级 0 无效，因为将 BASEPRI 寄存器设为 0 会使所有中断解除屏蔽，
         * 且优先级为 0 的中断无法使用 BASEPRI 屏蔽。
         * 参见 https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
         */
        configASSERT( ucMaxSysCallPriority );

        /*
         * 检查 configMAX_SYSCALL_INTERRUPT_PRIORITY 中
         * 硬件未实现的位为 0。
         */
        configASSERT( ( configMAX_SYSCALL_INTERRUPT_PRIORITY & ( ~ucMaxPriorityValue ) ) == 0U );

        /*
         * 根据回读的位数计算可接受的最大优先级分组值。
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
             * 不存在子优先级。因此最低有效位始终用于子优先级，
             * 共有 128 个抢占优先级和 2 个子优先级。
             *
             * 这在某些情况下可能引起困惑——例如，若
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 设为 5，则优先级 5 和 4
             * 的中断都会在临界区中被屏蔽，因为它们处于同一抢占优先级。
             * 这可能显得费解，因为 4 的优先级高于（数值上小于）
             * configMAX_SYSCALL_INTERRUPT_PRIORITY，本不应被屏蔽。
             * 反之，若将 configMAX_SYSCALL_INTERRUPT_PRIORITY 设为 4，
             * 这种困惑便不会发生，而行为保持不变。
             *
             * 以下断言确保 configMAX_SYSCALL_INTERRUPT_PRIORITY 中的
             * 子优先级位为 0，以避免上述困惑。
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

        /*
         * 将先前被改写的中断优先级寄存器恢复为原值。
         */
        *pucFirstUserPriorityRegister = ucOriginalPriority;
    }
    #endif /* configASSERT_DEFINED 条件编译结束 */

    /*
     * 将 PendSV 和 SysTick 配置为最低优先级中断，并为 SVCall 配置足够的抢占优先级。
     */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
    portNVIC_SHPR2_REG = portNVIC_SVC_PRI;

    /* 配置 MPU 中所有任务共用的区域。 */
    prvSetupMPU();

    /*
     * 启动产生时钟节拍 ISR 的定时器。此处中断已处于禁用状态。
     */
    vPortSetupTimerInterrupt();

    /* 初始化临界区嵌套计数，为第一个任务做好准备。 */
    uxCriticalNesting = 0;

    #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
    {
        xSchedulerRunning = pdTRUE;
    }
    #endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */

    /* 确保 VFP 已使能——无论如何它都应已使能。 */
    vPortEnableVFP();

    /* 始终采用惰性保存。 */
    *( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;

    /*
     * 启动第一个任务。这还会清除表示 FPU 正在使用的位，以防在调度器
     * 启动前使用了 FPU——否则会在 SVC 栈中不必要地预留空间用于 FPU
     * 寄存器的惰性保存。
     */
    __asm volatile (
        " ldr r0, =0xE000ED08   \n" /* 使用 NVIC 偏移寄存器定位栈。 */
        " ldr r0, [r0]          \n"
        " ldr r0, [r0]          \n"
        " msr msp, r0           \n" /* 将 MSP 重新设置为栈起始地址。 */
        " mov r0, #0            \n" /* 清除表示 FPU 正在使用的位，参见上面的注释。 */
        " msr control, r0       \n"
        " cpsie i               \n" /* 全局使能中断。 */
        " cpsie f               \n"
        " dsb                   \n"
        " isb                   \n"
        " svc %0                \n" /* 通过系统调用启动首个任务。 */
        " nop                   \n"
        " .ltorg                \n"
        ::"i" ( portSVC_START_SCHEDULER ) : "memory" );

    /* 不应到达此处！ */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在无返回目标的移植层中不实现该函数。
     * 人为触发断言。
     */
    configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    #if ( configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1 )
        if( portIS_PRIVILEGED() == pdFALSE )
        {
            portRAISE_PRIVILEGE();
            portMEMORY_BARRIER();

            portDISABLE_INTERRUPTS();
            uxCriticalNesting++;
            portMEMORY_BARRIER();

            portRESET_PRIVILEGE();
            portMEMORY_BARRIER();
        }
        else
        {
            portDISABLE_INTERRUPTS();
            uxCriticalNesting++;
        }
    #else /* configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1（允许非特权临界区） */
        portDISABLE_INTERRUPTS();
        uxCriticalNesting++;
    #endif /* configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1（允许非特权临界区） */
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    #if ( configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1 )
        if( portIS_PRIVILEGED() == pdFALSE )
        {
            portRAISE_PRIVILEGE();
            portMEMORY_BARRIER();

            configASSERT( uxCriticalNesting );
            uxCriticalNesting--;

            if( uxCriticalNesting == 0 )
            {
                portENABLE_INTERRUPTS();
            }

            portMEMORY_BARRIER();

            portRESET_PRIVILEGE();
            portMEMORY_BARRIER();
        }
        else
        {
            configASSERT( uxCriticalNesting );
            uxCriticalNesting--;

            if( uxCriticalNesting == 0 )
            {
                portENABLE_INTERRUPTS();
            }
        }
    #else /* configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1（允许非特权临界区） */
        configASSERT( uxCriticalNesting );
        uxCriticalNesting--;

        if( uxCriticalNesting == 0 )
        {
            portENABLE_INTERRUPTS();
        }
    #endif /* configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS == 1（允许非特权临界区） */
}
/*-----------------------------------------------------------*/

void xPortPendSVHandler( void )
{
    /* 这是一个裸函数。 */

    __asm volatile
    (
        " ldr r3, =pxCurrentTCB                 \n" /* r3 = &pxCurrentTCB（当前 TCB 的地址）。 */
        " ldr r2, [r3]                          \n" /* r2 = pxCurrentTCB（当前任务的 TCB）。 */
        " ldr r1, [r2]                          \n" /* r1 = 保存上下文的位置。 */
        "                                       \n"
        /* ------------ 保存上下文。----------- */
        " mrs r3, control                       \n"
        " mrs r0, psp                           \n"
        " isb                                   \n"
        "                                       \n"
        " add r0, r0, #0x20                     \n" /* 将 r0 移动到保存 s0 的位置。 */
        " tst lr, #0x10                         \n"
        " ittt eq                               \n"
        " vstmiaeq r1!, {s16-s31}               \n" /* 保存 s16-s31。 */
        " vldmiaeq r0, {s0-s16}                 \n" /* 将硬件保存的 FP 上下文复制到 s0-s16。 */
        " vstmiaeq r1!, {s0-s16}                \n" /* 保存硬件保存的 FP 上下文。 */
        " sub r0, r0, #0x20                     \n" /* 将 r0 重新设置为硬件保存上下文的位置。 */
        "                                       \n"
        " stmia r1!, {r3-r11, lr}               \n" /* 保存 CONTROL 寄存器、r4-r11 和 LR。 */
        " ldmia r0, {r4-r11}                    \n" /* 将硬件保存的上下文复制到 r4-r11。 */
        " stmia r1!, {r0, r4-r11}               \n" /* 保存原始 PSP（硬件保存上下文之后）以及硬件保存的上下文。 */
        " str r1, [r2]                          \n" /* 将恢复上下文的位置作为 TCB 的第一个成员保存。 */
        "                                       \n"
        /* ---------- 选择下一个任务。--------- */
        " mov r0, %0                            \n"
        #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
            " cpsid i                               \n" /* ARM Cortex-M7 r0p1 Errata 837070 的规避措施。 */
        #endif
        " msr basepri, r0                       \n"
        " dsb                                   \n"
        " isb                                   \n"
        #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
            " cpsie i                               \n" /* ARM Cortex-M7 r0p1 Errata 837070 的规避措施。 */
        #endif
        " bl vTaskSwitchContext                 \n"
        " mov r0, #0                            \n"
        " msr basepri, r0                       \n"
        "                                       \n"
        /* ------------ 配置 MPU。------------ */
        " ldr r3, =pxCurrentTCB                 \n" /* r3 = &pxCurrentTCB（当前 TCB 的地址）。 */
        " ldr r2, [r3]                          \n" /* r2 = pxCurrentTCB（当前任务的 TCB）。 */
        " add r2, r2, #4                        \n" /* r2 = TCB 中的第二项，即 xMPUSettings。 */
        "                                       \n"
        " dmb                                   \n" /* 在禁用 MPU 前完成未完成的传输。 */
        " ldr r0, =0xe000ed94                   \n" /* MPU_CTRL 寄存器。 */
        " ldr r3, [r0]                          \n" /* 读取 MPU_CTRL 的值。 */
        " bic r3, #1                            \n" /* r3 = r3 & ~1，即清除 r3 的 bit 0。 */
        " str r3, [r0]                          \n" /* 禁用 MPU。 */
        "                                       \n"
        " ldr r0, =0xe000ed9c                   \n" /* 区域基地址寄存器（RBAR）。 */
        " ldmia r2!, {r4-r11}                   \n" /* 读取 4 组 MPU 寄存器 [MPU 区域 #0 - 3]。 */
        " stmia r0, {r4-r11}                    \n" /* 写入 4 组 MPU 寄存器 [MPU 区域 #0 - 3]。 */
        "                                       \n"
        #if ( configTOTAL_MPU_REGIONS == 16 )
            " ldmia r2!, {r4-r11}                   \n" /* 读取 4 组 MPU 寄存器 [MPU 区域 #4 - 7]。 */
            " stmia r0, {r4-r11}                    \n" /* 写入 4 组 MPU 寄存器 [MPU 区域 #4 - 7]。 */
            " ldmia r2!, {r4-r11}                   \n" /* 读取 4 组 MPU 寄存器 [MPU 区域 #8 - 11]。 */
            " stmia r0, {r4-r11}                    \n" /* 写入 4 组 MPU 寄存器 [MPU 区域 #8 - 11]。 */
        #endif /* configTOTAL_MPU_REGIONS == 16（即 16 个区域）。 */
        "                                       \n"
        " ldr r0, =0xe000ed94                   \n" /* MPU_CTRL 寄存器。 */
        " ldr r3, [r0]                          \n" /* 读取 MPU_CTRL 的值。 */
        " orr r3, #1                            \n" /* r3 = r3 | 1，即设置 r3 的 bit 0。 */
        " str r3, [r0]                          \n" /* 使能 MPU。 */
        " dsb                                   \n" /* 继续执行前强制完成内存写入。 */
        "                                       \n"
        /* ---------- 恢复上下文。---------- */
        " ldr r3, =pxCurrentTCB                 \n" /* r3 = &pxCurrentTCB（当前 TCB 的地址）。 */
        " ldr r2, [r3]                          \n" /* r2 = pxCurrentTCB（当前任务的 TCB）。 */
        " ldr r1, [r2]                          \n" /* r1 = TCB 中保存上下文的位置。 */
        "                                       \n"
        " ldmdb r1!, {r0, r4-r11}               \n" /* r0 包含硬件保存上下文之后的 PSP。r4-r11 包含硬件保存的上下文。 */
        " msr psp, r0                           \n"
        " stmia r0!, {r4-r11}                   \n" /* 将硬件保存的上下文复制到任务栈上。 */
        " ldmdb r1!, {r3-r11, lr}               \n" /* r3 包含 CONTROL 寄存器。r4-r11 和 LR 已恢复。 */
        " msr control, r3                       \n"

        " tst lr, #0x10                         \n"
        " ittt eq                               \n"
        " vldmdbeq r1!, {s0-s16}                \n" /* s0-s16 中为硬件保存的 FP 上下文。 */
        " vstmiaeq r0!, {s0-s16}                \n" /* 将硬件保存的 FP 上下文复制到任务栈上。 */
        " vldmdbeq r1!, {s16-s31}               \n" /* 恢复 s16-s31。 */

        " str r1, [r2]                          \n" /* 将下次保存上下文的位置作为 TCB 的第一个成员保存。 */
        " bx lr                                 \n"
        "                                       \n"
        " .ltorg                                \n" /* 汇编当前文字池（literal pool），以避免链接时优化（LTO）出现偏移越界错误。 */
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
    );
}
/*-----------------------------------------------------------*/

void xPortSysTickHandler( void )
{
    uint32_t ulDummy;

    ulDummy = portSET_INTERRUPT_MASK_FROM_ISR();
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
    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulDummy );
}
/*-----------------------------------------------------------*/

/*
 * 配置 SysTick 定时器，以所需频率产生时钟节拍中断。
 */
__attribute__( ( weak ) ) void vPortSetupTimerInterrupt( void )
{
    /* 停止并清除 SysTick。 */
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* 配置 SysTick 以按请求的速率产生中断。 */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK | portNVIC_SYSTICK_INT | portNVIC_SYSTICK_ENABLE );
}
/*-----------------------------------------------------------*/

/* 这是一个裸函数。 */
static void vPortEnableVFP( void )
{
    __asm volatile
    (
        "   ldr.w r0, =0xE000ED88       \n" /* FPU 使能位位于 CPACR 中。 */
        "   ldr r1, [r0]                \n"
        "                               \n"
        "   orr r1, r1, #( 0xf << 20 )  \n" /* 使能 CP10 和 CP11 协处理器，然后写回。 */
        "   str r1, [r0]                \n"
        "   bx r14                      \n"
        "   .ltorg                      \n"
    );
}
/*-----------------------------------------------------------*/

static void prvSetupMPU( void )
{
    #if defined( __ARMCC_VERSION )

        /*
         * 当这些变量在代码中定义而非由链接脚本导出时的声明。
         */
        extern uint32_t * __privileged_functions_start__;
        extern uint32_t * __privileged_functions_end__;
        extern uint32_t * __FLASH_segment_start__;
        extern uint32_t * __FLASH_segment_end__;
        extern uint32_t * __privileged_data_start__;
        extern uint32_t * __privileged_data_end__;
    #else
        /* 当这些变量由链接脚本导出时的声明。 */
        extern uint32_t __privileged_functions_start__[];
        extern uint32_t __privileged_functions_end__[];
        extern uint32_t __FLASH_segment_start__[];
        extern uint32_t __FLASH_segment_end__[];
        extern uint32_t __privileged_data_start__[];
        extern uint32_t __privileged_data_end__[];
    #endif /* __ARMCC_VERSION 定义条件 */

    /* 允许的区域数量只有 8 或 16。 */
    configASSERT( ( configTOTAL_MPU_REGIONS == 8 ) || ( configTOTAL_MPU_REGIONS == 16 ) );

    /* 确保 configTOTAL_MPU_REGIONS 配置正确。 */
    configASSERT( portMPU_TYPE_REG == portEXPECTED_MPU_TYPE_VALUE );

    /* 检查预期的 MPU 是否存在。 */
    if( portMPU_TYPE_REG == portEXPECTED_MPU_TYPE_VALUE )
    {
        /* 首先配置非特权 Flash 区域，使非特权代码只能只读访问。 */
        portMPU_REGION_BASE_ADDRESS_REG = ( ( uint32_t ) __FLASH_segment_start__ ) | /* 基地址。 */
                                          ( portMPU_REGION_VALID ) |
                                          ( portUNPRIVILEGED_FLASH_REGION );

        portMPU_REGION_ATTRIBUTE_REG = ( portMPU_REGION_READ_ONLY ) |
                                       ( ( configTEX_S_C_B_FLASH & portMPU_RASR_TEX_S_C_B_MASK ) << portMPU_RASR_TEX_S_C_B_LOCATION ) |
                                       ( prvGetMPURegionSizeSetting( ( uint32_t ) __FLASH_segment_end__ - ( uint32_t ) __FLASH_segment_start__ ) ) |
                                       ( portMPU_REGION_ENABLE );

        /*
         * 配置特权 Flash 区域，仅允许特权访问。内核代码放置在这里。
         */
        portMPU_REGION_BASE_ADDRESS_REG = ( ( uint32_t ) __privileged_functions_start__ ) | /* 基地址。 */
                                          ( portMPU_REGION_VALID ) |
                                          ( portPRIVILEGED_FLASH_REGION );

        portMPU_REGION_ATTRIBUTE_REG = ( portMPU_REGION_PRIVILEGED_READ_ONLY ) |
                                       ( ( configTEX_S_C_B_FLASH & portMPU_RASR_TEX_S_C_B_MASK ) << portMPU_RASR_TEX_S_C_B_LOCATION ) |
                                       ( prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_functions_end__ - ( uint32_t ) __privileged_functions_start__ ) ) |
                                       ( portMPU_REGION_ENABLE );

        /*
         * 配置特权数据 RAM 区域。内核数据放置在这里。
         */
        portMPU_REGION_BASE_ADDRESS_REG = ( ( uint32_t ) __privileged_data_start__ ) | /* 基地址。 */
                                          ( portMPU_REGION_VALID ) |
                                          ( portPRIVILEGED_RAM_REGION );

        portMPU_REGION_ATTRIBUTE_REG = ( portMPU_REGION_PRIVILEGED_READ_WRITE ) |
                                       ( portMPU_REGION_EXECUTE_NEVER ) |
                                       ( ( configTEX_S_C_B_SRAM & portMPU_RASR_TEX_S_C_B_MASK ) << portMPU_RASR_TEX_S_C_B_LOCATION ) |
                                       prvGetMPURegionSizeSetting( ( uint32_t ) __privileged_data_end__ - ( uint32_t ) __privileged_data_start__ ) |
                                       ( portMPU_REGION_ENABLE );

        /*
         * 默认允许所有代码访问通用外设。系统外设和寄存器受到保护。
         */
        portMPU_REGION_BASE_ADDRESS_REG = ( portPERIPHERALS_START_ADDRESS ) |
                                          ( portMPU_REGION_VALID ) |
                                          ( portGENERAL_PERIPHERALS_REGION );

        portMPU_REGION_ATTRIBUTE_REG = ( portMPU_REGION_READ_WRITE | portMPU_REGION_EXECUTE_NEVER ) |
                                       ( prvGetMPURegionSizeSetting( portPERIPHERALS_END_ADDRESS - portPERIPHERALS_START_ADDRESS ) ) |
                                       ( portMPU_REGION_ENABLE );

        /* 使能存储器故障异常。 */
        portNVIC_SYS_CTRL_STATE_REG |= portNVIC_MEM_FAULT_ENABLE;

        /* 使能 MPU，并配置背景区域（background region）。 */
        portMPU_CTRL_REG |= ( portMPU_ENABLE | portMPU_BACKGROUND_ENABLE );
    }
}
/*-----------------------------------------------------------*/

static uint32_t prvGetMPURegionSizeSetting( uint32_t ulActualSizeInBytes )
{
    uint32_t ulRegionSize, ulReturnValue = 4;

    /*
     * 32 是最小的区域大小，31 是 ulReturnValue 的最大有效值。
     */
    for( ulRegionSize = 32UL; ulReturnValue < 31UL; ( ulRegionSize <<= 1UL ) )
    {
        if( ulActualSizeInBytes <= ulRegionSize )
        {
            break;
        }
        else
        {
            ulReturnValue++;
        }
    }

    /*
     * 返回前将编码左移一位，使其可以直接写入属性寄存器的正确位位置。
     */
    return( ulReturnValue << 1UL );
}
/*-----------------------------------------------------------*/

BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        "   mrs r0, control                         \n" /* r0 置为 CONTROL。 */
        "   tst r0, #1                              \n" /* 执行 r0 & 1（按位与）并更新条件标志。 */
        "   ite ne                                  \n"
        "   movne r0, #0                            \n" /* CONTROL[0]!=0。返回 false 表示处理器当前不处于特权状态。 */
        "   moveq r0, #1                            \n" /* CONTROL[0]==0。返回 true 表示处理器当前处于特权状态。 */
        "   bx lr                                   \n" /* 返回。 */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vResetPrivilege( void ) /* __attribute__ (( naked ))（裸函数） */
{
    __asm volatile
    (
        "   mrs r0, control                         \n" /* r0 置为 CONTROL。 */
        "   orr r0, #1                              \n" /* r0 = r0 | 1（设置 bit 0）。 */
        "   msr control, r0                         \n" /* CONTROL 置为 r0。 */
        "   bx lr                                   \n" /* 返回调用者。 */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vPortSwitchToUserMode( void )
{
    /* 从当前任务的 TCB 中加载其 MPU 设置。 */
    xMPU_SETTINGS * xTaskMpuSettings = xTaskGetMPUSettings( NULL );

    /* 将该任务标记为非特权。 */
    xTaskMpuSettings->ulTaskFlags &= ( ~( portTASK_IS_PRIVILEGED_FLAG ) );

    /* 降低处理器的特权级别。 */
    vResetPrivilege();
}
/*-----------------------------------------------------------*/

void vPortStoreTaskMPUSettings( xMPU_SETTINGS * xMPUSettings,
                                const struct xMEMORY_REGION * const xRegions,
                                StackType_t * pxBottomOfStack,
                                configSTACK_DEPTH_TYPE uxStackDepth )
{
    #if defined( __ARMCC_VERSION )

        /*
         * 当这些变量在代码中定义而非由链接脚本导出时的声明。
         */
        extern uint32_t * __SRAM_segment_start__;
        extern uint32_t * __SRAM_segment_end__;
        extern uint32_t * __privileged_data_start__;
        extern uint32_t * __privileged_data_end__;
    #else
        /* 当这些变量由链接脚本导出时的声明。 */
        extern uint32_t __SRAM_segment_start__[];
        extern uint32_t __SRAM_segment_end__[];
        extern uint32_t __privileged_data_start__[];
        extern uint32_t __privileged_data_end__[];
    #endif /* __ARMCC_VERSION 定义条件 */

    int32_t lIndex;
    uint32_t ul;

    if( xRegions == NULL )
    {
        /* 未指定任何 MPU 区域，因此允许访问所有 RAM。 */
        xMPUSettings->xRegion[ 0 ].ulRegionBaseAddress =
            ( ( uint32_t ) __SRAM_segment_start__ ) | /* 基地址。 */
            ( portMPU_REGION_VALID ) |
            ( portSTACK_REGION );                     /* 区域编号。 */

        xMPUSettings->xRegion[ 0 ].ulRegionAttribute =
            ( portMPU_REGION_READ_WRITE ) |
            ( portMPU_REGION_EXECUTE_NEVER ) |
            ( ( configTEX_S_C_B_SRAM & portMPU_RASR_TEX_S_C_B_MASK ) << portMPU_RASR_TEX_S_C_B_LOCATION ) |
            ( prvGetMPURegionSizeSetting( ( uint32_t ) __SRAM_segment_end__ - ( uint32_t ) __SRAM_segment_start__ ) ) |
            ( portMPU_REGION_ENABLE );

        xMPUSettings->xRegionSettings[ 0 ].ulRegionStartAddress = ( uint32_t ) __SRAM_segment_start__;
        xMPUSettings->xRegionSettings[ 0 ].ulRegionEndAddress = ( uint32_t ) __SRAM_segment_end__;
        xMPUSettings->xRegionSettings[ 0 ].ulRegionPermissions = ( tskMPU_READ_PERMISSION |
                                                                   tskMPU_WRITE_PERMISSION );

        /* 使用户可配置区域失效。 */
        for( ul = 1UL; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ )
        {
            xMPUSettings->xRegion[ ul ].ulRegionBaseAddress = ( ( ul - 1UL ) | portMPU_REGION_VALID );
            xMPUSettings->xRegion[ ul ].ulRegionAttribute = 0UL;
            xMPUSettings->xRegionSettings[ ul ].ulRegionStartAddress = 0UL;
            xMPUSettings->xRegionSettings[ ul ].ulRegionEndAddress = 0UL;
            xMPUSettings->xRegionSettings[ ul ].ulRegionPermissions = 0UL;
        }
    }
    else
    {
        /*
         * 创建任务时会自动调用该函数——此时栈区域参数有效。其他
         * 时候栈参数无效，并假定栈区域已经配置好。
         */
        if( uxStackDepth > 0 )
        {
            /* 定义允许访问栈的区域。 */
            xMPUSettings->xRegion[ 0 ].ulRegionBaseAddress =
                ( ( uint32_t ) pxBottomOfStack ) |
                ( portMPU_REGION_VALID ) |
                ( portSTACK_REGION ); /* 区域编号。 */

            xMPUSettings->xRegion[ 0 ].ulRegionAttribute =
                ( portMPU_REGION_READ_WRITE ) |
                ( portMPU_REGION_EXECUTE_NEVER ) |
                ( prvGetMPURegionSizeSetting( uxStackDepth * ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t ) ) ) |
                ( ( configTEX_S_C_B_SRAM & portMPU_RASR_TEX_S_C_B_MASK ) << portMPU_RASR_TEX_S_C_B_LOCATION ) |
                ( portMPU_REGION_ENABLE );

            xMPUSettings->xRegionSettings[ 0 ].ulRegionStartAddress = ( uint32_t ) pxBottomOfStack;
            xMPUSettings->xRegionSettings[ 0 ].ulRegionEndAddress = ( uint32_t ) ( ( uint32_t ) ( pxBottomOfStack ) +
                                                                                   ( uxStackDepth * ( configSTACK_DEPTH_TYPE ) sizeof( StackType_t ) ) - 1UL );
            xMPUSettings->xRegionSettings[ 0 ].ulRegionPermissions = ( tskMPU_READ_PERMISSION |
                                                                       tskMPU_WRITE_PERMISSION );
        }

        lIndex = 0;

        for( ul = 1UL; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ )
        {
            if( ( xRegions[ lIndex ] ).ulLengthInBytes > 0UL )
            {
                /*
                 * 将由 xRegions 给出的通用区域定义转换为 CM4 专用的 MPU 设置，
                 * 然后存入 xMPUSettings。
                 */
                xMPUSettings->xRegion[ ul ].ulRegionBaseAddress =
                    ( ( uint32_t ) xRegions[ lIndex ].pvBaseAddress ) |
                    ( portMPU_REGION_VALID ) |
                    ( ul - 1UL ); /* 区域编号。 */

                xMPUSettings->xRegion[ ul ].ulRegionAttribute =
                    ( prvGetMPURegionSizeSetting( xRegions[ lIndex ].ulLengthInBytes ) ) |
                    ( xRegions[ lIndex ].ulParameters ) |
                    ( portMPU_REGION_ENABLE );

                xMPUSettings->xRegionSettings[ ul ].ulRegionStartAddress = ( uint32_t ) xRegions[ lIndex ].pvBaseAddress;
                xMPUSettings->xRegionSettings[ ul ].ulRegionEndAddress = ( uint32_t ) ( ( uint32_t ) xRegions[ lIndex ].pvBaseAddress + xRegions[ lIndex ].ulLengthInBytes - 1UL );
                xMPUSettings->xRegionSettings[ ul ].ulRegionPermissions = 0UL;

                if( ( ( xRegions[ lIndex ].ulParameters & portMPU_REGION_READ_ONLY ) == portMPU_REGION_READ_ONLY ) ||
                    ( ( xRegions[ lIndex ].ulParameters & portMPU_REGION_PRIVILEGED_READ_WRITE_UNPRIV_READ_ONLY ) == portMPU_REGION_PRIVILEGED_READ_WRITE_UNPRIV_READ_ONLY ) )
                {
                    xMPUSettings->xRegionSettings[ ul ].ulRegionPermissions = tskMPU_READ_PERMISSION;
                }

                if( ( xRegions[ lIndex ].ulParameters & portMPU_REGION_READ_WRITE ) == portMPU_REGION_READ_WRITE )
                {
                    xMPUSettings->xRegionSettings[ ul ].ulRegionPermissions = ( tskMPU_READ_PERMISSION | tskMPU_WRITE_PERMISSION );
                }
            }
            else
            {
                /* 使该区域失效。 */
                xMPUSettings->xRegion[ ul ].ulRegionBaseAddress = ( ( ul - 1UL ) | portMPU_REGION_VALID );
                xMPUSettings->xRegion[ ul ].ulRegionAttribute = 0UL;
                xMPUSettings->xRegionSettings[ ul ].ulRegionStartAddress = 0UL;
                xMPUSettings->xRegionSettings[ ul ].ulRegionEndAddress = 0UL;
                xMPUSettings->xRegionSettings[ ul ].ulRegionPermissions = 0UL;
            }

            lIndex++;
        }
    }
}
/*-----------------------------------------------------------*/

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

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

                for( i = 0; i < portTOTAL_NUM_REGIONS_IN_TCB; i++ )
                {
                    if( portIS_ADDRESS_WITHIN_RANGE( ulBufferStartAddress,
                                                     xTaskMpuSettings->xRegionSettings[ i ].ulRegionStartAddress,
                                                     xTaskMpuSettings->xRegionSettings[ i ].ulRegionEndAddress ) &&
                        portIS_ADDRESS_WITHIN_RANGE( ulBufferEndAddress,
                                                     xTaskMpuSettings->xRegionSettings[ i ].ulRegionStartAddress,
                                                     xTaskMpuSettings->xRegionSettings[ i ].ulRegionEndAddress ) &&
                        portIS_AUTHORIZED( ulAccessRequested, xTaskMpuSettings->xRegionSettings[ i ].ulRegionPermissions ) )
                    {
                        xAccessGranted = pdTRUE;
                        break;
                    }
                }
            }
        }

        return xAccessGranted;
    }

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */
/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

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
             * 若某个中断已分配的优先级高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，
             * 而其服务程序（ISR）调用了 ISR 安全的 FreeRTOS API 函数，则下面的
             * 断言将失败。ISR 安全的 FreeRTOS API 函数*只能*从优先级等于或低于
             * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断中调用。
             *
             * 数值较小的中断优先级表示逻辑上较高的中断优先级，因此必须将中断
             * 优先级设置为等于或数值上*大于* configMAX_SYSCALL_INTERRUPT_PRIORITY
             * 的值。
             *
             * 使用 FreeRTOS API 的中断不得保持默认优先级 0，因为 0 是可能的最高
             * 优先级，必然高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，因此也必然是
             * 无效的。
             *
             * FreeRTOS 维护独立的线程和 ISR API 函数，以确保中断进入尽可能快速、
             * 简单。
             *
             * 以下链接提供详细信息：
             * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
             * https://www.freertos.org/Why-FreeRTOS/FAQs
             */
            configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
        }

        /*
         * 优先级分组：中断控制器（NVIC）允许将定义每个中断优先级的位
         * 划分为定义中断抢占优先级的位和定义子优先级的位。为简化起见，
         * 所有位都必须定义为抢占优先级位。若情况并非如此
         * （某些位表示子优先级），则以下断言将失败。
         *
         * 若应用仅使用 CMSIS 库配置中断，则可在启动调度器前调用
         * NVIC_SetPriorityGrouping( 0 );，从而在所有 Cortex-M 器件上
         * 获得正确设置。但请注意，某些厂商特定的外设库假定优先级
         * 分组设置为非零值，此时使用 0 会导致不可预测的行为。
         */
        configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
    }

#endif /* configASSERT_DEFINED 条件编译结束 */
/*-----------------------------------------------------------*/

#if ( ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

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

#endif /* configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1（不使用 V1 包装器并启用访问控制列表） */
/*-----------------------------------------------------------*/

#if ( ( configUSE_MPU_WRAPPERS_V1 == 0 ) && ( configENABLE_ACCESS_CONTROL_LIST == 1 ) )

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

#endif /* configUSE_MPU_WRAPPERS_V1 == 0 且 configENABLE_ACCESS_CONTROL_LIST == 1（不使用 V1 包装器并启用访问控制列表） */
/*-----------------------------------------------------------*/

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
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

        BaseType_t xPortIsAuthorizedToAccessKernelObject( int32_t lInternalIndexOfKernelObject ) /* PRIVILEGED_FUNCTION（特权函数） */
        {
            ( void ) lInternalIndexOfKernelObject;

            /*
             * 若不使用访问控制列表功能，所有任务均可访问所有内核对象。
             */
            return pdTRUE;
        }

    #endif /* configENABLE_ACCESS_CONTROL_LIST == 1（启用访问控制列表） */

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（不使用 V1 包装器） */
/*-----------------------------------------------------------*/
