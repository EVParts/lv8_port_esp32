#pragma once

#include <driver/gpio.h>
#include <driver/twai.h>
#include <freertos/FreeRTOS.h>

class CANBus {
    twai_general_config_t config;
    twai_timing_config_t timing_config;
    twai_filter_config_t filter_config;

    enum class State {
        NOT_STARTED,
        DRIVER_INSTALLED,
        STOPPED,
        STARTED
    } state = State::NOT_STARTED;
public:
    CANBus(gpio_num_t can_tx, gpio_num_t can_rx,
           twai_mode_t can_mode = TWAI_MODE_NORMAL,
           twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS(),
           twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL());

    ~CANBus();

    esp_err_t start();
    esp_err_t stop();
    esp_err_t wait_for_msg(twai_message_t& message, TickType_t timeout) const;
};