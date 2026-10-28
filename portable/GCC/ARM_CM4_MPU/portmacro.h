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
    #error "configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width."
#endif

/*
 * 必须在 Cortex-M7 r0p0 和 r0p1 内核上启用
 * Errata 837070 的规避措施。
 */
#ifndef configENABLE_ERRATA_837070_WORKAROUND
    #define configENABLE_ERRATA_837070_WORKAROUND    0
#endif
/*-----------------------------------------------------------*/

/* MPU 专用常量。 */
#define portUSING_MPU_WRAPPERS                                   1
#define portPRIVILEGE_BIT                                        ( 0x80000000UL )

#define portMPU_REGION_READ_WRITE                                ( 0x03UL << 24UL )
#define portMPU_REGION_PRIVILEGED_READ_ONLY                      ( 0x05UL << 24UL )
#define portMPU_REGION_READ_ONLY                                 ( 0x06UL << 24UL )
#define portMPU_REGION_PRIVILEGED_READ_WRITE                     ( 0x01UL << 24UL )
#define portMPU_REGION_PRIVILEGED_READ_WRITE_UNPRIV_READ_ONLY    ( 0x02UL << 24UL )
#define portMPU_REGION_CACHEABLE_BUFFERABLE                      ( 0x07UL << 16UL )
#define portMPU_REGION_EXECUTE_NEVER                             ( 0x01UL << 28UL )

/*
 * MPU 区域属性与大小寄存器（RASR）中 TEX、S、C、B 位的位置。
 */
#define portMPU_RASR_TEX_S_C_B_LOCATION                          ( 16UL )
#define portMPU_RASR_TEX_S_C_B_MASK                              ( 0x3FUL )

/* MPU 设置，可在 FreeRTOSConfig.h 中覆盖。 */
#ifndef configTOTAL_MPU_REGIONS
    /* 为向后兼容而定义为 8。 */
    #define configTOTAL_MPU_REGIONS    ( 8UL )
#endif

/*
 * TEX、Shareable (S)、Cacheable (C) 和 Bufferable (B) 位定义内存类型，
 * 并在必要时定义内存区域的可缓存和可共享属性。
 *
 * TEX、C 和 B 位共同表示区域的内存类型，并且：
 * - 对于普通内存（Normal memory），表示区域的可缓存属性。
 * - 对于设备内存（Device memory），表示区域是否可共享。
 *
 * 对于普通内存区域，S 位表示区域是否可共享。对于强序（Strongly-ordered）
 * 和设备内存，S 位被忽略。
 *
 * 有关为非特权 Flash、特权 Flash 和特权 RAM 区域设置 TEX、S、C 和 B 位的
 * 方法，请参见下表。
 *
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | TEX | C | B | Memory type            |  Description or Normal region cacheability             |  Shareable?             |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 000 | 0 | 0 | Strongly-ordered       |  Strongly ordered                                      |  Shareable              |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 000 | 0 | 1 | Device                 |  Shared device                                         |  Shareable              |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 000 | 1 | 0 | Normal                 |  Outer and inner   write-through; no write allocate    |  S bit                  |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 000 | 1 | 1 | Normal                 |  Outer and inner   write-back; no write allocate       |  S bit                  |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 001 | 0 | 0 | Normal                 |  Outer and inner   Non-cacheable                       |  S bit                  |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 001 | 0 | 1 | Reserved               |  Reserved                                              |  Reserved               |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 001 | 1 | 0 | IMPLEMENTATION DEFINED |  IMPLEMENTATION DEFINED                                |  IMPLEMENTATION DEFINED |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 001 | 1 | 1 | Normal                 |  Outer and inner   write-back; write and read allocate |  S bit                  |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 010 | 0 | 0 | Device                 |  Non-shared device                                     |  Not shareable          |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 010 | 0 | 1 | Reserved               |  Reserved                                              |  Reserved               |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 010 | 1 | X | Reserved               |  Reserved                                              |  Reserved               |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 011 | X | X | Reserved               |  Reserved                                              |  Reserved               |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * | 1BB | A | A | Normal                 | Cached memory, with AA and BB indicating the inner and |  Reserved               |
 * |     |   |   |                        | outer cacheability rules that must be exported on the  |                         |
 * |     |   |   |                        | bus. See the table below for the cacheability policy   |                         |
 * |     |   |   |                        | encoding. memory, BB=Outer policy, AA=Inner policy.    |                         |
 * +-----+---+---+------------------------+--------------------------------------------------------+-------------------------+
 * |
 * +-----------------------------------------+----------------------------------------+
 * | AA or BB subfield of {TEX,C,B} encoding |  Cacheability policy                   |
 * +-----------------------------------------+----------------------------------------+
 * | 00                                      |  Non-cacheable                         |
 * +-----------------------------------------+----------------------------------------+
 * | 01                                      |  Write-back, write and   read allocate |
 * +-----------------------------------------+----------------------------------------+
 * | 10                                      |  Write-through, no write   allocate    |
 * +-----------------------------------------+----------------------------------------+
 * | 11                                      |  Write-back, no write   allocate       |
 * +-----------------------------------------+----------------------------------------+
 */

