/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright 2024, 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
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

#ifndef PORTMACROCOMMON_H
#define PORTMACROCOMMON_H

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

/*
 * ------------------------------------------------------------------------------
 * 移植层专用定义。
 *
 * 本文件中的设置针对给定硬件和编译器正确配置 FreeRTOS。
 *
 * 不应修改这些设置。
 * ------------------------------------------------------------------------------
 */

#ifndef configENABLE_FPU
    #error configENABLE_FPU must be defined in FreeRTOSConfig.h.  Set configENABLE_FPU to 1 to enable the FPU or 0 to disable the FPU.
#endif /* configENABLE_FPU（使能 FPU） */

#ifndef configENABLE_MPU
    #error configENABLE_MPU must be defined in FreeRTOSConfig.h.  Set configENABLE_MPU to 1 to enable the MPU or 0 to disable the MPU.
#endif /* configENABLE_MPU（使能 MPU） */

#ifndef configENABLE_TRUSTZONE
    #error configENABLE_TRUSTZONE must be defined in FreeRTOSConfig.h.  Set configENABLE_TRUSTZONE to 1 to enable TrustZone or 0 to disable TrustZone.
#endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

#if ( configNUMBER_OF_CORES > 1 )
    #if ( portVALIDATED_FOR_SMP != 1 ) || ( configENABLE_MPU == 1 ) || ( configENABLE_TRUSTZONE == 1 )
        #error "Multi-core SMP is currently only validated for Cortex-M33 non-TrustZone non-MPU port."
    #endif /* if ( portVALIDATED_FOR_SMP != 1 ) || ( configENABLE_MPU == 1 ) || ( configENABLE_TRUSTZONE == 1 ) )（条件结束） */

    #ifndef configCORE_ID_REGISTER
        #error "configCORE_ID_REGISTER must be defined to the address of the register used to identify the core executing the code."
    #endif /* ifndef configCORE_ID_REGISTER（条件结束） */

    #ifndef configWAKE_SECONDARY_CORES
        #error "configWAKE_SECONDARY_CORES must be defined to a function that wakes the secondary cores."
    #endif /* ifndef configWAKE_SECONDARY_CORES（条件结束） */
#endif /* #if ( configNUMBER_OF_CORES > 1 )（条件结束） */
/*-----------------------------------------------------------*/

/**
 * @brief 类型定义。
 */
#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE     long

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffffffffUL

/*
 * 32 位架构上使用 32 位时钟节拍类型，因此读取节拍计数
 * 无需用临界区保护。
 */
    #define portTICK_TYPE_IS_ATOMIC    1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/**
 * 架构相关定义。
 */
#define portSTACK_GROWTH                   ( -1 )
#define portTICK_PERIOD_MS                 ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT                 8
#define portNOP()
#define portINLINE                         __inline
#ifndef portFORCE_INLINE
    #define portFORCE_INLINE               inline __attribute__( ( always_inline ) )
#endif
#define portHAS_STACK_OVERFLOW_CHECKING    1
/*-----------------------------------------------------------*/

/**
 * @brief 外部声明。
 */
extern BaseType_t xPortIsInsideInterrupt( void );

extern void vPortYield( void ) /* PRIVILEGED_FUNCTION（特权函数） */;

extern void vPortEnterCritical( void ) /* PRIVILEGED_FUNCTION（特权函数） */;
extern void vPortExitCritical( void ) /* PRIVILEGED_FUNCTION（特权函数） */;

