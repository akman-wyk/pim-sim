//
// Created by wyk on 2024/6/14.
//

#pragma once

#include <string>
#include <vector>

#include "config_enum.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace pimsim {

static constexpr int GENERAL_REG_NUM = 32;
static constexpr int SPECIAL_REG_NUM = 32;

static constexpr int SIMD_MAX_INPUT_NUM = 4;
static constexpr int SIMD_MAX_OPCODE = 255;

struct ControlUnitConfig {
    double controller_static_power_mW{0.0};   // mW
    double controller_dynamic_power_mW{0.0};  // mW

    double fetch_static_power_mW{0.0};   // mW
    double fetch_dynamic_power_mW{0.0};  // mW

    double decode_static_power_mW{0.0};   // mW
    double decode_dynamic_power_mW{0.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ControlUnitConfig)
};

struct SpecialRegisterBindingConfig {
    int special;  // 0-31
    int general;  // 0-31

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SpecialRegisterBindingConfig)
};

struct RegisterUnitConfig {
    double static_power_mW{0.0};   // mW
    double dynamic_power_mW{0.0};  // mW

    std::vector<SpecialRegisterBindingConfig> special_register_binding{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(RegisterUnitConfig)
};

struct ScalarFunctorConfig {
    std::string inst_name{"scalar-RR-add"};
    double static_power_mW{0.0};   // mW
    double dynamic_power_mW{0.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarFunctorConfig)
};

struct ScalarUnitConfig {
    double default_functor_static_power_mW{0.0};   // mW
    double default_functor_dynamic_power_mW{0.0};  // mW
    std::vector<ScalarFunctorConfig> functor_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarUnitConfig)
};

struct SIMDDataWidthConfig {
    // data bit-width of input and output of SIMD functor
    int input1{0};  // bit
    int input2{0};  // bit
    int input3{0};  // bit
    int input4{0};  // bit
    int output{0};  // bit

    [[nodiscard]] static bool checkDataWidth(int width);
    [[nodiscard]] bool inputBitWidthMatch(const SIMDDataWidthConfig& other) const;
    [[nodiscard]] bool checkValid(unsigned int input_cnt, bool check_output) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDDataWidthConfig)
};

struct SIMDFunctorConfig {
    std::string name{};  // need to write an execution function with the same name in file TODO
    unsigned int input_cnt{2};
    SIMDDataWidthConfig data_bit_width{};

    int functor_cnt{32};
    int latency_cycle{1};                      // cycle
    double static_power_per_functor_mW{1.0};   // mW
    double dynamic_power_per_functor_mW{1.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDFunctorConfig)
};

struct SIMDInstructionFunctorBindingConfig {
    SIMDDataWidthConfig input_bit_width;
    std::string functor_name{};

    [[nodiscard]] bool checkValid(unsigned int input_cnt) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInstructionFunctorBindingConfig)
};

struct SIMDInstructionConfig {
    std::string name{};
    unsigned int input_cnt{2};
    unsigned int opcode{0x00};

    SIMDInputType input1_type{SIMDInputType::vector};
    SIMDInputType input2_type{SIMDInputType::vector};
    SIMDInputType input3_type{SIMDInputType::vector};
    SIMDInputType input4_type{SIMDInputType::vector};

    std::vector<SIMDInstructionFunctorBindingConfig> functor_binding_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInstructionConfig)
};

struct SIMDUnitConfig {
    std::vector<SIMDFunctorConfig> functor_list;
    std::vector<SIMDInstructionConfig> instruction_list;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDUnitConfig)
};

struct PimMacroSizeConfig {
    // macro 4-dim size (H, W, m, n)
    int compartment_cnt_per_macro{};    // H, element row cnt
    int element_cnt_per_compartment{};  // W, element column cnt
    int row_cnt_per_element{};          // m, element row size
    int bit_width_per_row{};            // n, element column size

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimMacroSizeConfig)
};

struct PimModuleConfig {
    int latency_cycle{1};          // cycle
    double static_power_mW{1.0};   // mW
    double dynamic_power_mW{1.0};  // mW

    [[nodiscard]] bool checkValid(const std::string& module_name) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimModuleConfig)
};

