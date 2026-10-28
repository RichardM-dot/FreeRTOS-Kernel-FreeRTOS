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

/* 调度器头文件。 */
#include "FreeRTOS.h"
#include "task.h"

/* 硬件头文件。 */
#include <microblaze_exceptions_i.h>
#include <microblaze_exceptions_g.h>

/*
 * Xilinx 库定义的异常入口会压入多个寄存器。
 * 这些定义是从栈指针到各压栈寄存器值的偏移量。
 */
#define portexR3_STACK_OFFSET                 4
#define portexR4_STACK_OFFSET                 5
#define portexR5_STACK_OFFSET                 6
#define portexR6_STACK_OFFSET                 7
#define portexR7_STACK_OFFSET                 8
#define portexR8_STACK_OFFSET                 9
#define portexR9_STACK_OFFSET                 10
#define portexR10_STACK_OFFSET                11
#define portexR11_STACK_OFFSET                12
#define portexR12_STACK_OFFSET                13
#define portexR15_STACK_OFFSET                16
#define portexR18_STACK_OFFSET                19
#define portexMSR_STACK_OFFSET                20
#define portexR19_STACK_OFFSET                -1

/*
 * 该值等于 Xilinx 标准库异常入口生成的栈帧大小（字节）。
 * 在进入异常之前确定栈指针值需要使用它。
 */
#define portexASM_HANDLER_STACK_FRAME_SIZE    84UL

/* 一条 MicroBlaze 指令占用的字节数。 */
#define portexINSTRUCTION_SIZE                4

/*
 * 若 MicroBlaze 未配置为处理异常，或应用定义的配置常量
 * configINSTALL_EXCEPTION_HANDLERS 未设置为 1，则排除整个文件。
 */
#if ( MICROBLAZE_EXCEPTIONS_ENABLED == 1 ) && ( configINSTALL_EXCEPTION_HANDLERS == 1 )

/*
 * 该变量在异常入口代码中设置，位于调用 vPortExceptionHandler 之前。
 */
    uint32_t * pulStackPointerOnFunctionEntry = NULL;

/*
 * 该结构体填充异常发生前一刻的 MicroBlaze 上下文。
 * 若定义了 vApplicationExceptionRegisterDump() 回调函数，
 * 则将该结构体的指针传入该函数。
 */
    static xPortRegisterDump xRegisterDump;

/*
 * 这是为所有异常类型安装的 FreeRTOS 异常处理程序。
 * 它由 vPortExceptionHandlerEntry() 调用——后者本身定义在 portasm.S 中。
 */
    void vPortExceptionHandler( void * pvExceptionID );
    extern void vPortExceptionHandlerEntry( void * pvExceptionID );

/*-----------------------------------------------------------*/

/*
 * vApplicationExceptionRegisterDump() 是一个回调函数，应用可选择定义它，
 * 以接收填充好的 xPortRegisterDump 结构体。若应用选择不定义
 * vApplicationExceptionRegisterDump()，则改为调用此处的弱定义默认实现。
 */
    extern void vApplicationExceptionRegisterDump( xPortRegisterDump * xRegisterDump ) __attribute__( ( weak ) );
    void vApplicationExceptionRegisterDump( xPortRegisterDump * xRegisterDump )
    {
        ( void ) xRegisterDump;

        for( ; ; )
        {
            portNOP();
        }
    }
