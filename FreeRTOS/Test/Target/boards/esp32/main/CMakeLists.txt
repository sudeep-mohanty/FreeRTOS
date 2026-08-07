# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
#
# Generic runner for the FreeRTOS Test/Target tests on ESP32.
#
# Select a test with -DTEST=<name> (or the legacy -DPERF_TEST=<name>). The test
# source is discovered automatically as any <name>/<name>.c under tests/, and its
# entry function is read from the testENTRY_FUNCTION_PROTOTYPE( <fxn> ) line - so
# adding a new test needs no changes here.

if(NOT DEFINED TEST)
    if(DEFINED PERF_TEST)          # backwards-compatible alias
        set(TEST "${PERF_TEST}")
    else()
        set(TEST "crit_speed")
    endif()
endif()

set(TESTS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../tests")
set(INC_DIR    "${TESTS_ROOT}/include")

# Locate <name>/<name>.c anywhere under tests/.
file(GLOB_RECURSE _all_test_c LIST_DIRECTORIES false "${TESTS_ROOT}/*.c")
set(TEST_SRC "")
foreach(_c ${_all_test_c})
    get_filename_component(_fn  "${_c}" NAME_WE)
    get_filename_component(_dir "${_c}" DIRECTORY)
    get_filename_component(_dn  "${_dir}" NAME)
    if(_fn STREQUAL "${TEST}" AND _dn STREQUAL "${TEST}")
        set(TEST_SRC "${_c}")
        set(TEST_DIR "${_dir}")
        break()
    endif()
endforeach()
if(NOT TEST_SRC)
    message(FATAL_ERROR "No test '<dir>/${TEST}/${TEST}.c' found under ${TESTS_ROOT}")
endif()

# Read the entry function from testENTRY_FUNCTION_PROTOTYPE( <fxn> ).
file(READ "${TEST_SRC}" _test_src)
string(REGEX MATCH
       "testENTRY_FUNCTION_PROTOTYPE[ \t\r\n]*\\([ \t\r\n]*([A-Za-z_][A-Za-z0-9_]*)"
       _unused "${_test_src}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "No testENTRY_FUNCTION_PROTOTYPE(...) in ${TEST_SRC}")
endif()
set(TEST_ENTRY "${CMAKE_MATCH_1}")

idf_component_register(
    SRCS "main.c" "${TEST_SRC}"
    INCLUDE_DIRS "." "${INC_DIR}" "${TEST_DIR}"
    REQUIRES freertos esp_hw_support unity
)

# The upstream test files use bare kernel includes ("FreeRTOS.h", "task.h", ...),
# whereas ESP-IDF exposes them under a freertos/ prefix. Add the SMP kernel's
# header directory so the unmodified test sources compile.
idf_build_get_property(idf_path IDF_PATH)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${idf_path}/components/freertos/FreeRTOS-Kernel-SMP/include/freertos")

# Force-include the board config so the framework's TEST_CONFIG_H guard is
# satisfied and the custom test-setting hooks are picked up.
target_compile_options(${COMPONENT_LIB} PRIVATE
    "-include" "${CMAKE_CURRENT_LIST_DIR}/board_test_config.h")

target_compile_definitions(${COMPONENT_LIB} PRIVATE "TEST_ENTRY_FN=${TEST_ENTRY}")

message(STATUS "Target test '${TEST}': ${TEST_SRC} (entry ${TEST_ENTRY})")
