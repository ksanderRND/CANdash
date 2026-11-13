#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ByteOrder { Intel, Motorola };  // DBC "@1" / "@0"

struct CANSignal {
    std::string name;
    uint8_t startBit = 0;
    uint8_t bitLength = 0;
    ByteOrder byteOrder = ByteOrder::Intel;
    bool isSigned = false;                  // DBC "-" / "+"
    double factor = 1.0;                    // physical = raw * factor + offset
    double offset = 0.0;
    double min = 0.0;
    double max = 0.0;
    std::string unit;
};

struct CANMessage {
    uint32_t id = 0;
    std::string name;
    uint8_t byteLength = 0;                 // frame length in bytes (the DBC "DLC")
    std::vector<CANSignal> signals;
};
