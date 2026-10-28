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

#ifndef PORTMACRO_H
#define PORTMACRO_H

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

#ifndef configENABLE_MPU
    #error configENABLE_MPU must be defined in FreeRTOSConfig.h.  Set configENABLE_MPU to 1 to enable the MPU or 0 to disable the MPU.
#endif /* configENABLE_MPU（使能 MPU） */

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
    typedef uint16_t        TickType_t;
    #define portMAX_DELAY   ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t        TickType_t;
    #define portMAX_DELAY   ( TickType_t ) 0xffffffffUL

    /*
     * 32 位架构上使用 32 位 tick 类型，因此读取 tick 计数值
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
#define portARCH_NAME                      "Cortex-M0+"
#define portSTACK_GROWTH                   ( -1 )
#define portTICK_PERIOD_MS                 ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT                 8
#define portNOP()
#define portINLINE                         __inline
#ifndef portFORCE_INLINE
    #define portFORCE_INLINE               inline __attribute__( ( always_inline ) )
#endif
#define portDONT_DISCARD                   __attribute__( ( used ) )
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

#if ( configENABLE_MPU == 1 )
    extern BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked ))（裸函数） */;
    extern void vResetPrivilege( void ) /* __attribute__ (( naked ))（裸函数） */;
#endif /* configENABLE_MPU（使能 MPU） */
/*-----------------------------------------------------------*/

/**
 * @brief MPU 专用常量。
 */
#if ( configENABLE_MPU == 1 )
    #define portUSING_MPU_WRAPPERS          1
    #define portPRIVILEGE_BIT               ( 0x80000000UL )
#else
    #define portPRIVILEGE_BIT               ( 0x0UL )
#endif /* configENABLE_MPU（使能 MPU） */

/* Flash 区域的 Shareable（S）、Cacheable（C）和 Bufferable（B）位。 */
#ifndef configS_C_B_FLASH
    #define configS_C_B_FLASH               ( 0x07UL )
#endif

/* RAM 区域的 Shareable（S）、Cacheable（C）和 Bufferable（B）位。 */
#ifndef configS_C_B_SRAM
    #define configS_C_B_SRAM                ( 0x07UL )
#endif

/* MPU 区域。 */
#define portPRIVILEGED_RAM_REGION           ( 7UL )
#define portPRIVILEGED_FLASH_REGION         ( 6UL )
#define portUNPRIVILEGED_FLASH_REGION       ( 5UL )
#define portSTACK_REGION                    ( 4UL )
#define portFIRST_CONFIGURABLE_REGION       ( 0UL )
#define portLAST_CONFIGURABLE_REGION        ( 3UL )
#define portNUM_CONFIGURABLE_REGIONS        ( 4UL )
#define portTOTAL_NUM_REGIONS               ( portNUM_CONFIGURABLE_REGIONS + 1UL ) /* 加一，为栈区域留出空间。 */

/*
 * MPU 区域大小。该信息编码在 MPU 区域属性与大小寄存器（RASR）
 * 的 SIZE 位中。
 */
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
 * MPU 内存类型。该信息编码在 MPU 区域属性与大小寄存器（RASR）
 * 的 S（Shareable）、C（Cacheable）和 B（Bufferable）位中。
 */
#define portMPU_REGION_STRONGLY_ORDERED_SHAREABLE   ( 0x0UL << 16UL ) /* S=NA，C=0，B=0（不共享、不缓存、不缓冲）。 */
#define portMPU_REGION_DEVICE_SHAREABLE             ( 0x1UL << 16UL ) /* S=NA，C=0，B=1（不共享、不缓存、可缓冲）。 */
#define portMPU_REGION_NORMAL_OIWTNOWA_NONSHARED    ( 0x2UL << 16UL ) /* S=0，C=1，B=0（不共享、可缓存、不缓冲）。 */
#define portMPU_REGION_NORMAL_OIWTNOWA_SHARED       ( 0x6UL << 16UL ) /* S=1，C=1，B=0（共享、可缓存、不缓冲）。 */
#define portMPU_REGION_NORMAL_OIWBNOWA_NONSHARED    ( 0x3UL << 16UL ) /* S=0，C=1，B=1（不共享、可缓存、可缓冲）。 */
#define portMPU_REGION_NORMAL_OIWBNOWA_SHARED       ( 0x7UL << 16UL ) /* S=1，C=1，B=1（共享、可缓存、可缓冲）。 */

