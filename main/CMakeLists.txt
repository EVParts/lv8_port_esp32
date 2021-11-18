set(SOURCES main.cpp dash.c mono_mmm_5_36.c can.cpp lvgl_esp.c)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS .
                    REQUIRES lvgl_esp32_drivers lvgl_touch lvgl_tft lvgl lv_examples freertos esp_common)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)
