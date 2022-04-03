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

#ifndef HW_ST7789V_RCC_H
#define HW_ST7789V_RCC_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_ST7789V "st7789v"
OBJECT_DECLARE_SIMPLE_TYPE(ST7789VState, ST7789V)

typedef enum ST7789VStateMachine {
    ST7789V_STATE_RESET,

    ST7789V_STATE_READ_DISPLAY_ID_1,
    ST7789V_STATE_READ_DISPLAY_ID_2,
    ST7789V_STATE_READ_DISPLAY_ID_3,
    ST7789V_STATE_READ_DISPLAY_ID_4,

    ST7789V_STATE_READ_DISPLAY_STATUS_1,
    ST7789V_STATE_READ_DISPLAY_STATUS_2,
    ST7789V_STATE_READ_DISPLAY_STATUS_3,
    ST7789V_STATE_READ_DISPLAY_STATUS_4,
    ST7789V_STATE_READ_DISPLAY_STATUS_5,

    ST7789V_STATE_READ_DISPLAY_POWER_MODE_1,
    ST7789V_STATE_READ_DISPLAY_POWER_MODE_2,

    ST7789V_STATE_READ_DISPLAY_MADCTL_1,
    ST7789V_STATE_READ_DISPLAY_MADCTL_2,

    ST7789V_STATE_READ_DISPLAY_PIXEL_FORMAT_1,
    ST7789V_STATE_READ_DISPLAY_PIXEL_FORMAT_2,

    ST7789V_STATE_READ_DISPLAY_IMAGE_MODE_1,
    ST7789V_STATE_READ_DISPLAY_IMAGE_MODE_2,

    ST7789V_STATE_READ_DISPLAY_SIGNAL_MODE_1,
    ST7789V_STATE_READ_DISPLAY_SIGNAL_MODE_2,

    ST7789V_STATE_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT_1,
    ST7789V_STATE_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT_2,

    ST7789V_STATE_WRITE_GAMMA_SET,

    ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_1,
    ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_2,
    ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_3,
    ST7789V_STATE_WRITE_COLUMN_ADDRESS_SET_4,

    ST7789V_STATE_WRITE_ROW_ADDRESS_SET_1,
    ST7789V_STATE_WRITE_ROW_ADDRESS_SET_2,
    ST7789V_STATE_WRITE_ROW_ADDRESS_SET_3,
    ST7789V_STATE_WRITE_ROW_ADDRESS_SET_4,

    ST7789V_STATE_WRITE_MEMORY_DATA_ACCESS_CONTROL,

    ST7789V_STATE_WRITE_PIXEL_FORMAT,

    ST7789V_STATE_READ_MEMORY,
    ST7789V_STATE_WRITE_MEMORY,
} ST7789VStateMachine;

struct ST7789VState {
    SysBusDevice parent_obj;

    uint32_t display_id;
    uint32_t width;
    uint32_t height;
    bool rotate_right;

    MemoryRegion mmio;
    MemoryRegion framebuffer;
    MemoryRegionSection fbsection;
    uint32_t *vram;
    QemuConsole *con;
    int invalidate;

    ST7789VStateMachine state;

    bool bston;
    bool my;
    bool mx;
    bool mv;
    bool ml;
    bool rgb;
    bool mh;

    uint8_t ifpf;
    bool idmon;
    bool ptlon;
    bool slpout;
    bool noron;

    bool vsson;
    bool invon;
    bool dison;
    bool teon;
    uint8_t gcsel;
    bool tem;

    uint8_t rgb_fmt;
    uint8_t ctrl_fmt;

    uint16_t xs;
    uint16_t xe;
    uint16_t ys;
    uint16_t ye;

    int col;
    int row;
};

#endif
