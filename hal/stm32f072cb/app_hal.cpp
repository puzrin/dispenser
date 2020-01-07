#include "app.h"
#include "app_hal.h"
#include "etl/debounce.h"

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "dma.h"

#include "lvgl.h"
#include "st7735.h"
#include "stdio_retarget.h"

extern "C" void SystemClock_Config(void);

#if MEM_USE_LOG != 0
#include "stdio.h"
static void sysmon_task(lv_task_t * param)
{
    (void) param;

    uint8_t mem_used_pct = 0;
    lv_mem_monitor_t mem_mon;
    lv_mem_monitor(&mem_mon);
    mem_used_pct = mem_mon.used_pct;

    printf(
        "[Memory] total: %d bytes, free: %d bytes, use: %d%% (%d)\r\n",
        (int)mem_mon.total_size,
        (int)mem_mon.free_size,
        (int)mem_used_pct,
        (int)(mem_mon.total_size - mem_mon.free_size)
    );
}
#endif

namespace hal {

// Key debouncers, protect from jitter
static etl::debounce<3> key_up, key_down, key_left, key_right, key_enter, key_start;

static uint32_t kbd_last_key = 0;
static lv_indev_state_t kbd_last_state;

static lv_disp_drv_t disp_drv;

// Scan keys every 10ms.
static void key_scan_task(lv_task_t * task)
{
        // Forwars button pins to debouncers
        key_down.add(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13));
        key_up.add(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15));
        key_right.add(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14));
        key_left.add(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_0));
        key_enter.add(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_1));
        key_start.add(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8));

        // Record data for indev reader the same way as in emulator.
        // In theory, events should be queued, but currently LVGL allows only.
        // single press/release event at each moment.
        if (key_up.has_changed())
        {
            if (key_up.is_set()) { kbd_last_key = LV_KEY_UP; kbd_last_state = LV_INDEV_STATE_PR; }
            else kbd_last_state = LV_INDEV_STATE_REL;
        }
        if (key_left.has_changed())
        {
            if (key_left.is_set()) { kbd_last_key = LV_KEY_LEFT; kbd_last_state = LV_INDEV_STATE_PR; }
            else kbd_last_state = LV_INDEV_STATE_REL;
        }
        if (key_down.has_changed())
        {
            if (key_down.is_set()) { kbd_last_key = LV_KEY_DOWN; kbd_last_state = LV_INDEV_STATE_PR; }
            else kbd_last_state = LV_INDEV_STATE_REL;
        }
        if (key_right.has_changed())
        {
            if (key_right.is_set()) { kbd_last_key = LV_KEY_RIGHT; kbd_last_state = LV_INDEV_STATE_PR; }
            else kbd_last_state = LV_INDEV_STATE_REL;
        }
        if (key_enter.has_changed())
        {
            if (key_enter.is_set()) { kbd_last_key = LV_KEY_ENTER; kbd_last_state = LV_INDEV_STATE_PR; }
            else kbd_last_state = LV_INDEV_STATE_REL;
        }
}

static bool my_keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static int prev_state;
    // Load prepared data
    data->state = kbd_last_state;
    data->key = kbd_last_key;

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
    return key_start.is_set();
}


//
// HiRes timer to create software PWM-s.
//

static void (*hires_timer_cb)(void) = NULL;

void set_hires_timer_cb(void (*handler)(void))
{
    hires_timer_cb = handler;
}


void setup(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();

    // Attach display buffer and display driver
    static lv_disp_buf_t disp_buf;
    static lv_color_t buf[LV_HOR_RES_MAX * 20];
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 20);

    lv_disp_drv_init(&disp_drv);

    disp_drv.flush_cb = ST7735_flush_cb;
    disp_drv.buffer = &disp_buf;

    ST7735_Init(&hspi1, &disp_drv);

    lv_disp_drv_register(&disp_drv);

    //
    // Init keyboard
    //

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = my_keyboard_read;

    app_data.kbd = lv_indev_drv_register(&indev_drv);
    lv_task_create(key_scan_task, 10, LV_TASK_PRIO_HIGHEST, NULL);

    MX_ADC_Init();
    MX_TIM7_Init();
    MX_TIM6_Init();

    stdio_retarget_init();

    #if MEM_USE_LOG != 0
        lv_task_create(sysmon_task, 500, LV_TASK_PRIO_LOW, NULL);
    #endif

    HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);
}

//
// Main loop
//

void loop(void)
{
    uint32_t tick_start = HAL_GetTick();

    while(1) {
        uint32_t tick_current = HAL_GetTick();

        if (tick_current - tick_start >= 5)
        {
            // Call ~ every 5ms
            tick_start = tick_current;
        	lv_task_handler();
        }
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
    off(); // clrear previous

    switch (phase & 0x3)
    {
        case 0:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
            break;
        case 1:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            break;
        case 2:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
            break;
        case 3:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
            break;
    }
}

void StepperIO::off()
{
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
        GPIO_PIN_RESET
    );
}


// Enable/Disable LCD backlight
void backlight(bool on)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


} // namespace

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7)
    {
        // HiRes (100 uS) timer
        if (hal::hires_timer_cb) hal::hires_timer_cb();
    }
    else if (htim->Instance == TIM6)
    {
        // 1 mS timer
        lv_tick_inc(1);
    }
}
