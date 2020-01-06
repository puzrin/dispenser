#include "app.h"
#include "screen_settings.h"
#include "etl/to_string.h"
#include "etl/cyclic_value.h"


enum setting_type {
    TYPE_NEEDLE_DIA = 0,
    TYPE_SYRINGE_DIA = 1,
    TYPE_VISCOSITY = 2,
    TYPE_FLUX_PERCENT = 3,
    TYPE_MOVE_PUSHER = 4
};

typedef struct {
    float step_scale = 0.0f;
    float current_step = 0.0f;
    uint16_t repeat_count = 0;
} setting_data_state_t;

typedef struct _setting_data {
    enum setting_type const type;
    const char * const title;
    const char * (* const val_get_text_fn)(const struct _setting_data * data);
    void (* const val_update_fn)(const struct _setting_data * data, lv_event_t e, int const key_code);
    float * const val_ref;
    const float min_value = 0.0f;
    const float max_value = 0.0f;
    const float min_step = 0.0f;
    const float max_step = 0.0f;
    const uint8_t precision = 1;
    const char * const suffix = "";
    setting_data_state_t * const s;
} setting_data_t;

void base_update_value_fn(const setting_data_t * data, lv_event_t e, int key_code)
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
}

static const char * base_get_text_fn(const setting_data_t * data)
{
    static etl::string<10> buf;
    etl::to_string(*data->val_ref, buf, etl::format_spec().precision(data->precision));
    buf += data->suffix;
    return buf.c_str();
}


/*static setting_data_state_t s_data_needle_state = {};

static setting_data_t s_data_needle = {
    .type = TYPE_NEEDLE_DIA,
    .title = "Needle dia",
    .val_redraw_fn = &base_redraw_fn,
    .val_update_fn = &base_update_value_fn,
    .val_ref = &app_data.needle_dia,
    .min_value = 0.3f,
    .max_value = 2.0f,
    .min_step = 0.1f,
    .max_step = 0.1f,
    .precision = 1,
    .suffix = " mm",
    .s = &s_data_needle_state
};*/


static setting_data_state_t s_data_syringe_state = {};

static const setting_data_t s_data_syringe = {
    .type = TYPE_SYRINGE_DIA,
    .title = "Syringe dia",
    .val_get_text_fn = &base_get_text_fn,
    .val_update_fn = &base_update_value_fn,
    .val_ref = &app_data.syringe_dia,
    .min_value = 4.0f,
    .max_value = 30.0f,
    .min_step = 0.1f,
    .max_step = 1.0f,
    .precision = 1,
    .suffix = " mm",
    .s = &s_data_syringe_state
};


static setting_data_state_t s_data_viscosity_state = {};

static const setting_data_t s_data_viscosity = {
    .type = TYPE_VISCOSITY,
    .title = "Viscosity",
    .val_get_text_fn = &base_get_text_fn,
    .val_update_fn = &base_update_value_fn,
    .val_ref = &app_data.viscosity,
    .min_value = 1.0f,
    .max_value = 1000.0f,
    .min_step = 1.0f,
    .max_step = 100.0f,
    .precision = 1,
    .suffix = " P",
    .s = &s_data_viscosity_state
};


static setting_data_state_t s_data_flux_percent_state = {};

static const setting_data_t s_data_flux_percent = {
    .type = TYPE_FLUX_PERCENT,
    .title = "Flux part",
    .val_get_text_fn = &base_get_text_fn,
    .val_update_fn = &base_update_value_fn,
    .val_ref = &app_data.flux_percent,
    .min_value = 1.0f,
    .max_value = 50.0f,
    .min_step = 1.0f,
    .max_step = 3.0f,
    .precision = 0,
    .suffix = "%",
    .s = &s_data_flux_percent_state
};

static const char * move_pusher_get_text_fn(const setting_data_t * data)
{
    return U_ICON_ARROWS;
}

static void pusher_update_value_fn(const setting_data_t * data, lv_event_t e, int key_code)
{
    // TODO run motor in specified direction
    (void)data; (void)e; (void) key_code;
}


static setting_data_state_t s_data_move_pusher_state = {};

