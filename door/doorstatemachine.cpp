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
                                   uint32_t motorTimeoutMs,
                                   uint32_t motorTimeoutTopToBottomMs,
                                   uint32_t motorTimeoutMidToBottomMs)
    : button_(button),
      motor_(motor),
      topEndstop_(topEndstop),
      middleEndstop_(middleEndstop),
      bottomEndstop_(bottomEndstop),
      state_(State::UNKNOWN),
      motorTimeoutMs_(motorTimeoutMs),
      motorTimeoutTopToBottomMs_(motorTimeoutTopToBottomMs),
      motorTimeoutMidToBottomMs_(motorTimeoutMidToBottomMs),
      currentMotionTimeoutMs_(motorTimeoutMs),
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
    case State::TOP_TO_MID:
        return "TOP_TO_MID";
    case State::BOT_TO_MID:
        return "BOT_TO_MID";
    case State::MID_TO_TOP:
        return "MID_TO_TOP";
    case State::MID_TO_BOT:
        return "MID_TO_BOT";
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
        printf("ERROR: invalid endstop combination\n");
    }
    else if (top)
    {
        state_ = State::TOP;
    }
    else if (bottom)
    {
        state_ = State::BOTTOM;
    }
    else if (middle)
    {
        state_ = State::MIDDLE;
    }
    else
    {
        state_ = State::UNKNOWN;
    }
}

void DoorStateMachine::startMoveToTop_()
{
    motor_.open();
    motionStartTime_ = get_absolute_time();
    currentMotionTimeoutMs_ = motorTimeoutMs_;
    nextTargetAfterMiddle_ = NextTarget::NONE;
    state_ = State::MOVING_TO_TOP;
    printf("Moving to TOP\n");
}

void DoorStateMachine::startMoveToBottom_()
{
    motor_.close();
    motionStartTime_ = get_absolute_time();
    currentMotionTimeoutMs_ = (state_ == State::MIDDLE)
        ? motorTimeoutMidToBottomMs_
        : motorTimeoutTopToBottomMs_;
    nextTargetAfterMiddle_ = NextTarget::NONE;
    state_ = State::MOVING_TO_BOTTOM;
    printf("Moving to BOTTOM\n");
}

void DoorStateMachine::startTopToMid_(NextTarget nextTarget)
{
    motor_.close();
    motionStartTime_ = get_absolute_time();
    currentMotionTimeoutMs_ = motorTimeoutMs_;
    nextTargetAfterMiddle_ = nextTarget;
    state_ = State::TOP_TO_MID;
    printf("Moving from TOP to MIDDLE\n");
}

void DoorStateMachine::startBotToMid_(NextTarget nextTarget)
{
    motor_.open();
    motionStartTime_ = get_absolute_time();
    currentMotionTimeoutMs_ = motorTimeoutMs_;
    nextTargetAfterMiddle_ = nextTarget;
    state_ = State::BOT_TO_MID;
    printf("Moving from BOTTOM to MIDDLE\n");
}

void DoorStateMachine::startMidToTop_()
{
    motor_.open();
    motionStartTime_ = get_absolute_time();
    currentMotionTimeoutMs_ = motorTimeoutMs_;
    nextTargetAfterMiddle_ = NextTarget::NONE;
    state_ = State::MID_TO_TOP;
    printf("Moving from MIDDLE to TOP\n");
}

void DoorStateMachine::startMidToBot_()
{
    motor_.close();
    motionStartTime_ = get_absolute_time();
    currentMotionTimeoutMs_ = motorTimeoutMidToBottomMs_;
    nextTargetAfterMiddle_ = NextTarget::NONE;
    state_ = State::MID_TO_BOT;
    printf("Moving from MIDDLE to BOTTOM\n");
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
    return absolute_time_diff_us(motionStartTime_, get_absolute_time()) >= (int64_t)currentMotionTimeoutMs_ * 1000;
}

bool DoorStateMachine::isMovingState_() const
{
    switch (state_)
    {
    case State::MOVING_TO_TOP:
    case State::MOVING_TO_BOTTOM:
    case State::TOP_TO_MID:
    case State::BOT_TO_MID:
    case State::MID_TO_TOP:
    case State::MID_TO_BOT:
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
            startTopToMid_(NextTarget::NONE);
            break;

        case State::BOTTOM:
            startBotToMid_(NextTarget::NONE);
            break;

        case State::TOP_TO_MID:
        case State::BOT_TO_MID:
            nextTargetAfterMiddle_ = NextTarget::NONE;
            break;

        case State::ERROR:
            printf("Clearing ERROR state\n");
            motor_.stop();
            determineInitialState_();
            forceGoTopUntilTop_ = true;
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
                startTopToMid_(NextTarget::BOTTOM);
                break;

            case State::BOTTOM:
                startBotToMid_(NextTarget::TOP);
                break;

            case State::MIDDLE:
                startMidToBot_();
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

    case State::TOP_TO_MID:
        if (middleEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::MIDDLE;
            printf("Reached MIDDLE from TOP\n");
            if (nextTargetAfterMiddle_ == NextTarget::BOTTOM)
            {
                startMidToBot_();
            }
            else
            {
                nextTargetAfterMiddle_ = NextTarget::NONE;
            }
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout moving from TOP to MIDDLE");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::BOT_TO_MID:
        if (middleEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::MIDDLE;
            printf("Reached MIDDLE from BOTTOM\n");
            if (nextTargetAfterMiddle_ == NextTarget::TOP)
            {
                startMidToTop_();
            }
            else
            {
                nextTargetAfterMiddle_ = NextTarget::NONE;
            }
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout moving from BOTTOM to MIDDLE");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::MID_TO_TOP:
        if (topEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::TOP;
            forceGoTopUntilTop_ = false;
            printf("Reached TOP from MIDDLE\n");
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout moving from MIDDLE to TOP");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::MID_TO_BOT:
        if (bottomEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::BOTTOM;
            printf("Reached BOTTOM from MIDDLE\n");
        }
        else if (motionTimedOut_())
        {
            enterError_("Timeout moving from MIDDLE to BOTTOM");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT:
        if (topEndstop_.isActive())
        {
            motor_.stop();
            state_ = State::ERROR;
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