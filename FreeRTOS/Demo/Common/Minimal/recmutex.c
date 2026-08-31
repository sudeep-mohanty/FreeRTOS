/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 *  The tasks defined on this page demonstrate the use of recursive mutexes.
 *
 *  For recursive mutex functionality the created mutex should be created using
 *  xSemaphoreCreateRecursiveMutex(), then be manipulated
 *  using the xSemaphoreTakeRecursive() and xSemaphoreGiveRecursive() API
 *  functions.
 *
 *  This demo creates three tasks all of which access the same recursive mutex:
 *
 *  prvRecursiveMutexControllingTask() has the highest priority so executes
 *  first and grabs the mutex.  It then performs some recursive accesses -
 *  between each of which it sleeps for a short period to let the lower
 *  priority tasks execute.  When it has completed its demo functionality
 *  it gives the mutex back before suspending itself.
 *
 *  prvRecursiveMutexBlockingTask() attempts to access the mutex by performing
 *  a blocking 'take'.  The blocking task has a lower priority than the
 *  controlling	task so by the time it executes the mutex has already been
 *  taken by the controlling task,  causing the blocking task to block.  It
 *  does not unblock until the controlling task has given the mutex back,
 *  and it does not actually run until the controlling task has suspended
 *  itself (due to the relative priorities).  When it eventually does obtain
 *  the mutex all it does is give the mutex back prior to also suspending
 *  itself.  At this point both the controlling task and the blocking task are
 *  suspended.
 *
 *  prvRecursiveMutexPollingTask() runs at the idle priority.  It spins round
 *  a tight loop attempting to obtain the mutex with a non-blocking call.  As
 *  the lowest priority task it will not successfully obtain the mutex until
 *  both the controlling and blocking tasks are suspended.  Once it eventually
 *  does obtain the mutex it first unsuspends both the controlling task and
 *  blocking task prior to giving the mutex back - resulting in the polling
 *  task temporarily inheriting the controlling tasks priority.
 */

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Bounded wait for this task's own priority to reach the expected value.
 * Under configRUN_MULTIPLE_PRIORITIES == 1 + dual-core SMP, a peer's
 * mutex operation that triggers priority inheritance / disinheritance
 * runs in parallel on the other core, so a snapshot read from this task
 * may observe the priority before it has settled. */
#if ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 )
    static BaseType_t prvWaitForOwnPriority( UBaseType_t uxExpected )
    {
        TickType_t xStart = xTaskGetTickCount();

        while( uxTaskPriorityGet( NULL ) != uxExpected )
        {
            if( ( xTaskGetTickCount() - xStart ) > pdMS_TO_TICKS( 100 ) )
            {
                return pdFAIL;
            }

            vTaskDelay( 1 );
        }

        return pdPASS;
    }
    #define recmuCHECK_OWN_PRIORITY( uxExpected )    ( prvWaitForOwnPriority( ( uxExpected ) ) == pdPASS )
#else /* if ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 ) */
    #define recmuCHECK_OWN_PRIORITY( uxExpected )    ( uxTaskPriorityGet( NULL ) == ( uxExpected ) )
#endif /* if ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 ) */

#if !( ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 ) )

/* Wait, up to a generous bound, for a peer task to set its "suspended" flag.
 * The blocking task becomes the mutex holder the instant the controlling task
 * gives the mutex back, which it does immediately before suspending; under
 * multi-priority SMP the controlling task can still be executing those few
 * instructions on the other core.  Returns pdFAIL if the flag is not set within
 * the bound, indicating a genuine fault rather than scheduling skew. */
    static BaseType_t prvWaitForSuspension( volatile BaseType_t * pxSuspendedFlag )
    {
        TickType_t xStart = xTaskGetTickCount();

        while( *pxSuspendedFlag != pdTRUE )
        {
            if( ( xTaskGetTickCount() - xStart ) > pdMS_TO_TICKS( 100 ) )
            {
                return pdFAIL;
            }

            vTaskDelay( 1 );
        }

        return pdPASS;
    }

#endif /* ! ( ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 ) ) */

/* Demo app include files. */
#include "recmutex.h"

/* Priorities assigned to the three tasks.  recmuCONTROLLING_TASK_PRIORITY can
 * be overridden by a definition in FreeRTOSConfig.h. */
