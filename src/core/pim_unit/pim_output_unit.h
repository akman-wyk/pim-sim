//
// Created by wyk on 2024/7/31.
//

#pragma once

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "config/config.h"
#include "core/payload/payload.h"
#include "base_component/submodule_socket.h"

namespace pimsim {

class PimOutputUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimOutputUnit);

    PimOutputUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    EnergyReporter getEnergyReporter() override;

private:
    void checkPimOutputInst();

    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();
    void processOnlyOutput(const PimOutputInsPayload& payload);
    void processOutputSum(const PimOutputInsPayload& payload);
    void processOutputSumMove(const PimOutputInsPayload& payload);

    void finishInstruction();
    void finishRun();

public:
    sc_core::sc_in<PimOutputInsPayload> id_pim_output_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;

    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_pc_port_;

    sc_core::sc_out<bool> finish_run_port_;

private:
    const PimUnitConfig& config_;

    SubmoduleSocket<PimOutputInsPayload> execute_socket_;

    FSM<PimOutputInsPayload> fsm_;
    sc_core::sc_signal<PimOutputInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimOutputInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};

    EnergyCounter result_adder_energy_counter_;
};

}  // namespace pimsim
