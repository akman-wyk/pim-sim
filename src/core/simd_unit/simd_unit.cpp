//
// Created by wyk on 2024/7/5.
//

#include "simd_unit.h"

#include "fmt/core.h"
#include "util/util.h"

namespace pimsim {

static constexpr int SIMD_INSTRUCTION_OPCODE_BIT_LENGTH = 8;

SIMDUnit::SIMDUnit(const char* name, const SIMDUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk)
    : BaseModule(name, sim_config, core, clk), config_(config), simd_fsm_("SIMD_UNIT_FSM", clk) {
    simd_fsm_.input_.bind(simd_fsm_in_);
    simd_fsm_.enable_.bind(id_ex_enable_port_);
    simd_fsm_.output_.bind(simd_fsm_out_);

    SC_METHOD(checkSIMDInst)
    sensitive << id_simd_payload_port_;

    SC_THREAD(process)
    SC_THREAD(processReadSubmodule)
    SC_THREAD(processExecuteSubmodule)
    SC_THREAD(processWriteSubmodule)

    for (const auto& ins_config : config_.instruction_list) {
        instruction_config_map_.emplace(getSIMDInstructionIdentityCode(ins_config.input_cnt, ins_config.opcode),
                                        &ins_config);
    }

    double SIMD_functors_total_static_power_mW = 0.0;
    for (const auto& functor_config : config_.functor_list) {
        SIMD_functors_total_static_power_mW += functor_config.static_power_per_functor_mW * functor_config.functor_cnt;
        functor_config_map_.emplace(functor_config.name, &functor_config);
    }

    energy_counter_.setStaticPowerMW(SIMD_functors_total_static_power_mW);
}

void SIMDUnit::checkSIMDInst() {
    if (const auto& payload = id_simd_payload_port_.read(); payload.ins->valid()) {
        simd_fsm_in_.write({payload, true});
    } else {
        simd_fsm_in_.write({{}, false});
    }
}

void SIMDUnit::process() {
    while (true) {
        wait(simd_fsm_.start_exec_);

        busy_port_.write(true);
        finish_port_.write(false);

        // Find instruction and functor
        const auto& payload = simd_fsm_out_.read();
        const auto [instruction, functor] = getSIMDInstructionAndFunctor(payload);
        if (instruction == nullptr || functor == nullptr) {
            throw std::runtime_error(fmt::format("Invalid SIMD instruction: \n{}", payload.toString()));
        }

        // Decode instruction
        auto ins_info = decodeAndGetInstructionInfo(instruction, functor, payload);
        int vector_total_len = ins_info.vector_inputs.empty() ? 1 : payload.len;
        int process_times = IntDivCeil(vector_total_len, functor->functor_cnt);

        for (int batch = 0; batch < process_times; batch++) {
            read_submodule_socket_.waitUtilFinish();

            if (batch == 0) {
                read_submodule_socket_.payload.ins_info = ins_info;
            }

            read_submodule_socket_.payload.batch_info = {
                .batch_num = batch,
                .batch_vector_len = (batch == process_times - 1) ? (vector_total_len - batch * functor->functor_cnt)
                                                                 : functor->functor_cnt,
                .first_batch = (batch == 0),
                .last_batch = (batch == process_times - 1)};

            read_submodule_socket_.start_exec.notify();
            wait(next_batch);
        }

        busy_port_.write(false);
    }
}

void SIMDUnit::processReadSubmodule() {
    while (true) {
        wait(read_submodule_socket_.start_exec);
        read_submodule_socket_.busy = true;

        const auto& payload = read_submodule_socket_.payload;
        if (payload.batch_info.first_batch) {
            for (const auto& scalar_input : payload.ins_info.scalar_inputs) {
                local_memory_socket_.readData(payload.ins_info.ins, scalar_input.start_address_byte,
                                              scalar_input.data_bit_width / BYTE_TO_BIT);
            }
        }

        for (const auto& vector_input : payload.ins_info.vector_inputs) {
            int address_byte =
                vector_input.start_address_byte + (payload.batch_info.batch_num * vector_input.data_bit_width *
                                                   payload.ins_info.functor_config->functor_cnt / BYTE_TO_BIT);
            int size_byte = vector_input.data_bit_width * payload.batch_info.batch_vector_len / BYTE_TO_BIT;
            local_memory_socket_.readData(payload.ins_info.ins, address_byte, size_byte);
        }

        execute_submodule_socket_.waitUtilFinish();

        if (payload.batch_info.first_batch) {
            execute_submodule_socket_.payload.ins_info = payload.ins_info;
        }
        execute_submodule_socket_.payload.batch_info = payload.batch_info;

        execute_submodule_socket_.start_exec.notify();
        if (payload.ins_info.use_pipeline) {
            next_batch.notify();
        }
        read_submodule_socket_.finish_exec.notify();
        read_submodule_socket_.busy = false;
    }
}

void SIMDUnit::processExecuteSubmodule() {
    while (true) {
        wait(execute_submodule_socket_.start_exec);
        execute_submodule_socket_.busy = true;

        const auto& payload = execute_submodule_socket_.payload;
        double dynamic_power_mW =
            payload.ins_info.functor_config->dynamic_power_per_functor_mW * payload.batch_info.batch_vector_len;
        double latency = payload.ins_info.functor_config->latency_cycle * period_ns_;
        energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);
        wait(latency, SC_NS);

        write_submodule_socket_.waitUtilFinish();

        if (payload.batch_info.first_batch) {
            write_submodule_socket_.payload.ins_info = payload.ins_info;
        }
        write_submodule_socket_.payload.batch_info = payload.batch_info;

        write_submodule_socket_.start_exec.notify();
        execute_submodule_socket_.finish_exec.notify();
        execute_submodule_socket_.busy = false;
    }
}

