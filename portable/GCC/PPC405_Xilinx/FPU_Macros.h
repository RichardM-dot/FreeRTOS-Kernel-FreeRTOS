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
 * 切换出任务时，若任务标签包含缓冲区地址，则将浮点上下文保存到该缓冲区。
 */
#define traceTASK_SWITCHED_OUT()                                         \
    if( pxCurrentTCB->pxTaskTag != NULL )                                \
    {                                                                    \
        extern void vPortSaveFPURegisters( void * );                     \
        vPortSaveFPURegisters( ( void * ) ( pxCurrentTCB->pxTaskTag ) ); \
    }

/*
 * 切换入任务时，若任务标签包含缓冲区地址，则从该缓冲区加载浮点上下文。
 */
#define traceTASK_SWITCHED_IN()                                             \
    if( pxCurrentTCB->pxTaskTag != NULL )                                   \
    {                                                                       \
        extern void vPortRestoreFPURegisters( void * );                     \
        vPortRestoreFPURegisters( ( void * ) ( pxCurrentTCB->pxTaskTag ) ); \
    }
