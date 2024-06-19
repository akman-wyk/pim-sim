//
// Created by wyk on 2024/6/14.
//

#include "config.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

#include "fmt/core.h"

namespace pimsim {

template <class T>
bool check_positive(const T& t) {
    return t > 0;
}

template <class T>
bool check_positive(const std::vector<T>& list) {
    return std::all_of(list.begin(), list.end(), [](const T& t) { return check_positive(t); });
}

template <class T, class... Args>
bool check_positive(const T& t, const Args&... args) {
    return check_positive(t) && check_positive(args...);
}

template <class T>
[[maybe_unused]] bool check_not_negative(const T& t) {
    return t >= 0;
}

template <class T, class... Args>
bool check_not_negative(const T& t, const Args&... args) {
    return check_not_negative(t) && check_not_negative(args...);
}

template <class T, class... Args>
bool check_vector_valid(const std::vector<T>& configs, [[maybe_unused]] const Args&... args) {
    return std::all_of(configs.begin(), configs.end(), [&](const T& config) { return config.checkValid(args...); });
}

// ControlUnit
bool ControlUnitConfig::checkValid() const {
    if (!check_not_negative(controller_static_power_mW, controller_dynamic_power_mW, fetch_static_power_mW,
                            fetch_dynamic_power_mW, decode_static_power_mW, decode_dynamic_power_mW)) {
        std::cerr << "ControlUnitConfig not valid, 'controller_static_power_mW, controller_dynamic_power_mW, "
                     "fetch_static_power_mW, fetch_dynamic_power_mW, decode_static_power_mW, decode_dynamic_power_mW' "
                     "must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ControlUnitConfig, controller_static_power_mW,
                                                controller_dynamic_power_mW, fetch_static_power_mW,
                                                fetch_dynamic_power_mW, decode_static_power_mW, decode_dynamic_power_mW)

// RegisterUnit
bool SpecialRegisterBindingConfig::checkValid() const {
    if (!(0 <= special && special < SPECIAL_REG_NUM)) {
        std::cerr
            << fmt::format(
                   "SpecialRegisterBindingConfig not valid, 'special' must be in [0, {}), while actually it is {}",
                   SPECIAL_REG_NUM, special)
            << std::endl;
        return false;
    }
    if (!(0 <= general && general < GENERAL_REG_NUM)) {
        std::cerr
            << fmt::format(
                   "SpecialRegisterBindingConfig not valid, 'general' must be in [0, {}), while actually it is {}",
                   GENERAL_REG_NUM, general)
            << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpecialRegisterBindingConfig, special, general)

bool RegisterUnitConfig::checkValid() const {
    if (!check_not_negative(static_power_mW, dynamic_power_mW)) {
        std::cerr << "RegisterUnitConfig not valid, 'static_power_mW, dynamic_power_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    if (!check_vector_valid(special_register_binding)) {
        std::cerr << "RegisterUnitConfig not valid" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RegisterUnitConfig, static_power_mW, dynamic_power_mW,
                                                special_register_binding)

// ScalarUnit
bool ScalarFunctorConfig::checkValid() const {
    if (inst_name.empty()) {
        std::cerr << "ScalarFunctorConfig not valid, 'inst_name' must be non-empty" << std::endl;
        return false;
    }
    if (!check_not_negative(static_power_mW, dynamic_power_mW)) {
        std::cerr
            << fmt::format(
                   "ScalarFunctorConfig of '{}' not valid, 'static_power_mW, dynamic_power_mW' must be non-negative",
                   inst_name)
            << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScalarFunctorConfig, inst_name, static_power_mW, dynamic_power_mW)

bool ScalarUnitConfig::checkValid() const {
    if (const bool valid = check_not_negative(default_functor_static_power_mW, default_functor_dynamic_power_mW) &&
                           check_vector_valid(functor_list);
        !valid) {
        std::cerr << "ScalarUnitConfig not valid" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScalarUnitConfig, default_functor_static_power_mW,
                                                default_functor_dynamic_power_mW, functor_list)

// SIMDUnit
bool SIMDDataWidthConfig::checkDataWidth(const int width) {
    return width == 1 || width == 2 || width == 4 || width == 8 || width == 16 || width == 32 || width == 64;
}

bool SIMDDataWidthConfig::inputBitWidthMatch(const pimsim::SIMDDataWidthConfig& other) const {
    return input1 == other.input1 && input2 == other.input2 && input3 == other.input3 && input4 == other.input4;
}

bool SIMDDataWidthConfig::checkValid(const unsigned int input_cnt, bool check_output) const {
    bool valid = checkDataWidth(input1);
    if (input_cnt >= 2) {
        valid = valid && checkDataWidth(input2);
    }
    if (input_cnt >= 3) {
        valid = valid && checkDataWidth(input3);
    }
    if (input_cnt == 4) {
        valid = valid && checkDataWidth(input4);
    }
    if (check_output) {
        valid = valid && checkDataWidth(output);
    }
    if (!valid) {
        std::cerr << "SIMDDataWidthConfig not valid, data width must be one of the following values: "
                     "ont-bit(1b), two-bit(2b), half-byte(4b), byte(8b), half-word(16b), word(32b), double-word(64b)"
                  << std::endl;
        return false;
    }
    return true;
}

inline void to_json(nlohmann::json& j, const SIMDDataWidthConfig& t) {
    if (t.input1 != 0) {
        j["input1"] = t.input1;
    }
    if (t.input2 != 0) {
        j["input2"] = t.input2;
    }
    if (t.input3 != 0) {
        j["input3"] = t.input3;
    }
    if (t.input4 != 0) {
        j["input4"] = t.input4;
    }
    if (t.output != 0) {
        j["output"] = t.output;
    }
}

inline void from_json(const nlohmann::json& j, SIMDDataWidthConfig& t) {
    constexpr SIMDDataWidthConfig default_obj{};
    t.input1 = j.value("input1", default_obj.input1);
    t.input2 = j.value("input2", default_obj.input2);
    t.input3 = j.value("input3", default_obj.input3);
    t.input4 = j.value("input4", default_obj.input4);
    t.output = j.value("output", default_obj.output);
}

bool SIMDFunctorConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "SIMDFunctorConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (input_cnt > SIMD_MAX_INPUT_NUM) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid, 'input_cnt' must be not greater than {}", name,
                                 SIMD_MAX_INPUT_NUM)
                  << std::endl;
        return false;
    }
    if (!check_positive(functor_cnt)) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid, 'functor_cnt' must be positive", name)
                  << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_per_functor_mW, dynamic_power_per_functor_mW)) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid, 'latency_cycle, static_power_per_functor_mW, "
                                 "dynamic_power_per_functor_mW' must be non-negative",
                                 name)
                  << std::endl;
        return false;
    }
    if (!data_bit_width.checkValid(input_cnt, true)) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid", name) << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SIMDFunctorConfig, name, data_bit_width, functor_cnt, latency_cycle,
                                                static_power_per_functor_mW, dynamic_power_per_functor_mW)

