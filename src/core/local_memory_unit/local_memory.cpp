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

int LocalMemory::getMemoryDataWidthByte(MemoryAccessType access_type) const {
    if (config_.type == +LocalMemoryType::ram) {
        return config_.ram_config.width_byte;
    } else {
        return access_type == +MemoryAccessType::read ? config_.reg_buffer_config.read_max_width_byte
                                                      : config_.reg_buffer_config.write_max_width_byte;
    }
}

int LocalMemory::getMemorySizeByte() const {
    if (config_.type == +LocalMemoryType::ram) {
        return config_.ram_config.size_byte;
    } else {
        return config_.reg_buffer_config.size_byte;
    }
}

EnergyReporter LocalMemory::getEnergyReporter() {
    if (config_.type == +LocalMemoryType::ram) {
        return ram.getEnergyReporter();
    } else {
        return reg_buffer.getEnergyReporter();
    }
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
        if (payload_ptr->ins.unit_type != +ExecuteUnitType::scalar) {
            wait(access_delay);
        }
        payload_ptr->finish_access.notify();
    }
}

}  // namespace pimsim
