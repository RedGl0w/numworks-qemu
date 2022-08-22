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
#include "hw/arm/stm32f730_soc.h"
#include "hw/arm/boot.h"
#include "hw/input/gpio-keypad.h"
#include "hw/display/st7789v.h"
#include "hw/arm/numworks.h"
#include "include/exec/address-spaces.h"

/* Main SYSCLK frequency in Hz (100MHz) */
#define SYSCLK_FRQ 100000000ULL

#define ST7789V_ADD 0x60000000

static const GpioKeypadKey numworks_keys[] = { // FIX ME : This isn't shared by both models
    { 0, 0, Q_KEY_CODE_LEFT }, // Key::Left
    { 1, 0, Q_KEY_CODE_UP }, // Key::Up
    { 2, 0, Q_KEY_CODE_DOWN }, // Key::Down
    { 3, 0, Q_KEY_CODE_RIGHT }, // Key::Right
    { 4, 0, Q_KEY_CODE_RET }, // Key::Ok
    { 5, 0, Q_KEY_CODE_ESC }, // Key::Back

    { 0, 1, Q_KEY_CODE_HOME }, // Key::Home
    { 1, 1, Q_KEY_CODE_END }, // Key::OnOff

    { 0, 2, Q_KEY_CODE_SHIFT }, // Key::Shift
    { 1, 2, Q_KEY_CODE_ALT }, // Key::Alpha
    { 2, 2, Q_KEY_CODE_3 }, // Key::XNT
    { 3, 2, Q_KEY_CODE_4 }, // Key::Var
    { 4, 2, Q_KEY_CODE_TAB }, // Key::Toolbox
    { 5, 2, Q_KEY_CODE_BACKSPACE }, // Key::Backspace

    { 0, 3, Q_KEY_CODE_A }, // Key::Exp
    { 1, 3, Q_KEY_CODE_B }, // Key::Ln
    { 2, 3, Q_KEY_CODE_C }, // Key::Log
    { 3, 3, Q_KEY_CODE_D }, // Key::Imaginary
    { 4, 3, Q_KEY_CODE_E }, // Key::Comma
    { 5, 3, Q_KEY_CODE_F }, // Key::Power

    { 0, 4, Q_KEY_CODE_G }, // Key::Sine
    { 1, 4, Q_KEY_CODE_H }, // Key::Cosine
    { 2, 4, Q_KEY_CODE_I }, // Key::Tangent
    { 3, 4, Q_KEY_CODE_J }, // Key::Pi
    { 4, 4, Q_KEY_CODE_K }, // Key::Sqrt
    { 5, 4, Q_KEY_CODE_L }, // Key::Square

    { 0, 5, Q_KEY_CODE_M }, { 0, 5, Q_KEY_CODE_KP_7 }, // Key::Seven
    { 1, 5, Q_KEY_CODE_N }, { 1, 5, Q_KEY_CODE_KP_8 }, // Key::Eight
    { 2, 5, Q_KEY_CODE_O }, { 2, 5, Q_KEY_CODE_KP_9 }, // Key::Nine
    { 3, 5, Q_KEY_CODE_P }, // Key::LeftParenthesis
    { 4, 5, Q_KEY_CODE_Q }, // Key::RightParenthesis

    { 0, 6, Q_KEY_CODE_R }, { 0, 6, Q_KEY_CODE_KP_4 }, // Key::Four
    { 1, 6, Q_KEY_CODE_S }, { 1, 6, Q_KEY_CODE_KP_5 }, // Key::Five
    { 2, 6, Q_KEY_CODE_T }, { 2, 6, Q_KEY_CODE_KP_6 }, // Key::Six
    { 3, 6, Q_KEY_CODE_U }, { 3, 6, Q_KEY_CODE_KP_MULTIPLY }, // Key::Multiplication
    { 4, 6, Q_KEY_CODE_V }, { 4, 6, Q_KEY_CODE_KP_DIVIDE }, // Key::Division

    { 0, 7, Q_KEY_CODE_W }, { 0, 7, Q_KEY_CODE_KP_1 }, // Key::One
    { 1, 7, Q_KEY_CODE_X }, { 1, 7, Q_KEY_CODE_KP_2 }, // Key::Two
    { 2, 7, Q_KEY_CODE_Y }, { 2, 7, Q_KEY_CODE_KP_3 }, // Key::Three
    { 3, 7, Q_KEY_CODE_Z }, { 3, 7, Q_KEY_CODE_KP_ADD }, // Key::Plus
    { 4, 7, Q_KEY_CODE_SPC }, { 4, 7, Q_KEY_CODE_KP_SUBTRACT }, // Key::Minus

    { 0, 8, Q_KEY_CODE_KP_0 }, // Key::Zero
    { 1, 8, Q_KEY_CODE_KP_DECIMAL }, // Key::Dot
    { 2, 8, Q_KEY_CODE_9 }, // Key::EE
    { 3, 8, Q_KEY_CODE_0 }, // Key::Ans
    { 4, 8, Q_KEY_CODE_KP_ENTER }, // Key::Exe

    { 0, 0, Q_KEY_CODE_UNMAPPED },
};