extern uint32_t ulSetInterruptMask( void ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */;
extern void vClearInterruptMask( uint32_t ulMask ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */;

#if ( configENABLE_TRUSTZONE == 1 )
    extern void vPortAllocateSecureContext( uint32_t ulSecureStackSize ); /* __attribute__ (( naked ))（裸函数） */
    extern void vPortFreeSecureContext( uint32_t * pulTCB ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION（裸函数，特权函数） */;
#endif /* configENABLE_TRUSTZONE（使能 TrustZone） */

#if ( configENABLE_MPU == 1 )
    extern BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked ))（裸函数） */;
    extern void vResetPrivilege( void ) /* __attribute__ (( naked ))（裸函数） */;
#endif /* configENABLE_MPU（使能 MPU） */

#if ( configENABLE_PAC == 1 )

    /**
     * @brief 生成任务的 128 位随机 PAC 密钥。
     *
     * @param[out] pulTaskPacKey 指向 4 字（128 位）数组的指针，该数组
     *             将填入 128 位随机数。
     */
    void vApplicationGenerateTaskRandomPacKey( uint32_t * pulTaskPacKey );

#endif /* configENABLE_PAC（使能 PAC） */
/*-----------------------------------------------------------*/

/**
 * @brief MPU 专用常量。
 */
#if ( configENABLE_MPU == 1 )
    #define portUSING_MPU_WRAPPERS    1
    #define portPRIVILEGE_BIT         ( 0x80000000UL )
#else
    #define portPRIVILEGE_BIT         ( 0x0UL )
#endif /* configENABLE_MPU（使能 MPU） */

/* MPU 设置，可在 FreeRTOSConfig.h 中覆盖。 */
#ifndef configTOTAL_MPU_REGIONS
    /* 为向后兼容而定义为 8。 */
    #define configTOTAL_MPU_REGIONS    ( 8UL )
#endif

/* MPU 区域。 */
#define portPRIVILEGED_FLASH_REGION                   ( 0UL )
#define portUNPRIVILEGED_FLASH_REGION                 ( 1UL )
#define portUNPRIVILEGED_SYSCALLS_REGION              ( 2UL )
#define portPRIVILEGED_RAM_REGION                     ( 3UL )
#define portSTACK_REGION                              ( 4UL )
#define portFIRST_CONFIGURABLE_REGION                 ( 5UL )
#define portLAST_CONFIGURABLE_REGION                  ( configTOTAL_MPU_REGIONS - 1UL )
#define portNUM_CONFIGURABLE_REGIONS                  ( ( portLAST_CONFIGURABLE_REGION - portFIRST_CONFIGURABLE_REGION ) + 1 )
#define portTOTAL_NUM_REGIONS                         ( portNUM_CONFIGURABLE_REGIONS + 1 )       /* 加一，为栈区域留出空间。 */

/*
 * MPU_MAIR 寄存器中使用的设备内存属性。
 *
 * 8 位值的编码如下：
 *  Bit[7:4] - 0000 - 设备内存
 *  Bit[3:2] - 00 --> Device-nGnRnE
 *              01 --> Device-nGnRE
 *              10 --> Device-nGRE
 *              11 --> Device-GRE
 *  Bit[1:0] - 00，保留。
 */
#define portMPU_DEVICE_MEMORY_nGnRnE                  ( 0x00 )       /* 0000 0000 */
#define portMPU_DEVICE_MEMORY_nGnRE                   ( 0x04 )       /* 0000 0100 */
#define portMPU_DEVICE_MEMORY_nGRE                    ( 0x08 )       /* 0000 1000 */
#define portMPU_DEVICE_MEMORY_GRE                     ( 0x0C )       /* 0000 1100 */

/* MPU_MAIR 寄存器中使用的普通内存属性。 */
#define portMPU_NORMAL_MEMORY_NON_CACHEABLE           ( 0x44 )       /* 不可缓存。 */
#define portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE    ( 0xFF )       /* 非瞬态、写回、读分配和写分配。 */

/* MPU_RBAR 寄存器中使用的属性。 */
#define portMPU_REGION_NON_SHAREABLE                  ( 0UL << 3UL )
#define portMPU_REGION_INNER_SHAREABLE                ( 1UL << 3UL )
#define portMPU_REGION_OUTER_SHAREABLE                ( 2UL << 3UL )

#define portMPU_REGION_PRIVILEGED_READ_WRITE          ( 0UL << 1UL )
#define portMPU_REGION_READ_WRITE                     ( 1UL << 1UL )
#define portMPU_REGION_PRIVILEGED_READ_ONLY           ( 2UL << 1UL )
#define portMPU_REGION_READ_ONLY                      ( 3UL << 1UL )

#define portMPU_REGION_EXECUTE_NEVER                  ( 1UL )
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 用于定义 MPU 区域的设置。
     */
    typedef struct MPURegionSettings
    {
        uint32_t ulRBAR; /**< 区域的 RBAR。 */
        uint32_t ulRLAR; /**< 区域的 RLAR。 */
    } MPURegionSettings_t;

    #if ( configUSE_MPU_WRAPPERS_V1 == 0 )

        #ifndef configSYSTEM_CALL_STACK_SIZE
            #error configSYSTEM_CALL_STACK_SIZE must be defined to the desired size of the system call stack in words for using MPU wrappers v2.
        #endif

        /**
         * @brief 系统调用栈。
         */
        typedef struct SYSTEM_CALL_STACK_INFO
        {
            uint32_t ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE ];
            uint32_t * pulSystemCallStack;
            uint32_t * pulSystemCallStackLimit;
            uint32_t * pulTaskStack;
            uint32_t ulLinkRegisterAtSystemCallEntry;
            uint32_t ulStackLimitRegisterAtSystemCallEntry;
        } xSYSTEM_CALL_STACK_INFO;

    #endif /* configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

    /**
     * @brief 存储在 TCB 中的 MPU 设置。
     */
    #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )

        #if ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 1 ) )

            /*
             * 任务上下文栈帧布局（使能 FPU/MVE、TrustZone 与 PAC）：
             * +-----------+---------------+----------+-----------------+------------------------------+------------+-----+
             * |  s16-s31  | s0-s15, FPSCR |  r4-r11  | r0-r3, r12, LR, | xSecureContext, PSP, PSPLIM, | TaskPacKey |     |
             * |           |               |          | PC, xPSR        | CONTROL, EXC_RETURN          |            |     |
             * +-----------+---------------+----------+-----------------+------------------------------+------------+-----+
             *
             * <-----------><--------------><---------><----------------><-----------------------------><-----------><---->
             *      16             17            8               8                     5                     16         1
             */
            #define MAX_CONTEXT_SIZE    71

        #elif ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 0 ) )

            /*
             * 任务上下文栈帧布局（使能 FPU/MVE 与 TrustZone，未使能 PAC）：
             * +-----------+---------------+----------+-----------------+------------------------------+-----+
             * |  s16-s31  | s0-s15, FPSCR |  r4-r11  | r0-r3, r12, LR, | xSecureContext, PSP, PSPLIM, |     |
             * |           |               |          | PC, xPSR        | CONTROL, EXC_RETURN          |     |
             * +-----------+---------------+----------+-----------------+------------------------------+-----+
             *
             * <-----------><--------------><---------><----------------><-----------------------------><---->
             *      16             17            8               8                     5                   1
             */
            #define MAX_CONTEXT_SIZE    55

        #elif ( ( configENABLE_TRUSTZONE == 0 ) && ( configENABLE_PAC == 1 ) )

            /*
             * 任务上下文栈帧布局（使能 FPU/MVE 与 PAC，未使能 TrustZone）：
             * +-----------+---------------+----------+-----------------+----------------------+------------+-----+
             * |  s16-s31  | s0-s15, FPSCR |  r4-r11  | r0-r3, r12, LR, | PSP, PSPLIM, CONTROL | TaskPacKey |     |
             * |           |               |          | PC, xPSR        | EXC_RETURN           |            |     |
             * +-----------+---------------+----------+-----------------+----------------------+------------+-----+
             *
             * <-----------><--------------><---------><----------------><---------------------><-----------><---->
             *      16             17            8               8                  4                16         1
             */
            #define MAX_CONTEXT_SIZE    70

        #else /* if ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 1 ) )（条件结束） */

            /*
             * 任务上下文栈帧布局（使能 FPU/MVE，未使能 TrustZone 与 PAC）：
             * +-----------+---------------+----------+-----------------+----------------------+-----+
             * |  s16-s31  | s0-s15, FPSCR |  r4-r11  | r0-r3, r12, LR, | PSP, PSPLIM, CONTROL |     |
             * |           |               |          | PC, xPSR        | EXC_RETURN           |     |
             * +-----------+---------------+----------+-----------------+----------------------+-----+
             *
             * <-----------><--------------><---------><----------------><---------------------><---->
             *      16             17            8               8                  4              1
             */
            #define MAX_CONTEXT_SIZE    54

        #endif /* #if ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 1 ) )（条件结束） */

    #else /* #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )（条件结束） */

        #if ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 1 ) )

            /*
             * 任务上下文栈帧布局（未使能 FPU/MVE，使能 TrustZone 与 PAC）：
             * +----------+-----------------+------------------------------+------------+-----+
             * |  r4-r11  | r0-r3, r12, LR, | xSecureContext, PSP, PSPLIM, | TaskPacKey |     |
             * |          | PC, xPSR        | CONTROL, EXC_RETURN          |            |     |
             * +----------+-----------------+------------------------------+------------+-----+
             *
             * <---------><----------------><------------------------------><-----------><---->
             *     8               8                      5                      16         1
             */
            #define MAX_CONTEXT_SIZE    38

        #elif ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 0 ) )

            /*
             * 任务上下文栈帧布局（未使能 FPU/MVE，使能 TrustZone，未使能 PAC）：
             * +----------+-----------------+------------------------------+-----+
             * |  r4-r11  | r0-r3, r12, LR, | xSecureContext, PSP, PSPLIM, |     |
             * |          | PC, xPSR        | CONTROL, EXC_RETURN          |     |
             * +----------+-----------------+------------------------------+-----+
             *
             * <---------><----------------><------------------------------><---->
             *     8               8                      5                   1
             */
            #define MAX_CONTEXT_SIZE    22

        #elif ( ( configENABLE_TRUSTZONE == 0 ) && ( configENABLE_PAC == 1 ) )

            /*
             * 任务上下文栈帧布局（未使能 FPU/MVE，未使能 TrustZone，使能 PAC）：
             * +----------+-----------------+----------------------+------------+-----+
             * |  r4-r11  | r0-r3, r12, LR, | PSP, PSPLIM, CONTROL | TaskPacKey |     |
             * |          | PC, xPSR        | EXC_RETURN           |            |     |
             * +----------+-----------------+----------------------+------------+-----+
             *
             * <---------><----------------><----------------------><-----------><---->
             *     8               8                  4                  16         1
             */
            #define MAX_CONTEXT_SIZE    37

        #else /* #if( configENABLE_TRUSTZONE == 1 )（条件结束） */

            /*
             * 任务上下文栈帧布局（未使能 FPU/MVE、TrustZone 与 PAC）：
             * +----------+-----------------+----------------------+-----+
             * |  r4-r11  | r0-r3, r12, LR, | PSP, PSPLIM, CONTROL |     |
             * |          | PC, xPSR        | EXC_RETURN           |     |
             * +----------+-----------------+----------------------+-----+
             *
             * <---------><----------------><----------------------><---->
             *     8               8                  4              1
             */
            #define MAX_CONTEXT_SIZE    21

        #endif /* #if ( ( configENABLE_TRUSTZONE == 1 ) && ( configENABLE_PAC == 1 ) )（条件结束） */

    #endif /* #if ( ( configENABLE_FPU == 1 ) || ( configENABLE_MVE == 1 ) )（条件结束） */

    /* xMPU_SETTINGS.ulTaskFlags 成员使用的标志位。 */
    #define portSTACK_FRAME_HAS_PADDING_FLAG    ( 1UL << 0UL )
    #define portTASK_IS_PRIVILEGED_FLAG         ( 1UL << 1UL )

    /* 访问控制列表（ACL）表项的位宽。 */
    #define portACL_ENTRY_SIZE_BITS             ( 32U )

    typedef struct MPU_SETTINGS
    {
        uint32_t ulMAIR0;                                              /**< 任务的 MAIR0，包含全部 4 个任务区域的属性。 */
        MPURegionSettings_t xRegionsSettings[ portTOTAL_NUM_REGIONS ]; /**< 每个任务的 4 个区域设置。 */
        uint32_t ulContext[ MAX_CONTEXT_SIZE ];
        uint32_t ulTaskFlags;

        #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
            xSYSTEM_CALL_STACK_INFO xSystemCallStackInfo;
            #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
                uint32_t ulAccessControlList[ ( configPROTECTED_KERNEL_OBJECT_POOL_SIZE / portACL_ENTRY_SIZE_BITS ) + 1 ];
            #endif
        #endif
    } xMPU_SETTINGS;

