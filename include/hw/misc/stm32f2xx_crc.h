/*
 * STM32F2xx RCC
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

#ifndef HW_STM32F4XX_CRC_H
#define HW_STM32F4XX_CRC_H

#define CRC_DR  0x00
#define CRC_IDR 0x04
#define CRC_CR  0x08

#define CRC_POLYMONIAL 0x04C11DB7

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_STM32F2XX_CRC "stm32f2xx-crc"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F2XXCrcState, STM32F2XX_CRC)

struct STM32F2XXCrcState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t DR;
    uint8_t IDR;
};

#endif
