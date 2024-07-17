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
        prepareInstructions();
        simd_unit_.setEndPC(static_cast<int>(simd_ins_list_.size()));
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
        const auto& simd_conflict_payload = simd_data_conflict_.read();
        auto ins_conflict_payload = getInsPayloadConflictInfos(simd_ins_list_[ins_index]);

        bool simd_busy = simd_busy_.read();
        bool simd_finish = simd_finish_ins_.read();
        int simd_finish_pc = simd_finish_ins_pc_.read();

        bool stall = MemoryConflictPayload::checkMemoryConflict(ins_conflict_payload, simd_conflict_payload, true)
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
            running_time = sc_core::sc_time_stamp();
            sc_stop();
        }
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
                                 .len = 63};
        SIMDInsPayload simd_ins2{.ins = {.pc = 2},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {2048, 0, 0, 0},
                                 .output_address_byte = 2560,
                                 .len = 49};
        SIMDInsPayload simd_ins3{.ins = {.pc = 3},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 1536,
                                 .len = 64};
        SIMDInsPayload simd_ins4{.ins = {.pc = 4},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 1536,
                                 .len = 64};
        SIMDInsPayload simd_ins5{.ins = {.pc = 5},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {2048, 0, 0, 0},
                                 .output_address_byte = 1024,
                                 .len = 64};
        SIMDInsPayload simd_ins6{.ins = {.pc = 6},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {2048, 0, 0, 0},
                                 .output_address_byte = 1024,
                                 .len = 64};
        SIMDInsPayload simd_ins7{.ins = {.pc = 7},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {2048, 0, 0, 0},
                                 .output_address_byte = 2560,
                                 .len = 64};
        SIMDInsPayload simd_ins8{.ins = {.pc = 8},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {2048, 0, 0, 0},
                                 .output_address_byte = 1024,
                                 .len = 64};
        SIMDInsPayload simd_ins9{.ins = {.pc = 9},
                                 .input_cnt = 1,
                                 .opcode = 0x00,
                                 .inputs_bit_width = {8, 0, 0, 0},
                                 .output_bit_width = 8,
                                 .inputs_address_byte = {1024, 0, 0, 0},
                                 .output_address_byte = 2048,
                                 .len = 64};

        simd_ins_list_.emplace_back(simd_ins1);
        simd_ins_list_.emplace_back(simd_ins2);
        simd_ins_list_.emplace_back(simd_ins3);
        simd_ins_list_.emplace_back(simd_ins4);
        simd_ins_list_.emplace_back(simd_ins5);
        simd_ins_list_.emplace_back(simd_ins6);
        simd_ins_list_.emplace_back(simd_ins7);
        simd_ins_list_.emplace_back(simd_ins8);
        simd_ins_list_.emplace_back(simd_ins9);
    }

    MemoryConflictPayload getInsPayloadConflictInfos(const SIMDInsPayload& ins_payload) const {
        MemoryConflictPayload conflict_payload{.pc = ins_payload.ins.pc};
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
    int ins_index{0};

    // modules
    LocalMemoryUnit local_memory_unit_;
    SIMDUnit simd_unit_;

    // port from instruction decode
    sc_core::sc_signal<SIMDInsPayload> decode_simd_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> id_stall_;

    // port about busy, finish and data conflict
    sc_core::sc_signal<bool> simd_busy_;
    sc_core::sc_signal<MemoryConflictPayload> simd_data_conflict_;
    sc_core::sc_signal<bool> simd_finish_ins_;
    sc_core::sc_signal<int> simd_finish_ins_pc_;
    sc_core::sc_signal<bool> simd_finish_run_;

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
