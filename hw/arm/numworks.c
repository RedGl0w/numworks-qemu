/*
 * NumWorks series of calculators
 *
 * Copyright (c) 2022 Jean-Baptiste Boric <jblbeurope@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qapi/qapi-types-ui.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "qemu/error-report.h"
#include "hw/arm/stm32f4xx_soc.h"
#include "hw/arm/boot.h"
#include "hw/input/gpio-keypad.h"
#include "hw/display/st7789v.h"

/* Main SYSCLK frequency in Hz (100MHz) */
#define SYSCLK_FRQ 100000000ULL

#define ST7789V_ADD 0x60000000

static const GpioKeypadKey n0100_keys[] = {
    { 0, 0, Q_KEY_CODE_LEFT },
    { 1, 0, Q_KEY_CODE_UP },
    { 2, 0, Q_KEY_CODE_DOWN },
    { 3, 0, Q_KEY_CODE_RIGHT },
    { 4, 0, Q_KEY_CODE_RET },
    { 5, 0, Q_KEY_CODE_ESC },

    { 0, 1, Q_KEY_CODE_HOME },
    { 1, 1, Q_KEY_CODE_END },

    { 0, 2, Q_KEY_CODE_SHIFT },
    { 1, 2, Q_KEY_CODE_ALT },
    { 2, 2, Q_KEY_CODE_3 },
    { 3, 2, Q_KEY_CODE_4 },
    { 4, 2, Q_KEY_CODE_TAB },
    { 5, 2, Q_KEY_CODE_BACKSPACE },

    { 0, 3, Q_KEY_CODE_A },
    { 1, 3, Q_KEY_CODE_B },
    { 2, 3, Q_KEY_CODE_C },
    { 3, 3, Q_KEY_CODE_D },
    { 4, 3, Q_KEY_CODE_E },
    { 5, 3, Q_KEY_CODE_F },

    { 0, 4, Q_KEY_CODE_G },
    { 1, 4, Q_KEY_CODE_H },
    { 2, 4, Q_KEY_CODE_I },
    { 3, 4, Q_KEY_CODE_J },
    { 4, 4, Q_KEY_CODE_K },
    { 5, 4, Q_KEY_CODE_L },

    { 0, 5, Q_KEY_CODE_M }, { 0, 5, Q_KEY_CODE_KP_7 },
    { 1, 5, Q_KEY_CODE_N }, { 1, 5, Q_KEY_CODE_KP_8 },
    { 2, 5, Q_KEY_CODE_O }, { 2, 5, Q_KEY_CODE_KP_9 },
    { 3, 5, Q_KEY_CODE_P },
    { 4, 5, Q_KEY_CODE_Q },

    { 0, 6, Q_KEY_CODE_R }, { 0, 6, Q_KEY_CODE_KP_4 },
    { 1, 6, Q_KEY_CODE_S }, { 1, 6, Q_KEY_CODE_KP_5 },
    { 2, 6, Q_KEY_CODE_T }, { 2, 6, Q_KEY_CODE_KP_6 },
    { 3, 6, Q_KEY_CODE_U }, { 3, 6, Q_KEY_CODE_KP_MULTIPLY },
    { 4, 6, Q_KEY_CODE_V }, { 4, 6, Q_KEY_CODE_KP_DIVIDE },

    { 0, 7, Q_KEY_CODE_W }, { 0, 7, Q_KEY_CODE_KP_1 },
    { 1, 7, Q_KEY_CODE_X }, { 1, 7, Q_KEY_CODE_KP_2 },
    { 2, 7, Q_KEY_CODE_Y }, { 2, 7, Q_KEY_CODE_KP_3 },
    { 3, 7, Q_KEY_CODE_Z }, { 3, 7, Q_KEY_CODE_KP_ADD },
    { 4, 7, Q_KEY_CODE_SPC }, { 4, 7, Q_KEY_CODE_KP_SUBTRACT },

    { 0, 8, Q_KEY_CODE_KP_0 },
    { 1, 8, Q_KEY_CODE_KP_DECIMAL },
    { 2, 8, Q_KEY_CODE_9 },
    { 3, 8, Q_KEY_CODE_0 },
    { 4, 8, Q_KEY_CODE_KP_ENTER },

    { 0, 0, Q_KEY_CODE_UNMAPPED },
};

static void n0100_init(MachineState *machine)
{
    DeviceState *soc;
    DeviceState *gpio;
    DeviceState *dev;
    Clock *sysclk;
    int i;

    /* This clock doesn't need migration because it is fixed-frequency */
    sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(sysclk, SYSCLK_FRQ);

    soc = qdev_new(TYPE_STM32F4XX_SOC);
    qdev_prop_set_string(soc, "soc-type", VARIANT_STM32F412_SOC);
    qdev_prop_set_uint32(DEVICE(&STM32F4XX_SOC(soc)->adc[0]), "value", 0xFFF);
    qdev_connect_clock_in(soc, "sysclk", sysclk);
    sysbus_realize(SYS_BUS_DEVICE(soc), &error_fatal);

    dev = qdev_new(TYPE_ST7789V);
    qdev_prop_set_bit(dev, "rotate-right", true);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, ST7789V_ADD);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    gpio = qdev_new(TYPE_GPIO_KEYPAD);
    qdev_prop_set_bit(gpio, "active-low", true);
    qdev_prop_set_uint32(gpio, "num-columns", 6);
    qdev_prop_set_uint32(gpio, "num-rows", 9);
    gpio_keypad_set_keys(gpio, n0100_keys);
    sysbus_realize(SYS_BUS_DEVICE(gpio), &error_fatal);
    for (i = 0; i < 9; i++) {
        qdev_connect_gpio_out_named(DEVICE(soc), "gpio-e-out", i,
                                    qdev_get_gpio_in(gpio, i));
    }
    for (i = 0; i < 6; i++) {
        qdev_connect_gpio_out(DEVICE(gpio), i,
                              qdev_get_gpio_in_named(soc, "gpio-c", i));
    }
    object_unref(OBJECT(gpio));

    object_unref(OBJECT(soc));

    armv7m_load_kernel(ARM_CPU(first_cpu),
                       machine->kernel_filename,
                       STM32F412_SOC_FLASH_SIZE);
}

static void n0100_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NumWorks N0100 calculator (Cortex-M4)";
    mc->init = n0100_init;
}

static const TypeInfo numworks_machine_types[] = {
    {
        .name           = MACHINE_TYPE_NAME("n0100"),
        .parent         = TYPE_MACHINE,
        .class_init     = n0100_machine_class_init,
    },
};

DEFINE_TYPES(numworks_machine_types)
