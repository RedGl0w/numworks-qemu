/*
 * Generic GPIO-driven keypad keyboard.
 *
 * Copyright 2022 Jean-Baptiste Boric <jblbeurope@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "qemu/osdep.h"
#include "qemu/cutils.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "hw/input/gpio-keypad.h"
#include "ui/input.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "trace.h"

void gpio_keypad_set_keys(DeviceState *dev, const GpioKeypadKey *keys)
{
    char key_index[16];
    char key_definition[16];
    unsigned count = 0;
    unsigned i;
    int rc;

    for (i = 0; keys[i].qcode != Q_KEY_CODE_UNMAPPED; i++) {
        count++;
    }

    qdev_prop_set_uint32(dev, "len-keys", count);

    for (i = 0; i < count; i++) {
        const GpioKeypadKey *key = &keys[i];

        rc = snprintf(key_definition, sizeof(key_definition), "%u;%u:%i",
                    key->column, key->row, key->qcode);
        assert(rc < sizeof(key_definition));
        rc = snprintf(key_index, sizeof(key_index), "keys[%u]", i);
        assert(rc < sizeof(key_index));

        qdev_prop_set_string(dev, key_index, key_definition);
    }
}

static void gpio_keypad_set_output(GpioKeypadState *s)
{
    uint32_t row, column;
    uint32_t output = 0;
    bool active;

    for (row = 0; row < s->num_rows; row++) {
        if (s->input & (1u << row)) {
            for (column = 0; column < s->num_columns; column++) {
                if (s->keypad_status[column][row]) {
                    output |= 1u << column;
                }
            }
        }
    }

    if (s->active_low) {
        output = ~output;
    }

    trace_gpio_keypad_set_output(DEVICE(s)->canonical_path, output);

    for (column = 0; column < s->num_columns; column++) {
        active = output & (1u << column);

        qemu_set_irq(s->output[column], active);
    }
}

static void gpio_keypad_set_input(void *opaque, int n, int level)
{
    GpioKeypadState *s = GPIO_KEYPAD(opaque);

    if (s->active_low) {
        level = !level;
    }

    s->input &= ~(1u << n);
    if (level) {
        s->input |= 1u << n;
    }

    trace_gpio_keypad_set_input(DEVICE(s)->canonical_path, s->input);
    gpio_keypad_set_output(s);
}

static void gpio_keypad_keyboard_event(DeviceState *dev, QemuConsole *src,
                                       InputEvent *evt)
{
    GpioKeypadState *s = GPIO_KEYPAD(dev);
    InputKeyEvent *key = evt->u.key.data;
    GpioKeypadKey *candidate;
    int qcode;
    uint32_t i;
    bool need_set_output = false;

    assert(evt->type == INPUT_EVENT_KIND_KEY);
    qcode = qemu_input_key_value_to_qcode(key->key);

    for (i = 0; i < s->num_keys; i++) {
        candidate = &s->keys[i];

        if (candidate->qcode == qcode) {
            trace_gpio_keypad_keyboard_event(DEVICE(s)->canonical_path,
                                             qcode, key->down);

            s->keypad_status[candidate->column][candidate->row] = key->down;
            need_set_output = true;
        }
    }

    if (need_set_output && s->input) {
        gpio_keypad_set_output(s);
    }
}

static QemuInputHandler gpio_keypad_keyboard_handler = {
    .name  = "GPIO Keypad Keyboard",
    .mask  = INPUT_EVENT_MASK_KEY,
    .event = gpio_keypad_keyboard_event,
};

static void gpio_keypad_realize(DeviceState *dev, Error **errp)
{
    GpioKeypadState *s = GPIO_KEYPAD(dev);
    uint32_t i;
    GpioKeypadKey *key;

    if (s->num_columns >= GPIO_KEYPAD_NR_PINS) {
        error_setg(errp,
                   "number of columns configured (%u) exceeds keypad capability (%u)",
                   s->num_columns, GPIO_KEYPAD_NR_PINS);
        return;
    } else if (s->num_rows >= GPIO_KEYPAD_NR_PINS) {
        error_setg(errp,
                   "number of rows configured (%u) exceeds keypad capability (%u)",
                   s->num_rows, GPIO_KEYPAD_NR_PINS);
        return;
    }

    for (i = 0; i < s->num_keys; i++) {
        key = &s->keys[i];

        if (key->column >= s->num_columns || key->row >= s->num_rows) {
            error_setg(errp,
                       "key %u location (%u; %u) exceeds keypad dimensions (%u; %u)",
                       i, key->column, key->row, s->num_columns, s->num_rows);
            return;
        }
    }

    qemu_input_handler_register(dev, &gpio_keypad_keyboard_handler);
}

/*
 * Accepted syntax:
 *   <column>:<row>:<qcode>
 *   where column/row addresses are unsigned decimal integers
 *   and qcode is a signed decimal integer
 */
