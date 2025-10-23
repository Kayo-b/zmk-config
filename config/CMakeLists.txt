# Custom ZMK Config CMakeLists.txt
# Enable custom status screen and widgets when display is enabled

if(CONFIG_ZMK_DISPLAY AND CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
    zephyr_library()
    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(widgets/bongo_cat.c)
    zephyr_library_sources(widgets/bongo_cat_images.c)
    
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()