/*
 * Flash 区域的 TEX、Shareable (S)、Cacheable (C) 和 Bufferable (B) 位。
 */
#ifndef configTEX_S_C_B_FLASH
    /* 为向后兼容，默认 TEX=000、S=1、C=1、B=1。 */
    #define configTEX_S_C_B_FLASH    ( 0x07UL )
#endif

/*
 * RAM 区域的 TEX、Shareable (S)、Cacheable (C) 和 Bufferable (B) 位。
 */
#ifndef configTEX_S_C_B_SRAM
    /* 为向后兼容，默认 TEX=000、S=1、C=1、B=1。 */
    #define configTEX_S_C_B_SRAM          ( 0x07UL )
#endif

#define portSTACK_REGION                  ( configTOTAL_MPU_REGIONS - 5UL )
#define portGENERAL_PERIPHERALS_REGION    ( configTOTAL_MPU_REGIONS - 4UL )
#define portUNPRIVILEGED_FLASH_REGION     ( configTOTAL_MPU_REGIONS - 3UL )
#define portPRIVILEGED_FLASH_REGION       ( configTOTAL_MPU_REGIONS - 2UL )
#define portPRIVILEGED_RAM_REGION         ( configTOTAL_MPU_REGIONS - 1UL )
#define portFIRST_CONFIGURABLE_REGION     ( 0UL )
#define portLAST_CONFIGURABLE_REGION      ( configTOTAL_MPU_REGIONS - 6UL )
#define portNUM_CONFIGURABLE_REGIONS      ( configTOTAL_MPU_REGIONS - 5UL )
#define portTOTAL_NUM_REGIONS_IN_TCB      ( portNUM_CONFIGURABLE_REGIONS + 1 ) /* 加 1 以为栈区域留出空间。 */

typedef struct MPU_REGION_REGISTERS
{
    uint32_t ulRegionBaseAddress;
    uint32_t ulRegionAttribute;
} xMPU_REGION_REGISTERS;

typedef struct MPU_REGION_SETTINGS
{
    uint32_t ulRegionStartAddress;
    uint32_t ulRegionEndAddress;
    uint32_t ulRegionPermissions;
} xMPU_REGION_SETTINGS;

#if ( configUSE_MPU_WRAPPERS_V1 == 0 )

    #ifndef configSYSTEM_CALL_STACK_SIZE
        #error "configSYSTEM_CALL_STACK_SIZE must be defined to the desired size of the system call stack in words for using MPU wrappers v2."
    #endif

    typedef struct SYSTEM_CALL_STACK_INFO
    {
        uint32_t ulSystemCallStackBuffer[ configSYSTEM_CALL_STACK_SIZE ];
        uint32_t * pulSystemCallStack;
        uint32_t * pulTaskStack;
        uint32_t ulLinkRegisterAtSystemCallEntry;
    } xSYSTEM_CALL_STACK_INFO;

#endif /* configUSE_MPU_WRAPPERS_V1 == 0（使用 V2 MPU 包装器） */

/*
 * 任务上下文栈帧布局：
 * +---------+---------------+-----------------+-----------------+-----+
 * | s16-s31 | s0-s15, FPSCR | CONTROL, r4-r11 | PSP, r0-r3, r12 |     |
 * |         |               | EXC_RETURN      | LR, PC, xPSR    |     |
 * +---------+---------------+-----------------+-----------------+-----+
 *
 * <--------><---------------><----------------><----------------><---->
 *     16           17               10                 9           1
 */
