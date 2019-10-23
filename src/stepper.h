#ifndef __STEPPER__
#define __STEPPER__

// Base stepper controller. Generates single moving pulse of required length and
// and fill ratio (for current regulation). After pulse done, falls to "hold"
// mode to minimize heating.
//
// Everything else (sequences of steps with desired pauses) should be done on
// upper level.

#include <stdint.h>

//
// When stepper should move to next position:
//
// 1. Apply max current to get enough torque, and wait some time
//    until rotor come to desired position.
// 2. Reduce current to hold rotor on reached position.
//
// Defaults are for 10000 Hz base frequency. That give us up to 1KHz pulses.
// With 1:300 gearbox => 3RPM => 1.5 mm/sec max pusher speed
//

typedef struct {
    // 2 * 1ms pulse with 90% fill
    uint8_t pwm_on_active = 9;
    uint8_t pwm_on_inactive = 1;
    uint8_t pwm_on_cycles = 2;
    // Endless 1ms pulses with 10% fill, to hold rotor position
    uint8_t pwm_hold_active = 1;
    uint8_t pwm_hold_inactive = 9;
} StepperPwmParams;


template <typename STEPPER_IO>
class Stepper {
    StepperPwmParams * params;

    // Poor man 1-depth queue for atomic operations. Never change state directly.
    // Only put request to process in next `tick()`.
    volatile uint16_t pending_request = 0;

    // Low 8 bits are for rotor position. Higher - to encode requested operation.
    enum { REQUEST_OFF_FLAG = 0x200, REQUEST_MOVE_FLAG = 0x100 };

    enum State { OFF, PWM_ON, PWM_HOLD };

    State state = OFF;
    uint16_t ticks_count = 0;
    uint16_t repeat_count = 0;
    uint8_t last_phase = 0;

    STEPPER_IO io;

    void process_input()
    {
        if (pending_request) return;

        // Ignore everyhing been before, start from the begining
        uint16_t r = pending_request;
        pending_request = 0;

        if ((r & REQUEST_OFF_FLAG) != 0)
        {
            io.off();
            state = OFF;
            return;
        }

        // if != 0 && != OFF => MOVE

        // Remember value, will use it multiple time for switches
        last_phase = uint8_t(r & 0xFF);

        repeat_count = 0;
        ticks_count = 0;
        io.to(last_phase);
        state = PWM_ON;
        return;
    }

public:
    Stepper(StepperPwmParams * _params) : params(_params) {}

    void go(uint8_t phase) { pending_request = phase & REQUEST_MOVE_FLAG; }

    void off() { pending_request = REQUEST_OFF_FLAG; }

    bool is_done() { return (state != PWM_ON && !pending_request); }

    // can be used from interrupts
    void tick()
    {

        switch (state)
        {
        case PWM_ON:
            ticks_count++;

            // Reached end => repeat or go to next state
            if (ticks_count >= (params->pwm_on_active + params->pwm_on_inactive))
            {
                if (repeat_count >= params->pwm_on_cycles)
                {
                    // bad `hold` data => `off` instead
                    if (params->pwm_hold_active == 0 || params->pwm_hold_inactive == 0)
                    {
                        io.off();
                        state = OFF;
                        return;
                    }

                    // Switch to hold phase
                    repeat_count = 0;
                    ticks_count = 0;
                    io.to(last_phase);
                    state = PWM_HOLD;
                    return;
                }

                repeat_count++;
                ticks_count = 0;
                io.to(last_phase);
                return;
            }

            // Still in progress of pwm cycle => set output according to
            // counter state. To simplify things - do it on every tick.
            if (ticks_count < params->pwm_on_active) io.to(last_phase);
            else io.off();

            return;

        case PWM_HOLD:
            // This phase is endless. Check counter overflow only for repeat.

            ticks_count++;

            if (ticks_count >= (params->pwm_hold_active + params->pwm_hold_inactive))
            {
                ticks_count = 0;
            }

            if (ticks_count < params->pwm_hold_active) io.to(last_phase);
            else io.off();

        default: // OFF
            break;
        }
    }
};

#endif
