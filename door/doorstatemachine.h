#pragma once

#include "pico/stdlib.h"
#include <src/button.h>
#include <src/motor.h>
#include <src/endstop.h>

class DoorStateMachine
{
public:
    enum class State
    {
        UNKNOWN,
        TOP,
        BOTTOM,
        MIDDLE,
        MOVING_TO_TOP,
        MOVING_TO_BOTTOM,
        MOVING_TO_MIDDLE_FROM_TOP,
        MOVING_TO_MIDDLE_FROM_BOTTOM,
        RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT,
        ERROR
    };

    enum class LastFullState
    {
        NONE,
        TOP,
        BOTTOM
    };

    // Example status colors
    enum class StatusColor
    {
        OFF,
        GREEN,
        RED,
        BLUE
    };

    DoorStateMachine(Button &button,
                     Motor &motor,
                     Endstop &topEndstop,
                     Endstop &middleEndstop,
                     Endstop &bottomEndstop,
                     uint32_t motorTimeoutMs = 10000);

    void init();
    void update();

    State state() const;
    const char *stateName() const;

private:
    bool errorClearHandled_ = false;

    Button &button_;
    Motor &motor_;
    Endstop &topEndstop_;
    Endstop &middleEndstop_;
    Endstop &bottomEndstop_;

    State state_;
    LastFullState lastFullState_;
    uint32_t motorTimeoutMs_;
    absolute_time_t motionStartTime_;
    absolute_time_t ledBlinkStartTime_;
    bool ledBlinkOn_;
    bool forceGoTopUntilTop_ = false;

    void determineInitialState_();
    void startMoveToTop_();
    void startMoveToBottom_();
    void startMoveToMiddleFromTop_();
    void startMoveToMiddleFromBottom_();
    void enterError_(const char *message);
    bool motionTimedOut_() const;
    bool isMovingState_() const;
    StatusColor currentLedColor_() const;
    void updateLed_();
};