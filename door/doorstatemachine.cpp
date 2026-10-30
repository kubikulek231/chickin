#include <stdio.h>
#include <doorstatemachine.h>
#include "pico/stdlib.h"

DoorStateMachine::DoorStateMachine(Button &button,
                                   Motor &motor,
                                   Endstop &topEndstop,
                                   Endstop &bottomEndstop,
                                   StatusLed &statusLed,
                                   uint32_t openTimeoutMs,
                                   uint32_t closeTimeoutMs)
    : buttonRef(button),
      motorRef(motor),
      topEndstopRef(topEndstop),
      bottomEndstopRef(bottomEndstop),
      statusLedRef(statusLed),
      currentState(State::Unknown),
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
    case State::MovingToTop:
        return "MOVING_TO_TOP";
    case State::MovingToBottom:
        return "MOVING_TO_BOTTOM";
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
    bool bottom = bottomEndstopRef.isActive();

    printf("Endstops: top=%d bottom=%d\n", top, bottom);

    if (top && bottom)
    {
        motorRef.stop();
        currentState = State::Error;
        printf("ERROR: invalid endstop combination\n");
    }
    else if (top)
    {
        currentState = State::Top;
    }
    else if (bottom)
    {
        currentState = State::Bottom;
    }
    else
    {
        currentState = State::Unknown;
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

    case State::RecoveringOpenAfterCloseTimeout:
        if (topEndstopRef.isActive())
        {
            motorRef.stop();
            currentState = State::Error;
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
    default:
        break;
    }

    updateLed();
}

bool DoorStateMachine::requestOpen()
{
    switch (currentState)
    {
    case State::Top:
        return true;
    case State::Bottom:
    case State::Unknown:
        startMoveToTop();
        return true;
    default:
        return false;
    }
}

bool DoorStateMachine::requestClose()
{
    switch (currentState)
    {
    case State::Bottom:
        return true;
    case State::Top:
        startMoveToBottom();
        return true;
    default:
        return false;
    }
}