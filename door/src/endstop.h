#pragma once

#include "pico/stdlib.h"

class Endstop {
public:
    Endstop(uint pin, bool active_low = true);

    void init();
    bool isActive() const;
    bool isInactive() const;

private:
    uint pin_;
    bool active_low_;
};