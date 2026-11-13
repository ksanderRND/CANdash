#include "DbcParser.hpp"
#include "CANFrame.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

// Problems reporter, will be switched to a logger later.
void reportError(int lineNum, const std::string& text) {
    std::cerr << "DBC line " << lineNum << ": " << text << "\n";
}

// Read "<name>:" or "<name> :" and strip the colon. Return false if the colon is missing.
bool readNameWithColon(std::istream& iss, std::string& name) {
    if (!(iss >> name))
        return false;
    if (name.back() == ':') {          // a successful stream never gives an empty string
        name.pop_back();
        return !name.empty();          // alone ":" is not a name
    }
    char colon = 0;
    return (iss >> colon) && colon == ':'; // missed colon is also an error
}

} // namespace

std::vector<CANMessage> DbcParser::parseFile(const std::string& filePath) const {
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("cannot open DBC file: " + filePath);
    return parse(file);
}

std::vector<CANMessage> DbcParser::parse(std::istream& input) const {
    std::vector<CANMessage> messages;
    std::string line;
    int lineNum = 0;
    int errorCount = 0;
    bool seenMessageLine = false;  // any BO_ line so far, valid or not
    bool currentValid = false;     // the last BO_ line was parsed successfully

    while (std::getline(input, line)) {
        ++lineNum;

        // Trim spaces, tabs and the '\r' left by Windows line endings.
        const auto start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos)
            continue;
        const auto end = line.find_last_not_of(" \t\r");
        line = line.substr(start, end - start + 1);

        if (line.rfind("BO_ ", 0) == 0) {
            seenMessageLine = true;
            auto message = parseMessageLine(line, lineNum);
            currentValid = message.has_value();
            if (message)
                messages.push_back(std::move(*message));
            else
                ++errorCount;
        }
        else if (line.rfind("SG_ ", 0) == 0) {
            if (!currentValid) {
                if (!seenMessageLine) {
                    reportError(lineNum, "SG_ before the first BO_");
                    ++errorCount;
                }
                continue; // malformed BO_ is already reported.
            }
            if (auto signal = parseSignalLine(line, lineNum))
                messages.back().signals.push_back(std::move(*signal));
            else
                ++errorCount;
        }
    }

    if (errorCount > 0)
        throw std::runtime_error("DBC has " + std::to_string(errorCount) + " error(s), see above");
    return messages;
}

std::optional<CANMessage> DbcParser::parseMessageLine(const std::string& line, int lineNum) const {
    // BO_ <id> <name>: <dlc> <sender>
    std::istringstream iss(line);
    std::string keyword, name;
    int64_t id = -1;   // signed and wide: "-5" and 29-bit IDs get a clear error below
    int64_t byteLength = -1;

    iss >> keyword >> id;
    if (!readNameWithColon(iss, name) || !(iss >> byteLength)) {
        reportError(lineNum, "malformed BO_ line: " + line);
        return std::nullopt;
    }
    if (id < 0 || id > 0x7FF) {
        reportError(lineNum, "message ID " + std::to_string(id) + " is not a standard 11-bit ID");
        return std::nullopt;
    }
    if (byteLength < 0 || byteLength > kMaxFrameBytes) {
        reportError(lineNum, "DLC " + std::to_string(byteLength) + " is out of range 0.."
                    + std::to_string(kMaxFrameBytes) + " (CAN FD is not supported)");
        return std::nullopt;
    }

    CANMessage message;
    message.id = static_cast<uint32_t>(id);
    message.name = std::move(name);
    message.byteLength = static_cast<uint8_t>(byteLength);
    return message;
}

std::optional<CANSignal> DbcParser::parseSignalLine(const std::string& line, int lineNum) const {
    // SG_ <name> : <start>|<length>@<order><sign> (<factor>,<offset>) [<min>|<max>] "<unit>" <receivers>
    std::istringstream iss(line);
    std::string keyword, name;
    iss >> keyword;
    if (!readNameWithColon(iss, name)) {
        reportError(lineNum, "malformed SG_ line, expected '<name> :': " + line);
        return std::nullopt;
    }

    // Bit layout: "16|8@1+"
    int64_t startBit = -1, bitLength = -1;  // wide: range-checked before narrowing to uint8_t
    char bar = 0, at = 0, order = 0, sign = 0;
    iss >> startBit >> bar >> bitLength >> at >> order >> sign;
    if (!iss || bar != '|' || at != '@') {
        reportError(lineNum, "signal " + name + ": expected '<start>|<length>@<order><sign>'");
        return std::nullopt;
    }
    if (order == '0') {
        reportError(lineNum, "signal " + name + ": Motorola byte order (@0) is not supported yet");
        return std::nullopt;
    }
    if (order != '1' || (sign != '+' && sign != '-')) {
        reportError(lineNum, "signal " + name + ": bad byte order or sign after '@'");
        return std::nullopt;
    }
    constexpr int kMaxBits = kMaxFrameBytes * 8;
    if (startBit < 0 || startBit >= kMaxBits || bitLength < 1 || bitLength > kMaxBits) {
        reportError(lineNum, "signal " + name + ": start bit must be 0.." + std::to_string(kMaxBits - 1)
                    + " and length 1.." + std::to_string(kMaxBits));
        return std::nullopt;
    }

    // Scaling: "(1,-40)"
    double factor = 0.0, offset = 0.0;
    char lparen = 0, comma = 0, rparen = 0;
    iss >> lparen >> factor >> comma >> offset >> rparen;
    if (!iss || lparen != '(' || comma != ',' || rparen != ')') {
        reportError(lineNum, "signal " + name + ": expected '(<factor>,<offset>)'");
        return std::nullopt;
    }
    if (factor == 0.0) {
        reportError(lineNum, "signal " + name + ": factor must not be 0");
        return std::nullopt;
    }

    // Range: "[-40|150]"
    double min = 0.0, max = 0.0;
    char lbracket = 0, rangeBar = 0, rbracket = 0;
    iss >> lbracket >> min >> rangeBar >> max >> rbracket;
    if (!iss || lbracket != '[' || rangeBar != '|' || rbracket != ']') {
        reportError(lineNum, "signal " + name + ": expected '[<min>|<max>]'");
        return std::nullopt;
    }

    // Unit: "degC", may contain spaces: "km h"
    char quote = 0;
    std::string unit;
    iss >> quote;
    std::getline(iss, unit, '"');      // reads up to the closing quote and drops it
    if (!iss || quote != '"' || iss.eof()) {  // eof: the closing quote was never found
        reportError(lineNum, "signal " + name + ": expected a quoted unit");
        return std::nullopt;
    }

    CANSignal signal;
    signal.name = std::move(name);
    signal.startBit = static_cast<uint8_t>(startBit);
    signal.bitLength = static_cast<uint8_t>(bitLength);
    signal.byteOrder = ByteOrder::Intel;
    signal.isSigned = (sign == '-');
    signal.factor = factor;
    signal.offset = offset;
    signal.min = min;
    signal.max = max;
    signal.unit = std::move(unit);
    return signal;
}
