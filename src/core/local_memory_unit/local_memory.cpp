//
// Created by wyk on 2024/7/4.
//

#include "local_memory.h"

namespace pimsim {

LocalMemory::LocalMemory(const char *name, const pimsim::LocalMemoryConfig &config, const pimsim::SimConfig &sim_config,
                         pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , ram(name, config.ram_config, sim_config, core, clk)
    , reg_buffer(name, config.reg_buffer_config, sim_config, core, clk) {
    SC_THREAD(process)
}

void LocalMemory::access(MemoryAccessPayload *payload) {
    access_queue_.emplace(payload);
    start_process_.notify();
}

int LocalMemory::getAddressSpaceBegin() const {
    return config_.addressing.offset_byte;
}

int LocalMemory::getAddressSpaceEnd() const {
    return config_.addressing.offset_byte + config_.addressing.size_byte;
}

void LocalMemory::process() {
    while (true) {
        while (access_queue_.empty()) {
            wait(start_process_);
        }
        MemoryAccessPayload *payload_ptr = access_queue_.front();
        access_queue_.pop();

        sc_core::sc_time access_delay;
        if (config_.type == +LocalMemoryType::ram) {
            access_delay = ram.accessAndGetDelay(*payload_ptr);
        } else {
            access_delay = reg_buffer.accessAndGetDelay(*payload_ptr);
        }
        wait(access_delay);
        payload_ptr->finish_access.notify();
    }
}

}  // namespace pimsim
