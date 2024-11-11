/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#define EVENT_SUBCORE_READY (1 << 0)

#define EVENT_MAINCORE_EVENT   0x00000001
#define EVENT_SUBCORE_EVENT_1  0x00001010 /* test wait for all */
#define EVENT_SUBCORE_EVENT_2  0x00000101 /* test wait for any */
