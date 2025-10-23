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
        char display_text[16];
        snprintf(display_text, sizeof(display_text), "KEY: %s", key_str);
        lv_label_set_text(widget->key_label, display_text);
        
        LOG_DBG("Key pressed: %s at position %d", key_str, (int)state.position);
    } else {
        // Show ready state when no key is pressed
        lv_label_set_text(widget->key_label, "READY");
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
    // Create main container - simpler sizing for debugging
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 32);  // Full screen size
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_0, LV_PART_MAIN);  // Transparent background
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);   // No border for now
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);        // No padding
    
    // Create key label - main key display - use default font for now
    widget->key_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->key_label, "READY");  // Show "READY" initially instead of "---"
    lv_obj_align(widget->key_label, LV_ALIGN_CENTER, 0, 0);  // Center it
    
    // Skip the other labels for now to test basic functionality
    widget->position_label = NULL;
    widget->timestamp_label = NULL;
    
    // Position the widget to fill the screen
    lv_obj_align(widget->obj, LV_ALIGN_CENTER, 0, 0);
    
    // Make sure it's visible
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
    
    sys_slist_append(&widgets, &widget->node);
    widget_pressed_key_init();
    
    LOG_DBG("Pressed key widget initialized (simple version)");
    return 0;
}

lv_obj_t *zmk_widget_pressed_key_obj(struct zmk_widget_pressed_key *widget) {
    return widget->obj;
}