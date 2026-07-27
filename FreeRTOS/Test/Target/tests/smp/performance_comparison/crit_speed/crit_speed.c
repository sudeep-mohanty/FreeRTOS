/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file crit_speed.c
 * @brief Test the speed of taskENTER_CRITICAL() and taskEXIT_CRITICAL()
 * functions without any lock contention.
 *
 * Procedure:
 *   - Measure elapsed time of taskENTER_CRITICAL()
 *   - Measure elapsed time of taskEXIT_CRITICAL()
 *   - Sample and average over testNUM_SAMPLES number of samples
 */
/*-----------------------------------------------------------*/
#include  <stdio.h>
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
#endif /* if configNUMBER_OF_CORES != 2 */

#if ( configRUN_MULTIPLE_PRIORITIES != 1 )
    #error configRUN_MULTIPLE_PRIORITIES must be set to 1 for this test.
#endif /* if ( configRUN_MULTIPLE_PRIORITIES != 1 ) */

#ifndef testGET_TIME_FUNCTION
    #error testGET_TIME_FUNCTION must be defined to run the test
#endif

#ifndef testPRINTF_FUNCTION
    #error testPRINTF_FUNCTION must be defined to run the test
#endif

#define testNUM_SAMPLES    ( 128U )
/*-----------------------------------------------------------*/

/**
 * @brief Test case "Critical Section Speed"
 */
static void Test_CriticalSectionSpeed( void );
/*-----------------------------------------------------------*/

static void Test_CriticalSectionSpeed( void )
{
    int i;

    UBaseType_t uxEntryElapsedCumulative = 0;
    UBaseType_t uxExitElapsedCumulative = 0;
    UBaseType_t uxTemp = 0;

    for( i = 0; i < testNUM_SAMPLES; i++ )
    {
        /* Test taskENTER_CRITICAL() elapsed time */
        uxTemp = testGET_TIME_FUNCTION();

        #if ( portUSING_GRANULAR_LOCKS == 1 )
            taskENTER_CRITICAL();
        #else
            vTaskEnterCritical();
        #endif
        uxEntryElapsedCumulative += ( testGET_TIME_FUNCTION() - uxTemp );

        /* Test taskEXIT_CRITICAL elapsed time */
        uxTemp = testGET_TIME_FUNCTION();

        #if ( portUSING_GRANULAR_LOCKS == 1 )
            taskEXIT_CRITICAL();
        #else
            vTaskExitCritical();
        #endif
        uxExitElapsedCumulative += ( testGET_TIME_FUNCTION() - uxTemp );
    }

    testPRINTF_FUNCTION( "taskENTER_CRITICAL() accumulated elapsed time: %u\n", uxEntryElapsedCumulative );
    testPRINTF_FUNCTION( "taskEXIT_CRITICAL() accumulated elapsed time: %u\n", uxExitElapsedCumulative );
    testPRINTF_FUNCTION( "taskENTER_CRITICAL() elapsed time: %u\n", uxEntryElapsedCumulative / testNUM_SAMPLES );
    testPRINTF_FUNCTION( "taskEXIT_CRITICAL() elapsed time: %u\n", uxExitElapsedCumulative / testNUM_SAMPLES );
}
/*-----------------------------------------------------------*/

/* Runs before every test, put init calls here. */
testSETUP_FUNCTION_PROTOTYPE( setUp )
{
    /* Nothing to do */
}
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
testTEARDOWN_FUNCTION_PROTOTYPE( tearDown )
{
    /* Nothing to do */
}
/*-----------------------------------------------------------*/

testENTRY_FUNCTION_PROTOTYPE( vRunCriticalSectionSpeed )
{
    testBEGIN_FUNCTION();

    testRUN_TEST_CASE_FUNCTION( Test_CriticalSectionSpeed );

    testEND_FUNCTION();
}
/*-----------------------------------------------------------*/
