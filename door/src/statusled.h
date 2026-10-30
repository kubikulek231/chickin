#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

class StatusLed
{
public:
    enum class Color
    {
        Off,
        Green,
        Red,
        Blue,
        Pink
    };

    void init(PIO pio, uint sm, uint pin, bool isRgbw = false);
    void set(Color color);

private:
    PIO pioInstance = pio0;
    uint stateMachine = 0;

    static inline uint32_t urgbToUint32(uint8_t r, uint8_t g, uint8_t b)
    {
        return ((uint32_t)(g) << 16) |
               ((uint32_t)(r) << 8) |
               (uint32_t)(b);
    }

    static inline void putPixel(PIO pio, uint sm, uint32_t pixelGrb)
    {
        pio_sm_put_blocking(pio, sm, pixelGrb << 8u);
    }
};