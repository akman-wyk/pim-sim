//
// Created by wyk on 2024/7/15.
//

#pragma once

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/payload/execute_unit_payload.h"
#include "core/payload/payload.h"

namespace pimsim {

struct TransferInstructionInfo {
    InstructionPayload ins{};

    int src_start_address_byte{0};
    int dst_start_address_byte{0};
    int data_width_byte{0};

    bool use_pipeline{false};
};

struct TransferBatchInfo {
    int batch_num{0};
    int batch_data_size_byte{0};
    bool first_batch{false};
    bool last_batch{false};
};

struct TransferSubmodulePayload {
    TransferInstructionInfo ins_info;
    TransferBatchInfo batch_info;
};

class TransferUnit : public BaseModule {
public:
    SC_HAS_PROCESS(TransferUnit);

    TransferUnit(const char* name, const TransferUnitConfig& config, const SimConfig& sim_config, Core* core,
                 Clock* clk);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadSubmodule();
    [[noreturn]] void processWriteSubmodule();
    void finishInstruction();
    void finishRun();

    void checkTransferInst();

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

private:
    static void waitAndStartNextSubmodule(const TransferSubmodulePayload& cur_payload,
                                          SubmoduleSocket<TransferSubmodulePayload>& next_submodule_socket);

    std::pair<TransferInstructionInfo, DataConflictPayload> decodeAndGetInfo(const TransferInsPayload& payload) const;

public:
    ExecuteUnitResponseIOPorts<TransferInsPayload> ports_;

private:
    const TransferUnitConfig& config_;

    FSM<TransferInsPayload> transfer_fsm_;
    sc_core::sc_signal<TransferInsPayload> transfer_fsm_out_;
    sc_core::sc_signal<FSMPayload<TransferInsPayload>> transfer_fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event cur_ins_next_batch_;
    SubmoduleSocket<TransferSubmodulePayload> read_submodule_socket_{};
    SubmoduleSocket<TransferSubmodulePayload> write_submodule_socket_{};

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};
};

}  // namespace pimsim