bool SIMDInstructionFunctorBindingConfig::checkValid(const unsigned int input_cnt) const {
    if (functor_name.empty()) {
        std::cerr << "SIMDInstructionFunctorBindingConfig not valid, 'functor_name' must be non-empty" << std::endl;
        return false;
    }
    if (!input_bit_width.checkValid(input_cnt, false)) {
        std::cerr << fmt::format("SIMDInstructionFunctorBindingConfig with functor '{}' not valid", functor_name)
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SIMDInstructionFunctorBindingConfig, input_bit_width, functor_name)

bool SIMDInstructionConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "SIMDInstructionConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (input_cnt > SIMD_MAX_INPUT_NUM) {
        std::cerr << fmt::format("SIMDInstructionConfig of '{}' not valid, 'input_cnt' must be not greater than {}",
                                 name, SIMD_MAX_INPUT_NUM)
                  << std::endl;
        return false;
    }
    if (opcode > SIMD_MAX_OPCODE) {
        std::cerr << fmt::format("SIMDInstructionConfig of '{}' not valid, 'opcode' must be not greater than {}", name,
                                 SIMD_MAX_OPCODE)
                  << std::endl;
        return false;
    }
    if (input1_type == +SIMDInputType::other || input2_type == +SIMDInputType::other ||
        input3_type == +SIMDInputType::other || input4_type == +SIMDInputType::other) {
        std::cerr << fmt::format("SIMDInstructionConfig of '{}' not valid, 'input_type' must be 'vector' or 'scalar'",
                                 name)
                  << std::endl;
        return false;
    }
    if (!check_vector_valid(functor_binding_list, input_cnt)) {
        std::cerr << "SIMDInstructionConfig not valid" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SIMDInstructionConfig, name, input_cnt, opcode, input1_type,
                                                input2_type, input3_type, input4_type, functor_binding_list)

