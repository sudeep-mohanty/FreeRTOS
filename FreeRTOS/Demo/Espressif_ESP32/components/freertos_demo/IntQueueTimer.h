/*
 * IntQueueTimer port shim for the ESP32 Common/Minimal demo.
 *
 * IntQueue.c includes "IntQueueTimer.h" and calls vInitialiseTimerForIntQueueTest()
 * to start dedicated timer interrupts that drive xFirstTimerHandler() and
 * xSecondTimerHandler().  The ESP32 demo instead drives those two handlers from
 * the FreeRTOS tick hook (see vFullDemoTickHookFunction in main_full.c), so this
 * header only needs to declare the init entry point; its implementation is a
 * no-op provided by the demo.
 */
#ifndef INT_QUEUE_TIMER_H
#define INT_QUEUE_TIMER_H

void vInitialiseTimerForIntQueueTest( void );

#endif /* INT_QUEUE_TIMER_H */
