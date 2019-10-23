#include "app.h"
#include "screen_flow.h"
#include "etl/cyclic_value.h"

typedef struct {
    float speed_scale;
    const char * title;
} flow_scale_t;

flow_scale_t flow_scales[] = {
    { .speed_scale = 0.25f, .title = "¼" },
    { .speed_scale = 0.5f,  .title = "½" },
    { .speed_scale = 1.0f,  .title = "Normal" },
    { .speed_scale = 2.0f,  .title = "x2" },
    { .speed_scale = 4.0f,  .title = "x4" },
    { .speed_scale = 0.0f,  .title = "" }
};


static lv_obj_t * page;
static bool destroyed = false;

typedef struct {
    lv_cont_ext_t cont;
    float speed_scale;
} menu_flow_item_ext_t;


static void group_style_mod_cb(lv_group_t * group, lv_style_t * style)
{
    if (destroyed) return;
    style->body.main_color = lv_color_mix(style->body.main_color, LV_COLOR_ORANGE, LV_OPA_40);
    style->body.grad_color = lv_color_mix(style->body.grad_color, LV_COLOR_ORANGE, LV_OPA_40);

    (void)group;
}


static void screen_flow_menu_item_cb(lv_obj_t * item, lv_event_t e)
{
    if (destroyed) return;

    // Next part is for keyboard only
    if ((e != LV_EVENT_KEY) && (e != LV_EVENT_LONG_PRESSED) &&
        (e != LV_EVENT_RELEASED) && (e != HACK_EVENT_RELEASED)) {
        return;
    }

    int key_code = lv_indev_get_key(app_data.kbd);
    static etl::cyclic_value<uint8_t, 0, 1> skip_cnt;

    switch (e)
    {
        // All keys except enter have `LV_EVENT_KEY` event for both
        // click & repeat. And do not provide release & long press events.
        case LV_EVENT_KEY:
            switch (key_code)
            {
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
            skip_cnt = 0;
            return;
    }

    (void)item;
}


static void focus_cb(lv_group_t * group)
{
    if (destroyed) return;

    menu_flow_item_ext_t * ext;
    ext = (menu_flow_item_ext_t *)lv_obj_get_ext_attr(lv_group_get_focused(group));

    if (app_data.speed_scale != ext->speed_scale)
    {
        app_data.speed_scale = ext->speed_scale;
        app_update_settings();
    }
}


void screen_flow_create()
{
    lv_obj_t * screen = lv_scr_act();

    lv_group_set_style_mod_cb(app_data.group, group_style_mod_cb);

    lv_obj_t * mode_label = lv_label_create(screen, NULL);
    lv_label_set_text(mode_label, "mode: flow");
    lv_obj_set_pos(mode_label, 1, 5);

    //
    // Create list
    //

    page = lv_page_create(screen, NULL);
    lv_page_set_anim_time(page, 150);
    lv_page_set_style(page, LV_PAGE_STYLE_BG, &app_data.styles.main);
    lv_page_set_style(page, LV_PAGE_STYLE_SCRL, &app_data.styles.main);
    lv_page_set_sb_mode(page, LV_SB_MODE_OFF);
    lv_page_set_scrl_layout(page, LV_LAYOUT_COL_L);
    lv_obj_set_size(
        page,
        lv_obj_get_width(screen),
        lv_obj_get_height(lv_scr_act()) - LIST_MARGIN_TOP
    );
    lv_obj_set_pos(page, 0, LIST_MARGIN_TOP);

    lv_obj_t * selected_item = NULL;

    for (int i=0;; i++)
    {
        flow_scale_t flow_scale = flow_scales[i];

        if (flow_scale.speed_scale == 0) break;

        //
        // Add container to accept navigation clicks
        //

        lv_obj_t * item = lv_cont_create(page, NULL);
        lv_cont_set_style(item, LV_CONT_STYLE_MAIN, &app_data.styles.main);
        lv_cont_set_layout(item, LV_LAYOUT_OFF);
        lv_cont_set_fit2(item, LV_FIT_NONE, LV_FIT_NONE);
        lv_obj_set_size(item, lv_obj_get_width(page), 22);
        lv_obj_set_event_cb(item, screen_flow_menu_item_cb);
        lv_group_add_obj(app_data.group, item);

        //
        // Add text
        //

        lv_obj_t * lbl_title = lv_label_create(item, NULL);
        lv_label_set_text(lbl_title, flow_scale.title);
        lv_label_set_style(lbl_title, LV_LABEL_STYLE_MAIN, &app_data.styles.list_title);
        lv_obj_set_pos(lbl_title, 1, 2);
        lv_obj_align(lbl_title, NULL, LV_ALIGN_CENTER, 0, 0);

        //
        // Populate with extended data to simplify navigation
        //
        menu_flow_item_ext_t * ext = (menu_flow_item_ext_t *)lv_obj_allocate_ext_attr(
            item,
            sizeof(menu_flow_item_ext_t)
        );
        ext->speed_scale = flow_scale.speed_scale;

        if (app_data.speed_scale == flow_scale.speed_scale) selected_item = item;
    }

    //
    // Sync data
    //

    if (selected_item) {
        lv_group_focus_obj(selected_item);

        if (app_data.screen_flow_scroll_pos != INT16_MAX)
        {
            lv_obj_set_y(lv_page_get_scrl(page), app_data.screen_flow_scroll_pos);
        }
        else lv_page_focus(page, selected_item, LV_ANIM_OFF);
    }
    else
    {
        app_data.speed_scale = flow_scales[0].speed_scale;
        app_update_settings();
    }


    lv_group_set_focus_cb(app_data.group, focus_cb);
    destroyed = false;
}


void screen_flow_destroy()
{
    if (destroyed) return;
    destroyed = true;
    app_data.screen_flow_scroll_pos = lv_obj_get_y(lv_page_get_scrl(page));
    lv_group_set_focus_cb(app_data.group, NULL);
    lv_group_remove_all_objs(app_data.group);
    lv_obj_clean(lv_scr_act());
}
