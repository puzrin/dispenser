#include "app.h"
#include "screen_flow.h"
#include "etl/to_string.h"
#include "etl/cyclic_value.h"

#include <stdio.h>

static lv_obj_t * cont;
static bool destroyed = false;

typedef struct {
    float step_scale = 0.0f;
    float current_step = 0.0f;
    uint16_t repeat_count = 0;
    lv_obj_t * lbl_desc = NULL;
    etl::string<10> buf = "";
} param_data_state_t;

typedef struct _param_data {
    void (* const val_redraw_fn)(const struct _param_data * data);
    void (* const val_update_fn)(const struct _param_data * data, lv_event_t e, int key_code);
    float * const val_ref;
    const float min_value = 0.0f;
    const float max_value = 0.0f;
    const float min_step = 0.0f;
    const float max_step = 0.0f;
    const uint8_t precision = 1;

    param_data_state_t * const s;
} param_data_t;

void base_update_value_fn(const param_data_t * data, lv_event_t e, int key_code)
{
    // lazy init for first run
    if (data->s->current_step < data->min_step) data->s->current_step = data->min_step;
    if (data->s->step_scale <= 0.0f) data->s->step_scale = 10.0f;

    if (e == LV_EVENT_RELEASED)
    {
        data->s->repeat_count = 0;
        data->s->current_step = data->min_step;
        return;
    }

    data->s->repeat_count++;

    if (data->s->repeat_count >= 10)
    {
        data->s->repeat_count = 0;
        data->s->current_step = fmin(data->s->current_step * data->s->step_scale, data->max_step);
    }

    float val = *data->val_ref;

    if (key_code == LV_KEY_RIGHT) val = fmin(val + data->s->current_step, data->max_value);
    else val = fmax(val - data->s->current_step, data->min_value);

    *data->val_ref = val;
    data->val_redraw_fn(data);
}

static void base_redraw_fn(const param_data_t * data)
{
    data->s->buf = "x";
    etl::to_string(*data->val_ref, data->s->buf, etl::format_spec().precision(data->precision), true);
    lv_label_set_text(data->s->lbl_desc, data->s->buf.c_str());
}

static param_data_state_t scale_state = {};

static const param_data_t scale = {
    .val_redraw_fn = &base_redraw_fn,
    .val_update_fn = &base_update_value_fn,
    .val_ref = &app_data.speed_scale,
    .min_value = 0.1f,
    .max_value = 9.9f,
    .min_step = 0.1f,
    .max_step = 1.0f,
    .precision = 1,
    .s = &scale_state
};


static void screen_flow_menu_item_cb(lv_obj_t * item, lv_event_t e)
{
    if (destroyed) return;

    // Next part is for keyboard only
    if ((e != LV_EVENT_KEY) && (e != LV_EVENT_LONG_PRESSED) &&
        (e != LV_EVENT_RELEASED) && (e != HACK_EVENT_RELEASED)) {
        return;
    }

    int key_code = lv_indev_get_key(app_data.kbd);

    switch (e)
    {
        // All keys except enter have `LV_EVENT_KEY` event for both
        // click & repeat. And do not provide release & long press events.
        case LV_EVENT_KEY:
            switch (key_code)
            {
                // Value change
                case LV_KEY_LEFT:
                case LV_KEY_RIGHT:
                    scale.val_update_fn(&scale, e, key_code);
                    app_update_settings();
                    return;
            }
            return;

        // for ENTER only
        case LV_EVENT_LONG_PRESSED:
            if (key_code == LV_KEY_ENTER) {
                app_data.skip_key_enter_release = true;
                app_screen_create(true);
            }
            return;

        // for ENTER only
        case LV_EVENT_RELEASED:
            if (key_code == LV_KEY_ENTER) {
                if (app_data.skip_key_enter_release)
                {
                    app_data.skip_key_enter_release = false;
                    return;
                }
                app_mode_switch();
            }
            return;

        // injected event from hacked driver
        case HACK_EVENT_RELEASED:
            switch (key_code)
            {
                case LV_KEY_LEFT:
                case LV_KEY_RIGHT:
                    scale.val_update_fn(&scale, LV_EVENT_RELEASED, key_code);
                    return;
            }
            return;
    }

    (void)item;
}


static void group_style_mod_cb(lv_group_t * group, lv_style_t * style)
{
    // Dummy handler, just to override lvgl's default preset
    (void)group; (void)style;
}


void screen_flow_create()
{
    lv_obj_t * screen = lv_scr_act();

    lv_group_set_style_mod_cb(app_data.group, group_style_mod_cb);

    lv_obj_t * mode_label = lv_label_create(screen, NULL);
    lv_label_set_style(mode_label, LV_LABEL_STYLE_MAIN, &app_data.styles.header_icon);
    lv_label_set_text(mode_label, U_ICON_FLOW);
    lv_obj_set_pos(mode_label, 4, 4);


    cont = lv_cont_create(screen, NULL);
    lv_cont_set_style(cont, LV_CONT_STYLE_MAIN, &app_data.styles.main);
    lv_obj_set_pos(cont, 0, 50);
    lv_obj_set_size(cont,
        lv_obj_get_width(screen),
        lv_obj_get_height(lv_scr_act()) - LIST_MARGIN_TOP
    );

    // Land events anywhere to process joystick
    lv_obj_set_event_cb(cont, screen_flow_menu_item_cb);
    lv_group_add_obj(app_data.group, cont);

    // Title
    lv_obj_t * lbl_title1 = lv_label_create(cont, NULL);
    lv_label_set_text(lbl_title1, "Speed");
    lv_obj_align(lbl_title1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

    lv_obj_t * lbl_title2 = lv_label_create(cont, NULL);
    lv_label_set_text(lbl_title2, "scale");
    lv_obj_align(lbl_title2, NULL, LV_ALIGN_IN_TOP_MID, 0, 18);

    // Container with value and arrows
    lv_obj_t * num_cont = lv_cont_create(cont, NULL);
    lv_cont_set_style(num_cont, LV_CONT_STYLE_MAIN, &app_data.styles.flow_num);
    lv_obj_set_size(num_cont, lv_obj_get_width(screen) - 8, 20);
    lv_cont_set_layout(num_cont, LV_LAYOUT_OFF);
    lv_obj_align(num_cont, NULL, LV_ALIGN_IN_TOP_MID, 0, 38);

    lv_obj_t * lbl_left = lv_label_create(num_cont, NULL);
    lv_label_set_text(lbl_left, U_ICON_LEFT);
    lv_obj_align(lbl_left, NULL, LV_ALIGN_IN_LEFT_MID, 0, -1);

    lv_obj_t * lbl_value = lv_label_create(num_cont, NULL);
    scale.s->lbl_desc = lbl_value;
    scale.val_redraw_fn(&scale);

    lv_obj_align(lbl_value, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * lbl_right = lv_label_create(num_cont, NULL);
    lv_label_set_text(lbl_right, U_ICON_RIGHT);
    lv_obj_align(lbl_right, NULL, LV_ALIGN_IN_RIGHT_MID, 0, -1);

    //
    // Sync data
    //

    destroyed = false;
}


void screen_flow_destroy()
{
    if (destroyed) return;
    destroyed = true;
    lv_group_set_focus_cb(app_data.group, NULL);
    lv_group_set_style_mod_cb(app_data.group, NULL);
    lv_group_remove_all_objs(app_data.group);
    lv_obj_clean(lv_scr_act());
}
