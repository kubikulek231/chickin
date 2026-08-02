#pragma once

#include "pico/stdlib.h"

class Motor {
public:
    Motor(uint dir1_pin, uint dir2_pin, bool invert_direction = false);

    void init();
    void open();
    void close();
    void stop();

private:
    uint dir1_pin_;
    uint dir2_pin_;
    bool invert_direction_;

    void driveForward_();
    void driveReverse_();
};