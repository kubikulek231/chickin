#include "motor.h"
#include "hardware/pwm.h"

Motor::Motor(uint dir1_pin, uint dir2_pin, bool invert_direction)
    : dir1_pin_(dir1_pin),
      dir2_pin_(dir2_pin),
      invert_direction_(invert_direction),
      dir1_pwm_slice_(0),
      dir1_pwm_chan_(0),
      dir2_pwm_slice_(0),
      dir2_pwm_chan_(0) {
}

void Motor::init() {
    gpio_set_function(dir1_pin_, GPIO_FUNC_PWM);
    gpio_set_function(dir2_pin_, GPIO_FUNC_PWM);

    dir1_pwm_slice_ = pwm_gpio_to_slice_num(dir1_pin_);
    dir1_pwm_chan_ = pwm_gpio_to_channel(dir1_pin_);
    dir2_pwm_slice_ = pwm_gpio_to_slice_num(dir2_pin_);
    dir2_pwm_chan_ = pwm_gpio_to_channel(dir2_pin_);

    pwm_set_wrap(dir1_pwm_slice_, PWM_WRAP);
    pwm_set_wrap(dir2_pwm_slice_, PWM_WRAP);

    pwm_set_chan_level(dir1_pwm_slice_, dir1_pwm_chan_, 0);
    pwm_set_chan_level(dir2_pwm_slice_, dir2_pwm_chan_, 0);

    pwm_set_enabled(dir1_pwm_slice_, true);
    pwm_set_enabled(dir2_pwm_slice_, true);
}

void Motor::driveForward_(uint16_t duty) {
    pwm_set_chan_level(dir1_pwm_slice_, dir1_pwm_chan_, duty);
    pwm_set_chan_level(dir2_pwm_slice_, dir2_pwm_chan_, 0);
}

void Motor::driveReverse_(uint16_t duty) {
    pwm_set_chan_level(dir1_pwm_slice_, dir1_pwm_chan_, 0);
    pwm_set_chan_level(dir2_pwm_slice_, dir2_pwm_chan_, duty);
}

void Motor::open() {
    if (invert_direction_) {
        driveReverse_(DEFAULT_DUTY);
    } else {
        driveForward_(DEFAULT_DUTY);
    }
}

void Motor::close() {
    if (invert_direction_) {
        driveForward_(DEFAULT_DUTY);
    } else {
        driveReverse_(DEFAULT_DUTY);
    }
}

void Motor::stop() {
    pwm_set_chan_level(dir1_pwm_slice_, dir1_pwm_chan_, 0);
    pwm_set_chan_level(dir2_pwm_slice_, dir2_pwm_chan_, 0);
}