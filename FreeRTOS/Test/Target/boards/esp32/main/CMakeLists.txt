# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED PERF_TEST)
    set(PERF_TEST "crit_speed")
endif()

set(TESTS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../tests/smp/performance_comparison")
set(INC_DIR   "${CMAKE_CURRENT_LIST_DIR}/../../../tests/include")

# Map the selected test to its entry function.
if(PERF_TEST STREQUAL "crit_speed")
    set(PERF_ENTRY "vRunCriticalSectionSpeed")
elseif(PERF_TEST STREQUAL "queue_speed")
    set(PERF_ENTRY "vRunQueueSpeedNonBlocking")
elseif(PERF_TEST STREQUAL "queue_contention")
    set(PERF_ENTRY "vRunQueueContention")
elseif(PERF_TEST STREQUAL "lock_contention_end_to_end")
    set(PERF_ENTRY "vRunLockContentionEndToEnd")
else()
    message(FATAL_ERROR "Unknown PERF_TEST '${PERF_TEST}'")
endif()

idf_component_register(
    SRCS "main.c" "${TESTS_DIR}/${PERF_TEST}/${PERF_TEST}.c"
    INCLUDE_DIRS "." "${INC_DIR}" "${TESTS_DIR}/${PERF_TEST}"
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
