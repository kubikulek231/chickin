#include <stdio.h>
#include <doorstatemachine.h>
#include "pico/stdlib.h"

DoorStateMachine::DoorStateMachine(Button &button,
                                   Motor &motor,
                                   Endstop &topEndstop,
                                   Endstop &middleEndstop,
                                   Endstop &bottomEndstop,
                                   StatusLed &statusLed,
                                   uint32_t openTimeoutMs,
                                   uint32_t closeTimeoutMs)
    : buttonRef(button),
      motorRef(motor),
      topEndstopRef(topEndstop),
      middleEndstopRef(middleEndstop),
      bottomEndstopRef(bottomEndstop),
      statusLedRef(statusLed),
      currentState(State::Unknown),
      lastFullState(LastFullState::None),
      openTimeoutDurationMs(openTimeoutMs),
      closeTimeoutDurationMs(closeTimeoutMs),
      motionStartTime(nil_time),
      ledBlinkStartTime(nil_time),
      ledBlinkOn(false)
{
}

void DoorStateMachine::init()
{
    determineInitialState();
    ledBlinkStartTime = get_absolute_time();
    ledBlinkOn = false;
    printf("Initial door state: %s\n", stateName());
}

DoorStateMachine::State DoorStateMachine::state() const
{
    return currentState;
}

const char *DoorStateMachine::stateName() const
{
    switch (currentState)
    {
    case State::Unknown:
        return "UNKNOWN";
    case State::Top:
        return "TOP";
    case State::Bottom:
        return "BOTTOM";
    case State::Middle:
        return "MIDDLE";
    case State::MovingToTop:
        return "MOVING_TO_TOP";
    case State::MovingToBottom:
        return "MOVING_TO_BOTTOM";
    case State::MovingToMiddleFromTop:
        return "MOVING_TO_MIDDLE_FROM_TOP";
    case State::MovingToMiddleFromBottom:
        return "MOVING_TO_MIDDLE_FROM_BOTTOM";
    case State::RecoveringOpenAfterCloseTimeout:
        return "RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT";
    case State::Error:
        return "ERROR";
    default:
        return "INVALID";
    }
}

void DoorStateMachine::determineInitialState()
{
    bool top = topEndstopRef.isActive();
    bool middle = middleEndstopRef.isActive();
    bool bottom = bottomEndstopRef.isActive();

    printf("Endstops: top=%d middle=%d bottom=%d\n", top, middle, bottom);

    if ((top && middle) || (top && bottom) || (middle && bottom))
    {
        motorRef.stop();
        currentState = State::Error;
        lastFullState = LastFullState::None;
        printf("ERROR: invalid endstop combination\n");
    }
    else if (top)
    {
        currentState = State::Top;
        lastFullState = LastFullState::Top;
    }
    else if (bottom)
    {
        currentState = State::Bottom;
        lastFullState = LastFullState::Bottom;
    }
    else if (middle)
    {
        currentState = State::Middle;
        lastFullState = LastFullState::None;
    }
    else
    {
        currentState = State::Unknown;
        lastFullState = LastFullState::None;
    }
}

void DoorStateMachine::startMoveToTop()
{
    motorRef.forward();
    motionStartTime = get_absolute_time();
    currentState = State::MovingToTop;
    printf("Moving to TOP\n");
}

void DoorStateMachine::startMoveToBottom()
{
    motorRef.reverse();
    motionStartTime = get_absolute_time();
    currentState = State::MovingToBottom;
    printf("Moving to BOTTOM\n");
}

void DoorStateMachine::startMoveToMiddleFromTop()
{
    motorRef.reverse();
    motionStartTime = get_absolute_time();
    currentState = State::MovingToMiddleFromTop;
    printf("Moving to MIDDLE from TOP\n");
}

void DoorStateMachine::startMoveToMiddleFromBottom()
{
    motorRef.forward();
    motionStartTime = get_absolute_time();
    currentState = State::MovingToMiddleFromBottom;
    printf("Moving to MIDDLE from BOTTOM\n");
}

void DoorStateMachine::enterError(const char *message)
{
    if (currentState != State::Error)
    {
        printf("ERROR: %s\n", message);
    }
    motorRef.stop();
    currentState = State::Error;
}

bool DoorStateMachine::motionTimedOut(uint32_t timeoutMs) const
{
    return absolute_time_diff_us(motionStartTime, get_absolute_time()) >= (int64_t)timeoutMs * 1000;
}

bool DoorStateMachine::isMovingState() const
{
    switch (currentState)
    {
    case State::MovingToTop:
    case State::MovingToBottom:
    case State::MovingToMiddleFromTop:
    case State::MovingToMiddleFromBottom:
    case State::RecoveringOpenAfterCloseTimeout:
        return true;
    default:
        return false;
    }
}