#endif /* configENABLE_MPU == 1（已启用 MPU） */
/*-----------------------------------------------------------*/

/**
 * @brief 校验允许调用 FreeRTOS 系统调用的 ISR 的优先级。
 */
#if ( configASSERT_DEFINED == 1 )
    #if ( portHAS_ARMV8M_MAIN_EXTENSION == 1 )
        void vPortValidateInterruptPriority( void );
        #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()    vPortValidateInterruptPriority()
    #endif
#endif

/**
 * @brief SVC 编号。
 */
#define portSVC_ALLOCATE_SECURE_CONTEXT    100
#define portSVC_FREE_SECURE_CONTEXT        101
#define portSVC_START_SCHEDULER            102
#define portSVC_RAISE_PRIVILEGE            103
#define portSVC_SYSTEM_CALL_EXIT           104
#define portSVC_YIELD                      105
/*-----------------------------------------------------------*/

/**
 * @brief 调度器工具。
 */
#if ( configENABLE_MPU == 1 )
    #define portYIELD()               __asm volatile ( "svc %0" ::"i" ( portSVC_YIELD ) : "memory" )
    #define portYIELD_WITHIN_API()    vPortYield()
#else
    #define portYIELD()               vPortYield()
    #define portYIELD_WITHIN_API()    vPortYield()