bool SIMDUnitConfig::checkValid() const {
    if (const bool valid = check_vector_valid(functor_list) && check_vector_valid(instruction_list); !valid) {
        std::cerr << "SIMDUnitConfig not valid" << std::endl;
        return false;
    }

    // check instruction functor binding
    std::unordered_map<std::string, const SIMDFunctorConfig&> functor_map;
    std::transform(functor_list.begin(), functor_list.end(), std::inserter(functor_map, functor_map.end()),
                   [](const SIMDFunctorConfig& functor) { return std::make_pair(functor.name, functor); });
    for (const auto& instruction : instruction_list) {
        for (const auto& functor_binding : instruction.functor_binding_list) {
            auto functor_found = functor_map.find(functor_binding.functor_name);
            // check functor exist
            if (functor_found == functor_map.end()) {
                std::cerr << "SIMDUnitConfig not valid, instruction functor binding error" << std::endl;
                std::cerr << fmt::format("Functor '{}' not exist", functor_binding.functor_name) << std::endl;
                return false;
            }

            auto& functor = functor_found->second;

            // check input cnt match
            if (instruction.input_cnt != functor.input_cnt) {
                std::cerr << "SIMDUnitConfig not valid, instruction functor binding error" << std::endl;
                std::cerr << fmt::format("Input cnt not match between instruction '{}' functor '{}'", instruction.name,
                                         functor.name)
                          << std::endl;
                return false;
            }

            // check input bit width match
            if (!functor_binding.input_bit_width.inputBitWidthMatch(functor.data_bit_width)) {
                std::cerr << "SIMDUnitConfig not valid, instruction functor binding error" << std::endl;
                std::cerr << fmt::format("Input bit-width not match between instruction '{}' functor '{}'",
                                         instruction.name, functor.name)
                          << std::endl;
                return false;
            }
        }
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SIMDUnitConfig, functor_list, instruction_list)

// PimUnit
bool PimMacroSizeConfig::checkValid() const {
    if (!check_positive(compartment_cnt_per_macro, element_cnt_per_compartment, row_cnt_per_element,
                        bit_width_per_row)) {
        std::cerr << "PimMacroSizeConfig not valid, 'compartment_cnt_per_macro, element_cnt_per_compartment, "
                     "row_cnt_per_element, bit_width_per_row' must be positive"
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PimMacroSizeConfig, compartment_cnt_per_macro,
                                                element_cnt_per_compartment, row_cnt_per_element, bit_width_per_row)

bool PimModuleConfig::checkValid(const std::string& module_name) const {
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW)) {
        std::cerr << fmt::format("PimModuleConfig of '{}' not valid, 'latency_cycle, static_power_mW, "
                                 "dynamic_power_mW' must be non-negative",
                                 module_name)
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PimModuleConfig, latency_cycle, static_power_mW, dynamic_power_mW)

bool PimSRAMConfig::checkValid() const {
    if (as_mode == +PimSRAMAddressSpaceContinuousMode::other) {
        std::cerr << "PimSRAMConfig not valid, 'as_mode' must be 'intergroup' or 'intragroup'" << std::endl;
        return false;
    }
    if (!check_not_negative(write_latency_cycle, read_latency_cycle, static_power_mW, write_dynamic_power_per_bit_mW,
                            read_dynamic_power_per_bit_mW)) {
        std::cerr << "PimSRAMConfig not valid, 'write_latency_cycle, read_latency_cycle, static_power_mW, "
                     "write_dynamic_power_per_bit_mW, read_dynamic_power_per_bit_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PimSRAMConfig, write_latency_cycle, read_latency_cycle, static_power_mW,
                                                write_dynamic_power_per_bit_mW, read_dynamic_power_per_bit_mW)

bool PimValueSparseConfig::checkValid() const {
    if (!check_positive(mask_bit_width, output_macro_cnt)) {
        std::cerr << "PimValueSparseConfig not valid, 'mask_bit_width, output_macro_cnt' must be positive" << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW)) {
        std::cerr
            << "PimValueSparseConfig not valid, 'latency_cycle, static_power_mW, dynamic_power_mW' must be non-negative"
            << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PimValueSparseConfig, mask_bit_width, latency_cycle, static_power_mW,
                                                dynamic_power_mW, output_macro_cnt)

bool PimBitSparseConfig::checkValid() const {
    if (!check_positive(mask_bit_width)) {
        std::cerr << "PimBitSparseConfig not valid, 'mask_bit_width' must be positive" << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW)) {
        std::cerr
            << "PimBitSparseConfig not valid, 'latency_cycle, static_power_mW, dynamic_power_mW' must be non-negative"
            << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PimBitSparseConfig, mask_bit_width, latency_cycle, static_power_mW,
                                                dynamic_power_mW)

bool PimUnitConfig::checkValid() const {
    if (!check_positive(macro_total_cnt, macro_group_size_configurable_values)) {
        std::cerr << "PimUnitConfig not valid, 'macro_total_cnt, macro_group_size_configurable_values' must be positive"
                  << std::endl;
        return false;
    }
    if (const bool valid = macro_size.checkValid() && ipu.checkValid("ipu") && sram.checkValid() &&
                           adder_tree.checkValid("adder_tree") && shift_adder.checkValid("shift_adder") &&
                           result_adder.checkValid("result_adder") &&
                           (!value_sparse || value_sparse_config.checkValid()) &&
                           (!bit_sparse || bit_sparse_config.checkValid());
        !valid) {
        std::cerr << "PimUnitConfig not valid" << std::endl;
        return false;
    }
    return true;
}

inline void to_json(nlohmann::json& j, const PimUnitConfig& t) {
    j["macro_total_cnt"] = t.macro_total_cnt;
    j["macro_group_size_configurable_values"] = t.macro_group_size_configurable_values;
    j["macro_size"] = t.macro_size;
    j["ipu"] = t.ipu;
    j["sram"] = t.sram;
    j["adder_tree"] = t.adder_tree;
    j["shift_adder"] = t.shift_adder;
    j["result_adder"] = t.result_adder;
    j["value_sparse"] = t.value_sparse;
    if (t.value_sparse) {
        j["value_sparse_config"] = t.value_sparse_config;
    }
    j["bit_sparse"] = t.bit_sparse;
    if (t.bit_sparse) {
        j["bit_sparse_config"] = t.bit_sparse_config;
    }
}

inline void from_json(const nlohmann::json& j, PimUnitConfig& t) {
    const PimUnitConfig default_obj{};
    t.macro_total_cnt = j.value("macro_total_cnt", default_obj.macro_total_cnt);
    t.macro_group_size_configurable_values =
        j.value("macro_group_size_configurable_values", default_obj.macro_group_size_configurable_values);
    t.macro_size = j.value("macro_size", default_obj.macro_size);
    t.ipu = j.value("ipu", default_obj.ipu);
    t.sram = j.value("sram", default_obj.sram);
    t.adder_tree = j.value("adder_tree", default_obj.adder_tree);
    t.shift_adder = j.value("shift_adder", default_obj.shift_adder);
    t.result_adder = j.value("result_adder", default_obj.result_adder);
    t.value_sparse = j.value("value_sparse", default_obj.value_sparse);
    t.value_sparse_config = j.value("value_sparse_config", default_obj.value_sparse_config);
    t.bit_sparse = j.value("bit_sparse", default_obj.bit_sparse);
    t.bit_sparse_config = j.value("bit_sparse_config", default_obj.bit_sparse_config);
}

// LocalMemoryUnit
bool RAMConfig::checkValid() const {
    if (!check_positive(size, width)) {
        std::cerr << "RAMConfig not valid, 'size, width' must be positive" << std::endl;
        return false;
    }
    if (!check_not_negative(write_latency_cycle, read_latency_cycle, static_power_mW, write_dynamic_power_mW,
                            read_dynamic_power_mW)) {
        std::cerr << "RAMConfig not valid, 'write_latency_cycle, read_latency_cycle, static_power_mW, "
                     "write_dynamic_power_mW, read_dynamic_power_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RAMConfig, size, width, write_latency_cycle, read_latency_cycle,
                                                static_power_mW, write_dynamic_power_mW, read_dynamic_power_mW)

bool RegBufferConfig::checkValid() const {
    if (!check_positive(size, read_width, write_width, rw_unit)) {
        std::cerr << "RegBufferConfig not valid, 'size, read_width, write_width, rw_unit' must be positive"
                  << std::endl;
        return false;
    }
    if (!check_not_negative(static_power_mW, rw_dynamic_power_per_unit_mW)) {
        std::cerr << "RegBufferConfig not valid, 'static_power_mW, rw_dynamic_power_per_unit_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RegBufferConfig, size, read_width, write_width, rw_unit,
                                                static_power_mW, rw_dynamic_power_per_unit_mW)

int LocalMemoryAddressSpaceConfig::end() const {
    return offset + size;
}

bool LocalMemoryAddressSpaceConfig::checkValid() const {
    if (!check_not_negative(offset, size)) {
        std::cerr << "LocalMemoryAddressSpaceConfig not valid, 'offset, size' must be non-negative" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LocalMemoryAddressSpaceConfig, offset, size)

bool LocalMemoryConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "LocalMemoryConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (type == +LocalMemoryType::other) {
        std::cerr << fmt::format("LocalMemoryConfig of '{}' not valid, 'type' must be 'ram' or 'reg_buffer'", name)
                  << std::endl;
        return false;
    }
    if (const bool valid =
            addressing.checkValid() && ((type == +LocalMemoryType::ram && ram_config.checkValid()) ||
                                        (type == +LocalMemoryType::reg_buffer && reg_buffer_config.checkValid()));
        !valid) {
        std::cerr << fmt::format("LocalMemoryConfig of '{}' not valid", name) << std::endl;
        return false;
    }
    return true;
}

inline void to_json(nlohmann::json& j, const LocalMemoryConfig& config) {
    j["name"] = config.name;
    j["type"] = config.type;
    j["addressing"] = config.addressing;
    if (config.type == +LocalMemoryType::ram) {
        j["hardware_config"] = config.ram_config;
    } else if (config.type == +LocalMemoryType::reg_buffer) {
        j["hardware_config"] = config.reg_buffer_config;
    }
}

inline void from_json(const nlohmann::json& j, LocalMemoryConfig& config) {
    const LocalMemoryConfig default_obj{};
    config.name = j.value("name", default_obj.name);
    config.type = j.value("type", LocalMemoryType::other);
    config.addressing = j.value("addressing", default_obj.addressing);
    if (config.type == +LocalMemoryType::ram) {
        config.ram_config = j.value("hardware_config", default_obj.ram_config);
    } else if (config.type == +LocalMemoryType::reg_buffer) {
        config.reg_buffer_config = j.value("hardware_config", default_obj.reg_buffer_config);
    }
}

bool LocalMemoryUnitConfig::checkValid() const {
    if (!check_vector_valid(local_memory_list)) {
        std::cerr << "LocalMemoryUnitConfig not valid" << std::endl;
        return false;
    }

    // check if address space overlap
    LocalMemoryConfig pim_memory_config{"PimUnit", LocalMemoryType::ram, pim_unit_address_space, {}, {}};
    std::vector<const LocalMemoryConfig*> memory_check_list{&pim_memory_config};
    std::transform(local_memory_list.begin(), local_memory_list.end(),
                   std::inserter(memory_check_list, memory_check_list.end()),
                   [](const LocalMemoryConfig& config) { return &config; });
    std::sort(memory_check_list.begin(), memory_check_list.end(),
              [](const LocalMemoryConfig* config1, const LocalMemoryConfig* config2) {
                  return config1->addressing.offset < config2->addressing.offset;
              });
    for (int i = 0; i < memory_check_list.size() - 1; i++) {
        if (memory_check_list[i]->addressing.end() > memory_check_list[i + 1]->addressing.offset) {
            std::cerr << "LocalMemoryUnitConfig not valid" << std::endl;
            std::cerr << fmt::format("There is an overlap in address space between local memory '{}' and '{}'",
                                     memory_check_list[i]->name, memory_check_list[i + 1]->name)
                      << std::endl;
            return false;
        }
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LocalMemoryUnitConfig, pim_unit_address_space, local_memory_list)

// CoreConfig
bool CoreConfig::checkValid() const {
    if (const bool valid = control_unit_config.checkValid() && register_unit_config.checkValid() &&
                           scalar_unit_config.checkValid() && simd_unit_config.checkValid() &&
                           pim_unit_config.checkValid() && local_memory_unit_config.checkValid();
        !valid) {
        std::cerr << "CoreConfig not valid" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CoreConfig, control_unit_config, register_unit_config,
                                                scalar_unit_config, simd_unit_config, pim_unit_config,
                                                local_memory_unit_config)

// ChipConfig
bool ChipConfig::checkValid() const {
    if (!check_positive(core_cnt)) {
        std::cerr << "ChipConfig not valid, 'core_cnt' must be positive" << std::endl;
        return false;
    }
    if (const bool valid = core_config.checkValid() && global_memory_config.checkValid(); !valid) {
        std::cerr << "ChipConfig not valid" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChipConfig, core_cnt, core_config, global_memory_config)

// SimConfig
bool SimConfig::checkValid() const {
    if (sim_mode == +SimMode::other) {
        std::cerr << "SimConfig not valid, 'sim_mode' must be 'run_until_time' or 'run_one_round'" << std::endl;
        return false;
    }
    if (data_mode == +DataMode::other) {
        std::cerr << "SimConfig not valid, 'data_mode' must be 'real_data' or 'not_real_data'" << std::endl;
        return false;
    }
    if (sim_mode == +SimMode::run_until_time && !check_positive(sim_time)) {
        std::cerr << "SimConfig not valid, 'sim_time' must be positive" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SimConfig, sim_mode, data_mode, sim_time)

// Config
bool Config::checkValid() const {
    if (!(chip_config.checkValid() && sim_config.checkValid())) {
        std::cerr << "Config not valid" << std::endl;
        return false;
    }
    return true;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config, chip_config, sim_config)

}  // namespace pimsim