void SIMDUnit::processWriteSubmodule() {
    while (true) {
        wait(write_submodule_socket_.start_exec);
        write_submodule_socket_.busy = true;

        const auto& payload = write_submodule_socket_.payload;
        int address_byte = payload.ins_info.output.start_address_byte +
                           (payload.batch_info.batch_num * payload.ins_info.output.data_bit_width *
                            payload.ins_info.functor_config->functor_cnt / BYTE_TO_BIT);
        int size_byte = payload.ins_info.output.data_bit_width * payload.batch_info.batch_vector_len / BYTE_TO_BIT;
        local_memory_socket_.writeData(payload.ins_info.ins, address_byte, size_byte, {});

        if (payload.batch_info.last_batch) {
            finish_port_.write(true);
            finish_pc_port_.write(payload.ins_info.ins->pc);
        }

        if (!payload.ins_info.use_pipeline) {
            next_batch.notify();
        }
        write_submodule_socket_.finish_exec.notify();
        write_submodule_socket_.busy = false;
    }
}

void SIMDUnit::bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

unsigned int SIMDUnit::getSIMDInstructionIdentityCode(unsigned int input_cnt, unsigned int opcode) {
    return ((input_cnt << SIMD_INSTRUCTION_OPCODE_BIT_LENGTH) | opcode);
}

std::pair<const SIMDInstructionConfig*, const SIMDFunctorConfig*> SIMDUnit::getSIMDInstructionAndFunctor(
    const SIMDInsPayload& payload) {
    auto ins_found = instruction_config_map_.find(getSIMDInstructionIdentityCode(payload.input_cnt, payload.opcode));
    if (ins_found == instruction_config_map_.end()) {
        return {nullptr, nullptr};
    }

    auto* ins_config = ins_found->second;
    for (const auto& bind_info : ins_config->functor_binding_list) {
        if (bind_info.input_bit_width.inputs == payload.inputs_bit_width) {
            if (auto functor_found = functor_config_map_.find(bind_info.functor_name);
                functor_found != functor_config_map_.end()) {
                if (auto* functor_config = functor_found->second;
                    payload.input_cnt == functor_config->input_cnt &&
                    payload.inputs_bit_width == functor_config->data_bit_width.inputs &&
                    payload.output_bit_width == functor_config->data_bit_width.output) {
                    return {ins_config, functor_config};
                }
            }
        }
    }

    return {nullptr, nullptr};
}

SIMDInstructionInfo SIMDUnit::decodeAndGetInstructionInfo(const SIMDInstructionConfig* instruction,
                                                          const SIMDFunctorConfig* functor,
                                                          const SIMDInsPayload& payload) const {
    SIMDInputOutputInfo output = {payload.output_bit_width, payload.output_address_byte};
    std::vector<SIMDInputOutputInfo> vector_inputs;
    std::vector<SIMDInputOutputInfo> scalar_inputs;
    for (unsigned int i = 0; i < payload.input_cnt; i++) {
        if (instruction->inputs_type[i] == +SIMDInputType::vector) {
            vector_inputs.emplace_back(
                SIMDInputOutputInfo{payload.inputs_bit_width[i], payload.inputs_address_byte[i]});
        } else {
            scalar_inputs.emplace_back(
                SIMDInputOutputInfo{payload.inputs_bit_width[i], payload.inputs_address_byte[i]});
        }
    }

    bool use_pipeline = config_.pipeline && payload.pipelined;

    return {.ins = payload.ins,
            .vector_inputs = vector_inputs,
            .scalar_inputs = scalar_inputs,
            .output = output,
            .functor_config = functor,
            .use_pipeline = use_pipeline};
}

}  // namespace pimsim
