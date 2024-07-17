//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <algorithm>
#include <unordered_set>

namespace pimsim {

static constexpr double delta = 1e-8;

inline int IntDivCeil(int a, int b) {
    return (a - 1) / b + 1;
}

inline bool DoubleEqual(double a, double b) {
    return std::abs(a - b) < delta;
}

template <class V>
inline bool SetsIntersection(const std::unordered_set<V>& s1, const std::unordered_set<V>& s2) {
    return std::any_of(s1.begin(), s1.end(), [&](V ele) { return s2.find(ele) != s2.end(); });
}

}  // namespace pimsim
