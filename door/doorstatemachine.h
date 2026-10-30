#pragma once

#include "pico/stdlib.h"
#include <src/button.h>
#include <src/motor.h>
#include <src/endstop.h>
#include <src/statusled.h>

class DoorStateMachine
{
public:
    enum class State
    {
        Unknown,
        Top,
        Bottom,
        Middle,
        MovingToTop,
        MovingToBottom,
        MovingToMiddleFromTop,
        MovingToMiddleFromBottom,
        RecoveringOpenAfterCloseTimeout,
        Error
    };

    enum class LastFullState
    {
        None,
        Top,
        Bottom
    };

    DoorStateMachine(Button &button,
                     Motor &motor,
                     Endstop &topEndstop,
                     Endstop &middleEndstop,
                     Endstop &bottomEndstop,
                     StatusLed &statusLed,
                     uint32_t openTimeoutMs = 30000,
                     uint32_t closeTimeoutMs = 30000);

    void init();
    void update();

    State state() const;
    const char *stateName() const;

private:
    bool errorClearHandled = false;

    Button &buttonRef;
    Motor &motorRef;
    Endstop &topEndstopRef;
    Endstop &middleEndstopRef;
    Endstop &bottomEndstopRef;
    StatusLed &statusLedRef;

    State currentState;
    LastFullState lastFullState;
    uint32_t openTimeoutDurationMs;
    uint32_t closeTimeoutDurationMs;
    absolute_time_t motionStartTime;
    absolute_time_t ledBlinkStartTime;
    bool ledBlinkOn;

    void determineInitialState();
    void startMoveToTop();
    void startMoveToBottom();
    void startMoveToMiddleFromTop();
    void startMoveToMiddleFromBottom();
    void enterError(const char *message);
    bool motionTimedOut(uint32_t timeoutMs) const;
    bool isMovingState() const;
    StatusLed::Color currentLedColor() const;
    void updateLed();
};