#endif

#define portNVIC_INT_CTRL_REG     ( *( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT    ( 1UL << 28UL )
#define portEND_SWITCHING_ISR( xSwitchRequired )            \
    do                                                      \
    {                                                       \
        if( xSwitchRequired )                               \
        {                                                   \
            traceISR_EXIT_TO_SCHEDULER();                   \
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
        }                                                   \
        else                                                \
        {                                                   \
            traceISR_EXIT();                                \
        }                                                   \
    } while( 0 )
#define portYIELD_FROM_ISR( x )    portEND_SWITCHING_ISR( x )
/*-----------------------------------------------------------*/

/**
 * @brief 临界区管理。
 */

#define portSET_INTERRUPT_MASK()                  ulSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK( x )             vClearInterruptMask( x )
#define portSET_INTERRUPT_MASK_FROM_ISR()         ulSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vClearInterruptMask( x )

#if ( configNUMBER_OF_CORES == 1 )
    #define portENTER_CRITICAL()                      vPortEnterCritical()
    #define portEXIT_CRITICAL()                       vPortExitCritical()
#else /* ( configNUMBER_OF_CORES == 1 )（否则分支） */
    extern void vTaskEnterCritical( void );
    extern void vTaskExitCritical( void );
    extern UBaseType_t vTaskEnterCriticalFromISR( void );
    extern void vTaskExitCriticalFromISR( UBaseType_t uxSavedInterruptStatus );

    #define portENTER_CRITICAL()                      vTaskEnterCritical()
    #define portEXIT_CRITICAL()                       vTaskExitCritical()
    #define portENTER_CRITICAL_FROM_ISR()             vTaskEnterCriticalFromISR()
    #define portEXIT_CRITICAL_FROM_ISR( x )           vTaskExitCriticalFromISR( x )
