/*
 * STM32F2XX PWR
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
#include "hw/misc/stm32f2xx_pwr.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"
#include "hw/qdev-clock.h"

static void stm32f2xx_pwr_reset(DeviceState *dev)
{
    STM32F2XXPwrState *s = STM32F2XX_PWR(dev);
    s->cr1 = 0;
    s->csr1 = 0;
}

static uint64_t stm32f2xx_pwr_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXPwrState *s = opaque;
    uint64_t value = 0;

    switch (addr) {
        case PWR_CR1:
            value = s->cr1;
            break;
        case PWR_CSR1:
            value = s->csr1 | 0x34000;
            break;
        case PWR_CR2:
        case PWR_CSR2:
        default: {
            qemu_log_mask(LOG_UNIMP,
                        "%s: Unimplemented PWR read 0x%"HWADDR_PRIx"\n", __func__,
                        addr);
            break;
        }
    }

    trace_stm32f2xx_pwr_read(s, addr, size, value);
    return value;
}

static void stm32f2xx_pwr_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F2XXPwrState *s = opaque;
    uint32_t value = val64;

    trace_stm32f2xx_pwr_read(s, addr, size, val64);

    switch (addr) {
        case PWR_CR1:
            s->cr1 = value;
            break;
        case PWR_CSR1:
            s->csr1 = value;
            break;
        case PWR_CR2:
        case PWR_CSR2:
        default: {
            qemu_log_mask(LOG_UNIMP,
                        "%s: Unimplemented PWR write 0x%"HWADDR_PRIx"\n",
                        __func__, addr);
        }
    }
}

static const MemoryRegionOps stm32f2xx_pwr_ops = {
    .read = stm32f2xx_pwr_read,
    .write = stm32f2xx_pwr_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f2xx_pwr_init(Object *obj)
{
    STM32F2XXPwrState *s = STM32F2XX_PWR(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_pwr_ops, s,
                          TYPE_STM32F2XX_PWR, 0x10);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f2xx_pwr_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_pwr_reset;
}

static const TypeInfo stm32f2xx_pwr_info = {
    .name          = TYPE_STM32F2XX_PWR,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F2XXPwrState),
    .instance_init = stm32f2xx_pwr_init,
    .class_init    = stm32f2xx_pwr_class_init,
};

static void stm32f2xx_pwr_register_types(void)
{
    type_register_static(&stm32f2xx_pwr_info);
}

type_init(stm32f2xx_pwr_register_types)
