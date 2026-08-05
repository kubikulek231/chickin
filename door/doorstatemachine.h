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
        TOP_TO_MID,
        BOT_TO_MID,
        MID_TO_TOP,
        MID_TO_BOT,
        MOVING_TO_TOP,
        MOVING_TO_BOTTOM,
        RECOVERING_OPEN_AFTER_CLOSE_TIMEOUT,
        ERROR
    };

    enum class NextTarget
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
                     uint32_t motorTimeoutMs = 6500,
                     uint32_t motorTimeoutTopToBottomMs = 6500,
                     uint32_t motorTimeoutMidToBottomMs = 1500);

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
    uint32_t motorTimeoutMs_;
    uint32_t motorTimeoutTopToBottomMs_;
    uint32_t motorTimeoutMidToBottomMs_;
    uint32_t currentMotionTimeoutMs_;
    absolute_time_t motionStartTime_;
    absolute_time_t ledBlinkStartTime_;
    bool ledBlinkOn_;
    bool forceGoTopUntilTop_ = false;
    NextTarget nextTargetAfterMiddle_ = NextTarget::NONE;

    void determineInitialState_();
    void startMoveToTop_();
    void startMoveToBottom_();
    void startTopToMid_(NextTarget nextTarget = NextTarget::NONE);
    void startBotToMid_(NextTarget nextTarget = NextTarget::NONE);
    void startMidToTop_();
    void startMidToBot_();
    void enterError_(const char *message);
    bool motionTimedOut_() const;
    bool isMovingState_() const;
    StatusColor currentLedColor_() const;
    void updateLed_();
};