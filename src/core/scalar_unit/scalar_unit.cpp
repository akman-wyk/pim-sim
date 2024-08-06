//
// Created by wyk on 2024/7/19.
//

#include "scalar_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

ScalarUnit::ScalarUnit(const char *name, const pimsim::ScalarUnitConfig &config, const pimsim::SimConfig &sim_config,
                       pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), scalar_fsm_("ScalarUnitFSM", clk) {
    scalar_fsm_.input_.bind(scalar_fsm_in_);
    scalar_fsm_.enable_.bind(ports_.id_ex_enable_port_);
    scalar_fsm_.output_.bind(scalar_fsm_out_);

    SC_METHOD(checkScalarInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_THREAD(process)
    SC_THREAD(executeInst)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;

    double scalar_functors_total_static_power_mW = config_.default_functor_static_power_mW;
    for (const auto &scalar_functor_config : config_.functor_list) {
        scalar_functors_total_static_power_mW += scalar_functor_config.static_power_mW;
        functor_config_map_.emplace(scalar_functor_config.inst_name, &scalar_functor_config);
    }
    energy_counter_.setStaticPowerMW(scalar_functors_total_static_power_mW);
}

void ScalarUnit::checkScalarInst() {
    if (const auto &payload = ports_.id_ex_payload_port_.read(); payload.ins.valid()) {
        scalar_fsm_in_.write({payload, true});
    } else {
        scalar_fsm_in_.write({{}, false});
    }
}

void ScalarUnit::process() {
    while (true) {
        wait(scalar_fsm_.start_exec_);

        ports_.busy_port_.write(true);

        const auto &payload = scalar_fsm_out_.read();
        DataConflictPayload conflict_payload{
            .pc = payload.ins.pc,
            .unit_type = ExecuteUnitType::scalar,
            .write_reg_id = (payload.op == +ScalarOperator::load ? payload.dst_reg : -1)};
        ports_.data_conflict_port_.write(conflict_payload);

        LOG(fmt::format("scalar {} start, pc: {}", payload.op._to_string(), payload.ins.pc));

        // statistic energy
        auto functor_found = functor_config_map_.find(payload.op._to_string());
        double dynamic_power_mW = functor_found == functor_config_map_.end() ? config_.default_functor_dynamic_power_mW
                                                                             : functor_found->second->dynamic_power_mW;
        energy_counter_.addDynamicEnergyPJ(period_ns_, dynamic_power_mW);

        // execute instruction
        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = payload;
        execute_socket_.start_exec.notify();

        ports_.busy_port_.write(false);
        scalar_fsm_.finish_exec_.notify();
    }
}

void ScalarUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_pc_port_.write(finish_ins_pc_);
}

void ScalarUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

void ScalarUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void ScalarUnit::bindRegUnit(pimsim::RegUnit *reg_unit) {
    reg_unit_socket_.bindRegUnit(reg_unit);
}

void ScalarUnit::executeInst() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        LOG(fmt::format("Scalar start execute, pc: {}", payload.ins.pc));

        if (payload.op == +ScalarOperator::store) {
            finish_ins_ = true;
            finish_ins_pc_ = payload.ins.pc;
            finish_ins_trigger_.notify(SC_ZERO_TIME);

            int address_byte = payload.src1_value + payload.offset;
            int size_byte = WORD_BYTE_SIZE;
            auto write_data = IntToBytes(payload.src2_value, true);
            local_memory_socket_.writeData(payload.ins, address_byte, size_byte, std::move(write_data));
        } else {
            reg_file_write_port_.write(executeAndWriteRegister(payload));

            finish_ins_ = true;
            finish_ins_pc_ = payload.ins.pc;
            finish_ins_trigger_.notify(SC_ZERO_TIME);
        }

        // check if last pc
        if (isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            finish_run_ = true;
            if (payload.op == +ScalarOperator::store) {
                finish_run_trigger_.notify(SC_ZERO_TIME);
            } else {
                finish_run_trigger_.notify(period_ns_, SC_NS);
            }
        }

        execute_socket_.finish();
    }
}

RegUnitWriteRequest ScalarUnit::executeAndWriteRegister(const pimsim::ScalarInsPayload &payload) {
    RegUnitWriteRequest reg_file_write_req{.reg_id = payload.dst_reg, .write_special_register = false};
    switch (payload.op) {
        case ScalarOperator::add: {
            reg_file_write_req.reg_value = payload.src1_value + payload.src2_value;
            break;
        }
        case ScalarOperator::sub: {
            reg_file_write_req.reg_value = payload.src1_value - payload.src2_value;
            break;
        }
        case ScalarOperator::mul: {
            reg_file_write_req.reg_value = payload.src1_value * payload.src2_value;
            break;
        }
        case ScalarOperator::div: {
            reg_file_write_req.reg_value = payload.src1_value / payload.src2_value;
            break;
        }
        case ScalarOperator::sll: {
            reg_file_write_req.reg_value = (payload.src1_value << payload.src2_value);
            break;
        }
        case ScalarOperator::srl: {
            unsigned int result =
                (static_cast<unsigned int>(payload.src1_value) >> static_cast<unsigned int>(payload.src2_value));
            reg_file_write_req.reg_value = static_cast<int>(result);
            break;
        }
        case ScalarOperator::sra: {
            reg_file_write_req.reg_value = (payload.src1_value >> payload.src2_value);
            break;
        }
        case ScalarOperator::mod: {
            reg_file_write_req.reg_value = payload.src1_value % payload.src2_value;
            break;
        }
        case ScalarOperator::min: {
            reg_file_write_req.reg_value = std::min(payload.src1_value, payload.src2_value);
            break;
        }
        case ScalarOperator::lui: {
            reg_file_write_req.reg_value = (payload.src2_value << 16);
            break;
        }
        case ScalarOperator::load: {
            int address_byte = payload.src1_value + payload.offset;
            int size_byte = WORD_BYTE_SIZE;
            auto read_result = local_memory_socket_.readData(payload.ins, address_byte, size_byte);
            reg_file_write_req.reg_value = BytesToInt(read_result, true);
            break;
        }
        case ScalarOperator::assign: {
            reg_file_write_req.reg_value = payload.src1_value;
            reg_file_write_req.write_special_register = payload.write_special_register;
            if (payload.write_special_register) {
                int special_bound_general_id = reg_unit_socket_.getSpecialBoundGeneralId(reg_file_write_req.reg_id);
                if (special_bound_general_id != -1) {
                    reg_file_write_req.reg_id = special_bound_general_id;
                    reg_file_write_req.write_special_register = false;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    return reg_file_write_req;
}

}  // namespace pimsim
