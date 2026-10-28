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

/* 标准头文件。 */
#include <limits.h>

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1 )
    /* 检查配置。 */
    #if ( configMAX_PRIORITIES > 32 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
    #endif
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION（使用移植层优化的任务选择） */

#if ( configISR_STACK_SIZE < ( configMINIMAL_STACK_SIZE * 2 ) )
    #warning configISR_STACK_SIZE is probably too small!
#endif /* ( configISR_STACK_SIZE < configMINIMAL_STACK_SIZE * 2 )（ISR 栈大小小于最小任务栈的两倍） */

#if ( ( configMAX_API_CALL_INTERRUPT_PRIORITY > portMAX_PRIORITY ) || ( configMAX_API_CALL_INTERRUPT_PRIORITY < 2 ) )
    #error configMAX_API_CALL_INTERRUPT_PRIORITY must be between 2 and 15
#endif

#if ( ( configSUPPORT_FPU == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 0 ) )
    #error configSUPPORT_DYNAMIC_ALLOCATION must be set to 1 to use this port with an FPU
#endif

/*
 * 临界区嵌套计数达到该值时，即退出临界区。
 */
#define portNO_CRITICAL_NESTING          ( ( uint32_t ) 0 )

/*
 * 任务创建时没有浮点上下文，但创建后可为其分配浮点上下文。
 * 任务上下文中会保存一个变量：若任务没有 FPU 上下文，该变量保存
 * portNO_FLOATING_POINT_CONTEXT；若任务有 FPU 上下文，则保存其他值。
 */
#define portNO_FLOATING_POINT_CONTEXT    ( ( StackType_t ) 0 )

/* 仅置位 IF 位，因此任务启动时中断处于使能状态。 */
#define portINITIAL_EFLAGS               ( 0x200UL )

/* 错误中断位于最高优先级向量。 */
#define portAPIC_LVT_ERROR_VECTOR        ( 0xfe )
#define portAPIC_SPURIOUS_INT_VECTOR     ( 0xff )

/* EFLAGS 标志位。 */
#define portEFLAGS_IF                    ( 0x200UL )

/* 使用 FSAVE 时的 FPU 上下文大小。 */
#define portFPU_CONTEXT_SIZE_BYTES       108

/*
 * IDT 中每个表项的预期大小。用于检查结构体打包设置是否正确。
 */
#define portEXPECTED_IDT_ENTRY_SIZE      8

/* IDT 表项的默认标志设置。 */
#define portIDT_FLAGS                    ( 0x8E )

/* 这是应用代码可用的最低 ISR 向量。 */
#define portAPIC_MIN_ALLOWABLE_VECTOR    ( 0x20 )

/*
 * 若定义了 configASSERT()，则用该值填充系统栈，
 * 以便进行粗略的栈溢出检查。
 */
#define portSTACK_WORD                   ( 0xecececec )
/*-----------------------------------------------------------*/

/*
 * 启动第一个任务开始执行。
 */
extern void vPortStartFirstTask( void );

/*
 * 用于捕获试图从其实现函数返回的任务。
 */
static void prvTaskExitError( void );

/*
 * 完整填写 IDT 中的一个描述符。
 */
static void prvSetInterruptGate( uint8_t ucNumber,
                                 ISR_Handler_t pxHandlerFunction,
                                 uint8_t ucFlags );

/*
 * 安装到每个 IDT 位置的默认处理程序。
 */
extern void vPortCentralInterruptWrapper( void );

/*
 * portYIELD() 的处理程序。
 */
extern void vPortYieldCall( void );

/*
 * 配置 APIC 以产生 RTOS tick。
 */
static void prvSetupTimerInterrupt( void );

/*
 * tick 中断处理程序。
 */
extern void vPortTimerHandler( void );

/*
 * 检查中断向量是否过高、过低、已被 FreeRTOS 占用，
 * 或已被应用占用。
 */
static BaseType_t prvCheckValidityOfVectorNumber( uint32_t ulVectorNumber );

/*-----------------------------------------------------------*/

/*
 * 用于记录临界区嵌套层次的变量。该变量必须初始化为非零值，
 * 以确保调度器启动前中断不会被意外解除屏蔽。
 * 在第一个任务开始执行之前将其置为零。
 */
