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

#ifndef STACK_MACROS_H
#define STACK_MACROS_H

/*
 * 如果被换出任务的栈当前已溢出，或看起来过去可能曾溢出，
 * 则调用栈溢出钩子函数。
 *
 * 将 configCHECK_FOR_STACK_OVERFLOW 设为 1 会使该宏仅检查
 * 当前栈状态——将当前栈顶值与栈界限进行比较。将
 * configCHECK_FOR_STACK_OVERFLOW 设为大于 1 还会检查最后几个
 * 栈字节，以确保创建任务时写入这些字节的值未被覆写。
 * 注意，第二种检查并不保证总能识别出已溢出的栈。
 */

/*-----------------------------------------------------------*/

/*
 * portSTACK_LIMIT_PADDING 是视为已占用栈空间的额外字数。
 */
#ifndef portSTACK_LIMIT_PADDING
    #define portSTACK_LIMIT_PADDING    0
#endif

/*
 * 由于以下原因，MPU 移植层的栈溢出检查实现并不直接：
 * 1. 上下文保存在 TCB 中，因此 pxTopOfStack 成员指向 TCB 中的
 *    上下文位置。
 * 2. 系统调用在单独的特权栈上执行。
 *
 * 这仍然没有问题，因为使用 MPU 区域保护任务栈，这意味着
 * 非特权任务发生任务栈溢出会触发 MPU 故障。此外，具有硬件
 * 栈溢出检查支持的架构（例如 Armv8-M）会在任务栈溢出时触发故障。
 */
#if ( ( configCHECK_FOR_STACK_OVERFLOW == 1 ) && ( portSTACK_GROWTH < 0 ) && ( portUSING_MPU_WRAPPERS != 1 ) )

/* 仅检查当前栈状态。 */
    #define taskCHECK_FOR_STACK_OVERFLOW()                                                      \
    do                                                                                          \
    {                                                                                           \
        /* Is the currently saved stack pointer within the stack limit? */                      \
        if( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack + portSTACK_LIMIT_PADDING )     \
        {                                                                                       \
            char * pcOverflowTaskName = pxCurrentTCB->pcTaskName;                               \
            vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB, pcOverflowTaskName ); \
        }                                                                                       \
    } while( 0 )

#endif /* 配置项 configCHECK_FOR_STACK_OVERFLOW == 1 */
/*-----------------------------------------------------------*/

#if ( ( configCHECK_FOR_STACK_OVERFLOW == 1 ) && ( portSTACK_GROWTH > 0 ) && ( portUSING_MPU_WRAPPERS != 1 ) )

/* 仅检查当前栈状态。 */
    #define taskCHECK_FOR_STACK_OVERFLOW()                                                       \
    do                                                                                           \
    {                                                                                            \
        /* Is the currently saved stack pointer within the stack limit? */                       \
        if( pxCurrentTCB->pxTopOfStack >= pxCurrentTCB->pxEndOfStack - portSTACK_LIMIT_PADDING ) \
        {                                                                                        \
            char * pcOverflowTaskName = pxCurrentTCB->pcTaskName;                                \
            vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB, pcOverflowTaskName );  \
        }                                                                                        \
    } while( 0 )

#endif /* 配置项 configCHECK_FOR_STACK_OVERFLOW == 1 */
/*-----------------------------------------------------------*/

#if ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) && ( portSTACK_GROWTH < 0 ) && ( portUSING_MPU_WRAPPERS != 1 ) )

    #define taskCHECK_FOR_STACK_OVERFLOW()                                                       \
    do                                                                                           \
    {                                                                                            \
        const uint32_t * const pulStack = ( uint32_t * ) pxCurrentTCB->pxStack;                  \
        const uint32_t ulCheckValue = ( uint32_t ) 0xa5a5a5a5U;                                  \
                                                                                                 \
        if( ( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack + portSTACK_LIMIT_PADDING ) || \
            ( pulStack[ 0 ] != ulCheckValue ) ||                                                 \
            ( pulStack[ 1 ] != ulCheckValue ) ||                                                 \
            ( pulStack[ 2 ] != ulCheckValue ) ||                                                 \
            ( pulStack[ 3 ] != ulCheckValue ) )                                                  \
        {                                                                                        \
            char * pcOverflowTaskName = pxCurrentTCB->pcTaskName;                                \
            vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB, pcOverflowTaskName );  \
        }                                                                                        \
    } while( 0 )

#endif /* 条件编译结束：#if( configCHECK_FOR_STACK_OVERFLOW > 1 ) */
/*-----------------------------------------------------------*/

#if ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) && ( portSTACK_GROWTH > 0 ) && ( portUSING_MPU_WRAPPERS != 1 ) )

    #define taskCHECK_FOR_STACK_OVERFLOW()                                                                                                \
    do                                                                                                                                    \
    {                                                                                                                                     \
        int8_t * pcEndOfStack = ( int8_t * ) pxCurrentTCB->pxEndOfStack;                                                                  \
        static const uint8_t ucExpectedStackBytes[] = { tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,   \
                                                        tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,   \
                                                        tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,   \
                                                        tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE,   \
                                                        tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE, tskSTACK_FILL_BYTE }; \
                                                                                                                                          \
        pcEndOfStack -= sizeof( ucExpectedStackBytes );                                                                                   \
                                                                                                                                          \
        if( ( pxCurrentTCB->pxTopOfStack >= pxCurrentTCB->pxEndOfStack - portSTACK_LIMIT_PADDING ) ||                                     \
            ( memcmp( ( void * ) pcEndOfStack, ( void * ) ucExpectedStackBytes, sizeof( ucExpectedStackBytes ) ) != 0 ) )                 \
        {                                                                                                                                 \
            char * pcOverflowTaskName = pxCurrentTCB->pcTaskName;                                                                         \
            vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB, pcOverflowTaskName );                                           \
        }                                                                                                                                 \
    } while( 0 )

#endif /* 条件编译结束：#if( configCHECK_FOR_STACK_OVERFLOW > 1 ) */
/*-----------------------------------------------------------*/

/* 如果未使用栈溢出检查宏，则移除该宏。 */
#ifndef taskCHECK_FOR_STACK_OVERFLOW
    #define taskCHECK_FOR_STACK_OVERFLOW()
#endif



#endif /* 头文件保护：STACK_MACROS_H */
