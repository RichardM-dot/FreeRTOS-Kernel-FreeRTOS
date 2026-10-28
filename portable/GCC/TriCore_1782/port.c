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

/* 标准头文件包含。 */
#include <stdlib.h>
#include <string.h>

/* TriCore 专用头文件包含。 */
#include <tc1782.h>
#include <machine/intrinsics.h>
#include <machine/cint.h>
#include <machine/wdtcon.h>

/* 内核头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"
#include "list.h"

#if configCHECK_FOR_STACK_OVERFLOW > 0
    #error "Stack checking cannot be used with this port, as, unlike most ports, the pxTopOfStack member of the TCB is consumed CSA.  CSA starvation, loosely equivalent to stack overflow, will result in a trap exception."
    /* 可使用 portCSA_TO_ADDRESS( portCSA_TO_ADDRESS( pxCurrentTCB->pxTopOfStack )[ 0 ] )[ 2 ]; 访问栈指针。 */
#endif /* configCHECK_FOR_STACK_OVERFLOW（宏） */


/*-----------------------------------------------------------*/

/* 系统寄存器定义。 */
#define portSYSTEM_PROGRAM_STATUS_WORD                  ( 0x000008FFUL ) /* 超级用户模式，MPU 寄存器组 0，调用深度计数禁用。 */
#define portINITIAL_PRIVILEGED_PROGRAM_STATUS_WORD      ( 0x000014FFUL ) /* IO 级别 1，MPU 寄存器组 1，调用深度计数禁用。 */
#define portINITIAL_UNPRIVILEGED_PROGRAM_STATUS_WORD    ( 0x000010FFUL ) /* IO 级别 0，MPU 寄存器组 1，调用深度计数禁用。 */
#define portINITIAL_PCXI_UPPER_CONTEXT_WORD             ( 0x00C00000UL ) /* 低 20 位标识 CSA 地址。 */
#define portINITIAL_SYSCON                              ( 0x00000000UL ) /* 禁用 MPU。 */

/* CSA 操作宏。 */
#define portCSA_FCX_MASK                                ( 0x000FFFFFUL )

/* OS 中断与陷阱机制。 */
#define portRESTORE_PSW_MASK                            ( ~( 0x000000FFUL ) )
#define portSYSCALL_TRAP                                ( 6 )

/* 每个 CSA 包含 16 个字的数据。 */
#define portNUM_WORDS_IN_CSA                            ( 16 )

/* PCP_SRC 寄存器中的中断使能位。 */
#define portENABLE_CPU_INTERRUPT                        ( 1U << 12U )
/*-----------------------------------------------------------*/

/*
 * 执行产生时钟节拍（tick）中断所需的硬件配置。
 */
static void prvSystemTickHandler( int ) __attribute__( ( longcall ) );
static void prvSetupTimerInterrupt( void );

/*
 * 用于让出（yield）的陷阱处理程序。
 */
static void prvTrapYield( int iTrapIdentification );

/*
 * 用于处理从中断中挂起的让出（yield）的优先级 1 中断处理程序。
 */
static void prvInterruptYield( int iTrapIdentification );

/*-----------------------------------------------------------*/

/* 保存/恢复上下文宏需要此引用。 */
extern volatile uint32_t * pxCurrentTCB;

/* 在编译时预先计算比较匹配值。 */
static const uint32_t ulCompareMatchValue = ( configPERIPHERAL_CLOCK_HZ / configTICK_RATE_HZ );

