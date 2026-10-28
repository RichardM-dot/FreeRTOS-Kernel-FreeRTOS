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

/* GCC/HCS12 移植层，作者 Jefferson L Smith，2005 年 */

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 移植层头文件包含。 */
#include <sys/ports_def.h>

/*
 * -----------------------------------------------------------
 * HCS12 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */


/*
 * 配置定时器，按 FreeRTOSConfig.h 中指定的频率产生 RTOS 时钟节拍（tick）。
 */
static void prvSetupTimerInterrupt( void );

/*
 * 注意：中断服务程序必须位于非分 bank 存储器中 - 调度器启动函数也是如此。
 */
#define ATTR_NEAR    __attribute__( ( near ) )

/* 手动上下文切换函数。这是 SWI ISR。 */
/* __attribute__((interrupt))（中断属性） */
void ATTR_NEAR vPortYield( void );

/* 时钟节拍上下文切换函数。这是定时器 ISR。 */
/* __attribute__((interrupt))（中断属性） */
void ATTR_NEAR vPortTickInterrupt( void );

/* 位于非分 bank 存储器中、实际切换到第一个任务的函数。 */
BaseType_t ATTR_NEAR xStartSchedulerNear( void );

/*
 * 对 portENTER_CRITICAL() 的调用可以嵌套。嵌套时，在嵌套深度降到 0 之前
 * 不应离开临界区（即不应重新使能中断）。此变量仅用于跟踪嵌套深度。每个任务
 * 维护各自的临界区嵌套深度变量，因此上下文切换期间 uxCriticalNesting 会随
 * 任务栈一起保存和恢复。
 */
volatile UBaseType_t uxCriticalNesting = 0x80; /* 未初始化 */

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /*
     * 设置任务的初始栈。栈内容按 portRESTORE_CONTEXT() 宏的预期精确设置。
     * 此处栈的布局与 HCS12 RTI 指令的预期一致。
     */


    /* 任务函数地址按字节逐个放入栈中。 */
    *pxTopOfStack = ( StackType_t ) *( ( ( StackType_t * ) ( &pxCode ) ) + 1 );
    *--pxTopOfStack = ( StackType_t ) *( ( ( StackType_t * ) ( &pxCode ) ) + 0 );

    /* 接下来是构成任务上下文的所有寄存器。 */

    /* Y 寄存器 */
    *--pxTopOfStack = ( StackType_t ) 0xff;
    *--pxTopOfStack = ( StackType_t ) 0xee;

    /* X 寄存器 */
    *--pxTopOfStack = ( StackType_t ) 0xdd;
    *--pxTopOfStack = ( StackType_t ) 0xcc;

    /* A 寄存器包含参数高字节。 */
    *--pxTopOfStack = ( StackType_t ) *( ( ( StackType_t * ) ( &pvParameters ) ) + 0 );

    /* B 寄存器包含参数低字节。 */
    *--pxTopOfStack = ( StackType_t ) *( ( ( StackType_t * ) ( &pvParameters ) ) + 1 );

    /*
     * CCR：注意任务启动时中断将使能，因为 CCR 的 "I" 位已清零
     */
    *--pxTopOfStack = ( StackType_t ) 0x80; /* 保持 Stop 禁用（MCU 默认） */

    /* GCC 使用的临时软寄存器。当前取值无关紧要。 */
    __asm( "\n\
        movw _.frame, 2,-%0                         \n\
        movw _.tmp, 2,-%0                           \n\
        movw _.z, 2,-%0                             \n\
        movw _.xy, 2,-%0                            \n\
        ;movw _.d2, 2,-%0                           \n\
        ;movw _.d1, 2,-%0                           \n\
    " : "=A" ( pxTopOfStack ) : "0" ( pxTopOfStack ) );

    #ifdef BANKED_MODEL
        /* 任务所在的页。 */
        *--pxTopOfStack = 0x30; /* 只能直接从 PPAGE 0x30 启动 */
    #endif

    /*
     * 临界区嵌套深度初始化为 0（表示不在临界区中）。
     */
    *--pxTopOfStack = ( StackType_t ) 0x00;


    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* HCS12 移植层不太可能被停止。 */
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
    /* 使能硬件 RTI 定时器 */
    /* 忽略 configTICK_RATE_HZ */
    RTICTL = 0x50;  /* 16 MHz 晶振：976.56 Hz，1024mS */
    CRGINT |= 0x80; /* RTIE 位 */
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    /*
     * xPortStartScheduler() 不直接启动调度器，因为包含 xPortStartScheduler()
     * 原型的头文件属于公共内核代码，因而不能使用 CODE_SEG 编译指示。
     * 它改为调用本地定义的 xNearStartScheduler() - 后者确实使用了 CODE_SEG
     * 编译指示。
     */

    int16_t register d;

    __asm( "jmp  xStartSchedulerNear        ; will never return" : "=d" ( d ) );
    return d;
}
/*-----------------------------------------------------------*/

BaseType_t xStartSchedulerNear( void )
{
    /*
     * 配置将产生 RTOS 时钟节拍（tick）的定时器。调用此函数时中断已被禁止。
     */
    prvSetupTimerInterrupt();

    /* 恢复第一个任务的上下文。 */
    portRESTORE_CONTEXT();

    portISR_TAIL();

    /* 不应执行到此处！ */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

/*
 * 上下文切换函数。这些是中断服务程序。
 */

/*
 * 通过调用 portYIELD() 强制执行的手动上下文切换。这是 SWI 处理程序。
 */
void vPortYield( void )
{
    portISR_HEAD();

    /*
     * 注意：这是陷阱例程（swi），尽管并未定义为陷阱。它按 ISR 的方式填充栈，
     * 以便混合使用抢占式让出和协作式让出。
     */

    portSAVE_CONTEXT();
    vTaskSwitchContext();
    portRESTORE_CONTEXT();

    portISR_TAIL();
}
/*-----------------------------------------------------------*/

/*
 * RTOS 时钟节拍中断服务程序。若使用协作式调度器，则仅递增 tick 计数。
 * 若使用抢占式调度器，则可能发生上下文切换。
 */
void vPortTickInterrupt( void )
{
    portISR_HEAD();

    /* 清除时钟节拍定时器标志 */
    CRGFLG = 0x80;

    #if configUSE_PREEMPTION == 1
    {
        /* 可能发生上下文切换，因此保存上下文。 */
        portSAVE_CONTEXT();

        /* 递增 tick …… */
        if( xTaskIncrementTick() != pdFALSE )
        {
            /* 需要进行上下文切换。 */
            vTaskSwitchContext();
        }

        /*
         * 恢复任务的上下文 - 该任务可能不同于被中断的任务。
         */
        portRESTORE_CONTEXT();
    }
    #else /* 若 configUSE_PREEMPTION == 1 */
    {
        xTaskIncrementTick();
    }
    #endif /* 若 configUSE_PREEMPTION == 1 */

    portISR_TAIL();
}
