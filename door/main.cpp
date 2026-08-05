#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

#include "src/button.h"
#include "src/motor.h"
#include "src/endstop.h"
#include "src/statusled.h"
#include "doorstatemachine.h"
#include "pico/stdlib.h"

// === Endstops ===
#define ENDSTOP_TOP_GPIO       14
#define ENDSTOP_MIDDLE_GPIO    15
#define ENDSTOP_BOTTOM_GPIO    26

// === Button ===
#define DOOR_BUTTON_GPIO       6

// === Motor ===
#define MOTOR_DIR1_GPIO        7
#define MOTOR_DIR2_GPIO        8

// UART defines
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART0_TX_GPIO          0
#define UART0_RX_GPIO          1

// Status LED defines
#define WS2812_PIO pio0
#define WS2812_SM  0
#define WS2812_PIN 16
#define WS2812_IS_RGBW false

int main()
{
    stdio_init_all();

    absolute_time_t usb_wait_start = get_absolute_time();
    while (!stdio_usb_connected() &&
           absolute_time_diff_us(usb_wait_start, get_absolute_time()) < 5000000) {
        sleep_ms(50);
    }

    if (stdio_usb_connected()) {
        sleep_ms(200);
        printf("USB serial connected.\n");
    } else {
        printf("USB serial not connected after 5s, continuing anyway.\n");
    }

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART0_TX_GPIO, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_GPIO, GPIO_FUNC_UART);

    Button openButton(DOOR_BUTTON_GPIO);
    openButton.init();

    Motor doorMotor(MOTOR_DIR1_GPIO, MOTOR_DIR2_GPIO);
    doorMotor.init();

    Endstop topEndstop(ENDSTOP_TOP_GPIO, true);
    topEndstop.init();

    Endstop middleEndstop(ENDSTOP_MIDDLE_GPIO, true);
    middleEndstop.init();

    Endstop bottomEndstop(ENDSTOP_BOTTOM_GPIO, true);
    bottomEndstop.init();

    DoorStateMachine door(openButton, doorMotor, topEndstop, middleEndstop, bottomEndstop, 30000);
    door.init();

    StatusLed::init(WS2812_PIO, WS2812_SM, WS2812_PIN, WS2812_IS_RGBW);

    printf("Entering the main loop...\n");

    while (true) {
        door.update();
        // doorMotor.open();
        sleep_ms(5);
    }
}