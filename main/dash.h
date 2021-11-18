#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dash dash_t;

typedef struct dash_value {
  char const * title;
  char const * format;
  lv_obj_t* _label;
  enum {
      DASH_NO_DATA,
      DASH_INT,
      DASH_FLOAT,
  } _value_type;
  union {
      int int_value;
      float float_value;
  } _current;
} dash_value_t;

dash_t* dash_new(void);
void dash_free(dash_t* dash);

dash_value_t dash_new_value(dash_t* dash, 
  char const * title, char const * format);

void dash_update_float(dash_value_t* desc, float new_value);
void dash_update_int(dash_value_t* desc, int new_value);

void dash_flush_value(dash_value_t const * desc);

#ifdef __cplusplus
}
#endif