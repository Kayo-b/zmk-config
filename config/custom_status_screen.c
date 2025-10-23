/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/wpm_status.h>

#include "custom_status_screen.h"
#include "widgets/pressed_key.h"

// Widget instances
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_wpm_status wpm_status_widget;
static struct zmk_widget_pressed_key pressed_key_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_opa(screen, LV_OPA_0, LV_PART_MAIN);
    
    // For 128x32 display, arrange widgets efficiently
    // Screen dimensions: 128 pixels wide, 32 pixels tall
    
    // Initialize standard widgets on the left side
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 0, 10);
    
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 20);
    
    // WPM status at bottom left
    zmk_widget_wpm_status_init(&wpm_status_widget, screen);
    lv_obj_align(zmk_widget_wpm_status_obj(&wpm_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    // Initialize the pressed key widget on the right side
    zmk_widget_pressed_key_init(&pressed_key_widget, screen);
    // Widget positioning is handled internally in the widget for right-side vertical layout
    
    LOG_DBG("Custom status screen initialized for 128x32 display");
    return screen;
}