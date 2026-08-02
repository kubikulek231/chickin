#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

class StatusLed
{
public:
    enum class Color
    {
        OFF,
        GREEN,
        RED,
        BLUE
    };

    static void init(PIO pio, uint sm, uint pin, bool is_rgbw = false);
    static void set(Color color);

private:
    static PIO pio_;
    static uint sm_;

    static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
    {
        return ((uint32_t)(g) << 16) |
               ((uint32_t)(r) << 8) |
               (uint32_t)(b);
    }

    static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb)
    {
        pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
    }
};