StatusLed::Color DoorStateMachine::currentLedColor() const
{
    if (currentState == State::Unknown)
    {
        return StatusLed::Color::Pink;
    }

    if (currentState == State::Error)
    {
        return StatusLed::Color::Red;
    }

    if (isMovingState())
    {
        return StatusLed::Color::Blue;
    }

    return StatusLed::Color::Green;
}

void DoorStateMachine::updateLed()
{
    if (is_nil_time(ledBlinkStartTime))
    {
        ledBlinkStartTime = get_absolute_time();
    }

    if (absolute_time_diff_us(ledBlinkStartTime, get_absolute_time()) >= 1000000)
    {
        ledBlinkStartTime = get_absolute_time();
        ledBlinkOn = !ledBlinkOn;
    }

    if (ledBlinkOn)
    {
        statusLedRef.set(currentLedColor());
    }
    else
    {
        statusLedRef.set(StatusLed::Color::Off);
    }
}

void DoorStateMachine::update()
{
    static absolute_time_t lastErrorPrintTime = nil_time;

    buttonRef.update();

    if (buttonRef.held())
    {
        printf("HOLD detected in state %s\n", stateName());

        switch (currentState)
        {
        case State::Top:
            startMoveToMiddleFromTop();
            break;

        case State::Bottom:
            startMoveToMiddleFromBottom();
            break;

        case State::MovingToBottom:
            motorRef.stop();
            startMoveToMiddleFromTop();
            break;

        case State::MovingToTop:
            motorRef.stop();
            startMoveToMiddleFromBottom();
            break;

        case State::Error:
            printf("Clearing ERROR state\n");
            motorRef.stop();
            determineInitialState();
            printf("State after clear: %s\n", stateName());
            lastErrorPrintTime = get_absolute_time();
            break;

        case State::Unknown:
            printf("UNKNOWN state: long hold homing to TOP\n");
            startMoveToTop();
            break;

        default:
            break;
        }
    }

    if (buttonRef.pressed())
    {
        switch (currentState)
        {
        case State::Top:
            startMoveToBottom();
            break;

        case State::Bottom:
            startMoveToTop();
            break;

        case State::Middle:
            if (lastFullState == LastFullState::Top)
            {
                startMoveToBottom();
            }
            else if (lastFullState == LastFullState::Bottom)
            {
                startMoveToTop();
            }
            else
            {
                printf("MIDDLE state but no last full state known\n");
            }
            break;

        case State::Unknown:
            printf("UNKNOWN state: homing to TOP\n");
            startMoveToTop();
            break;

        default:
            break;
        }
    }

    switch (currentState)
    {
    case State::MovingToTop:
        if (topEndstopRef.isActive())
        {
            motorRef.stop();
            currentState = State::Top;
            lastFullState = LastFullState::Top;
            printf("Reached TOP\n");
        }
        else if (motionTimedOut(openTimeoutDurationMs))
        {
            enterError("Opening timeout");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::MovingToBottom:
        if (bottomEndstopRef.isActive())
        {
            motorRef.stop();
            currentState = State::Bottom;
            lastFullState = LastFullState::Bottom;
            printf("Reached BOTTOM\n");
        }
        else if (motionTimedOut(closeTimeoutDurationMs))
        {
            printf("ERROR: Closing timeout, reopening\n");
            motorRef.forward();
            motionStartTime = get_absolute_time();
            currentState = State::RecoveringOpenAfterCloseTimeout;
        }
        break;

    case State::MovingToMiddleFromTop:
        if (middleEndstopRef.isActive())
        {
            motorRef.stop();
            currentState = State::Middle;
            lastFullState = LastFullState::Top;
            printf("Reached MIDDLE from TOP\n");
        }
        else if (motionTimedOut(closeTimeoutDurationMs))
        {
            enterError("Timeout moving to MIDDLE from TOP");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::MovingToMiddleFromBottom:
        if (middleEndstopRef.isActive())
        {
            motorRef.stop();
            currentState = State::Middle;
            lastFullState = LastFullState::Bottom;
            printf("Reached MIDDLE from BOTTOM\n");
        }
        else if (motionTimedOut(openTimeoutDurationMs))
        {
            enterError("Timeout moving to MIDDLE from BOTTOM");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::RecoveringOpenAfterCloseTimeout:
        if (topEndstopRef.isActive())
        {
            motorRef.stop();
            currentState = State::Error;
            lastFullState = LastFullState::Top;
            printf("Recovery complete, now in ERROR\n");
            lastErrorPrintTime = get_absolute_time();
        }
        else if (motionTimedOut(openTimeoutDurationMs))
        {
            enterError("Timeout during recovery open");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::Error:
        if (is_nil_time(lastErrorPrintTime) ||
            absolute_time_diff_us(lastErrorPrintTime, get_absolute_time()) >= 1000000)
        {
            printf("Currently in ERROR state\n");
            lastErrorPrintTime = get_absolute_time();
        }
        break;

    case State::Unknown:
    case State::Top:
    case State::Bottom:
    case State::Middle:
    default:
        break;
    }

    updateLed();
}