#pragma once

#include "pico/stdlib.h"

class Button {
public:
    Button(uint pinNumber, bool activeLow = true, uint32_t debounceMs = 30, uint32_t holdMs = 3000);

    void init();
    void update();

    bool pressed();
    bool released();
    bool held();
    bool isDown() const;

private:
    uint gpioPin;
    bool activeLowInput;
    uint32_t debounceIntervalMs;
    uint32_t holdDurationMs;

    bool rawState;
    bool stableState;
    absolute_time_t lastChangeTime;
    absolute_time_t pressedTime;

    bool pressEvent;
    bool releaseEvent;
    bool holdEvent;
    bool holdReported;

    bool readRaw() const;
};