/*
 * MPU 访问权限。该信息编码在 MPU 区域属性与大小寄存器（RASR）
 * 的 AP 和 XN 位中。
 */
#define portMPU_REGION_PRIV_NA_UNPRIV_NA            ( 0x0UL << 24UL )
#define portMPU_REGION_PRIV_RW_UNPRIV_NA            ( 0x1UL << 24UL )
#define portMPU_REGION_PRIV_RW_UNPRIV_RO            ( 0x2UL << 24UL )
#define portMPU_REGION_PRIV_RW_UNPRIV_RW            ( 0x3UL << 24UL )
#define portMPU_REGION_PRIV_RO_UNPRIV_NA            ( 0x5UL << 24UL )
#define portMPU_REGION_PRIV_RO_UNPRIV_RO            ( 0x6UL << 24UL )
#define portMPU_REGION_EXECUTE_NEVER                ( 0x1UL << 28UL )

#if ( configENABLE_MPU == 1 )

    /**
     * @brief 用于定义 MPU 区域的设置。
     */
    typedef struct MPURegionSettings
    {
        uint32_t ulRBAR; /**< MPU 区域的区域基地址寄存器（RBAR）。 */
        uint32_t ulRASR; /**< MPU 区域的区域属性与大小寄存器（RASR）。 */
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
            uint32_t * pulTaskStack;
            uint32_t ulLinkRegisterAtSystemCallEntry;
        } xSYSTEM_CALL_STACK_INFO;

    #endif /* configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

    /**
     * @brief 存储在 TCB 中的 MPU 设置。
     */

    /*
     * TCB 中的上下文布局如下：
     * +----------+-----------------+---------------+-----+
     * |  r4-r11  | r0-r3, r12, LR, | PSP, CONTROL  |     |
     * |          | PC, xPSR        | EXC_RETURN    |     |
     * +----------+-----------------+---------------+-----+
     *
     * <---------><----------------><---------------><---->
     *     8               8                3          1
     */
    #define CONTEXT_SIZE    20

    /* xMPU_SETTINGS.ulTaskFlags 成员使用的标志位。 */
    #define portSTACK_FRAME_HAS_PADDING_FLAG    ( 1UL << 0UL )
    #define portTASK_IS_PRIVILEGED_FLAG         ( 1UL << 1UL )

    /* 访问控制列表（ACL）表项的位宽。 */
    #define portACL_ENTRY_SIZE_BITS             ( 32U )

    typedef struct MPU_SETTINGS
    {
        MPURegionSettings_t xRegionsSettings[ portTOTAL_NUM_REGIONS ]; /**< 每个任务的 4 个区域设置。 */
        uint32_t ulContext[ CONTEXT_SIZE ];
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
 * @brief SVC 编号。
 */
#define portSVC_START_SCHEDULER            100
#define portSVC_RAISE_PRIVILEGE            101
#define portSVC_SYSTEM_CALL_EXIT           102
#define portSVC_YIELD                      103
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
#define portSET_INTERRUPT_MASK_FROM_ISR()       ulSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )  vClearInterruptMask( x )
#define portDISABLE_INTERRUPTS()                __asm volatile ( " cpsid i " ::: "memory" )
#define portENABLE_INTERRUPTS()                 __asm volatile ( " cpsie i " ::: "memory" )
#define portENTER_CRITICAL()                    vPortEnterCritical()
#define portEXIT_CRITICAL()                     vPortExitCritical()
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
 * @brief 如 FreeRTOS.org 网站所述的任务函数宏。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
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

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* PORTMACRO_H（头文件保护宏） */
