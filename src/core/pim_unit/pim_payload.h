//
// Created by wyk on 2024/7/20.
//

#pragma once

#include <vector>

namespace pimsim {

struct MacroPayload {
    int ins_pc{-1}, sub_ins_num{-1};

    int row{0};
    int input_bit_width{0};
    int activation_element_col_num{0};

    std::vector<unsigned long long> inputs{};
};

struct MacroSubInsInfo {
    int ins_pc{-1}, sub_ins_num{-1};
    int compartment_num{0}, element_col_num{0};
};

struct MacroBatchInfo {
    int batch_num{0};
    bool first_batch{false};
    bool last_batch{false};
};

struct MacroSubmodulePayload {
    MacroSubInsInfo sub_ins_info;
    MacroBatchInfo batch_info;
};

}
