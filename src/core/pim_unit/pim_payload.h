//
// Created by wyk on 2024/7/20.
//

#pragma once

#include <vector>

#include "better-enums/enum.h"

namespace pimsim {

struct PimInsInfo {
    int ins_pc{-1}, sub_ins_num{-1};
    bool last_ins{false}, last_sub_ins{false};
};

struct MacroPayload {
    PimInsInfo pim_ins_info{};

    int row{0};
    int input_bit_width{0};
    int activation_element_col_num{0};

    std::vector<unsigned long long> inputs{};
};

struct MacroSubInsInfo {
    PimInsInfo pim_ins_info{};
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

struct MacroGroupSubInsInfo {
    // ins info and sub ins info
    PimInsInfo pim_ins_info{};

    // group info
    bool last_group{false};

    // result adder info
    int activation_element_col_num{0};
};

struct MacroGroupPayload {
    MacroGroupSubInsInfo sub_ins_info{};

    // macro compute info
    int row{0};
    int input_bit_width{0};

    // inputs
    std::vector<std::vector<unsigned long long>> macro_inputs{};
};

struct MacroGroupControllerPayload {
    MacroGroupSubInsInfo sub_ins_info{};

    int input_bit_width{0};
};

struct MacroGroupSubmodulePayload {
    MacroGroupSubInsInfo sub_ins_info{};
    MacroBatchInfo batch_info{};
};

}  // namespace pimsim