/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

#include "pressed_key.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct pressed_key_state {
    uint32_t keycode;
    bool pressed;
    uint32_t position;
    int64_t timestamp;
};

// Keycode to string conversion - optimized for small display
static const char* keycode_to_string(uint32_t keycode) {
    if (keycode >= HID_USAGE_KEY_KEYBOARD_A && keycode <= HID_USAGE_KEY_KEYBOARD_Z) {
        static char letter[2] = {0};
        letter[0] = 'A' + (keycode - HID_USAGE_KEY_KEYBOARD_A);
        return letter;
    }
    
    if (keycode >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION && 
        keycode <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) {
        static char number[2] = {0};
        if (keycode == HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) {
            number[0] = '0';
        } else {
            number[0] = '1' + (keycode - HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION);
        }
        return number;
    }

    // Special keys - abbreviated for small display
    switch (keycode) {
        case HID_USAGE_KEY_KEYBOARD_SPACEBAR: return "SPC";
        case HID_USAGE_KEY_KEYBOARD_RETURN_ENTER: return "ENT";
        case HID_USAGE_KEY_KEYBOARD_ESCAPE: return "ESC";
        case HID_USAGE_KEY_KEYBOARD_TAB: return "TAB";
        case HID_USAGE_KEY_KEYBOARD_BACKSPACE: return "BSP";
        case HID_USAGE_KEY_KEYBOARD_DELETE: return "DEL";
        case HID_USAGE_KEY_KEYBOARD_LEFT_SHIFT: return "LSH";
        case HID_USAGE_KEY_KEYBOARD_RIGHT_SHIFT: return "RSH";
        case HID_USAGE_KEY_KEYBOARD_LEFT_CONTROL: return "LCT";
        case HID_USAGE_KEY_KEYBOARD_RIGHT_CONTROL: return "RCT";
        case HID_USAGE_KEY_KEYBOARD_LEFT_ALT: return "LAL";
        case HID_USAGE_KEY_KEYBOARD_RIGHT_ALT: return "RAL";
        case HID_USAGE_KEY_KEYBOARD_LEFT_GUI: return "LGU";
        case HID_USAGE_KEY_KEYBOARD_RIGHT_GUI: return "RGU";
        case HID_USAGE_KEY_KEYBOARD_UP_ARROW: return "UP";
        case HID_USAGE_KEY_KEYBOARD_DOWN_ARROW: return "DN";
        case HID_USAGE_KEY_KEYBOARD_LEFT_ARROW: return "LF";
        case HID_USAGE_KEY_KEYBOARD_RIGHT_ARROW: return "RT";
        case HID_USAGE_KEY_KEYBOARD_F1: return "F1";
        case HID_USAGE_KEY_KEYBOARD_F2: return "F2";
        case HID_USAGE_KEY_KEYBOARD_F3: return "F3";
        case HID_USAGE_KEY_KEYBOARD_F4: return "F4";
        case HID_USAGE_KEY_KEYBOARD_F5: return "F5";
        case HID_USAGE_KEY_KEYBOARD_F6: return "F6";
        case HID_USAGE_KEY_KEYBOARD_F7: return "F7";
        case HID_USAGE_KEY_KEYBOARD_F8: return "F8";
        case HID_USAGE_KEY_KEYBOARD_F9: return "F9";
        case HID_USAGE_KEY_KEYBOARD_F10: return "F10";
        case HID_USAGE_KEY_KEYBOARD_F11: return "F11";
        case HID_USAGE_KEY_KEYBOARD_F12: return "F12";
        default: return "???";
    }
}

static void set_pressed_key_display(struct zmk_widget_pressed_key *widget, 
                                   struct pressed_key_state state) {
    if (state.pressed) {
        // Show the pressed key
        const char* key_str = keycode_to_string(state.keycode);
        lv_label_set_text(widget->key_label, key_str);
        
        // Show position if available
        char pos_text[8];
        snprintf(pos_text, sizeof(pos_text), "P%d", (int)state.position);
        lv_label_set_text(widget->position_label, pos_text);
        
        // Show timestamp (in ms, last 3 digits)
        char time_text[8];
        snprintf(time_text, sizeof(time_text), "%03d", (int)(state.timestamp % 1000));
        lv_label_set_text(widget->timestamp_label, time_text);
        
        // Make visible
        lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
        
        LOG_DBG("Key pressed: %s at position %d", key_str, (int)state.position);
    } else {
        // Clear labels but keep widget visible briefly
        lv_label_set_text(widget->key_label, "---");
        lv_label_set_text(widget->position_label, "---");
        lv_label_set_text(widget->timestamp_label, "---");
        
        // Could add a timer here to hide after delay if desired
        LOG_DBG("Key released");
    }
}

static void pressed_key_update_cb(struct pressed_key_state state) {
    struct zmk_widget_pressed_key *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_pressed_key_display(widget, state);
    }
}

static struct pressed_key_state get_state(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    return (struct pressed_key_state) {
        .keycode = ev->keycode,
        .pressed = ev->state,
        .position = ev->position,
        .timestamp = ev->timestamp
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_pressed_key, struct pressed_key_state,
                            pressed_key_update_cb, get_state)

ZMK_SUBSCRIPTION(widget_pressed_key, zmk_keycode_state_changed);

int zmk_widget_pressed_key_init(struct zmk_widget_pressed_key *widget, lv_obj_t *parent) {
    // Create main container - sized for 128x32 display vertical layout
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 120, 30);  // Leave some margin
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_0, LV_PART_MAIN);  // Transparent background
    lv_obj_set_style_border_width(widget->obj, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 2, LV_PART_MAIN);
    
    // Create key label - main key display
    widget->key_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->key_label, "---");
    lv_obj_set_style_text_font(widget->key_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(widget->key_label, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create position label - smaller, on the right
    widget->position_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->position_label, "---");
    lv_obj_set_style_text_font(widget->position_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(widget->position_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    // Create timestamp label - smaller, on the left
    widget->timestamp_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->timestamp_label, "---");
    lv_obj_set_style_text_font(widget->timestamp_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(widget->timestamp_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    
    // Position the widget on the right side of a 128x32 display
    lv_obj_align(widget->obj, LV_ALIGN_TOP_RIGHT, -4, 2);
    
    // Initially visible (show waiting state)
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
    
    sys_slist_append(&widgets, &widget->node);
    widget_pressed_key_init();
    
    LOG_DBG("Pressed key widget initialized");
    return 0;
}

lv_obj_t *zmk_widget_pressed_key_obj(struct zmk_widget_pressed_key *widget) {
    return widget->obj;
}