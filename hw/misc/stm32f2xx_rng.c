/*
 * STM32F2XX RNG
 *
 * Copyright (c) 2022 Joachim Le Fournis <joachimlf@pm.me>
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
#include "hw/misc/stm32f2xx_rng.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"
#include "qemu/guest-random.h"


static void stm32f2xx_rng_reset(DeviceState *dev)
{
    STM32F2XXRngState *s = STM32F2XX_RNG(dev);
    (void)s;
}

static uint64_t stm32f2xx_rng_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXRngState *s = opaque;
    uint64_t value = 0;

    switch (addr) {
    case RNG_CR:
        value = s->CR;
        break;
    case RNG_SR:
        value = 0x1; // DRDR is always 1
        break;
    case RNG_DR:
        qemu_guest_getrandom(&value, sizeof(value), NULL);
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented RNG read 0x%"HWADDR_PRIx"\n", __func__,
                      addr);
        break;
    }

    trace_stm32f2xx_rng_read(s, addr, size, value);
    return value;
}

static void stm32f2xx_rng_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F2XXRngState *s = opaque;
    uint32_t value = val64;

    trace_stm32f2xx_rng_write(s, addr, size, val64);

    switch (addr) {
    case RNG_CR:
        s->CR = value & 0x6;
        break;
    case RNG_SR:
        qemu_log_mask(LOG_UNIMP,
                      "%s : Unimplemented RNG write in SR\n", __func__);
        break;
    case RNG_DR:
        qemu_log_mask(LOG_UNIMP,
                      "%s : Unimplemented RNG write in DR\n", __func__);
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented RNG write 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps stm32f2xx_rng_ops = {
    .read = stm32f2xx_rng_read,
    .write = stm32f2xx_rng_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f2xx_rng_init(Object *obj)
{
    STM32F2XXRngState *s = STM32F2XX_RNG(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_rng_ops, s,
                          TYPE_STM32F2XX_RNG, 0xC);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f2xx_rng_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_rng_reset;
}

static const TypeInfo stm32f2xx_rng_info = {
    .name          = TYPE_STM32F2XX_RNG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F2XXRngState),
    .instance_init = stm32f2xx_rng_init,
    .class_init    = stm32f2xx_rng_class_init,
};

static void stm32f2xx_rng_register_types(void)
{
    type_register_static(&stm32f2xx_rng_info);
}

type_init(stm32f2xx_rng_register_types)
