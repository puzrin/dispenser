#ifndef __STEPPER_CONTROL__
#define __STEPPER_CONTROL__

#include "stepper.h"
#include "etl/cyclic_value.h"

// Use instead of "cyclic_value" when borders can be updated on the fly
#define INC_BY_MOD(X, Y) if ((++X) >= (Y)) X = 0;

template <typename STEPPER_IO>
class StepperControl
{
    Stepper<STEPPER_IO> stepper;

    enum State {
        STATE_STOPPED,

        STATE_FAST_FORWARD,
        STATE_FAST_BACK,

        STATE_FLOW_UNRETRACT,
        STATE_FLOW,
        STATE_FLOW_RETRACT,

        STATE_DOSE_UNRETRACT,
        STATE_DOSE,
        STATE_DOSE_RETRACT

    } state = STATE_STOPPED;

    enum Cmd {
        CMD_NONE = 0,
        CMD_FAST_FORWARD,
        CMD_FAST_BACK,
        CMD_FLOW,
        CMD_DOSE,
        CMD_STOP,
    };

    volatile Cmd pending_cmd = CMD_NONE;
    uint16_t ticks_count = 0;
    uint16_t steps_count = 0;
    uint16_t dose_count = 0;

    void to_state(State new_state)
    {
        ticks_count = 0;
        steps_count = 0;
        state = new_state;
    }

    void step_next()
    {
        current_stepper_phase++;
        stepper.go(current_stepper_phase);
    }

    void step_prev()
    {
        current_stepper_phase++;
        stepper.go(current_stepper_phase);
    }

    void off()
    {
        // Do not disable motor power. Use stepper's "idle" feature, to reduce
        // current but keep motor position with little effort.
        // If full disable needed - uncomment line below.

        // stepper.off()
    }

    void process_commands()
    {
        switch (pending_cmd)
        {
        case CMD_FAST_FORWARD:
            // Switch immediately from any state
            to_state(STATE_FAST_FORWARD);
            break;

        case CMD_FAST_BACK:
            // Switch immediately from any state
            to_state(STATE_FAST_BACK);
            break;

        case CMD_STOP:
            switch (state)
            {
            case STATE_FAST_FORWARD:
            case STATE_FAST_BACK:
                to_state(STATE_STOPPED);
                break;

            case STATE_FLOW:
                to_state(STATE_FLOW_RETRACT);
                break;

            case STATE_FLOW_UNRETRACT:
            case STATE_FLOW_RETRACT:
                // Ignore until state ended
                return;

            // Other combinations should never happen
            default:
                off();
                to_state(STATE_STOPPED);
                break;
            }
            break;

        case CMD_FLOW:
            switch (state)
            {
            case STATE_STOPPED:
                to_state(STATE_FLOW_UNRETRACT);
                break;

            case STATE_FLOW:
            case STATE_FLOW_UNRETRACT:
                break;

            case STATE_FLOW_RETRACT:
                return;

            case STATE_DOSE:
                // Jump to flow directly, skip retracts
                to_state(STATE_FLOW);
                break;

            case STATE_DOSE_UNRETRACT:
            case STATE_DOSE_RETRACT:
                // Ignore until state ended
                return;

            // Other combinations should never happen
            default:
                off();
                to_state(STATE_STOPPED);
                break;
            }
            break;

        case CMD_DOSE:
            switch (state)
            {
            case STATE_STOPPED:
                to_state(STATE_DOSE_UNRETRACT);
                dose_count = dose_steps;
                break;

            case STATE_DOSE:
            case STATE_DOSE_UNRETRACT:
                // I receive one more request while in this state,
                // just increase dose size without interrupting process.
                dose_count += dose_steps;
                break;

            case STATE_DOSE_RETRACT:
                // Ignore until state ended
                return;

            case STATE_FLOW:
                // Jump to dose directly, skip retracts
                to_state(STATE_FLOW);
                dose_count = dose_steps;
                break;

            case STATE_FLOW_UNRETRACT:
            case STATE_FLOW_RETRACT:
                // Ignore until state ended
                return;

            // Other combinations should never happen
            default:
                off();
                to_state(STATE_STOPPED);
                break;
            }
            break;

        default:
            // This should never happen
            off();
            to_state(STATE_STOPPED);
            break;
        }

        pending_cmd = CMD_NONE;
    }

public:

    StepperControl(StepperPwmParams * _params) : stepper(_params) {}

    //
    // This variables should be initialised & updated externally,
    // according to config
    //

    // Defines rotation speed. Length of 1 step in 0.1us/sec
    uint16_t flow_pulse_period = 50;
    uint16_t retract_pulse_period = 20;

    // Current motor position, to calculate next one.
    etl::cyclic_value<uint8_t, 0, 3>  current_stepper_phase;

    // Dose size, in motor steps
    uint16_t dose_steps = 10;
    uint16_t retract_steps = 2;

    //
    // Public api
    //

    void flow() { pending_cmd = CMD_FLOW; };
    void stop() { pending_cmd = CMD_STOP; };
    void fast_forward() { pending_cmd = CMD_FAST_FORWARD; };
    void fast_back() { pending_cmd = CMD_FAST_BACK; };
    void dose() { pending_cmd = CMD_DOSE; };

    void tick() {
        process_commands();

        switch (state)
        {
        case STATE_FAST_FORWARD:
            if (ticks_count == 0) step_next();

            INC_BY_MOD(ticks_count, retract_pulse_period);
            break;

        case STATE_FAST_BACK:
            if (ticks_count == 0) step_prev();

            INC_BY_MOD(ticks_count, retract_pulse_period);
            break;

        case STATE_FLOW_UNRETRACT:
            if (ticks_count == 0) {
                if (++steps_count > retract_steps)
                {
                    to_state(STATE_FLOW);
                    break;
                }
                step_next();
            }
            INC_BY_MOD(ticks_count, retract_pulse_period);
            break;

        case STATE_FLOW:
            if (ticks_count == 0) step_next();

            INC_BY_MOD(ticks_count, flow_pulse_period);
            break;

        case STATE_FLOW_RETRACT:
            if (ticks_count == 0) {
                if (++steps_count > retract_steps)
                {
                    to_state(STATE_STOPPED);
                    break;
                }
                step_prev();
            }
            INC_BY_MOD(ticks_count, retract_pulse_period);
            break;

        case STATE_DOSE_UNRETRACT:
            if (ticks_count == 0) {
                if (++steps_count > retract_steps)
                {
                    to_state(STATE_DOSE);
                    break;
                }
                step_next();
            }
            INC_BY_MOD(ticks_count, retract_pulse_period);
            break;

        case STATE_DOSE:
            if (ticks_count == 0) {
                if (--dose_count == 0)
                {
                    to_state(STATE_DOSE_RETRACT);
                    break;
                }
                step_next();
            }
            INC_BY_MOD(ticks_count, flow_pulse_period);
            break;

        case STATE_DOSE_RETRACT:
            if (ticks_count == 0) {
                if (++steps_count > retract_steps)
                {
                    to_state(STATE_STOPPED);
                    break;
                }
                step_prev();
            }
            INC_BY_MOD(ticks_count, retract_pulse_period);
            break;


        case STATE_STOPPED:
        default:
            break;
        }

        stepper.tick();
    };
};


#endif
