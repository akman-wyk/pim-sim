//
// Created by wyk on 2024/7/29.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/payload.h"
#include "macro_group.h"

namespace pimsim {

struct PimComputeReadDataPayload {
    InstructionPayload ins{};
    int addr_byte{0};
    int size_byte{0};
    std::vector<unsigned char> data{};
};

struct PimComputeSubInsPayload {
    PimInsInfo pim_ins_info{};
    const PimComputeInsPayload& ins_payload;
    int activation_macro_cnt{};
};

class PimComputeUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimComputeUnit);

    PimComputeUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    EnergyReporter getEnergyReporter() override;

private:
    void checkPimComputeInst();

    [[noreturn]] void processIssue();
    void processSubInsReadData(const PimComputeSubInsPayload& sub_ins_payload);
    void processSubInsCompute(const PimComputeSubInsPayload& sub_ins_payload);

    [[noreturn]] void readValueSparseMaskSubmodule();
    [[noreturn]] void readBitSparseMetaSubmodule();

    void finishInstruction();
    void finishRun();

    std::vector<std::vector<unsigned long long>> getMacroGroupInputs(int addr_byte, int size_byte,
                                                                     const PimComputeSubInsPayload& sub_ins_payload);

public:
    sc_core::sc_in<PimComputeInsPayload> id_pim_compute_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;

    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_pc_port_;

    sc_core::sc_out<bool> finish_run_port_;

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    std::vector<MacroGroup*> macro_group_list_;

    SubmoduleSocket<PimComputeReadDataPayload> read_value_sparse_mask_socket_;
    SubmoduleSocket<PimComputeReadDataPayload> read_bit_sparse_meta_socket_;

    FSM<PimComputeInsPayload> fsm_;
    sc_core::sc_signal<PimComputeInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimComputeInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};

    EnergyCounter value_sparse_network_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
};

}  // namespace pimsim