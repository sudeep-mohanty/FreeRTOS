/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#ifdef configRUN_MULTIPLE_PRIORITIES
    #undef configRUN_MULTIPLE_PRIORITIES
#endif /* ifdef configRUN_MULTIPLE_PRIORITIES */

#define configRUN_MULTIPLE_PRIORITIES    1

#endif /* ifndef TEST_CONFIG_H */
