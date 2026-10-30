#include "statusled.h"
#include <lib/w2812.pio.h>

void StatusLed::init(PIO pio, uint sm, uint pin, bool isRgbw)
{
    pioInstance = pio;
    stateMachine = sm;

    uint offset = pio_add_program(pioInstance, &ws2812_program);
    ws2812_program_init(pioInstance, stateMachine, offset, pin, 800000, isRgbw);
    set(Color::Off);
}

void StatusLed::set(Color color)
{
    switch (color)
    {
    case Color::Green:
        putPixel(pioInstance, stateMachine, urgbToUint32(0, 20, 0));
        break;
    case Color::Red:
        putPixel(pioInstance, stateMachine, urgbToUint32(20, 0, 0));
        break;
    case Color::Blue:
        putPixel(pioInstance, stateMachine, urgbToUint32(0, 0, 20));
        break;
    case Color::Pink:
        putPixel(pioInstance, stateMachine, urgbToUint32(20, 0, 20));
        break;
    case Color::Off:
    default:
        putPixel(pioInstance, stateMachine, urgbToUint32(0, 0, 0));
        break;
    }
}