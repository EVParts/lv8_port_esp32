#include "can.h"

#include <esp_log.h>

CANBus::CANBus(gpio_num_t can_tx, gpio_num_t can_rx,
               twai_mode_t can_mode,
               twai_timing_config_t timing_config,
               twai_filter_config_t filter_config)
    {
        this->config = TWAI_GENERAL_CONFIG_DEFAULT(can_tx, can_rx, can_mode);
        this->timing_config = timing_config;
        this->filter_config = filter_config;
    }

constexpr auto LOG_TAG = "CANBus";

esp_err_t CANBus::start() {
    if (this->state == State::NOT_STARTED) {
        if (twai_driver_install(&this->config, &this->timing_config, &this->filter_config) != ESP_OK) {
            ESP_LOGE(LOG_TAG, "Failed to install driver");
            return ESP_FAIL;
        }
        ESP_LOGI(LOG_TAG, "Driver installed");
        this->state = State::DRIVER_INSTALLED;
    }
    if (twai_start() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to start driver");
        return ESP_FAIL;
    }
    ESP_LOGI(LOG_TAG, "CAN started");
    this->state = State::STARTED;
    return ESP_OK;
}

esp_err_t CANBus::stop() {
    if (this->state != State::STARTED) {
        return ESP_FAIL;
    }
    this->state = State::DRIVER_INSTALLED;
    return twai_stop();
}

CANBus::~CANBus() {
    if (this->state == State::STARTED) {
        (void) this->stop();
    }
    if (this->state == State::DRIVER_INSTALLED) {
        (void) twai_driver_uninstall();
    }
}

esp_err_t CANBus::wait_for_msg(twai_message_t& message, TickType_t timeout) const {
    esp_err_t rx_status = twai_receive(&message, timeout);
    if (rx_status == ESP_OK) {
        return ESP_OK;
    }
    switch (rx_status) {
        case ESP_OK: return ESP_OK;
        default:
        case ESP_ERR_TIMEOUT:
        case ESP_ERR_INVALID_ARG:
        case ESP_ERR_INVALID_STATE: {
            ESP_LOGE(LOG_TAG, "message recv failed: %s", esp_err_to_name(rx_status));
            return rx_status;
        }
    }
}