volatile uint32_t ulCriticalNesting = 9999UL;

/*
 * 用于将 IDT 表项各字段映射到独立结构成员的结构体。
 */
struct IDTEntry
{
    uint16_t usISRLow;          /* 处理程序地址的低 16 位。 */
    uint16_t usSegmentSelector; /* 平坦模型意味着此项不变。 */
    uint8_t ucZero;             /* 必须设置为零。 */
    uint8_t ucFlags;            /* 该表项的标志。 */
    uint16_t usISRHigh;         /* 处理程序地址的高 16 位。 */
}
__attribute__( ( packed ) );
typedef struct IDTEntry IDTEntry_t;


/* 用于将 IDT 的位置传递给 CPU。 */
struct IDTPointer
{
    uint16_t usTableLimit;
    uint32_t ulTableBase; /* xInterruptDescriptorTable 中第一个表项的地址。 */
}
__attribute__( ( __packed__ ) );
typedef struct IDTPointer IDTPointer_t;

/* IDT 本身。 */
static __attribute__( ( aligned( 32 ) ) ) IDTEntry_t xInterruptDescriptorTable[ portNUM_VECTORS ];

#if ( configUSE_COMMON_INTERRUPT_ENTRY_POINT == 1 )

/*
 * 用于存储应用定义的中断处理程序表。若使用公共中断入口，
 * 则由中央中断处理程序调用这些处理程序。
 */
static ISR_Handler_t xInterruptHandlerTable[ portNUM_VECTORS ] = { NULL };

#endif /* configUSE_COMMON_INTERRUPT_ENTRY_POINT（使用公共中断入口点） */

#if ( configSUPPORT_FPU == 1 )

/*
 * 作为任务上下文的一部分保存。若 pucPortTaskFPUContextBuffer 为 NULL，
 * 则任务没有 FPU 上下文；若 pucPortTaskFPUContextBuffer 非 NULL，
 * 则它指向可用于保存 FPU 上下文的缓冲区。
 */
    uint8_t * pucPortTaskFPUContextBuffer __attribute__( ( used ) ) = pdFALSE;

#endif /* configSUPPORT_FPU（支持 FPU） */

/* 中断处理程序使用的栈。 */
static uint32_t ulSystemStack[ configISR_STACK_SIZE ] __attribute__( ( used ) ) = { 0 };

/*
 * 不要使用系统栈的最顶端，这样调试器尝试展开栈时
 * 返回地址会显示为 0。
 */
volatile uint32_t ulTopOfSystemStack __attribute__( ( used ) ) = ( uint32_t ) &( ulSystemStack[ configISR_STACK_SIZE - 5 ] );

/*
 * 若从中断或临界区请求让出 CPU，则不立即执行让出，
 * 而是将 ulPortYieldPending 置为 pdTRUE，表示应在中断结束、
 * 退出临界区时执行让出。
 */
volatile uint32_t ulPortYieldPending __attribute__( ( used ) ) = pdFALSE;

/*
 * 记录中断嵌套深度。用于判断何时切换到中断/系统栈，
 * 以及何时保存/恢复完整上下文。
 */
