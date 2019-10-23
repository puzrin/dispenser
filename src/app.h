#ifndef __APP__
#define __APP__
#ifdef __cplusplus
  extern "C" {
#endif

#include "lvgl.h"

#define LIST_MARGIN_LEFT 5
#define LIST_MARGIN_RIGHT 5
#define LIST_MARGIN_BOTTOM 5
#define LIST_MARGIN_TOP 22

// For hal driver
#define HACK_EVENT_RELEASED 0xFE

typedef struct {
    // Physics
    float needle_dia;
    float syringe_dia;
    float viscosity;
    float flux_percent;

    float dose_volume;
    float speed_scale;

    // UI
    bool flow_mode; // true => endless flow, false - portion
    uint8_t lcd_brightness;

    int16_t screen_dose_scroll_pos;
    int16_t screen_flow_scroll_pos;
    int16_t screen_settings_scroll_pos;
    int8_t screen_settings_selected_id;
    bool skip_key_enter_release;

    struct {
        lv_style_t main;
        lv_style_t list_title;
        lv_style_t list_desc;
        lv_style_t setting_desc;
    } styles;

    lv_indev_t * kbd;
    lv_group_t * group;

} app_data_t;


extern app_data_t app_data;
void app_update_settings();
void app_mode_switch();
void app_screen_create(bool to_settings);

#ifdef __cplusplus
  }
#endif
#endif
