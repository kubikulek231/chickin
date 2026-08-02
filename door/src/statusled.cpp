#include "statusled.h"
#include <lib/w2812.pio.h>

PIO StatusLed::pio_ = pio0;
uint StatusLed::sm_ = 0;

void StatusLed::init(PIO pio, uint sm, uint pin, bool is_rgbw)
{
    pio_ = pio;
    sm_ = sm;

    uint offset = pio_add_program(pio_, &ws2812_program);
    ws2812_program_init(pio_, sm_, offset, pin, 800000, is_rgbw);
    set(Color::OFF);
}

void StatusLed::set(Color color)
{
    switch (color)
    {
    case Color::GREEN:
        put_pixel(pio_, sm_, urgb_u32(0, 20, 0));
        break;
    case Color::RED:
        put_pixel(pio_, sm_, urgb_u32(20, 0, 0));
        break;
    case Color::BLUE:
        put_pixel(pio_, sm_, urgb_u32(0, 0, 20));
        break;
    case Color::OFF:
    default:
        put_pixel(pio_, sm_, urgb_u32(0, 0, 0));
        break;
    }
}