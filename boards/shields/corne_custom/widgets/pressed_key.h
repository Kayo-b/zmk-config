/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_pressed_key {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *key_label;
    lv_obj_t *position_label;
    lv_obj_t *timestamp_label;
};

int zmk_widget_pressed_key_init(struct zmk_widget_pressed_key *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_pressed_key_obj(struct zmk_widget_pressed_key *widget);