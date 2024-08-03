//
// Created by wyk on 2024/8/3.
//

#pragma once
#include <fstream>
#include <functional>
#include <vector>

#include "../base/test_macro.h"
#include "base_component/base_module.h"
#include "config/config.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/payload/execute_unit_payload.h"
#include "fmt/format.h"
#include "systemc.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

template <class TestUnitModule, class TestUnit, class TestUnitConfig, class InsPayload, class TestInstruction,
          class TestExpectedInfo, class TestInfo>
class ExecuteUnitTestModule : public BaseModule {
public:
    using TestInfoType = TestInfo;
    using TestBaseModule = ExecuteUnitTestModule<TestUnitModule, TestUnit, TestUnitConfig, InsPayload, TestInstruction,
                                                 TestExpectedInfo, TestInfo>;

    SC_HAS_PROCESS(TestUnitModule);

public:
    ExecuteUnitTestModule(const char* name, const char* test_unit_name, const TestUnitConfig& test_unit_config,
                          const Config& config, Clock* clk, std::vector<TestInstruction> codes)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , local_memory_unit_("LocalMemoryUnit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, clk)
        , test_unit_(test_unit_name, test_unit_config, config.sim_config, nullptr, clk) {
        test_unit_.ports_.bind(signals_);

        SC_THREAD(issue)

        SC_METHOD(processStall)
        sensitive << signals_.data_conflict_ << signals_.busy_ << signals_.finish_ins_ << signals_.finish_ins_pc_;

        SC_METHOD(processIdExEnable)
        sensitive << id_stall_;

        SC_METHOD(processFinishIns)
        sensitive << signals_.finish_ins_ << signals_.finish_ins_pc_;

        SC_METHOD(processFinishRun)
        sensitive << signals_.finish_run_;

        test_unit_.bindLocalMemoryUnit(&local_memory_unit_);
        ins_list_ = std::move(codes);
        test_unit_.setEndPC(static_cast<int>(ins_list_.size()));
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1000, getName(), getEnergyReporter(), 0};
    }

    virtual bool checkTestResult(const TestExpectedInfo& expected) {
        return true;
    }

private:
    void issue() {
        wait(8, SC_NS);

        while (ins_index_ < ins_list_.size()) {
            signals_.id_ex_payload_.write(ins_list_[ins_index_].payload);
            ins_index_++;
            wait(next_ins_);
        }
        InsPayload nop{};
        signals_.id_ex_payload_.write(nop);
    }

    void processStall() {
        const auto& pim_set_conflict_payload = signals_.data_conflict_.read();
        auto ins_conflict_payload = getInsPayloadConflictInfos(ins_list_[ins_index_].payload);

        bool pim_set_busy = signals_.busy_.read();
        bool pim_set_finish = signals_.finish_ins_.read();
        int pim_set_finish_pc = signals_.finish_ins_pc_.read();

        bool stall = DataConflictPayload::checkDataConflict(ins_conflict_payload, pim_set_conflict_payload, true)
                         ? (!pim_set_finish || pim_set_finish_pc != pim_set_conflict_payload.pc)
                         : pim_set_busy;
        id_stall_.write(stall);
        if (!stall) {
            next_ins_.notify();
        }
    }

    void processIdExEnable() {
        signals_.id_ex_enable_.write(!id_stall_.read());
    }

    void processFinishIns() {
        if (signals_.finish_ins_.read()) {
            LOG(fmt::format("pim set ins finish, pc: {}", signals_.finish_ins_pc_.read()));
        }
    }

    void processFinishRun() {
        if (signals_.finish_run_.read()) {
            running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        }
    }

    virtual DataConflictPayload getInsPayloadConflictInfos(const InsPayload& ins_payload) = 0;

protected:
    // instruction list
    std::vector<TestInstruction> ins_list_;
    int ins_index_{0};

    // modules
    LocalMemoryUnit local_memory_unit_;
    TestUnit test_unit_;

    // stall
    sc_core::sc_signal<bool> id_stall_;

    // id ex signals
    ExecuteUnitSignalPorts<InsPayload> signals_;

    sc_core::sc_event next_ins_;
    sc_core::sc_time running_time_;
};

template <class TestUnitModule>
int pimsim_unit_test(
    int argc, char* argv[],
    std::function<TestUnitModule*(const Config& config, Clock* clk, typename TestUnitModule::TestInfoType& test_info)>
        test_module_initializer) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    std::string exec_file_name{argv[0]};
    if (argc != 4) {
        std::cout << fmt::format("Usage: {} [config_file] [instruction_file] [report_file]", exec_file_name)
                  << std::endl;
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
    auto test_info = ins_j.get<typename TestUnitModule::TestInfoType>();

    Clock clk{"clock", config.sim_config.period_ns};
    auto* test_module = test_module_initializer(config, &clk, test_info);
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = test_module->getReporter();
    reporter.report(ofs);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getTotalEnergyPJ(), test_info.expected.energy_pj) &&
        test_module->checkTestResult(test_info.expected)) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}

}  // namespace pimsim
