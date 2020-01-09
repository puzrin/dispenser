#include "app.h"
#include "app_hal.h"
#include "screen_dose.h"
#include "screen_flow.h"
#include "screen_settings.h"
#include"doses.h"
#include "lvgl.h"
#include "etl/cyclic_value.h"

#include "eeprom_emu.h"
#include "eeprom_flash_driver.h"
#include "stepper_control.h"

//#include <stdio.h>
#include "fonts_custom.h"

EepromEmu<EepromFlashDriver> eeprom;
app_data_t app_data;

StepperPwmParams pwm_params;
StepperControl<hal::StepperIO> stepper_control(&pwm_params);

// eeprom map
enum {
    ADDR_NEEDLE_DIA = 0,
    ADDR_SYRINGE_DIA = 1,
    ADDR_VISCOSITY = 2,
    ADDR_FLUX_PERCENT = 3,
    ADDR_DOSE_VOLUME = 4,
    ADDR_SPEED_SCALE = 5,
    ADDR_FLOW_MODE = 6,
    ADDR_LCD_BRIGHTNESS = 7
};


static void load_settings()
{
    app_data.needle_dia     = eeprom.read_float(ADDR_NEEDLE_DIA, 0.5f);
    app_data.syringe_dia    = eeprom.read_float(ADDR_SYRINGE_DIA, 15.0f);
    app_data.viscosity      = eeprom.read_float(ADDR_VISCOSITY, 1.0);
    app_data.flux_percent   = eeprom.read_float(ADDR_FLUX_PERCENT, 10.0f);
    app_data.dose_volume    = eeprom.read_float(ADDR_DOSE_VOLUME, doses[0].volume);
    app_data.speed_scale    = eeprom.read_float(ADDR_SPEED_SCALE, 1.0f);
    app_data.flow_mode      = (bool)eeprom.read_u32(ADDR_FLOW_MODE, 0);
    app_data.lcd_brightness = (uint8_t)eeprom.read_u32(ADDR_LCD_BRIGHTNESS, 100);
}

static void save_settings()
{
    eeprom.write_float(ADDR_NEEDLE_DIA, app_data.needle_dia);
    eeprom.write_float(ADDR_SYRINGE_DIA, app_data.syringe_dia);
    eeprom.write_float(ADDR_VISCOSITY,app_data.viscosity);
    eeprom.write_float(ADDR_FLUX_PERCENT, app_data.flux_percent);
    eeprom.write_float(ADDR_DOSE_VOLUME, app_data.dose_volume);
    eeprom.write_float(ADDR_SPEED_SCALE, app_data.speed_scale);
    eeprom.write_u32(ADDR_FLOW_MODE, (uint32_t)app_data.flow_mode);
    eeprom.write_u32(ADDR_LCD_BRIGHTNESS, app_data.lcd_brightness);
}


static bool saver_has_data = false;

static void saver_task(lv_task_t * task)
{
    if (!saver_has_data) return;
    saver_has_data = false;
    save_settings();

    (void)task;
}

// Save config to eeprom, but wait 1s of inactivity first
static void save_settings_debounced()
{
    static lv_task_t * task = lv_task_create(saver_task, 1000, LV_TASK_PRIO_LOWEST, NULL);

    lv_task_reset(task); // Start counting timeout from zero
    saver_has_data = true;
}

// Called after any settings update. Recalculate motor params and store
// state in "eeprom"
void app_update_settings()
{
    save_settings_debounced();
}


static void (*prev_screen_destroy)() = NULL;

void app_screen_create(bool to_settings)
{
    if (prev_screen_destroy) (*prev_screen_destroy)();

    if (to_settings)
    {
        prev_screen_destroy = &screen_settings_destroy;
        screen_settings_create();
        return;
    }

    if (app_data.flow_mode)
    {
        prev_screen_destroy = &screen_flow_destroy;
        screen_flow_create();
    }
    else {
        prev_screen_destroy = &screen_dose_destroy;
        screen_dose_create();
    }
}

void app_mode_switch()
{
    app_data.flow_mode = !app_data.flow_mode;
    app_update_settings();
    app_screen_create(false);
}


static void create_styles()
{
    lv_style_t * s;

    s = &app_data.styles.main;
    lv_style_copy(s, &lv_style_pretty);
    s->body.main_color = LV_COLOR_BLACK;
    s->body.grad_color = LV_COLOR_BLACK;
    s->body.radius = 0;
    s->body.border.width = 0;
    s->body.padding.bottom = 0;
    s->body.padding.left = 0;
    s->body.padding.right = 0;
    s->body.padding.top = 0;
    s->body.padding.inner = 0;
    s->text.color = LV_COLOR_WHITE;
    s->text.font = &my_font_roboto_14;

    s = &app_data.styles.list;
    lv_style_copy(s, &app_data.styles.main);
    s->body.padding.inner = 1;

    s = &app_data.styles.list_item;
    lv_style_copy(s, &app_data.styles.main);
    //s->body.padding.bottom = 10;

    s = &app_data.styles.list_title;
    lv_style_copy(s, &app_data.styles.main);
    s->text.font = &my_font_roboto_14;

    s = &app_data.styles.list_desc;
    lv_style_copy(s, &app_data.styles.main);
    s->text.font = &my_font_roboto_12;
    s->text.color = COLOR_SECONDARY;

    s = &app_data.styles.header_icon;
    lv_style_copy(s, &app_data.styles.main);
    s->text.font = &my_font_icons_18;
    s->text.color = COLOR_SECONDARY;

    s = &app_data.styles.flow_num;
    lv_style_copy(s, &app_data.styles.main);
    s->text.font = &my_font_roboto_num_18;
    s->text.color = COLOR_SECONDARY;
}


static void dispence_btn_scan_task(lv_task_t * task)
{
    static bool prev_pressed = false;
    static bool prev_press_mode_flow = false;

    if (!prev_pressed)
    {
        if (hal::key_start_on())
        {
            prev_pressed = true;
            prev_press_mode_flow = app_data.flow_mode;

            if (app_data.flow_mode) stepper_control.flow();
            else stepper_control.dose();
        }
    }
    else
    {
        if (!hal::key_start_on())
        {
            prev_pressed = false;
            if (prev_press_mode_flow) stepper_control.stop();
        }
    }

    (void)task;
}


static void hires_tick_handler()
{
    stepper_control.tick();

    static etl::cyclic_value<uint8_t, 0, 99> brightness_count;

    if (++brightness_count < app_data.lcd_brightness) hal::backlight(true);
    else hal::backlight(false);
}


int main()
{
    lv_init();
    hal::setup();
    hal::backlight(true);
    load_settings();

    create_styles();
    lv_obj_set_style(lv_scr_act(), &app_data.styles.main);

    app_data.group = lv_group_create();
    lv_indev_set_group(app_data.kbd, app_data.group);
    lv_group_set_wrap(app_data.group, false);

    app_data.screen_dose_scroll_pos = INT16_MAX;
    app_data.screen_flow_scroll_pos = INT16_MAX;
    app_data.screen_settings_scroll_pos = INT16_MAX;
    app_data.screen_settings_selected_id = -1;
    app_data.skip_key_enter_release = false;

    app_screen_create(false);

    hal::set_hires_timer_cb(hires_tick_handler);
    lv_task_create(dispence_btn_scan_task, 30, LV_TASK_PRIO_HIGH, NULL);

    hal::loop();
}
