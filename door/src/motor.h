#pragma once

#include "pico/stdlib.h"

class Motor {
public:
    static constexpr uint16_t PWM_WRAP = 6249; // About 20 kHz at a 125 MHz system clock
    static constexpr uint16_t DEFAULT_DUTY = PWM_WRAP + 1; // 100% duty cycle

    Motor(uint direction1Pin, uint direction2Pin, bool invertDirection = false);

    void init();
    void forward();
    void reverse();
    void stop();

private:
    uint directionOnePin;
    uint directionTwoPin;
    bool directionInverted;

    void driveForward();
    void driveReverse();
    uint directionOnePwmSlice;
    uint directionOnePwmChannel;
    uint directionTwoPwmSlice;
    uint directionTwoPwmChannel;
};