/*-----------------------------------------------------------*/

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    uint32_t * pulUpperCSA = NULL;
    uint32_t * pulLowerCSA = NULL;

    /*
     * 16 个地址寄存器（其中 4 个为全局地址寄存器）、16 个数据寄存器和 3 个系统寄存器。
     *
     * 有 3 个寄存器用于跟踪 CSA。
     *  FCX 指向全局空闲 CSA 集合的头部。
     *  任务的 PCX 需要指向 Lower->Upper->NULL 的排列。
     *  LCX 指向最后一个空闲 CSA，以便采取纠正措施。
     *
     * 需要两个 CSA 来保存任务上下文。
     *  上部上下文包含 D8-D15、A10-A15、PSW 和 PCXI->NULL。
     *  下部上下文包含 D0-D7、A2-A7、A11 和 PCXI->UpperContext。
     *  pxCurrentTCB->pxTopOfStack 指向与初始 BISR 匹配的下部上下文 RSLCX。
     *  下部上下文指向上部上下文，已为从中断处理程序返回做好准备。
     *
     * 任务的真实栈指针存储在 A10 中，随上部上下文一起恢复。
     */

    /*
     * 此处必须禁止中断，因为将要操作 CSA。
     */
    portENTER_CRITICAL();
    {
        /* 执行 DSync，以确保缓冲不会造成问题。 */
        _dsync();

        /* 消耗两个空闲 CSA。 */
        pulLowerCSA = portCSA_TO_ADDRESS( __MFCR( $FCX ) );

        if( NULL != pulLowerCSA )
        {
            /* 下部上下文链接到上部上下文。 */
            pulUpperCSA = portCSA_TO_ADDRESS( pulLowerCSA[ 0 ] );
        }

        /* 检查是否成功预留了两个 CSA。 */
        if( ( NULL != pulLowerCSA ) && ( NULL != pulUpperCSA ) )
        {
            /* 从空闲 CSA 链表中移除这两个已消耗的 CSA。 */
            _disable();
            _dsync();
            _mtcr( $FCX, pulUpperCSA[ 0 ] );
            _isync();
            _enable();
        }
        else
        {
            /* 直接触发上下文链表耗尽陷阱。 */
            _svlcx();
        }
    }
    portEXIT_CRITICAL();

    /* 清除上部 CSA。 */
    memset( pulUpperCSA, 0, portNUM_WORDS_IN_CSA * sizeof( uint32_t ) );

    /* 上部上下文。 */
    pulUpperCSA[ 2 ] = ( uint32_t ) pxTopOfStack;      /* A10；栈返回地址，即栈指针 */
    pulUpperCSA[ 1 ] = portSYSTEM_PROGRAM_STATUS_WORD; /* PSW 寄存器 */

    /* 清除下部 CSA。 */
    memset( pulLowerCSA, 0, portNUM_WORDS_IN_CSA * sizeof( uint32_t ) );

    /* 下部上下文。 */
    pulLowerCSA[ 8 ] = ( uint32_t ) pvParameters; /* A4；地址类型参数寄存器 */
    pulLowerCSA[ 1 ] = ( uint32_t ) pxCode;       /* A11；返回地址，即 RA */

    /* PCXI 指向上部上下文。 */
    pulLowerCSA[ 0 ] = ( portINITIAL_PCXI_UPPER_CONTEXT_WORD | ( uint32_t ) portADDRESS_TO_CSA( pulUpperCSA ) );

    /* 将 CSA 的链接保存到栈顶。 */
    pxTopOfStack = ( uint32_t * ) portADDRESS_TO_CSA( pulLowerCSA );

    /* 执行 DSync，以确保缓冲不会造成问题。 */
    _dsync();

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