#define MAX_CONTEXT_SIZE                    ( 53 )

/* 访问控制列表（ACL）表项的位宽。 */
#define portACL_ENTRY_SIZE_BITS             ( 32U )

/* xMPU_SETTINGS.ulTaskFlags 成员使用的标志位。 */
#define portSTACK_FRAME_HAS_PADDING_FLAG    ( 1UL << 0UL )
#define portTASK_IS_PRIVILEGED_FLAG         ( 1UL << 1UL )

typedef struct MPU_SETTINGS
{
    xMPU_REGION_REGISTERS xRegion[ portTOTAL_NUM_REGIONS_IN_TCB ];
    xMPU_REGION_SETTINGS xRegionSettings[ portTOTAL_NUM_REGIONS_IN_TCB ];
    uint32_t ulContext[ MAX_CONTEXT_SIZE ];
    uint32_t ulTaskFlags;

    #if ( configUSE_MPU_WRAPPERS_V1 == 0 )
        xSYSTEM_CALL_STACK_INFO xSystemCallStackInfo;
        #if ( configENABLE_ACCESS_CONTROL_LIST == 1 )
            uint32_t ulAccessControlList[ ( configPROTECTED_KERNEL_OBJECT_POOL_SIZE / portACL_ENTRY_SIZE_BITS ) + 1 ];
        #endif
    #endif
} xMPU_SETTINGS;

/* 体系结构相关定义。 */
#define portSTACK_GROWTH      ( -1 )
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT    8
#define portDONT_DISCARD      __attribute__( ( used ) )
/*-----------------------------------------------------------*/

/* 各种服务使用的 SVC 编号。 */
#define portSVC_START_SCHEDULER        100
#define portSVC_YIELD                  101
#define portSVC_RAISE_PRIVILEGE        102
#define portSVC_SYSTEM_CALL_EXIT       103

/* 调度器工具。 */

#define portYIELD()    __asm volatile ( "   SVC %0  \n" ::"i" ( portSVC_YIELD ) : "memory" )
#define portYIELD_WITHIN_API()                          \
    {                                                   \
        /* Set a PendSV to request a context switch. */ \
        portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
                                                        \
        /* Barriers are normally not required but do ensure the code is completely \
         * within the specified behaviour for the architecture. */ \
        __asm volatile ( "dsb" ::: "memory" );                     \
        __asm volatile ( "isb" );                                  \
    }

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

/* 临界区管理。 */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()         ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vPortSetBASEPRI( x )
#define portDISABLE_INTERRUPTS()                  vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()                   vPortSetBASEPRI( 0 )
#define portENTER_CRITICAL()                      vPortEnterCritical()
#define portEXIT_CRITICAL()                       vPortExitCritical()

/*-----------------------------------------------------------*/

/*
 * 如 FreeRTOS.org 网站所述的任务函数宏。
 * 本移植层并不需要使用它们。定义它们是为了让
 * 通用示例文件（可在所有移植层上构建）能够编译。
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
/*-----------------------------------------------------------*/

/* 体系结构相关优化。 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

/* 通用辅助函数。 */
    __attribute__( ( always_inline ) ) static inline uint8_t ucPortCountLeadingZeros( uint32_t ulBitmap )
    {
        uint8_t ucReturn;

        __asm volatile ( "clz %0, %1" : "=r" ( ucReturn ) : "r" ( ulBitmap ) : "memory" );

        return ucReturn;
    }

/* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error "configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice."
    #endif

/* 在位图中记录/清除就绪优先级。 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

/*-----------------------------------------------------------*/

    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )    uxTopPriority = ( 31UL - ( uint32_t ) ucPortCountLeadingZeros( ( uxReadyPriorities ) ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION 条件编译结束 */

/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )
    void vPortValidateInterruptPriority( void );
    #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()    vPortValidateInterruptPriority()
#endif

/* 本移植层不需要 portNOP()。 */
#define portNOP()

#define portINLINE              __inline

#ifndef portFORCE_INLINE
    #define portFORCE_INLINE    inline __attribute__( ( always_inline ) )
#endif
/*-----------------------------------------------------------*/

extern BaseType_t xIsPrivileged( void );
extern void vResetPrivilege( void );
extern void vPortSwitchToUserMode( void );

/**
 * @brief 检查处理器当前是否处于特权状态。
 *
 * @return 若处理器已处于特权状态则返回 1，否则返回 0。
 */
#define portIS_PRIVILEGED()          xIsPrivileged()

/**
 * @brief 触发 SVC 请求以提升特权。
 */
#define portRAISE_PRIVILEGE()        __asm volatile ( "svc %0 \n" ::"i" ( portSVC_RAISE_PRIVILEGE ) : "memory" );

/**
 * @brief 通过设置 CONTROL 寄存器的 bit 0 降低特权级别。
 */
#define portRESET_PRIVILEGE()        vResetPrivilege()

/**
 * @brief 将任务设置为非特权。
 *
 * 只能从特权任务中调用。从非特权任务调用将导致内存保护错误
 * （Memory Protection Fault）。
 */
#define portSWITCH_TO_USER_MODE()    vPortSwitchToUserMode()
/*-----------------------------------------------------------*/

extern BaseType_t xPortIsTaskPrivileged( void );

/**
 * @brief 检查调用任务是否具有特权。
 *
 * @return 若调用任务具有特权则返回 pdTRUE，否则返回 pdFALSE。
 */
#define portIS_TASK_PRIVILEGED()    xPortIsTaskPrivileged()
/*-----------------------------------------------------------*/

portFORCE_INLINE static BaseType_t xPortIsInsideInterrupt( void )
{
    uint32_t ulCurrentInterrupt;
    BaseType_t xReturn;

    /* 获取当前正在执行的中断编号。 */
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

portFORCE_INLINE static void vPortRaiseBASEPRI( void )
{
    uint32_t ulNewBASEPRI;

    __asm volatile
    (
        "   mov %0, %1                                              \n"
        #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
            "   cpsid i                                             \n" /* ARM Cortex-M7 r0p1 Errata 837070 的规避措施。 */
        #endif
        "   msr basepri, %0                                         \n"
        "   isb                                                     \n"
        "   dsb                                                     \n"
        #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
            "   cpsie i                                             \n" /* ARM Cortex-M7 r0p1 Errata 837070 的规避措施。 */
        #endif
        : "=r" ( ulNewBASEPRI ) : "i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) : "memory"
    );
}

/*-----------------------------------------------------------*/

portFORCE_INLINE static uint32_t ulPortRaiseBASEPRI( void )
{
    uint32_t ulOriginalBASEPRI, ulNewBASEPRI;

    __asm volatile
    (
        "   mrs %0, basepri                                         \n"
        "   mov %1, %2                                              \n"
        #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
            "   cpsid i                                             \n" /* ARM Cortex-M7 r0p1 Errata 837070 的规避措施。 */
        #endif
        "   msr basepri, %1                                         \n"
        "   isb                                                     \n"
        "   dsb                                                     \n"
        #if ( configENABLE_ERRATA_837070_WORKAROUND == 1 )
            "   cpsie i                                             \n" /* ARM Cortex-M7 r0p1 Errata 837070 的规避措施。 */
        #endif
        : "=r" ( ulOriginalBASEPRI ), "=r" ( ulNewBASEPRI ) : "i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) : "memory"
    );

    /*
     * 此返回语句不会被执行，但为消除编译器
     * 警告而保留。
     */
    return ulOriginalBASEPRI;
}
/*-----------------------------------------------------------*/

portFORCE_INLINE static void vPortSetBASEPRI( uint32_t ulNewMaskValue )
{
    __asm volatile
    (
        "   msr basepri, %0 " ::"r" ( ulNewMaskValue ) : "memory"
    );
}
/*-----------------------------------------------------------*/

#define portMEMORY_BARRIER()    __asm volatile ( "" ::: "memory" )

#ifndef configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY
    #warning "configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY is not defined. We recommend defining it to 1 in FreeRTOSConfig.h for better security. www.FreeRTOS.org/FreeRTOS-V10.3.x.html"
    #define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY    0
#endif
/*-----------------------------------------------------------*/

/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
    #ifdef __cplusplus
        }
    #endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* 头文件保护：PORTMACRO_H */
