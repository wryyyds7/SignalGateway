#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <chrono>
using SignalValue = std::variant<double, int, bool>;
struct Signal {
    std::string name;
    uint32_t messageId = 0;
    SignalValue value;
    int64_t timestamp_ns = 0;
    double asDouble() const { return std::get<double>(value); }
    int asInt() const { return std::get<int>(value); }
    bool asBool() const { return std::get<bool>(value); }
    static int64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
