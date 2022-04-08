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

#ifndef HW_STM32F2XX_ADC_H
#define HW_STM32F2XX_ADC_H

#include "hw/sysbus.h"
#include "qom/object.h"

typedef struct STM32F2XXADCState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t adc_sr;
    uint32_t adc_cr1;
    uint32_t adc_cr2;
    uint32_t adc_smpr1;
    uint32_t adc_smpr2;
    uint32_t adc_jofr[4];
    uint32_t adc_htr;
    uint32_t adc_ltr;
    uint32_t adc_sqr1;
    uint32_t adc_sqr2;
    uint32_t adc_sqr3;
    uint32_t adc_jsqr;
    uint32_t adc_jdr[4];
    uint32_t adc_dr;

    qemu_irq irq;
} STM32F2XXADCState;

#define TYPE_STM32F2XX_ADC "stm32f2xx-adc"
#define STM32F2XX_ADC(obj) \
    OBJECT_CHECK(STM32F2XXADCState, (obj), TYPE_STM32F2XX_ADC)

#endif /* HW_STM32F2XX_ADC_H */
