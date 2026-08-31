# freertos_demo: vendors the FreeRTOS Common/Minimal test sources into
# the ESP-IDF component system.
set(COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../Common")

set(COMMON_MINIMAL_SRCS
    "${COMMON_DIR}/Minimal/AbortDelay.c"
    "${COMMON_DIR}/Minimal/BlockQ.c"
    "${COMMON_DIR}/Minimal/blocktim.c"
    "${COMMON_DIR}/Minimal/countsem.c"
    "${COMMON_DIR}/Minimal/death.c"
    "${COMMON_DIR}/Minimal/dynamic.c"
    "${COMMON_DIR}/Minimal/EventGroupsDemo.c"
    "${COMMON_DIR}/Minimal/flop.c"
    "${COMMON_DIR}/Minimal/GenQTest.c"
    "${COMMON_DIR}/Minimal/integer.c"
    "${COMMON_DIR}/Minimal/IntQueue.c"
    "${COMMON_DIR}/Minimal/IntSemTest.c"
    "${COMMON_DIR}/Minimal/MessageBufferAMP.c"
    "${COMMON_DIR}/Minimal/MessageBufferDemo.c"
    "${COMMON_DIR}/Minimal/PollQ.c"
    "${COMMON_DIR}/Minimal/QPeek.c"
    "${COMMON_DIR}/Minimal/QueueOverwrite.c"
    "${COMMON_DIR}/Minimal/QueueSet.c"
    "${COMMON_DIR}/Minimal/QueueSetPolling.c"
    "${COMMON_DIR}/Minimal/recmutex.c"
    "${COMMON_DIR}/Minimal/semtest.c"
    "${COMMON_DIR}/Minimal/StaticAllocation.c"
    "${COMMON_DIR}/Minimal/StreamBufferDemo.c"
    "${COMMON_DIR}/Minimal/StreamBufferInterrupt.c"
    "${COMMON_DIR}/Minimal/TaskNotify.c"
    "${COMMON_DIR}/Minimal/TimerDemo.c"
)

idf_component_register(
    SRCS ${COMMON_MINIMAL_SRCS}
    INCLUDE_DIRS "${COMMON_DIR}/include" "${CMAKE_CURRENT_LIST_DIR}"
    PRIV_REQUIRES freertos
)

# Common/Minimal includes kernel headers by bare name (#include "FreeRTOS.h")
# whereas ESP-IDF exposes them under a freertos/ prefix.  Add the kernel's
# include/freertos directory so the bare includes resolve.
idf_component_get_property(freertos_dir freertos COMPONENT_DIR)
if(CONFIG_FREERTOS_SMP)
    set(_kernel_subdir "FreeRTOS-Kernel-SMP")
else()
    set(_kernel_subdir "FreeRTOS-Kernel")
endif()
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${freertos_dir}/${_kernel_subdir}/include/freertos"
)

# Suppress warnings noise from the Common/Minimal sources so the demo's
# own code stays under -Werror.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=unused-parameter
    -Wno-error=unused-variable
    -Wno-error=unused-but-set-variable
    -Wno-error=format
    -Wno-error=sign-compare
    -Wno-error=missing-field-initializers
    -Wno-error=implicit-function-declaration
)

# StreamBufferDemo's prvInterruptTriggerLevelTest expects sub-tick-precise
# receiver wakeup; allow a small tolerance to absorb dispatch jitter.
#
# TimerDemo's prvTest1_CreateTimersWithoutSchedulerRunning assumes
# vStartTimerDemoTask runs before vTaskStartScheduler so the timer command
# queue stays full long enough for the (N+1)th xTimerStart to observe pdFAIL.
# On ESP-IDF the scheduler is already running, so Tmr Svc drains the queue
# and the (N+1)th xTimerStart returns pdPASS, asserting in the upstream test.
# Disable just that sub-check; the rest of prvTest1 + Test2-Test7 still run.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    configSTREAM_BUFFER_TRIGGER_LEVEL_TEST_MARGIN=4
    tmrdemoRUN_QUEUE_FULL_CHECK=0
)
