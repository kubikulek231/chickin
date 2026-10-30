#include "motor.h"

Motor::Motor(uint direction1Pin, uint direction2Pin, bool invertDirection)
    : directionOnePin(direction1Pin),
      directionTwoPin(direction2Pin),
      directionInverted(invertDirection) {
}

void Motor::init() {
    gpio_init(directionOnePin);
    gpio_set_dir(directionOnePin, GPIO_OUT);
    gpio_put(directionOnePin, 0);

    gpio_init(directionTwoPin);
    gpio_set_dir(directionTwoPin, GPIO_OUT);
    gpio_put(directionTwoPin, 0);
}

void Motor::driveForward() {
    gpio_put(directionOnePin, 1);
    gpio_put(directionTwoPin, 0);
}

void Motor::driveReverse() {
    gpio_put(directionOnePin, 0);
    gpio_put(directionTwoPin, 1);
}

void Motor::open() {
    if (directionInverted) {
        driveReverse();
    } else {
        driveForward();
    }
}

void Motor::close() {
    if (directionInverted) {
        driveForward();
    } else {
        driveReverse();
    }
}

void Motor::stop() {
    gpio_put(directionOnePin, 0);
    gpio_put(directionTwoPin, 0);
}