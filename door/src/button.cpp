#include "button.h"

Button::Button(uint pin, bool active_low, uint32_t debounce_ms, uint32_t hold_ms)
    : pin_(pin),
      active_low_(active_low),
      debounce_ms_(debounce_ms),
      hold_ms_(hold_ms),
      raw_state_(false),
      stable_state_(false),
      last_change_time_(nil_time),
      pressed_time_(nil_time),
      press_event_(false),
      release_event_(false),
      hold_event_(false),
      hold_reported_(false) {}

bool Button::readRaw() const {
    bool level = gpio_get(pin_);
    return active_low_ ? !level : level;
}

void Button::init() {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);

    if (active_low_) {
        gpio_pull_up(pin_);
    } else {
        gpio_pull_down(pin_);
    }

    raw_state_ = readRaw();
    stable_state_ = raw_state_;
    last_change_time_ = get_absolute_time();
    pressed_time_ = get_absolute_time();
}

void Button::update() {
    press_event_ = false;
    release_event_ = false;
    hold_event_ = false;

    bool raw = readRaw();

    if (raw != raw_state_) {
        raw_state_ = raw;
        last_change_time_ = get_absolute_time();
    }

    if (raw != stable_state_ &&
        absolute_time_diff_us(last_change_time_, get_absolute_time()) >= debounce_ms_ * 1000) {

        stable_state_ = raw;

        if (stable_state_) {
            pressed_time_ = get_absolute_time();
            press_event_ = true;
            hold_reported_ = false;
        } else {
            release_event_ = true;
            hold_reported_ = false;
        }
    }

    if (stable_state_ && !hold_reported_) {
        if (absolute_time_diff_us(pressed_time_, get_absolute_time()) >= hold_ms_ * 1000) {
            hold_event_ = true;
            hold_reported_ = true;
        }
    }
}

bool Button::pressed() {
    bool e = press_event_;
    press_event_ = false;
    return e;
}

bool Button::released() {
    bool e = release_event_;
    release_event_ = false;
    return e;
}

bool Button::held() {
    bool e = hold_event_;
    hold_event_ = false;
    return e;
}

bool Button::isDown() const {
    return stable_state_;
}