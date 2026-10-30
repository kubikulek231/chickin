#pragma once

#include "pico/stdlib.h"

class Motor {
public:
    Motor(uint direction1Pin, uint direction2Pin, bool invertDirection = false);

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
};