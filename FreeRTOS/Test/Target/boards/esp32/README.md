# ESP32 board — granular-lock performance-comparison tests

An ESP-IDF integration of the FreeRTOS `Test/Target` performance-comparison
tests (`crit_speed`, `queue_speed`, `queue_contention`,
`lock_contention_end_to_end`) for the dual-core ESP32.

Unlike bare-metal targets that append the per-test `test_config.h` to
`FreeRTOSConfig.h`, ESP-IDF generates its kernel configuration from Kconfig, so:

- The required kernel options are set through `sdkconfig.defaults`
  (`CONFIG_FREERTOS_SMP`, `CONFIG_FREERTOS_RUN_MULTIPLE_PRIORITIES`, ...).
- `main/board_test_config.h` is force-included into every TU to satisfy the
  framework's include-order contract (`TEST_CONFIG_H`,
  `configTARGET_TEST_USE_CUSTOM_SETTING`).
- `main/test_setting_config.h` provides the two ESP32 hooks:
  `testGET_TIME_FUNCTION()` (the CPU cycle counter) and `testPRINTF_FUNCTION`.

The framework builds one test per binary, so the test is selected at build time.

## Build & run

```bash
. $IDF_PATH/export.sh
cd FreeRTOS/Test/Target/boards/esp32

# PERF_TEST in {crit_speed, queue_speed, queue_contention, lock_contention_end_to_end}
idf.py -B build_crit_speed -DIDF_TARGET=esp32 -DPERF_TEST=crit_speed \
       -DSDKCONFIG=build_crit_speed/sdkconfig build
idf.py -B build_crit_speed -p <PORT> flash monitor
```

## Granular vs. non-granular

The tests select the critical-section API via `portUSING_GRANULAR_LOCKS`, which
the ESP-IDF SMP port ties to `configNUMBER_OF_CORES > 1`. To measure the
non-granular baseline on the same two-core kernel, temporarily force
`portUSING_GRANULAR_LOCKS` to `0` in the port's `portmacro.h` and rebuild.
