//
// Created by wyk on 2024/7/16.
//

#include "base_component/base_module.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "core/transfer_unit/transfer_unit.h"
#include "fmt/core.h"
#include "util/log.h"

static const std::string CONFIG_FILE = "../config/config_template.json";
static const std::string TEST_REPORT_FILE = "../report/Transfer_unit_test_report.txt";

namespace pimsim {

class TransferUnitTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(TransferUnitTestModule);

    TransferUnitTestModule(const char* name, const Config& config, Clock* clk)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , local_memory_unit_("local_memory_unit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, clk)
        , transfer_unit_("transfer_unit", config.chip_config.core_config.transfer_unit_config, config.sim_config,
                         nullptr, clk) {
        // bind ports
        transfer_unit_.id_transfer_payload_port_.bind(decode_transfer_payload_);
        transfer_unit_.id_ex_enable_port_.bind(id_ex_enable_);
        transfer_unit_.busy_port_.bind(transfer_busy_);
        transfer_unit_.data_conflict_port_.bind(transfer_data_conflict_);
        transfer_unit_.finish_ins_port_.bind(transfer_finish_ins_);
        transfer_unit_.finish_ins_pc_port_.bind(transfer_finish_ins_pc_);
        transfer_unit_.finish_run_port_.bind(transfer_finish_run_);

        SC_THREAD(issue)

        SC_METHOD(processStall)
        sensitive << transfer_data_conflict_ << transfer_busy_ << transfer_finish_ins_ << transfer_finish_ins_pc_;

        SC_METHOD(processIdExEnable)
        sensitive << id_stall_;

        SC_METHOD(processFinishIns)
        sensitive << transfer_finish_ins_ << transfer_finish_ins_pc_;

        SC_METHOD(processFinishRun)
        sensitive << transfer_finish_run_;

        transfer_unit_.bindLocalMemoryUnit(&local_memory_unit_);
        prepareInstructions();
        transfer_unit_.setEndPC(static_cast<int>(transfer_ins_list_.size()));
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(transfer_unit_.getName(), transfer_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1000, "Transfer_unit_test_module", getEnergyReporter(), 0};
    }

private:
    void issue() {
        wait(8, SC_NS);

        while (ins_index < transfer_ins_list_.size()) {
            decode_transfer_payload_.write(transfer_ins_list_[ins_index]);
            ins_index++;
            wait(next_ins_);
        }
        TransferInsPayload transfer_nop{};
        decode_transfer_payload_.write(transfer_nop);
    }

    void processStall() {
        const auto& transfer_conflict_payload = transfer_data_conflict_.read();
        auto ins_conflict_payload = getInsPayloadConflictPayload(transfer_ins_list_[ins_index]);

        auto transfer_busy = transfer_busy_.read();
        auto transfer_finish = transfer_finish_ins_.read();
        auto transfer_finish_pc = transfer_finish_ins_pc_.read();

        bool stall = MemoryConflictPayload::checkMemoryConflict(ins_conflict_payload, transfer_conflict_payload, true)
                         ? !(transfer_finish && transfer_finish_pc == transfer_conflict_payload.pc)
                         : transfer_busy;
        id_stall_.write(stall);
        if (!stall) {
            next_ins_.notify();
        }
    }

    void processIdExEnable() {
        id_ex_enable_.write(!id_stall_.read());
    }

    void processFinishIns() {
        if (transfer_finish_ins_.read()) {
            LOG(fmt::format("transfer ins finish, pc: {}", transfer_finish_ins_pc_.read()));
        }
    }

    void processFinishRun() {
        if (transfer_finish_run_.read()) {
            running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        }
    }

    void prepareInstructions() {
        TransferInsPayload ins1 = {
            .ins = {.pc = 1}, .src_address_byte = 1024, .dst_address_byte = 2048, .size_byte = 16};
        TransferInsPayload ins2 = {
            .ins = {.pc = 2}, .src_address_byte = 1024, .dst_address_byte = 2048, .size_byte = 16};
        TransferInsPayload ins3 = {
            .ins = {.pc = 3}, .src_address_byte = 1024, .dst_address_byte = 2048, .size_byte = 16};
        TransferInsPayload ins4 = {
            .ins = {.pc = 4}, .src_address_byte = 1024, .dst_address_byte = 2048, .size_byte = 16};

        transfer_ins_list_.emplace_back(ins1);
        transfer_ins_list_.emplace_back(ins2);
        transfer_ins_list_.emplace_back(ins3);
        transfer_ins_list_.emplace_back(ins4);
    }

    MemoryConflictPayload getInsPayloadConflictPayload(const TransferInsPayload& ins_payload) const {
        MemoryConflictPayload conflict_payload{.pc = ins_payload.ins.pc};

        int src_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.src_address_byte);
        int dst_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.dst_address_byte);

        conflict_payload.read_memory_id.insert(src_memory_id);
        conflict_payload.write_memory_id.insert(dst_memory_id);
        conflict_payload.used_memory_id.insert({src_memory_id, dst_memory_id});

        return std::move(conflict_payload);
    }

private:
    // instruction list
    std::vector<TransferInsPayload> transfer_ins_list_;
    int ins_index{0};

    // modules
    LocalMemoryUnit local_memory_unit_;
    TransferUnit transfer_unit_;

    // port from instruction decode
    sc_core::sc_signal<TransferInsPayload> decode_transfer_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> id_stall_;

    // port about busy, finish and data conflict
    sc_core::sc_signal<bool> transfer_busy_;
    sc_core::sc_signal<MemoryConflictPayload> transfer_data_conflict_;
    sc_core::sc_signal<bool> transfer_finish_ins_;
    sc_core::sc_signal<int> transfer_finish_ins_pc_;
    sc_core::sc_signal<bool> transfer_finish_run_;

    sc_core::sc_event next_ins_;
    sc_core::sc_time running_time_;
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
    TransferUnitTestModule test_module{"transfer_unit_test_module", config, &clk};
    sc_start();

    std::ofstream ofs;
    ofs.open(TEST_REPORT_FILE);
    auto reporter = test_module.getReporter();
    reporter.report(ofs);
    ofs.close();

    return 0;
}
