//
// Created by wyk on 2024/7/19.
//

#pragma once
#include <array>
#include <unordered_map>
#include <vector>

#include "base_component/base_module.h"
#include "base_component/register.h"
#include "config/config.h"
#include "core/payload/payload.h"

namespace pimsim {

class RegUnit : public BaseModule {
public:
    SC_HAS_PROCESS(RegUnit);

    RegUnit(const char* name, const RegisterUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    int getSpecialBoundGeneralId(int special_id) const;

    void writeRegister(const RegUnitWriteRequest& write_req);

    int readRegister(int id, bool special = false);

private:
    void readValue();
    void writeValue();
    void updateValue();

    int readGeneralRegValue(int id, const RegUnitWriteRequest& cur_write_req,
                            const RegUnitWriteRequest& last_write_req) const;

    int readSpecialRegValue(int id, const RegUnitWriteRequest& cur_write_req,
                            const RegUnitWriteRequest& last_write_req) const;

public:
    sc_core::sc_in<RegUnitReadRequest> read_req_port_;
    sc_core::sc_out<RegUnitReadResponse> read_rsp_port_;

    sc_core::sc_in<RegUnitWriteRequest> write_req_port_;

private:
    const RegisterUnitConfig& config_;
    std::unordered_map<int, int> special_bind_map_{};

    std::array<int, GENERAL_REG_NUM> general_regs_{};
    std::array<int, SPECIAL_REG_NUM> special_regs_{};

    Register<RegUnitWriteRequest> write_req_reg_;
    sc_core::sc_signal<RegUnitWriteRequest> write_req_reg_in_;
    sc_core::sc_signal<RegUnitWriteRequest> write_req_reg_out_;
};

}  // namespace pimsim
