/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ESP32 test-setting overrides for the target test framework. Included by every
 * test file when configTARGET_TEST_USE_CUSTOM_SETTING == 1, before
 * test_default_setting_config.h. Only the two target hooks are overridden; the
 * default (Unity-based) harness macros are kept, since some tests use Unity
 * assertions (TEST_ASSERT_*).
 */

#ifndef TEST_SETTING_CONFIG_H
#define TEST_SETTING_CONFIG_H

#include <stdio.h>
#include "esp_cpu.h"

/* High-resolution timer: the CPU cycle counter. */
#define testGET_TIME_FUNCTION()    ( ( UBaseType_t ) esp_cpu_get_cycle_count() )

/* Output. */
#define testPRINTF_FUNCTION        printf

#endif /* TEST_SETTING_CONFIG_H */
