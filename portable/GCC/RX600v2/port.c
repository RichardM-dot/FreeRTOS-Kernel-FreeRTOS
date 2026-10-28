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

/*
 * -----------------------------------------------------------
 * SH2A 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

/* 调度器头文件包含。 */
#include "FreeRTOS.h"
#include "task.h"

/* 库头文件包含。 */
#include "string.h"

/* 硬件相关定义。 */
#if ( configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H == 1 )

    #include "platform.h"

#else /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

    #include "iodefine.h"

#endif /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

/*-----------------------------------------------------------*/

/*
 * 任务应以中断使能且处于超级用户模式的状态启动，因此 PSW 中 U 和 I 置位，
 * PM 和 IPL 清零。
 */
#define portINITIAL_PSW     ( ( StackType_t ) 0x00030000 )
#define portINITIAL_FPSW    ( ( StackType_t ) 0x00000100 )

/*
 * 这些宏允许在 xTaskIncrementTick() 调用周围添加临界区，该函数仅在中断中以内核
 * 优先级（即已知优先级）调用。因此相比调用全局 SET/CLEAR_INTERRUPT_MASK 宏，
 * 这些本地宏是轻微优化，因为全局宏需要先读取旧 IPL 并存入局部变量。
 */
#define portMASK_INTERRUPTS_FROM_KERNEL_ISR()      __asm volatile ( "MVTIPL    %0" ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) )
#define portUNMASK_INTERRUPTS_FROM_KERNEL_ISR()    __asm volatile ( "MVTIPL    %0" ::"i" ( configKERNEL_INTERRUPT_PRIORITY ) )

/*-----------------------------------------------------------*/

/*
 * 启动第一个任务执行的函数 - 用汇编代码编写，因为需要直接访问寄存器。
 */
static void prvStartFirstTask( void ) __attribute__( ( naked ) );

/*
 * 软件中断处理程序。执行实际的上下文切换（保存和恢复寄存器）。用汇编代码编写，
 * 因为需要直接访问寄存器。
 */
#if ( configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H == 1 )
    R_BSP_PRAGMA_INTERRUPT( vSoftwareInterruptISR, VECT( ICU, SWINT ) )
    R_BSP_ATTRIB_INTERRUPT void vSoftwareInterruptISR( void ) __attribute__( ( naked ) );
#else /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */
    void vSoftwareInterruptISR( void ) __attribute__( ( naked ) );
#endif /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

/*
 * 时钟节拍 ISR 处理程序。使用的外设由应用程序通过钩子/回调函数配置。
 */
#if ( configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H == 1 )

    R_BSP_PRAGMA_INTERRUPT( vTickISR, _VECT( configTICK_VECTOR ) )
    R_BSP_ATTRIB_INTERRUPT void vTickISR( void ); /* 不要添加 __attribute__( ( interrupt ) )。 */

#else /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */

    void vTickISR( void ) __attribute__( ( interrupt ) );

#endif /* 若定义了 configINCLUDE_PLATFORM_H_INSTEAD_OF_IODEFINE_H */
/*-----------------------------------------------------------*/

extern void * pxCurrentTCB;

/*-----------------------------------------------------------*/

