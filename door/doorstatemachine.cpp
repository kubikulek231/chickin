#include <stdio.h>
#include <doorstatemachine.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"

static PIO g_pio = pio0;
static uint g_sm = 0;

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(g) << 16) |
           ((uint32_t)(r) << 8) |
           (uint32_t)(b);
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static void setRgb(DoorStateMachine::StatusColor color)
{
    switch (color)
    {
    case DoorStateMachine::StatusColor::GREEN:
        put_pixel(g_pio, g_sm, urgb_u32(0, 20, 0));
        break;
    case DoorStateMachine::StatusColor::RED:
        put_pixel(g_pio, g_sm, urgb_u32(20, 0, 0));
        break;
    case DoorStateMachine::StatusColor::BLUE:
        put_pixel(g_pio, g_sm, urgb_u32(0, 0, 20));
        break;
    case DoorStateMachine::StatusColor::OFF:
    default:
        put_pixel(g_pio, g_sm, urgb_u32(0, 0, 0));
        break;
    }
}

DoorStateMachine::DoorStateMachine(Button &button,
                                   Motor &motor,
                                   Endstop &topEndstop,
                                   Endstop &middleEndstop,
                                   Endstop &bottomEndstop,
                                   uint32_t motorTimeoutMs)
    : button_(button),
      motor_(motor),
      topEndstop_(topEndstop),
      middleEndstop_(middleEndstop),
      bottomEndstop_(bottomEndstop),
      state_(State::UNKNOWN),
      lastFullState_(LastFullState::NONE),
      motorTimeoutMs_(motorTimeoutMs),
      motionStartTime_(nil_time),
      ledBlinkStartTime_(nil_time),
      ledBlinkOn_(false)
{
}

void DoorStateMachine::init()
{
    determineInitialState_();
    forceGoTopUntilTop_ = (state_ != State::TOP && state_ != State::ERROR);
    ledBlinkStartTime_ = get_absolute_time();
    ledBlinkOn_ = false;
    printf("Initial door state: %s\n", stateName());
}

DoorStateMachine::State DoorStateMachine::state() const
{
    return state_;
}

const char *DoorStateMachine::stateName() const
{
    switch (state_)
    {
    case State::UNKNOWN:
        return "UNKNOWN";
    case State::TOP:
        return "TOP";
    case State::BOTTOM:
        return "BOTTOM";
    case State::MIDDLE:
        return "MIDDLE";
    case State::MOVING_TO_TOP:
        return "MOVING_TO_TOP";
    case State::MOVING_TO_BOTTOM:
        return "MOVING_TO_BOTTOM";
    case State::MOVING_TO_MIDDLE_FROM_TOP:
        return "MOVING_TO_MIDDLE_FROM_TOP";
    case State::MOVING_TO_MIDDLE_FROM_BOTTOM:
        return "MOVING_TO_MIDDLE_FROM_BOTTOM";
    case State::RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT:
        return "RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT";
    case State::ERROR:
        return "ERROR";
    default:
        return "INVALID";
    }
}

void DoorStateMachine::determineInitialState_()
{
    bool top = topEndstop_.isActive();
    bool middle = middleEndstop_.isActive();
    bool bottom = bottomEndstop_.isActive();

    printf("Endstops: top=%d middle=%d bottom=%d\n", top, middle, bottom);

    if ((top && middle) || (top && bottom) || (middle && bottom))
    {
        motor_.stop();
        state_ = State::ERROR;
        lastFullState_ = LastFullState::NONE;
        printf("ERROR: invalid endstop combination\n");
    }
    else if (top)
    {
        state_ = State::TOP;
        lastFullState_ = LastFullState::TOP;
    }
    else if (bottom)
    {
        state_ = State::BOTTOM;
        lastFullState_ = LastFullState::BOTTOM;
    }
    else if (middle)
    {
        state_ = State::MIDDLE;
        lastFullState_ = LastFullState::NONE;
    }
    else
    {
        state_ = State::UNKNOWN;
        lastFullState_ = LastFullState::NONE;
    }
}

void DoorStateMachine::startMoveToTop_()
{
    motor_.open();
    motionStartTime_ = get_absolute_time();
    state_ = State::MOVING_TO_TOP;
    printf("Moving to TOP\n");
}

void DoorStateMachine::startMoveToBottom_()
{
    motor_.close();
    motionStartTime_ = get_absolute_time();
    state_ = State::MOVING_TO_BOTTOM;
    printf("Moving to BOTTOM\n");
}

void DoorStateMachine::startMoveToMiddleFromTop_()
{
    motor_.close();
    motionStartTime_ = get_absolute_time();
    state_ = State::MOVING_TO_MIDDLE_FROM_TOP;
    printf("Moving to MIDDLE from TOP\n");
}

void DoorStateMachine::startMoveToMiddleFromBottom_()
{
    motor_.open();
    motionStartTime_ = get_absolute_time();
    state_ = State::MOVING_TO_MIDDLE_FROM_BOTTOM;
    printf("Moving to MIDDLE from BOTTOM\n");
}

void DoorStateMachine::enterError_(const char *message)
{
    if (state_ != State::ERROR)
    {
        printf("ERROR: %s\n", message);
    }
    motor_.stop();
    state_ = State::ERROR;
}