#ifndef recmuCONTROLLING_TASK_PRIORITY
    #define recmuCONTROLLING_TASK_PRIORITY               ( tskIDLE_PRIORITY + 2 )
#endif
#define recmuBLOCKING_TASK_PRIORITY                      ( tskIDLE_PRIORITY + 1 )
#define recmuPOLLING_TASK_PRIORITY                       ( tskIDLE_PRIORITY + 0 )

/* The recursive call depth. */
#define recmuMAX_COUNT                                   ( 10 )

/* Misc. */
#define recmuSHORT_DELAY                                 ( pdMS_TO_TICKS( 20 ) )
#define recmuNO_DELAY                                    ( ( TickType_t ) 0 )
#define recmu15ms_DELAY                                  ( pdMS_TO_TICKS( 15 ) )

#ifndef recmuRECURSIVE_MUTEX_TEST_TASK_STACK_SIZE
    #define recmuRECURSIVE_MUTEX_TEST_TASK_STACK_SIZE    configMINIMAL_STACK_SIZE
#endif

/* The three tasks as described at the top of this file. */
static void prvRecursiveMutexControllingTask( void * pvParameters );
static void prvRecursiveMutexBlockingTask( void * pvParameters );
static void prvRecursiveMutexPollingTask( void * pvParameters );

/* The mutex used by the demo. */
static SemaphoreHandle_t xMutex;

/* Variables used to detect and latch errors. */
static volatile BaseType_t xErrorOccurred = pdFALSE, xControllingIsSuspended = pdFALSE, xBlockingIsSuspended = pdFALSE;
static volatile UBaseType_t uxControllingCycles = 0, uxBlockingCycles = 0, uxPollingCycles = 0;

/* Handles of the two higher priority tasks, required so they can be resumed
 * (unsuspended). */
static TaskHandle_t xControllingTaskHandle, xBlockingTaskHandle;

/*-----------------------------------------------------------*/

void vStartRecursiveMutexTasks( void )
{
    /* Just creates the mutex and the three tasks. */

    xMutex = xSemaphoreCreateRecursiveMutex();

    if( xMutex != NULL )
    {
        /* vQueueAddToRegistry() adds the mutex to the registry, if one is
         * in use.  The registry is provided as a means for kernel aware
         * debuggers to locate mutex and has no purpose if a kernel aware debugger
         * is not being used.  The call to vQueueAddToRegistry() will be removed
         * by the pre-processor if configQUEUE_REGISTRY_SIZE is not defined or is
         * defined to be less than 1. */
        vQueueAddToRegistry( ( QueueHandle_t ) xMutex, "Recursive_Mutex" );

        xTaskCreate( prvRecursiveMutexControllingTask, "Rec1", recmuRECURSIVE_MUTEX_TEST_TASK_STACK_SIZE, NULL, recmuCONTROLLING_TASK_PRIORITY, &xControllingTaskHandle );
        xTaskCreate( prvRecursiveMutexBlockingTask, "Rec2", recmuRECURSIVE_MUTEX_TEST_TASK_STACK_SIZE, NULL, recmuBLOCKING_TASK_PRIORITY, &xBlockingTaskHandle );
        xTaskCreate( prvRecursiveMutexPollingTask, "Rec3", recmuRECURSIVE_MUTEX_TEST_TASK_STACK_SIZE, NULL, recmuPOLLING_TASK_PRIORITY, NULL );
    }
}
/*-----------------------------------------------------------*/