static const setting_data_t s_data_move_pusher = {
    .type = TYPE_MOVE_PUSHER,
    .title = "Fast move",
    .val_get_text_fn = &move_pusher_get_text_fn,
    .val_update_fn = &pusher_update_value_fn,
    .val_ref = NULL,
    .min_value = 0.0f,
    .max_value = 0.0f,
    .min_step = 0.0f,
    .max_step = 0.0f,
    .precision = 0,
    .suffix = "",
    .s = &s_data_move_pusher_state
};


static lv_obj_t * page;
static bool destroyed = false;

static lv_design_cb_t orig_design_cb;

// Custom drawer to avoid labels create & save RAM.
static bool list_item_design_cb(lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
    auto * s_data = reinterpret_cast<const setting_data_t *>(lv_obj_get_user_data(obj));

    if(mode == LV_DESIGN_DRAW_MAIN)
    {
        orig_design_cb(obj, mask_p, mode);

        lv_point_t title_pos = { .x = 4, .y = 4 };
        lv_draw_label(
            &obj->coords,
            mask_p,
            &app_data.styles.list_title,
            lv_obj_get_opa_scale(obj),
            s_data->title,
            LV_TXT_FLAG_NONE,
            &title_pos,
            NULL,
            NULL,
            lv_obj_get_base_dir(obj)
        );

        lv_point_t desc_pos = { .x = 4, .y = 19 };
        lv_draw_label(
            &obj->coords,
            mask_p,
            &app_data.styles.list_desc,
            lv_obj_get_opa_scale(obj),
            s_data->val_get_text_fn(s_data),
            LV_TXT_FLAG_NONE,
            &desc_pos,
            NULL,
            NULL,
            lv_obj_get_base_dir(obj)
        );

        return true;
    }

    return orig_design_cb(obj, mask_p, mode);
}


static void group_style_mod_cb(lv_group_t * group, lv_style_t * style)
{
    if (destroyed) return;
    style->body.main_color = COLOR_BG_HIGHLIGHT;
    style->body.grad_color = COLOR_BG_HIGHLIGHT;

    (void)group;
}


static void screen_settings_menu_item_cb(lv_obj_t * item, lv_event_t e)
{
    if (destroyed) return;

    // Next part is for keyboard only
    if ((e != LV_EVENT_KEY) && (e != LV_EVENT_LONG_PRESSED) &&
        (e != LV_EVENT_RELEASED) && (e != HACK_EVENT_RELEASED)) {
        return;
    }

    int key_code = lv_indev_get_key(app_data.kbd);
    static etl::cyclic_value<uint8_t, 0, 1> skip_cnt;
    auto * s_data = reinterpret_cast<const setting_data_t *>(lv_obj_get_user_data(item));

    switch (e)
    {
        // All keys except enter have `LV_EVENT_KEY` event for both
        // click & repeat. And do not provide release & long press events.
        case LV_EVENT_KEY:
            switch (key_code)
            {
                // List navigation
                case LV_KEY_UP:
                    skip_cnt++;
                    if (skip_cnt != 1) return;

                    lv_group_focus_prev(app_data.group);
                    // Scroll focused to visible area
                    lv_page_focus(page, lv_group_get_focused(app_data.group), true);
                    return;

                case LV_KEY_DOWN:
                    skip_cnt++;
                    if (skip_cnt != 1) return;

                    lv_group_focus_next(app_data.group);
                    // Scroll focused to visible area
                    lv_page_focus(page, lv_group_get_focused(app_data.group), true);
                    return;

                // Value change
                case LV_KEY_LEFT:
                case LV_KEY_RIGHT:
                    s_data->val_update_fn(s_data, e, key_code);
                    lv_obj_invalidate(item);
                    app_update_settings();
                    return;
            }
            return;

        // for ENTER only
        case LV_EVENT_LONG_PRESSED:
            if (key_code == LV_KEY_ENTER)
            {
                app_data.skip_key_enter_release = true;
                app_screen_create(false);
            }
            return;

        // for ENTER only
        case LV_EVENT_RELEASED:
            if (key_code == LV_KEY_ENTER) {
                    if (app_data.skip_key_enter_release) {
                        app_data.skip_key_enter_release = false;
                        return;
                    }
                    app_screen_create(false);
                    return;
            }
            return;

        // injected event from hacked driver
        case HACK_EVENT_RELEASED:
            skip_cnt = 0;

            switch (key_code)
            {
                case LV_KEY_LEFT:
                case LV_KEY_RIGHT:
                    s_data->val_update_fn(s_data, LV_EVENT_RELEASED, key_code);
                    return;
            }
            return;
    }
}


