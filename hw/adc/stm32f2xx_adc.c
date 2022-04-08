/*
 * STM32F2XX ADC
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/adc/stm32f2xx_adc.h"
#include "trace.h"

#define ADC_SR    0x00
#define ADC_CR1   0x04
#define ADC_CR2   0x08
#define ADC_SMPR1 0x0C
#define ADC_SMPR2 0x10
#define ADC_JOFR1 0x14
#define ADC_JOFR2 0x18
#define ADC_JOFR3 0x1C
#define ADC_JOFR4 0x20
#define ADC_HTR   0x24
#define ADC_LTR   0x28
#define ADC_SQR1  0x2C
#define ADC_SQR2  0x30
#define ADC_SQR3  0x34
#define ADC_JSQR  0x38
#define ADC_JDR1  0x3C
#define ADC_JDR2  0x40
#define ADC_JDR3  0x44
#define ADC_JDR4  0x48
#define ADC_DR    0x4C

#define ADC_CR2_ADON    0x01
#define ADC_CR2_CONT    0x02
#define ADC_CR2_ALIGN   0x800
#define ADC_CR2_SWSTART 0x40000000

#define ADC_CR1_RES 0x3000000

#define ADC_COMMON_ADDRESS 0x100

static void stm32f2xx_adc_reset(DeviceState *dev)
{
    STM32F2XXADCState *s = STM32F2XX_ADC(dev);

    s->adc_sr = 0x00000000;
    s->adc_cr1 = 0x00000000;
    s->adc_cr2 = 0x00000000;
    s->adc_smpr1 = 0x00000000;
    s->adc_smpr2 = 0x00000000;
    s->adc_jofr[0] = 0x00000000;
    s->adc_jofr[1] = 0x00000000;
    s->adc_jofr[2] = 0x00000000;
    s->adc_jofr[3] = 0x00000000;
    s->adc_htr = 0x00000FFF;
    s->adc_ltr = 0x00000000;
    s->adc_sqr1 = 0x00000000;
    s->adc_sqr2 = 0x00000000;
    s->adc_sqr3 = 0x00000000;
    s->adc_jsqr = 0x00000000;
    s->adc_jdr[0] = 0x00000000;
    s->adc_jdr[1] = 0x00000000;
    s->adc_jdr[2] = 0x00000000;
    s->adc_jdr[3] = 0x00000000;
    s->adc_dr = 0x00000000;
}

static uint32_t stm32f2xx_adc_generate_value(STM32F2XXADCState *s)
{
    /* Attempts to fake some ADC values */
    s->adc_dr = s->adc_dr + 7;

    switch ((s->adc_cr1 & ADC_CR1_RES) >> 24) {
    case 0:
        /* 12-bit */
        s->adc_dr &= 0xFFF;
        break;
    case 1:
        /* 10-bit */
        s->adc_dr &= 0x3FF;
        break;
    case 2:
        /* 8-bit */
        s->adc_dr &= 0xFF;
        break;
    default:
        /* 6-bit */
        s->adc_dr &= 0x3F;
    }

    if (s->adc_cr2 & ADC_CR2_ALIGN) {
        return (s->adc_dr << 1) & 0xFFF0;
    } else {
        return s->adc_dr;
    }
}

static uint64_t stm32f2xx_adc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXADCState *s = opaque;
    uint32_t value = 0;

    switch (addr) {
    case ADC_SR:
        value = s->adc_sr;
        break;
    case ADC_CR1:
        value = s->adc_cr1;
        break;
    case ADC_CR2:
        value = s->adc_cr2 & 0x0FFFFFFF;
        break;
    case ADC_SMPR1:
        value = s->adc_smpr1;
        break;
    case ADC_SMPR2:
        value = s->adc_smpr2;
        break;
    case ADC_JOFR1:
    case ADC_JOFR2:
    case ADC_JOFR3:
    case ADC_JOFR4:
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Injection ADC is not implemented, the registers are " \
                      "included for compatibility\n", __func__);
        value = s->adc_jofr[(addr - ADC_JOFR1) / 4];
        break;
    case ADC_HTR:
        value = s->adc_htr;
        break;
    case ADC_LTR:
        value = s->adc_ltr;
        break;
    case ADC_SQR1:
        value = s->adc_sqr1;
        break;
    case ADC_SQR2:
        value = s->adc_sqr2;
        break;
    case ADC_SQR3:
        value = s->adc_sqr3;
        break;
    case ADC_JSQR:
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Injection ADC is not implemented, the registers are " \
                      "included for compatibility\n", __func__);
        value = s->adc_jsqr;
        break;
    case ADC_JDR1:
    case ADC_JDR2:
    case ADC_JDR3:
    case ADC_JDR4:
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Injection ADC is not implemented, the registers are " \
                      "included for compatibility\n", __func__);
        value = s->adc_jdr[(addr - ADC_JDR1) / 4] -
               s->adc_jofr[(addr - ADC_JDR1) / 4];
        break;
    case ADC_DR:
        if ((s->adc_cr2 & ADC_CR2_ADON) && (s->adc_cr2 & ADC_CR2_SWSTART)) {
            s->adc_cr2 ^= ADC_CR2_SWSTART;
            value = stm32f2xx_adc_generate_value(s);
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        break;
    }

    trace_stm32f2xx_adc_read(DEVICE(opaque)->canonical_path, addr, value);
    return value;
}