/*-----------------------------------------------------------*/

    void vPortExceptionHandler( void * pvExceptionID )
    {
        extern void * pxCurrentTCB;

        /*
         * 用异常发生前一刻的 MicroBlaze 上下文填充 xPortRegisterDump 结构体。
         */

        /*
         * 首先填写异常发生时处于运行态的任务的名称和句柄。
         */
        xRegisterDump.xCurrentTaskHandle = pxCurrentTCB;
        xRegisterDump.pcCurrentTaskName = pcTaskGetName( NULL );

        configASSERT( pulStackPointerOnFunctionEntry );

        /*
         * 获取在本函数调用之前已压栈、并且此后可能已发生变化的寄存器的值。
         */
        xRegisterDump.ulR3 = pulStackPointerOnFunctionEntry[ portexR3_STACK_OFFSET ];
        xRegisterDump.ulR4 = pulStackPointerOnFunctionEntry[ portexR4_STACK_OFFSET ];
        xRegisterDump.ulR5 = pulStackPointerOnFunctionEntry[ portexR5_STACK_OFFSET ];
        xRegisterDump.ulR6 = pulStackPointerOnFunctionEntry[ portexR6_STACK_OFFSET ];
        xRegisterDump.ulR7 = pulStackPointerOnFunctionEntry[ portexR7_STACK_OFFSET ];
        xRegisterDump.ulR8 = pulStackPointerOnFunctionEntry[ portexR8_STACK_OFFSET ];
        xRegisterDump.ulR9 = pulStackPointerOnFunctionEntry[ portexR9_STACK_OFFSET ];
        xRegisterDump.ulR10 = pulStackPointerOnFunctionEntry[ portexR10_STACK_OFFSET ];
        xRegisterDump.ulR11 = pulStackPointerOnFunctionEntry[ portexR11_STACK_OFFSET ];
        xRegisterDump.ulR12 = pulStackPointerOnFunctionEntry[ portexR12_STACK_OFFSET ];
        xRegisterDump.ulR15_return_address_from_subroutine = pulStackPointerOnFunctionEntry[ portexR15_STACK_OFFSET ];
        xRegisterDump.ulR18 = pulStackPointerOnFunctionEntry[ portexR18_STACK_OFFSET ];
        xRegisterDump.ulR19 = pulStackPointerOnFunctionEntry[ portexR19_STACK_OFFSET ];
        xRegisterDump.ulMSR = pulStackPointerOnFunctionEntry[ portexMSR_STACK_OFFSET ];

        /* 获取所有其他寄存器的值。 */
        xRegisterDump.ulR2_small_data_area = mfgpr( R2 );
        xRegisterDump.ulR13_read_write_small_data_area = mfgpr( R13 );
        xRegisterDump.ulR14_return_address_from_interrupt = mfgpr( R14 );
        xRegisterDump.ulR16_return_address_from_trap = mfgpr( R16 );
        xRegisterDump.ulR17_return_address_from_exceptions = mfgpr( R17 );
        xRegisterDump.ulR20 = mfgpr( R20 );
        xRegisterDump.ulR21 = mfgpr( R21 );
        xRegisterDump.ulR22 = mfgpr( R22 );
        xRegisterDump.ulR23 = mfgpr( R23 );
        xRegisterDump.ulR24 = mfgpr( R24 );
        xRegisterDump.ulR25 = mfgpr( R25 );
        xRegisterDump.ulR26 = mfgpr( R26 );
        xRegisterDump.ulR27 = mfgpr( R27 );
        xRegisterDump.ulR28 = mfgpr( R28 );
        xRegisterDump.ulR29 = mfgpr( R29 );
        xRegisterDump.ulR30 = mfgpr( R30 );
        xRegisterDump.ulR31 = mfgpr( R31 );
        xRegisterDump.ulR1_SP = ( ( uint32_t ) pulStackPointerOnFunctionEntry ) + portexASM_HANDLER_STACK_FRAME_SIZE;
        xRegisterDump.ulEAR = mfear();
        xRegisterDump.ulESR = mfesr();
        xRegisterDump.ulEDR = mfedr();

        /*
         * 将保存的程序计数器移回异常发生时正在执行的指令。
         * 这仅对某些类型的异常有效。
         */
        xRegisterDump.ulPC = xRegisterDump.ulR17_return_address_from_exceptions - portexINSTRUCTION_SIZE;

        #if ( XPAR_MICROBLAZE_USE_FPU != 0 )
        {
            xRegisterDump.ulFSR = mffsr();
        }
        #else
        {
            xRegisterDump.ulFSR = 0UL;
        }
        #endif

        /*
         * 同时填写描述异常类型的字符串。
         * 该字符串使用 MicroBlaze 标准库异常头文件中定义的相同 ID 名称。
         */
        switch( ( uint32_t ) pvExceptionID )
        {
            case XEXC_ID_FSL:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_FSL";
                break;

            case XEXC_ID_UNALIGNED_ACCESS:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_UNALIGNED_ACCESS";
                break;

            case XEXC_ID_ILLEGAL_OPCODE:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_ILLEGAL_OPCODE";
                break;

            case XEXC_ID_M_AXI_I_EXCEPTION:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_M_AXI_I_EXCEPTION or XEXC_ID_IPLB_EXCEPTION";
                break;

            case XEXC_ID_M_AXI_D_EXCEPTION:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_M_AXI_D_EXCEPTION or XEXC_ID_DPLB_EXCEPTION";
                break;

            case XEXC_ID_DIV_BY_ZERO:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_DIV_BY_ZERO";
                break;

            case XEXC_ID_STACK_VIOLATION:
                xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_STACK_VIOLATION or XEXC_ID_MMU";
                break;

                #if ( XPAR_MICROBLAZE_USE_FPU != 0 )
                    case XEXC_ID_FPU:
                        xRegisterDump.pcExceptionCause = ( int8_t * const ) "XEXC_ID_FPU see ulFSR value";
                        break;
                #endif /* XPAR_MICROBLAZE_USE_FPU（使用 FPU） */
        }

        /*
         * vApplicationExceptionRegisterDump() 是一个回调函数，应用可选择定义它，
         * 以接收填充好的 xPortRegisterDump 结构体。若应用选择不定义
         * vApplicationExceptionRegisterDump()，则改为调用本文件中的弱定义默认实现。
         */
        vApplicationExceptionRegisterDump( &xRegisterDump );

        /* 不得试图离开本函数！ */
        for( ; ; )
        {
            portNOP();
        }
    }
