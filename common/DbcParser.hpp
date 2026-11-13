#pragma once

#include <istream>
#include <optional>
#include <string>
#include <vector>
#include "CANMessage.hpp"

class DbcParser {
public:
    // Print every problem to std::cerr, then throw std::runtime_error if there were any.
    std::vector<CANMessage> parseFile(const std::string& filePath) const;
    std::vector<CANMessage> parse(std::istream& input) const;

private:
    // Return std::nullopt and print to std::cerr if the line is malformed.
    std::optional<CANMessage> parseMessageLine(const std::string& line, int lineNum) const;
    std::optional<CANSignal> parseSignalLine(const std::string& line, int lineNum) const;
};