static void stm32f2xx_adc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F2XXADCState *s = opaque;
    uint32_t value = (uint32_t) val64;

    trace_stm32f2xx_adc_write(DEVICE(opaque)->canonical_path, addr, value);

    switch (addr) {
    case ADC_SR:
        s->adc_sr &= (value & 0x3F);
        break;
    case ADC_CR1:
        s->adc_cr1 = value;
        break;
    case ADC_CR2:
        s->adc_cr2 = value;
        break;
    case ADC_SMPR1:
        s->adc_smpr1 = value;
        break;
    case ADC_SMPR2:
        s->adc_smpr2 = value;
        break;
    case ADC_JOFR1:
    case ADC_JOFR2:
    case ADC_JOFR3:
    case ADC_JOFR4:
        s->adc_jofr[(addr - ADC_JOFR1) / 4] = (value & 0xFFF);
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Injection ADC is not implemented, the registers are " \
                      "included for compatibility\n", __func__);
        break;
    case ADC_HTR:
        s->adc_htr = value;
        break;
    case ADC_LTR:
        s->adc_ltr = value;
        break;
    case ADC_SQR1:
        s->adc_sqr1 = value;
        break;
    case ADC_SQR2:
        s->adc_sqr2 = value;
        break;
    case ADC_SQR3:
        s->adc_sqr3 = value;
        break;
    case ADC_JSQR:
        s->adc_jsqr = value;
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Injection ADC is not implemented, the registers are " \
                      "included for compatibility\n", __func__);
        break;
    case ADC_JDR1:
    case ADC_JDR2:
    case ADC_JDR3:
    case ADC_JDR4:
        s->adc_jdr[(addr - ADC_JDR1) / 4] = value;
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Injection ADC is not implemented, the registers are " \
                      "included for compatibility\n", __func__);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        break;
    }
}

static const MemoryRegionOps stm32f2xx_adc_ops = {
    .read = stm32f2xx_adc_read,
    .write = stm32f2xx_adc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const VMStateDescription vmstate_stm32f2xx_adc = {
    .name = TYPE_STM32F2XX_ADC,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(adc_sr, STM32F2XXADCState),
        VMSTATE_UINT32(adc_cr1, STM32F2XXADCState),
        VMSTATE_UINT32(adc_cr2, STM32F2XXADCState),
        VMSTATE_UINT32(adc_smpr1, STM32F2XXADCState),
        VMSTATE_UINT32(adc_smpr2, STM32F2XXADCState),
        VMSTATE_UINT32_ARRAY(adc_jofr, STM32F2XXADCState, 4),
        VMSTATE_UINT32(adc_htr, STM32F2XXADCState),
        VMSTATE_UINT32(adc_ltr, STM32F2XXADCState),
        VMSTATE_UINT32(adc_sqr1, STM32F2XXADCState),
        VMSTATE_UINT32(adc_sqr2, STM32F2XXADCState),
        VMSTATE_UINT32(adc_sqr3, STM32F2XXADCState),
        VMSTATE_UINT32(adc_jsqr, STM32F2XXADCState),
        VMSTATE_UINT32_ARRAY(adc_jdr, STM32F2XXADCState, 4),
        VMSTATE_UINT32(adc_dr, STM32F2XXADCState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f2xx_adc_init(Object *obj)
{
    STM32F2XXADCState *s = STM32F2XX_ADC(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_adc_ops, s,
                          TYPE_STM32F2XX_ADC, 0x100);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f2xx_adc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_adc_reset;
    dc->vmsd = &vmstate_stm32f2xx_adc;
}

static const TypeInfo stm32f2xx_adc_types[] = {
    {
        .name          = TYPE_STM32F2XX_ADC,
        .parent        = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(STM32F2XXADCState),
        .instance_init = stm32f2xx_adc_init,
        .class_init    = stm32f2xx_adc_class_init,
    },
};

DEFINE_TYPES(stm32f2xx_adc_types);
