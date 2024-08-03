//
// Created by wyk on 2024/7/9.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/simd_unit/simd_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct SIMDTestInstruction {
    SIMDInsPayload payload;
};

struct SIMDTestInfo {
    std::vector<SIMDTestInstruction> code{};
    TestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDTestInfo, code, expected)

class SIMDUnitTestModule : public ExecuteUnitTestModule<SIMDUnitTestModule, SIMDUnit, SIMDUnitConfig, SIMDInsPayload,
                                                        SIMDTestInstruction, TestExpectedInfo, SIMDTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

private:
    DataConflictPayload getInsPayloadConflictInfos(const pimsim::SIMDInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.pc = ins_payload.ins.pc};
        for (unsigned int i = 0; i < ins_payload.input_cnt; i++) {
            int read_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.inputs_address_byte[i]);
            conflict_payload.read_memory_id.insert(read_memory_id);
            conflict_payload.used_memory_id.insert(read_memory_id);
        }
        int write_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.output_address_byte);
        conflict_payload.write_memory_id.insert(write_memory_id);
        conflict_payload.used_memory_id.insert(write_memory_id);
        return std::move(conflict_payload);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, SIMDTestInfo& test_info) {
        return new SIMDUnitTestModule{
            "SIMD_unit_test_module",  "SIMDUnit", config.chip_config.core_config.simd_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return pimsim_unit_test<SIMDUnitTestModule>(argc, argv, initializer);
}