static void numworks_init(MachineState *machine)
{
    NumworksState *s = NUMWORKS(machine);
    NumworksClass *sc = NUMWORKS_GET_CLASS(s);
    DeviceState *soc;
    DeviceState *gpio;
    DeviceState *dev;
    Clock *sysclk;
    int i;

    /* This clock doesn't need migration because it is fixed-frequency */
    sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(sysclk, SYSCLK_FRQ);

    soc = sc->init(s);
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
    gpio_keypad_set_keys(gpio, numworks_keys);
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
                       sc->flash_size);
}

static void numworks_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->init = numworks_init;
}


static DeviceState* n0100_init(NumworksState *s)
{
    DeviceState *soc;
    soc = qdev_new(TYPE_STM32F4XX_SOC);
    qdev_prop_set_string(soc, "soc-type", VARIANT_STM32F412_SOC);
    qdev_prop_set_uint32(DEVICE(&STM32F4XX_SOC(soc)->adc[0]), "value", 0xFFF);
    return soc;
}

static void n0100_machine_class_init(ObjectClass *oc, void *data)
{
    NumworksClass *nc = NUMWORKS_CLASS(oc);
    nc->init = &n0100_init;
    nc->flash_size = STM32F412_SOC_FLASH_SIZE;

    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NumWorks N0100 calculator (Cortex-M4)";
}

static DeviceState* n0110_init(NumworksState *s)
{
    DeviceState *soc;
    Error *err = NULL;
    soc = qdev_new(TYPE_STM32F730_SOC);
    qdev_prop_set_uint32(DEVICE(&STM32F730_SOC(soc)->adc[0]), "value", 0xFFF);

    memory_region_init_rom(&s->external_flash, OBJECT(s), "numworks.external.flash", 8 * MiB, &err);
    if (err != NULL) {
        assert(false); // Propagate error ?
    }
    MemoryRegion *system_memory = get_system_memory();
    memory_region_add_subregion(system_memory, 0x90000000, &s->external_flash);

    return soc;
}

static void n0110_machine_class_init(ObjectClass *oc, void *data)
{
    NumworksClass *nc = NUMWORKS_CLASS(oc);
    nc->init = &n0110_init;
    nc->flash_size = STM32F730_SOC_FLASH_SIZE;

    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NumWorks N0110 calculator (Cortex-M7)";
}

static const TypeInfo numworks_machine_types[] = {
    {
        .name           = MACHINE_TYPE_NAME("n0100"),
        .parent         = TYPE_NUMWORKS,
        .class_init     = n0100_machine_class_init,
    }, {
        .name           = MACHINE_TYPE_NAME("n0110"),
        .parent         = TYPE_NUMWORKS,
        .class_init     = n0110_machine_class_init,
    }, {
        .name           = TYPE_NUMWORKS,
        .parent         = TYPE_MACHINE,
        .class_init     = numworks_machine_class_init,
        .class_size    = sizeof(NumworksClass),
        .instance_size = sizeof(NumworksState),
    },
};

DEFINE_TYPES(numworks_machine_types)
