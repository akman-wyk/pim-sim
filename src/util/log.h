//
// Created by wyk on 2024/7/9.
//

#pragma once
#include <string>

#include "systemc.h"

namespace pimsim {

inline void log(const std::string& msg) {
    std::cout << sc_core::sc_time_stamp() << ", " << msg << std::endl;
}

}  // namespace pimsim

#define LOG(msg)
