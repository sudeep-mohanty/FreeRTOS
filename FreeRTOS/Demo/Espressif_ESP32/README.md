# FreeRTOS Common/Minimal Demo on Espressif Targets

A port of the FreeRTOS `Common/Minimal` test bar to ESP-IDF.  A Check
task polls each demo task's `xAreXxxStillRunning()` probe on a fixed
cycle and reports per-test results in a Unity-style format.

## Quickstart

```sh
. $IDF_PATH/export.sh
idf.py set-target esp32 build flash monitor
```

Per-cycle output:

```
----- Cycle 1 (tick 10032) -----
test_MessageBuffer:RUNNING
test_MessageBuffer:PASS
test_TaskNotification:RUNNING
test_TaskNotification:PASS
...
test_AbortDelay:RUNNING
test_AbortDelay:IGNORED
Cycle 1: 18 PASS  0 FAIL  7 IGNORED
```

After the configured run duration, a final summary is printed:

```
-----------------------
Demo run complete: 18 cycles, 180 s.

Per-test totals (PASS / FAIL / IGNORED over 18 cycles):
  test_MessageBuffer            18 /     0 /     0
  ...
  test_AbortDelay                0 /     0 /    18

450 Tests 0 Failures 126 Ignored
OK
-----------------------
```

## Compile-time knobs

| Macro                  | Default | Effect                                          |
| ---------------------- | ------- | ----------------------------------------------- |
| `mainDEMO_DURATION_S`  | `180`   | Total runtime in seconds.  `0` runs unbounded.  |

Override via `EXTRA_CFLAGS`:

```sh
idf.py build -DEXTRA_CFLAGS="-DmainDEMO_DURATION_S=3600"   # 1 hour
idf.py build -DEXTRA_CFLAGS="-DmainDEMO_DURATION_S=0"      # forever
```

## Config sweep

Four sweep configs, selected with `-DSDKCONFIG_DEFAULTS`:

```sh
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci.smp_single_prio" \
       set-target esp32 build
```

| Config                          | Coverage                                              |
| ------------------------------- | ----------------------------------------------------- |
| `sdkconfig.ci.smp_single_prio`  | Primary config.  Dual-core, single-priority.          |
| `sdkconfig.ci.smp_multi_prio`   | Dual-core, multi-priority scheduling.                 |
| `sdkconfig.ci.smp_unicore`      | Single-core mode.                                     |
| `sdkconfig.ci.idf`              | Baseline.                                             |

`sdkconfig.defaults` selects the primary config by default.

## Multi-target support

Add an SoC-specific overrides file alongside `sdkconfig.defaults.esp32`:

```
sdkconfig.defaults.esp32s3
sdkconfig.defaults.esp32p4
...
```

`idf.py set-target <target>` picks up the matching one.

## Ignored tests

Several `Common/Minimal` tests are reported as `IGNORED` rather than
exercised on this port.  The per-test rationale is documented inline in
`main/main_full.c` next to each excluded `vStartXxx()` call.

## Pass criterion

The final summary line must end with `OK`.  Any `FAIL` in any cycle is
reported in the per-test totals and flips the verdict to `FAILED`.
