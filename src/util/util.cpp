//
// Created by wyk on 2024/7/4.
//

#include "util.h"

namespace pimsim {

int BytesToInt(const std::vector<unsigned char>& bytes, bool little_endian) {
    unsigned int result = 0;
    if (little_endian) {
        for (int i = 3; i >= 0; i--) {
            result = (result << 8) + bytes[i];
        }
    } else {
        for (int i = 0; i <= 3; i++) {
            result = (result << 8) + bytes[i];
        }
    }
    return static_cast<int>(result);
}

std::vector<unsigned char> IntToBytes(int value, bool little_endian) {
    std::vector<unsigned char> bytes{0, 0, 0, 0};
    auto un_value = static_cast<unsigned int>(value);
    if (little_endian) {
        for (int i = 0; i <= 3; i++) {
            bytes[i] = (un_value & 0xff);
            un_value >>= 8;
        }
    } else {
        for (int i = 3; i >= 0; i--) {
            bytes[i] = (un_value & 0xff);
            un_value >>= 8;
        }
    }
    return std::move(bytes);
}

}  // namespace pimsim
