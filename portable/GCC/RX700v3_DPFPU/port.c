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
 * RXv3 DPFPU 移植层中 portable.h 所定义函数的实现。
 * ----------------------------------------------------------
 */

#warning Testing for DFPU support in this port is not yet complete

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
#define portINITIAL_PSW                  ( ( StackType_t ) 0x00030000 )
#define portINITIAL_FPSW                 ( ( StackType_t ) 0x00000100 )
#define portINITIAL_DPSW                 ( ( StackType_t ) 0x00000100 )
#define portINITIAL_DCMR                 ( ( StackType_t ) 0x00000000 )
#define portINITIAL_DECNT                ( ( StackType_t ) 0x00000001 )

/*
 * 任务创建时不带 DPFPU 上下文，但可以在创建后为其分配 DPFPU 上下文。任务上下文中
 * 保存一个变量：若任务没有 DPFPU 上下文则存放 portNO_DPFPU_CONTEXT，否则存放
 * 其他值。
 */
#define portNO_DPFPU_CONTEXT             ( ( StackType_t ) 0 )
#define portHAS_DPFPU_CONTEXT            ( ( StackType_t ) 1 )

/*
 * 栈上保存 DPFPU 数据寄存器所需的空间。共有 16 个 64 位寄存器。
 */
#define portDPFPU_DATA_REGISTER_WORDS    ( 16 * 2 )

/*
 * 这些宏允许在 xTaskIncrementTick() 调用周围添加临界区，该函数仅在中断中以内核
 * 优先级（即已知优先级）调用。因此相比调用全局 SET/CLEAR_INTERRUPT_MASK 宏，
 * 这些本地宏是轻微优化，因为全局宏需要先读取旧 IPL 并存入局部变量。
 */
#define portMASK_INTERRUPTS_FROM_KERNEL_ISR()      __asm volatile ( "MVTIPL %0" ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) )
#define portUNMASK_INTERRUPTS_FROM_KERNEL_ISR()    __asm volatile ( "MVTIPL %0" ::"i" ( configKERNEL_INTERRUPT_PRIORITY ) )

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

/*
 * 作为任务上下文的一部分保存。若 ulPortTaskHasDPFPUContext 非零，则必须为该任务
 * 保存和恢复 DPFPU 上下文。
 */
#if ( configUSE_TASK_DPFPU_SUPPORT == 1 )

    StackType_t ulPortTaskHasDPFPUContext = portNO_DPFPU_CONTEXT;

#endif /* configUSE_TASK_DPFPU_SUPPORT（宏） */

/*
 * 内联汇编函数会访问它，因此为方便起见将其设为文件作用域。
 */
extern void * pxCurrentTCB;
extern void vTaskSwitchContext( void );

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
    *pxTopOfStack = 0x11111111; /* 累加器 1。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x22222222; /* 累加器 1。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x33333333; /* 累加器 1。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x44444444; /* 累加器 0。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x55555555; /* 累加器 0。 */
    pxTopOfStack--;
    *pxTopOfStack = 0x66666666; /* 累加器 0。 */

    #if ( configUSE_TASK_DPFPU_SUPPORT == 1 )
    {
        /*
         * 任务启动时不带 DPFPU 上下文。使用 DPFPU 硬件的任务必须在执行任何浮点指令
         * 之前调用 vPortTaskUsesDPFPU()。
         */
        pxTopOfStack--;
        *pxTopOfStack = portNO_DPFPU_CONTEXT;
    }
    #elif ( configUSE_TASK_DPFPU_SUPPORT == 2 )
    {
        /*
         * 任务启动时带有 DPFPU 上下文。为寄存器留出足够空间 - 并在需要时确保它们被初始化。
         */
        #ifdef USE_FULL_REGISTER_INITIALISATION
        {
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 1515.1515;  /* DR15 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 1414.1414;  /* DR14 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 1313.1313;  /* DR13 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 1212.1212;  /* DR12 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 1111.1111;  /* DR11 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 1010.1010;  /* DR10 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 909.0909;   /* DR9 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 808.0808;   /* DR8 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 707.0707;   /* DR7 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 606.0606;   /* DR6 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 505.0505;   /* DR5 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 404.0404;   /* DR4 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 303.0303;   /* DR3 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 202.0202;   /* DR2 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 101.0101;   /* DR1 寄存器。 */
            pxTopOfStack -= 2;
            *( double * ) pxTopOfStack = 9876.54321; /* DR0 寄存器。 */
        }
        #else /* 若定义了 USE_FULL_REGISTER_INITIALISATION */
        {
            pxTopOfStack -= portDPFPU_DATA_REGISTER_WORDS;
            memset( pxTopOfStack, 0x00, portDPFPU_DATA_REGISTER_WORDS * sizeof( StackType_t ) );
        }
        #endif /* 若定义了 USE_FULL_REGISTER_INITIALISATION */
        pxTopOfStack--;
        *pxTopOfStack = portINITIAL_DECNT; /* DECNT 寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = portINITIAL_DCMR;  /* DCMR 寄存器。 */
        pxTopOfStack--;
        *pxTopOfStack = portINITIAL_DPSW;  /* DPSW 寄存器。 */
    }
    #elif ( configUSE_TASK_DPFPU_SUPPORT == 0 )
    {
        /* 省略 DPFPU 支持。 */
    }
    #else /* 若 ( configUSE_TASK_DPFPU_SUPPORT == 1 ) */
    {
        #error Invalid configUSE_TASK_DPFPU_SUPPORT setting - configUSE_TASK_DPFPU_SUPPORT must be set to 0, 1, 2, or left undefined.
    }
    #endif /* 若 ( configUSE_TASK_DPFPU_SUPPORT == 1 ) */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

