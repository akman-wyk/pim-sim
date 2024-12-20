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
#include "network/payload.h"
#include "network/switch_socket.h"

namespace pimsim {

struct TransferInstructionInfo {
    InstructionPayload ins{};

    TransferType type{TransferType::local_trans};

    int src_start_address_byte{0};
    int dst_start_address_byte{0};
    int batch_max_data_size_byte{0};

    int src_id{0};
    int dst_id{0};
    int transfer_id_tag{0};

    bool use_pipeline{false};
};

struct TransferBatchInfo {
    int batch_num{0};
    int batch_data_size_byte{0};
    bool first_batch{false};
    bool last_batch{false};
    std::vector<unsigned char> data{};
};

struct TransferSubmodulePayload {
    TransferInstructionInfo ins_info;
    TransferBatchInfo batch_info;
};

class TransferUnit : public BaseModule {
public:
    SC_HAS_PROCESS(TransferUnit);

    TransferUnit(const char* name, const TransferUnitConfig& config, const SimConfig& sim_config, Core* core,
                 Clock* clk, int core_id = 0, int global_memory_switch_id = -10);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadSubmodule();
    [[noreturn]] void processWriteSubmodule();
    void finishInstruction();
    void finishRun();

    void checkTransferInst();

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);
    void bindSwitch(Switch* switch_);

private:
    static void waitAndStartNextSubmodule(TransferSubmodulePayload& cur_payload,
                                          SubmoduleSocket<TransferSubmodulePayload>& next_submodule_socket);

    std::pair<TransferInstructionInfo, DataConflictPayload> decodeAndGetInfo(const TransferInsPayload& payload) const;

    void switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload);

    void processSendHandshake(int dst_id, int transfer_id_tag);
    void processSendData(int dst_id, int transfer_id_tag, int dst_address_byte, int data_size_byte);

    void processReceiveHandshake(int src_id, int transfer_id_tag);
    void processReceiveData(int src_id);

    void processLoadGlobalData(const InstructionPayload& ins, int src_address_byte, int data_size_byte);
    void processStoreGlobalData(const InstructionPayload& ins, int dst_address_byte, int data_size_byte);

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
    int finish_ins_id_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};

    // send receive
    const int core_id_;
    SwitchSocket switch_socket_;

    // -1 for not ready
    int expected_receiver_core_id_ = -1;
    int expected_sender_core_id_ = -1;  // execute recv and recv sender_core_id's send inst
    int expected_transfer_id_tag_ = -1;

    sc_event sender_wait_receiver_ready_;
    sc_event receiver_wait_sender_ready_;
    sc_event receiver_wait_data_ready_;

    std::unordered_map<int, int> receiver_waiting_sender_map;  // <core_id_, transfer_id_tag>

    // load store
    const int global_memory_switch_id_;
    sc_event finish_read_global_;
    sc_event finish_write_global_;
};

}  // namespace pimsim
