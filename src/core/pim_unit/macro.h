//
// Created by wyk on 2024/7/20.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "pim_payload.h"

namespace pimsim {

class Macro : public BaseModule {
public:
    SC_HAS_PROCESS(Macro);

    Macro(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
          bool independent_ipu);

    void startExecute(MacroPayload payload);
    void waitUntilFinishIfBusy();

    EnergyReporter getEnergyReporter() override;

    static void waitAndStartNextSubmodule(const MacroSubmodulePayload& cur_payload,
                                          SubmoduleSocket<MacroSubmodulePayload>& next_submodule_socket);

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processIPUSubmodule();
    [[noreturn]] void processSRAMSubmodule();
    [[noreturn]] void processPostProcessSubmodule();
    [[noreturn]] void processAdderTreeSubmodule1();
    [[noreturn]] void processAdderTreeSubmodule2();
    [[noreturn]] void processShiftAdderSubmodule();

    static int getBatchCount(const MacroPayload& payload, int valid_input_cnt);

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;
    bool independent_ipu_;

    SubmoduleSocket<MacroPayload> macro_socket_{};

    sc_core::sc_event next_batch_;
    SubmoduleSocket<MacroSubmodulePayload> ipu_socket_{};
    SubmoduleSocket<MacroSubmodulePayload> sram_socket_{};
    SubmoduleSocket<MacroSubmodulePayload> post_process_socket_{};
    SubmoduleSocket<MacroSubmodulePayload> adder_tree_socket_1_{};
    SubmoduleSocket<MacroSubmodulePayload> adder_tree_socket_2_{};
    SubmoduleSocket<MacroSubmodulePayload> shift_adder_socket_{};

    EnergyCounter ipu_energy_counter_;
    EnergyCounter sram_energy_counter_;
    EnergyCounter post_process_energy_counter_;
    EnergyCounter adder_tree_energy_counter_;
    EnergyCounter shift_adder_energy_counter_;
};

}  // namespace pimsim
