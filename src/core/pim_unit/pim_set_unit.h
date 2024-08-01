//
// Created by wyk on 2024/8/1.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/payload.h"

namespace pimsim {

class PimComputeUnit;

class PimSetUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimSetUnit);

    PimSetUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    void bindPimComputeUnit(PimComputeUnit* pim_compute_unit);

private:
    void checkPimSetInst();

    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();

    void finishInstruction();
    void finishRun();

public:
    sc_core::sc_in<PimSetInsPayload> id_pim_set_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;

    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_pc_port_;

    sc_core::sc_out<bool> finish_run_port_;

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    SubmoduleSocket<PimSetInsPayload> execute_socket_;

    FSM<PimSetInsPayload> fsm_;
    sc_core::sc_signal<PimSetInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimSetInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;
    PimComputeUnit* pim_compute_unit_{nullptr};

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};
};

}  // namespace pimsim
