#include "button.h"

Button::Button(uint pinNumber, bool activeLow, uint32_t debounceMs, uint32_t holdMs)
        : gpioPin(pinNumber),
            activeLowInput(activeLow),
            debounceIntervalMs(debounceMs),
            holdDurationMs(holdMs),
            rawState(false),
            stableState(false),
            lastChangeTime(nil_time),
            pressedTime(nil_time),
            pressEvent(false),
            releaseEvent(false),
            holdEvent(false),
            holdReported(false) {}

bool Button::readRaw() const {
    bool level = gpio_get(gpioPin);
    return activeLowInput ? !level : level;
}

void Button::init() {
    gpio_init(gpioPin);
    gpio_set_dir(gpioPin, GPIO_IN);

    if (activeLowInput) {
        gpio_pull_up(gpioPin);
    } else {
        gpio_pull_down(gpioPin);
    }

    rawState = readRaw();
    stableState = rawState;
    lastChangeTime = get_absolute_time();
    pressedTime = get_absolute_time();
}

void Button::update() {
    pressEvent = false;
    releaseEvent = false;
    holdEvent = false;

    bool raw = readRaw();

    if (raw != rawState) {
        rawState = raw;
        lastChangeTime = get_absolute_time();
    }

    if (raw != stableState &&
        absolute_time_diff_us(lastChangeTime, get_absolute_time()) >= debounceIntervalMs * 1000) {

        stableState = raw;

        if (stableState) {
            pressedTime = get_absolute_time();
            pressEvent = true;
            holdReported = false;
        } else {
            releaseEvent = true;
            holdReported = false;
        }
    }

    if (stableState && !holdReported) {
        if (absolute_time_diff_us(pressedTime, get_absolute_time()) >= holdDurationMs * 1000) {
            holdEvent = true;
            holdReported = true;
        }
    }
}

bool Button::pressed() {
    bool e = pressEvent;
    pressEvent = false;
    return e;
}

bool Button::released() {
    bool e = releaseEvent;
    releaseEvent = false;
    return e;
}

bool Button::held() {
    bool e = holdEvent;
    holdEvent = false;
    return e;
}

bool Button::isDown() const {
    return stableState;
}