//
// Created by wyk on 2024/7/9.
//

#pragma once
#include <string>

#include "systemc.h"

namespace pimsim {

class Core;

void log(const std::string& msg, Core* core);

}  // namespace pimsim

#define LOG(msg)
