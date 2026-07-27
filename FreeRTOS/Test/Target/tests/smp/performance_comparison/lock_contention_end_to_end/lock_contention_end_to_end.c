/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lock_contention_end_to_end.c
 * @brief End to end speed test of lock contention scenario
 *
 * This tests recreates a common lock contention scenario under the BKL (i.e.
 * global locking) scheme. Each core has a producer task, consumer task, and a
 * queue.This is to simulate two independent stream of data being independently
 * processed in parallel on each core.
 *
 * The lower throughput under BKL due to lock contention between the multiple
 * cores should be demonstrated by this test.
 *
 * Procedure:
 *   - For each core
 *      - Create a queue
 *      - Create a producer task that sends data to the queue
 *      - Create a consumer task that reads data from the queue
 *   - Repeat for testNUM_SAMPLES iterations
 *      - Start the producer and consumer tasks
 *      - Measure time elapsed for producer/consumer task to send/receive
 *        testNUM_ITEMS to/from the queue
 */

/*-----------------------------------------------------------*/
#include  <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#ifndef TEST_CONFIG_H
    #error test_config.h must be included at the end of FreeRTOSConfig.h.
#endif

#if ( configTARGET_TEST_USE_CUSTOM_SETTING == 1 )
    #include "test_setting_config.h"
#endif

#include "test_default_setting_config.h"

#if ( configNUMBER_OF_CORES < 2 )
    #error This test is for FreeRTOS SMP and therefore, requires at least 2 cores.
#endif /* if configNUMBER_OF_CORES != 2 */

#if ( configRUN_MULTIPLE_PRIORITIES != 1 )
    #error configRUN_MULTIPLE_PRIORITIES must be set to 1 for this test.
#endif /* if ( configRUN_MULTIPLE_PRIORITIES != 1 ) */

/*-----------------------------------------------------------*/

#ifndef testNUMBER_OF_CORES
    #define testNUMBER_OF_CORES    ( configNUMBER_OF_CORES )
#endif

#ifndef testGET_TIME_FUNCTION
    #error testGET_TIME_FUNCTION must be defined to run the test
#endif

#ifndef testPRINTF_FUNCTION
    #error testPRINTF_FUNCTION must be defined to run the test
#endif

#ifndef testTASK_WITH_AFFINITY
    #define testTASK_WITH_AFFINITY        ( 1 )
#endif

#define testNUM_SAMPLES                   ( 128U )
#define testNUM_ITEMS                     ( 256U )

#define TEST_START_BIT                    ( 1 << ( configNUMBER_OF_CORES ) )
#define TEST_END_BIT                      ( 1 << ( configNUMBER_OF_CORES + 1 ) )

#define TEST_BUSYLOOPING_TASK_PRIORITY    ( configMAX_PRIORITIES - 2 )
#define TEST_PRODUCER_PRIORITY            ( configMAX_PRIORITIES - 4 )
#define TEST_CONSUMER_PRIORITY            ( configMAX_PRIORITIES - 3 )

/*-----------------------------------------------------------*/

/**
 * @brief Test case "Lock Contention"
 */
static void Test_LockContentionEndToEnd( void );
/*-----------------------------------------------------------*/

/**
 * @brief Producer task run on each core
 */
static void prvProducerTask( void * pvParameters );
/*-----------------------------------------------------------*/

/**
 * @brief Consumer task run on each core
 */
static void prvConsumerTask( void * pvParameters );
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the queues accessed by each core
 */
