//
// Created by wyk on 2024/7/5.
//

#pragma once
#include "systemc.h"

namespace pimsim {

template <class PayloadType>
struct SubmoduleSocket {
    PayloadType payload;
    bool busy{false};
    sc_core::sc_event start_exec;
    sc_core::sc_event finish_exec;

    void waitUtilFinish() const {
        if (busy) {
            wait(finish_exec);
        }
    }
};

}  // namespace pimsim
