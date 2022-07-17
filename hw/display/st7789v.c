/*
 * Sitronix ST7789V display controller
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
#include "ui/console.h"
#include "ui/pixel_ops.h"
#include "hw/display/framebuffer.h"
#include "hw/display/st7789v.h"
#include "hw/qdev-properties.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "qom/object.h"
#include "trace.h"

#define ST7789V_COMMAND 0x00000
#define ST7789V_DATA    0x20000

enum ST7789V_Command {
    ST7789V_NOP = 0x00,
    ST7789V_RESET = 0x01,
    ST7789V_READ_DISPLAY_ID = 0x04,
    ST7789V_READ_DISPLAY_STATUS = 0x09,
    ST7789V_READ_DISPLAY_POWER_MODE = 0x0A,
    ST7789V_READ_DISPLAY_MADCTL = 0x0B,
    ST7789V_READ_DISPLAY_PIXEL_FORMAT = 0x0C,
    ST7789V_READ_DISPLAY_IMAGE_MODE = 0x0D,
    ST7789V_READ_DISPLAY_SIGNAL_MODE = 0x0E,
    ST7789V_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT = 0x0F,
    ST7789V_SLEEP_IN = 0x10,
    ST7789V_SLEEP_OUT = 0x11,
    ST7789V_PARTIAL_DISPLAY_MODE_ON = 0x12,
    ST7789V_NORMAL_DISPLAY_MODE_ON = 0x13,
    ST7789V_DISPLAY_INVERSION_OFF = 0x20,
    ST7789V_DISPLAY_INVERSION_ON = 0x21,
    ST7789V_GAMMA_SET = 0x26,
    ST7789V_DISPLAY_OFF = 0x28,
    ST7789V_DISPLAY_ON = 0x29,
    ST7789V_COLUMN_ADDRESS_SET = 0x2A,
    ST7789V_ROW_ADDRESS_SET = 0x2B,
    ST7789V_MEMORY_WRITE = 0x2C,
    ST7789V_MEMORY_READ = 0x2E,
    ST7789V_PARTIAL_AREA = 0x30,
    ST7789V_VERTICAL_SCROLLING_DEFINITION = 0x33,
    ST7789V_TEARING_EFFECT_LINE_OFF = 0x34,
    ST7789V_TEARING_EFFECT_LINE_ON = 0x35,
    ST7789V_MEMORY_ACCESS_CONTROL = 0x36,
    ST7789V_VERTICAL_SCROLL_START_ADDRESS_OF_RAM = 0x37,
    ST7789V_IDLE_MODE_OFF = 0x38,
    ST7789V_IDLE_MODE_ON = 0x39,
    ST7789V_PIXEL_FORMAT_SET = 0x3A,
    ST7789V_WRITE_MEMORY_CONTINUE = 0x3C,
    ST7789V_READ_MEMORY_CONTINUE = 0x3E,
    ST7789V_SET_TEAR_SCANLINE = 0x44,
    ST7789V_GET_SCANLINE = 0x45,
    ST7789V_WRITE_DISPLAY_BRIGHTNESS = 0x51,
    ST7789V_READ_DISPLAY_BRIGHTNESS_VALUE = 0x52,
    ST7789V_WRITE_CTRL_DISPLAY = 0x53,
    ST7789V_READ_CTRL_VALUE_DISPLAY = 0x54,
    ST7789V_WRITE_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL_AND_COLOR_ENHANCEMENT = 0x55,
    ST7789V_READ_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL = 0x56,
    ST7789V_WRITE_CABC_MINIMUM_BRIGHTNESS = 0x5E,
    ST7789V_READ_CABC_MINIMUM_BRIGHTNESS = 0x5F,
    ST7789V_FRAMERATE_CONTROL = 0xC6,
    ST7789V_READ_ID1 = 0xDA,
    ST7789V_READ_ID2 = 0xDB,
    ST7789V_READ_ID3 = 0xDC,
    ST7789V_POSITIVE_VOLTAGE_GAMMA_CONTROL = 0xE0,
    ST7789V_NEGATIVE_VOLTAGE_GAMMA_CONTROL = 0xE1,
};

static bool st7789v_compute_offset(ST7789VState *s, int *x, int *y)
{
    if (s->mv) {
        *x = (!s->my) ? (s->width - 1) - s->row : s->row;
        *y = (!s->mx) ? (s->height - 1) - s->col : s->col;
    } else {
        *x = (s->mx) ? (s->width - 1) - s->col : s->col;
        *y = (s->my) ? (s->height - 1) - s->row : s->row;
    }

    return *x >= 0 && *x < s->width && *y >= 0 && *y < s->height;
}

static inline void st7789v_postop(ST7789VState *s)
{
    if (++s->col > s->xe) {
        s->col = s->xs;
        if (++s->row > s->ye) {
            s->row = s->ys;
        }
    }
}

static void st7789v_reset(DeviceState *dev)
{
    ST7789VState *s = ST7789V(dev);

    s->bston = false;
    s->mh = false;
    s->idmon = false;
    s->ptlon = false;
    s->slpout = false;
    s->noron = true;
    s->vsson = false;
    s->invon = false;
    s->dison = false;
    s->teon = false;
    s->gcsel = 0;
    s->tem = false;

    s->xs = 0;
    s->ys = 0;
    if (s->mv) {
        s->xe = 0x013F;
        s->ye = 0x00EF;
    } else {
        s->xe = 0x00EF;
        s->ye = 0x013F;
    }
}

static void st7789v_update(void *opaque)
{
    ST7789VState *s = ST7789V(opaque);
    DisplaySurface *surface = qemu_console_surface(s->con);

    if (s->rotate_right) {
        int stride = surface_width(surface);
        uint32_t *console = surface_data(surface);
        uint32_t *vram = s->vram;

        for (int x = s->height - 1; x >= 0; x--) {
            for (int y = 0; y < s->width; y++) {
                console[y * stride + x] = *vram++;
            }
        }
    } else {
        memcpy(surface_data(surface), s->vram, s->width * s->height * 4);
    }

    if (s->rotate_right) {
        dpy_gfx_update(s->con, 0, 0, s->height, s->width);
    } else {
        dpy_gfx_update(s->con, 0, 0, s->width, s->height);
    }
}

static void st7789v_invalidate(void *opaque)
{
    ST7789VState *s = ST7789V(opaque);
    s->invalidate = 1;
}

static const GraphicHwOps st7789v_ops = {
    .invalidate  = st7789v_invalidate,
    .gfx_update  = st7789v_update,
};

static void st7789v_realize(DeviceState *dev, Error **errp)
{
    ST7789VState *s = ST7789V(dev);

    memory_region_init_ram(&s->framebuffer, OBJECT(s), "st7789v-framebuffer",
                           s->width * s->height * 4, &error_fatal);
    s->vram = memory_region_get_ram_ptr(&s->framebuffer);

    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    s->con = graphic_console_init(dev, 0, &st7789v_ops, s);
    if (s->rotate_right) {
        qemu_console_resize(s->con, s->height, s->width);
    } else {
        qemu_console_resize(s->con, s->width, s->height);
    }

    s->invalidate = 1;
}

static uint64_t st7789v_read(void *opaque, hwaddr addr, unsigned int size)
{
    ST7789VState *s = opaque;
    uint64_t value = 0;

    switch (addr) {
    case ST7789V_COMMAND:
        value = 0;
        break;
    case ST7789V_DATA:
        switch (s->state) {
        case ST7789V_STATE_READ_DISPLAY_ID_1:
            s->state = ST7789V_STATE_READ_DISPLAY_ID_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_ID_2:
            s->state = ST7789V_STATE_READ_DISPLAY_ID_3;
            value = (s->display_id >> 16) & 0xFF;
            break;
        case ST7789V_STATE_READ_DISPLAY_ID_3:
            s->state = ST7789V_STATE_READ_DISPLAY_ID_4;
            value = (s->display_id >> 8) & 0xFF;
            break;
        case ST7789V_STATE_READ_DISPLAY_ID_4:
            s->state = ST7789V_STATE_RESET;
            value = s->display_id & 0xFF;
            break;

        case ST7789V_STATE_READ_DISPLAY_STATUS_1:
            s->state = ST7789V_STATE_READ_DISPLAY_STATUS_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_STATUS_2:
            s->state = ST7789V_STATE_READ_DISPLAY_STATUS_3;
            value = s->bston << 7 | s->my << 6 | s->mx << 5 | s->mv << 4 |
                    s->ml << 3 | s->rgb << 2 | s->mh << 1;
            break;
        case ST7789V_STATE_READ_DISPLAY_STATUS_3:
            s->state = ST7789V_STATE_READ_DISPLAY_STATUS_4;
            value = s->ifpf << 4 | s->idmon << 3 | s->ptlon << 2 |
                    s->slpout << 1 | s->noron;
            break;
        case ST7789V_STATE_READ_DISPLAY_STATUS_4:
            s->state = ST7789V_STATE_READ_DISPLAY_STATUS_5;
            value = s->vsson << 7 | s->invon << 5 | s->dison << 2 |
                    s->teon << 1 | (s->gcsel & 0b100) >> 2;
            break;
        case ST7789V_STATE_READ_DISPLAY_STATUS_5:
            s->state = ST7789V_STATE_RESET;
            value = (s->gcsel & 0b011) << 6 | s->tem << 5;
            break;

        case ST7789V_STATE_READ_DISPLAY_POWER_MODE_1:
            s->state = ST7789V_STATE_READ_DISPLAY_POWER_MODE_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_POWER_MODE_2:
            s->state = ST7789V_STATE_RESET;
            value = s->bston << 7 | s->idmon << 6 | s->ptlon << 5 |
                    s->slpout << 4 | s->noron << 3 | s->dison << 2;
            break;

        case ST7789V_STATE_READ_DISPLAY_MADCTL_1:
            s->state = ST7789V_STATE_READ_DISPLAY_MADCTL_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_MADCTL_2:
            s->state = ST7789V_STATE_RESET;
            value = s->my << 7 | s->mx << 6 | s->mv << 5 | s->ml << 4 |
                    s->rgb << 3 | s->mh << 2;
            break;

        case ST7789V_STATE_READ_DISPLAY_PIXEL_FORMAT_1:
            s->state = ST7789V_STATE_READ_DISPLAY_PIXEL_FORMAT_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_PIXEL_FORMAT_2:
            s->state = ST7789V_STATE_RESET;
            value = s->rgb_fmt << 4 | s->ctrl_fmt;
            break;

        case ST7789V_STATE_READ_DISPLAY_IMAGE_MODE_1:
            s->state = ST7789V_STATE_READ_DISPLAY_IMAGE_MODE_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_IMAGE_MODE_2:
            s->state = ST7789V_STATE_RESET;
            value = s->vsson << 7 | s->invon << 5 | s->gcsel;
            break;

        case ST7789V_STATE_READ_DISPLAY_SIGNAL_MODE_1:
            s->state = ST7789V_STATE_READ_DISPLAY_SIGNAL_MODE_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_SIGNAL_MODE_2:
            s->state = ST7789V_STATE_RESET;
            value = s->teon << 7 | s->tem << 6;
            break;

        case ST7789V_STATE_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT_1:
            s->state = ST7789V_STATE_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT_2;
            value = 0;
            break;
        case ST7789V_STATE_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT_2:
            s->state = ST7789V_STATE_RESET;
            value = 0;
            break;

        case ST7789V_STATE_READ_MEMORY:
            int x,y;
            assert(s->ctrl_fmt == 0b110); // Reading memory should always be done in 18bits/pixel
            switch (s->memory_read_step) {
                case 0:
                    if (st7789v_compute_offset(s, &x, &y)) {
                        uint32_t color32 = s->vram[y * s->width + x];
                        int r = color32 >> 16;
                        int g = (color32 >> 8) & 0xFF;
                        value = (r << 8) | g;
                        s->memory_read_step += 1;
                    }
                    break;
                case 1:
                    if (st7789v_compute_offset(s, &x, &y)) {
                        uint32_t color32 = s->vram[y * s->width + x];
                        int b = color32 & 0xFF;
                        value = b << 8;
                    }
                    st7789v_postop(s);
                    if (st7789v_compute_offset(s, &x, &y)) {
                        uint32_t color32 = s->vram[y * s->width + x];
                        int r = color32 >> 16;
                        value |= r;
                    }
                    s->memory_read_step += 1;
                    break;
                case 2:
                    if (st7789v_compute_offset(s, &x, &y)) {
                        uint32_t color32 = s->vram[y * s->width + x];
                        int g = (color32 >> 8) & 0xFF;
                        int b = color32 & 0xFF;
                        value = (g << 8) | b;
                    }
                    s->memory_read_step = 0;
                    st7789v_postop(s);
                    break;
            }
            break;

        default:
            value = 0;
            break;
        }
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented st7789v read 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
        break;
    }

    trace_st7789v_read(s, addr, size, value);
    return value;
}

static void st7789v_write(void *opaque, hwaddr addr, uint64_t val64,
                              unsigned int size)
{
    ST7789VState *s = opaque;
    uint16_t value = val64;
    int x, y;
    int r, g, b;

    trace_st7789v_write(s, addr, size, val64);

    switch (addr) {
    case ST7789V_COMMAND:
        switch (value) {
        case ST7789V_NOP:
            break;
        case ST7789V_RESET:
            st7789v_reset(opaque);
            s->state = ST7789V_STATE_RESET;
            break;
        case ST7789V_READ_DISPLAY_ID:
            s->state = ST7789V_STATE_READ_DISPLAY_ID_1;
            break;
        case ST7789V_READ_DISPLAY_STATUS:
            s->state = ST7789V_STATE_READ_DISPLAY_STATUS_1;
            break;
        case ST7789V_READ_DISPLAY_POWER_MODE:
            s->state = ST7789V_STATE_READ_DISPLAY_POWER_MODE_1;
            break;
        case ST7789V_READ_DISPLAY_MADCTL:
            s->state = ST7789V_STATE_READ_DISPLAY_MADCTL_1;
            break;
        case ST7789V_READ_DISPLAY_PIXEL_FORMAT:
            s->state = ST7789V_STATE_READ_DISPLAY_PIXEL_FORMAT_1;
            break;
        case ST7789V_READ_DISPLAY_IMAGE_MODE:
            s->state = ST7789V_STATE_READ_DISPLAY_IMAGE_MODE_1;
            break;
        case ST7789V_READ_DISPLAY_SIGNAL_MODE:
            s->state = ST7789V_STATE_READ_DISPLAY_SIGNAL_MODE_1;
            break;
        case ST7789V_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT:
            s->state = ST7789V_STATE_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT_1;
            break;
        case ST7789V_SLEEP_IN:
            s->slpout = false;
            break;
        case ST7789V_SLEEP_OUT:
            s->slpout = true;
            break;
        case ST7789V_PARTIAL_DISPLAY_MODE_ON:
            s->ptlon = true;
            break;
        case ST7789V_NORMAL_DISPLAY_MODE_ON:
            s->noron = true;
            break;
        case ST7789V_DISPLAY_INVERSION_OFF:
            s->invon = false;
            break;
        case ST7789V_DISPLAY_INVERSION_ON:
            s->invon = true;
            break;
        case ST7789V_DISPLAY_OFF:
            s->dison = false;
            break;
        case ST7789V_DISPLAY_ON:
            s->dison = true;
            break;
        case ST7789V_COLUMN_ADDRESS_SET:
            s->state = ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_1;
            break;
        case ST7789V_ROW_ADDRESS_SET:
            s->state = ST7789V_STATE_WRITE_ROW_ADDRESS_SET_1;
            break;
        case ST7789V_MEMORY_WRITE:
            s->state = ST7789V_STATE_WRITE_MEMORY;
            s->col = s->xs;
            s->row = s->ys;
            break;
        case ST7789V_MEMORY_READ:
            s->state = ST7789V_STATE_READ_MEMORY;
            s->col = s->xs;
            s->row = s->ys;
            s->memory_read_step = 0;
            break;
        case ST7789V_TEARING_EFFECT_LINE_OFF:
            s->teon = false;
            break;
        case ST7789V_TEARING_EFFECT_LINE_ON:
            s->teon = true;
            break;
        case ST7789V_MEMORY_ACCESS_CONTROL:
            s->state = ST7789V_STATE_WRITE_MEMORY_DATA_ACCESS_CONTROL;
            break;
        case ST7789V_IDLE_MODE_OFF:
            s->idmon = false;
            break;
        case ST7789V_IDLE_MODE_ON:
            s->idmon = true;
            break;
        case ST7789V_PIXEL_FORMAT_SET:
            s->state = ST7789V_STATE_WRITE_PIXEL_FORMAT;
            break;
        default:
            qemu_log_mask(LOG_UNIMP,
                        "%s: Unimplemented st7789v command 0x%x\n", __func__,
                        value);
        }
        break;
    case ST7789V_DATA:
        switch (s->state) {
        case ST7789V_STATE_WRITE_GAMMA_SET:
            s->state = ST7789V_STATE_RESET;
            if (value == 1) {
                s->gcsel = 0;
            } else if (value == 2) {
                s->gcsel = 1;
            } else if (value == 4) {
                s->gcsel = 2;
            } else if (value == 8) {
                s->gcsel = 3;
            }
            break;

        case ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_1:
            s->state = ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_2;
            s->xs &= 0x00FF;
            s->xs |= (value << 8) & 0xFF00;
            break;
        case ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_2:
            s->state = ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_3;
            s->xs &= 0xFF00;
            s->xs |= value & 0xFF;
            break;
        case ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_3:
            s->state = ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_4;
            s->xe &= 0x00FF;
            s->xe |= (value << 8) & 0xFF00;
            break;
        case ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_4:
            s->state = ST7789V_STATE_RESET;
            s->xe &= 0xFF00;
            s->xe |= value & 0xFF;
            break;

        case ST7789V_STATE_WRITE_ROW_ADDRESS_SET_1:
            s->state = ST7789V_STATE_WRITE_ROW_ADDRESS_SET_2;
            s->ys &= 0x00FF;
            s->ys |= (value << 8) & 0xFF00;
            break;
        case ST7789V_STATE_WRITE_ROW_ADDRESS_SET_2:
            s->state = ST7789V_STATE_WRITE_ROW_ADDRESS_SET_3;
            s->ys &= 0xFF00;
            s->ys |= value & 0xFF;
            break;
        case ST7789V_STATE_WRITE_ROW_ADDRESS_SET_3:
            s->state = ST7789V_STATE_WRITE_ROW_ADDRESS_SET_4;
            s->ye &= 0x00FF;
            s->ye |= (value << 8) & 0xFF00;
            break;
        case ST7789V_STATE_WRITE_ROW_ADDRESS_SET_4:
            s->state = ST7789V_STATE_RESET;
            s->ye &= 0xFF00;
            s->ye |= value & 0xFF;
            break;

        case ST7789V_STATE_WRITE_MEMORY_DATA_ACCESS_CONTROL:
            s->state = ST7789V_STATE_RESET;
            s->my = (value >> 7) & 1;
            s->mx = (value >> 6) & 1;
            s->mv = (value >> 5) & 1;
            s->ml = (value >> 4) & 1;
            s->rgb = (value >> 3) & 1;
            s->mh = (value >> 2) & 1;
            break;

        case ST7789V_STATE_WRITE_PIXEL_FORMAT:
            s->state = ST7789V_STATE_RESET;
            s->rgb_fmt = (value >> 4) & 0b111;
            s->ctrl_fmt = value & 0b111;
            break;

        case ST7789V_STATE_READ_MEMORY:
            st7789v_postop(s);
            break;

        case ST7789V_STATE_WRITE_MEMORY:
            r = (value >> 8) & 0b11111000;
            g = (value >> 3) & 0b11111100;
            b = (value << 3) & 0b11111000;
            if (st7789v_compute_offset(s, &x, &y)) {
                s->vram[y * s->width + x] = rgb_to_pixel32(r, g, b);
                memory_region_set_dirty(&s->framebuffer, y * s->width + x, 4);
            }

            st7789v_postop(s);
            break;

        default:
            break;
        }
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Unimplemented st7789v write 0x%"HWADDR_PRIx"\n",
                      __func__, addr);
        break;
    }
}

static const MemoryRegionOps st7789v_mmio_ops = {
    .read = st7789v_read,
    .write = st7789v_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property st7789v_properties[] = {
    DEFINE_PROP_UINT32("display-id", ST7789VState, display_id, 0x858552),
    DEFINE_PROP_UINT32("width", ST7789VState, width, 240),
    DEFINE_PROP_UINT32("height", ST7789VState, height, 320),
    DEFINE_PROP_BOOL("rotate-right", ST7789VState, rotate_right, false),
    DEFINE_PROP_END_OF_LIST(),
};

static void st7789v_init(Object *obj)
{
    ST7789VState *s = ST7789V(obj);

    s->bston = false;
    s->my = false;
    s->mx = false;
    s->mv = false;
    s->ml = false;
    s->rgb = false;
    s->mh = false;

    s->ifpf = 6;
    s->idmon = false;
    s->ptlon = false;
    s->slpout = false;
    s->noron = true;

    s->vsson = false;
    s->invon = false;
    s->dison = false;
    s->teon = false;
    s->gcsel = 0;
    s->tem = false;

    s->rgb_fmt = 0;
    s->ctrl_fmt = 6;

    s->xs = 0;
    s->xe = 0xEF;

    s->memory_read_step = 0;

    memory_region_init_io(&s->mmio, obj, &st7789v_mmio_ops, s, TYPE_ST7789V,
                          0x40000);

    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void st7789v_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    device_class_set_props(dc, st7789v_properties);
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
    dc->realize = st7789v_realize;
    dc->reset = st7789v_reset;

    /* Note: This device does not any state that we have to reset or migrate */
}

static const TypeInfo st7789v_info = {
    .name          = TYPE_ST7789V,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ST7789VState),
    .instance_init = st7789v_init,
    .class_init    = st7789v_class_init,
};

static void st7789v_register_types(void)
{
    type_register_static(&st7789v_info);
}

type_init(st7789v_register_types)
