# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
#
# ESP32 board integration for the FreeRTOS granular-lock performance-comparison
# target tests. Builds one test at a time, selected with -DPERF_TEST=<name>
# (crit_speed | queue_speed | queue_contention | lock_contention_end_to_end).
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(freertos_perf_comp_tests)
