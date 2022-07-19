/*
 * STM32F2XX RCC
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
#include "hw/misc/stm32f2xx_crc.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"
#include "hw/qdev-clock.h"


static void stm32f2xx_crc_reset(DeviceState *dev)
{
    STM32F2XXCrcState *s = STM32F2XX_CRC(dev);
    s->DR = 0xFFFFFFFF;
    s->IDR = 0;
}

static uint32_t stm32f2xx_crc_eat_byte(uint32_t crc, uint8_t byte) {
    crc ^= byte << 24;
    for (int i=8; i--;) {
      crc = crc & 0x80000000 ? ((crc<<1)^CRC_POLYMONIAL) : (crc << 1);
    }
    return crc;
}

static void stm32f2xx_crc_eat_word(STM32F2XXCrcState *s, uint32_t word) {
    uint32_t crc = s->DR;
    for (int i = 3; i >= 0 ; i--) {
      crc = stm32f2xx_crc_eat_byte(crc, (uint8_t)(word >> i * 8));
    }
    s->DR = crc;
}

static uint64_t stm32f2xx_crc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXCrcState *s = opaque;
    uint64_t value = 0;

    switch (addr) {
    case CRC_DR:
        value = s->DR;
        break;
    case CRC_IDR:
        value = s->IDR;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented RCC read 0x%"HWADDR_PRIx"\n", __func__,
                      addr);
        break;
    }

    trace_stm32f2xx_crc_read(s, addr, size, value);
    return value;
}

static void stm32f2xx_crc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F2XXCrcState *s = opaque;
    uint32_t value = val64;

    trace_stm32f2xx_crc_write(s, addr, size, val64);

    switch (addr) {
    case CRC_DR:
        stm32f2xx_crc_eat_word(s, value);
        break;
    case CRC_IDR:
        s->IDR = value;
        break;
    case CRC_CR:
        assert(value == 0b1);
        s->DR = 0xFFFFFFFF;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented RCC write 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps stm32f2xx_crc_ops = {
    .read = stm32f2xx_crc_read,
    .write = stm32f2xx_crc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f2xx_crc_init(Object *obj)
{
    STM32F2XXCrcState *s = STM32F2XX_CRC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_crc_ops, s,
                          TYPE_STM32F2XX_CRC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f2xx_crc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_crc_reset;
}

static const TypeInfo stm32f2xx_crc_info = {
    .name          = TYPE_STM32F2XX_CRC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F2XXCrcState),
    .instance_init = stm32f2xx_crc_init,
    .class_init    = stm32f2xx_crc_class_init,
};

static void stm32f2xx_crc_register_types(void)
{
    type_register_static(&stm32f2xx_crc_info);
}

type_init(stm32f2xx_crc_register_types)
