#pragma once

#include <cstdint>

// Both peers must compile with the exact same struct layout.
// Keep members trivially copyable and fixed-width.

struct JoyCmd
{
    double lx = 0.0;
    double ly = 0.0;
    double rx = 0.0;
    double ry = 0.0;
    uint8_t buttons = 0;
};

struct RobotFb
{
    uint8_t state = 0;
    double battery = 0.0;
};