/*
 * 参见头文件中的说明。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /* 未包含 R0，因为它是栈指针。 */

    *pxTopOfStack = 0x00;
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_PSW;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode;

    /*
     * 调试时，将每个寄存器设为已知值会很有帮助。否则，为节省代码空间，可以只设置
     * 需要设置的寄存器。
     */
    #ifdef USE_FULL_REGISTER_INITIALISATION
    {
        pxTopOfStack--;
        *pxTopOfStack = 0xffffffff; /* r15 寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = 0xeeeeeeee;
        pxTopOfStack--;
        *pxTopOfStack = 0xdddddddd;
        pxTopOfStack--;
        *pxTopOfStack = 0xcccccccc;
        pxTopOfStack--;
        *pxTopOfStack = 0xbbbbbbbb;
        pxTopOfStack--;
        *pxTopOfStack = 0xaaaaaaaa;
        pxTopOfStack--;
        *pxTopOfStack = 0x99999999;
        pxTopOfStack--;
        *pxTopOfStack = 0x88888888;
        pxTopOfStack--;
        *pxTopOfStack = 0x77777777;
        pxTopOfStack--;
        *pxTopOfStack = 0x66666666;
        pxTopOfStack--;
        *pxTopOfStack = 0x55555555;
        pxTopOfStack--;
        *pxTopOfStack = 0x44444444;
        pxTopOfStack--;
        *pxTopOfStack = 0x33333333;
        pxTopOfStack--;
        *pxTopOfStack = 0x22222222;
        pxTopOfStack--;
    }
    #else /* 若定义了 USE_FULL_REGISTER_INITIALISATION */
    {
        pxTopOfStack -= 15;
    }
    #endif /* 若定义了 USE_FULL_REGISTER_INITIALISATION */

    *pxTopOfStack = ( StackType_t ) pvParameters; /* R1 寄存器 */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_FPSW;
    pxTopOfStack--;
    *pxTopOfStack = 0x11111111; /* 累加器 0。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x22222222; /* 累加器 0。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x33333333; /* 累加器 0。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x44444444; /* 累加器 1。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x55555555; /* 累加器 1。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x66666666; /* 累加器 1。 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    extern void vApplicationSetupTimerInterrupt( void );

    /* 使用 pxCurrentTCB，以免其被优化掉。 */
    if( pxCurrentTCB != NULL )
    {
        /*
         * 调用应用程序函数来设置将产生时钟节拍中断的定时器。这样应用程序可以决定使用
         * 哪个外设。提供了演示应用程序以展示合适的示例。
         */
        vApplicationSetupTimerInterrupt();

        /* 使能软件中断。 */
        _IEN( _ICU_SWINT ) = 1;

        /* 确保软件中断已清除。 */
        _IR( _ICU_SWINT ) = 0;

        /* 确保软件中断被设置为内核优先级。 */
        _IPR( _ICU_SWINT ) = configKERNEL_INTERRUPT_PRIORITY;

        /* 启动第一个任务。 */
        prvStartFirstTask();
    }

    /* 不应执行到此处。 */
    return pdFAIL;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /*
     * 在无需返回的移植层中未实现。
     * 人为强制触发断言。
     */
    configASSERT( pxCurrentTCB == NULL );
}
/*-----------------------------------------------------------*/

static void prvStartFirstTask( void )
{
    __asm volatile
    (

        /*
         * 启动调度器时无需将任何内容移到中断栈，因为该函数并非从中断中调用。
         * 只需确保当前栈为用户栈。
         */
        "SETPSW     U                       \n" \


        /*
         * 获取 pxCurrentTCB 当前所指向任务的相关栈位置。
         */
        "MOV.L      #_pxCurrentTCB, R15     \n" \
        "MOV.L      [R15], R15              \n" \
        "MOV.L      [R15], R0               \n" \


        /*
         * 从 pxCurrentTCB 所指向任务的栈中恢复寄存器。
         */
        "POP        R15                     \n" \

        /* 累加器低 32 位。 */
        "MVTACLO    R15, A0                 \n" \
        "POP        R15                     \n" \

        /* 累加器高 32 位。 */
        "MVTACHI    R15, A0                 \n" \
        "POP        R15                     \n" \

        /* 累加器保护字。 */
        "MVTACGU    R15, A0                 \n" \
        "POP        R15                     \n" \

        /* 累加器低 32 位。 */
        "MVTACLO    R15, A1                 \n" \
        "POP        R15                     \n" \

        /* 累加器高 32 位。 */
        "MVTACHI    R15, A1                 \n" \
        "POP        R15                     \n" \

        /* 累加器保护字。 */
        "MVTACGU    R15, A1                 \n" \
        "POP        R15                     \n" \

        /* 浮点状态字。 */
        "MVTC       R15, FPSW               \n" \

        /* R1 至 R15 - 未包含 R0，因为它是 SP。 */
        "POPM       R1-R15                  \n" \

        /* 这将弹出其余寄存器。 */
        "RTE                                \n" \
        "NOP                                \n" \
        "NOP                                \n"
    );
}
/*-----------------------------------------------------------*/

