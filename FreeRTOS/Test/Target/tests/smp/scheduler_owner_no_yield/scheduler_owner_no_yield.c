/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file scheduler_owner_no_yield.c
 * @brief The core that owns the scheduler suspension must not be forced to yield.
 *
 * A task that has suspended the scheduler and then enters a TCB critical section
 * runs the run-state-change helper. If that core was allowed to be marked
 * taskTASK_SCHEDULED_TO_YIELD while it holds the suspension, the helper spins
 * forever: vTaskSwitchContext will not switch while the scheduler is suspended,
 * so the run state never clears. The fix pends the owner core's yield instead of
 * forcing it.
 *
 * Procedure (single core, no ISR needed):
 *   - Create a ready busy helper task, H, pinned to the test core at low priority.
 *   - Pin the test (victim) task V to the same core, higher priority than H.
 *   - V suspends the scheduler.
 *   - V raises H above itself. prvYieldForTask marks V's core to yield, even
 *     though the scheduler is suspended.
 *   - V enters and exits a TCB critical section (vTaskPreemptionDisable/Enable),
 *     which runs the run-state-change helper on the suspension owner core.
 *   - V restores H's priority and resumes the scheduler.
 * Expected:
 *   - V completes the TCB critical section and resumes the scheduler. Without
 *     the fix, V spins forever inside the run-state-change helper and the test
 *     hangs.
 */

/* Standard includes. */
#include <stdint.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#ifndef TEST_CONFIG_H
    #error test_config.h must be included at the end of FreeRTOSConfig.h.
#endif

#if ( configTARGET_TEST_USE_CUSTOM_SETTING == 1 )
    #include "test_setting_config.h"
#endif

#include "test_default_setting_config.h"

/*-----------------------------------------------------------*/

#if ( configNUMBER_OF_CORES < 2 )
    #error This test is for FreeRTOS SMP and therefore, requires at least 2 cores.
#endif

#if ( configRUN_MULTIPLE_PRIORITIES != 1 )
    #error configRUN_MULTIPLE_PRIORITIES must be set to 1 for this test.
#endif

#if ( configUSE_CORE_AFFINITY != 1 )
    #error configUSE_CORE_AFFINITY must be set to 1 for this test.
#endif

#if ( configUSE_TASK_PREEMPTION_DISABLE != 1 )
    #error configUSE_TASK_PREEMPTION_DISABLE must be set to 1 for this test.
#endif

/*-----------------------------------------------------------*/

/* The core on which the suspension owner and helper run. */
#define testCORE_UNDER_TEST    ( 0 )
#define testCORE_MASK          ( ( UBaseType_t ) ( 1U << testCORE_UNDER_TEST ) )

/* Mid-range priorities so the helper can be raised above the victim while
 * staying within the valid range, independent of the test runner's priority. */
#define testVICTIM_PRIORITY    ( ( UBaseType_t ) ( configMAX_PRIORITIES / 2U ) )
#define testHELPER_PRIORITY    ( ( UBaseType_t ) ( testVICTIM_PRIORITY - 1U ) )

/*-----------------------------------------------------------*/

static TaskHandle_t xHelperTaskHandle = NULL;
static volatile uint32_t ulHelperSpin = 0;

/*-----------------------------------------------------------*/

/* A ready task pinned to the core under test. It only needs to be runnable so
 * that raising its priority above the victim marks the victim's core to yield. */
static void prvHelperTask( void * pvParameters )
{
    ( void ) pvParameters;

    for( ; ; )
    {
        ulHelperSpin++;
    }
}
/*-----------------------------------------------------------*/

static void Test_SchedulerOwnerNotForcedToYield( void )
{
    /* Run on the same core as the helper, at a known mid-range priority - this
     * task becomes the suspension owner core. Setting our own priority makes the
     * test independent of the runner's priority (so raising the helper above us
     * stays in range). */
    vTaskCoreAffinitySet( NULL, testCORE_MASK );
    vTaskPrioritySet( NULL, testVICTIM_PRIORITY );
    vTaskDelay( pdMS_TO_TICKS( 10 ) );

    vTaskSuspendAll();
    {
        /* Raise the (ready) helper above ourselves on the same core. This marks
         * this core taskTASK_SCHEDULED_TO_YIELD even though the scheduler is
         * suspended. */
        vTaskPrioritySet( xHelperTaskHandle, testVICTIM_PRIORITY + 1U );

        /* Enter and exit a TCB critical section. vTaskPreemptionDisable() calls
         * vTaskTCBEnterCritical(), which runs prvTaskTCBLockCheckForRunStateChange
         * on the suspension owner core; with the fix it returns, and without it,
         * it spins forever (the test hangs). */
        vTaskPreemptionDisable( NULL );
        vTaskPreemptionEnable( NULL );

        /* Restore the helper priority before resuming. */
        vTaskPrioritySet( xHelperTaskHandle, testHELPER_PRIORITY );
    }
    ( void ) xTaskResumeAll();

    /* Reaching here means the suspension owner core was not forced to yield. */
    TEST_ASSERT_TRUE_MESSAGE( pdTRUE,
                              "Suspension owner core completed the TCB critical section" );
}
/*-----------------------------------------------------------*/

/* Runs before every test, put init calls here. */
testSETUP_FUNCTION_PROTOTYPE( setUp )
{
    BaseType_t xReturn;

    xHelperTaskHandle = NULL;
    xReturn = xTaskCreateAffinitySet( prvHelperTask,
                                      "helper",
                                      configMINIMAL_STACK_SIZE * 2,
                                      NULL,
                                      testHELPER_PRIORITY,
                                      testCORE_MASK,
                                      &xHelperTaskHandle );
    configASSERT( xReturn == pdPASS );
    ( void ) xReturn;
}
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
testTEARDOWN_FUNCTION_PROTOTYPE( tearDown )
{
    if( xHelperTaskHandle != NULL )
    {
        vTaskDelete( xHelperTaskHandle );
        xHelperTaskHandle = NULL;
    }
}
/*-----------------------------------------------------------*/

testENTRY_FUNCTION_PROTOTYPE( vRunSchedulerOwnerNoYield )
{
    testBEGIN_FUNCTION();

    testRUN_TEST_CASE_FUNCTION( Test_SchedulerOwnerNotForcedToYield );

    testEND_FUNCTION();
}
/*-----------------------------------------------------------*/
