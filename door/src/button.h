#pragma once

#include "pico/stdlib.h"

class Button {
public:
    Button(uint pin, bool active_low = true, uint32_t debounce_ms = 30, uint32_t hold_ms = 3000);

    void init();
    void update();

    bool pressed();
    bool released();
    bool held();
    bool isDown() const;

private:
    uint pin_;
    bool active_low_;
    uint32_t debounce_ms_;
    uint32_t hold_ms_;

    bool raw_state_;
    bool stable_state_;
    absolute_time_t last_change_time_;
    absolute_time_t pressed_time_;

    bool press_event_;
    bool release_event_;
    bool hold_event_;
    bool hold_reported_;

    bool readRaw() const;
};