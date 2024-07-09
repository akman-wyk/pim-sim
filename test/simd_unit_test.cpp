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
#include "util/log.h"

static const std::string CONFIG_FILE = "../config/config_template.json";
static const std::string TEST_REPORT_FILE = "../report/SIMD_unit_test_report.txt";

namespace pimsim {

class SIMDUnitTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(SIMDUnitTestModule);

    SIMDUnitTestModule(const char* name, const Config& config, Clock* clk)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , local_memory_unit_("local_memory_unit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, clk)
        , simd_unit_("SIMD_unit", config.chip_config.core_config.simd_unit_config, config.sim_config, nullptr, clk) {
        // bind ports
        simd_unit_.id_simd_payload_port_.bind(decode_simd_payload_);
        simd_unit_.id_ex_enable_port_.bind(id_ex_enable_);
        simd_unit_.busy_port_.bind(simd_busy_);
        simd_unit_.finish_port_.bind(simd_finish_);
        simd_unit_.finish_pc_port_.bind(simd_finish_pc_);

        simd_unit_.bindLocalMemoryUnit(&local_memory_unit_);

        SC_THREAD(issue)

        SC_METHOD(processBusy)
        sensitive << simd_busy_;

        SC_METHOD(processFinish)
        sensitive << simd_finish_ << simd_finish_pc_;

        // prepare SIMD instructions
        prepareInstructions();
    }

    void issue() {
        id_ex_enable_.write(true);
        wait(8, SC_NS);

        for (const auto& simd_ins : simd_ins_list_) {
            decode_simd_payload_.write(simd_ins);
            wait(next_ins_);
        }
        SIMDInsPayload simd_nop{};
        decode_simd_payload_.write(simd_nop);
    }

    void processBusy() {
        if (!simd_busy_.read()) {
            next_ins_.notify();
        }
    }

    void processFinish() {
        if (simd_finish_.read()) {
            LOG(fmt::format("simd ins finish, pc: {}", simd_finish_pc_.read()));
            if (simd_finish_pc_.read() == simd_ins_list_.size()) {
                running_time = sc_core::sc_time_stamp();
                sc_stop();
            }
        }
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(simd_unit_.getName(), simd_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time);
        return Reporter{running_time.to_seconds() * 1000, "SIMD_test_module", getEnergyReporter(), 0};
    }

private:
    void prepareInstructions() {
        SIMDInsPayload simd_ins1{.ins = {.pc = 1},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 1024,
                                 .len = 63};

        SIMDInsPayload simd_ins2{.ins = {.pc = 2},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1536, 0, 0, 0},
                                 .output_address_byte = 1536,
                                 .len = 59};

        simd_ins_list_.emplace_back(simd_ins1);
        simd_ins_list_.emplace_back(simd_ins2);
    }

private:
    std::vector<SIMDInsPayload> simd_ins_list_;

    LocalMemoryUnit local_memory_unit_;
    SIMDUnit simd_unit_;

    sc_core::sc_signal<SIMDInsPayload> decode_simd_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> simd_busy_;
    sc_core::sc_signal<bool> simd_finish_;
    sc_core::sc_signal<int> simd_finish_pc_;

    sc_core::sc_event next_ins_;

    sc_core::sc_time running_time;
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    std::ifstream ifs;
    ifs.open(CONFIG_FILE);
    nlohmann::ordered_json j = nlohmann::ordered_json::parse(ifs);
    ifs.close();

    auto config = j.get<Config>();
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return 1;
    }

    Clock clk{"clock", config.sim_config.period_ns};
    SIMDUnitTestModule test_module{"SIMD_unit_test_module", config, &clk};
    sc_start();

    std::ofstream ofs;
    ofs.open(TEST_REPORT_FILE);
    auto reporter = test_module.getReporter();
    reporter.report(ofs);
    ofs.close();

    return 0;
}
