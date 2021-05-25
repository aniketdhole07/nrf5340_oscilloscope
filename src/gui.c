#include "gui.h"
#include <device.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gui);

bool gui_initialized = false;
uint32_t count = 0U;
char count_str[11] = {0};
const struct device *display_dev;

static gui_event_t m_gui_event;
static gui_callback_t m_gui_callback = 0;

// Create a message queue for handling external GUI commands
K_MSGQ_DEFINE(m_gui_cmd_queue, sizeof(gui_message_t), 8, 4);

char *on_off_strings[2] = {"On", "Off"};

// GUI objects
lv_obj_t *top_header;
lv_obj_t *top_header_logo;
lv_obj_t *label_button, *label_led, *label_bt_state_hdr, *label_bt_state;
lv_obj_t *connected_background;
lv_obj_t *label_btn_state, *label_led_state;
lv_obj_t *btn1, *btn1_label;
lv_obj_t *checkbox_led;
lv_obj_t *image_led;
lv_obj_t *image_bg[12];

// Styles
lv_style_t style_btn, style_label, style_label_value, style_checkbox;
lv_style_t style_header, style_con_bg, style_chart, style_series;

// Fonts
LV_FONT_DECLARE(arial_20bold);
LV_FONT_DECLARE(calibri_20b);
LV_FONT_DECLARE(calibri_20);
LV_FONT_DECLARE(calibri_24b);
LV_FONT_DECLARE(calibri_32b);


// Data chart
lv_obj_t *chart;
lv_chart_series_t *temperature;