int32_t xPortStartScheduler( void )
{
    extern void vTrapInstallHandlers( void );
    uint32_t ulMFCR = 0UL;
    uint32_t * pulUpperCSA = NULL;
    uint32_t * pulLowerCSA = NULL;

    /*
     * 调用此函数时，优先级等于或低于 configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断已被禁止。
     */

    /* 设置定时器中断。 */
    prvSetupTimerInterrupt();

    /* 安装陷阱处理程序。 */
    vTrapInstallHandlers();

    /* 安装用于 yield 调用的系统调用处理程序。 */
    if( 0 == _install_trap_handler( portSYSCALL_TRAP, prvTrapYield ) )
    {
        /* 安装 yield 处理程序失败，强制触发断言。 */
        configASSERT( ( ( volatile void * ) NULL ) );
    }

    /*
     * 使能并安装用于从 ISR 挂起上下文切换的优先级 1 中断。参见 TriCore 手册中的 mod_SRC。
     */
    CPU_SRC0.reg = ( portENABLE_CPU_INTERRUPT ) | ( configKERNEL_YIELD_PRIORITY );

    if( 0 == _install_int_handler( configKERNEL_YIELD_PRIORITY, prvInterruptYield, 0 ) )
    {
        /* 安装 yield 处理程序失败，强制触发断言。 */
        configASSERT( ( ( volatile void * ) NULL ) );
    }

    _disable();

    /* 加载初始 SYSCON。 */
    _mtcr( $SYSCON, portINITIAL_SYSCON );
    _isync();

    /* ENDINIT 已在 'cstart.c' 代码中应用。 */

    /*
     * 清除 PSW.CDC，以便使用 RFE 而不会因这段代码并非真正处于异常中而产生异常。
     */
    ulMFCR = __MFCR( $PSW );
    ulMFCR &= portRESTORE_PSW_MASK;
    _dsync();
    _mtcr( $PSW, ulMFCR );
    _isync();

    /* 最后，执行等效于 portRESTORE_CONTEXT() 的操作 */
    pulLowerCSA = portCSA_TO_ADDRESS( ( *pxCurrentTCB ) );
    pulUpperCSA = portCSA_TO_ADDRESS( pulLowerCSA[ 0 ] );
    _dsync();
    _mtcr( $PCXI, *pxCurrentTCB );
    _isync();
    _nop();
    _rslcx();
    _nop();

    /* 返回到选定的第一个要执行的任务。 */
    __asm volatile ( "rfe" );

    /* 不会执行到此处。 */
    return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
    /* 设置时钟分频器。 */
    unlock_wdtcon();
    {
        /* 等待对 Endinit 保护寄存器的访问被使能。 */
        while( 0 != ( WDT_CON0.reg & 0x1UL ) )
        {
        }

        /* RMC == 1，因此 STM 时钟 == FPI */
        STM_CLC.reg = ( 1UL << 8 );
    }
    lock_wdtcon();

    /* 确定使用了多少位，同时不改变 CMCON 寄存器中的其他位。 */
    STM_CMCON.reg &= ~( 0x1fUL );
    STM_CMCON.reg |= ( 0x1fUL - __CLZ( configPERIPHERAL_CLOCK_HZ / configTICK_RATE_HZ ) );

    /* 考虑当前时间，以免立即产生一个 tick。 */
    STM_CMP0.reg = ulCompareMatchValue + STM_TIM0.reg;

    if( 0 != _install_int_handler( configKERNEL_INTERRUPT_PRIORITY, prvSystemTickHandler, 0 ) )
    {
        /* 设置中断。 */
        STM_SRC0.reg = ( configKERNEL_INTERRUPT_PRIORITY | 0x00005000UL );

        /* 使能中断。 */
        STM_ISRR.reg &= ~( 0x03UL );
        STM_ISRR.reg |= 0x1UL;
        STM_ISRR.reg &= ~( 0x07UL );
        STM_ICR.reg |= 0x1UL;
    }
    else
    {
        /* 安装 Tick 中断失败。 */
        configASSERT( ( ( volatile void * ) NULL ) );
    }
}
/*-----------------------------------------------------------*/

static void prvSystemTickHandler( int iArg )
{
    uint32_t ulSavedInterruptMask;
    uint32_t * pxUpperCSA = NULL;
    uint32_t xUpperCSA = 0UL;
    extern volatile uint32_t * pxCurrentTCB;
    int32_t lYieldRequired;

    /* 仅为避免编译器关于未使用参数的警告。 */
    ( void ) iArg;

    /* 清除中断源。 */
    STM_ISRR.reg = 1UL;

    /*
     * 为未来 X 个 tick 重新加载比较匹配寄存器。
     *
     * 若临界区或中断嵌套预算被超出，则计算出的下一个比较匹配值可能已处于过去。
     * 若发生这种情况（不太可能），由此造成的时间滑移可能超过一个 tick 周期。
     * 其任何不利影响在时间上都是有限的，因为 56 位 STM 定时器中只有前 n 位用于
     * 比较匹配，因此仅在这 n 位（而非整个 56 位）溢出后就会发生另一次比较匹配。
     * 例如，若外设时钟为 75MHz、tick 速率为 1KHz，错过一个 tick 可能导致下一个
     * tick 中断在期望周期的 1.7 倍时间内发生。大于一个 tick 周期这一事实，是使用
     * 无法在硬件上由 tick 中断自动复位的定时器所造成的影响。将 tick 源改为比较
     * 匹配时自动复位的定时器（如 GPTA 定时器）可将最大可能附加周期恰好降为期望
     * 周期的 1 倍。
     */
    STM_CMP0.reg += ulCompareMatchValue;

    /* 内核 API 调用需要临界区。 */
    ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
    {
        /* 递增 Tick。 */
        lYieldRequired = xTaskIncrementTick();
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulSavedInterruptMask );

    if( lYieldRequired != pdFALSE )
    {
        /*
         * 保存任务上下文。
         * 进入陷阱或中断时，上部上下文会自动保存。还需要保存下部上下文，并将 PCXI CSA ID
         * 复制到 pxCurrentTCB->pxTopOfStack。只能将下部上下文的 CSA ID 保存到任务的 TCB 中。
         *
         * 调用 vTaskSwitchContext 选择下一个任务，注意这会改变 pxCurrentTCB 的值，因此需要
         * 重新加载。
         *
         * 调用 vPortSetMPURegisterSetOne 更改刚刚切换进来的任务的 MPU 映射。
         *
         * 加载任务上下文。
         * 需要将 CSA 从 pxCurrentTCB->pxTopOfStack 加载到 PCXI（实际上是更改调用栈），以恢复
         * 下部上下文。在中断处理程序的后缀中，RSLCX 将恢复任务的下部上下文。RFE 将恢复
         * 任务的上部上下文、跳转到返回地址，并恢复之前的中断使能/禁止状态。
         */
        _disable();
        _dsync();
        xUpperCSA = __MFCR( $PCXI );
        pxUpperCSA = portCSA_TO_ADDRESS( xUpperCSA );
        *pxCurrentTCB = pxUpperCSA[ 0 ];
        vTaskSwitchContext();
        pxUpperCSA[ 0 ] = *pxCurrentTCB;
        CPU_SRC0.bits.SETR = 0;
        _isync();
    }
}
/*-----------------------------------------------------------*/

