/*
 * STM32F2XX USB OTG FS
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
#include "hw/misc/stm32f2xx_usb_otg_fs.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"

static void stm32f2xx_usb_otg_fs_reset(DeviceState *dev)
{
    STM32F2XXUsbOtgFsState *s = STM32F2XX_USB_OTG_FS(dev);

    s->grstctl = 0x80000000;
}

static uint64_t stm32f2xx_usb_otg_fs_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXUsbOtgFsState *s = opaque;
    uint64_t value = 0;

    switch (addr) {
    case OTG_FS_GRSTCTL:
        value = s->grstctl;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented USB_OTG_FS read 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
    }

    trace_stm32f2xx_usb_otg_fs_read(s, addr, size, value);
    return value;
}

static void stm32f2xx_usb_otg_fs_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F2XXUsbOtgFsState *s = opaque;
    uint32_t value = val64;

    trace_stm32f2xx_usb_otg_fs_write(s, addr, size, val64);

    switch (addr) {
    case OTG_FS_GRSTCTL:
        value &= 0x000007F7;
        /* Simulate core soft reset. */
        value &= ~(1u << 0);
        value |= 0x80000000;
        s->grstctl = value;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented USB OTG FS write 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps stm32f2xx_usb_otg_fs_ops = {
    .read = stm32f2xx_usb_otg_fs_read,
    .write = stm32f2xx_usb_otg_fs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f2xx_usb_otg_fs_init(Object *obj)
{
    STM32F2XXUsbOtgFsState *s = STM32F2XX_USB_OTG_FS(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_usb_otg_fs_ops, s,
                          TYPE_STM32F2XX_USB_OTG_FS, 0x31000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f2xx_usb_otg_fs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_usb_otg_fs_reset;
}

static const TypeInfo stm32f2xx_usb_otg_fs_info = {
    .name          = TYPE_STM32F2XX_USB_OTG_FS,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F2XXUsbOtgFsState),
    .instance_init = stm32f2xx_usb_otg_fs_init,
    .class_init    = stm32f2xx_usb_otg_fs_class_init,
};

static void stm32f2xx_usb_otg_fs_register_types(void)
{
    type_register_static(&stm32f2xx_usb_otg_fs_info);
}

type_init(stm32f2xx_usb_otg_fs_register_types)
