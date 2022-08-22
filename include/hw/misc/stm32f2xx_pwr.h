/*
 * STM32F2xx RCC
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

#ifndef HW_STM32F4XX_PWR_H
#define HW_STM32F4XX_PWR_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define PWR_CR1  0x00
#define PWR_CSR1 0x04
#define PWR_CR2  0x08
#define PWR_CSR2 0x0C

#define TYPE_STM32F2XX_PWR "stm32f2xx-pwr"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F2XXPwrState, STM32F2XX_PWR)

// typedef enum {
//     Scale3 = 0b01,
//     Scale2 = 0b10,
//     Scal1 = 0b11,
// } VOSScale;

// typedef enum {
//     mV2000 = 0b000,
//     mV2100 = 0b001,
//     mV2300 = 0b010,
//     mV2500 = 0b011,
//     mV2600 = 0b100,
//     mV2700 = 0b101,
//     mV2800 = 0b110,
//     mV2900 = 0b111,
// } PVD;

struct STM32F2XXPwrState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    // bool UDEN;
    // bool ODSWEN;
    // bool ODEN;
    // VOSScale VOS;
    // bool ADCDC1;
    // bool MRUDS;
    // bool LPUDS;
    // bool FPDS;
    // bool BDP;
    // PVD PLS;
    // bool PVDE;
    // bool CSBF;
    // bool PPDS;
    // bool LPDS;
    uint32_t cr1;
    uint32_t csr1;
};

#endif
