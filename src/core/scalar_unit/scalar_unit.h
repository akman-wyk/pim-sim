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
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/payload/execute_unit_payload.h"
#include "core/payload/payload.h"

namespace pimsim {

class ScalarUnit : public BaseModule {
public:
    SC_HAS_PROCESS(ScalarUnit);

    ScalarUnit(const char* name, const ScalarUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);
    void bindRegUnit(RegUnit* reg_unit);

private:
    [[noreturn]] void process();
    [[noreturn]] void executeInst();
    RegUnitWriteRequest executeAndWriteRegister(const ScalarInsPayload& payload);

    void finishInstruction();
    void finishRun();

    void checkScalarInst();

public:
    ExecuteUnitResponseIOPorts<ScalarInsPayload> ports_;

private:
    const ScalarUnitConfig& config_;
    std::unordered_map<std::string, const ScalarFunctorConfig*> functor_config_map_;

    SubmoduleSocket<ScalarInsPayload> execute_socket_;

    FSM<ScalarInsPayload> scalar_fsm_;
    sc_core::sc_signal<ScalarInsPayload> scalar_fsm_out_;
    sc_core::sc_signal<FSMPayload<ScalarInsPayload>> scalar_fsm_in_;

    MemorySocket local_memory_socket_;
    RegUnitSocket reg_unit_socket_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_id_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};
};

}  // namespace pimsim
