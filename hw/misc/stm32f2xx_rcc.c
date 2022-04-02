/*
 * STM32F2XX RCC
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
#include "hw/misc/stm32f2xx_rcc.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"

static void stm32f2xx_rcc_reset(DeviceState *dev)
{
    STM32F2XXRccState *s = STM32F2XX_RCC(dev);

    s->rcc_cr = 0x00000083;
    s->rcc_cfgr = 0x00000000;
}

static uint64_t stm32f2xx_rcc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXRccState *s = opaque;
    uint64_t value = 0;

    switch (addr) {
    case RCC_CR:
        value = s->rcc_cr;
        break;
    case RCC_CFGR:
        value = s->rcc_cfgr;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented RCC read 0x%"HWADDR_PRIx"\n", __func__,
                      addr);
        break;
    }

    trace_stm32f2xx_rcc_read(s, addr, size, value);
    return value;
}

static void stm32f2xx_rcc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    const uint32_t clocks_on_mask = 0x15010001;
    STM32F2XXRccState *s = opaque;
    uint32_t value = val64;

    trace_stm32f2xx_rcc_read(s, addr, size, val64);

    switch (addr) {
    case RCC_CR:
        /* Crudely simulate clock readiness. */
        value = (value & ~(clocks_on_mask << 1));
        value |= (value & clocks_on_mask) << 1;
        s->rcc_cr = value;
        break;
    case RCC_CFGR:
        value &= ~0xC;
        value |= (value & 0x3) << 2;
        s->rcc_cfgr = value;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented RCC write 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps stm32f2xx_rcc_ops = {
    .read = stm32f2xx_rcc_read,
    .write = stm32f2xx_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f2xx_rcc_init(Object *obj)
{
    STM32F2XXRccState *s = STM32F2XX_RCC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_rcc_ops, s,
                          TYPE_STM32F2XX_RCC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f2xx_rcc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_rcc_reset;
}

static const TypeInfo stm32f2xx_rcc_info = {
    .name          = TYPE_STM32F2XX_RCC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F2XXRccState),
    .instance_init = stm32f2xx_rcc_init,
    .class_init    = stm32f2xx_rcc_class_init,
};

static void stm32f2xx_rcc_register_types(void)
{
    type_register_static(&stm32f2xx_rcc_info);
}

type_init(stm32f2xx_rcc_register_types)
