#pragma once

#include <cstdint>

struct CANFrame {
    uint32_t id;
    uint8_t len;
    uint8_t data[8];  
};