#include "endstop.h"

Endstop::Endstop(uint pin, bool active_low)
    : pin_(pin), active_low_(active_low) {
}

void Endstop::init() {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);

    if (active_low_) {
        gpio_pull_up(pin_);
    } else {
        gpio_pull_down(pin_);
    }
}

bool Endstop::isActive() const {
    bool level = gpio_get(pin_);
    return active_low_ ? level : !level; // End stop is connected to NC
}

bool Endstop::isInactive() const {
    return !isActive();
}