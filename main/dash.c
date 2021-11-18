#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "lvgl/lvgl.h"
#include "dash.h"

// LV_FONT_DECLARE(mono_mmm_5_40);
LV_FONT_DECLARE(mono_mmm_5_36);

#define BG_BLACK 0x0c0c0c

typedef struct dash_screen dash_screen_t;
typedef struct dash_screen {
  lv_obj_t* screen;
  lv_obj_t* container;
  dash_screen_t* next_screen;
} dash_screen_t;

#define DASH_COLS 2
#define DASH_ROWS 3

typedef struct dash {
  struct {
    int col;
    int row;
  } next;
  dash_screen_t* current_screen;
  // Linked list:
  dash_screen_t* head_screen;
  dash_screen_t* tail_screen;
} dash_t;

dash_t* dash_new(void) {
  dash_t* dash = calloc(1, sizeof(dash_t));
  return dash;
}

void dash_free(dash_t* dash) {
  lv_scr_load(dash->head_screen->screen);
  dash_screen_t* current = dash->head_screen;
  while (current) {
    if (lv_scr_act() == current->screen) {
      lv_obj_del(current->container);
    } else {
      lv_obj_del(current->screen);
    }
    dash_screen_t* next = current->next_screen;
    free(current);
    current = next;
  }
  free(dash);
}

static lv_obj_t* lv_create_reading_box(char const * title, lv_obj_t* parent, lv_obj_t** container_out) {
  // Style
  static bool _style_init = false;
  static lv_style_t style_reading_label;
  static lv_style_t style_reading_box;
  if (!_style_init) {
    lv_style_init(&style_reading_label);
    lv_style_init(&style_reading_box);
    lv_style_set_text_font(&style_reading_label, &mono_mmm_5_36);
    lv_style_set_flex_flow(&style_reading_box, LV_FLEX_FLOW_COLUMN_WRAP);
    lv_style_set_flex_main_place(&style_reading_box, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&style_reading_box, LV_LAYOUT_FLEX);
    lv_style_set_opa(&style_reading_box, LV_OPA_90);
    lv_style_set_min_width(&style_reading_box, 160);
    _style_init = true;
  }

  // Box
  lv_obj_t* box = lv_obj_create(parent);
  lv_obj_add_style(box, &style_reading_box, 0);
  lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_t* box_title = lv_label_create(box);
  lv_label_set_text(box_title, title);
  lv_obj_t* reading_label = lv_label_create(box);
  lv_obj_add_style(reading_label, &style_reading_label, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);

  if (container_out) {
    *container_out = box;
  }
  return reading_label;
}

static const lv_coord_t GRID_COLS[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static const lv_coord_t GRID_ROWS[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

static void dash_tapped_cb(lv_event_t* event) {
  dash_t* dash = event->user_data;
  dash_screen_t* next_screen = dash->current_screen->next_screen;
  if (next_screen) {
    dash->current_screen = next_screen;
  } else {
    dash->current_screen = dash->head_screen;
  }
  lv_scr_load(dash->current_screen->screen);
}

static lv_style_t* dash_screen_style() {
  static bool _style_init = false;
  static lv_style_t style;
  if (!_style_init) {
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(BG_BLACK));
    _style_init = true;
  }
  return &style;
}

dash_value_t dash_new_value(
  dash_t* dash, char const * title, char const * format
) {
  dash_screen_t* screen = dash->tail_screen;
  if (dash->next.col == 0 && dash->next.row == 0) {
    /* Allocate new screen */
    bool first_screen = !dash->head_screen;
    dash_screen_t* new_screen = malloc(sizeof(dash_value_t));

    new_screen->screen = first_screen ? lv_scr_act() : lv_obj_create(NULL);
    new_screen->next_screen = NULL;

    lv_obj_set_size(new_screen->screen, LV_HOR_RES, LV_VER_RES);
    if (first_screen) {
      dash->head_screen = new_screen;
      dash->tail_screen = new_screen;
      dash->current_screen = new_screen;
    } else {
      screen->next_screen = new_screen;
      dash->tail_screen = new_screen;
    }

    // Setup grid
    new_screen->container = lv_obj_create(new_screen->screen);
    lv_obj_set_size(new_screen->container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_grid_dsc_array(new_screen->container, GRID_COLS, GRID_ROWS);
    lv_obj_center(new_screen->container);
    lv_obj_add_style(new_screen->container, dash_screen_style(), 0);
    lv_obj_add_event_cb(new_screen->container, dash_tapped_cb, LV_EVENT_CLICKED, dash);
    screen = new_screen;
  }

  lv_obj_t* reading_box;
  dash_value_t reading = {
    .title = title,
    .format = format,
    ._label = lv_create_reading_box(title, screen->container, &reading_box),
    ._value_type = DASH_NO_DATA
  };
  lv_label_set_text(reading._label, "<no data>");

  lv_obj_set_grid_cell(reading_box,
    LV_GRID_ALIGN_STRETCH, dash->next.col, 1,
    LV_GRID_ALIGN_STRETCH, dash->next.row, 1);

  // Go to next slot
  dash->next.col = (dash->next.col + 1) % DASH_COLS;
  if (dash->next.col == 0) {
    dash->next.row = (dash->next.row + 1) % DASH_ROWS;
  }
  return reading;
} 

static char* float_to_string(double v, char const * float_format) {
  int len = snprintf(NULL, 0, float_format, v);
  char* result = (char *)malloc(len + 1);
  snprintf(result, len + 1, float_format, v);
  return result;
}

void dash_update_float(dash_value_t* desc, float new_value) {
  desc->_current.float_value = new_value;
  desc->_value_type = DASH_FLOAT;
}

void dash_update_int(dash_value_t* desc, int new_value) {
  desc->_current.int_value = new_value;
  desc->_value_type = DASH_INT;
}

void dash_flush_value(dash_value_t const * desc) {
    switch (desc->_value_type) {
      case DASH_FLOAT: {
        char* float_label = float_to_string(desc->_current.float_value, desc->format);
        lv_label_set_text(desc->_label, float_label);
        free(float_label);
        return;
      }
      case DASH_INT: {
        lv_label_set_text_fmt(desc->_label, desc->format, desc->_current.int_value);
      }
      case DASH_NO_DATA: return;
    }
}
