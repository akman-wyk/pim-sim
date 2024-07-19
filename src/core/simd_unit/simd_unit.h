//
// Created by wyk on 2024/7/5.
//

#pragma once
#include <string>
#include <unordered_map>
#include <utility>

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/payload.h"
#include "systemc.h"

namespace pimsim {

class LocalMemoryUnit;

struct SIMDInputOutputInfo {
    int data_bit_width{0};
    int start_address_byte{0};
};

struct SIMDInstructionInfo {
    InstructionPayload ins{};

    std::vector<SIMDInputOutputInfo> scalar_inputs{};
    std::vector<SIMDInputOutputInfo> vector_inputs{};
    SIMDInputOutputInfo output{};

    const SIMDFunctorConfig* functor_config{nullptr};
    bool use_pipeline{false};
};

struct SIMDBatchInfo {
    int batch_vector_len{0};
    int batch_num{0};
    bool first_batch{false};
    bool last_batch{false};
};

struct SIMDSubmodulePayload {
    SIMDInstructionInfo ins_info;
    SIMDBatchInfo batch_info;
};

class SIMDUnit : public BaseModule {
public:
    SC_HAS_PROCESS(SIMDUnit);

    SIMDUnit(const char* name, const SIMDUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadSubmodule();
    [[noreturn]] void processExecuteSubmodule();
    [[noreturn]] void processWriteSubmodule();
    void finishInstruction();

    void checkSIMDInst();

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

private:
    static unsigned int getSIMDInstructionIdentityCode(unsigned int input_cnt, unsigned int opcode);

    std::pair<const SIMDInstructionConfig*, const SIMDFunctorConfig*> getSIMDInstructionAndFunctor(
        const SIMDInsPayload& payload);

    std::pair<SIMDInstructionInfo, DataConflictPayload> decodeAndGetInfo(const SIMDInstructionConfig* instruction,
                                                                           const SIMDFunctorConfig* functor,
                                                                           const SIMDInsPayload& payload) const;

public:
    sc_core::sc_in<SIMDInsPayload> id_simd_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;

    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_pc_port_;

    sc_core::sc_out<bool> finish_run_port_;

private:
    const SIMDUnitConfig& config_;
    std::unordered_map<unsigned int, const SIMDInstructionConfig*> instruction_config_map_;
    std::unordered_map<std::string, const SIMDFunctorConfig*> functor_config_map_;

    FSM<SIMDInsPayload> simd_fsm_;
    sc_core::sc_signal<SIMDInsPayload> simd_fsm_out_;
    sc_core::sc_signal<FSMPayload<SIMDInsPayload>> simd_fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event cur_ins_next_batch_;
    SubmoduleSocket<SIMDSubmodulePayload> read_submodule_socket_{};
    SubmoduleSocket<SIMDSubmodulePayload> execute_submodule_socket_{};
    SubmoduleSocket<SIMDSubmodulePayload> write_submodule_socket_{};

    sc_core::sc_event finish_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};
};

}  // namespace pimsim