static QueueHandle_t xQueueHandles[ testNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the producer tasks created for each core
 */
static TaskHandle_t xProducerTaskHandles[ testNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the consumer tasks created for each core
 */
static TaskHandle_t xConsumerTaskHandles[ testNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the busylooping task for unused cores.
 */
static TaskHandle_t xBusyLoopingTaskHandles[ configNUMBER_OF_CORES - testNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Start times of each iteration for each core
 */
static UBaseType_t uxStartTimes[ testNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Cumulative elapsed time of all iterations for each core
 */
static UBaseType_t uxElapsedCumulative[ testNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Event group to control and wait signal from consumer tasks
 */
static EventGroupHandle_t xControlEventGroup;
static EventGroupHandle_t xSignalEventGroup;

/*-----------------------------------------------------------*/

static void prvBusyLoopingTask( void * pvParameters )
{
    ( void ) pvParameters;

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

static void prvProducerTask( void * pvParameters )
{
    int iSamples;
    int iItems;
    BaseType_t testIndex = ( BaseType_t ) ( pvParameters );

    for( iSamples = 0; iSamples < testNUM_SAMPLES; iSamples++ )
    {
        /* Wait to be started by consumer task */
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

        /* Record the start time for this sample */
        uxStartTimes[ testIndex ] = testGET_TIME_FUNCTION();

        for( iItems = 0; iItems < testNUM_ITEMS; iItems++ )
        {
            BaseType_t xReturn;
            xReturn = xQueueSend( xQueueHandles[ testIndex ], &iItems, 0 );
            TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xReturn, "xQueueSend() failed" );
        }
    }

    /* Wait to be deleted */
    vTaskSuspend( NULL );
}
/*-----------------------------------------------------------*/

static void prvConsumerTask( void * pvParameters )
{
    int iSamples;
    int iItems;
    BaseType_t testIndex = ( BaseType_t ) ( pvParameters );

    for( iSamples = 0; iSamples < testNUM_SAMPLES; iSamples++ )
    {
        /* Notify main task that iteration is ready */
        xEventGroupSetBits( xSignalEventGroup, ( 1U << ( testIndex ) ) );

        /* Wait to be started by consumer task */
        EventBits_t bits = xEventGroupWaitBits(
            xControlEventGroup, /* Event group handle */
            TEST_START_BIT,     /* Bits to wait for */
            pdFALSE,            /* Clear bits before returning */
            pdTRUE,             /* Wait for all bits */
            portMAX_DELAY );    /* Wait indefinitely */

        /* Start the producer task */
        xTaskNotifyGive( xProducerTaskHandles[ testIndex ] );

        /* Consume queue messages sent by producer tasks */
        for( iItems = 0; iItems < testNUM_ITEMS; iItems++ )
        {
            int Temp;
            BaseType_t xReturn;

            xReturn = xQueueReceive( xQueueHandles[ testIndex ], &Temp, portMAX_DELAY );
            TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xReturn, "xQueueReceive() failed" );
            TEST_ASSERT_EQUAL_MESSAGE( iItems, Temp, "Received incorrect value" );
        }

        /* Record end time for this iteration and add it to the cumulative count */
        uxElapsedCumulative[ testIndex ] += testGET_TIME_FUNCTION() - uxStartTimes[ testIndex ];

        /* Notify main task that iteration is complete */
        xEventGroupSetBits( xSignalEventGroup, ( 1U << ( testIndex ) ) );

        /* Wait to be started by consumer task */
        bits = xEventGroupWaitBits(
            xControlEventGroup, /* Event group handle */
            TEST_END_BIT,       /* Bits to wait for */
            pdFALSE,            /* Clear bits before returning */
            pdTRUE,             /* Wait for all bits */
            portMAX_DELAY );    /* Wait indefinitely */
    }

    /* Wait to be deleted */
    vTaskSuspend( NULL );
}
/*-----------------------------------------------------------*/

static void Test_LockContentionEndToEnd( void )
{
    int iCore;
    int iIter;
    int i;
    EventBits_t xEventBits = 0U;

    for( i = 0; i < testNUMBER_OF_CORES; i++ )
    {
        xEventBits = xEventBits | ( 1 << i );
    }

    /* Run test for testNUM_SAMPLES number of iterations */
    for( iIter = 0; iIter < testNUM_SAMPLES; iIter++ )
    {
        /* Wait until all cores have ready for the tests. */
        ( void ) xEventGroupWaitBits( xSignalEventGroup,
                                      xEventBits,
                                      pdTRUE,
                                      pdTRUE,
                                      portMAX_DELAY );
        xEventGroupClearBits( xControlEventGroup, TEST_END_BIT );
        xEventGroupSetBits( xControlEventGroup, TEST_START_BIT );

        /* Wait until both cores have completed this iteration */
        ( void ) xEventGroupWaitBits( xSignalEventGroup,
                                      xEventBits,
                                      pdTRUE,
                                      pdTRUE,
                                      portMAX_DELAY );
        xEventGroupClearBits( xControlEventGroup, TEST_START_BIT );
        xEventGroupSetBits( xControlEventGroup, TEST_END_BIT );
    }

    /* Print average results */
    testPRINTF_FUNCTION( "Time taken to send %d items, averaged over %d samples\n", testNUM_ITEMS, testNUM_SAMPLES );

    for( iCore = 0; iCore < testNUMBER_OF_CORES; iCore++ )
    {
        testPRINTF_FUNCTION( "Core %d: %d\n", iCore, ( int ) ( uxElapsedCumulative[ iCore ] / testNUM_SAMPLES ) );
    }
}
/*-----------------------------------------------------------*/

/* Runs before every test, put init calls here. */
testSETUP_FUNCTION_PROTOTYPE( setUp )
{
    int i;
    BaseType_t xRet;

    /* Create event group. */
    xControlEventGroup = xEventGroupCreate();
    TEST_ASSERT_NOT_NULL_MESSAGE( xControlEventGroup, "Failed to create event group" );

    xSignalEventGroup = xEventGroupCreate();
    TEST_ASSERT_NOT_NULL_MESSAGE( xSignalEventGroup, "Failed to create event group" );

    /* Create separate queues and tasks for each core */
    for( i = 0; i < testNUMBER_OF_CORES; i++ )
    {
        /* Separate queues for each core */
        xQueueHandles[ i ] = xQueueCreate( testNUM_ITEMS, sizeof( int ) );
        TEST_ASSERT_NOT_NULL_MESSAGE( xQueueHandles[ i ], "Queue creation failed" );

        /* A producer task for each core to send to its queue */
        #if ( testTASK_WITH_AFFINITY == 1 )
            xRet = xTaskCreateAffinitySet( prvProducerTask,
                                           "prod",
                                           configMINIMAL_STACK_SIZE * 4,
                                           ( void * ) ( i ),
                                           TEST_PRODUCER_PRIORITY,
                                           ( 1 << i ),
                                           &xProducerTaskHandles[ i ] );
        #else
            xRet = xTaskCreate( prvProducerTask,
                                "prod",
                                configMINIMAL_STACK_SIZE * 4,
                                ( void * ) ( i ),
                                TEST_PRODUCER_PRIORITY,
                                &xProducerTaskHandles[ i ] );
        #endif /* if ( testTASK_WITH_AFFINITY == 1 ) */
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xRet, "Creating producer task failed" );

        /* A consumer task for each core to read items from its queue. The
         * consumer tasks has a higher priority than producer tasks so that sent
         * items are read immediately. */
        #if ( testTASK_WITH_AFFINITY == 1 )
            xRet = xTaskCreateAffinitySet( prvConsumerTask,
                                           "con",
                                           configMINIMAL_STACK_SIZE * 4,
                                           ( void * ) ( i ),
                                           TEST_CONSUMER_PRIORITY,
                                           ( 1 << i ),
                                           &xConsumerTaskHandles[ i ] );
        #else
            xRet = xTaskCreate( prvConsumerTask,
                                "con",
                                configMINIMAL_STACK_SIZE * 4,
                                ( void * ) ( i ),
                                TEST_CONSUMER_PRIORITY,
                                &xConsumerTaskHandles[ i ] );
        #endif /* if ( testTASK_WITH_AFFINITY == 1 ) */
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xRet, "Creating consumer task failed" );
    }

    #if ( testTASK_WITH_AFFINITY == 0 )
        for( ; i < configNUMBER_OF_CORES; i++ )
        {
            xRet = xTaskCreate( prvBusyLoopingTask,
                                "busy",
                                configMINIMAL_STACK_SIZE * 4,
                                ( void * ) ( i ),
                                TEST_BUSYLOOPING_TASK_PRIORITY,
                                &xBusyLoopingTaskHandles[ i - testNUMBER_OF_CORES ] );
        }
    #endif
}
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
testTEARDOWN_FUNCTION_PROTOTYPE( tearDown )
{
    int i;

    /* Delete tasks and queues */
    for( i = 0; i < testNUMBER_OF_CORES; i++ )
    {
        vTaskDelete( xProducerTaskHandles[ i ] );
        vTaskDelete( xConsumerTaskHandles[ i ] );
        vQueueDelete( xQueueHandles[ i ] );
    }

    #if ( testTASK_WITH_AFFINITY == 0 )
        for( ; i < configNUMBER_OF_CORES; i++ )
        {
            vTaskDelete( &xBusyLoopingTaskHandles[ i - testNUMBER_OF_CORES ] );
        }
    #endif
}
/*-----------------------------------------------------------*/

testENTRY_FUNCTION_PROTOTYPE( vRunLockContentionEndToEnd )
{
    testBEGIN_FUNCTION();

    testRUN_TEST_CASE_FUNCTION( Test_LockContentionEndToEnd );

    testEND_FUNCTION();
}
