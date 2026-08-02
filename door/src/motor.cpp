#include "motor.h"

Motor::Motor(uint dir1_pin, uint dir2_pin, bool invert_direction)
    : dir1_pin_(dir1_pin),
      dir2_pin_(dir2_pin),
      invert_direction_(invert_direction) {
}

void Motor::init() {
    gpio_init(dir1_pin_);
    gpio_set_dir(dir1_pin_, GPIO_OUT);
    gpio_put(dir1_pin_, 0);

    gpio_init(dir2_pin_);
    gpio_set_dir(dir2_pin_, GPIO_OUT);
    gpio_put(dir2_pin_, 0);
}

void Motor::driveForward_() {
    gpio_put(dir1_pin_, 1);
    gpio_put(dir2_pin_, 0);
}

void Motor::driveReverse_() {
    gpio_put(dir1_pin_, 0);
    gpio_put(dir2_pin_, 1);
}

void Motor::open() {
    if (invert_direction_) {
        driveReverse_();
    } else {
        driveForward_();
    }
}

void Motor::close() {
    if (invert_direction_) {
        driveForward_();
    } else {
        driveReverse_();
    }
}

void Motor::stop() {
    gpio_put(dir1_pin_, 0);
    gpio_put(dir2_pin_, 0);
}