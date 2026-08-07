/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief Runner for a single granular-lock performance-comparison test on ESP32.
 *
 * The test to run is selected at build time via -DPERF_TEST=<name>; the CMake
 * layer passes the matching entry function as TEST_ENTRY_FN.
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/* Provided by the selected test file (see boards/esp32/main/CMakeLists.txt). */
extern void TEST_ENTRY_FN( void );

/*-----------------------------------------------------------*/

static void prvRunnerTask( void * pvParameters )
{
    ( void ) pvParameters;

    printf( "\n===PERF_TEST_BEGIN===\n" );
    TEST_ENTRY_FN();
    printf( "===PERF_TEST_END===\n" );

    vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

void app_main( void )
{
    /* Match the reference (pico) board: highest priority, no core affinity. The
     * tests that care about placement set their own affinity; the perf tests
     * spawn their own affinity-pinned worker tasks from here. */
    ( void ) xTaskCreate( prvRunnerTask,
                          "test_runner",
                          8192,
                          NULL,
                          configMAX_PRIORITIES - 1,
                          NULL );
}
