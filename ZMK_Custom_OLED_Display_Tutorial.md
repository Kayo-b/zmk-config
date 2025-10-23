# ZMK Custom OLED Display Tutorial

## Table of Contents
1. [Introduction](#introduction)
2. [Understanding the Architecture](#understanding-the-architecture)
3. [Basic Widget Structure](#basic-widget-structure)
4. [Creating a Pressed Key Widget](#creating-a-pressed-key-widget)
5. [Integration Guide](#integration-guide)
6. [Configuration Options](#configuration-options)
7. [Advanced Topics](#advanced-topics)
8. [Troubleshooting](#troubleshooting)

## Introduction

This tutorial explains how to create custom OLED display UIs for ZMK (Zephyr Mechanical Keyboard) firmware, with a focus on creating a widget that displays the currently pressed key. The tutorial is based on the dongle-display project architecture and uses LVGL (LittlevGL) for graphics rendering.

### Prerequisites
- Basic understanding of C programming
- ZMK firmware knowledge
- LVGL familiarity (helpful but not required)
- Hardware: MCU with I2C OLED display (128x64 or 128x32 pixels)

### Key Technologies
- **ZMK**: Mechanical keyboard firmware based on Zephyr RTOS
- **LVGL**: Graphics library for embedded systems
- **Zephyr Event System**: For handling keyboard events
- **I2C**: Communication protocol for OLED displays

## Understanding the Architecture

### Display System Overview

The ZMK display system follows a modular widget-based architecture:

```
┌─────────────────────────────────────┐
│           Main Screen               │
│  ┌─────────┐ ┌─────────┐ ┌────────┐ │
│  │Widget 1 │ │Widget 2 │ │Widget 3│ │
│  │         │ │         │ │        │ │
│  └─────────┘ └─────────┘ └────────┘ │
│  ┌─────────┐ ┌─────────┐           │
│  │Widget 4 │ │Widget 5 │           │
│  │         │ │         │           │
│  └─────────┘ └─────────┘           │
└─────────────────────────────────────┘
```

### Event System

ZMK uses an event-driven architecture where widgets subscribe to specific events:

```c
// Event flow
Keypress → ZMK Event Manager → Widget Listeners → Display Update
```

### File Structure

A typical ZMK display project has this structure:

```
boards/shields/your_display/
├── CMakeLists.txt              # Build configuration
├── Kconfig.defconfig           # Default configuration
├── Kconfig.shield              # Shield definition
├── your_display.conf           # Display configuration
├── your_display.overlay        # Device tree overlay
├── custom_status_screen.c      # Main screen implementation
├── custom_status_screen.h      # Main screen header
└── widgets/                    # Widget implementations
    ├── widget_name.c
    ├── widget_name.h
    └── widget_symbols.c        # Graphics/symbols
```

## Basic Widget Structure

### Widget Header File Pattern

Every widget follows a consistent pattern. Here's the basic structure:

```c
// widgets/pressed_key.h
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_pressed_key {
    sys_snode_t node;      // Linked list node
    lv_obj_t *obj;         // Main LVGL object
    lv_obj_t *key_label;   // Label for displaying key
};

int zmk_widget_pressed_key_init(struct zmk_widget_pressed_key *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_pressed_key_obj(struct zmk_widget_pressed_key *widget);
```

### Widget Implementation Pattern

All widgets follow this implementation pattern:

```c
// widgets/pressed_key.c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#include "pressed_key.h"

// Global widget list for managing multiple instances
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// Widget state structure
struct pressed_key_state {
    uint32_t keycode;
    bool pressed;
    uint32_t position;
};

// Update function called when state changes
static void pressed_key_update_cb(struct pressed_key_state state) {
    struct zmk_widget_pressed_key *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_pressed_key_display(widget, state);
    }
}

// Extract state from ZMK event
static struct pressed_key_state get_state(const zmk_event_t *eh) {
    // Implementation details below
}

// Macro to register widget listener
ZMK_DISPLAY_WIDGET_LISTENER(widget_pressed_key, struct pressed_key_state,
                            pressed_key_update_cb, get_state)

// Subscribe to events
ZMK_SUBSCRIPTION(widget_pressed_key, zmk_keycode_state_changed);

// Widget initialization
int zmk_widget_pressed_key_init(struct zmk_widget_pressed_key *widget, lv_obj_t *parent) {
    // Create LVGL objects
    // Add to widget list
    // Initialize widget listener
}

// Widget object getter
lv_obj_t *zmk_widget_pressed_key_obj(struct zmk_widget_pressed_key *widget) {
    return widget->obj;
}
```

## Creating a Pressed Key Widget

This section demonstrates creating a pressed key widget optimized for a 128×32 OLED display positioned on the right side with vertical layout.

### Implementation Overview

The pressed key widget displays:
- **Main key label**: The currently pressed key (large font)
- **Position label**: Key position in matrix (small font, bottom-left)
- **Timestamp label**: Last 3 digits of timestamp (small font, bottom-right)

### Widget Structure for 128×32 Display

For a 128×32 OLED display, we need to optimize space usage. The widget is positioned on the right side and displays information vertically to make the most of the limited height.

### Step 1: Create the Header File

```c
// widgets/pressed_key.h
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
```

### Step 2: Implement the Widget for 128×32 Display

The implementation is optimized for the smaller display dimensions:

```c
// widgets/pressed_key.c - Key sections only
static void set_pressed_key_display(struct zmk_widget_pressed_key *widget, 
                                   struct pressed_key_state state) {
    if (state.pressed) {
        // Show the pressed key
        const char* key_str = keycode_to_string(state.keycode);
        lv_label_set_text(widget->key_label, key_str);
        
        // Show position (abbreviated for space)
        char pos_text[8];
        snprintf(pos_text, sizeof(pos_text), "P%d", (int)state.position);
        lv_label_set_text(widget->position_label, pos_text);
        
        // Show timestamp (last 3 digits)
        char time_text[8];
        snprintf(time_text, sizeof(time_text), "%03d", (int)(state.timestamp % 1000));
        lv_label_set_text(widget->timestamp_label, time_text);
        
        // Make visible
        lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Clear labels but keep widget visible briefly
        lv_label_set_text(widget->key_label, "---");
        lv_label_set_text(widget->position_label, "---");
        lv_label_set_text(widget->timestamp_label, "---");
    }
}

int zmk_widget_pressed_key_init(struct zmk_widget_pressed_key *widget, lv_obj_t *parent) {
    // Create main container - sized for 128x32 display
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 120, 30);  // Leave margins
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 1, LV_PART_MAIN);
    
    // Create key label - main display with larger font
    widget->key_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->key_label, "---");
    lv_obj_set_style_text_font(widget->key_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(widget->key_label, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create position and timestamp labels with smaller font
    widget->position_label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->position_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(widget->position_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    widget->timestamp_label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->timestamp_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(widget->timestamp_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    
    // Position on right side of 128x32 display
    lv_obj_align(widget->obj, LV_ALIGN_TOP_RIGHT, -4, 2);
    
    sys_slist_append(&widgets, &widget->node);
    widget_pressed_key_init();
    
    return 0;
}
```

### Step 3: Custom Status Screen for 128×32

The custom status screen arranges widgets efficiently for the small display:

```c
// custom_status_screen.c
lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_opa(screen, LV_OPA_0, LV_PART_MAIN);
    
    // Arrange standard widgets on left side (64px width)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 0, 10);
    
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 20);
    
    // WPM at bottom left
    zmk_widget_wpm_status_init(&wpm_status_widget, screen);
    lv_obj_align(zmk_widget_wpm_status_obj(&wpm_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    // Pressed key widget on right side (64px width)
    zmk_widget_pressed_key_init(&pressed_key_widget, screen);
    
    return screen;
}
```

### Physical Implementation

To use this with your Corne keyboard:

1. **Build Configuration**: Use the `corne_custom_right` shield for the right half
2. **OLED Connection**: Connect your 128×32 SSD1306 OLED to I2C pins
3. **Display Position**: The widget is positioned on the right side of the screen

### Configuration

Add to your `config/corne.conf`:

```ini
# Enable custom pressed key widget display
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y
CONFIG_ZMK_WIDGET_PRESSED_KEY=y

# 128x32 display optimizations
CONFIG_LV_Z_VDB_SIZE=64
CONFIG_LV_Z_MEM_POOL_SIZE=8192
CONFIG_LV_Z_BITS_PER_PIXEL=1
```

### Visual Layout

```
128×32 OLED Display Layout:
┌─────────────────────────────────────────────────────────┐
│ Layer: 0      │                │  [A] ← Current key     │
│ Batt: 85%     │                │                        │
│ Out: USB      │                │  P42      158          │
│               │                │   ↑       ↑           │
│ WPM: 45       │                │ Position  Time         │
└─────────────────────────────────────────────────────────┘
 ← Left side (64px)               → Right side (64px) →
```
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

// Keycode to string conversion
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

    // Special keys
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
        char pos_text[16];
        snprintf(pos_text, sizeof(pos_text), "P:%d", (int)state.position);
        lv_label_set_text(widget->position_label, pos_text);
        
        // Make visible
        lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Hide when no key is pressed
        lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
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
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 60, 30);
    
    // Create key label
    widget->key_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->key_label, "---");
    lv_obj_align(widget->key_label, LV_ALIGN_TOP_MID, 0, 2);
    
    // Create position label
    widget->position_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->position_label, "P:--");
    lv_obj_align(widget->position_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // Initially hidden
    lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_HIDDEN);
    
    sys_slist_append(&widgets, &widget->node);
    widget_pressed_key_init();
    
    return 0;
}

lv_obj_t *zmk_widget_pressed_key_obj(struct zmk_widget_pressed_key *widget) {
    return widget->obj;
}
```

### Step 3: Enhanced Version with Key History

For a more advanced implementation that shows the last few pressed keys:

```c
// Enhanced pressed_key.c with history
#define MAX_KEY_HISTORY 3

struct pressed_key_state {
    uint32_t recent_keys[MAX_KEY_HISTORY];
    uint8_t history_count;
    uint32_t current_keycode;
    bool pressed;
};

static void add_to_history(struct pressed_key_state *state, uint32_t keycode) {
    // Shift existing keys
    for (int i = MAX_KEY_HISTORY - 1; i > 0; i--) {
        state->recent_keys[i] = state->recent_keys[i - 1];
    }
    
    // Add new key
    state->recent_keys[0] = keycode;
    if (state->history_count < MAX_KEY_HISTORY) {
        state->history_count++;
    }
}

static void set_key_history_display(struct zmk_widget_pressed_key *widget,
                                   struct pressed_key_state state) {
    char history_text[64] = {0};
    
    if (state.pressed) {
        // Show current key prominently
        const char* current_key = keycode_to_string(state.current_keycode);
        snprintf(history_text, sizeof(history_text), "[%s]", current_key);
    } else if (state.history_count > 0) {
        // Show recent key history
        strcat(history_text, "Last: ");
        for (int i = 0; i < state.history_count && i < 3; i++) {
            if (i > 0) strcat(history_text, " ");
            strcat(history_text, keycode_to_string(state.recent_keys[i]));
        }
    } else {
        strcpy(history_text, "No keys");
    }
    
    lv_label_set_text(widget->key_label, history_text);
}
```

## Integration Guide

### Step 1: Add Widget to Custom Status Screen

```c
// custom_status_screen.c
#include "widgets/pressed_key.h"

// Add widget instance
static struct zmk_widget_pressed_key pressed_key_widget;

// In zmk_display_status_screen() function:
lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // ... other widget initialization ...
    
    // Initialize pressed key widget
    zmk_widget_pressed_key_init(&pressed_key_widget, screen);
    lv_obj_align(zmk_widget_pressed_key_obj(&pressed_key_widget), 
                 LV_ALIGN_CENTER, 0, 0);
    
    return screen;
}
```

### Step 2: Update CMakeLists.txt

```cmake
# CMakeLists.txt
if(CONFIG_ZMK_DISPLAY AND CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
    zephyr_library()
    # ... existing sources ...
    zephyr_library_sources(widgets/pressed_key.c)
endif()
```

### Step 3: Add Configuration Options

```bash
# Kconfig.defconfig
config ZMK_DONGLE_DISPLAY_PRESSED_KEY
    bool "Display the currently pressed key"
    default y

config ZMK_DONGLE_DISPLAY_KEY_HISTORY
    bool "Show recent key press history"
    depends on ZMK_DONGLE_DISPLAY_PRESSED_KEY
    default n
```

### Step 4: Conditional Compilation

```c
// custom_status_screen.c
#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_PRESSED_KEY)
static struct zmk_widget_pressed_key pressed_key_widget;
#endif

// In initialization:
#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_PRESSED_KEY)
    zmk_widget_pressed_key_init(&pressed_key_widget, screen);
    lv_obj_align(zmk_widget_pressed_key_obj(&pressed_key_widget), 
                 LV_ALIGN_CENTER, 0, 0);
#endif
```

## Configuration Options

### Display Configuration

```ini
# your_display.conf

# Enable display
CONFIG_ZMK_DISPLAY=y

# Use custom status screen
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y

# Enable pressed key widget
CONFIG_ZMK_DONGLE_DISPLAY_PRESSED_KEY=y

# Optional: Enable key history
CONFIG_ZMK_DONGLE_DISPLAY_KEY_HISTORY=y

# Optional: Configure display settings
CONFIG_LV_Z_VDB_SIZE=64          # Display buffer size
CONFIG_LV_Z_MEM_POOL_SIZE=8192   # Memory pool size
CONFIG_LV_DPI_DEF=148            # Display DPI
CONFIG_LV_Z_BITS_PER_PIXEL=1     # Monochrome display
```

### Device Tree Configuration

```dts
// your_display.overlay
&i2c0 {
    oled: ssd1306@3c {
        compatible = "solomon,ssd1306fb";
        reg = <0x3c>;
        width = <128>;
        height = <64>;
        segment-offset = <0>;
        page-offset = <0>;
        display-offset = <0>;
        multiplex-ratio = <63>;
        segment-remap;
        com-invdir;
        prechargep = <0x22>;
    };
};

/ {
    chosen {
        zephyr,display = &oled;
    };
};
```

## Advanced Topics

### Custom Graphics and Symbols

You can create custom graphics for your widgets:

```c
// widgets/pressed_key_symbols.c
#include <lvgl.h>

#ifndef LV_ATTRIBUTE_IMG_KEY_ICON
#define LV_ATTRIBUTE_IMG_KEY_ICON
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_KEY_ICON 
uint8_t key_icon_map[] = {
    0xff, 0xff, 0xff, 0xff,  /*Color of index 0*/
    0x00, 0x00, 0x00, 0xff,  /*Color of index 1*/
    
    // Bitmap data for a key icon
    0x00, 0x00,
    0x7f, 0xfe,
    0x40, 0x02,
    0x5f, 0xfa,
    0x40, 0x02,
    0x7f, 0xfe,
    0x00, 0x00,
};

const lv_img_dsc_t key_icon = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 16,
    .header.h = 7,
    .data_size = 22,
    .data = key_icon_map,
};
```

### Animation Support

Add animations to your widgets:

```c
// In your widget implementation
static void animate_key_press(lv_obj_t *obj) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_time(&a, 200);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(obj), lv_obj_get_y(obj) - 2);
    lv_anim_set_path_cb(&a, lv_anim_path_bounce);
    lv_anim_start(&a);
}
```

### Multi-Event Widgets

Subscribe to multiple events:

```c
// Multiple event subscriptions
ZMK_SUBSCRIPTION(widget_pressed_key, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_pressed_key, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(widget_pressed_key, zmk_modifiers_changed);

// Handle different event types in get_state()
static struct pressed_key_state get_state(const zmk_event_t *eh) {
    if (as_zmk_keycode_state_changed(eh) != NULL) {
        // Handle keycode events
    } else if (as_zmk_layer_state_changed(eh) != NULL) {
        // Handle layer changes
    }
    // Return appropriate state
}
```

### Performance Optimization

For smooth display updates:

```c
// Debounce rapid updates
#define UPDATE_THROTTLE_MS 50
static int64_t last_update_time = 0;

static void throttled_update_cb(struct pressed_key_state state) {
    int64_t current_time = k_uptime_get();
    if (current_time - last_update_time < UPDATE_THROTTLE_MS) {
        return; // Skip this update
    }
    last_update_time = current_time;
    
    // Proceed with update
    pressed_key_update_cb(state);
}
```

## Troubleshooting

### Common Issues

1. **Widget Not Displaying**
   - Check that the widget is properly initialized
   - Verify the widget is not hidden: `lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_HIDDEN)`
   - Ensure proper alignment and sizing

2. **Events Not Triggering**
   - Verify event subscription with `ZMK_SUBSCRIPTION`
   - Check that `widget_[name]_init()` is called
   - Ensure the event type matches your listener

3. **Memory Issues**
   - Increase `CONFIG_LV_Z_MEM_POOL_SIZE`
   - Reduce `CONFIG_LV_Z_VDB_SIZE` for smaller displays
   - Check for memory leaks in widget cleanup

4. **Display Corruption**
   - Verify I2C configuration in device tree
   - Check display dimensions match hardware
   - Ensure proper power supply to display

### Debugging Tips

```c
// Add logging to your widgets
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// In your functions:
LOG_DBG("Key pressed: %d", state.keycode);
LOG_WRN("Widget initialization failed");
LOG_ERR("Critical display error");
```

### Build Issues

```bash
# Clean build when adding new files
west build -t clean
west build

# Check for missing includes
west build -v | grep "undefined reference"
```

## Example Projects

### Minimal Pressed Key Widget

Here's a complete minimal example you can use as a starting point:

```c
// minimal_pressed_key.h
#pragma once
#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_minimal_key {
    sys_snode_t node;
    lv_obj_t *label;
};