bool DoorStateMachine::motionTimedOut_() const
{
    return absolute_time_diff_us(motionStartTime_, get_absolute_time()) >= (int64_t)motorTimeoutMs_ * 1000;
}

bool DoorStateMachine::isMovingState_() const
{
    switch (state_)
    {
    case State::MOVING_TO_TOP:
    case State::MOVING_TO_BOTTOM:
    case State::MOVING_TO_MIDDLE_FROM_TOP:
    case State::MOVING_TO_MIDDLE_FROM_BOTTOM:
    case State::RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT:
        return true;
    default:
        return false;
    }
}

DoorStateMachine::StatusColor DoorStateMachine::currentLedColor_() const
{
    if (state_ == State::ERROR)
    {
        return StatusColor::RED;
    }

    if (isMovingState_())
    {
        return StatusColor::BLUE;
    }

    return StatusColor::GREEN;
}

void DoorStateMachine::updateLed_()
{
    if (is_nil_time(ledBlinkStartTime_))
    {
        ledBlinkStartTime_ = get_absolute_time();
    }

    if (absolute_time_diff_us(ledBlinkStartTime_, get_absolute_time()) >= 1000000)
    {
        ledBlinkStartTime_ = get_absolute_time();
        ledBlinkOn_ = !ledBlinkOn_;
    }

    if (ledBlinkOn_)
    {
        setRgb(currentLedColor_());
    }
    else
    {
        setRgb(StatusColor::OFF);
    }
}

void DoorStateMachine::update()
{
    static absolute_time_t lastErrorPrintTime = nil_time;

    button_.update();

    if (button_.held())
    {
        printf("HOLD detected in state %s\n", stateName());

        switch (state_)
        {
        case State::TOP:
            startMoveToMiddleFromTop_();
            break;

        case State::BOTTOM:
            startMoveToMiddleFromBottom_();
            break;

        case State::MOVING_TO_BOTTOM:
            motor_.stop();
            startMoveToMiddleFromTop_();
            break;

        case State::MOVING_TO_TOP:
            motor_.stop();
            startMoveToMiddleFromBottom_();
            break;

        case State::ERROR:
            printf("Clearing ERROR state\n");
            motor_.stop();
            determineInitialState_();
            printf("State after clear: %s\n", stateName());
            lastErrorPrintTime = get_absolute_time();
            break;

        case State::UNKNOWN:
            printf("UNKNOWN state: long hold homing to TOP\n");
            startMoveToTop_();
            break;

        default:
            break;
        }
    }

    if (button_.pressed())
    {
        if (forceGoTopUntilTop_ && state_ != State::TOP)
        {
            if (state_ != State::MOVING_TO_TOP)
            {
                startMoveToTop_();
            }
        }
        else
        {
            switch (state_)
            {
            case State::TOP:
                startMoveToBottom_();
                break;

            case State::BOTTOM:
                startMoveToTop_();
                break;

            case State::MIDDLE:
                if (lastFullState_ == LastFullState::TOP)
                {
                    startMoveToBottom_();
                }
                else if (lastFullState_ == LastFullState::BOTTOM)
                {
                    startMoveToTop_();
                }
                else
                {
                    printf("MIDDLE state but no last full state known\n");
                }
                break;

            case State::UNKNOWN:
                printf("UNKNOWN state: homing to TOP\n");
                startMoveToTop_();
                break;

            default:
                break;
            }
        }
    }

    switch (state_)
    {
    case State::MOVING_TO_TOP:
        if (topEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::TOP;
            lastFullState_ = LastFullState::TOP;
            forceGoTopUntilTop_ = false;
            printf("Reached TOP\n");
        }
        else if (motionTimedOut_())
        {
            enterError_("Opening timeout");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::MOVING_TO_BOTTOM:
        if (bottomEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::BOTTOM;
            lastFullState_ = LastFullState::BOTTOM;
            printf("Reached BOTTOM\n");
        }
        else if (motionTimedOut_())
        {
            printf("ERROR: Closing timeout, reopening\n");
            motor_.open();
            motionStartTime_ = get_absolute_time();
            state_ = State::RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT;
        }
        break;

    case State::MOVING_TO_MIDDLE_FROM_TOP:
        if (middleEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::MIDDLE;
            lastFullState_ = LastFullState::TOP;
            printf("Reached MIDDLE from TOP\n");
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout moving to MIDDLE from TOP");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::MOVING_TO_MIDDLE_FROM_BOTTOM:
        if (middleEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::MIDDLE;
            lastFullState_ = LastFullState::BOTTOM;
            printf("Reached MIDDLE from BOTTOM\n");
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout moving to MIDDLE from BOTTOM");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT:
        if (topEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::ERROR;
            lastFullState_ = LastFullState::TOP;
            printf("Recovery complete, now in ERROR\n");
            lastErrorPrintTime = get_absolute_time();
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout during recovery open");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::ERROR:
        if (is_nil_time(lastErrorPrintTime) ||
            absolute_time_diff_us(lastErrorPrintTime, get_absolute_time()) >= 1000000)
        {
            printf("Currently in ERROR state\n");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::UNKNOWN:
    case State::TOP:
    case State::BOTTOM:
    case State::MIDDLE:
    default:
        break;
    }

    updateLed_();
}