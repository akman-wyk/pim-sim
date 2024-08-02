//
// Created by wyk on 2024/7/31.
//

#include "pim_output_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

PimOutputUnit::PimOutputUnit(const char *name, const pimsim::PimUnitConfig &config, const pimsim::SimConfig &sim_config,
                             pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), fsm_("PimOutputFSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkPimOutputInst)
    sensitive << id_pim_output_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processExecute)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
}

void PimOutputUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

EnergyReporter PimOutputUnit::getEnergyReporter() {
    EnergyReporter pim_output_reporter;
    pim_output_reporter.addSubModule("result adder", EnergyReporter{result_adder_energy_counter_});
    return std::move(pim_output_reporter);
}

void PimOutputUnit::checkPimOutputInst() {
    if (const auto &payload = id_pim_output_payload_port_.read(); payload.ins.valid()) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void PimOutputUnit::processIssue() {
    while (true) {
        wait(fsm_.start_exec_);

        busy_port_.write(true);

        const auto &payload = fsm_out_.read();
        LOG(fmt::format("Pim output start, pc: {}", payload.ins.pc));

        DataConflictPayload conflict_payload{.pc = payload.ins.pc};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.output_addr_byte));
        if (payload.output_type == +PimOutputType::output_sum) {
            conflict_payload.addWriteMemoryId(
                local_memory_socket_.getLocalMemoryIdByAddress(payload.output_mask_addr_byte));
        }
        data_conflict_port_.write(conflict_payload);

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = payload;
        execute_socket_.start_exec.notify();

        busy_port_.write(false);
        fsm_.finish_exec_.notify(SC_ZERO_TIME);
    }
}

void PimOutputUnit::processExecute() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        LOG(fmt::format("Pim output start execute, pc: {}", payload.ins.pc));

        if (payload.output_type == +PimOutputType::only_output) {
            processOnlyOutput(payload);
        } else if (payload.output_type == +PimOutputType::output_sum) {
            processOutputSum(payload);
        } else {
            processOutputSumMove(payload);
        }

        // check if last pc
        if (isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }

        execute_socket_.finish();
    }
}

void PimOutputUnit::processOnlyOutput(const pimsim::PimOutputInsPayload &payload) {
    finish_ins_ = true;
    finish_ins_pc_ = payload.ins.pc;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

    int size_byte =
        IntDivCeil(payload.output_bit_width * payload.output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

void PimOutputUnit::processOutputSum(const pimsim::PimOutputInsPayload &payload) {
    // read and process sum mask
    int mask_size_byte = IntDivCeil(payload.output_cnt_per_group, BYTE_TO_BIT);
    auto mask_byte_data = local_memory_socket_.readData(payload.ins, payload.output_mask_addr_byte, mask_size_byte);
    int sum_times_per_group = 0;
    for (int i = 0; i < payload.output_cnt_per_group; i++) {
        if (getMaskBit(mask_byte_data, i) != 0) {
            sum_times_per_group++;
        }
    }

    // sum
    double sum_latency = config_.result_adder.latency_cycle * period_ns_;
    double sum_dynamic_power_mW =
        config_.result_adder.dynamic_power_mW * sum_times_per_group * payload.activation_group_num;
    result_adder_energy_counter_.addDynamicEnergyPJ(sum_latency, sum_dynamic_power_mW);

    // need not wait for result adder finish, because result is written to memory instead of registers in result adder
    double sum_stall_ns = (config_.result_adder.latency_cycle - 1) * period_ns_;
    wait(sum_stall_ns, SC_NS);

    finish_ins_ = true;
    finish_ins_pc_ = payload.ins.pc;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

    // write to memory
    int valid_output_cnt_per_group = payload.output_cnt_per_group - sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

void PimOutputUnit::processOutputSumMove(const pimsim::PimOutputInsPayload &payload) {
    int sum_times_per_group = payload.output_cnt_per_group;

    double sum_latency = config_.result_adder.latency_cycle * period_ns_;
    double sum_dynamic_power_mW =
        config_.result_adder.dynamic_power_mW * sum_times_per_group * payload.activation_group_num;
    result_adder_energy_counter_.addDynamicEnergyPJ(sum_latency, sum_dynamic_power_mW);

    // need not wait for result adder finish, because result is written to memory instead of registers in result adder
    double sum_stall_ns = (config_.result_adder.latency_cycle - 1) * period_ns_;
    wait(sum_stall_ns, SC_NS);

    finish_ins_ = true;
    finish_ins_pc_ = payload.ins.pc;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

    // write to memory
    int valid_output_cnt_per_group = sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

void PimOutputUnit::finishInstruction() {
    finish_ins_port_.write(finish_ins_);
    finish_ins_pc_port_.write(finish_ins_pc_);
}

void PimOutputUnit::finishRun() {
    finish_run_port_.write(finish_run_);
}

}  // namespace pimsim