volatile uint32_t ulInterruptNesting __attribute__( ( used ) ) = 0;

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    uint32_t ulCodeSegment;

    /* 按 portFREERTOS_INTERRUPT_EXIT 宏的期望设置初始栈。 */

    *pxTopOfStack = 0x00;
    pxTopOfStack--;
    *pxTopOfStack = 0x00;
    pxTopOfStack--;

    /* 首先处理参数。 */
    *pxTopOfStack = ( StackType_t ) pvParameters;
    pxTopOfStack--;

    /*
     * 没有可返回之处，因此若试图使用返回地址则触发断言。
     */
    *pxTopOfStack = ( StackType_t ) prvTaskExitError;
    pxTopOfStack--;

    /* 用于启动任务的 iret 会弹到此位置。 */
    *pxTopOfStack = portINITIAL_EFLAGS;
    pxTopOfStack--;

    /* CS（代码段寄存器） */
    __asm volatile ( "movl %%cs, %0" : "=r" ( ulCodeSegment ) );
    *pxTopOfStack = ulCodeSegment;
    pxTopOfStack--;

    /* 任务中的第一条指令。 */
    *pxTopOfStack = ( StackType_t ) pxCode;
    pxTopOfStack--;

    /* POPA 指令所期望的通用寄存器。 */
    *pxTopOfStack = 0xEA;
    pxTopOfStack--;

    *pxTopOfStack = 0xEC;
    pxTopOfStack--;

    *pxTopOfStack = 0xED1; /* EDX（扩展数据寄存器） */
    pxTopOfStack--;

    *pxTopOfStack = 0xEB1; /* EBX（扩展基址寄存器） */
    pxTopOfStack--;

    /* ESP 占位。 */
    pxTopOfStack--;

    *pxTopOfStack = 0x00; /* EBP（扩展基址指针寄存器） */
    pxTopOfStack--;

    *pxTopOfStack = 0xE5; /* ESI（扩展源变址寄存器） */
    pxTopOfStack--;

    *pxTopOfStack = 0xeeeeeeee; /* EDI（扩展目的变址寄存器） */

    #if ( configSUPPORT_FPU == 1 )
    {
        pxTopOfStack--;

        /*
         * FPU 上下文缓冲区；由于任务创建时没有 FPU 上下文，该缓冲区初始化为 NULL。
         */
        *pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;
    }
    #endif /* configSUPPORT_FPU（支持 FPU） */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvSetInterruptGate( uint8_t ucNumber,
                                 ISR_Handler_t pxHandlerFunction,
                                 uint8_t ucFlags )
{
    uint16_t usCodeSegment;
    uint32_t ulBase = ( uint32_t ) pxHandlerFunction;

    xInterruptDescriptorTable[ ucNumber ].usISRLow = ( uint16_t ) ( ulBase & USHRT_MAX );
    xInterruptDescriptorTable[ ucNumber ].usISRHigh = ( uint16_t ) ( ( ulBase >> 16UL ) & USHRT_MAX );

    /* 使用平坦模型时，CS 永远不会改变。 */
    __asm volatile ( "mov %%cs, %0" : "=r" ( usCodeSegment ) );
    xInterruptDescriptorTable[ ucNumber ].usSegmentSelector = usCodeSegment;
    xInterruptDescriptorTable[ ucNumber ].ucZero = 0;
    xInterruptDescriptorTable[ ucNumber ].ucFlags = ucFlags;
}
/*-----------------------------------------------------------*/

