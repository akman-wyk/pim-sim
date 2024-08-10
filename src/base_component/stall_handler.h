//
// Created by wyk on 2024/8/10.
//

#pragma once
#include <unordered_map>

#include "core/payload/execute_unit_payload.h"
#include "core/payload/payload.h"
#include "systemc.h"

namespace pimsim {

class StallHandler : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(StallHandler);

    StallHandler();

    template <class InsPayload>
    void bind(ExecuteUnitSignalPorts<InsPayload>& signals, sc_core::sc_signal<bool>& conflict_signal,
              DataConflictPayload* cur_ins_conflict_info) {
        busy_.bind(signals.busy_);
        data_conflict_.bind(signals.data_conflict_);
        finish_ins_.bind(signals.finish_ins_);
        finish_ins_pc_.bind(signals.finish_ins_pc_);
        conflict_.bind(conflict_signal);

        this->cur_ins_conflict_info_ = cur_ins_conflict_info;
    }

private:
    void processAddUnitDataConflict();
    void processDeleteUnitDataConflict();
    void processUnitDataConflict();

public:
    sc_core::sc_in<bool> busy_;
    sc_core::sc_in<DataConflictPayload> data_conflict_;
    sc_core::sc_in<bool> finish_ins_;
    sc_core::sc_in<int> finish_ins_pc_;

    sc_core::sc_out<bool> conflict_;

private:
    DataConflictPayload* cur_ins_conflict_info_{nullptr};

    std::unordered_map<int, DataConflictPayload> ins_data_conflict_info_map_{};
    DataConflictPayload unit_data_conflict_info_{};
    sc_core::sc_event trigger_;
};

}  // namespace pimsim
