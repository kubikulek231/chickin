#include "motor.h"
#include "hardware/pwm.h"

Motor::Motor(uint direction1Pin, uint direction2Pin, bool invertDirection)
        : directionOnePin(direction1Pin),
            directionTwoPin(direction2Pin),
            directionInverted(invertDirection),
            directionOnePwmSlice(pwm_gpio_to_slice_num(direction1Pin)),
            directionOnePwmChannel(pwm_gpio_to_channel(direction1Pin)),
            directionTwoPwmSlice(pwm_gpio_to_slice_num(direction2Pin)),
            directionTwoPwmChannel(pwm_gpio_to_channel(direction2Pin)) {
}

void Motor::init() {
    gpio_set_function(directionOnePin, GPIO_FUNC_PWM);
    gpio_set_function(directionTwoPin, GPIO_FUNC_PWM);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWM_WRAP);
    pwm_init(directionOnePwmSlice, &config, false);

    if (directionTwoPwmSlice != directionOnePwmSlice) {
        pwm_init(directionTwoPwmSlice, &config, false);
    }

    pwm_set_chan_level(directionOnePwmSlice, directionOnePwmChannel, 0);
    pwm_set_chan_level(directionTwoPwmSlice, directionTwoPwmChannel, 0);
    pwm_set_enabled(directionOnePwmSlice, true);
    if (directionTwoPwmSlice != directionOnePwmSlice) {
        pwm_set_enabled(directionTwoPwmSlice, true);
    }
}

void Motor::driveForward() {
    pwm_set_chan_level(directionOnePwmSlice, directionOnePwmChannel, DEFAULT_DUTY);
    pwm_set_chan_level(directionTwoPwmSlice, directionTwoPwmChannel, 0);
}

void Motor::driveReverse() {
    pwm_set_chan_level(directionOnePwmSlice, directionOnePwmChannel, 0);
    pwm_set_chan_level(directionTwoPwmSlice, directionTwoPwmChannel, DEFAULT_DUTY);
}

void Motor::forward() {
    if (directionInverted) {
        driveReverse();
    } else {
        driveForward();
    }
}

void Motor::reverse() {
    if (directionInverted) {
        driveForward();
    } else {
        driveReverse();
    }
}

void Motor::stop() {
    pwm_set_chan_level(directionOnePwmSlice, directionOnePwmChannel, 0);
    pwm_set_chan_level(directionTwoPwmSlice, directionTwoPwmChannel, 0);
}