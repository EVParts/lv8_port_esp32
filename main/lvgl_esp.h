#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void lv_esp_user_gui_init(void);

esp_err_t lv_esp_init();

#ifdef __cplusplus
}
#endif