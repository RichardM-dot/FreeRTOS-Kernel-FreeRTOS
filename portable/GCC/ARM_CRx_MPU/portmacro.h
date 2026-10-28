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

#ifndef PORTMACRO_H
#define PORTMACRO_H

/**
 * @brief ARM_CRx_MPU FreeRTOS 移植层使用的函数、宏定义和结构体
 * @file portmacro.h
 * @note 本文件中的设置使 FreeRTOS 针对给定硬件和编译器正确配置。
 *  不应修改这些设置。
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 包含 stdint，以获得特定位宽的整数类型。 */
#include <stdint.h>

/* ------------------------------ FreeRTOS 配置检查 ------------------------------ */

#ifndef configSYSTEM_CALL_STACK_SIZE
    #error "Define configSYSTEM_CALL_STACK_SIZE to a length, in bytes, " \
            "to use when an unprivileged task makes a FreeRTOS Kernel call. "
#endif /* configSYSTEM_CALL_STACK_SIZE 条件编译结束 */

#if( configUSE_MPU_WRAPPERS_V1 == 1 )
    #error This port is usable with MPU wrappers V2 only.
#endif /* configUSE_MPU_WRAPPERS_V1 条件编译结束 */

#ifndef configSETUP_TICK_INTERRUPT
    #error "configSETUP_TICK_INTERRUPT() must be defined in FreeRTOSConfig.h " \
           "to call the function that sets up the tick interrupt."
#endif /* configSETUP_TICK_INTERRUPT 条件编译结束 */

/* ----------------------------------------------------------------------------------- */

