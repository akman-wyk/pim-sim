//
// Created by wyk on 2024/7/9.
//

#include <fstream>
#include <vector>

#include "base_component/base_module.h"
#include "base_component/clock.h"
#include "config/config.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/payload/payload.h"
#include "core/simd_unit/simd_unit.h"
#include "fmt/core.h"
#include "test_macro.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

class SIMDUnitTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(SIMDUnitTestModule);

    SIMDUnitTestModule(const char* name, const Config& config, Clock* clk, std::vector<SIMDInsPayload> codes)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , local_memory_unit_("local_memory_unit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, clk)
        , simd_unit_("SIMD_unit", config.chip_config.core_config.simd_unit_config, config.sim_config, nullptr, clk) {
        // bind ports
        simd_unit_.id_simd_payload_port_.bind(decode_simd_payload_);
        simd_unit_.id_ex_enable_port_.bind(id_ex_enable_);
        simd_unit_.busy_port_.bind(simd_busy_);
        simd_unit_.data_conflict_port_.bind(simd_data_conflict_);
        simd_unit_.finish_ins_port_.bind(simd_finish_ins_);
        simd_unit_.finish_ins_pc_port_.bind(simd_finish_ins_pc_);
        simd_unit_.finish_run_port_.bind(simd_finish_run_);

        SC_THREAD(issue)

        SC_METHOD(processStall)
        sensitive << simd_data_conflict_ << simd_busy_ << simd_finish_ins_ << simd_finish_ins_pc_;

        SC_METHOD(processIdExEnable)
        sensitive << id_stall_;

        SC_METHOD(processFinishIns)
        sensitive << simd_finish_ins_ << simd_finish_ins_pc_;

        SC_METHOD(processFinishRun)
        sensitive << simd_finish_run_;

        simd_unit_.bindLocalMemoryUnit(&local_memory_unit_);
        simd_ins_list_ = std::move(codes);
        simd_unit_.setEndPC(static_cast<int>(simd_ins_list_.size()));
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(simd_unit_.getName(), simd_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1000, "SIMD_test_module", getEnergyReporter(), 0};
    }

private:
    void issue() {
        wait(8, SC_NS);

        while (ins_index_ < simd_ins_list_.size()) {
            decode_simd_payload_.write(simd_ins_list_[ins_index_]);
            ins_index_++;
            wait(next_ins_);
        }
        SIMDInsPayload simd_nop{};
        decode_simd_payload_.write(simd_nop);
    }

    void processStall() {
        const auto& simd_conflict_payload = simd_data_conflict_.read();
        auto ins_conflict_payload = getInsPayloadConflictInfos(simd_ins_list_[ins_index_]);

        bool simd_busy = simd_busy_.read();
        bool simd_finish = simd_finish_ins_.read();
        int simd_finish_pc = simd_finish_ins_pc_.read();

        bool stall = DataConflictPayload::checkDataConflict(ins_conflict_payload, simd_conflict_payload, true)
                         ? (!simd_finish || simd_finish_pc != simd_conflict_payload.pc)
                         : simd_busy;
        id_stall_.write(stall);
        if (!stall) {
            next_ins_.notify();
        }
    }

    void processIdExEnable() {
        id_ex_enable_.write(!id_stall_.read());
    }

    void processFinishIns() {
        if (simd_finish_ins_.read()) {
            LOG(fmt::format("simd ins finish, pc: {}", simd_finish_ins_pc_.read()));
        }
    }

    void processFinishRun() {
        if (simd_finish_run_.read()) {
            running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        }
    }

private:
    DataConflictPayload getInsPayloadConflictInfos(const SIMDInsPayload& ins_payload) const {
        DataConflictPayload conflict_payload{.pc = ins_payload.ins.pc};
        for (unsigned int i = 0; i < ins_payload.input_cnt; i++) {
            int read_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.inputs_address_byte[i]);
            conflict_payload.read_memory_id.insert(read_memory_id);
            conflict_payload.used_memory_id.insert(read_memory_id);
        }
        int write_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.output_address_byte);
        conflict_payload.write_memory_id.insert(write_memory_id);
        conflict_payload.used_memory_id.insert(write_memory_id);
        return std::move(conflict_payload);
    }

private:
    // instruction list
    std::vector<SIMDInsPayload> simd_ins_list_;
    int ins_index_{0};

    // modules
    LocalMemoryUnit local_memory_unit_;
    SIMDUnit simd_unit_;

    // port from instruction decode
    sc_core::sc_signal<SIMDInsPayload> decode_simd_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> id_stall_;

    // port about busy, finish and data conflict
    sc_core::sc_signal<bool> simd_busy_;
    sc_core::sc_signal<DataConflictPayload> simd_data_conflict_;
    sc_core::sc_signal<bool> simd_finish_ins_;
    sc_core::sc_signal<int> simd_finish_ins_pc_;
    sc_core::sc_signal<bool> simd_finish_run_;

    sc_core::sc_event next_ins_;
    sc_core::sc_time running_time_;
};

struct ExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ExpectedInfo)
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ExpectedInfo, time_ns, energy_pj)

struct TestInfo {
    std::vector<SIMDInsPayload> code{};
    ExpectedInfo expected{};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(TestInfo)
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TestInfo, code, expected)

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 4) {
        std::cout << "Usage: ./SIMDUnitTest [config_file] [instruction_file] [report_file]" << std::endl;
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
    auto test_info = ins_j.get<TestInfo>();

    Clock clk{"clock", config.sim_config.period_ns};
    SIMDUnitTestModule test_module{"SIMD_unit_test_module", config, &clk, std::move(test_info.code)};
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = test_module.getReporter();
    reporter.report(ofs);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getTotalEnergyPJ(), test_info.expected.energy_pj)) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}
