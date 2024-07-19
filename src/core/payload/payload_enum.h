//
// Created by wyk on 2024/7/4.
//

#pragma once
#include "better-enums/enum.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace pimsim {

BETTER_ENUM(MemoryAccessType, int, read = 1, write = 2)  // NOLINT(*-explicit-constructor, *-no-recursion)

BETTER_ENUM(ScalarOperator, int,  // NOLINT(*-explicit-constructor)
            add, sub, mul, div, sll, srl, sra, mod, lui, load, store, assign)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(ScalarOperator)

}  // namespace pimsim
