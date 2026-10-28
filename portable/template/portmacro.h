/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * license and copyright intentionally withheld to promote copying into user code.
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

/*
 * 移植层专用定义。
 *
 * 本文件中的设置针对给定硬件和编译器正确配置 FreeRTOS。
 *
 * 不应修改这些设置。
 */

/* 类型定义。 */
#define portCHAR                 char
#define portFLOAT                float
#define portDOUBLE               double
#define portLONG                 long
#define portSHORT                int
#define portSTACK_TYPE           uint8_t
#define portBASE_TYPE            char

#define portSTACK_GROWTH         ( -1 )
#define portBYTE_ALIGNMENT       4
#define portPOINTER_SIZE_TYPE    size_t
typedef portSTACK_TYPE   StackType_t;
typedef signed char      BaseType_t;
typedef unsigned char    UBaseType_t;

#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t     TickType_t;
    #define portMAX_DELAY    ( TickType_t ) 0xffffU
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY    ( TickType_t ) 0xffffffffU
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_64_BITS )
    typedef uint64_t     TickType_t;
    #define portMAX_DELAY    ( TickType_t ) 0xffffffffffffffffU
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif

/* 架构相关的优化。 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

/* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
    #endif

/* 在位图中记录/清除就绪优先级。 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

/*-----------------------------------------------------------*/

    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) \
    do {                                                                 \
        uxTopPriority = 0;                                               \
    } while( 0 )

#endif /* 配置项 configUSE_PORT_OPTIMISED_TASK_SELECTION */

/* 禁用中断 */
#define portDISABLE_INTERRUPTS()    do {} while( 0 )

/* 使能中断 */
#define portENABLE_INTERRUPTS()     do {} while( 0 )

#if ( configNUMBER_OF_CORES == 1 )
/* 保存当前中断状态，然后禁用中断 */
    #define portENTER_CRITICAL()    do {} while( 0 )

/* 恢复先前保存的中断状态 */
    #define portEXIT_CRITICAL()     do {} while( 0 )
#else

/*
 * 移植层可以将临界区嵌套计数保存在 TCB 中，
 * 也可以在移植层中维护临界区嵌套计数。
 */
    #define portCRITICAL_NESTING_IN_TCB    1

/*
 * 如果系统中的核数大于 1，
 * portENTER/EXIT_CRITICAL 的实现应使用
 * vTaskEnterCritical 和 vTaskExitCritical。
 */
    #define portENTER_CRITICAL             vTaskEnterCritical
    #define portEXIT_CRITICAL              vTaskExitCritical

/*
 * 如果系统中的核数大于 1，
 * portENTER/EXIT_CRITICAL_FROM_ISR 的实现应使用
 * vTaskEnterCriticalFromISR 和 vTaskExitCriticalFromISR。
 */
    #define portENTER_CRITICAL_FROM_ISR    vTaskEnterCriticalFromISR
    #define portEXIT_CRITICAL_FROM_ISR     vTaskExitCriticalFromISR

#endif /* 条件块结束：if ( configNUMBER_OF_CORES == 1 ) */

extern void vPortYield( void );
#define portYIELD()                                           vPortYield()

/* 任务函数宏，如 FreeRTOS.org 网站所述。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters ) __attribute__( ( noreturn ) )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )

#if ( configNUMBER_OF_CORES > 1 )
    /* 返回代码运行所在的核 ID。 */
    #define portGET_CORE_ID()                0

/* 设置中断屏蔽。 */
    #define portSET_INTERRUPT_MASK()         0

/* 清除中断屏蔽。 */
    #define portCLEAR_INTERRUPT_MASK( x )    ( ( void ) ( x ) )

/* 请求核 ID 为 x 的核让出 CPU。 */
    #define portYIELD_CORE( x )              do {} while( 0 )

/*
 * 获取 TASK 锁。TASK 锁是递归锁，
 * 同一核应能多次获取该锁。
 */
    #define portGET_TASK_LOCK( xCoreID )     do {} while( 0 )

/*
 * 释放 TASK 锁。如果同一核多次获取了 TASK 锁，
 * 则应释放相同次数。
 */
    #define portRELEASE_TASK_LOCK( xCoreID ) do {} while( 0 )

/*
 * 获取 ISR 锁。ISR 锁是递归锁，
 * 同一核应能多次获取该锁。
 */
    #define portGET_ISR_LOCK( xCoreID )      do {} while( 0 )

/*
 * 释放 ISR 锁。如果同一核多次获取了 ISR 锁，
 * 则应释放相同次数。
 */
    #define portRELEASE_ISR_LOCK( xCoreID )  do {} while( 0 )

#endif /* 条件块结束：if ( configNUMBER_OF_CORES > 1 ) */

#endif /* 头文件保护 PORTMACRO_H */
