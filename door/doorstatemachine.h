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
        MovingToTop,
        MovingToBottom,
        RecoveringOpenAfterCloseTimeout,
        Error
    };

    DoorStateMachine(Button &button,
                     Motor &motor,
                     Endstop &topEndstop,
                     Endstop &bottomEndstop,
                     StatusLed &statusLed,
                     uint32_t openTimeoutMs = 30000,
                     uint32_t closeTimeoutMs = 30000);

    void init();
    void update();
    bool requestOpen();
    bool requestClose();

    State state() const;
    const char *stateName() const;

private:
    bool errorClearHandled = false;

    Button &buttonRef;
    Motor &motorRef;
    Endstop &topEndstopRef;
    Endstop &bottomEndstopRef;
    StatusLed &statusLedRef;

    State currentState;
    uint32_t openTimeoutDurationMs;
    uint32_t closeTimeoutDurationMs;
    absolute_time_t motionStartTime;
    absolute_time_t ledBlinkStartTime;
    bool ledBlinkOn;

    void determineInitialState();
    void startMoveToTop();
    void startMoveToBottom();
    void enterError(const char *message);
    bool motionTimedOut(uint32_t timeoutMs) const;
    bool isMovingState() const;
    StatusLed::Color currentLedColor() const;
    void updateLed();
};