//
// Created by wyk on 2024/8/2.
//

#include <vector>

#include "base_component/base_module.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/payload/payload.h"
#include "core/pim_unit/pim_compute_unit.h"
#include "core/pim_unit/pim_set_unit.h"
#include "fmt/format.h"
#include "test_macro.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

struct PimSetTestExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};

    std::vector<int> groups_activation_macro_cnt{};
    std::vector<int> groups_activation_element_col_cnt{};
};

struct PimSetTestInstruction {
    PimSetInsPayload payload;
};

struct PimSetTestInfo {
    std::vector<PimSetTestInstruction> code{};
    PimSetTestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetTestExpectedInfo, time_ns, energy_pj, groups_activation_macro_cnt,
                                               groups_activation_element_col_cnt)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetInsPayload, ins, group_broadcast, group_id, mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetTestInfo, code, expected)

class PimSetUnitTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(PimSetUnitTestModule);

    PimSetUnitTestModule(const char* name, const Config& config, Clock* clk, std::vector<PimSetTestInstruction> codes)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , local_memory_unit_("LocalMemoryUnit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, clk)
        , pim_set_unit_("PimSetUnit", config.chip_config.core_config.pim_unit_config, config.sim_config, nullptr, clk)
        , pim_compute_unit_("PimComputeUnit", config.chip_config.core_config.pim_unit_config, config.sim_config,
                            nullptr, clk) {
        pim_set_unit_.id_pim_set_payload_port_.bind(decode_pim_set_payload_);
        pim_set_unit_.id_ex_enable_port_.bind(id_ex_enable_);
        pim_set_unit_.busy_port_.bind(pim_set_busy_);
        pim_set_unit_.data_conflict_port_.bind(pim_set_data_conflict_);
        pim_set_unit_.finish_ins_port_.bind(pim_set_finish_ins_);
        pim_set_unit_.finish_ins_pc_port_.bind(pim_set_finish_ins_pc_);
        pim_set_unit_.finish_run_port_.bind(pim_set_finish_run_);

        pim_compute_unit_.id_pim_compute_payload_port_.bind(decode_pim_compute_payload_);
        pim_compute_unit_.id_ex_enable_port_.bind(id_ex_enable_2_);
        pim_compute_unit_.busy_port_.bind(pim_compute_busy_);
        pim_compute_unit_.data_conflict_port_.bind(pim_compute_data_conflict_);
        pim_compute_unit_.finish_ins_port_.bind(pim_compute_finish_ins_);
        pim_compute_unit_.finish_ins_pc_port_.bind(pim_compute_finish_ins_pc_);
        pim_compute_unit_.finish_run_port_.bind(pim_compute_finish_run_);

        SC_THREAD(issue)

        SC_METHOD(processStall)
        sensitive << pim_set_data_conflict_ << pim_set_busy_ << pim_set_finish_ins_ << pim_set_finish_ins_pc_;

        SC_METHOD(processIdExEnable)
        sensitive << id_stall_;

        SC_METHOD(processFinishIns)
        sensitive << pim_set_finish_ins_ << pim_set_finish_ins_pc_;

        SC_METHOD(processFinishRun)
        sensitive << pim_set_finish_run_;

        pim_set_unit_.bindLocalMemoryUnit(&local_memory_unit_);
        pim_set_unit_.bindPimComputeUnit(&pim_compute_unit_);
        pim_set_ins_list_ = std::move(codes);
        pim_set_unit_.setEndPC(static_cast<int>(pim_set_ins_list_.size()));
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1000, "PimSetUnitTestModule", getEnergyReporter(), 0};
    }

    bool checkTestResult(const PimSetTestExpectedInfo& expected) {
        for (int group_id = 0; group_id < expected.groups_activation_element_col_cnt.size(); group_id++) {
            if (pim_compute_unit_.getMacroGroupActivationElementColumnCount(group_id) !=
                expected.groups_activation_element_col_cnt[group_id]) {
                std::cout << fmt::format("activation element col cnt error, group id: {}, expected: {}, actual: {}",
                                         group_id, expected.groups_activation_element_col_cnt[group_id],
                                         pim_compute_unit_.getMacroGroupActivationElementColumnCount(group_id))
                          << std::endl;
                return false;
            }
        }
        for (int group_id = 0; group_id < expected.groups_activation_macro_cnt.size(); group_id++) {
            if (pim_compute_unit_.getMacroGroupActivationMacroCount(group_id) !=
                expected.groups_activation_macro_cnt[group_id]) {
                std::cout << fmt::format("activation macro cnt error, group id: {}, expected: {}, actual: {}",
                                         group_id, expected.groups_activation_macro_cnt[group_id],
                                         pim_compute_unit_.getMacroGroupActivationMacroCount(group_id))
                          << std::endl;
                return false;
            }
        }
        return true;
    }

