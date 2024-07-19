//
// Created by wyk on 2024/7/19.
//

#pragma once
#include <string>
#include <unordered_map>

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/reg_unit_socket.h"
#include "config/config.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/payload/payload.h"

namespace pimsim {

class ScalarUnit : public BaseModule {
public:
    SC_HAS_PROCESS(ScalarUnit);

    ScalarUnit(const char* name, const ScalarUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    [[noreturn]] void process();

    void finishInstruction();

    void checkScalarInst();

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);
    void bindRegUnit(RegUnit* reg_unit);

private:
    void executeInst(const ScalarInsPayload& payload);

public:
    sc_core::sc_in<ScalarInsPayload> id_scalar_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<RegUnitWriteRequest> reg_file_write_port_;

    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;
    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_pc_port_;
    sc_core::sc_out<bool> finish_run_port_;

private:
    const ScalarUnitConfig& config_;
    std::unordered_map<std::string, const ScalarFunctorConfig*> functor_config_map_;

    FSM<ScalarInsPayload> scalar_fsm_;
    sc_core::sc_signal<ScalarInsPayload> scalar_fsm_out_;
    sc_core::sc_signal<FSMPayload<ScalarInsPayload>> scalar_fsm_in_;

    MemorySocket local_memory_socket_;
    RegUnitSocket reg_unit_socket_;

    sc_core::sc_event finish_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};
};

}  // namespace pimsim
