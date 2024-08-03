//
// Created by wyk on 2024/7/31.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/execute_unit_payload.h"
#include "core/payload/payload.h"

namespace pimsim {

class PimTransferUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimTransferUnit);

    PimTransferUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

private:
    void checkPimTransferInst();

    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();

    void finishInstruction();
    void finishRun();

public:
    ExecuteUnitResponseIOPorts<PimTransferInsPayload> ports_;

private:
    const PimUnitConfig& config_;

    SubmoduleSocket<PimTransferInsPayload> execute_socket_;

    FSM<PimTransferInsPayload> fsm_;
    sc_core::sc_signal<PimTransferInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimTransferInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};
};

}  // namespace pimsim