void vPortSetupIDT( void )
{
    uint32_t ulNum;
    IDTPointer_t xIDT;

    #if ( configUSE_COMMON_INTERRUPT_ENTRY_POINT == 1 )
    {
        for( ulNum = 0; ulNum < portNUM_VECTORS; ulNum++ )
        {
            /* 该向量上尚未安装处理程序时。 */
            if( ( xInterruptDescriptorTable[ ulNum ].usISRLow == 0x00 ) && ( xInterruptDescriptorTable[ ulNum ].usISRHigh == 0x00 ) )
            {
                prvSetInterruptGate( ( uint8_t ) ulNum, vPortCentralInterruptWrapper, portIDT_FLAGS );
            }
        }
    }
    #endif /* configUSE_COMMON_INTERRUPT_ENTRY_POINT（使用公共中断入口点） */

    /* 设置 IDT 地址。 */
    xIDT.ulTableBase = ( uint32_t ) xInterruptDescriptorTable;
    xIDT.usTableLimit = sizeof( xInterruptDescriptorTable ) - 1;

    /* 在 CPU 中设置 IDT。 */
    __asm volatile ( "lidt %0" ::"m" ( xIDT ) );
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    /*
     * 实现任务的函数不得退出或试图返回调用者，因为没有可返回之处。
     * 任务若要退出，应改为调用 vTaskDelete( NULL )。
     *
     * 若定义了 configASSERT()，则人为触发 assert()，
     * 然后在此处停止，以便应用编写者捕获该错误。
     */
    configASSERT( ulCriticalNesting == ~0UL );
    portDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
    extern void vPortAPICErrorHandlerWrapper( void );
    extern void vPortAPICSpuriousHandler( void );

    /* 将 LAPIC 初始化为已知状态。 */
    portAPIC_LDR = 0xFFFFFFFF;
    portAPIC_LDR = ( ( portAPIC_LDR & 0x00FFFFFF ) | 0x00000001 );
    portAPIC_LVT_TIMER = portAPIC_DISABLE;
    portAPIC_LVT_PERF = portAPIC_NMI;
    portAPIC_LVT_LINT0 = portAPIC_DISABLE;
    portAPIC_LVT_LINT1 = portAPIC_DISABLE;
    portAPIC_TASK_PRIORITY = 0;

    /* 安装 APIC 定时器 ISR 向量。 */
    prvSetInterruptGate( ( uint8_t ) portAPIC_TIMER_INT_VECTOR, vPortTimerHandler, portIDT_FLAGS );

    /* 安装 API 错误处理程序。 */
    prvSetInterruptGate( ( uint8_t ) portAPIC_LVT_ERROR_VECTOR, vPortAPICErrorHandlerWrapper, portIDT_FLAGS );

    /* 安装让出 CPU 的处理程序。 */
    prvSetInterruptGate( ( uint8_t ) portAPIC_YIELD_INT_VECTOR, vPortYieldCall, portIDT_FLAGS );

    /* 安装伪中断向量。 */
    prvSetInterruptGate( ( uint8_t ) portAPIC_SPURIOUS_INT_VECTOR, vPortAPICSpuriousHandler, portIDT_FLAGS );

    /* 使能 APIC，同时映射伪中断。 */
    portAPIC_SPURIOUS_INT = portAPIC_SPURIOUS_INT_VECTOR | portAPIC_ENABLE_BIT;

    /* 设置定时器错误向量。 */
    portAPIC_LVT_ERROR = portAPIC_LVT_ERROR_VECTOR;

    /* 设置中断频率。 */
    portAPIC_TMRDIV = portAPIC_DIV_16;
    portAPIC_TIMER_INITIAL_COUNT = ( ( configCPU_CLOCK_HZ >> 4UL ) / configTICK_RATE_HZ ) - 1UL;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    BaseType_t xWord;

    /*
     * 某些版本的 GCC 需要 -mno-ms-bitfields 命令行选项才能正确进行结构体打包。
     */
    configASSERT( sizeof( struct IDTEntry ) == portEXPECTED_IDT_ENTRY_SIZE );

    /*
     * 用已知值填充部分系统栈，以帮助检测栈溢出。
     * 预留几个零，以免 GDB 在展开栈时产生混乱。
     */
    for( xWord = 0; xWord < configISR_STACK_SIZE - 20; xWord++ )
    {
        ulSystemStack[ xWord ] = portSTACK_WORD;
    }

    /* 初始化中断描述符表（IDT）。 */
    vPortSetupIDT();

    /* 初始化 LAPIC 并安装系统处理程序。 */
    prvSetupTimerInterrupt();

    /* 确保中断使用的栈已对齐。 */
    ulTopOfSystemStack &= ~portBYTE_ALIGNMENT_MASK;

    ulCriticalNesting = 0;

    /* 使能 LAPIC 计数器。 */
    portAPIC_LVT_TIMER = portAPIC_TIMER_PERIODIC | portAPIC_TIMER_INT_VECTOR;

    /* 有时需要。 */
    portAPIC_TMRDIV = portAPIC_DIV_16;

    /*
     * 不应从下面这个函数返回，因为此后调度器将开始执行任务。
     */
    vPortStartFirstTask();

    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在没有可返回之处的移植层中不实现。
     * 人为强制触发断言。
     */
    configASSERT( ulCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    if( ulCriticalNesting == 0 )
    {
        #if ( configMAX_API_CALL_INTERRUPT_PRIORITY == portMAX_PRIORITY )
        {
            __asm volatile ( "cli" );
        }
        #else
        {
            portAPIC_TASK_PRIORITY = portMAX_API_CALL_PRIORITY;
            configASSERT( portAPIC_TASK_PRIORITY == portMAX_API_CALL_PRIORITY );
        }
        #endif
    }

    /*
     * 现在中断已禁用，可以直接访问 ulCriticalNesting。
     * 递增 ulCriticalNesting，以记录 portENTER_CRITICAL() 被调用的次数。
     */
    ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    if( ulCriticalNesting > portNO_CRITICAL_NESTING )
    {
        /*
         * 退出临界区时递减嵌套计数。
         */
        ulCriticalNesting--;

        /*
         * 若嵌套层次已归零，则必须重新使能所有中断优先级。
         */
        if( ulCriticalNesting == portNO_CRITICAL_NESTING )
        {
            /*
             * 临界区嵌套计数已归零，因此应解除所有中断优先级的屏蔽。
             */
            #if ( configMAX_API_CALL_INTERRUPT_PRIORITY == portMAX_PRIORITY )
            {
                __asm volatile ( "sti" );
            }
            #else
            {
                portAPIC_TASK_PRIORITY = 0;
            }
            #endif

            /*
             * 若在临界区内挂起了让出请求，则现在执行让出。
             */
            if( ulPortYieldPending != pdFALSE )
            {
                ulPortYieldPending = pdFALSE;
                __asm volatile ( portYIELD_INTERRUPT );
            }
        }
    }
}
/*-----------------------------------------------------------*/

uint32_t ulPortSetInterruptMask( void )
{
    volatile uint32_t ulOriginalMask;

    /* 将屏蔽值设为系统调用最大优先级。 */
    #if ( configMAX_API_CALL_INTERRUPT_PRIORITY == portMAX_PRIORITY )
    {
        /*
         * 返回中断此前是否已使能。Pop 会先调整栈。
         */
        __asm volatile ( "pushf      \t\n"
                         "pop %0     \t\n"
                         "cli            "
                         : "=rm" ( ulOriginalMask )::"memory" );

        ulOriginalMask &= portEFLAGS_IF;
    }
    #else
    {
        /* 返回原始屏蔽值。 */
        ulOriginalMask = portAPIC_TASK_PRIORITY;
        portAPIC_TASK_PRIORITY = portMAX_API_CALL_PRIORITY;
        configASSERT( portAPIC_TASK_PRIORITY == portMAX_API_CALL_PRIORITY );
    }
    #endif /* if ( configMAX_API_CALL_INTERRUPT_PRIORITY == portMAX_PRIORITY )（系统调用中断最大优先级等于最高优先级） */

    return ulOriginalMask;
}
/*-----------------------------------------------------------*/

void vPortClearInterruptMask( uint32_t ulNewMaskValue )
{
    #if ( configMAX_API_CALL_INTERRUPT_PRIORITY == portMAX_PRIORITY )
    {
        if( ulNewMaskValue != pdFALSE )
        {
            __asm volatile ( "sti" );
        }
    }
    #else
    {
        portAPIC_TASK_PRIORITY = ulNewMaskValue;
        configASSERT( portAPIC_TASK_PRIORITY == ulNewMaskValue );
    }
    #endif /* if ( configMAX_API_CALL_INTERRUPT_PRIORITY == portMAX_PRIORITY )（系统调用中断最大优先级等于最高优先级） */
}
/*-----------------------------------------------------------*/

#if ( configSUPPORT_FPU == 1 )

    void vPortTaskUsesFPU( void )
    {
        /*
         * 任务正在登记其需要 FPU 上下文。分配一个缓冲区用于保存该上下文。
         */
        pucPortTaskFPUContextBuffer = ( uint8_t * ) pvPortMalloc( portFPU_CONTEXT_SIZE_BYTES );
        configASSERT( pucPortTaskFPUContextBuffer );

        /* 初始化浮点寄存器。 */
        __asm volatile ( "fninit" );
    }

#endif /* configSUPPORT_FPU（支持 FPU） */
/*-----------------------------------------------------------*/

void vPortAPICErrorHandler( void )
{
/* 保存 APIC 错误状态的变量，便于在调试器中查看。 */
    volatile uint32_t ulErrorStatus = 0;

    portAPIC_ERROR_STATUS = 0;
    ulErrorStatus = portAPIC_ERROR_STATUS;
    ( void ) ulErrorStatus;

    /* 强制触发断言。 */
    configASSERT( ulCriticalNesting == ~0UL );
}
/*-----------------------------------------------------------*/

#if ( configUSE_COMMON_INTERRUPT_ENTRY_POINT == 1 )

    void vPortCentralInterruptHandler( uint32_t ulVector )
    {
        if( ulVector < portNUM_VECTORS )
        {
            if( xInterruptHandlerTable[ ulVector ] != NULL )
            {
                ( xInterruptHandlerTable[ ulVector ] )();
            }
        }

        /* 检查系统栈是否溢出。 */
        configASSERT( ulSystemStack[ 10 ] == portSTACK_WORD );
        configASSERT( ulSystemStack[ 12 ] == portSTACK_WORD );
        configASSERT( ulSystemStack[ 14 ] == portSTACK_WORD );
    }

#endif /* configUSE_COMMON_INTERRUPT_ENTRY_POINT（使用公共中断入口点） */
/*-----------------------------------------------------------*/

#if ( configUSE_COMMON_INTERRUPT_ENTRY_POINT == 1 )

    BaseType_t xPortRegisterCInterruptHandler( ISR_Handler_t pxHandler,
                                               uint32_t ulVectorNumber )
    {
        BaseType_t xReturn;

        xReturn = prvCheckValidityOfVectorNumber( ulVectorNumber );

        if( xReturn != pdFAIL )
        {
            /*
             * 将应用传入的处理程序安装到传入的向量号中。
             * 随后由中央中断处理程序调用这些地址。
             */
            xInterruptHandlerTable[ ulVectorNumber ] = pxHandler;
        }

        return xReturn;
    }

#endif /* configUSE_COMMON_INTERRUPT_ENTRY_POINT（使用公共中断入口点） */
/*-----------------------------------------------------------*/

BaseType_t xPortInstallInterruptHandler( ISR_Handler_t pxHandler,
                                         uint32_t ulVectorNumber )
{
    BaseType_t xReturn;

    xReturn = prvCheckValidityOfVectorNumber( ulVectorNumber );

    if( xReturn != pdFAIL )
    {
        taskENTER_CRITICAL();
        {
            /* 更新 IDT，以包含应用定义的处理程序。 */
            prvSetInterruptGate( ( uint8_t ) ulVectorNumber, ( ISR_Handler_t ) pxHandler, portIDT_FLAGS );
        }
        taskEXIT_CRITICAL();
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvCheckValidityOfVectorNumber( uint32_t ulVectorNumber )
{
    BaseType_t xReturn;

    /* 检查向量号的有效性。 */
    if( ulVectorNumber >= portNUM_VECTORS )
    {
        /* 过高。 */
        xReturn = pdFAIL;
    }
    else if( ulVectorNumber < portAPIC_MIN_ALLOWABLE_VECTOR )
    {
        /* 过低。 */
        xReturn = pdFAIL;
    }
    else if( ulVectorNumber == portAPIC_TIMER_INT_VECTOR )
    {
        /* FreeRTOS 正在使用。 */
        xReturn = pdFAIL;
    }
    else if( ulVectorNumber == portAPIC_YIELD_INT_VECTOR )
    {
        /* FreeRTOS 正在使用。 */
        xReturn = pdFAIL;
    }
    else if( ulVectorNumber == portAPIC_LVT_ERROR_VECTOR )
    {
        /* FreeRTOS 正在使用。 */
        xReturn = pdFAIL;
    }
    else if( ulVectorNumber == portAPIC_SPURIOUS_INT_VECTOR )
    {
        /* FreeRTOS 正在使用。 */
        xReturn = pdFAIL;
    }
#if ( configUSE_COMMON_INTERRUPT_ENTRY_POINT == 1 )
    else if( xInterruptHandlerTable[ ulVectorNumber ] != NULL )
    {
        /* 已被应用占用。 */
        xReturn = pdFAIL;
    }
#endif  /* configUSE_COMMON_INTERRUPT_ENTRY_POINT（使用公共中断入口点） */
    else
    {
        xReturn = pdPASS;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

void vGenerateYieldInterrupt( void )
{
    __asm volatile ( portYIELD_INTERRUPT );
}
