#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
struct CanFrame {
    uint32_t id = 0; uint8_t dlc = 0; std::vector<uint8_t> data; int64_t timestamp_ns = 0;
    CanFrame() = default;
    CanFrame(uint32_t id_, std::vector<uint8_t> data_) : id(id_), dlc(static_cast<uint8_t>(data_.size())), data(std::move(data_)) {}
    std::string toHex() const { std::string s; s.reserve(data.size()*3); char b[4]; for (auto x: data) { snprintf(b,4,"%02X ",x); s+=b; } return s; }
};