int zmk_widget_minimal_key_init(struct zmk_widget_minimal_key *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_minimal_key_obj(struct zmk_widget_minimal_key *widget);
```

```c
// minimal_pressed_key.c
#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include "minimal_pressed_key.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct key_state { uint32_t keycode; bool pressed; };

static void update_cb(struct key_state state) {
    struct zmk_widget_minimal_key *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (state.pressed) {
            char text[8];
            snprintf(text, sizeof(text), "K:%d", (int)state.keycode);
            lv_label_set_text(widget->label, text);
        } else {
            lv_label_set_text(widget->label, "---");
        }
    }
}

static struct key_state get_state(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    return (struct key_state){.keycode = ev->keycode, .pressed = ev->state};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_minimal_key, struct key_state, update_cb, get_state)
ZMK_SUBSCRIPTION(widget_minimal_key, zmk_keycode_state_changed);

int zmk_widget_minimal_key_init(struct zmk_widget_minimal_key *widget, lv_obj_t *parent) {
    widget->label = lv_label_create(parent);
    lv_label_set_text(widget->label, "---");
    sys_slist_append(&widgets, &widget->node);
    widget_minimal_key_init();
    return 0;
}

lv_obj_t *zmk_widget_minimal_key_obj(struct zmk_widget_minimal_key *widget) {
    return widget->label;
}
```

This tutorial provides a comprehensive guide to creating custom OLED displays for ZMK firmware. Start with the minimal example and gradually add more features as you become comfortable with the architecture.

## Resources

- [ZMK Documentation](https://zmk.dev/docs)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/)
- [ZMK Display Feature Guide](https://zmk.dev/docs/features/displays)
- [Example Dongle Display Repository](https://github.com/englmaxi/zmk-dongle-display)