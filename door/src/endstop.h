#pragma once

#include "pico/stdlib.h"

class Endstop {
public:
    Endstop(uint pinNumber, bool activeLow = true);

    void init();
    bool isActive() const;
    bool isInactive() const;

private:
    uint gpioPin;
    bool activeLowInput;
};