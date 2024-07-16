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
        simd_unit_.data_conflict_port_.bind(simd_data_conflict_);
        simd_unit_.finish_ins_port_.bind(simd_finish_);
        simd_unit_.finish_ins_pc_port_.bind(simd_finish_pc_);
        simd_unit_.finish_run_.bind(simd_finish_run_);

        simd_unit_.bindLocalMemoryUnit(&local_memory_unit_);


        SC_THREAD(issue)

        SC_METHOD(processStall)
        sensitive << simd_data_conflict_ << simd_busy_ << simd_finish_ << simd_finish_pc_;

        SC_METHOD(processFinishRun)
        sensitive << simd_finish_run_;

        SC_METHOD(processIdExEnable)
        sensitive << id_stall_;

        SC_METHOD(processFinishIns)
        sensitive << simd_finish_ << simd_finish_pc_;

        // prepare SIMD instructions
        prepareInstructions();
        simd_unit_.setEndPC(static_cast<int>(simd_ins_list_.size()));
    }

    void issue() {
        wait(8, SC_NS);

        while (ins_index < simd_ins_list_.size()) {
            decode_simd_payload_.write(simd_ins_list_[ins_index]);
            ins_index++;
            wait(next_ins_);
        }
        SIMDInsPayload simd_nop{};
        decode_simd_payload_.write(simd_nop);
    }

    void processStall() {
        const auto& simd_data_conflict_info = simd_data_conflict_.read();
        auto simd_unit_conflict_infos = getSIMDUnitConflictInfos(simd_data_conflict_info);

        const auto& next_ins_payload = simd_ins_list_[ins_index];
        auto ins_conflict_infos = getInsPayloadConflictInfos(next_ins_payload);

        bool simd_busy = simd_busy_.read();
        bool simd_finish = simd_finish_.read();
        int simd_finish_pc = simd_finish_pc_.read();

        bool stall = checkMemoryConflict(ins_conflict_infos, simd_unit_conflict_infos)
                         ? (!simd_finish || simd_finish_pc != simd_data_conflict_info.pc)
                         : simd_busy;
        id_stall_.write(stall);
        if (!stall) {
            next_ins_.notify();
        }
    }

    void processFinishIns() {
        if (simd_finish_.read()) {
            LOG(fmt::format("simd ins finish, pc: {}", simd_finish_pc_.read()));
        }
    }

    void processFinishRun() {
        if (simd_finish_run_.read()) {
            running_time = sc_core::sc_time_stamp();
            sc_stop();
        }
    }

    void processIdExEnable() {
        id_ex_enable_.write(!id_stall_.read());
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
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};
        SIMDInsPayload simd_ins2{.ins = {.pc = 2},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};
        SIMDInsPayload simd_ins3{.ins = {.pc = 3},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};
        SIMDInsPayload simd_ins4{.ins = {.pc = 4},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};
        SIMDInsPayload simd_ins5{.ins = {.pc = 5},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};
        SIMDInsPayload simd_ins6{.ins = {.pc = 6},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};
        SIMDInsPayload simd_ins7{.ins = {.pc = 7},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 16,
                                 .pipelined = true};

        simd_ins_list_.emplace_back(simd_ins1);
        simd_ins_list_.emplace_back(simd_ins2);
        simd_ins_list_.emplace_back(simd_ins3);
        simd_ins_list_.emplace_back(simd_ins4);
        simd_ins_list_.emplace_back(simd_ins5);
        simd_ins_list_.emplace_back(simd_ins6);
        simd_ins_list_.emplace_back(simd_ins7);
    }

    MemoryConflictInfo getInsPayloadConflictInfos(const SIMDInsPayload& ins_payload) const {
        MemoryConflictInfo conflict_info;
        for (unsigned int i = 0; i < ins_payload.input_cnt; i++) {
            conflict_info.read_memory_id.emplace(
                local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.inputs_address_byte[i]));
        }
        conflict_info.write_memory_id.emplace(
            local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.output_address_byte));
        return std::move(conflict_info);
    }

    MemoryConflictInfo getSIMDUnitConflictInfos(const SIMDInsDataConflictPayload& simd_data_conflict_info) const {
        MemoryConflictInfo conflict_info;
        for (auto input_addr : simd_data_conflict_info.inputs_address_byte) {
            if (input_addr != -1) {
                conflict_info.read_memory_id.emplace(local_memory_unit_.getLocalMemoryIdByAddress(input_addr));
            }
        }
        if (simd_data_conflict_info.output_address_byte != -1) {
            conflict_info.write_memory_id.emplace(
                local_memory_unit_.getLocalMemoryIdByAddress(simd_data_conflict_info.output_address_byte));
        }
        return std::move(conflict_info);
    }

    static bool checkMemoryConflict(const MemoryConflictInfo& ins_conflict_info,
                                    const MemoryConflictInfo& unit_conflict_info) {
        return std::any_of(unit_conflict_info.write_memory_id.begin(), unit_conflict_info.write_memory_id.end(),
                           [&](int unit_write_memory_id) {
                               return ins_conflict_info.read_memory_id.find(unit_write_memory_id) !=
                                      ins_conflict_info.read_memory_id.end();
                           });
    }

private:
    std::vector<SIMDInsPayload> simd_ins_list_;
    int ins_index{0};

    LocalMemoryUnit local_memory_unit_;
    SIMDUnit simd_unit_;

    std::vector<MemoryConflictInfo> simd_memory_conflict_infos_{};

    sc_core::sc_signal<SIMDInsPayload> decode_simd_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> simd_busy_;
    sc_core::sc_signal<SIMDInsDataConflictPayload> simd_data_conflict_;
    sc_core::sc_signal<bool> simd_finish_;
    sc_core::sc_signal<int> simd_finish_pc_;
    sc_core::sc_signal<bool> simd_finish_run_;

    sc_core::sc_signal<bool> id_stall_;

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