/*
 * 任务被删除时，会永久让出，直到空闲任务有机会回收该任务曾使用的内存。通常任务
 * 使用的内存是 TCB 和栈，但在 TriCore 中还包括作为调用栈一部分消耗的 CSA。这些 CSA
 * 只有在不再属于当前调用栈时才能归还到全局空闲池，因此将回收推迟到空闲任务释放
 * 任务其他资源时进行。此函数使用 CSA 链表头（来自任务最后一次让出时）并找到尾部
 * （调用栈的最底部），将此链表插入空闲链表头部，并将现有空闲链表附加到回收调用栈
 * 的尾部。
 *
 * 注意：空闲任务需要处理时间来完成此函数，在负载很重的系统中，假设任务被频繁
 * 创建和删除，空闲 CSA 的消耗速度可能快于释放速度。
 */
void vPortReclaimCSA( uint32_t * pxTCB )
{
    uint32_t pxHeadCSA, pxTailCSA, pxFreeCSA;
    uint32_t * pulNextCSA;

    /*
     * 指向任务所消耗 CSA 链表中第一个 CSA 的指针存储在任务 TCB 结构的第一个元素中
     * （在传统的基于栈的体系结构中该位置存放栈指针）。
     */
    pxHeadCSA = ( *pxTCB ) & portCSA_FCX_MASK;

    /*
     * 屏蔽 CSA 链接字段中除地址外的所有内容。若地址为 NULL，则 CSA 不链接到任何位置，
     * 无需执行任何操作。
     */
    pxTailCSA = pxHeadCSA;

    /*
     * 将链接值转换为仅包含原始地址，并存入局部变量。
     */
    pulNextCSA = portCSA_TO_ADDRESS( pxTailCSA );

    /*
     * 遍历作为任务一部分消耗的 CSA。CSA 的第一个字段是指向下一个 CSA 的指针。屏蔽
     * 指向下一个 CSA 的指针中除链接地址外的所有内容。若为 NULL，则当前指向的 CSA
     * 是链中的最后一个。
     */
    while( 0UL != ( pulNextCSA[ 0 ] & portCSA_FCX_MASK ) )
    {
        /*
         * 清除指向链中下一个 CSA 的指针中除地址位本身之外的所有位。
         */
        pulNextCSA[ 0 ] = pulNextCSA[ 0 ] & portCSA_FCX_MASK;

        /* 移动指针，使其指向链表中的下一个 CSA。 */
        pxTailCSA = pulNextCSA[ 0 ];

        /* 将局部指针更新为该 CSA。 */
        pulNextCSA = portCSA_TO_ADDRESS( pxTailCSA );
    }

    _disable();
    {
        /* 查找当前空闲 CSA 头。 */
        _dsync();
        pxFreeCSA = __MFCR( $FCX );

        /* 将当前空闲链表接到正在回收链表的尾部。 */
        portCSA_TO_ADDRESS( pxTailCSA )[ 0 ] = pxFreeCSA;

        /* 将回收链表的头移入空闲链表。 */
        _dsync();
        _mtcr( $FCX, pxHeadCSA );
        _isync();
    }
    _enable();
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* 无需执行任何操作。不太可能希望结束。 */
}
/*-----------------------------------------------------------*/

