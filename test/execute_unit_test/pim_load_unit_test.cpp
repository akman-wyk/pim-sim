//
// Created by wyk on 2024/8/8.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/pim_unit/pim_load_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct PimLoadTestInstruction {
    PimLoadInsPayload payload;
};

struct PimLoadTestInfo {
    std::vector<PimLoadTestInstruction> code{};
    TestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimLoadInsPayload, ins, src_address_byte, size_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimLoadTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimLoadTestInfo, code, expected)

class PimLoadUnitTestModule
    : public ExecuteUnitTestModule<PimLoadUnitTestModule, PimLoadUnit, PimUnitConfig, PimLoadInsPayload,
                                   PimLoadTestInstruction, TestExpectedInfo, PimLoadTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

private:
    DataConflictPayload getInsPayloadConflictInfos(const pimsim::PimLoadInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.ins_id = ins_payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_load};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addReadMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.src_address_byte));
        return std::move(conflict_payload);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, PimLoadTestInfo& test_info) {
        return new PimLoadUnitTestModule{
            "PimLoadUnitTestModule",  "PimLoadUnit", config.chip_config.core_config.pim_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return pimsim_unit_test<PimLoadUnitTestModule>(argc, argv, initializer);
}