#if( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1 )

    /* 检查配置。 */
    #if( configMAX_PRIORITIES > 32 )
        #error "configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when " \
                "configMAX_PRIORITIES is less than or equal to 32. " \
                "It is very rare that a system requires more than 10 to 15 difference " \
                "priorities as tasks that share a priority will time slice."
    #endif /* ( configMAX_PRIORITIES > 32 ) 条件编译结束 */

    /**
     * @brief 标记给定优先级的任务已就绪。
     *
     * @ingroup Scheduler
     *
     * @param[in] uxPriority 已就绪任务的优先级。
     * @param[in] uxTopReadyPriority 就绪任务优先级的位图。
     */
    #define portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority ) \
        ( uxTopReadyPriority ) |= ( 1UL << ( uxPriority ) )

    /**
     * @brief 标记给定优先级的任务不再就绪。
     *
     * @ingroup Scheduler
     *
     * @param[in] uxPriority 不再就绪任务的优先级。
     * @param[in] uxTopReadyPriority 就绪任务优先级的位图。
     */
    #define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority ) \
        ( uxTopReadyPriority ) &= ~( 1UL << ( uxPriority ) )

    /**
     * @brief 确定最高优先级就绪任务的优先级。
     *
     * @ingroup Scheduler
     *
     * @param[in] uxTopReadyPriority 就绪任务优先级的位图。
     * @param[in] uxTopPriority 最高优先级就绪任务的优先级。
     */
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority ) \
        ( uxTopPriority ) = ( 31UL - ulPortCountLeadingZeros( ( uxTopReadyPriority ) ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

/* ------------------------------ 移植层类型定义 ------------------------------ */

#include "portmacro_asm.h"

/**
 * @brief 临界区嵌套值。
 *
 * @ingroup Critical Sections
 *
 * @note 任务的嵌套计数达到该值时会退出临界区并使能 IRQ。
 */
#define portNO_CRITICAL_NESTING ( ( uint32_t ) 0x0 )

/**
 * @brief 当前程序状态寄存器（CPSR）中用于指示 CPU 处于 Thumb 状态的位。
 *
 * @ingroup Task Context
 */
#define portTHUMB_MODE_BIT      ( ( StackType_t ) 0x20 )

/**
 * @brief 用于检查地址是否为 Thumb 代码的位掩码。
 *
 * @ingroup Task Context
 */
#define portTHUMB_MODE_ADDRESS  ( 0x01UL )

/**
 * @brief 表示栈字的数据类型。
 *
 * @ingroup Port Interface Specifications
 */
typedef uint32_t StackType_t;

/**
 * @brief 与 CPU 数据字操作宽度相同的有符号数据类型。
 *
 * @ingroup Port Interface Specifications
 */
typedef int32_t BaseType_t;

/**
 * @brief 与 CPU 数据字操作宽度相同的无符号数据类型。
 *
 * @ingroup Port Interface Specifications
 */
typedef uint32_t UBaseType_t;

/**
 * @brief FreeRTOS 时钟节拍计数器使用的数据类型。
 *
 * @note 在 32 位架构上使用 32 位节拍类型，可确保读取节拍计数
 * 无需用临界区保护。
 */
typedef uint32_t TickType_t;

/**
 * @brief 标记目标 CPU 上栈的增长方向。
 *
 * @ingroup Port Interface Specifications
 */
#define portSTACK_GROWTH   ( -1 )

/**
 * @brief 指定目标 CPU 对栈指针对齐的要求。
 *
 * @ingroup Port Interface Specifications
 */
#define portBYTE_ALIGNMENT 8U

/**
 * @brief 如 FreeRTOS.org 所述的任务函数原型宏。
 *
 * @ingroup Port Interface Specifications
 *
 * @note 本移植层并不需要它，但保留下来以便通用示例代码使用。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) \
    void vFunction( void * pvParameters )

/**
 * @brief 如 FreeRTOS.org 所述的任务函数原型宏。
 *
 * @ingroup Port Interface Specifications
 *
 * @note 本移植层并不需要它，但保留下来以便通用示例代码使用。
 */
#define portTASK_FUNCTION( vFunction, pvParameters ) \
    void vFunction( void * pvParameters )

/**
 * @brief 空操作 ARM 汇编指令。
 *
 * @ingroup Port Interface Specifications
 */
#define portNOP()                                    __asm volatile( "NOP" )

/**
 * @brief 内联 GCC 标签。
 *
 * @ingroup Port Interface Specifications
 */
#define portINLINE                                   __inline

/**
 * @brief 内存访问同步屏障。
 *
 * @ingroup Port Interface Specifications
 */
#define portMEMORY_BARRIER()                         __asm volatile( "" ::: "memory" )

/**
 * @brief 确保符号不会被从编译单元中移除。
 *
 * @ingroup Port Interface Specifications
 */
#define portDONT_DISCARD                             __attribute__( ( used ) )

/**
 * @brief 定义节拍计数是否可原子访问。
 *
 * @ingroup System Clock
 */
#define portTICK_TYPE_IS_ATOMIC                      1

/**
 * @brief 系统节拍之间的毫秒数。
 *
 * @ingroup System Clock
 */
#define portTICK_PERIOD_MS                           ( ( TickType_t ) 1000UL / configTICK_RATE_HZ )

/**
 * @brief 任何 FreeRTOS API 可能使用的最大延时值。
 *
 * @ingroup System Clock
 */
#define portMAX_DELAY                                ( TickType_t ) 0xFFFFFFFFUL

/* ----------------------------- 移植层汇编函数 ----------------------------- */

/**
 * @brief FreeRTOS 管理程序调用（SVC）处理程序。
 *
 * @ingroup Scheduler
 */
void FreeRTOS_SVC_Handler( void );

/**
 * @brief FreeRTOS 中断处理程序。
 *
 * @ingroup Scheduler
 */
void FreeRTOS_IRQ_Handler( void );

/**
 * @brief 让出 CPU。
 *
 * @ingroup Scheduler
 */
void vPortYield( void );

#define portYIELD() vPortYield()

/**
 * @brief 使能中断。
 *
 * @ingroup Interrupt Management
 */
void vPortEnableInterrupts( void );

#define portENABLE_INTERRUPTS() vPortEnableInterrupts()

/**
 * @brief 禁止中断。
 *
 * @ingroup Interrupt Management
 */
void vPortDisableInterrupts( void );

#define portDISABLE_INTERRUPTS() vPortDisableInterrupts()

/**
 * @brief 从 FreeRTOS 系统调用返回。
 *
 * @ingroup Port Privilege
 */
void vPortSystemCallExit( void );

/**
 * @brief 开始执行第一个任务。
 *
 * @ingroup Scheduler
 */
void vPortStartFirstTask( void );

/**
 * @brief 使能片上 MPU。
 *
 * @ingroup MPU Control
 */
void vMPUEnable( void );

/**
 * @brief 禁用片上 MPU。
 *
 * @ingroup MPU Control
 */
void vMPUDisable( void );

/**
 * @brief 使能 MPU 背景区域。
 *
 * @ingroup MPU Control
 */
void vMPUEnableBackgroundRegion( void );

/**
 * @brief 禁用 MPU 背景区域。
 *
 * @ingroup MPU Control
 */
void vMPUDisableBackgroundRegion( void );

/**
 * @brief 设置 MPU 区域的权限。
 *
 * @ingroup MPU Control
 *
 * @param[in] ulRegionNumber 要设置权限的 MPU 区域编号。
 * @param[in] ulBaseAddress MPU 区域的基地址。
 * @param[in] ulRegionSize MPU 区域的大小（字节）。
 * @param[in] ulRegionPermissions 与 MPU 区域关联的权限。
 *
 * @note 这是内部函数，调用前应确保其输入已经过检查。
 */
void vMPUSetRegion( uint32_t ulRegionNumber,
                    uint32_t ulBaseAddress,
                    uint32_t ulRegionSize,
                    uint32_t ulRegionPermissions );

/* ------------------------------- port.c 声明 ------------------------------- */

/**
 * @brief 进入临界区。
 *
 * @ingroup Critical Section
 */
void vPortEnterCritical( void );

#define portENTER_CRITICAL() vPortEnterCritical()

/**
 * @brief 退出临界区。
 *
 * @ingroup Critical Section
 */
void vPortExitCritical( void );

#define portEXIT_CRITICAL() vPortExitCritical()

/**
 * @brief 检查处理器是否处于特权级。
 *
 * @ingroup Port Privilege
 *
 * @note 处理器特权级通过检查当前程序状态寄存器（CPSR）的模式位 [4:0] 确定。
 *
 * @return 若处理器处于特权级则返回 pdTRUE，否则返回 pdFALSE。
 */
BaseType_t xPortIsPrivileged( void );

#define portIS_PRIVILEGED() xPortIsPrivileged()

/**
 * @brief 检查任务是否具有特权。
 *
 * @ingroup Port Privilege
 *
 * @note 任务的特权级属于该任务本身，与 xPortIsPrivileged 返回的处理器特权级
 * 不同。例如，非特权任务执行系统调用时，处理器处于特权级。
 *
 * @return 若任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
 */
BaseType_t xPortIsTaskPrivileged( void );

#define portIS_TASK_PRIVILEGED() xPortIsTaskPrivileged()

/**
 * @brief 任务的默认返回地址。
 *
 * @ingroup Task Context
 *
 * @note 若 FreeRTOSConfig.h 中未定义 configTASK_RETURN_ADDRESS，
 * 则该函数用作任务的默认返回地址。
 */
void prvTaskExitError( void );

#ifdef configTASK_RETURN_ADDRESS
    #define portTASK_RETURN_ADDRESS configTASK_RETURN_ADDRESS
#else
    #define portTASK_RETURN_ADDRESS prvTaskExitError
#endif /* configTASK_RETURN_ADDRESS 条件编译结束 */

/**
 * @brief 返回 32 位变量中前导零的个数。
 *
 * @param[in] ulBitmap 要统计前导零个数的 32 位数。
 *
 * @return ulBitmap 中前导零的个数。
 */
UBaseType_t ulPortCountLeadingZeros( UBaseType_t ulBitmap );

/**
 * @brief 结束 FreeRTOS 调度器。
 *
 * 本移植层未实现。
 *
 * @ingroup Scheduler
 */
void vPortEndScheduler( void );

/* --------------------------------- MPU 定义 --------------------------------- */

/**
 * @brief 标记本移植层使用片上 ARM MPU。
 *
 * @ingroup MPU Control
 */
#define portUSING_MPU_WRAPPERS     1

/**
 * @brief 用于标记是否应将任务创建为特权任务。
 *
 * @ingroup Task Context
 * @ingroup MPU Control
 *
 * @note 通过将该值与任务优先级按位或来创建特权任务。例如，要创建优先级为 2
 * 的特权任务，应将 uxPriority 参数设为 ( 2 | portPRIVILEGE_BIT )。
 */
#define portPRIVILEGE_BIT          ( 0x80000000UL )

/**
 * @brief 访问控制列表（ACL）表项的位数。
 */
#define portACL_ENTRY_SIZE_BITS    ( 32UL )

/**
 * @brief 保存 MPU 寄存器值的结构体。
 *
 * @struct xMPU_REGION_REGISTERS
 *
 * @ingroup MPU Control
 *
 * @note 该结构体的成员顺序必须与 portRESTORE_CONTEXT 中的顺序一致。
 */
typedef struct MPU_REGION_REGISTERS
{
    uint32_t ulRegionSize;        /* MPU 区域大小与使能寄存器的信息。 */
    uint32_t ulRegionAttribute;   /* MPU 区域访问控制寄存器的信息。 */
    uint32_t ulRegionBaseAddress; /* MPU 区域基地址寄存器的信息。 */
} xMPU_REGION_REGISTERS;

/**
 * @brief 保存每任务系统调用栈信息的结构体。
 *
 * @struct xSYSTEM_CALL_STACK_INFO
 *
 * @ingroup Port Privilege
 *
 * @note 该结构体的成员顺序必须与移植层汇编代码一致。
 */
typedef struct SYSTEM_CALL_STACK_INFO
{
    uint32_t * pulTaskStackPointer; /**< 任务发起 FreeRTOS 系统调用时的栈指针。 */
    uint32_t * pulLinkRegisterAtSystemCallEntry; /**< 任务发起 FreeRTOS 系统调用时的链接寄存器。 */
    uint32_t * pulSystemCallStackPointer; /**< 执行 FreeRTOS 系统调用时使用的栈指针。 */
    uint32_t * pulSystemCallExitAddress; /**< 系统调用退出地址。 */
    uint32_t ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE ]; /**< 执行 FreeRTOS 系统调用时用作栈的缓冲区。 */
} xSYSTEM_CALL_STACK_INFO;

/**
 * @brief 保存在 TCB 中的每任务 MPU 设置结构体。
 * @struct xMPU_SETTINGS
 *
 * @ingroup MPU Control
 * @ingroup Task Context
 * @ingroup Port Privilege
 *
 * @note 该结构体的成员顺序必须与移植层汇编代码一致。
 */
typedef struct MPU_SETTINGS
{
    xMPU_REGION_REGISTERS xRegion[ portTOTAL_NUM_REGIONS_IN_TCB ];
    uint32_t ulTaskFlags;
    xSYSTEM_CALL_STACK_INFO xSystemCallStackInfo;
    uint32_t ulContext[ CONTEXT_SIZE ]; /**< 用于保存任务上下文的缓冲区。 */

    #if( configENABLE_ACCESS_CONTROL_LIST == 1 )
        uint32_t ulAccessControlList[ ( configPROTECTED_KERNEL_OBJECT_POOL_SIZE
                                        / portACL_ENTRY_SIZE_BITS )
                                      + 1UL ];
    #endif
} xMPU_SETTINGS;

#ifdef __cplusplus
} /* extern C 块结束 */
#endif

#endif /* 头文件保护：PORTMACRO_H */
