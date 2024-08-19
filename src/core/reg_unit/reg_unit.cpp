//
// Created by wyk on 2024/7/19.
//

#include "reg_unit.h"

#include "fmt/format.h"

namespace pimsim {

RegUnit::RegUnit(const char *name, const pimsim::RegisterUnitConfig &config, const pimsim::SimConfig &sim_config,
                 pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), write_req_reg_("RegUnitWriteRequestReg", clk) {
    write_req_reg_.input_.bind(write_req_reg_in_);
    write_req_reg_.output_.bind(write_req_reg_out_);

    SC_METHOD(readValue)
    sensitive << read_req_port_ << write_req_port_ << write_req_reg_out_;

    SC_METHOD(writeValue)
    sensitive << write_req_port_;

    SC_METHOD(updateValue)
    sensitive << write_req_reg_out_;

    for (const auto &[special, general] : config_.special_register_binding) {
        special_bind_map_.emplace(special, general);
    }

    energy_counter_.setStaticPowerMW(config_.static_power_mW);
}

int RegUnit::getSpecialBoundGeneralId(int special_id) const {
    auto found = special_bind_map_.find(special_id);
    if (found == special_bind_map_.end()) {
        return -1;
    }
    return found->second;
}

void RegUnit::writeRegister(const pimsim::RegUnitWriteRequest &write_req) {
    if (write_req.write_special_register) {
        special_regs_[write_req.reg_id] = write_req.reg_value;
    } else {
        general_regs_[write_req.reg_id] = write_req.reg_value;
    }
}

int RegUnit::readRegister(int id, bool special) {
    if (!special) {
        return general_regs_[id];
    }

    int special_bound_general_id = getSpecialBoundGeneralId(id);
    if (special_bound_general_id != -1) {
        return general_regs_[special_bound_general_id];
    }
    return special_regs_[id];
}

bool RegUnit::checkRegValues(const std::array<int, GENERAL_REG_NUM> &general_reg_expected_values,
                             const std::array<int, SPECIAL_REG_NUM> &special_reg_expected_values) {
    for (int i = 0; i < GENERAL_REG_NUM; i++) {
        if (general_regs_[i] != general_reg_expected_values[i]) {
            std::cout << fmt::format("index: {}, actual: {}, expected: {}", i, general_regs_[i],
                                     general_reg_expected_values[i])
                      << std::endl;
            return false;
        }
    }

    for (int i = 0; i < SPECIAL_REG_NUM; i++) {
        if (special_regs_[i] != special_reg_expected_values[i]) {
            std::cout << fmt::format("index: {}, actual: {}, expected: {}", i, special_regs_[i],
                                     special_reg_expected_values[i])
                      << std::endl;
            return false;
        }
    }
    return true;
}

std::string RegUnit::getGeneralRegistersString() const {
    std::stringstream ss;
    for (int i = 0; i < GENERAL_REG_NUM; i++) {
        ss << general_regs_[i];
        if (i != GENERAL_REG_NUM - 1) {
            ss << ", ";
        }
    }
    return ss.str();
}

void RegUnit::readValue() {
    const auto &read_req = read_req_port_.read();
    const auto &cur_write_req = write_req_port_.read();
    const auto &last_write_req = write_req_reg_out_.read();

    RegUnitReadResponse read_rsp;
    // read general regs
    read_rsp.rs1_value = readGeneralRegValue(read_req.rs1_id, cur_write_req, last_write_req);
    read_rsp.rs2_value = readGeneralRegValue(read_req.rs2_id, cur_write_req, last_write_req);
    read_rsp.rs3_value = readGeneralRegValue(read_req.rs3_id, cur_write_req, last_write_req);
    read_rsp.rs4_value = readGeneralRegValue(read_req.rs4_id, cur_write_req, last_write_req);
    read_rsp.rd_value = readGeneralRegValue(read_req.rd_id, cur_write_req, last_write_req);
    if (read_req.rs1_read_double) {
        read_rsp.rs1_double_value = readGeneralRegValue(read_req.rs1_id + 1, cur_write_req, last_write_req);
    }
    if (read_req.rs2_read_double) {
        read_rsp.rs2_double_value = readGeneralRegValue(read_req.rs2_id + 1, cur_write_req, last_write_req);
    }

    // read special regs
    for (const auto &special_reg_id : read_req.special_reg_ids) {
        int special_reg_value = readSpecialRegValue(special_reg_id, cur_write_req, last_write_req);
        read_rsp.special_reg_values.emplace(special_reg_id, special_reg_value);
    }

    read_rsp_port_.write(read_rsp);
    energy_counter_.addDynamicEnergyPJ(period_ns_, config_.dynamic_power_mW);
}

void RegUnit::writeValue() {
    const auto &write_req = write_req_port_.read();
    write_req_reg_in_.write(write_req);
    energy_counter_.addDynamicEnergyPJ(period_ns_, config_.dynamic_power_mW);
}

void RegUnit::updateValue() {
    const auto &last_write_req = write_req_reg_out_.read();
    if (last_write_req.write_special_register) {
        special_regs_[last_write_req.reg_id] = last_write_req.reg_value;
    } else {
        general_regs_[last_write_req.reg_id] = last_write_req.reg_value;
    }
}

int RegUnit::readGeneralRegValue(int id, const pimsim::RegUnitWriteRequest &cur_write_req,
                                 const pimsim::RegUnitWriteRequest &last_write_req) const {
    if (!cur_write_req.write_special_register && id == cur_write_req.reg_id) {
        return cur_write_req.reg_value;
    }
    if (!last_write_req.write_special_register && id == last_write_req.reg_id) {
        return last_write_req.reg_value;
    }
    return general_regs_[id];
}

int RegUnit::readSpecialRegValue(int id, const pimsim::RegUnitWriteRequest &cur_write_req,
                                 const pimsim::RegUnitWriteRequest &last_write_req) const {
    int special_bound_general_id = getSpecialBoundGeneralId(id);
    if (special_bound_general_id != -1) {
        return readGeneralRegValue(special_bound_general_id, cur_write_req, last_write_req);
    }
    if (cur_write_req.write_special_register && id == cur_write_req.reg_id) {
        return cur_write_req.reg_value;
    }
    if (last_write_req.write_special_register && id == last_write_req.reg_id) {
        return last_write_req.reg_value;
    }
    return special_regs_[id];
}

}  // namespace pimsim
