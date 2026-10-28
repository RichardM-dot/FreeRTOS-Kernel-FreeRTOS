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

#ifndef PORTMACRO_H
#define PORTMACRO_H

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

/*
 * -----------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置使 FreeRTOS 针对给定硬件和编译器正确配置。
 *
 * 不应修改这些设置。
 * -----------------------------------------------------------
 */

/* 类型定义。 */
#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    size_t
#define portBASE_TYPE     long

#if !defined(__ASSEMBLER__)
    typedef portSTACK_TYPE   StackType_t;
    typedef portBASE_TYPE    BaseType_t;
    typedef uint64_t         UBaseType_t;

    typedef uint64_t         TickType_t;
    #define portMAX_DELAY    ( ( TickType_t ) 0xffffffffffffffff )
#endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

/*
 * 64 位架构上使用 64 位时钟节拍类型，因此读取节拍计数
 * 无需用临界区保护。
 */
#define portTICK_TYPE_IS_ATOMIC    1

/*-----------------------------------------------------------*/

/* 硬件相关定义。 */
#define portSTACK_GROWTH         ( -1 )
#define portTICK_PERIOD_MS       ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT       16
#define portPOINTER_SIZE_TYPE    uint64_t

/*-----------------------------------------------------------*/

/* 任务相关工具宏。 */

#if !defined(__ASSEMBLER__)
    /* 在可能引发上下文切换的中断服务程序（ISR）末尾调用。 */
    #if ( configNUMBER_OF_CORES == 1 )
        #define portEND_SWITCHING_ISR( xSwitchRequired ) \
        {                                                \
            extern uint64_t ullPortYieldRequired;        \
                                                        \
            if( xSwitchRequired != pdFALSE )             \
            {                                            \
                ullPortYieldRequired = pdTRUE;           \
            }                                            \
        }
    #else
        #define portEND_SWITCHING_ISR( xSwitchRequired )                   \
        {                                                                  \
            extern uint64_t ullPortYieldRequired[ configNUMBER_OF_CORES ]; \
                                                                        \
            if( xSwitchRequired != pdFALSE )                               \
            {                                                              \
                ullPortYieldRequired[ portGET_CORE_ID() ] = pdTRUE;        \
            }                                                              \
        }
    #endif /* if ( configNUMBER_OF_CORES == 1 ) 条件编译结束 */
#endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

/**
 * @brief SVC 编号。
 */

#define portSVC_SYSTEM_CALL_EXIT                    104
#define portSVC_YIELD                               105
#define portSVC_START_FIRST_TASK                    106
#define portSVC_DISABLE_INTERRUPTS                  107
#define portSVC_ENABLE_INTERRUPTS                   108
#define portSVC_GET_CORE_ID                         109
#define portSVC_MASK_ALL_INTERRUPTS                 110
#define portSVC_UNMASK_ALL_INTERRUPTS               111
#define portSVC_UNMASK_INTERRUPTS                   112
#define portSVC_CHECK_PRIVILEGE                     113
#define portSVC_SAVE_TASK_CONTEXT                   114
#define portSVC_RESTORE_CONTEXT                     115
#define portSVC_DELETE_CURRENT_TASK                 116
#define portSVC_INTERRUPT_CORE                      117

#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )
#define portYIELD()                __asm volatile ( "SVC %0" : : "i" ( portSVC_YIELD ) : "memory" )

/*
 * -----------------------------------------------------------
 * 临界区控制
 * ----------------------------------------------------------
 */

#if !defined(__ASSEMBLER__)
    extern UBaseType_t vTaskEnterCriticalFromISR( void );
    extern void vTaskExitCriticalFromISR( UBaseType_t uxSavedInterruptStatus );
    extern UBaseType_t uxPortSetInterruptMask( void );
    extern UBaseType_t uxPortSetInterruptMaskFromISR( void );
    extern void vPortClearInterruptMask( UBaseType_t uxNewMaskValue );
    extern void vPortClearInterruptMaskFromISR( UBaseType_t uxNewMaskValue );
    extern void vInterruptCore( uint32_t ulInterruptID, uint8_t ucCoreID );
#endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

/* 使用 SVC，因此从 EL0 调用是安全的。移植层中 EL1 处的代码直接使用 MSR。 */
#define portDISABLE_INTERRUPTS() __asm volatile ( "SVC %0" : : "i" ( portSVC_DISABLE_INTERRUPTS ) : "memory" )

