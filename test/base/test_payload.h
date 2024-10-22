//
// Created by wyk on 2024/8/2.
//

#pragma once
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace pimsim {

struct TestExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};
};

DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(TestExpectedInfo)

}  // namespace pimsim