static void get_keypad_key(Object *obj, Visitor *v, const char *name,
                           void *opaque, Error **errp)
{
    Property *prop = opaque;
    GpioKeypadKey *key = object_field_prop_ptr(obj, prop);
    char buffer[64];
    char *p = buffer;
    int rc;

    rc = snprintf(buffer, sizeof(buffer), "%u;%u:%i",
                  key->column, key->row, key->qcode);
    assert(rc < sizeof(buffer));

    visit_type_str(v, name, &p, errp);
}

static void set_keypad_key(Object *obj, Visitor *v, const char *name,
                           void *opaque, Error **errp)
{
    Property *prop = opaque;
    GpioKeypadKey *key = object_field_prop_ptr(obj, prop);
    Error *local_err = NULL;
    const char *endptr;
    char *str;
    int ret;

    visit_type_str(v, name, &str, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    ret = qemu_strtoui(str, &endptr, 10, &key->column);
    if (ret) {
        error_setg(errp, "column address of '%s'"
                   " must be an unsigned decimal integer", name);
        goto out;
    }
    if (*endptr != ';') {
        error_setg(errp, "keypad key coordinates must be separated with ';'");
        goto out;
    }

    ret = qemu_strtoui(endptr + 1, &endptr, 10, &key->row);
    if (ret) {
        error_setg(errp, "row address of '%s'"
                   " must be an unsigned decimal integer", name);
        goto out;
    }
    if (*endptr != ':') {
        error_setg(errp, "keypad key code field must be separated with ':'");
        goto out;
    }

    ret = qemu_strtoi(endptr + 1, &endptr, 10, &key->qcode);
    if (ret) {
        error_setg(errp, "keycode of '%s'"
                   " must be a decimal integer", name);
    }

out:
    g_free(str);
    return;
}

const PropertyInfo gpio_keypad_key_property_info = {
    .name  = "gpio_keypad_key",
    .description = "Keypad key, example: 3;2:136",
    .get   = get_keypad_key,
    .set   = set_keypad_key,
};

static Property gpio_keypad_properties[] = {
    DEFINE_PROP_BOOL("active-low", GpioKeypadState, active_low, 0),
    DEFINE_PROP_UINT32("num-rows", GpioKeypadState, num_rows, 0),
    DEFINE_PROP_UINT32("num-columns", GpioKeypadState, num_columns, 0),
    DEFINE_PROP_ARRAY("keys", GpioKeypadState, num_keys, keys,
                      gpio_keypad_key_property_info, GpioKeypadKey),
    DEFINE_PROP_END_OF_LIST(),
};

static void gpio_keypad_initfn(Object *obj)
{
    GpioKeypadState *s = GPIO_KEYPAD(obj);

    qdev_init_gpio_in(DEVICE(obj), gpio_keypad_set_input, GPIO_KEYPAD_NR_PINS);
    qdev_init_gpio_out(DEVICE(obj), s->output, GPIO_KEYPAD_NR_PINS);
}

static void gpio_keypad_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "GPIO-based keypad keyboard";
    dc->realize = gpio_keypad_realize;
    device_class_set_props(dc, gpio_keypad_properties);
}

static const TypeInfo gpio_keypad_types[] = {
    {
        .name          = TYPE_GPIO_KEYPAD,
        .parent        = TYPE_SYS_BUS_DEVICE,
        .instance_init = gpio_keypad_initfn,
        .instance_size = sizeof(GpioKeypadState),
        .class_init    = gpio_keypad_class_init,
    },
};

DEFINE_TYPES(gpio_keypad_types);
