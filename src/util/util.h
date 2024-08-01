//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <algorithm>
#include <unordered_set>
#include <vector>

#include "config/config.h"

namespace pimsim {

static constexpr double delta = 1e-8;

inline int IntDivCeil(int a, int b) {
    return (a - 1) / b + 1;
}

inline bool DoubleEqual(double a, double b) {
    return std::abs(a - b) < delta;
}

inline auto getMaskBit(const std::vector<unsigned char>& mask_byte_data, int index) {
    if (index / BYTE_TO_BIT >= mask_byte_data.size()) {
        return 0;
    }
    return (mask_byte_data[index / BYTE_TO_BIT] & (1 << (index % BYTE_TO_BIT)));
}

template <class V>
inline bool SetsIntersection(const std::unordered_set<V>& s1, const std::unordered_set<V>& s2) {
    return std::any_of(s1.begin(), s1.end(), [&](V ele) { return s2.find(ele) != s2.end(); });
}

int BytesToInt(const std::vector<unsigned char>& bytes, bool little_endian);

std::vector<unsigned char> IntToBytes(int value, bool little_endian);

}  // namespace pimsim
