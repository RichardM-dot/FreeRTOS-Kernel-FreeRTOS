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

/* BSP 头文件。 */
#include <mb_interface.h>
#include <xparameters.h>

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
#define portCHAR              char
#define portFLOAT             float
#define portDOUBLE            double
#define portLONG              long
#define portSHORT             short
#ifdef __arch64__
    #define portSTACK_TYPE    size_t
    typedef uint64_t        UBaseType_t;
#else
    #define portSTACK_TYPE    uint32_t
    typedef unsigned long   UBaseType_t;
#endif
#define portBASE_TYPE         long

typedef portSTACK_TYPE      StackType_t;
typedef long                BaseType_t;


#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t        TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t     TickType_t;
    #define portMAX_DELAY              ( TickType_t ) 0xffffffffUL

/*
 * 32 位架构上使用 32 位 tick 类型，因此读取 tick 计数
 * 无需用临界区保护。
 */
    #define portTICK_TYPE_IS_ATOMIC    1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif
/*-----------------------------------------------------------*/

/* 中断控制宏和函数。 */
void microblaze_disable_interrupts( void );
void microblaze_enable_interrupts( void );
#define portDISABLE_INTERRUPTS()    microblaze_disable_interrupts()
#define portENABLE_INTERRUPTS()     microblaze_enable_interrupts()
/*-----------------------------------------------------------*/

/* 临界区宏。 */
void vPortEnterCritical( void );
void vPortExitCritical( void );
#define portENTER_CRITICAL()                           \
    {                                                  \
        extern volatile UBaseType_t uxCriticalNesting; \
        microblaze_disable_interrupts();               \
        uxCriticalNesting++;                           \
    }

#define portEXIT_CRITICAL()                            \
    {                                                  \
        extern volatile UBaseType_t uxCriticalNesting; \
        /* Interrupts are disabled, so we can */       \
        /* access the variable directly. */            \
        uxCriticalNesting--;                           \
        if( uxCriticalNesting == 0 )                   \
        {                                              \
            /* The nesting has unwound and we \
             * can enable interrupts again. */ \
            portENABLE_INTERRUPTS();           \
        }                                      \
    }

/*-----------------------------------------------------------*/

/* 让出宏直接映射到 vPortYield() 函数。 */
void vPortYield( void );
#define portYIELD()    vPortYield()

/*
 * portYIELD_FROM_ISR() 不直接调用 vTaskSwitchContext()，
 * 而是设置一个标志以表示已请求让出。中断退出代码随后检查该标志，
 * 若标志不为 false，则在恢复任务上下文之前调用 vTaskSwitchContext()。
 * 这样可避免从一次中断中多次调用 vTaskSwitchContext()，
 * 因为一次中断可能引发多个外设的服务。
 */
extern volatile uint32_t ulTaskSwitchRequested;
#define portYIELD_FROM_ISR( x )                              \
    do { if( ( x ) != pdFALSE ) ulTaskSwitchRequested = 1; } \
    while( 0 )

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1 )

/* 通用辅助函数。 */
    __attribute__( ( always_inline ) ) static inline uint8_t ucPortCountLeadingZeros( uint32_t ulBitmap )
    {
        uint8_t ucReturn;

        __asm volatile ( "clz %0, %1" : "=r" ( ucReturn ) : "r" ( ulBitmap ) );

        return ucReturn;
    }

/* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
    #endif

/* 用位图保存/清除就绪优先级。 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )    ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )     ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

/*-----------------------------------------------------------*/

    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )    uxTopPriority = ( 31UL - ( uint32_t ) ucPortCountLeadingZeros( ( uxReadyPriorities ) ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION（使用移植层优化的任务选择） */

/*-----------------------------------------------------------*/

/* 硬件相关细节。 */
#ifdef __arch64__
    #define portBYTE_ALIGNMENT    8
#else
    #define portBYTE_ALIGNMENT    4
#endif
#define portSTACK_GROWTH          ( -1 )
#define portTICK_PERIOD_MS        ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portNOP()    asm volatile ( "NOP" )
#define portMEMORY_BARRIER()    asm volatile ( "" ::: "memory" )
/*-----------------------------------------------------------*/

#if ( XPAR_MICROBLAZE_USE_STACK_PROTECTION )
    #define portHAS_STACK_OVERFLOW_CHECKING    1
#endif
/*-----------------------------------------------------------*/

/* FreeRTOS.org 网站上所述的任务函数宏。 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void * pvParameters )
/*-----------------------------------------------------------*/

/*
 * 以下结构体供 FreeRTOS 异常处理程序使用。它填充异常发生时的 MicroBlaze 上下文，
 * 以便于调试异常。
 */
typedef struct PORT_REGISTER_DUMP
{
    /*
     * 以下结构成员保存异常触发时 MicroBlaze 各寄存器的值。
     */
    UINTPTR ulR1_SP;
    UINTPTR ulR2_small_data_area;
    UINTPTR ulR3;
    UINTPTR ulR4;
    UINTPTR ulR5;
    UINTPTR ulR6;
    UINTPTR ulR7;
    UINTPTR ulR8;
    UINTPTR ulR9;
    UINTPTR ulR10;
    UINTPTR ulR11;
    UINTPTR ulR12;
    UINTPTR ulR13_read_write_small_data_area;
    UINTPTR ulR14_return_address_from_interrupt;
    UINTPTR ulR15_return_address_from_subroutine;
    UINTPTR ulR16_return_address_from_trap;
    UINTPTR ulR17_return_address_from_exceptions; /* 若异常发生在分支指令的延迟槽中，异常入口代码会将 BTR 复制到 R17。 */
    UINTPTR ulR18;
    UINTPTR ulR19;
    UINTPTR ulR20;
    UINTPTR ulR21;
    UINTPTR ulR22;
    UINTPTR ulR23;
    UINTPTR ulR24;
    UINTPTR ulR25;
    UINTPTR ulR26;
    UINTPTR ulR27;
    UINTPTR ulR28;
    UINTPTR ulR29;
    UINTPTR ulR30;
    UINTPTR ulR31;
    UINTPTR ulPC;
    UINTPTR ulESR;
    UINTPTR ulMSR;
    UINTPTR ulEAR;
    UINTPTR ulFSR;
    UINTPTR ulEDR;

    /*
     * 以人类可读的形式描述异常原因。所用字符串与
     * microblaze_exceptions_i.h 头文件中的 #define 常量名一致。
     */
    int8_t * pcExceptionCause;

    /*
     * 异常发生时正在运行的任务的人类可读名称。
     * 该名称是使用 FreeRTOS xTaskCreate() API 函数创建任务时赋予任务的名称。
     */
    char * pcCurrentTaskName;

    /*
     * 异常发生时正在运行的任务的句柄。
     */
    void * xCurrentTaskHandle;
} xPortRegisterDump;


/*
 * 将 pxHandler 安装为由 ucInterruptID 参数指定的外设的中断处理程序。
 *
 * ucInterruptID：
 *
 * 将 pxHandler 指定为其中断处理程序的外设 ID。外设 ID 定义在
 * xparameters.h 头文件中，该文件本身属于 BSP 工程。例如，
 * 在本移植层的官方演示应用中，xparameters.h 为四个可能的中断源
 * 定义了以下 ID：
 *
 * XPAR_INTC_0_UARTLITE_1_VEC_ID  -  UARTlite 外设。
 * XPAR_INTC_0_TMRCTR_0_VEC_ID    -  AXI Timer 0 外设。
 * XPAR_INTC_0_EMACLITE_0_VEC_ID  -  Ethernet lite 外设。
 * XPAR_INTC_0_GPIO_1_VEC_ID      -  按键输入。
 *
 *
 * pxHandler：
 *
 * 指向中断处理函数本身的指针。该函数必须是接受 (void *) 参数的 void 函数。
 *
 *
 * pvCallBackRef：
 *
 * 传入处理函数的参数。多数情况下不使用，可以为 NULL。
 * 有时用于传入对外设实例变量的引用，以便在处理函数内部访问该外设。
 *
 *
 * 函数成功执行时返回 pdPASS。返回任何其他值都表示函数未正确执行。
 */
BaseType_t xPortInstallInterruptHandler( uint8_t ucInterruptID,
                                         XInterruptHandler pxHandler,
                                         void * pvCallBackRef );


/*
 * 在中断控制器中使能由 ucInterruptID 参数指定的外设中断。
 *
 * ucInterruptID：
 *
 * 要在中断控制器中使能其中断的外设 ID。外设 ID 定义在
 * xparameters.h 头文件中，该文件本身属于 BSP 工程。例如，
 * 在本移植层的官方演示应用中，xparameters.h 为四个可能的中断源
 * 定义了以下 ID：
 *
 * XPAR_INTC_0_UARTLITE_1_VEC_ID  -  UARTlite 外设。
 * XPAR_INTC_0_TMRCTR_0_VEC_ID    -  AXI Timer 0 外设。
 * XPAR_INTC_0_EMACLITE_0_VEC_ID  -  Ethernet lite 外设。
 * XPAR_INTC_0_GPIO_1_VEC_ID      -  按键输入。
 */
void vPortEnableInterrupt( uint8_t ucInterruptID );

/*
 * 在中断控制器中禁用由 ucInterruptID 参数指定的外设中断。
 *
 * ucInterruptID：
 *
 * 要在中断控制器中禁用其中断的外设 ID。外设 ID 定义在
 * xparameters.h 头文件中，该文件本身属于 BSP 工程。例如，
 * 在本移植层的官方演示应用中，xparameters.h 为四个可能的中断源
 * 定义了以下 ID：
 *
 * XPAR_INTC_0_UARTLITE_1_VEC_ID  -  UARTlite 外设。
 * XPAR_INTC_0_TMRCTR_0_VEC_ID    -  AXI Timer 0 外设。
 * XPAR_INTC_0_EMACLITE_0_VEC_ID  -  Ethernet lite 外设。
 * XPAR_INTC_0_GPIO_1_VEC_ID      -  按键输入。
 */
void vPortDisableInterrupt( uint8_t ucInterruptID );

/*
 * 这是应用定义的回调函数，用于安装 tick 中断处理程序。
 * 之所以作为应用回调提供，是因为内核要运行在多种 MicroBlaze 和 FPGA 配置上——
 * 并非所有配置都定义或提供相同的定时器外设。本示例使用 AXI Timer 0。
 * 若硬件平台提供该定时器，则本示例回调实现无需修改。
 * 应安装的中断处理程序名称为 vPortTickISR()，下面的函数将其声明为 extern。
 */
void vApplicationSetupTimerInterrupt( void );

/*
 * 这是应用定义的回调函数，用于清除由 vApplicationSetupTimerInterrupt()
 * 回调函数安装的中断——此处即 AXI 定时器产生的中断。
 * 之所以作为应用回调提供，是因为内核要运行在多种 MicroBlaze 和 FPGA 配置上——
 * 并非所有配置都定义或提供相同的定时器外设。本示例使用 AXI Timer 0。
 * 若硬件平台提供该定时器，且 vApplicationSetupTimerInterrupt() 的示例定义
 * 也未修改，则本示例回调实现无需修改。
 */
void vApplicationClearTimerInterrupt( void );

/*
 * 仅当 MicroBlaze 配置为包含异常功能，且 FreeRTOSConfig.h 中
 * configINSTALL_EXCEPTION_HANDLERS 设置为 1 时，
 * vPortExceptionsInstallHandlers() 才可用。
 *
 * vPortExceptionsInstallHandlers() 为所有可能的异常原因安装 FreeRTOS 异常处理程序。
 *
 * vPortExceptionsInstallHandlers() 可由应用代码显式调用。完成之后，
 * 对于任何特定异常原因，都可以使用标准 Xilinx 库函数
 * microblaze_register_exception_handler() 替换已安装的默认 FreeRTOS 异常处理程序。
 *
 * 若应用未显式调用 vPortExceptionsInstallHandlers()，则内核在首次调用
 * xPortInstallInterruptHandler() 时会自动调用它。届时，
 * 任何可能已安装的异常处理程序都将被替换。
 *
 * 有关 FreeRTOS 异常处理程序所执行处理的说明，请参见
 * vApplicationExceptionRegisterDump() 的描述。
 */
void vPortExceptionsInstallHandlers( void );

/*
 * FreeRTOS 异常处理程序用异常发生时的 MicroBlaze 上下文填充
 * xPortRegisterDump 结构体（定义于 portmacro.h）。随后异常处理程序调用
 * vApplicationExceptionRegisterDump()，并将填写完毕的
 * xPortRegisterDump 结构体作为参数传入。
 *
 * FreeRTOS 内核提供了自己的 vApplicationExceptionRegisterDump() 实现，
 * 但内核提供的实现声明为 'weak'（弱符号）。弱定义允许应用编写者
 * 在需要使用寄存器转储信息时提供自己的实现。例如，可以提供一种实现，
 * 将寄存器转储数据写入显示器或 UART 端口。
 */
void vApplicationExceptionRegisterDump( xPortRegisterDump * xRegisterDump );


/* *INDENT-OFF*（关闭自动缩进，保持标记原样） */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON*（恢复自动缩进，保持标记原样） */

#endif /* PORTMACRO_H（移植宏头文件） */