#define portENABLE_INTERRUPTS()  __asm volatile ( "SVC %0" : : "i" ( portSVC_ENABLE_INTERRUPTS ) : "memory" )

/*
 * 在所有 GIC 中，可向优先级屏蔽寄存器写入 255，以解除屏蔽
 * 除最低优先级以外的所有中断优先级。
 */
#define portUNMASK_VALUE           ( 0xFFUL )

#if !defined(__ASSEMBLER__)
    /*
     * 这些宏不会全局禁止/使能中断，而是屏蔽优先级低于
     * configMAX_API_CALL_INTERRUPT_PRIORITY 的中断。
     */
    #if  ( configNUMBER_OF_CORES == 1 )
        extern void vPortEnterCritical( void );
        extern void vPortExitCritical( void );
        #define portENTER_CRITICAL()                  vPortEnterCritical()
        #define portEXIT_CRITICAL()                   vPortExitCritical()
    #else
        #define portENTER_CRITICAL()                  vTaskEnterCritical()
        #define portEXIT_CRITICAL()                   vTaskExitCritical()
    #endif
    #define portSET_INTERRUPT_MASK_FROM_ISR()         uxPortSetInterruptMaskFromISR()
    #define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vPortClearInterruptMaskFromISR( x )
    #define portENTER_CRITICAL_FROM_ISR()             vTaskEnterCriticalFromISR()
    #define portEXIT_CRITICAL_FROM_ISR( x )           vTaskExitCriticalFromISR( x )
#endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

/*-----------------------------------------------------------*/

/*
 * FreeRTOS.org 网站上所述的任务函数宏。本移植层并不需要它们，
 * 但保留下来以便使用这些宏的通用示例代码仍可编译。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

#if !defined(__ASSEMBLER__)
    /*
     * FreeRTOS 时钟节拍处理程序的原型。必须将该处理程序安装为产生
     * RTOS 节拍的外设所对应的中断处理程序。
     */
    void FreeRTOS_Tick_Handler( void );
#endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

#define portTASK_NO_FPU_CONTEXT_BY_DEFAULT     ( 1U )
#define portTASK_HAVE_FPU_CONTEXT_BY_DEFAULT   ( 2U )

/*
 * 若 configUSE_TASK_FPU_SUPPORT 设为 portTASK_NO_FPU_CONTEXT_BY_DEFAULT (1U)
 * （或未定义），任务创建时不带 FPU 上下文，必须先调用 vPortTaskUsesFPU()
 * 获得 FPU 上下文，才能使用任何 FPU 指令。若 configUSE_TASK_FPU_SUPPORT
 * 设为 portTASK_HAVE_FPU_CONTEXT_BY_DEFAULT (2U)，则所有任务默认都具有
 * FPU 上下文。
 */
#if ( configUSE_TASK_FPU_SUPPORT == portTASK_NO_FPU_CONTEXT_BY_DEFAULT )
    void vPortTaskUsesFPU( void );
#else
/*
 * 每个任务已具有 FPU 上下文，因此将该函数定义为空，
 * 防止被误调用。
 */
    #define vPortTaskUsesFPU()
#endif

#define portTASK_USES_FLOATING_POINT()          vPortTaskUsesFPU()

#define portLOWEST_INTERRUPT_PRIORITY           ( ( ( uint32_t ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define portLOWEST_USABLE_INTERRUPT_PRIORITY    ( portLOWEST_INTERRUPT_PRIORITY - 1UL )

/* 体系结构相关的优化。 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

    /* 在位图中存储/清除就绪优先级。 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )    uxTopPriority = ( 31 - __builtin_clz( uxReadyPriorities ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

#if ( configASSERT_DEFINED == 1 )
    void vPortValidateInterruptPriority( void );
    #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()    vPortValidateInterruptPriority()
#endif /* configASSERT 条件编译结束 */

#define portNOP()     __asm volatile ( "NOP" )
#define portINLINE    __inline

/*
 * 中断优先级需要移位的位数取决于中断控制器实现的位数。
 */
#if configUNIQUE_INTERRUPT_PRIORITIES == 16
    #define portPRIORITY_SHIFT            4
    #define portMAX_BINARY_POINT_VALUE    3