void screen_settings_create()
{
    lv_obj_t * screen = lv_scr_act();

    lv_group_set_style_mod_cb(app_data.group, group_style_mod_cb);

    lv_obj_t * mode_label = lv_label_create(screen, NULL);
    lv_label_set_style(mode_label, LV_LABEL_STYLE_MAIN, &app_data.styles.header_icon);
    lv_label_set_text(mode_label, U_ICON_SETTINGS);
    lv_obj_set_pos(mode_label, 4, 4);

    //
    // Create list
    //

    page = lv_page_create(screen, NULL);
    lv_page_set_anim_time(page, 150);
    lv_page_set_style(page, LV_PAGE_STYLE_BG, &app_data.styles.list);
    lv_page_set_style(page, LV_PAGE_STYLE_SCRL, &app_data.styles.list);
    lv_page_set_sb_mode(page, LV_SB_MODE_OFF);
    lv_page_set_scrl_layout(page, LV_LAYOUT_COL_L);
    lv_obj_set_size(
        page,
        lv_obj_get_width(screen),
        lv_obj_get_height(lv_scr_act()) - LIST_MARGIN_TOP
    );
    lv_obj_set_pos(page, 0, LIST_MARGIN_TOP);

    lv_obj_t * selected_item = NULL;
    uint8_t selected_id = (uint8_t)app_data.screen_settings_selected_id;

    const setting_data_t * s_data_list[] = {
        &s_data_syringe,
        //&s_data_needle,
        &s_data_viscosity,
        &s_data_flux_percent,
        &s_data_move_pusher,
        NULL
    };

    for (uint8_t i = 0; s_data_list[i] != NULL; i++)
    {
        //
        // Create list item & attach user data
        //

        auto * data = s_data_list[i];

        lv_obj_t * item = lv_obj_create(page, NULL);
        lv_obj_set_user_data(item, const_cast<setting_data_t *>(data));
        lv_obj_set_style(item, &app_data.styles.main);
        lv_obj_set_size(item, lv_obj_get_width(page), 37);
        lv_obj_set_event_cb(item, screen_settings_menu_item_cb);
        lv_group_add_obj(app_data.group, item);

        // We use custom drawer to avoid labels use and reduce RAM comsumption
        // significantly. Now it's ~ 170 bytes per entry.
        // Since all callbacks are equal - use the same var to store old ones.
        orig_design_cb = lv_obj_get_design_cb(item);
        lv_obj_set_design_cb(item, list_item_design_cb);

        if (data->type == selected_id) selected_item = item;
    }

    //
    // Restore previous state
    //

    if (selected_item)
    {
        lv_group_focus_obj(selected_item);

        if (app_data.screen_settings_scroll_pos != INT16_MAX)
        {
            lv_obj_set_y(lv_page_get_scrl(page), app_data.screen_settings_scroll_pos);
        }
        else lv_page_focus(page, lv_group_get_focused(app_data.group), LV_ANIM_OFF);
    }

    destroyed = false;
}

void screen_settings_destroy()
{
    if (destroyed) return;
    destroyed = true;

    app_data.screen_settings_scroll_pos = lv_obj_get_y(lv_page_get_scrl(page));
    auto * s_data = reinterpret_cast<const setting_data_t *>(lv_obj_get_user_data(
        lv_group_get_focused(app_data.group)
    ));
    app_data.screen_settings_selected_id = s_data->type;

    lv_group_set_focus_cb(app_data.group, NULL);
    lv_group_remove_all_objs(app_data.group);
    lv_obj_clean(lv_scr_act());
}
