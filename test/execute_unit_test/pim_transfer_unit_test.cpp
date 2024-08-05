//
// Created by wyk on 2024/8/2.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/pim_unit/pim_transfer_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct PimTransferTestInstruction {
    PimTransferInsPayload payload;
};

struct PimTransferTestInfo {
    std::vector<PimTransferTestInstruction> code{};
    TestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimTransferInsPayload, ins, output_num, output_bit_width,
                                               output_mask_addr_byte, src_addr_byte, dst_addr_byte, buffer_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimTransferTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimTransferTestInfo, code, expected)

class PimTransferUnitTestModule
    : public ExecuteUnitTestModule<PimTransferUnitTestModule, PimTransferUnit, PimUnitConfig, PimTransferInsPayload,
                                   PimTransferTestInstruction, TestExpectedInfo, PimTransferTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        return std::move(reporter);
    }

private:
    DataConflictPayload getInsPayloadConflictInfos(const pimsim::PimTransferInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.pc = ins_payload.ins.pc};
        conflict_payload.addReadMemoryId(
            {local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.src_addr_byte),
             local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.output_mask_addr_byte)});
        conflict_payload.addWriteMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.dst_addr_byte));
        conflict_payload.addReadWriteMemoryId(
            local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.buffer_addr_byte));
        return std::move(conflict_payload);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, PimTransferTestInfo& test_info) {
        return new PimTransferUnitTestModule{
            "PimTransferUnitTestModule", "PimTransferUnit", config.chip_config.core_config.pim_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return pimsim_unit_test<PimTransferUnitTestModule>(argc, argv, initializer);
}
