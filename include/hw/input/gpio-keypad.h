/*
 * GPIO-based keypad matrix
 *
 * Copyright 2022 Jean-Baptiste Boric <jblbeurope@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef HW_INPUT_GPIO_KEYPAD_H
#define HW_INPUT_GPIO_KEYPAD_H

#include "hw/sysbus.h"

/* Max number of pins managed by keypad. */
#define GPIO_KEYPAD_NR_PINS (32)

typedef struct GpioKeypadKey {
    unsigned column;
    unsigned row;
    int qcode;
} GpioKeypadKey;

typedef struct GpioKeypadState {
    SysBusDevice parent;

    bool active_low;
    uint32_t num_rows;
    uint32_t num_columns;
    uint32_t num_keys;
    GpioKeypadKey *keys;

    uint32_t input;
    bool keypad_status[GPIO_KEYPAD_NR_PINS][GPIO_KEYPAD_NR_PINS];

    qemu_irq output[GPIO_KEYPAD_NR_PINS];
} GpioKeypadState;

void gpio_keypad_set_keys(DeviceState *dev, const GpioKeypadKey *keys);

#define TYPE_GPIO_KEYPAD "gpio-keypad"
#define GPIO_KEYPAD(obj) \
    OBJECT_CHECK(GpioKeypadState, (obj), TYPE_GPIO_KEYPAD)

#endif /* HW_INPUT_GPIO_KEYPAD_H */