#elif configUNIQUE_INTERRUPT_PRIORITIES == 32
    #define portPRIORITY_SHIFT            3
    #define portMAX_BINARY_POINT_VALUE    2
#elif configUNIQUE_INTERRUPT_PRIORITIES == 64
    #define portPRIORITY_SHIFT            2
    #define portMAX_BINARY_POINT_VALUE    1
#elif configUNIQUE_INTERRUPT_PRIORITIES == 128
    #define portPRIORITY_SHIFT            1
    #define portMAX_BINARY_POINT_VALUE    0
#elif configUNIQUE_INTERRUPT_PRIORITIES == 256
    #define portPRIORITY_SHIFT            0
    #define portMAX_BINARY_POINT_VALUE    0
#else /* if configUNIQUE_INTERRUPT_PRIORITIES == 16 条件分支 */
    #error Invalid configUNIQUE_INTERRUPT_PRIORITIES setting.  configUNIQUE_INTERRUPT_PRIORITIES must be set to the number of unique priorities implemented by the target hardware
#endif /* if configUNIQUE_INTERRUPT_PRIORITIES == 16 条件结束 */

#define portINTERRUPT_PRIORITY_REGISTER_OFFSET    ( 0x400U )
#define portYIELD_CORE_INT_ID                     ( 0x0U )
#define portMAX_API_PRIORITY_MASK                 ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT )

#if ( configNUMBER_OF_CORES > 1 )

    #if !defined(__ASSEMBLER__)
        typedef enum
        {
            eIsrLock = 0,
            eTaskLock,
            eLockCount
        } ePortRTOSLock;

        extern volatile uint64_t ullCriticalNestings[ configNUMBER_OF_CORES ];
        extern void vPortRecursiveLock( uint8_t ucCoreID,
                                        ePortRTOSLock eLockNum,
                                        BaseType_t uxAcquire );
        extern uint8_t ucPortGetCoreID( void );
        extern uint8_t ucPortGetCoreIDFromIsr( void );
    #endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

    #define portSET_INTERRUPT_MASK()         uxPortSetInterruptMask()
    #define portCLEAR_INTERRUPT_MASK( x )    vPortClearInterruptMask( x )

    #define portMAX_CORE_COUNT               configNUMBER_OF_CORES
    #define portGET_CORE_ID()                ucPortGetCoreID()
    #define portGET_CORE_ID_FROM_ISR()       ucPortGetCoreIDFromIsr()

    /* 使用 SGI 0 作为让出 CPU 的核心间中断。 */
    #define portYIELD_CORE( xCoreID )                          vInterruptCore( portYIELD_CORE_INT_ID, ( uint8_t ) xCoreID )

    #define portRELEASE_ISR_LOCK( xCoreID )                    vPortRecursiveLock( ( uint8_t ) xCoreID, eIsrLock, pdFALSE )
    #define portGET_ISR_LOCK( xCoreID )                        vPortRecursiveLock( ( uint8_t ) xCoreID, eIsrLock, pdTRUE )

    #define portRELEASE_TASK_LOCK( xCoreID )                   vPortRecursiveLock( ( uint8_t ) xCoreID, eTaskLock, pdFALSE )
    #define portGET_TASK_LOCK( xCoreID )                       vPortRecursiveLock( ( uint8_t ) xCoreID, eTaskLock, pdTRUE )
    #define portGET_CRITICAL_NESTING_COUNT( xCoreID )          ( ullCriticalNestings[ ( uint8_t ) xCoreID ] )
    #define portSET_CRITICAL_NESTING_COUNT( xCoreID, x )       ( ullCriticalNestings[ ( uint8_t ) xCoreID ] = ( x ) )
    #define portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( ullCriticalNestings[ ( uint8_t ) xCoreID ]++ )
    #define portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( ullCriticalNestings[ ( uint8_t ) xCoreID ]-- )

#endif /* configNUMBER_OF_CORES > 1 条件编译结束 */

#define portMEMORY_BARRIER()    __asm volatile ( "" ::: "memory" )

/**
 * @brief MPU 专用常量。
 */
