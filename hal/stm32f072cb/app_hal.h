#ifndef __APP_HAL__
#define __APP_HAL__

#include <stdint.h>

namespace hal {

void setup();
void loop();
void set_hires_timer_cb(void (*handler)(void));
bool key_start_on();
void backlight(bool on);

class StepperIO {
public:
    static void to(uint16_t phase);
    static void off();
};

} // namespace

#endif