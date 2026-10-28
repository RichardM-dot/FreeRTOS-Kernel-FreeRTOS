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

/* 内核头文件包含。 */
#include "FreeRTOS.h"

/* 机器相关头文件包含。 */
#include <tc1782.h>
#include <machine/intrinsics.h>
#include <machine/cint.h>
/*---------------------------------------------------------------------------*/

/*
 * 保存/恢复上下文宏需要此引用。
 */
extern volatile uint32_t * pxCurrentTCB;
/*-----------------------------------------------------------*/

/*
 * 本文件包含系统中所有可能陷阱的基础定义。建议为所有陷阱提供实现，但目前它们
 * 只是触发 DEBUG 指令，以便容易看出是哪个陷阱被触发。
 *
 * 陷阱类 6（SYSCALL）专供操作系统使用。
 */

/* 陷阱类。 */
#define portMMU_TRAP                                                      0
#define portIPT_TRAP                                                      1
#define portIE_TRAP                                                       2
#define portCM_TRAP                                                       3
#define portSBP_TRAP                                                      4
#define portASSERT_TRAP                                                   5
#define portNMI_TRAP                                                      7

/* MMU 陷阱标识。 */
#define portTIN_MMU_VIRTUAL_ADDRESS_FILL                                  0
#define portTIN_MMU_VIRTUAL_ADDRESS_PROTECTION                            1

/* 内部保护陷阱标识。 */
#define portTIN_IPT_PRIVILEGED_INSTRUCTION                                1
#define portTIN_IPT_MEMORY_PROTECTION_READ                                2
#define portTIN_IPT_MEMORY_PROTECTION_WRITE                               3
#define portTIN_IPT_MEMORY_PROTECTION_EXECUTION                           4
#define portTIN_IPT_MEMORY_PROTECTION_PERIPHERAL_ACCESS                   5
#define portTIN_IPT_MEMORY_PROTECTION_NULL_ADDRESS                        6
#define portTIN_IPT_MEMORY_PROTECTION_GLOBAL_REGISTER_WRITE_PROTECTION    7

/* 指令错误陷阱标识。 */
#define portTIN_IE_ILLEGAL_OPCODE                                         1
#define portTIN_IE_UNIMPLEMENTED_OPCODE                                   2
#define portTIN_IE_INVALID_OPERAND                                        3
#define portTIN_IE_DATA_ADDRESS_ALIGNMENT                                 4
#define portTIN_IE_INVALID_LOCAL_MEMORY_ADDRESS                           5

/* 上下文管理陷阱标识。 */
#define portTIN_CM_FREE_CONTEXT_LIST_DEPLETION                            1
#define portTIN_CM_CALL_DEPTH_OVERFLOW                                    2
#define portTIN_CM_CALL_DEPTH_UNDEFLOW                                    3
#define portTIN_CM_FREE_CONTEXT_LIST_UNDERFLOW                            4
#define portTIN_CM_CALL_STACK_UNDERFLOW                                   5
#define portTIN_CM_CONTEXT_TYPE                                           6
#define portTIN_CM_NESTING_ERROR                                          7

/* 系统总线与外设陷阱标识。 */
#define portTIN_SBP_PROGRAM_FETCH_SYNCHRONOUS_ERROR                       1
#define portTIN_SBP_DATA_ACCESS_SYNCHRONOUS_ERROR                         2
#define portTIN_SBP_DATA_ACCESS_ASYNCHRONOUS_ERROR                        3
#define portTIN_SBP_COPROCESSOR_TRAP_ASYNCHRONOUS_ERROR                   4
#define portTIN_SBP_PROGRAM_MEMORY_INTEGRITY_ERROR                        5
#define portTIN_SBP_DATA_MEMORY_INTEGRITY_ERROR                           6

/* 断言陷阱标识。 */
#define portTIN_ASSERT_ARITHMETIC_OVERFLOW                                1
#define portTIN_ASSERT_STICKY_ARITHMETIC_OVERFLOW                         2

/* 不可屏蔽中断陷阱标识。 */
#define portTIN_NMI_NON_MASKABLE_INTERRUPT                                0
/*---------------------------------------------------------------------------*/

void vMMUTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
void vInternalProtectionTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
void vInstructionErrorTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
void vContextManagementTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
void vSystemBusAndPeripheralsTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
void vAssertionTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
void vNonMaskableInterruptTrap( int iTrapIdentification ) __attribute__( ( longcall, weak ) );
/*---------------------------------------------------------------------------*/

