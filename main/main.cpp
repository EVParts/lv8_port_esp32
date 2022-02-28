#include <esp_log.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "can.h"
#include "lvgl_esp.h"
#include "dash.h"


constexpr auto LOG_TAG = "Leaf Can";
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_32;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_27;

static SemaphoreHandle_t g_lv_init_sema = nullptr;

static struct leaf_dash {
    dash_t* handle;
    dash_value_t voltage, current, soc, gids;
} g_leaf_dash;

static void leaf_dash_update(lv_timer_t*) {
    // this is called within the LVGL task
    dash_flush_value(&g_leaf_dash.voltage);
    dash_flush_value(&g_leaf_dash.current);
    dash_flush_value(&g_leaf_dash.soc);
    dash_flush_value(&g_leaf_dash.gids);
}

extern "C" void lv_esp_user_gui_init() {
    constexpr auto DISPLAY_UPDATE_INTERVAL_MS = 100;

    g_leaf_dash.handle = dash_new();
    g_leaf_dash.voltage = dash_new_value(
        g_leaf_dash.handle , "Voltage", "%d V");
    g_leaf_dash.current = dash_new_value(
        g_leaf_dash.handle , "Current", "%d A");
    g_leaf_dash.soc = dash_new_value(
        g_leaf_dash.handle , "SoC", "%d%%");
    g_leaf_dash.gids = dash_new_value(
        g_leaf_dash.handle , "Energy Left", "%d Gids");

    (void) lv_timer_create(leaf_dash_update, DISPLAY_UPDATE_INTERVAL_MS, nullptr);
    xSemaphoreGive(g_lv_init_sema);
}

extern "C" void app_main() {
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    g_lv_init_sema = xSemaphoreCreateBinary();
    assert(g_lv_init_sema);
    ESP_ERROR_CHECK(lv_esp_init());
    xSemaphoreTake(g_lv_init_sema, portMAX_DELAY);
    vSemaphoreDelete(g_lv_init_sema);
    g_lv_init_sema = nullptr;

    CANBus can_bus(CAN_TX_PIN, CAN_RX_PIN);
    ESP_ERROR_CHECK(can_bus.start());

    while (true) {
        twai_message_t can_message;
        if (can_bus.wait_for_msg(can_message, pdMS_TO_TICKS(1000)) == ESP_OK) {
            ESP_LOGI(LOG_TAG, "Message received: ID=0x%03x, DLC=%d", can_message.identifier, can_message.data_length_code);
            ESP_LOG_BUFFER_HEXDUMP(LOG_TAG, can_message.data, can_message.data_length_code, ESP_LOG_INFO);

            dash_update_int(&g_leaf_dash.voltage, can_message.data[0]);
            dash_update_int(&g_leaf_dash.current, can_message.data[1]);
            dash_update_int(&g_leaf_dash.soc, can_message.data[2]);
            dash_update_int(&g_leaf_dash.gids, can_message.data[3]);
        }
    }
}

