//
// Created by wyk on 2024/7/24.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "config/config.h"
#include "macro.h"
#include "macro_group_controller.h"

namespace pimsim {

class MacroGroup : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroup);

    MacroGroup(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
               SubmoduleSocket<PimWriteOutputPayload>& write_output_socket);

    void startExecute(MacroGroupPayload payload);
    void waitUntilFinishIfBusy();

    EnergyReporter getEnergyReporter() override;

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processResultAdderSubmodule();

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    MacroGroupController controller_;
    std::vector<Macro*> macro_list_;

    SubmoduleSocket<MacroGroupPayload> macro_group_socket_{};
    SubmoduleSocket<MacroGroupSubmodulePayload> result_adder_socket_{};
    SubmoduleSocket<PimWriteOutputPayload>& write_output_socket_;

    sc_core::sc_event next_sub_ins_;

    EnergyCounter result_adder_energy_counter_;
};

}  // namespace pimsim
