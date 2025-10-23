# Copyright (c) 2024
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_DISPLAY AND CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
    zephyr_library()
    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(widgets/pressed_key.c)
    
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()