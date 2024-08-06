//
// Created by wyk on 2024/7/15.
//

#include "transfer_unit.h"

#include "fmt/core.h"
#include "systemc.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

TransferUnit::TransferUnit(const char* name, const TransferUnitConfig& config, const SimConfig& sim_config, Core* core,
                           Clock* clk)
    : BaseModule(name, sim_config, core, clk), config_(config), transfer_fsm_("TransferUnitFSM", clk) {
    transfer_fsm_.input_.bind(transfer_fsm_in_);
    transfer_fsm_.enable_.bind(ports_.id_ex_enable_port_);
    transfer_fsm_.output_.bind(transfer_fsm_out_);

    SC_METHOD(checkTransferInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processReadSubmodule)
    SC_THREAD(processWriteSubmodule)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
}

void TransferUnit::checkTransferInst() {
    if (const auto& payload = ports_.id_ex_payload_port_.read(); payload.ins.valid()) {
        transfer_fsm_in_.write({payload, true});
    } else {
        transfer_fsm_in_.write({{}, false});
    }
}

void TransferUnit::processIssue() {
    while (true) {
        wait(transfer_fsm_.start_exec_);
        ports_.busy_port_.write(true);

        const auto& payload = transfer_fsm_out_.read();
        const auto& [ins_info, conflict_payload] = decodeAndGetInfo(payload);
        ports_.data_conflict_port_.write(conflict_payload);

        int process_times = IntDivCeil(payload.size_byte, ins_info.data_width_byte);
        TransferSubmodulePayload submodule_payload{.ins_info = ins_info};
        for (int batch = 0; batch < process_times; batch++) {
            submodule_payload.batch_info = {
                .batch_num = batch,
                .batch_data_size_byte = (batch == process_times - 1)
                                            ? payload.size_byte - batch * ins_info.data_width_byte
                                            : ins_info.data_width_byte,
                .first_batch = (batch == 0),
                .last_batch = (batch == process_times - 1)};
            waitAndStartNextSubmodule(submodule_payload, read_submodule_socket_);

            if (!submodule_payload.batch_info.last_batch) {
                wait(cur_ins_next_batch_);
            }
        }

        ports_.busy_port_.write(false);
        transfer_fsm_.finish_exec_.notify(SC_ZERO_TIME);
    }
}

void TransferUnit::processReadSubmodule() {
    while (true) {
        read_submodule_socket_.waitUntilStart();

        const auto& payload = read_submodule_socket_.payload;
        LOG(fmt::format("transfer read start, pc: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.batch_info.batch_num));

        int address_byte =
            payload.ins_info.src_start_address_byte + payload.batch_info.batch_num * payload.ins_info.data_width_byte;
        int size_byte = payload.batch_info.batch_data_size_byte;
        local_memory_socket_.readData(payload.ins_info.ins, address_byte, size_byte);

        waitAndStartNextSubmodule(payload, write_submodule_socket_);

        if (!payload.batch_info.last_batch && payload.ins_info.use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        read_submodule_socket_.finish();
    }
}

void TransferUnit::processWriteSubmodule() {
    while (true) {
        write_submodule_socket_.waitUntilStart();

        const auto& payload = write_submodule_socket_.payload;
        LOG(fmt::format("transfer write start, pc: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.batch_info.batch_num));

        if (payload.batch_info.last_batch) {
            finish_ins_ = true;
            finish_ins_pc_ = payload.ins_info.ins.pc;
            finish_ins_trigger_.notify(SC_ZERO_TIME);
        }

        int address_byte =
            payload.ins_info.dst_start_address_byte + payload.batch_info.batch_num * payload.ins_info.data_width_byte;
        int size_byte = payload.batch_info.batch_data_size_byte;
        local_memory_socket_.writeData(payload.ins_info.ins, address_byte, size_byte, {});

        LOG(fmt::format("transfer write end, pc: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.batch_info.batch_num));

        if (!payload.batch_info.last_batch && !payload.ins_info.use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        write_submodule_socket_.finish();

        if (payload.batch_info.last_batch && isEndPC(payload.ins_info.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }
    }
}

void TransferUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_pc_port_.write(finish_ins_pc_);
}

void TransferUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

void TransferUnit::bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void TransferUnit::waitAndStartNextSubmodule(const pimsim::TransferSubmodulePayload& cur_payload,
                                             SubmoduleSocket<pimsim::TransferSubmodulePayload>& next_submodule_socket) {
    next_submodule_socket.waitUntilFinishIfBusy();
    if (cur_payload.batch_info.first_batch) {
        next_submodule_socket.payload.ins_info = cur_payload.ins_info;
    }
    next_submodule_socket.payload.batch_info = cur_payload.batch_info;
    next_submodule_socket.start_exec.notify();
}

std::pair<TransferInstructionInfo, DataConflictPayload> TransferUnit::decodeAndGetInfo(
    const TransferInsPayload& payload) const {
    int src_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.src_address_byte);
    int dst_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.dst_address_byte);

    int data_width_byte = std::min(local_memory_socket_.getMemoryDataWidthById(src_memory_id, MemoryAccessType::read),
                                   local_memory_socket_.getMemoryDataWidthById(dst_memory_id, MemoryAccessType::write));
    bool use_pipeline = config_.pipeline && (src_memory_id != dst_memory_id);

    TransferInstructionInfo ins_info{.ins = payload.ins,
                                     .src_start_address_byte = payload.src_address_byte,
                                     .dst_start_address_byte = payload.dst_address_byte,
                                     .data_width_byte = data_width_byte,
                                     .use_pipeline = use_pipeline};
    DataConflictPayload conflict_payload{.pc = payload.ins.pc, .unit_type = ExecuteUnitType::transfer};
    conflict_payload.read_memory_id.insert(src_memory_id);
    conflict_payload.write_memory_id.insert(dst_memory_id);
    conflict_payload.used_memory_id.insert({src_memory_id, dst_memory_id});

    return {ins_info, std::move(conflict_payload)};
}

}  // namespace pimsim