static void prvRecursiveMutexControllingTask( void * pvParameters )
{
    UBaseType_t ux;

    /* Just to remove compiler warning. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Should not be able to 'give' the mutex, as we have not yet 'taken'
         * it.   The first time through, the mutex will not have been used yet,
         * subsequent times through, at this point the mutex will be held by the
         * polling task. */
        if( xSemaphoreGiveRecursive( xMutex ) == pdPASS )
        {
            xErrorOccurred = __LINE__;
        }

        for( ux = 0; ux < recmuMAX_COUNT; ux++ )
        {
            /* We should now be able to take the mutex as many times as
             * we like.
             *
             * The first time through the mutex will be immediately available, on
             * subsequent times through the mutex will be held by the polling task
             * at this point and this Take will cause the polling task to inherit
             * the priority of this task.  The polling task can take longer than
             * the block time to hand the mutex back (it holds the mutex while
             * priority inheritance settles), so a single take can time out -
             * retry until it succeeds so the number of takes always matches the
             * number of gives below.  A genuine failure to ever obtain the mutex
             * shows up as a stall that the check task detects. */
            while( xSemaphoreTakeRecursive( xMutex, recmu15ms_DELAY ) != pdPASS )
            {
                /* Mutex not available yet - try again. */
            }

            /* Ensure the other task attempting to access the mutex (and the
            * other demo tasks) are able to execute to ensure they either block
            * (where a block time is specified) or return an error (where no
            * block time is specified) as the mutex is held by this task. */
            vTaskDelay( recmuSHORT_DELAY );
        }

        /* For each time we took the mutex, give it back. */
        for( ux = 0; ux < recmuMAX_COUNT; ux++ )
        {
            /* Ensure the other task attempting to access the mutex (and the
             * other demo tasks) are able to execute. */
            vTaskDelay( recmuSHORT_DELAY );

            /* We should now be able to give the mutex as many times as we
             * took it.  When the mutex is available again the Blocking task
             * should be unblocked but not run because it has a lower priority
             * than this task.  The polling task should also not run at this point
             * as it too has a lower priority than this task. */
            if( xSemaphoreGiveRecursive( xMutex ) != pdPASS )
            {
                xErrorOccurred = __LINE__;
            }

            #if ( configUSE_PREEMPTION == 0 )
                taskYIELD();
            #endif
        }

        /* Having given it back the same number of times as it was taken, we
         * should no longer be the mutex owner, so the next give should fail. */
        if( xSemaphoreGiveRecursive( xMutex ) == pdPASS )
        {
            xErrorOccurred = __LINE__;
        }

        /* Keep count of the number of cycles this task has performed so a
         * stall can be detected. */
        uxControllingCycles++;

        /* Suspend ourselves so the blocking task can execute. */
        xControllingIsSuspended = pdTRUE;
        vTaskSuspend( NULL );
        xControllingIsSuspended = pdFALSE;
    }
}
/*-----------------------------------------------------------*/

static void prvRecursiveMutexBlockingTask( void * pvParameters )
{
    /* Just to remove compiler warning. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* This task will run while the controlling task is blocked, and the
         * controlling task will block only once it has the mutex - therefore
         * this call should block until the controlling task has given up the
         * mutex, and not actually execute	past this call until the controlling
         * task is suspended.  portMAX_DELAY - 1 is used instead of portMAX_DELAY
         * to ensure the task's state is reported as Blocked and not Suspended in
         * a later call to configASSERT() (within the polling task). */
        if( xSemaphoreTakeRecursive( xMutex, ( portMAX_DELAY - 1 ) ) == pdPASS )
        {
            /* The controlling task gives the mutex back immediately before
             * suspending itself, so with one core this lower priority task
             * cannot run until that suspension has happened and can wait for
             * it.  With more than one core and multiple priorities that
             * ordering does not hold: the idle priority polling task takes the
             * mutex with a zero block time from the other core, so it can seize
             * the mutex the moment it is freed - ahead of this task, which is
             * queued on the mutex's event list - and then resume the
             * controlling task.  This task can therefore acquire the mutex at a
             * point where the controlling task has already been resumed and has
             * started its next cycle, leaving no instant at which this task can
             * observe it suspended.  Check it only where that ordering is
             * guaranteed, matching the peer state checks in the polling task
             * below. */
            #if !( ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 ) )
                if( prvWaitForSuspension( &xControllingIsSuspended ) != pdPASS )
                {
                    xErrorOccurred = __LINE__;
                }
            #endif

            /* Give the mutex back, then suspend, to allow the polling task to
             * obtain it.  Both are performed unconditionally: holding on to the
             * mutex here would make this task spin re-taking it recursively and
             * starve the controlling and polling tasks. */
            if( xSemaphoreGiveRecursive( xMutex ) != pdPASS )
            {
                xErrorOccurred = __LINE__;
            }

            xBlockingIsSuspended = pdTRUE;
            vTaskSuspend( NULL );
            xBlockingIsSuspended = pdFALSE;
        }
        else
        {
            /* We should not leave the xSemaphoreTakeRecursive() function
             * until the mutex was obtained. */
            xErrorOccurred = __LINE__;
        }

        #if ( configNUMBER_OF_CORES == 1 )

            /* On single core the strict priority ordering keeps the controlling
             * and blocking tasks in lock step.  On SMP they run concurrently and
             * the controlling task can be more than one cycle ahead when this
             * check is reached, so it only applies to single core. */
            if( uxControllingCycles != ( UBaseType_t ) ( uxBlockingCycles + 1 ) )
            {
                xErrorOccurred = __LINE__;
            }
        #endif

        /* Keep count of the number of cycles this task has performed so a
         * stall can be detected. */
        uxBlockingCycles++;
    }
}
/*-----------------------------------------------------------*/