#if ( configUSE_TASK_DPFPU_SUPPORT == 1 )

    void vPortTaskUsesDPFPU( void )
    {
        /*
         * 任务正在登记其需要 DPFPU 上下文。设置 DPFPU 标志（该标志作为任务上下文的一部分保存）。
         */
        ulPortTaskHasDPFPUContext = portHAS_DPFPU_CONTEXT;
    }

#endif /* configUSE_TASK_DPFPU_SUPPORT（宏） */
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

        #if ( configUSE_TASK_DPFPU_SUPPORT == 1 )

            /*
             * 此处恢复的 ulPortTaskHasDPFPUContext 应为零。
             * 因此此处永远不需要恢复 DPFPU 上下文。
             */
            "POP        R15                                 \n" \
            "MOV.L      #_ulPortTaskHasDPFPUContext, R14    \n" \
            "MOV.L      R15, [R14]                          \n" \

        #elif ( configUSE_TASK_DPFPU_SUPPORT == 2 )
            /* 恢复 DPFPU 上下文。 */
            "DPOPM.L    DPSW-DECNT              \n" \
            "DPOPM.D    DR0-DR15                \n" \

        #endif /* 若 ( configUSE_TASK_DPFPU_SUPPORT == 1 ) */

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
        "MVFC       FPSW, R15                   \n"                       \
        "PUSH.L     R15                         \n"                       \
        "MVFACGU    #0, A1, R15                 \n"                       \
        "PUSH.L     R15                         \n"                       \
        "MVFACHI    #0, A1, R15                 \n"                       \
        "PUSH.L     R15                         \n"                       \
        "MVFACLO    #0, A1, R15                 \n" /* 低序字。 */ \
        "PUSH.L     R15                         \n"                       \
        "MVFACGU    #0, A0, R15                 \n"                       \
        "PUSH.L     R15                         \n"                       \
        "MVFACHI    #0, A0, R15                 \n"                       \
        "PUSH.L     R15                         \n"                       \
        "MVFACLO    #0, A0, R15                 \n" /* 低序字。 */ \
        "PUSH.L     R15                         \n"                       \

        #if ( configUSE_TASK_DPFPU_SUPPORT == 1 )

            /*
             * 任务是否有需要保存的 DPFPU 上下文？若 ulPortTaskHasDPFPUContext 为 0，则没有。
             */
            "MOV.L      #_ulPortTaskHasDPFPUContext, R15    \n" \
            "MOV.L      [R15], R15                          \n" \
            "CMP        #0, R15                             \n" \

            /* 保存 DPFPU 上下文（如有）。 */
            "BEQ.B      ?+                          \n" \
            "DPUSHM.D   DR0-DR15                    \n" \
            "DPUSHM.L   DPSW-DECNT                  \n" \
            "?:                                     \n" \

            /* 保存 ulPortTaskHasDPFPUContext 本身。 */
            "PUSH.L     R15                         \n" \

        #elif ( configUSE_TASK_DPFPU_SUPPORT == 2 )
            /* 始终保存 DPFPU 上下文。 */
            "DPUSHM.D   DR0-DR15                    \n" \
            "DPUSHM.L   DPSW-DECNT                  \n" \

        #endif /* 若 ( configUSE_TASK_DPFPU_SUPPORT == 1 ) */


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

        #if ( configUSE_TASK_DPFPU_SUPPORT == 1 )

            /*
             * 是否有需要恢复的 DPFPU 上下文？若恢复的 ulPortTaskHasDPFPUContext 为零，则没有。
             */
            "POP        R15                                 \n" \
            "MOV.L      #_ulPortTaskHasDPFPUContext, R14    \n" \
            "MOV.L      R15, [R14]                          \n" \
            "CMP        #0, R15                             \n" \

            /* 恢复 DPFPU 上下文（如有）。 */
            "BEQ.B      ?+                          \n" \
            "DPOPM.L    DPSW-DECNT                  \n" \
            "DPOPM.D    DR0-DR15                    \n" \
            "?:                                     \n" \

        #elif ( configUSE_TASK_DPFPU_SUPPORT == 2 )
            /* 始终恢复 DPFPU 上下文。 */
            "DPOPM.L    DPSW-DECNT                  \n" \
            "DPOPM.D    DR0-DR15                    \n" \

        #endif /* 若 ( configUSE_TASK_DPFPU_SUPPORT == 1 ) */

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
    __asm volatile ( "SETPSW    I" );

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
/*-----------------------------------------------------------*/