static void prvTrapYield( int iTrapIdentification )
{
    uint32_t * pxUpperCSA = NULL;
    uint32_t xUpperCSA = 0UL;
    extern volatile uint32_t * pxCurrentTCB;

    switch( iTrapIdentification )
    {
        case portSYSCALL_TASK_YIELD:

            /*
             * 保存任务上下文。
             * 进入陷阱或中断时，上部上下文会自动保存。还需要保存下部上下文，并将 PCXI CSA ID
             * 复制到 pxCurrentTCB->pxTopOfStack。只能将下部上下文的 CSA ID 保存到任务的 TCB 中。
             *
             * 调用 vTaskSwitchContext 选择下一个任务，注意这会改变 pxCurrentTCB 的值，因此需要
             * 重新加载。
             *
             * 调用 vPortSetMPURegisterSetOne 更改刚刚切换进来的任务的 MPU 映射。
             *
             * 加载任务上下文。
             * 需要将 CSA 从 pxCurrentTCB->pxTopOfStack 加载到 PCXI（实际上是更改调用栈），以恢复
             * 下部上下文。在中断处理程序的后缀中，RSLCX 将恢复任务的下部上下文。RFE 将恢复
             * 任务的上部上下文、跳转到返回地址，并恢复之前的中断使能/禁止状态。
             */
            _disable();
            _dsync();
            xUpperCSA = __MFCR( $PCXI );
            pxUpperCSA = portCSA_TO_ADDRESS( xUpperCSA );
            *pxCurrentTCB = pxUpperCSA[ 0 ];
            vTaskSwitchContext();
            pxUpperCSA[ 0 ] = *pxCurrentTCB;
            CPU_SRC0.bits.SETR = 0;
            _isync();
            break;

        default:
            /* 调用了未实现的陷阱。 */
            configASSERT( ( ( volatile void * ) NULL ) );
            break;
    }
}
/*-----------------------------------------------------------*/

static void prvInterruptYield( int iId )
{
    uint32_t * pxUpperCSA = NULL;
    uint32_t xUpperCSA = 0UL;
    extern volatile uint32_t * pxCurrentTCB;

    /* 仅为消除编译器警告。 */
    ( void ) iId;

    /*
     * 保存任务上下文。
     * 进入陷阱或中断时，上部上下文会自动保存。还需要保存下部上下文，并将 PCXI CSA ID
     * 复制到 pxCurrentTCB->pxTopOfStack。只能将下部上下文的 CSA ID 保存到任务的 TCB 中。
     *
     * 调用 vTaskSwitchContext 选择下一个任务，注意这会改变 pxCurrentTCB 的值，因此需要
     * 重新加载。
     *
     * 调用 vPortSetMPURegisterSetOne 更改刚刚切换进来的任务的 MPU 映射。
     *
     * 加载任务上下文。
     * 需要将 CSA 从 pxCurrentTCB->pxTopOfStack 加载到 PCXI（实际上是更改调用栈），以恢复
     * 下部上下文。在中断处理程序的后缀中，RSLCX 将恢复任务的下部上下文。RFE 将恢复
     * 任务的上部上下文、跳转到返回地址，并恢复之前的中断使能/禁止状态。
     */
    _disable();
    _dsync();
    xUpperCSA = __MFCR( $PCXI );
    pxUpperCSA = portCSA_TO_ADDRESS( xUpperCSA );
    *pxCurrentTCB = pxUpperCSA[ 0 ];
    vTaskSwitchContext();
    pxUpperCSA[ 0 ] = *pxCurrentTCB;
    CPU_SRC0.bits.SETR = 0;
    _isync();
}
/*-----------------------------------------------------------*/

uint32_t uxPortSetInterruptMaskFromISR( void )
{
    uint32_t uxReturn = 0UL;

    _disable();
    uxReturn = __MFCR( $ICR );
    _mtcr( $ICR, ( ( uxReturn & ~portCCPN_MASK ) | configMAX_SYSCALL_INTERRUPT_PRIORITY ) );
    _isync();
    _enable();

    /* 仅返回中断屏蔽位。 */
    return( uxReturn & portCCPN_MASK );
}
/*-----------------------------------------------------------*/
