/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file must be included at the end of the FreeRTOSConfig.h. It contains
 * any FreeRTOS specific configurations that the test requires. */

#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#ifdef configRUN_MULTIPLE_PRIORITIES
    #undef configRUN_MULTIPLE_PRIORITIES
#endif
#define configRUN_MULTIPLE_PRIORITIES    1

#ifdef configUSE_CORE_AFFINITY
    #undef configUSE_CORE_AFFINITY
#endif
#define configUSE_CORE_AFFINITY    1

#ifdef configUSE_TASK_PREEMPTION_DISABLE
    #undef configUSE_TASK_PREEMPTION_DISABLE
#endif
#define configUSE_TASK_PREEMPTION_DISABLE    1

#endif /* ifndef TEST_CONFIG_H */
