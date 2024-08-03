//
// Created by wyk on 2024/8/3.
//

#pragma once

#include "payload.h"
#include "systemc.h"

namespace pimsim {

template <class InsPayload>
struct ExecuteUnitSignalPorts {
    sc_core::sc_signal<InsPayload> id_ex_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> busy_;
    sc_core::sc_signal<DataConflictPayload> data_conflict_;

    sc_core::sc_signal<bool> finish_ins_;
    sc_core::sc_signal<int> finish_ins_pc_;

    sc_core::sc_signal<bool> finish_run_;
};

template <class InsPayload>
struct ExecuteUnitRequestIOPorts {
    sc_core::sc_out<InsPayload> id_ex_payload_port_;
    sc_core::sc_out<bool> id_ex_enable_port_;
    sc_core::sc_in<bool> busy_port_;
    sc_core::sc_in<DataConflictPayload> data_conflict_port_;

    sc_core::sc_in<bool> finish_ins_port_;
    sc_core::sc_in<int> finish_ins_pc_port_;

    sc_core::sc_in<bool> finish_run_port_;
};

template <class InsPayload>
struct ExecuteUnitResponseIOPorts {
    sc_core::sc_in<InsPayload> id_ex_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;

    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_pc_port_;

    sc_core::sc_out<bool> finish_run_port_;

    void bind(ExecuteUnitSignalPorts<InsPayload>& signals) {
        id_ex_payload_port_.bind(signals.id_ex_payload_);
        id_ex_enable_port_.bind(signals.id_ex_enable_);
        busy_port_.bind(signals.busy_);
        data_conflict_port_.bind(signals.data_conflict_);
        finish_ins_port_.bind(signals.finish_ins_);
        finish_ins_pc_port_.bind(signals.finish_ins_pc_);
        finish_run_port_.bind(signals.finish_run_);
    }
};

}  // namespace pimsim
