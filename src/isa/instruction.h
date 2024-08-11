//
// Created by wyk on 2024/8/8.
//

#pragma once

#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace pimsim {

struct Instruction {
    // instruction class, type, opcode
    int class_code{0};
    int type{0};
    int opcode{0};

    // general
    int rs1{0}, rs2{0}, rs3{0}, rs4{0}, rd{0};
    int imm{0}, offset{0};

    // pim compute
    int value_sparse{0}, bit_sparse{0}, group{0}, group_input_mode{0};

    // pim set
    int group_broadcast{0};

    // pim output
    int outsum_move{0}, outsum{0};

    // SIMD
    int input_num{0};

    // transfer
    int offset_mask{0};
};

DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(Instruction)

}  // namespace pimsim
