# ESP32 board — FreeRTOS Test/Target tests

An ESP-IDF integration that can build and run any `Test/Target` SMP test on the
dual-core ESP32 (the granular-lock performance-comparison tests, the functional
SMP tests, and the scheduler-owner regression test).

## How it works

Unlike bare-metal targets that append the per-test `test_config.h` to
`FreeRTOSConfig.h`, ESP-IDF generates its kernel configuration from Kconfig, so:

- Kernel options are set through `sdkconfig.defaults` (`CONFIG_FREERTOS_SMP`,
  `CONFIG_FREERTOS_RUN_MULTIPLE_PRIORITIES`, ...).
- `main/board_test_config.h` is force-included into every TU to satisfy the
  framework's include-order contract (`TEST_CONFIG_H`,
  `configTARGET_TEST_USE_CUSTOM_SETTING`) and to provide an `XT_NOP()` fallback
  used by tests that busy-loop with `portNOP()`.
- `main/test_setting_config.h` supplies the ESP32 hooks:
  `testGET_TIME_FUNCTION()` (CPU cycle counter) and `testPRINTF_FUNCTION`.

The `main/CMakeLists.txt` is **generic**: it discovers the test source as any
`<name>/<name>.c` under `tests/` and reads its entry function from the
`testENTRY_FUNCTION_PROTOTYPE( ... )` line - adding a new test needs no changes.

## Build & run

One binary per test, selected with `-DTEST=<name>` (legacy `-DPERF_TEST=` also
works):

```bash
. $IDF_PATH/export.sh
cd FreeRTOS/Test/Target/boards/esp32

idf.py -B build_suspend_scheduler -DIDF_TARGET=esp32 -DTEST=suspend_scheduler \
       -DSDKCONFIG=build_suspend_scheduler/sdkconfig build
idf.py -B build_suspend_scheduler -p <PORT> flash monitor
```

`<name>` is any test dir, e.g. `crit_speed`, `queue_speed`, `queue_contention`,
`lock_contention_end_to_end`, `scheduler_owner_no_yield`, `disable_preemption`,
`suspend_scheduler`, `task_delete`, `schedule_affinity`, ...

## ESP-IDF compatibility notes

ESP-IDF only lets `configRUN_MULTIPLE_PRIORITIES` be configured (via
`CONFIG_FREERTOS_RUN_MULTIPLE_PRIORITIES`); `configUSE_PREEMPTION`,
`configUSE_TIME_SLICING`, `configUSE_CORE_AFFINITY` and
`configUSE_TASK_PREEMPTION_DISABLE` are fixed to `1`. Therefore:

- Tests requiring `configRUN_MULTIPLE_PRIORITIES=0` use the
  `sdkconfig.no_multi_prio` fragment
  (`-DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.no_multi_prio"`).
- `disable_multiple_priorities` cannot build (requires `configUSE_CORE_AFFINITY=0`).
- `interrupt_wait_critical` fails: it expects a task spinning to enter
  `taskENTER_CRITICAL()` to be preempted before acquiring the lock, but the
  ESP-IDF port disables interrupts before spinning, so the entry is not
  preemptible - a port-behavior difference, not a test bug.

## Granular vs. non-granular

The tests select the critical-section API via `portUSING_GRANULAR_LOCKS`, which
the ESP-IDF SMP port ties to `configNUMBER_OF_CORES > 1`. To measure the
non-granular baseline on the same two-core kernel, temporarily force
`portUSING_GRANULAR_LOCKS` to `0` in the port's `portmacro.h` and rebuild.
