/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * license and copyright intentionally withheld to promote copying into user code.
 */

#include "FreeRTOS.h"
#include "task.h"

BaseType_t xPortStartScheduler( void )
{
    return pdTRUE;
}

void vPortEndScheduler( void )
{
}

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    ( void ) pxTopOfStack;
    ( void ) pvParameters;
    ( void ) * pxCode;

    return NULL;
}

void vPortYield( void )
{
    /* 保存当前上下文 */

    /* 切换到已就绪的最高优先级任务。 */
    #if ( configNUMBER_OF_CORES == 1 )
    {
        vTaskSwitchContext();
    }
    #else
    {
        vTaskSwitchContext( portGET_CORE_ID() );
    }
    #endif

    /* 开始执行刚刚切换到的任务。 */
}

static void prvTickISR( void )
{
    /*
     * 必须已使能中断，ISR 才能触发，因此需要在中断使能状态下
     * 保存上下文。
     */

    #if ( configNUMBER_OF_CORES == 1 )
    {
        /* 维护时钟节拍计数。 */
        if( xTaskIncrementTick() != pdFALSE )
        {
            /* 切换到已就绪的最高优先级任务。 */
            vTaskSwitchContext();
        }
    }
    #else
    {
        UBaseType_t ulPreviousMask;

        /*
         * 在多核环境中，运行在其他核上的任务或 ISR 可能仍处于
         * 临界区。递增节拍计数必须在临界区中执行。
         */
        ulPreviousMask = taskENTER_CRITICAL_FROM_ISR();

        /* 维护时钟节拍计数。 */
        if( xTaskIncrementTick() != pdFALSE )
        {
            /* 切换到已就绪的最高优先级任务。 */
            vTaskSwitchContext( portGET_CORE_ID() );
        }

        taskEXIT_CRITICAL_FROM_ISR( ulPreviousMask );
    }
    #endif /* 条件块结束：if ( configNUMBER_OF_CORES == 1 ) */

    /* 开始执行新任务 */
}
