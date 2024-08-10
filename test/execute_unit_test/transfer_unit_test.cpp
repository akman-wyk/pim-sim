//
// Created by wyk on 2024/7/16.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/transfer_unit/transfer_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct TransferTestInstruction {
    TransferInsPayload payload;
};

struct TransferTestInfo {
    std::vector<TransferTestInstruction> code{};
    TestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferTestInfo, code, expected)

class TransferUnitTestModule
    : public ExecuteUnitTestModule<TransferUnitTestModule, TransferUnit, TransferUnitConfig, TransferInsPayload,
                                   TransferTestInstruction, TestExpectedInfo, TransferTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    DataConflictPayload getInsPayloadConflictInfos(const pimsim::TransferInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.ins_id = ins_payload.ins.ins_id, .unit_type = ExecuteUnitType::transfer};

        int src_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.src_address_byte);
        int dst_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.dst_address_byte);

        conflict_payload.read_memory_id.insert(src_memory_id);
        conflict_payload.write_memory_id.insert(dst_memory_id);
        conflict_payload.used_memory_id.insert({src_memory_id, dst_memory_id});

        return std::move(conflict_payload);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, TransferTestInfo& test_info) {
        return new TransferUnitTestModule{"transfer_unit_test_module",
                                          "TransferUnit",
                                          config.chip_config.core_config.transfer_unit_config,
                                          config,
                                          clk,
                                          std::move(test_info.code)};
    };
    return pimsim_unit_test<TransferUnitTestModule>(argc, argv, initializer);
}
