# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED PERF_TEST)
    set(PERF_TEST "crit_speed")
endif()

set(TESTS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../tests")
set(INC_DIR    "${TESTS_ROOT}/include")

# Map the selected test to its directory (relative to tests/) and entry function.
if(PERF_TEST STREQUAL "crit_speed")
    set(TEST_SUBDIR "smp/performance_comparison/crit_speed")
    set(PERF_ENTRY "vRunCriticalSectionSpeed")
elseif(PERF_TEST STREQUAL "queue_speed")
    set(TEST_SUBDIR "smp/performance_comparison/queue_speed")
    set(PERF_ENTRY "vRunQueueSpeedNonBlocking")
elseif(PERF_TEST STREQUAL "queue_contention")
    set(TEST_SUBDIR "smp/performance_comparison/queue_contention")
    set(PERF_ENTRY "vRunQueueContention")
elseif(PERF_TEST STREQUAL "lock_contention_end_to_end")
    set(TEST_SUBDIR "smp/performance_comparison/lock_contention_end_to_end")
    set(PERF_ENTRY "vRunLockContentionEndToEnd")
elseif(PERF_TEST STREQUAL "scheduler_owner_no_yield")
    set(TEST_SUBDIR "smp/scheduler_owner_no_yield")
    set(PERF_ENTRY "vRunSchedulerOwnerNoYield")
else()
    message(FATAL_ERROR "Unknown PERF_TEST '${PERF_TEST}'")
endif()

idf_component_register(
    SRCS "main.c" "${TESTS_ROOT}/${TEST_SUBDIR}/${PERF_TEST}.c"
    INCLUDE_DIRS "." "${INC_DIR}" "${TESTS_ROOT}/${TEST_SUBDIR}"
    REQUIRES freertos esp_hw_support unity
)

# The upstream target-test files use bare kernel includes ("FreeRTOS.h",
# "task.h", ...), whereas ESP-IDF exposes them under a freertos/ prefix. Add the
# SMP kernel's header directory so the unmodified test sources compile.
idf_build_get_property(idf_path IDF_PATH)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${idf_path}/components/freertos/FreeRTOS-Kernel-SMP/include/freertos")

# Force-include the board config so the framework's TEST_CONFIG_H guard is
# satisfied and the custom test-setting hooks are picked up.
target_compile_options(${COMPONENT_LIB} PRIVATE
    "-include" "${CMAKE_CURRENT_LIST_DIR}/board_test_config.h")

target_compile_definitions(${COMPONENT_LIB} PRIVATE "PERF_ENTRY_FN=${PERF_ENTRY}")

message(STATUS "Perf-comp board: building test '${PERF_TEST}' (entry ${PERF_ENTRY})")
