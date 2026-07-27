/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file queue_speed.c
 * @brief Test the speed of xQueueSend() and xQueueReceive() when not blocking
 *
 * Procedure:
 *   - Measure elapsed time of xQueueSend()
 *   - Measure elapsed time of xQueueReceive()
 *   - Sample and average over testNUM_SAMPLES number of samples
 */

/*-----------------------------------------------------------*/
#include  <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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
 * @brief Test case "Queue Speed Non-Blocking"
 */
static void Test_QueueSpeedNonBlocking( void );
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the queue used in this test
 */
static QueueHandle_t xQueueHandle;
/*-----------------------------------------------------------*/

static void Test_QueueSpeedNonBlocking( void )
{
    int i;

    UBaseType_t uxSendElapsedCumulative = 0;
    UBaseType_t uxRecvElapsedCumulative = 0;
    UBaseType_t uxTemp = 0;

    for( i = 0; i < testNUM_SAMPLES; i++ )
    {
        /* Test xQueueSend() elapsed time */
        uxTemp = testGET_TIME_FUNCTION();
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xQueueSend( xQueueHandle, &i, 0 ), "xQueueSend() failed" );
        uxSendElapsedCumulative += ( testGET_TIME_FUNCTION() - uxTemp );
    }

    for( i = 0; i < testNUM_SAMPLES; i++ )
    {
        int iValue;
        uxTemp = testGET_TIME_FUNCTION();
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xQueueReceive( xQueueHandle, &iValue, 0 ), "xQueueReceive() failed" );
        uxRecvElapsedCumulative += ( testGET_TIME_FUNCTION() - uxTemp );
    }

    testPRINTF_FUNCTION( "xQueueSend() average elapsed time: %u\n", uxSendElapsedCumulative / testNUM_SAMPLES );
    testPRINTF_FUNCTION( "xQueueReceive() average elapsed time: %u\n", uxRecvElapsedCumulative / testNUM_SAMPLES );
}
/*-----------------------------------------------------------*/

/* Runs before every test, put init calls here. */
testSETUP_FUNCTION_PROTOTYPE( setUp )
{
    xQueueHandle = xQueueCreate( testNUM_SAMPLES, sizeof( int ) );
    TEST_ASSERT_NOT_NULL_MESSAGE( xQueueHandle, "Queue creation failed" );
}
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
testTEARDOWN_FUNCTION_PROTOTYPE( tearDown )
{
    vQueueDelete( xQueueHandle );
    xQueueHandle = NULL;
}
/*-----------------------------------------------------------*/

testENTRY_FUNCTION_PROTOTYPE( vRunQueueSpeedNonBlocking )
{
    testBEGIN_FUNCTION();

    testRUN_TEST_CASE_FUNCTION( Test_QueueSpeedNonBlocking );

    testEND_FUNCTION();
}
/*-----------------------------------------------------------*/
