//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <queue>

#include "../base_component/base_module.h"
#include "../config/config.h"
#include "../core/payload/payload.h"
#include "memory_hardware.h"

namespace pimsim {

class Memory : public BaseModule {
public:
    SC_HAS_PROCESS(Memory);

    Memory(const char* name, const RAMConfig& ram_config, const AddressSpaceConfig& addressing,
                const SimConfig& sim_config, Core* core, Clock* clk);

    Memory(const char* name, const RegBufferConfig& reg_buffer_config, const AddressSpaceConfig& addressing,
                const SimConfig& sim_config, Core* core, Clock* clk);

    void access(std::shared_ptr<MemoryAccessPayload> payload);

    [[nodiscard]] int getAddressSpaceBegin() const;
    [[nodiscard]] int getAddressSpaceEnd() const;
    [[nodiscard]] int getMemoryDataWidthByte(MemoryAccessType access_type) const;
    [[nodiscard]] int getMemorySizeByte() const;

    EnergyReporter getEnergyReporter() override;

private:
    [[noreturn]] void process();

private:
    const AddressSpaceConfig& addressing_;

    std::queue<std::shared_ptr<MemoryAccessPayload>> access_queue_;
    std::shared_ptr<MemoryHardware> hardware_;

    sc_core::sc_event start_process_;
};

}  // namespace pimsim
