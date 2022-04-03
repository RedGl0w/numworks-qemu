/*
 * STM32F2xx USB OTG FS
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

#ifndef HW_STM_USB_OTG_FS_H
#define HW_STM_USB_OTG_FS_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define OTG_FS_GRSTCTL     0x10

#define TYPE_STM32F2XX_USB_OTG_FS "stm32f2xx-usb-otg-fs"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F2XXUsbOtgFsState, STM32F2XX_USB_OTG_FS)

struct STM32F2XXUsbOtgFsState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t grstctl;
};

#endif
