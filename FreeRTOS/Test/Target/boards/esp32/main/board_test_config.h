/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Force-included (via -include) into every translation unit of the ESP32
 * perf-comparison board. ESP-IDF generates its FreeRTOSConfig from Kconfig, so
 * instead of appending the per-test test_config.h to FreeRTOSConfig.h (as the
 * framework does on bare-metal targets), the required kernel options are set
 * through sdkconfig (e.g. CONFIG_FREERTOS_RUN_MULTIPLE_PRIORITIES) and this
 * header only satisfies the framework's include-order contract:
 *   - defines TEST_CONFIG_H so the "test_config.h must be included" guard passes
 *   - selects the custom (non-Unity) test-setting framework
 */

#ifndef BOARD_TEST_CONFIG_H
#define BOARD_TEST_CONFIG_H

#define TEST_CONFIG_H
#define configTARGET_TEST_USE_CUSTOM_SETTING    1

#endif /* BOARD_TEST_CONFIG_H */