private:
    void issue() {
        wait(8, SC_NS);

        while (ins_index_ < pim_set_ins_list_.size()) {
            decode_pim_set_payload_.write(pim_set_ins_list_[ins_index_].payload);
            ins_index_++;
            wait(next_ins_);
        }
        PimSetInsPayload nop{};
        decode_pim_set_payload_.write(nop);
    }

    void processStall() {
        const auto& pim_set_conflict_payload = pim_set_data_conflict_.read();
        auto ins_conflict_payload = getInsPayloadConflictInfos(pim_set_ins_list_[ins_index_].payload);

        bool pim_set_busy = pim_set_busy_.read();
        bool pim_set_finish = pim_set_finish_ins_.read();
        int pim_set_finish_pc = pim_set_finish_ins_pc_.read();

        bool stall = DataConflictPayload::checkDataConflict(ins_conflict_payload, pim_set_conflict_payload, true)
                         ? (!pim_set_finish || pim_set_finish_pc != pim_set_conflict_payload.pc)
                         : pim_set_busy;
        id_stall_.write(stall);
        if (!stall) {
            next_ins_.notify();
        }
    }

    void processIdExEnable() {
        id_ex_enable_.write(!id_stall_.read());
    }

    void processFinishIns() {
        if (pim_set_finish_ins_.read()) {
            LOG(fmt::format("pim set ins finish, pc: {}", pim_set_finish_ins_pc_.read()));
        }
    }

    void processFinishRun() {
        if (pim_set_finish_run_.read()) {
            running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        }
    }

    DataConflictPayload getInsPayloadConflictInfos(const PimSetInsPayload& ins_payload) {
        DataConflictPayload conflict_payload{.pc = ins_payload.ins.pc};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addReadMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.mask_addr_byte));
        return std::move(conflict_payload);
    }

private:
    // instruction list
    std::vector<PimSetTestInstruction> pim_set_ins_list_;
    int ins_index_{0};

    // modules
    LocalMemoryUnit local_memory_unit_;
    PimSetUnit pim_set_unit_;
    PimComputeUnit pim_compute_unit_;

    // port from instruction decode
    sc_core::sc_signal<PimSetInsPayload> decode_pim_set_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> id_stall_;

    // port about busy, finish and data conflict
    sc_core::sc_signal<bool> pim_set_busy_;
    sc_core::sc_signal<DataConflictPayload> pim_set_data_conflict_;
    sc_core::sc_signal<bool> pim_set_finish_ins_;
    sc_core::sc_signal<int> pim_set_finish_ins_pc_;
    sc_core::sc_signal<bool> pim_set_finish_run_;

    // port from instruction decode
    sc_core::sc_signal<PimComputeInsPayload> decode_pim_compute_payload_;
    sc_core::sc_signal<bool> id_ex_enable_2_;
    sc_core::sc_signal<bool> id_stall_2_;

    // port about busy, finish and data conflict
    sc_core::sc_signal<bool> pim_compute_busy_;
    sc_core::sc_signal<DataConflictPayload> pim_compute_data_conflict_;
    sc_core::sc_signal<bool> pim_compute_finish_ins_;
    sc_core::sc_signal<int> pim_compute_finish_ins_pc_;
    sc_core::sc_signal<bool> pim_compute_finish_run_;

    sc_core::sc_event next_ins_;
    sc_core::sc_time running_time_;
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 4) {
        std::cout << "Usage: ./PimSetUnitTest [config_file] [instruction_file] [report_file]" << std::endl;
        return INVALID_USAGE;
    }

    auto* config_file = argv[1];
    auto* instruction_file = argv[2];
    auto* report_file = argv[3];

    std::ifstream config_ifs;
    config_ifs.open(config_file);
    nlohmann::ordered_json config_j = nlohmann::ordered_json::parse(config_ifs);
    config_ifs.close();
    auto config = config_j.get<Config>();
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return INVALID_CONFIG;
    }

    std::ifstream ins_ifs;
    ins_ifs.open(instruction_file);
    nlohmann::ordered_json ins_j = nlohmann::ordered_json::parse(ins_ifs);
    ins_ifs.close();
    auto test_info = ins_j.get<PimSetTestInfo>();

    Clock clk{"clock", config.sim_config.period_ns};
    PimSetUnitTestModule test_module{"PimSetUnitTestModule", config, &clk, std::move(test_info.code)};
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = test_module.getReporter();
    reporter.report(ofs);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getTotalEnergyPJ(), test_info.expected.energy_pj) &&
        test_module.checkTestResult(test_info.expected)) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}
