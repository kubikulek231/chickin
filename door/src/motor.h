#pragma once

#include "pico/stdlib.h"

class Motor {
public:
    static constexpr uint16_t DEFAULT_DUTY = 65535;
    static constexpr uint16_t PWM_WRAP = 65535; // 16 bit resolution, max duty cycle is 65535

    Motor(uint dir1_pin, uint dir2_pin, bool invert_direction = false);

    void init();
    void open();
    void close();
    void stop();

private:
    uint directionOnePin;
    uint directionTwoPin;
    bool directionInverted;

    void driveForward();
    void driveReverse();
    uint dir1_pwm_slice_;
    uint dir1_pwm_chan_;
    uint dir2_pwm_slice_;
    uint dir2_pwm_chan_;
};