//
// Created by wyk on 2024/7/24.
//

#include "macro_group.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

MacroGroup::MacroGroup(const char *name, const pimsim::PimUnitConfig &config, const pimsim::SimConfig &sim_config,
                       pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config.macro_size)
    , controller_(std::string(name) + "_controller", config, sim_config, core, clk, next_sub_ins_,
                  result_adder_socket_) {
    SC_THREAD(processIssue)
    SC_THREAD(processResultAdderSubmodule)

    for (int i = 0; i < config_.macro_group_size; i++) {
        auto macro_name = fmt::format("{}_macro_{}", getName(), i);
        bool independent_ipu = config_.value_sparse || i == 0;
        macro_list_.push_back(
            new Macro(macro_name.c_str(), config_, sim_config, core, clk, independent_ipu, &result_adder_socket_));
    }
}

void MacroGroup::startExecute(pimsim::MacroGroupPayload payload) {
    macro_group_socket_.payload = std::move(payload);
    macro_group_socket_.start_exec.notify();
}

void MacroGroup::waitUntilFinishIfBusy() {
    macro_group_socket_.waitUntilFinishIfBusy();
}

EnergyReporter MacroGroup::getEnergyReporter() {
    EnergyReporter macro_group_reporter;
    for (auto *macro : macro_list_) {
        macro_group_reporter += macro->getEnergyReporter();
    }
    return std::move(macro_group_reporter);
}

void MacroGroup::setFinishInsFunc(std::function<void(int)> finish_ins_func) {
    finish_ins_func_ = std::move(finish_ins_func);
}

void MacroGroup::setFinishRunFunc(std::function<void()> finish_run_func) {
    finish_run_func_ = std::move(finish_run_func);
}

void MacroGroup::processIssue() {
    while (true) {
        macro_group_socket_.waitUntilStart();

        auto &payload = macro_group_socket_.payload;
        auto &pim_ins_info = payload.pim_ins_info;
        LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num));

        int activation_element_col_num = std::min(payload.activation_element_col_num,
                                                  macro_size_.element_cnt_per_compartment * config_.macro_group_size);
        int activation_macro_num = IntDivCeil(activation_element_col_num, macro_size_.element_cnt_per_compartment);
        for (int macro_id = 0; macro_id < activation_macro_num; macro_id++) {
            int macro_activation_element_col_num =
                (macro_id < activation_macro_num - 1)
                    ? macro_size_.element_cnt_per_compartment
                    : activation_element_col_num - macro_id * macro_size_.element_cnt_per_compartment;

            MacroPayload macro_payload{.pim_ins_info = pim_ins_info,
                                       .row = payload.row,
                                       .input_bit_width = payload.input_bit_width,
                                       .activation_element_col_num = macro_activation_element_col_num,
                                       .bit_sparse = payload.bit_sparse};
            if (macro_id < payload.macro_inputs.size()) {
                macro_payload.inputs.swap(payload.macro_inputs[macro_id]);
            }

            auto *macro = macro_list_[macro_id];
            macro->waitUntilFinishIfBusy();
            macro->startExecute(std::move(macro_payload));
        }

        controller_.waitUntilFinishIfBusy();
        controller_.start({.pim_ins_info = pim_ins_info,
                           .last_group = payload.last_group,
                           .input_bit_width = payload.input_bit_width,
                           .bit_sparse = payload.bit_sparse});
        wait(next_sub_ins_);

        macro_group_socket_.finish();
    }
}

void MacroGroup::processResultAdderSubmodule() {
    while (true) {
        result_adder_socket_.waitUntilStart();

        auto &sub_ins_info = result_adder_socket_.payload.sub_ins_info;
        auto &pim_ins_info = sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start result adder, ins pc: {}, sub ins num: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num));

        if (sub_ins_info.last_group && pim_ins_info.last_sub_ins && finish_ins_func_) {
            finish_ins_func_(pim_ins_info.ins_pc);
        }

        double latency = config_.result_adder.latency_cycle * period_ns_;
        wait(latency, SC_NS);

        if (sub_ins_info.last_group && pim_ins_info.last_sub_ins && pim_ins_info.last_ins && finish_run_func_) {
            finish_run_func_();
        }

        LOG(fmt::format("{} end result adder, ins pc: {}, sub ins num: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num));
        result_adder_socket_.finish();
    }
}

}  // namespace pimsim
