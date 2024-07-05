//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <queue>

#include "base_component/base_module.h"
#include "config/config.h"
#include "core/payload/payload.h"
#include "ram.h"
#include "reg_buffer.h"

namespace pimsim {

class LocalMemory : public BaseModule {
public:
    SC_HAS_PROCESS(LocalMemory);

    LocalMemory(const sc_core::sc_module_name& name, const LocalMemoryConfig& config, const SimConfig& sim_config,
                Core* core, Clock* clk);

    void access(MemoryAccessPayload* payload);

    [[nodiscard]] int getAddressSpaceBegin() const;
    [[nodiscard]] int getAddressSpaceEnd() const;

private:
    [[noreturn]] void process();

private:
    const LocalMemoryConfig& config_;

    std::queue<MemoryAccessPayload*> access_queue_;
    RAM ram;
    RegBuffer reg_buffer;

    sc_core::sc_event start_process_;
};

}  // namespace pimsim
