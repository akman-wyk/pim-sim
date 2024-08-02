//
// Created by wyk on 2024/7/31.
//

#include "pim_transfer_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

PimTransferUnit::PimTransferUnit(const char *name, const pimsim::PimUnitConfig &config,
                                 const pimsim::SimConfig &sim_config, pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), fsm_("PimTransferFSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkPimTransferInst)
    sensitive << id_pim_transfer_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processExecute)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
}

void PimTransferUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void PimTransferUnit::checkPimTransferInst() {
    if (const auto &payload = id_pim_transfer_payload_port_.read(); payload.ins.valid()) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void PimTransferUnit::processIssue() {
    while (true) {
        wait(fsm_.start_exec_);

        busy_port_.write(true);

        const auto &payload = fsm_out_.read();
        LOG(fmt::format("Pim transfer start, pc: {}", payload.ins.pc));

        DataConflictPayload conflict_payload{.pc = payload.ins.pc};
        conflict_payload.addReadMemoryId(
            {local_memory_socket_.getLocalMemoryIdByAddress(payload.src_addr_byte),
             local_memory_socket_.getLocalMemoryIdByAddress(payload.output_mask_addr_byte)});
        conflict_payload.addWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.dst_addr_byte));
        conflict_payload.addReadWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.buffer_addr_byte));
        data_conflict_port_.write(conflict_payload);

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = payload;
        execute_socket_.start_exec.notify();

        busy_port_.write(false);
        fsm_.finish_exec_.notify(SC_ZERO_TIME);
    }
}

void PimTransferUnit::processExecute() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        LOG(fmt::format("Pim transfer start execute, pc: {}", payload.ins.pc));

        // read ans process transfer mask
        int mask_size_byte = IntDivCeil(payload.output_num, BYTE_TO_BIT);
        auto mask_byte_data = local_memory_socket_.readData(payload.ins, payload.output_mask_addr_byte, mask_size_byte);
        int valid_output_cnt = 0;
        for (int i = 0; i < payload.output_num; i++) {
            if (getMaskBit(mask_byte_data, i) != 0) {
                valid_output_cnt++;
            }
        }

        // calculate parameters
        int buffer_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.buffer_addr_byte);
        int buffer_max_size_byte = local_memory_socket_.getMemorySizeById(buffer_memory_id);
        int output_byte_width = IntDivCeil(payload.output_bit_width, BYTE_TO_BIT);
        int buffer_max_output_cnt = buffer_max_size_byte / output_byte_width;
        int buffer_max_output_size_byte = output_byte_width * buffer_max_output_cnt;
        int dst_max_write_times = IntDivCeil(valid_output_cnt, buffer_max_output_cnt);

        // src -> buffer -> dst
        int buffer_output_cnt = 0;
        int dst_write_times = 0;
        for (int i = 0; i < payload.output_num; i++) {
            if (getMaskBit(mask_byte_data, i) != 0) {
                int src_addr_byte = payload.src_addr_byte + i * output_byte_width;
                local_memory_socket_.readData(payload.ins, src_addr_byte, output_byte_width);

                int buffer_addr_byte = payload.buffer_addr_byte + buffer_output_cnt * output_byte_width;
                local_memory_socket_.writeData(payload.ins, buffer_addr_byte, output_byte_width, {});

                buffer_output_cnt++;
            }

            if (buffer_output_cnt == buffer_max_output_cnt || (i == payload.output_num - 1 && buffer_output_cnt > 0)) {
                int size_byte = buffer_output_cnt * output_byte_width;
                local_memory_socket_.readData(payload.ins, payload.buffer_addr_byte, size_byte);

                if (dst_write_times == dst_max_write_times - 1) {
                    finish_ins_ = true;
                    finish_ins_pc_ = payload.ins.pc;
                    finish_ins_trigger_.notify(SC_ZERO_TIME);
                }

                int dst_addr_byte = payload.dst_addr_byte + dst_write_times * buffer_max_output_size_byte;
                local_memory_socket_.writeData(payload.ins, dst_addr_byte, size_byte, {});

                dst_write_times++;
                buffer_output_cnt = 0;
            }
        }

        // check if last pc
        if (isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }

        execute_socket_.finish();
    }
}

void PimTransferUnit::finishInstruction() {
    finish_ins_port_.write(finish_ins_);
    finish_ins_pc_port_.write(finish_ins_pc_);
}

void PimTransferUnit::finishRun() {
    finish_run_port_.write(finish_run_);
}

}  // namespace pimsim