#endif /* if ( configNUMBER_OF_CORES != 1 )（条件结束） */
/*-----------------------------------------------------------*/

/**
 * @brief Tickless 空闲/低功耗功能。
 */
#ifndef portSUPPRESS_TICKS_AND_SLEEP
    extern void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );
    #define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime )    vPortSuppressTicksAndSleep( xExpectedIdleTime )
#endif
/*-----------------------------------------------------------*/

/**
 * @brief FreeRTOS.org 网站所述的任务函数宏。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
/*-----------------------------------------------------------*/

#if ( configENABLE_TRUSTZONE == 1 )

/**
 * @brief 为任务分配安全上下文。
 *
 * 任务创建时并不带有安全上下文。任何将要调用安全函数的任务都必须
 * 调用 portALLOCATE_SECURE_CONTEXT() 为自身分配安全上下文，然后才能
 * 调用任何安全函数。
 *
 * @param[in] ulSecureStackSize 待分配的安全栈大小。
 */
    #define portALLOCATE_SECURE_CONTEXT( ulSecureStackSize )    vPortAllocateSecureContext( ulSecureStackSize )

/**
 * @brief 在删除任务时调用，用于删除该任务的安全上下文（若存在）。
 *
 * @param[in] pxTCB 正在删除的任务的 TCB。
 */
    #define portCLEAN_UP_TCB( pxTCB )                           vPortFreeSecureContext( ( uint32_t * ) pxTCB )
#endif /* configENABLE_TRUSTZONE（使能 TrustZone） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

/**
 * @brief 检查处理器当前是否处于特权状态。
 *
 * @return 若处理器已处于特权状态则返回 1，否则返回 0。
 */
    #define portIS_PRIVILEGED()      xIsPrivileged()

/**
 * @brief 触发 SVC 请求以提升特权。
 *
 * SVC 处理程序会检查 SVC 是否从系统调用触发，只有满足该条件
 * 才会提升特权。若从其他任何位置调用，则不会提升特权。
 */
    #define portRAISE_PRIVILEGE()    __asm volatile ( "svc %0 \n" ::"i" ( portSVC_RAISE_PRIVILEGE ) : "memory" );

/**
 * @brief 通过设置 CONTROL 寄存器的 bit 0 降低特权级别。
 */
    #define portRESET_PRIVILEGE()    vResetPrivilege()
#else
    #define portIS_PRIVILEGED()
    #define portRAISE_PRIVILEGE()
    #define portRESET_PRIVILEGE()
#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

#if ( configENABLE_MPU == 1 )

    extern BaseType_t xPortIsTaskPrivileged( void );

