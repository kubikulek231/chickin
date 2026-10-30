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
#include <string.h>

// === Endstops ===
#define ENDSTOP_TOP_GPIO       14
#define ENDSTOP_BOTTOM_GPIO    26

// === Button ===
#define DOOR_BUTTON_GPIO       6

// === Motor ===
#define MOTOR_DIR1_GPIO        7
#define MOTOR_DIR2_GPIO        8

// UART defines
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART1_TX_GPIO          4
#define UART1_RX_GPIO          5
#define UART_COMMAND_BUFFER_SIZE 32

constexpr char UART_COMMAND_GET_STATUS[] = "GET_STATUS";
constexpr char UART_COMMAND_OPEN[] = "OPEN";
constexpr char UART_COMMAND_CLOSE[] = "CLOSE";
constexpr char UART_RESPONSE_LINE_ENDING[] = "\r\n";
constexpr char UART_RESPONSE_ACK[] = "ACK";
constexpr char UART_RESPONSE_GET_STATUS[] = "ACK %s";
constexpr char UART_RESPONSE_OPEN_ERROR[] = "ERR OPEN STATE";
constexpr char UART_RESPONSE_CLOSE_ERROR[] = "ERR CLOSE STATE";
constexpr char UART_RESPONSE_UNKNOWN_COMMAND[] = "ERR UNKNOWN_COMMAND";
constexpr char UART_RESPONSE_COMMAND_TOO_LONG[] = "ERR COMMAND_TOO_LONG";

// Status LED defines
#define WS2812_PIO pio0
#define WS2812_SM  0
#define WS2812_PIN 16
#define WS2812_IS_RGBW false

static void sendUartResponse(const char *response)
{
    uart_puts(UART_ID, response);
    uart_puts(UART_ID, UART_RESPONSE_LINE_ENDING);
}

static void processUartCommand(const char *command, DoorStateMachine &door)
{
    char response[64];

    if (strcmp(command, UART_COMMAND_GET_STATUS) == 0)
    {
        snprintf(response, sizeof(response), UART_RESPONSE_GET_STATUS, door.stateName());
        sendUartResponse(response);
    }
    else if (strcmp(command, UART_COMMAND_OPEN) == 0)
    {
        if (door.requestOpen())
        {
            sendUartResponse(UART_RESPONSE_ACK);
        }
        else
        {
            sendUartResponse(UART_RESPONSE_OPEN_ERROR);
        }
    }
    else if (strcmp(command, UART_COMMAND_CLOSE) == 0)
    {
        if (door.requestClose())
        {
            sendUartResponse(UART_RESPONSE_ACK);
        }
        else
        {
            sendUartResponse(UART_RESPONSE_CLOSE_ERROR);
        }
    }
    else
    {
        sendUartResponse(UART_RESPONSE_UNKNOWN_COMMAND);
    }
}

static void updateUart(DoorStateMachine &door, char *command, size_t &commandLength, bool &commandTooLong)
{
    while (uart_is_readable(UART_ID))
    {
        char incoming = uart_getc(UART_ID);

        if (incoming == '\r')
        {
            continue;
        }

        if (incoming == '\n')
        {
            if (commandTooLong)
            {
                sendUartResponse(UART_RESPONSE_COMMAND_TOO_LONG);
            }
            else if (commandLength > 0)
            {
                command[commandLength] = '\0';
                processUartCommand(command, door);
            }

            commandLength = 0;
            commandTooLong = false;
        }
        else if (!commandTooLong)
        {
            if (commandLength < UART_COMMAND_BUFFER_SIZE - 1)
            {
                command[commandLength++] = incoming;
            }
            else
            {
                commandTooLong = true;
            }
        }
    }
}

int main()
{
    stdio_init_all();

        absolute_time_t usbWaitStart = get_absolute_time();
    while (!stdio_usb_connected() &&
            absolute_time_diff_us(usbWaitStart, get_absolute_time()) < 5000000) {
        sleep_ms(50);
    }

    if (stdio_usb_connected()) {
        sleep_ms(200);
        printf("USB serial connected.\n");
    } else {
        printf("USB serial not connected after 5s, continuing anyway.\n");
    }

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART1_TX_GPIO, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_GPIO, GPIO_FUNC_UART);

    Button openButton(DOOR_BUTTON_GPIO);
    openButton.init();

    Motor doorMotor(MOTOR_DIR1_GPIO, MOTOR_DIR2_GPIO);
    doorMotor.init();

    Endstop topEndstop(ENDSTOP_TOP_GPIO, true);
    topEndstop.init();

    Endstop bottomEndstop(ENDSTOP_BOTTOM_GPIO, true);
    bottomEndstop.init();

    StatusLed statusLed;
    statusLed.init(WS2812_PIO, WS2812_SM, WS2812_PIN, WS2812_IS_RGBW);

    DoorStateMachine door(openButton, doorMotor, topEndstop, bottomEndstop, statusLed, 15000, 10000);
    door.init();

    char uartCommand[UART_COMMAND_BUFFER_SIZE];
    size_t uartCommandLength = 0;
    bool uartCommandTooLong = false;

    printf("Entering the main loop...\n");

    while (true) {
        door.update();
        updateUart(door, uartCommand, uartCommandLength, uartCommandTooLong);

        // doorMotor.open();
        sleep_ms(5);
    }
}