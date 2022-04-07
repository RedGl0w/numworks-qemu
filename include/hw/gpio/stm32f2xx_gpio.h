/*
 * STM32F2xx GPIO
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

#ifndef STM32F2XX_GPIO_H
#define STM32F2XX_GPIO_H

#include "exec/memory.h"
#include "hw/sysbus.h"

/* Number of pins managed by each controller. */
#define STM32F2XX_GPIO_NR_PINS (16)

typedef struct STM32F2xxGpioState {
    SysBusDevice parent;

    uint32_t mode;
    uint16_t otype;
    uint32_t ospeed;
    uint32_t pupd;
    uint16_t idr;
    uint16_t odr;
    uint32_t afrl;
    uint32_t afrh;

    uint32_t reset_mode;
    uint32_t reset_ospeed;
    uint32_t reset_pupd;

    MemoryRegion mmio;
    qemu_irq output[STM32F2XX_GPIO_NR_PINS];
} STM32F2xxGpioState;

#define TYPE_STM32F2XX_GPIO "stm32f2xx-gpio"
#define STM32F2XX_GPIO(obj) \
    OBJECT_CHECK(STM32F2xxGpioState, (obj), TYPE_STM32F2XX_GPIO)

#endif /* STM32F2XX_H */