struct PimSRAMConfig {
    /* Pim SRAM hardware config
     * Pim write:
     *      All Macros can be written in parallel, each Macro can only write a whole row of one Compartment or not.
     * Pim read:
     *      All Macros can be read in parallel, each Macro can read the same whole row of 0 to all Compartments in
     * parallel.
     *
     * Then, as_mode only influences write-data-width:
     *      When as_mode is intergroup, it means all MacroGroups can be written in parallel, so that
     *      write-data-width = macro-width * macro-total-count.
     *      When as_mode is intragroup, it means only one MacroGroup can be written at a time, so that
     *      write-data-width = macro-width * macro-group-size.
     * And write-unit is always macro-width, which means the minimum write data unit width.
     */
    PimSRAMAddressSpaceContinuousMode as_mode{PimSRAMAddressSpaceContinuousMode::intergroup};

    int write_latency_cycle{1};
    int read_latency_cycle{1};
    double static_power_mW{1.0};
    double write_dynamic_power_per_bit_mW{1.0};
    double read_dynamic_power_per_bit_mW{1.0};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimSRAMConfig)
};

struct PimValueSparseConfig {
    // sparse mask config
    int mask_bit_width{1};

    // Input process module config: PimUnit only has one
    int latency_cycle{1};          // cycle
    double static_power_mW{1.0};   // mW
    double dynamic_power_mW{1.0};  // mW
    int output_macro_cnt{1};       // The number of macros processed at a time

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimValueSparseConfig)
};

struct PimBitSparseConfig {
    // sparse mask config
    int mask_bit_width{3};

    // post process module config: each Element has one
    int latency_cycle{1};          // cycle
    double static_power_mW{1.0};   // mW
    double dynamic_power_mW{1.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimBitSparseConfig)
};

struct PimUnitConfig {
    // macro scale config
    int macro_total_cnt{};
    std::vector<int> macro_group_size_configurable_values{};
    PimMacroSizeConfig macro_size{};

    // modules config: ipu -> SRAM -> post process modules
    // ipu(input process unit) module
    PimModuleConfig ipu{};
    // SRAM module
    PimSRAMConfig sram{};
    // post process modules, each Element column has one
    PimModuleConfig adder_tree{};
    PimModuleConfig shift_adder{};
    PimModuleConfig result_adder{};

    // extensions config
    bool value_sparse{false};
    PimValueSparseConfig value_sparse_config{};

    bool bit_sparse{false};
    PimBitSparseConfig bit_sparse_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimUnitConfig)
};

struct RAMConfig {
    int size{1024};  // kB, total ram size
    int width{16};   // Byte, the byte-width of single read and write data

    int write_latency_cycle{1};  // cycle
    int read_latency_cycle{1};   // cycle

    double static_power_mW{1.0};         // mW
    double write_dynamic_power_mW{1.0};  // mW
    double read_dynamic_power_mW{1.0};   // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(RAMConfig)
};

struct RegBufferConfig {
    int size{1024};       // Byte, total register buffer size
    int read_width{16};   // Byte, the max byte-width of single read data
    int write_width{16};  // Byte, the max byte-width of single write data
    int rw_unit{4};       // Byte, the min unit byte-width of single read and write data

    double static_power_mW{1.0};               // mW
    double rw_dynamic_power_per_unit_mW{1.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(RegBufferConfig)
};

struct LocalMemoryAddressSpaceConfig {
    int offset;  // byte
    int size;    // byte

    [[nodiscard]] int end() const;
    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(LocalMemoryAddressSpaceConfig)
};

struct LocalMemoryConfig {
    std::string name{};
    LocalMemoryType type{LocalMemoryType::ram};

    LocalMemoryAddressSpaceConfig addressing{};

    RAMConfig ram_config{};
    RegBufferConfig reg_buffer_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(LocalMemoryConfig)
};

struct LocalMemoryUnitConfig {
    LocalMemoryAddressSpaceConfig pim_unit_address_space{};
    std::vector<LocalMemoryConfig> local_memory_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(LocalMemoryUnitConfig)
};

struct CoreConfig {
    ControlUnitConfig control_unit_config{};
    RegisterUnitConfig register_unit_config{};
    ScalarUnitConfig scalar_unit_config{};
    SIMDUnitConfig simd_unit_config{};
    PimUnitConfig pim_unit_config{};
    LocalMemoryUnitConfig local_memory_unit_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CoreConfig)
};

struct ChipConfig {
    int core_cnt{1};
    CoreConfig core_config{};
    RAMConfig global_memory_config{};
    // NetworkConfig network_config;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ChipConfig)
};

struct SimConfig {
    SimMode sim_mode{SimMode::run_one_round};
    DataMode data_mode{DataMode::real_data};
    double sim_time{1.0};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SimConfig)
};

struct Config {
    ChipConfig chip_config{};
    SimConfig sim_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(Config)
};

}  // namespace pimsim