void vSoftwareInterruptISR( void )
{
    __asm volatile
    (
        /* 重新使能中断。 */
        "SETPSW     I                           \n" \


        /*
         * 将中断发生时自动压入中断栈的数据从中断栈移到用户栈。
         *
         * 保存 R15 后再覆盖它。
         */
        "PUSH.L     R15                         \n" \

        /* 读取用户栈指针。 */
        "MVFC       USP, R15                    \n" \

        /* 将地址下移到要移动的数据处。 */
        "SUB        #12, R15                    \n" \
        "MVTC       R15, USP                    \n" \

        /* 复制数据，依次为 R15、PC、PSW。 */
        "MOV.L      [ R0 ], [ R15 ]             \n" \
        "MOV.L      4[ R0 ], 4[ R15 ]           \n" \
        "MOV.L      8[ R0 ], 8[ R15 ]           \n" \

        /* 将中断栈指针移到其新的正确位置。 */
        "ADD        #12, R0                     \n" \

        /* 其余所有寄存器直接保存到用户栈。 */
        "SETPSW     U                           \n" \

        /* 保存其余的通用寄存器（R15 已保存）。 */
        "PUSHM      R1-R14                      \n" \

        /* 保存 FPSW 和累加器。 */
        "MVFC       FPSW, R15                   \n" \
        "PUSH.L     R15                         \n" \
        "MVFACGU    #0, A1, R15                 \n" \
        "PUSH.L     R15                         \n" \
        "MVFACHI    #0, A1, R15                 \n" \
        "PUSH.L     R15                         \n" \
        /* 低序字。 */
        "MVFACLO    #0, A1, R15                 \n" \
        "PUSH.L     R15                         \n" \
        "MVFACGU    #0, A0, R15                 \n" \
        "PUSH.L     R15                         \n" \
        "MVFACHI    #0, A0, R15                 \n" \
        "PUSH.L     R15                         \n" \
        /* 低序字。 */
        "MVFACLO    #0, A0, R15                 \n" \
        "PUSH.L     R15                         \n" \

        /* 将栈指针保存到 TCB。 */
        "MOV.L      #_pxCurrentTCB, R15         \n" \
        "MOV.L      [ R15 ], R15                \n" \
        "MOV.L      R0, [ R15 ]                 \n" \


        /*
         * 在访问内核结构期间，确保中断屏蔽设置为系统调用优先级。
         */
        "MVTIPL     %0                          \n" \

        /* 选择下一个要运行的任务。 */
        "BSR.A      _vTaskSwitchContext         \n" \

        /* 不再需要访问数据结构，重置中断屏蔽。 */
        "MVTIPL     %1                          \n" \


        /*
         * 从 TCB 加载现已选为运行态任务的栈指针。
         */
        "MOV.L      #_pxCurrentTCB,R15          \n" \
        "MOV.L      [ R15 ], R15                \n" \
        "MOV.L      [ R15 ], R0                 \n" \


        /*
         * 恢复新任务的上下文。PSW（程序状态字）和 PC 将由 RTE 指令弹出。
         */
        "POP        R15                         \n" \

        /* 累加器低 32 位。 */
        "MVTACLO    R15, A0                     \n" \
        "POP        R15                         \n" \

        /* 累加器高 32 位。 */
        "MVTACHI    R15, A0                     \n" \
        "POP        R15                         \n" \

        /* 累加器保护字。 */
        "MVTACGU    R15, A0                     \n" \
        "POP        R15                         \n" \

        /* 累加器低 32 位。 */
        "MVTACLO    R15, A1                     \n" \
        "POP        R15                         \n" \

        /* 累加器高 32 位。 */
        "MVTACHI    R15, A1                     \n" \
        "POP        R15                         \n" \

        /* 累加器保护字。 */
        "MVTACGU    R15, A1                     \n" \
        "POP        R15                         \n" \
        "MVTC       R15, FPSW                   \n" \
        "POPM       R1-R15                      \n" \
        "RTE                                    \n" \
        "NOP                                    \n" \
        "NOP                                      "
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ), "i" ( configKERNEL_INTERRUPT_PRIORITY )
    );
}
/*-----------------------------------------------------------*/

void vTickISR( void )
{
    /* 重新使能中断。 */
    __asm volatile ( "SETPSW I" );

    /*
     * 递增 tick，并执行新 tick 值所需的任何处理。首先确保 IPL 处于最大系统调用值。
     */
    portMASK_INTERRUPTS_FROM_KERNEL_ISR();
    {
        if( xTaskIncrementTick() != pdFALSE )
        {
            taskYIELD();
        }
    }
    portUNMASK_INTERRUPTS_FROM_KERNEL_ISR();
}
/*-----------------------------------------------------------*/

uint32_t ulPortGetIPL( void )
{
    __asm volatile
    (
        "MVFC   PSW, R1         \n" \
        "SHLR   #24, R1         \n" \
        "RTS                      "
    );

    /* 这永远不会执行，但可避免编译器报怨。 */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortSetIPL( uint32_t ulNewIPL )
{
    /* 避免编译器报告未引用参数的警告。 */
    ( void ) ulNewIPL;

    __asm volatile
    (
        "PUSH   R5              \n" \
        "MVFC   PSW, R5         \n" \
        "SHLL   #24, R1         \n" \
        "AND    #-0F000001H, R5 \n" \
        "OR     R1, R5          \n" \
        "MVTC   R5, PSW         \n" \
        "POP    R5              \n" \
        "RTS                      "
    );
}