static void init_styles(void) {
  /*Create background style*/
  static lv_style_t style_screen;
  lv_style_set_bg_color(&style_screen, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xcb, 0xca, 0xff));
  lv_obj_add_style(lv_scr_act(), LV_BTN_PART_MAIN, &style_screen);

  /*Create the screen header label style*/
  lv_style_init(&style_header);
  lv_style_set_bg_opa(&style_header, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&style_header, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x1C, 0x11, 0xFD));
  lv_style_set_radius(&style_header, LV_STATE_DEFAULT, 8);
  lv_style_set_pad_left(&style_header, LV_STATE_DEFAULT, 70);
  lv_style_set_pad_top(&style_header, LV_STATE_DEFAULT, 30);
  lv_style_set_shadow_spread(&style_header, LV_STATE_DEFAULT, 1);
  lv_style_set_shadow_color(&style_header, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_shadow_opa(&style_header, LV_STATE_DEFAULT, 255);
  lv_style_set_shadow_width(&style_header, LV_STATE_DEFAULT, 1);
  lv_style_set_shadow_ofs_x(&style_header, LV_STATE_DEFAULT, 1);
  lv_style_set_shadow_ofs_y(&style_header, LV_STATE_DEFAULT, 2);
  lv_style_set_shadow_opa(&style_header, LV_STATE_DEFAULT, LV_OPA_50);

  /*Screen header text style*/
  lv_style_set_text_color(&style_header, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x7d, 0xce, 0xfd));
  lv_style_set_text_font(&style_header, LV_STATE_DEFAULT, &calibri_32b);

  lv_style_init(&style_con_bg);
  lv_style_copy(&style_con_bg, &style_header);
  lv_style_set_bg_color(&style_con_bg, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x69, 0xb0, 0x5a));
  lv_style_set_bg_opa(&style_con_bg, LV_STATE_DEFAULT, LV_OPA_50);
  lv_style_set_radius(&style_header, LV_STATE_DEFAULT, 4);

  /*Create a label style*/
  lv_style_init(&style_label);
  lv_style_set_bg_opa(&style_label, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&style_label, LV_STATE_DEFAULT, LV_COLOR_SILVER);
  lv_style_set_bg_grad_color(&style_label, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_style_set_bg_grad_dir(&style_label, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_pad_left(&style_label, LV_STATE_DEFAULT, 5);
  lv_style_set_pad_top(&style_label, LV_STATE_DEFAULT, 10);

  /*Add a border*/
  lv_style_set_border_color(&style_label, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_border_opa(&style_label, LV_STATE_DEFAULT, LV_OPA_70);
  lv_style_set_border_width(&style_label, LV_STATE_DEFAULT, 3);

  /*Set the text style*/
  lv_style_set_text_color(&style_label, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x00, 0x00, 0x30));
  lv_style_set_text_font(&style_label, LV_STATE_DEFAULT, &calibri_20b);

  /*Create a label value style*/
  lv_style_init(&style_label_value);
  lv_style_set_bg_opa(&style_label_value, LV_STATE_DEFAULT, LV_OPA_20);
  lv_style_set_bg_color(&style_label_value, LV_STATE_DEFAULT, LV_COLOR_SILVER);
  lv_style_set_bg_grad_color(&style_label_value, LV_STATE_DEFAULT, LV_COLOR_TEAL);
  lv_style_set_bg_grad_dir(&style_label_value, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_pad_left(&style_label_value, LV_STATE_DEFAULT, 0);
  lv_style_set_pad_top(&style_label_value, LV_STATE_DEFAULT, 3);

  /*Set the text style*/
  lv_style_set_text_color(&style_label_value, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x00, 0x00, 0x30));
  lv_style_set_text_font(&style_label_value, LV_STATE_DEFAULT, &calibri_20);

  /*Create a simple button style*/
  lv_style_init(&style_btn);
  lv_style_set_radius(&style_btn, LV_STATE_DEFAULT, 10);
  lv_style_set_bg_opa(&style_btn, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_SILVER);
  lv_style_set_bg_grad_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_style_set_bg_grad_dir(&style_btn, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_shadow_spread(&style_btn, LV_STATE_DEFAULT, 1);
  lv_style_set_shadow_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_style_set_shadow_opa(&style_btn, LV_STATE_DEFAULT, 255);
  lv_style_set_shadow_width(&style_btn, LV_STATE_DEFAULT, 1);

  /*Swap the colors in pressed state*/
  lv_style_set_bg_color(&style_btn, LV_STATE_PRESSED, LV_COLOR_GRAY);
  lv_style_set_bg_grad_color(&style_btn, LV_STATE_PRESSED, LV_COLOR_SILVER);

  /*Add a border*/
  lv_style_set_border_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_border_opa(&style_btn, LV_STATE_DEFAULT, LV_OPA_70);
  lv_style_set_border_width(&style_btn, LV_STATE_DEFAULT, 3);

  /*Different border color in focused state*/
  lv_style_set_border_color(&style_btn, LV_STATE_FOCUSED, LV_COLOR_BLACK);
  lv_style_set_border_color(&style_btn, LV_STATE_FOCUSED | LV_STATE_PRESSED, LV_COLOR_NAVY);

  /*Set the text style*/
  lv_style_set_text_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_TEAL);
  lv_style_set_text_font(&style_btn, LV_STATE_DEFAULT, &calibri_24b);

  /*Make the button smaller when pressed*/
  lv_style_set_transform_height(&style_btn, LV_STATE_PRESSED, -4);
  lv_style_set_transform_width(&style_btn, LV_STATE_PRESSED, -8);
#if LV_USE_ANIMATION
  /*Add a transition to the size change*/
  static lv_anim_path_t path;
  lv_anim_path_init(&path);
  lv_anim_path_set_cb(&path, lv_anim_path_overshoot);

  lv_style_set_transition_prop_1(&style_btn, LV_STATE_DEFAULT, LV_STYLE_TRANSFORM_HEIGHT);
  lv_style_set_transition_prop_2(&style_btn, LV_STATE_DEFAULT, LV_STYLE_TRANSFORM_WIDTH);
  lv_style_set_transition_time(&style_btn, LV_STATE_DEFAULT, 300);
  lv_style_set_transition_path(&style_btn, LV_STATE_DEFAULT, &path);
#endif
  lv_style_init(&style_chart);
  lv_style_set_radius(&style_chart, LV_STATE_DEFAULT, 0);
  lv_style_set_bg_opa(&style_chart, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&style_chart, LV_STATE_DEFAULT , LV_COLOR_ORANGE);

  lv_style_init(&style_series);
  lv_style_set_size(&style_series, LV_STATE_DEFAULT, 0);
}

void gui_add_point_to_chart(float val) {
  if(gui_initialized) {
    lv_chart_set_next(chart, temperature, (lv_coord_t)val);
    lv_chart_refresh(chart); /*Required after direct set*/

    char s[20];
    snprintk(s, sizeof(s), "Voltage: %d.%02d'C", (uint8_t)(val), (short)(val * 100) % 100);
    lv_label_set_text(label_btn_state, s);
  }
}

static void init_chart_gui(void) {
  /*Create a chart*/
  chart = lv_chart_create(lv_scr_act(), NULL);
  lv_obj_set_size(chart, 320, 220);
  
  lv_obj_align(chart, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE); /*Show lines and points too*/
  lv_chart_set_y_range(chart,LV_CHART_AXIS_PRIMARY_Y, 0, 400);
  lv_chart_set_point_count(chart, 200);
  lv_chart_set_x_tick_length(chart, 10, 1);
  lv_obj_add_style(chart, LV_CHART_PART_BG, &style_chart);
  lv_obj_add_style(chart, LV_CHART_PART_SERIES, &style_series);

  /* Create last value label */
  label_btn_state = lv_label_create(lv_scr_act(), NULL);
  lv_obj_align(label_btn_state, chart, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_label_set_long_mode(label_btn_state, LV_LABEL_LONG_DOT);
  lv_label_set_text(label_btn_state, "Voltage: ?");
  lv_label_set_align(label_btn_state, LV_LABEL_ALIGN_LEFT);
  lv_obj_add_style(label_btn_state, LV_LABEL_PART_MAIN, &style_label_value);
  lv_obj_set_size(label_btn_state, 130, 20);


  /*Add two data series*/
  temperature = lv_chart_add_series(chart, LV_COLOR_RED);
}

void gui_init(gui_config_t *config) {
  m_gui_callback = config->event_callback;
}


void gui_run(void) {
  display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

  if (display_dev == NULL) {
    LOG_ERR("Display device not found!");
    return;
  }

  init_styles();

  init_chart_gui();

  display_blanking_off(display_dev);

  gui_initialized = true;

  while (1) {
    lv_task_handler();
    k_sleep(K_MSEC(50));
  }
}

// Define our GUI thread, using a stack size of 4096 and a priority of 7
K_THREAD_DEFINE(gui_thread, 4096, gui_run, NULL, NULL, NULL, 7, 0, 0);