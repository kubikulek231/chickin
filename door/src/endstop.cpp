#include "endstop.h"

Endstop::Endstop(uint pinNumber, bool activeLow)
    : gpioPin(pinNumber), activeLowInput(activeLow) {
}

void Endstop::init() {
    gpio_init(gpioPin);
    gpio_set_dir(gpioPin, GPIO_IN);

    if (activeLowInput) {
        gpio_pull_up(gpioPin);
    } else {
        gpio_pull_down(gpioPin);
    }
}

bool Endstop::isActive() const {
    bool level = gpio_get(gpioPin);
    return activeLowInput ? level : !level; // End stop is connected to NC
}

bool Endstop::isInactive() const {
    return !isActive();
}