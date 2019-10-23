#define SDL_MAIN_HANDLED        /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#include "display/monitor.h"
#include "indev/keyboard.h"

#include "app_hal.h"
#include "app.h"

namespace hal {

// Debug log writer
#if LV_USE_LOG
void log(lv_log_level_t level, const char * file, uint32_t line, const char * dsc) {
    static const char * lvl_prefix[] = {"Trace", "Info", "Warn", "Error"};
    printf("%s: %s \t(%s #%d)\n", lvl_prefix[level], dsc, file, line);
}
#endif

static bool key_space_pressed = false;

static bool my_keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static int prev_state;

    // Call original function
    keyboard_read(indev_drv, data);

    // Translate space key state (replaces "dispence" button)
    if (data->key == 0x20)
    {
        if (data->state == LV_INDEV_STATE_PR) key_space_pressed = true;
        else key_space_pressed = false;
    }

    //---------------------------------------------------------
    // HACK! Send release codes to group, to compensate lack of
    // existing ones
    //---------------------------------------------------------
    if (app_data.group)
    {
        // Spy data & inject events before continue
        if (data->state == LV_INDEV_STATE_REL && prev_state == LV_INDEV_STATE_PR)
        {
            lv_obj_t * selected = lv_group_get_focused(app_data.group);
            if (selected)
            {
                lv_event_send(selected, HACK_EVENT_RELEASED, &data->key);
            }
        }
    }

    prev_state = data->state;

    return false;
}

// State of "dispense" key.
bool key_start_on()
{
    return key_space_pressed;
}

#if MEM_USE_LOG != 0
static void sysmon_task(lv_task_t * param)
{
    (void) param;    /*Unused*/

    uint8_t mem_used_pct = 0;
    lv_mem_monitor_t mem_mon;
    lv_mem_monitor(&mem_mon);
    mem_used_pct = mem_mon.used_pct;

    printf(
        "[Memory] total: %d bytes, free: %d bytes, use: %d%%\n",
        (int)mem_mon.total_size,
        (int)mem_mon.free_size,
        (int)mem_used_pct
    );
}
#endif


static int tick_thread(void * data)
{
    (void)data;

    while(1) {
        SDL_Delay(5);
        lv_tick_inc(5);
    }

    return 0;
}

//
// HiRes timer to create software PWM-s.
//

static void (*hires_timer_cb)(void) = NULL;

void set_hires_timer_cb(void (*handler)(void))
{
    hires_timer_cb = handler;
}

static SDL_mutex * mutex;

static Uint32 hires_timer_executor(Uint32 interval, void *param)
{
    //if (SDL_TryLockMutex(mutex) != 0) return interval;
    if (hires_timer_cb != NULL) hires_timer_cb();

    (void)param;
    return interval;
}

//
// Hardware setup
//

void setup(void)
{
    // Register debug log fn, write all to console
#if LV_USE_LOG
    lv_log_register_print_cb(log);
#endif

    //
    // Init display
    //

    monitor_init();

    static lv_disp_buf_t disp_buf;
    static lv_color_t buf[LV_HOR_RES_MAX * 10];
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.flush_cb = monitor_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    //
    // Init keyboard
    //

    keyboard_init();
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = my_keyboard_read;

    app_data.kbd = lv_indev_drv_register(&indev_drv);

    //
    // Emulate HiRes timer. Here we use 1000 Hz instead of 10000 Hz on bare metal.
    //
    mutex = SDL_CreateMutex();
    SDL_AddTimer(1, hires_timer_executor, NULL);

#if MEM_USE_LOG != 0
    lv_task_create(sysmon_task, 500, LV_TASK_PRIO_LOW, NULL);
#endif

    SDL_CreateThread(tick_thread, "tick", NULL);
}

//
// Main loop
//

void loop(void)
{
    // Loop
    while(1) {
        SDL_Delay(5);
        lv_task_handler();
    }
}


//
// Set motor pins according to desired position. Everithing else
// done in software
//
// 0, 1, 2, 3 - respond to desired rotor position in full step wave mode
//

void StepperIO::to(uint16_t phase)
{
    printf("motor: phase %d", phase);
}

void StepperIO::off()
{
    printf("motor: off");
}


// Enable/Disable LCD backlight. Do nothing in emulator
void backlight(bool on)
{
    (void)on;
}


} // namespace
