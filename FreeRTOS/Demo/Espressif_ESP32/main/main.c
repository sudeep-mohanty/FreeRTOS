/*
 * Common/Minimal demo entry point on Espressif targets.
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char * TAG = "freertos_demo";

/* Set after main_full() finishes wiring the demo so the tick hook does
 * not dispatch ISR-side exercises before their queues / timers exist. */
static volatile BaseType_t xDemoReady = pdFALSE;

extern void main_full( void );
extern void vFullDemoIdleFunction( void );
extern void vFullDemoTickHookFunction( void );

/* Runs the idle-side helper exactly once (mutex registry round-trip)
 * then self-deletes.  Lives in its own task pinned to Core 0 because
 * vApplicationIdleHook can fire concurrently on either core, which
 * races on the singleton state vFullDemoIdleFunction touches. */
static void prvIdleHelperTask( void * pvParameters )
{
    ( void ) pvParameters;

    while( xDemoReady != pdTRUE )
    {
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }

    vFullDemoIdleFunction();
    vTaskDelete( NULL );
}

void app_main( void )
{
    ESP_LOGI( TAG, "Common/Minimal demo" );
    ESP_LOGI( TAG, "  configNUMBER_OF_CORES = %d", configNUMBER_OF_CORES );
    ESP_LOGI( TAG, "  configUSE_PREEMPTION  = %d", configUSE_PREEMPTION );
    ESP_LOGI( TAG, "  configMAX_PRIORITIES  = %d", configMAX_PRIORITIES );
    ESP_LOGI( TAG, "  configRUN_MULTIPLE_PRIORITIES = %d", configRUN_MULTIPLE_PRIORITIES );

    main_full();

    xTaskCreatePinnedToCore( prvIdleHelperTask,
                             "IdleHelper",
                             configMINIMAL_STACK_SIZE * 4,
                             NULL,
                             tskIDLE_PRIORITY + 1,
                             NULL,
                             0 );

    xDemoReady = pdTRUE;
}

/* Tick hook fires from each core's SysTick; restrict to Core 0 so the
 * single-threaded ISR exercises don't race on shared state. */
void vApplicationTickHook( void )
{
    if( ( xDemoReady == pdTRUE ) && ( xPortGetCoreID() == 0 ) )
    {
        vFullDemoTickHookFunction();
    }
}

/* Idle-time work runs from prvIdleHelperTask (Core-0 pinned) instead of
 * here; leave the hook empty so configUSE_IDLE_HOOK is satisfied. */
void vApplicationIdleHook( void )
{
}