void vTrapInstallHandlers( void )
{
    if( 0 == _install_trap_handler( portMMU_TRAP, vMMUTrap ) )
    {
        _debug();
    }

    if( 0 == _install_trap_handler( portIPT_TRAP, vInternalProtectionTrap ) )
    {
        _debug();
    }

    if( 0 == _install_trap_handler( portIE_TRAP, vInstructionErrorTrap ) )
    {
        _debug();
    }

    if( 0 == _install_trap_handler( portCM_TRAP, vContextManagementTrap ) )
    {
        _debug();
    }

    if( 0 == _install_trap_handler( portSBP_TRAP, vSystemBusAndPeripheralsTrap ) )
    {
        _debug();
    }

    if( 0 == _install_trap_handler( portASSERT_TRAP, vAssertionTrap ) )
    {
        _debug();
    }

    if( 0 == _install_trap_handler( portNMI_TRAP, vNonMaskableInterruptTrap ) )
    {
        _debug();
    }
}
/*-----------------------------------------------------------*/

void vMMUTrap( int iTrapIdentification )
{
    switch( iTrapIdentification )
    {
        case portTIN_MMU_VIRTUAL_ADDRESS_FILL:
        case portTIN_MMU_VIRTUAL_ADDRESS_PROTECTION:
        default:
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/

void vInternalProtectionTrap( int iTrapIdentification )
{
    /* 有意落入 default 分支。 */
    switch( iTrapIdentification )
    {
        case portTIN_IPT_PRIVILEGED_INSTRUCTION:
        /* 当前执行级别不允许该指令，例如在 User-0 级别执行 DISABLE。 */

        case portTIN_IPT_MEMORY_PROTECTION_READ:
        /* 使用无效地址加载字。 */

        case portTIN_IPT_MEMORY_PROTECTION_WRITE:
        /* 使用无效地址存储字。 */

        case portTIN_IPT_MEMORY_PROTECTION_EXECUTION:
        /* PC 跳转到有效范围之外的地址。 */

        case portTIN_IPT_MEMORY_PROTECTION_PERIPHERAL_ACCESS:
        /* 在当前执行级别访问外设被拒绝。 */

        case portTIN_IPT_MEMORY_PROTECTION_NULL_ADDRESS:
        /* 空指针。 */

        case portTIN_IPT_MEMORY_PROTECTION_GLOBAL_REGISTER_WRITE_PROTECTION:
        /* 试图修改全局地址指针寄存器。 */

        default:

            pxCurrentTCB[ 0 ] = __MFCR( $PCXI );
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/

void vInstructionErrorTrap( int iTrapIdentification )
{
    /* 有意落入 default 分支。 */
    switch( iTrapIdentification )
    {
        case portTIN_IE_ILLEGAL_OPCODE:
        case portTIN_IE_UNIMPLEMENTED_OPCODE:
        case portTIN_IE_INVALID_OPERAND:
        case portTIN_IE_DATA_ADDRESS_ALIGNMENT:
        case portTIN_IE_INVALID_LOCAL_MEMORY_ADDRESS:
        default:
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/

void vContextManagementTrap( int iTrapIdentification )
{
    /* 有意落入 default 分支。 */
    switch( iTrapIdentification )
    {
        case portTIN_CM_FREE_CONTEXT_LIST_DEPLETION:
        case portTIN_CM_CALL_DEPTH_OVERFLOW:
        case portTIN_CM_CALL_DEPTH_UNDEFLOW:
        case portTIN_CM_FREE_CONTEXT_LIST_UNDERFLOW:
        case portTIN_CM_CALL_STACK_UNDERFLOW:
        case portTIN_CM_CONTEXT_TYPE:
        case portTIN_CM_NESTING_ERROR:
        default:
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/

void vSystemBusAndPeripheralsTrap( int iTrapIdentification )
{
    /* 有意落入 default 分支。 */
    switch( iTrapIdentification )
    {
        case portTIN_SBP_PROGRAM_FETCH_SYNCHRONOUS_ERROR:
        case portTIN_SBP_DATA_ACCESS_SYNCHRONOUS_ERROR:
        case portTIN_SBP_DATA_ACCESS_ASYNCHRONOUS_ERROR:
        case portTIN_SBP_COPROCESSOR_TRAP_ASYNCHRONOUS_ERROR:
        case portTIN_SBP_PROGRAM_MEMORY_INTEGRITY_ERROR:
        case portTIN_SBP_DATA_MEMORY_INTEGRITY_ERROR:
        default:
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/

void vAssertionTrap( int iTrapIdentification )
{
    /* 有意落入 default 分支。 */
    switch( iTrapIdentification )
    {
        case portTIN_ASSERT_ARITHMETIC_OVERFLOW:
        case portTIN_ASSERT_STICKY_ARITHMETIC_OVERFLOW:
        default:
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/

void vNonMaskableInterruptTrap( int iTrapIdentification )
{
    /* 有意落入 default 分支。 */
    switch( iTrapIdentification )
    {
        case portTIN_NMI_NON_MASKABLE_INTERRUPT:
        default:
            _debug();
            break;
    }
}
/*---------------------------------------------------------------------------*/