/*-----------------------------------------------------------*/

    void vPortExceptionsInstallHandlers( void )
    {
        static uint32_t ulHandlersAlreadyInstalled = pdFALSE;

        if( ulHandlersAlreadyInstalled == pdFALSE )
        {
            ulHandlersAlreadyInstalled = pdTRUE;

            #if XPAR_MICROBLAZE_UNALIGNED_EXCEPTIONS == 1
                microblaze_register_exception_handler( XEXC_ID_UNALIGNED_ACCESS, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_UNALIGNED_ACCESS );
            #endif /* XPAR_MICROBLAZE_UNALIGNED_EXCEPTIONS（非对齐访问异常） */

            #if XPAR_MICROBLAZE_ILL_OPCODE_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_ILLEGAL_OPCODE, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_ILLEGAL_OPCODE );
            #endif /* XPAR_MICROBLAZE_ILL_OPCODE_EXCEPTION（非法操作码异常） */

            #if XPAR_MICROBLAZE_M_AXI_I_BUS_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_M_AXI_I_EXCEPTION, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_M_AXI_I_EXCEPTION );
            #endif /* XPAR_MICROBLAZE_M_AXI_I_BUS_EXCEPTION（M_AXI 指令总线异常） */

            #if XPAR_MICROBLAZE_M_AXI_D_BUS_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_M_AXI_D_EXCEPTION, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_M_AXI_D_EXCEPTION );
            #endif /* XPAR_MICROBLAZE_M_AXI_D_BUS_EXCEPTION（M_AXI 数据总线异常） */

            #if XPAR_MICROBLAZE_IPLB_BUS_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_IPLB_EXCEPTION, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_IPLB_EXCEPTION );
            #endif /* XPAR_MICROBLAZE_IPLB_BUS_EXCEPTION（IPLB 总线异常） */

            #if XPAR_MICROBLAZE_DPLB_BUS_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_DPLB_EXCEPTION, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_DPLB_EXCEPTION );
            #endif /* XPAR_MICROBLAZE_DPLB_BUS_EXCEPTION（DPLB 总线异常） */

            #if XPAR_MICROBLAZE_DIV_ZERO_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_DIV_BY_ZERO, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_DIV_BY_ZERO );
            #endif /* XPAR_MICROBLAZE_DIV_ZERO_EXCEPTION（除零异常） */

            #if XPAR_MICROBLAZE_FPU_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_FPU, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_FPU );
            #endif /* XPAR_MICROBLAZE_FPU_EXCEPTION（FPU 异常） */

            #if XPAR_MICROBLAZE_FSL_EXCEPTION == 1
                microblaze_register_exception_handler( XEXC_ID_FSL, vPortExceptionHandlerEntry, ( void * ) XEXC_ID_FSL );
            #endif /* XPAR_MICROBLAZE_FSL_EXCEPTION（FSL 异常） */

            microblaze_enable_exceptions();
        }
    }

/*
 * 若 MicroBlaze 未配置为处理异常，或应用定义的配置项
 * configINSTALL_EXCEPTION_HANDLERS 未设置为 1，则排除整个文件。
 */
#endif /* ( MICROBLAZE_EXCEPTIONS_ENABLED == 1 ) && ( configINSTALL_EXCEPTION_HANDLERS == 1 )（MicroBlaze 异常功能与异常处理程序安装均已启用） */
