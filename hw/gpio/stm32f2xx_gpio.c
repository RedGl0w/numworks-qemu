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

#include "qemu/osdep.h"

#include "hw/gpio/stm32f2xx_gpio.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "trace.h"

#define STM32F2XX_GPIO_REGS_SIZE (1 * KiB)

/* 32-bit register indices. */
enum STM32F2xxGPIORegister {
    STM32F2XX_GPIO_MODE,
    STM32F2XX_GPIO_OTYPE,
    STM32F2XX_GPIO_OSPEED,
    STM32F2XX_GPIO_PUPD,
    STM32F2XX_GPIO_IDR,
    STM32F2XX_GPIO_ODR,
    STM32F2XX_GPIO_BSR,
    STM32F2XX_GPIO_LCK,
    STM32F2XX_GPIO_AFRL,
    STM32F2XX_GPIO_AFRH,
    STM32F2XX_GPIO_REGS_END,
};

static void stm32f2xx_gpio_update_pins(STM32F2xxGpioState *s, uint16_t diff)
{
    int i;

    for (i = 0; i < STM32F2XX_GPIO_NR_PINS; i++) {
        if (diff & (1u << i)) {
            bool value = s->odr & (1u << i);
            trace_stm32f2xx_gpio_update_pins(DEVICE(s)->canonical_path, i, value);
            qemu_set_irq(s->output[i], value);
        }
    }
}

static uint64_t stm32f2xx_gpio_regs_read(void *opaque, hwaddr addr,
                                       unsigned int size)
{
    hwaddr reg = addr / sizeof(uint32_t);
    STM32F2xxGpioState *s = opaque;
    uint64_t value = 0;

    switch (reg) {
    case STM32F2XX_GPIO_MODE:
        value = s->mode;
        break;
    case STM32F2XX_GPIO_OTYPE:
        value = s->mode;
        break;
    case STM32F2XX_GPIO_OSPEED:
        value = s->ospeed;
        break;
    case STM32F2XX_GPIO_PUPD:
        value = s->pupd;
        break;
    case STM32F2XX_GPIO_IDR:
        value = s->idr;
        break;
    case STM32F2XX_GPIO_ODR:
        value = s->odr;
        break;
    case STM32F2XX_GPIO_AFRL:
        value = s->afrl;
        break;
    case STM32F2XX_GPIO_AFRH:
        value = s->afrh;
        break;

    case STM32F2XX_GPIO_BSR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: read from write-only register 0x%" HWADDR_PRIx "\n",
                      DEVICE(s)->canonical_path, addr);
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: read from invalid offset 0x%" HWADDR_PRIx "\n",
                      DEVICE(s)->canonical_path, addr);
        break;
    }

    trace_stm32f2xx_gpio_read(DEVICE(s)->canonical_path, addr, value);

    return value;
}

static void stm32f2xx_gpio_regs_write(void *opaque, hwaddr addr, uint64_t v,
                                    unsigned int size)
{
    hwaddr reg = addr / sizeof(uint32_t);
    STM32F2xxGpioState *s = opaque;
    uint32_t value = v;
    uint16_t diff;

    trace_stm32f2xx_gpio_write(DEVICE(s)->canonical_path, addr, v);

    switch (reg) {
    case STM32F2XX_GPIO_ODR:
        diff = s->odr ^ value;
        s->odr = value;
        stm32f2xx_gpio_update_pins(s, diff);
        break;
    case STM32F2XX_GPIO_BSR:
        diff = value | (value >> 16);
        s->odr &= (~value >> 16);
        s->odr |= value;
        stm32f2xx_gpio_update_pins(s, diff);
        break;

    case STM32F2XX_GPIO_IDR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: write to read-only register @ 0x%" HWADDR_PRIx "\n",
                      DEVICE(s)->canonical_path, addr);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: write to invalid offset 0x%" HWADDR_PRIx "\n",
                      DEVICE(s)->canonical_path, addr);
        break;
    }
}

static const MemoryRegionOps stm32f2xx_gpio_regs_ops = {
    .read = stm32f2xx_gpio_regs_read,
    .write = stm32f2xx_gpio_regs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 2,
        .max_access_size = 4,
        .unaligned = false,
    },
};

static void stm32f2xx_gpio_set_input(void *opaque, int n, int level)
{
    STM32F2xxGpioState *s = opaque;

    s->idr &= ~(1u << n);
    if (level) {
        s->idr |= 1u << n;
    }
}

static void stm32f2xx_gpio_enter_reset(Object *obj, ResetType type)
{
    STM32F2xxGpioState *s = STM32F2XX_GPIO(obj);

    s->mode = s->reset_mode;
    s->otype = 0x0000;
    s->ospeed = s->reset_ospeed;
    s->pupd = s->reset_pupd;
    s->idr = 0x0000;
    s->odr = 0x0000;
    s->afrl = 0x00000000;
    s->afrh = 0x00000000;
}

static void stm32f2xx_gpio_hold_reset(Object *obj)
{
    STM32F2xxGpioState *s = STM32F2XX_GPIO(obj);

    stm32f2xx_gpio_update_pins(s, 0xFFFF);
}

static void stm32f2xx_gpio_init(Object *obj)
{
    STM32F2xxGpioState *s = STM32F2XX_GPIO(obj);
    DeviceState *dev = DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_gpio_regs_ops, s,
                          "regs", STM32F2XX_GPIO_REGS_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    qdev_init_gpio_in(dev, stm32f2xx_gpio_set_input, STM32F2XX_GPIO_NR_PINS);
    qdev_init_gpio_out(dev, s->output, STM32F2XX_GPIO_NR_PINS);
}

static Property stm32f2xx_gpio_properties[] = {
    DEFINE_PROP_UINT32("reset-mode", STM32F2xxGpioState, reset_mode, 0),
    DEFINE_PROP_UINT32("reset-ospeed", STM32F2xxGpioState, reset_ospeed, 0),
    DEFINE_PROP_UINT32("reset-pupd", STM32F2xxGpioState, reset_pupd, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f2xx_gpio_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *reset = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "STM32F2xx GPIO Controller";
    reset->phases.enter = stm32f2xx_gpio_enter_reset;
    reset->phases.hold = stm32f2xx_gpio_hold_reset;
    device_class_set_props(dc, stm32f2xx_gpio_properties);
}

static const TypeInfo stm32f2xx_gpio_types[] = {
    {
        .name = TYPE_STM32F2XX_GPIO,
        .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(STM32F2xxGpioState),
        .class_init = stm32f2xx_gpio_class_init,
        .instance_init = stm32f2xx_gpio_init,
    },
};

DEFINE_TYPES(stm32f2xx_gpio_types);