#if ( configENABLE_MPU == 1 )

    #if !defined(__ASSEMBLER__)
        extern BaseType_t xPortIsTaskPrivileged( void );
    #endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

    /*
     * MAIR_EL1 寄存器中使用的设备存储器属性。
     *
     * 8 位值编码如下：
     *  Bit[7:4] - 0000 - 设备存储器
     *  Bit[3:2] - 00 --> Device-nGnRnE
     *             01 --> Device-nGnRE
     *             10 --> Device-nGRE
     *             11 --> Device-GRE
     *  Bit[1:0] - 00.
     */
    #define portMPU_DEVICE_MEMORY_nGnRnE                 ( 0x00 )
    #define portMPU_DEVICE_MEMORY_nGnRE                  ( 0x04 )
    #define portMPU_DEVICE_MEMORY_nGRE                   ( 0x08 )
    #define portMPU_DEVICE_MEMORY_GRE                    ( 0x0C )

    /* 可在 FreeRTOSConfig.h 中覆盖的 MPU 设置。 */
    #ifndef configTOTAL_MPU_REGIONS
        #define configTOTAL_MPU_REGIONS                  ( 16UL )
    #endif

    #define portPRIVILEGED_FLASH_REGION                  ( 0ULL )                                /* 特权 Flash 区域编号。 */
    #define portUNPRIVILEGED_FLASH_REGION                ( 1ULL )                                /* 非特权 Flash 区域编号。 */
    #define portUNPRIVILEGED_SYSCALLS_REGION             ( 2ULL )                                /* 非特权系统调用区域编号。 */
    #define portPRIVILEGED_RAM_REGION                    ( 3ULL )                                /* 特权 RAM 区域编号。 */
    #define portSTACK_REGION                             ( 4ULL )                                /* 栈区域编号。 */
    #define portSTACK_REGION_INDEX                       ( 0ULL )                                /* 栈区域在 xRegionSettings 数组中的索引。 */
    #define portFIRST_CONFIGURABLE_REGION                ( 5ULL )                                /* 第一个用户可配置区域编号。 */
    #define portLAST_CONFIGURABLE_REGION                 ( configTOTAL_MPU_REGIONS - 1UL )
    #define portNUM_CONFIGURABLE_REGIONS                 ( ( portLAST_CONFIGURABLE_REGION - portFIRST_CONFIGURABLE_REGION ) + 1 )
    #define portTOTAL_NUM_REGIONS                        ( portNUM_CONFIGURABLE_REGIONS + 1 )    /* 加一，为栈区域留出空间。 */

    #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
        #define portACL_ENTRY_SIZE_BITS ( 32UL )
    #endif /* configENABLE_ACCESS_CONTROL_LIST == 1 条件编译结束 */

    #if !defined(__ASSEMBLER__)
        /**
         * @brief 定义 MPU 区域的设置。
         */
        typedef struct MPURegionSettings
        {
            uint64_t ullPrbarEl1; /**< 该区域的 PRBAR_EL1。 */
            uint64_t ullPrlarEl1; /**< 该区域的 PRLAR_EL1。 */
        } MPURegionSettings_t;

        #ifndef configSYSTEM_CALL_STACK_SIZE
            #define configSYSTEM_CALL_STACK_SIZE 128  /* 使用 MPU 包装函数 v2 时，必须按字（word）定义系统调用栈的期望大小。 */
        #endif

        /**
         * @brief 系统调用信息。
         */
        typedef struct SYSTEM_CALL_INFO
        {
            /*
             * 用于在系统调用入口保存用户模式栈指针（SP_EL0）和链接寄存器（X30），
             * 以便即使任务在执行系统调用期间被换出，也能安全地恢复或引用它们。
             */
            uint64_t ullLinkRegisterAtSystemCallEntry;
            uint64_t ullUserSPAtSystemCallEntry;
        } xSYSTEM_CALL_INFO;
    #endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

    /**
     * @brief 保存在 TCB 中的任务上下文。
     */
    #if ( configENABLE_FPU == 1 )
        /*
         * 任务上下文布局（含 FPU）：
         * +-----------+------------+--------------------------------+-------------+------------------+
         * |  Q0-Q31   | FPSR, FPCR |  CRITICAL_NESTING, FPU_CONTEXT | X0-X30, XZR | INIT_PSTATE, PC  |
         * +-----------+------------+--------------------------------+-------------+------------------+
         *
         * <-----------><-----------><-------------------------------><------------><----------------->
         *      64            2                      2                      32               2
         */
        #define MAX_CONTEXT_SIZE    102

    #else /* configENABLE_FPU == 1 条件分支 */
        /*
         * 任务上下文布局（不含 FPU）：
         * +--------------------------------+-------------+------------------+
         * |  CRITICAL_NESTING, FPU_CONTEXT | X0-X30, XZR | INIT_PSTATE, PC  |
         * +--------------------------------+-------------+------------------+
         * <-------------------------------><------------><------------------>
         *                 2                      32               2
         */
        #define MAX_CONTEXT_SIZE    36
    #endif /* configENABLE_FPU == 1 条件结束 */

    #if !defined(__ASSEMBLER__)
        typedef struct MPU_SETTINGS
        {
            uint64_t ullTaskUnprivilegedSP;                                              /* 任务的非特权用户栈指针。 */
            uint64_t ullMairEl1;                                                         /* 包含属性信息的任务 MAIR_EL1。 */
            MPURegionSettings_t xRegionsSettings[ portTOTAL_NUM_REGIONS ];               /* 任务各区域的设置。 */
            uint64_t ullContext[ MAX_CONTEXT_SIZE + configSYSTEM_CALL_STACK_SIZE ];      /* 任务保存的上下文。 */
            uint64_t ullTaskFlags;

            xSYSTEM_CALL_INFO xSystemCallInfo;

            #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
                uint32_t ulAccessControlList[ ( configPROTECTED_KERNEL_OBJECT_POOL_SIZE / portACL_ENTRY_SIZE_BITS ) + 1 ];
            #endif /* configENABLE_ACCESS_CONTROL_LIST 条件编译结束 */
        } xMPU_SETTINGS;
    #endif /* if !defined(__ASSEMBLER__) 条件编译结束 */

    #define portUSING_MPU_WRAPPERS                        ( 1 )
    #define portPRIVILEGE_BIT                             ( 0x80000000UL )

    /* MAIR_EL1 寄存器中使用的普通存储器属性。 */
    #define portMPU_NORMAL_MEMORY_NON_CACHEABLE           ( 0x44 )       /* 不可缓存。 */
    #define portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE    ( 0xFF )       /* 非瞬态、写回、读分配和写分配。 */

    #define portMPU_MAIR_EL1_ATTR0_POS                    ( 0UL )
    #define portMPU_MAIR_EL1_ATTR0_MASK                   ( 0x00000000000000ffULL )

    #define portMPU_MAIR_EL1_ATTR1_POS                    ( 8UL )
    #define portMPU_MAIR_EL1_ATTR1_MASK                   ( 0x000000000000ff00ULL )

    #define portMPU_MAIR_EL1_ATTR2_POS                    ( 16UL )
    #define portMPU_MAIR_EL1_ATTR2_MASK                   ( 0x0000000000ff0000ULL )

    #define portMPU_MAIR_EL1_ATTR3_POS                    ( 24UL )
    #define portMPU_MAIR_EL1_ATTR3_MASK                   ( 0x00000000ff000000ULL )

    #define portMPU_MAIR_EL1_ATTR4_POS                    ( 32UL )
    #define portMPU_MAIR_EL1_ATTR4_MASK                   ( 0x000000ff00000000ULL )

    #define portMPU_MAIR_EL1_ATTR5_POS                    ( 40UL )
    #define portMPU_MAIR_EL1_ATTR5_MASK                   ( 0x0000ff0000000000ULL )

    #define portMPU_MAIR_EL1_ATTR6_POS                    ( 48UL )
    #define portMPU_MAIR_EL1_ATTR6_MASK                   ( 0x00ff000000000000ULL )

    #define portMPU_MAIR_EL1_ATTR7_POS                    ( 56UL )
    #define portMPU_MAIR_EL1_ATTR7_MASK                   ( 0xff00000000000000ULL )

    #define portMPU_PRBAR_EL1_ADDRESS_MASK                ( 0x0000FFFFFFFFFFC0ULL )
    #define portMPU_PRLAR_EL1_ADDRESS_MASK                ( 0x0000FFFFFFFFFFC0ULL )
    #define portMPU_PRBAR_EL1_ACCESS_PERMISSIONS_MASK     ( 3ULL<< 2ULL )

    #define portMPU_REGION_NON_SHAREABLE                  ( 0ULL << 4ULL )
    #define portMPU_REGION_OUTER_SHAREABLE                ( 2ULL << 4ULL )
    #define portMPU_REGION_INNER_SHAREABLE                ( 3ULL << 4ULL )

    #define portMPU_REGION_PRIVILEGED_READ_WRITE          ( 0ULL << 2ULL )
    #define portMPU_REGION_READ_WRITE                     ( 1ULL << 2ULL )
    #define portMPU_REGION_PRIVILEGED_READ_ONLY           ( 2ULL << 2ULL )
    #define portMPU_REGION_READ_ONLY                      ( 3ULL << 2ULL )

    #define portMPU_REGION_EXECUTE_NEVER                  ( 1ULL << 1ULL )

    #define portMPU_PRLAR_EL1_ATTR_INDEX0                 ( 0ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX1                 ( 1ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX2                 ( 2ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX3                 ( 3ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX4                 ( 4ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX5                 ( 5ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX6                 ( 6ULL << 1ULL )
    #define portMPU_PRLAR_EL1_ATTR_INDEX7                 ( 7ULL << 1ULL )

    #define portMPU_PRLAR_EL1_REGION_ENABLE               ( 1ULL )

    #define portMPU_ENABLE_BIT                            ( 1ULL << 0ULL )
    #define portMPU_PRIV_BACKGROUND_ENABLE_BIT            ( 1ULL << 17ULL )

    /* uint64_t 类型可容纳的最大值。 */
    #define portUINT64_MAX                                ( ~( ( uint64_t ) 0 ) )
    #define portADD_UINT64_WILL_OVERFLOW( a, b )          ( ( a ) > ( portUINT64_MAX - ( b ) ) )

    /*
     * 从 PRBAR_EL1 寄存器值中提取 MPU 区域的起始地址。
     */
    #define portEXTRACT_FIRST_ADDRESS_FROM_PRBAR_EL1( prbar_el1 ) \
        ( ( prbar_el1 ) & portMPU_PRBAR_EL1_ADDRESS_MASK )

    /*
     * 从 PRLAR_EL1 寄存器值中提取 MPU 区域的结束地址。
     */
    #define portEXTRACT_LAST_ADDRESS_FROM_PRLAR_EL1( prlar_el1 ) \
        ( ( ( prlar_el1 ) & portMPU_PRLAR_EL1_ADDRESS_MASK ) | ~portMPU_PRLAR_EL1_ADDRESS_MASK )

    /* addr 是否位于 [start, end] 地址范围内？ */
    #define portIS_ADDRESS_WITHIN_RANGE( addr, start, end ) \
        ( ( ( addr ) >= ( start ) ) && ( ( addr ) <= ( end ) ) )

    /* 可用权限是否满足该访问请求？ */
    #define portIS_AUTHORIZED( accessRequest, permissions ) \
        ( ( ( permissions ) & ( accessRequest ) ) == accessRequest )

    /**
     * @brief 任务栈（上下文）中的偏移量。
     */
    #if ( configUSE_TASK_FPU_SUPPORT == portTASK_HAVE_FPU_CONTEXT_BY_DEFAULT )
        #define portOFFSET_TO_PC     ( 68 )
        #define portOFFSET_TO_LR     ( 70 )
        #define portOFFSET_TO_X0     ( 100 )
        #define portOFFSET_TO_X1     ( 101 )
        #define portOFFSET_TO_X2     ( 98 )
        #define portOFFSET_TO_X3     ( 99 )
    #else
        #define portOFFSET_TO_PC     ( 2 )
        #define portOFFSET_TO_LR     ( 4 )
        #define portOFFSET_TO_X0     ( 34 )
        #define portOFFSET_TO_X1     ( 35 )
        #define portOFFSET_TO_X2     ( 32 )
        #define portOFFSET_TO_X3     ( 33 )
    #endif

    /**
     * @brief 用于标记任务具有特权的标志。
     *
     * @ingroup Port Privilege
     */
    #define portTASK_IS_PRIVILEGED_FLAG   ( 1UL << 1UL )

    /**
     * @brief 检查调用任务是否具有特权。
     *
     * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
     */
    #define portIS_TASK_PRIVILEGED() xPortIsTaskPrivileged()

#else

    #define portPRIVILEGE_BIT        ( 0x0UL )

#endif /* configENABLE_MPU == 1 条件编译结束 */

#define portPSTATE_I_BIT             ( 0x7 )

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：PORTMACRO_H */