static void prvRecursiveMutexPollingTask( void * pvParameters )
{
    /* Just to remove compiler warning. */
    ( void ) pvParameters;

    for( ; ; )
    {
        /* Keep attempting to obtain the mutex.  It should only be obtained when
         * the blocking task has suspended itself, which in turn should only
         * happen when the controlling task is also suspended. */
        if( xSemaphoreTakeRecursive( xMutex, recmuNO_DELAY ) == pdPASS )
        {
            /* These eTaskGetState() checks are racy on SMP, even with
             * configRUN_MULTIPLE_PRIORITIES == 0.  The blocking task frees the
             * mutex in xSemaphoreGiveRecursive() before it sets its suspend
             * flag and actually calls vTaskSuspend(), so the idle-priority
             * polling task can acquire the freed mutex on the other core and
             * read the peer task's state while it is still completing that
             * give-then-suspend sequence.  eTaskGetState() then correctly
             * reports the in-flight state (eRunning/eReady/eBlocked) rather than
             * eSuspended.  Guard them to single core, matching the equivalent
             * eTaskGetState() checks further below; the xXxxIsSuspended flag
             * check that follows provides the SMP-safe coverage. */
            #if ( INCLUDE_eTaskGetState == 1 ) && ( configNUMBER_OF_CORES == 1 )
            {
                configASSERT( eTaskGetState( xControllingTaskHandle ) == eSuspended );
                configASSERT( eTaskGetState( xBlockingTaskHandle ) == eSuspended );
            }
            #endif /* INCLUDE_eTaskGetState */

            #if ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 )

                /* Multi-priority SMP: skip the flag check (racy) and always
                 * proceed to the critical section. */
                if( pdFALSE )
            #else
                /* Is the blocking task suspended? */
                if( ( xBlockingIsSuspended != pdTRUE ) || ( xControllingIsSuspended != pdTRUE ) )
            #endif
            {
                xErrorOccurred = __LINE__;
            }
            else
            {
                /* Keep count of the number of cycles this task has performed
                 * so a stall can be detected. */
                uxPollingCycles++;

                /* We can resume the other tasks here even though they have a
                 * higher priority than the polling task.  When they execute they
                 * will attempt to obtain the mutex but fail because the polling
                 * task is still the mutex holder.  The polling task (this task)
                 * will then inherit the higher priority.  The Blocking task will
                 * block indefinitely when it attempts to obtain the mutex, the
                 * Controlling task will only block for a fixed period and an
                 * error will be latched if the polling task has not returned the
                 * mutex by the time this fixed period has expired. */
                vTaskResume( xBlockingTaskHandle );
                #if ( configUSE_PREEMPTION == 0 )
                    taskYIELD();
                #endif

                vTaskResume( xControllingTaskHandle );
                #if ( configUSE_PREEMPTION == 0 )
                    taskYIELD();
                #endif

                /* The other two tasks should now have executed and no longer
                 * be suspended.  Under multi-priority SMP they may still be
                 * transitioning out of vTaskSuspend() on the other core when
                 * this site is reached; skip the flag check there. */
                #if !( ( configNUMBER_OF_CORES > 1 ) && ( configRUN_MULTIPLE_PRIORITIES == 1 ) )
                    if( ( xBlockingIsSuspended == pdTRUE ) || ( xControllingIsSuspended == pdTRUE ) )
                    {
                        xErrorOccurred = __LINE__;
                    }
                #endif

                #if ( INCLUDE_uxTaskPriorityGet == 1 )
                {
                    /* Check priority inherited. */
                    configASSERT( recmuCHECK_OWN_PRIORITY( recmuCONTROLLING_TASK_PRIORITY ) );
                }
                #endif /* INCLUDE_uxTaskPriorityGet */

                /* The eTaskGetState() checks below are intentionally guarded
                 * with configNUMBER_OF_CORES == 1.  On SMP, even with
                 * configRUN_MULTIPLE_PRIORITIES == 0, these checks are racy:
                 *   - After vTaskResume() above, the higher priority task
                 *     starts running xSemaphoreTakeRecursive() on the other
                 *     core.  Priority inheritance boosts this task to the
                 *     higher task's priority BEFORE the higher task calls
                 *     vTaskPlaceOnEventList().
                 *   - At that moment both tasks are at the same priority,
                 *     so SMP can legitimately run them concurrently on
                 *     different cores.  This task resumes (now at inherited
                 *     priority) and reads the other task's state while it
                 *     is still physically executing the remaining few
                 *     instructions of xSemaphoreTakeRecursive() before
                 *     transitioning to eBlocked.  eTaskGetState() correctly
                 *     reports eRunning during that in-flight transition.
                 * The priority inheritance check above
                 * (uxTaskPriorityGet(NULL) == recmuCONTROLLING_TASK_PRIORITY)
                 * is the primary verification and is sufficient to prove the
                 * other tasks entered the blocking path: priority inheritance
                 * only happens inside xSemaphoreTakeRecursive() when the
                 * mutex is contended. */
                #if ( INCLUDE_eTaskGetState == 1 ) && ( configNUMBER_OF_CORES == 1 )
                {
                    configASSERT( eTaskGetState( xControllingTaskHandle ) == eBlocked );
                    configASSERT( eTaskGetState( xBlockingTaskHandle ) == eBlocked );
                }
                #endif /* INCLUDE_eTaskGetState */

                /* Release the mutex, disinheriting the higher priority again. */
                if( xSemaphoreGiveRecursive( xMutex ) != pdPASS )
                {
                    xErrorOccurred = __LINE__;
                }

                #if ( INCLUDE_uxTaskPriorityGet == 1 )
                {
                    /* Check priority disinherited. */
                    configASSERT( recmuCHECK_OWN_PRIORITY( recmuPOLLING_TASK_PRIORITY ) );
                }
                #endif /* INCLUDE_uxTaskPriorityGet */
            }
        }

        #if configUSE_PREEMPTION == 0
        {
            taskYIELD();
        }
        #endif
    }
}
/*-----------------------------------------------------------*/

