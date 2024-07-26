//
// Created by wyk on 2024/7/21.
//

#include "base_component/base_module.h"
#include "config/config.h"
#include "core/pim_unit/macro.h"
#include "core/pim_unit/pim_payload.h"
#include "test_macro.h"
#include "util/macro_scope.h"
#include "util/util.h"

namespace pimsim {

struct MacroTestConfig {
    bool independent_ipu{false};
};

struct MacroExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};
};

struct MacroTestInfo {
    std::vector<MacroPayload> code{};
    MacroTestConfig config{};
    MacroExpectedInfo expected{};
};

class MacroTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(MacroTestModule);

    MacroTestModule(const char* name, const Config& config, Clock* clk, std::vector<MacroPayload> codes,
                    const MacroTestConfig& test_config)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , macro_("macro", config.chip_config.core_config.pim_unit_config, config.sim_config, nullptr, clk,
                 test_config.independent_ipu) {
        macro_ins_list_ = std::move(codes);

        SC_THREAD(issue)

        macro_.setFinishRunFunction([&]() {
            wait(SC_ZERO_TIME);
            this->running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        });
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(macro_.getName(), macro_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1e3, "MacroTestModule", getEnergyReporter(), 0};
    }

private:
    void issue() {
        wait(10, SC_NS);

        for (const auto& macro_ins : macro_ins_list_) {
            macro_.waitUntilFinishIfBusy();
            macro_.startExecute(macro_ins);
            double latency = macro_ins.input_bit_width * period_ns_;
            wait(latency, SC_NS);
        }
    }

private:
    // instruction list
    std::vector<MacroPayload> macro_ins_list_;

    // modules
    Macro macro_;

    sc_core::sc_time running_time_;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimInsInfo, ins_pc, sub_ins_num, last_ins, last_sub_ins)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroPayload, pim_ins_info, row, input_bit_width,
                                               activation_element_col_num, inputs)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroTestConfig, independent_ipu)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroExpectedInfo, time_ns, energy_pj)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroTestInfo, code, config, expected)

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 4) {
        std::cout << "Usage: ./MacroTest [config_file] [instruction_file] [report_file]" << std::endl;
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
    auto test_info = ins_j.get<MacroTestInfo>();

    Clock clk{"clock", config.sim_config.period_ns};
    MacroTestModule test_module{"MacroTestModule", config, &clk, std::move(test_info.code), test_info.config};
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
