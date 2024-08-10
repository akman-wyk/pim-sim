//
// Created by wyk on 2024/8/2.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/pim_unit/pim_output_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct PimOutputTestInstruction {
    PimOutputInsPayload payload;
};

struct PimOutputTestInfo {
    std::vector<PimOutputTestInstruction> code{};
    TestExpectedInfo expected{};
};

void to_json(nlohmann::ordered_json& j, const PimOutputType& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, PimOutputType& m) {
    const auto str = j.get<std::string>();
    m = PimOutputType::_from_string(str.c_str());
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimOutputInsPayload, ins, activation_group_num, output_type,
                                               output_addr_byte, output_cnt_per_group, output_bit_width,
                                               output_mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimOutputTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimOutputTestInfo, code, expected)

class PimOutputUnitTestModule
    : public ExecuteUnitTestModule<PimOutputUnitTestModule, PimOutputUnit, PimUnitConfig, PimOutputInsPayload,
                                   PimOutputTestInstruction, TestExpectedInfo, PimOutputTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

private:
    DataConflictPayload getInsPayloadConflictInfos(const pimsim::PimOutputInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.ins_id = ins_payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_output};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addWriteMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.output_addr_byte));
        if (ins_payload.output_type == +PimOutputType::output_sum) {
            conflict_payload.addReadMemoryId(
                local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.output_mask_addr_byte));
        }
        return std::move(conflict_payload);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, PimOutputTestInfo& test_info) {
        return new PimOutputUnitTestModule{
            "PimOutputUnitTestModule", "PimOutputUnit", config.chip_config.core_config.pim_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return pimsim_unit_test<PimOutputUnitTestModule>(argc, argv, initializer);
}
