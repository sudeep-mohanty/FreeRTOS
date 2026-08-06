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
 * layer passes the matching entry function as PERF_ENTRY_FN.
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/* Provided by the selected test file (see boards/esp32/main/CMakeLists.txt). */
extern void PERF_ENTRY_FN( void );

/*-----------------------------------------------------------*/

static void prvRunnerTask( void * pvParameters )
{
    ( void ) pvParameters;

    printf( "\n===PERF_TEST_BEGIN===\n" );
    PERF_ENTRY_FN();
    printf( "===PERF_TEST_END===\n" );

    vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

void app_main( void )
{
    /* Pin the runner to core 0 at a high priority. Contention tests spawn their
     * own affinity-pinned worker tasks from here. */
    ( void ) xTaskCreatePinnedToCore( prvRunnerTask,
                                      "perf_runner",
                                      8192,
                                      NULL,
                                      configMAX_PRIORITIES - 2,
                                      NULL,
                                      0 );
}