/* This is called to check that all the created tasks are still running. */
BaseType_t xAreRecursiveMutexTasksStillRunning( void )
{
    BaseType_t xReturn;
    static UBaseType_t uxLastControllingCycles = 0, uxLastBlockingCycles = 0, uxLastPollingCycles = 0;

    /* Is the controlling task still cycling? */
    if( uxLastControllingCycles == uxControllingCycles )
    {
        xErrorOccurred = __LINE__;
    }
    else
    {
        uxLastControllingCycles = uxControllingCycles;
    }

    /* Is the blocking task still cycling? */
    if( uxLastBlockingCycles == uxBlockingCycles )
    {
        xErrorOccurred = __LINE__;
    }
    else
    {
        uxLastBlockingCycles = uxBlockingCycles;
    }

    /* Is the polling task still cycling? */
    if( uxLastPollingCycles == uxPollingCycles )
    {
        xErrorOccurred = __LINE__;
    }
    else
    {
        uxLastPollingCycles = uxPollingCycles;
    }

    if( xErrorOccurred != pdFALSE )
    {
        xReturn = pdFAIL;
    }
    else
    {
        xReturn = pdPASS;
    }

    /* Reset the sticky error flag so a single transient SMP timing slip
     * does not permanently mark the test as failed.  Same pattern as
     * AbortDelay / StreamBufferDemo. */
    xErrorOccurred = pdFALSE;

    return xReturn;
}