/**
 * @brief 检查调用任务是否具有特权。
 *
 * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
 */
    #define portIS_TASK_PRIVILEGED()    xPortIsTaskPrivileged()

#endif /* configENABLE_MPU == 1（已启用 MPU） */
/*-----------------------------------------------------------*/

/**
 * @brief 屏障指令。
 */
#define portMEMORY_BARRIER()    __asm volatile ( "" ::: "memory" )
/*-----------------------------------------------------------*/

/*
 * 根据是否实现 Mainline 扩展，选择 configUSE_PORT_OPTIMISED_TASK_SELECTION
 * 的正确值。
 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #if ( ( portHAS_ARMV8M_MAIN_EXTENSION == 1 ) && ( configNUMBER_OF_CORES == 1 ) )
        #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
    #else
        #define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
    #endif
#endif /* #ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION（条件结束） */

/**
 * @brief 移植层优化的任务选择。
 */
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1 )

/**
 * @brief 统计 32 位数值中前导零的个数。
 */
    static portFORCE_INLINE uint32_t ulPortCountLeadingZeros( uint32_t ulBitmap )
    {
        uint32_t ulReturn;

        __asm volatile ( "clz %0, %1" : "=r" ( ulReturn ) : "r" ( ulBitmap ) : "memory" );

        return ulReturn;
    }

/* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 different priorities as tasks that share a priority will time slice.
    #endif

    #if ( portHAS_ARMV8M_MAIN_EXTENSION == 0 )
        #error ARMv8-M baseline implementations (such as Cortex-M23) do not support port-optimised task selection.  Please set configUSE_PORT_OPTIMISED_TASK_SELECTION to 0 or leave it undefined.
    #endif

/**
 * @brief 将就绪优先级存储到位图中/从位图中清除。
 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )      ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )       ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

/**
 * @brief 获取已就绪可执行的最高优先级任务的优先级。
 */
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )    uxTopPriority = ( 31UL - ulPortCountLeadingZeros( ( uxReadyPriorities ) ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */
/*-----------------------------------------------------------*/


#if ( configNUMBER_OF_CORES > 1 )
    typedef enum
    {
        eIsrLock = 0,
        eTaskLock,
        eLockCount
    } ePortRTOSLock;

    extern volatile uint32_t ulCriticalNestings[ configNUMBER_OF_CORES ];
    extern void vPortRecursiveLock( uint8_t ucCoreID,
                                    ePortRTOSLock eLockNum,
                                    BaseType_t uxAcquire );
    extern uint8_t ucPortGetCoreID( void );
    extern void vInterruptCore( uint8_t ucCoreID );

    #define portGET_CORE_ID()                                  ucPortGetCoreID()

    #define portGET_CRITICAL_NESTING_COUNT( xCoreID )          ( ulCriticalNestings[ ( uint8_t ) xCoreID ] )
    #define portSET_CRITICAL_NESTING_COUNT( xCoreID, x )       ( ulCriticalNestings[ ( uint8_t ) xCoreID ] = ( x ) )
    #define portINCREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( ulCriticalNestings[ ( uint8_t ) xCoreID ]++ )
    #define portDECREMENT_CRITICAL_NESTING_COUNT( xCoreID )    ( ulCriticalNestings[ ( uint8_t ) xCoreID ]-- )

    #define portMAX_CORE_COUNT                                 ( configNUMBER_OF_CORES )

    #define portYIELD_CORE( xCoreID )                          vInterruptCore( xCoreID )

    #define portRELEASE_ISR_LOCK( xCoreID )                    vPortRecursiveLock( ( uint8_t ) xCoreID, eIsrLock, pdFALSE )
    #define portGET_ISR_LOCK( xCoreID )                        vPortRecursiveLock( ( uint8_t ) xCoreID, eIsrLock, pdTRUE )

    #define portRELEASE_TASK_LOCK( xCoreID )                   vPortRecursiveLock( ( uint8_t ) xCoreID, eTaskLock, pdFALSE )
    #define portGET_TASK_LOCK( xCoreID )                       vPortRecursiveLock( ( uint8_t ) xCoreID, eTaskLock, pdTRUE )

    #if ( ( configENABLE_PAC == 1 ) || ( configENABLE_BTI == 1 ) )
        uint32_t vConfigurePACBTI( BaseType_t xWriteControlRegister );
    #endif /* ( configENABLE_PAC == 1 || configENABLE_BTI == 1 )（条件结束） */
#endif

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* PORTMACROCOMMON_H（头文件保护宏） */
