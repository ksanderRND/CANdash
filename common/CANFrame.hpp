#pragma once

#include <cstdint>

constexpr uint8_t kMaxFrameBytes = 8;  // classic CAN payload limit

struct CANFrame {
    uint32_t id;
    uint8_t len;
    uint8_t data[kMaxFrameBytes];
};
