//
// Created by wyk on 2024/11/10.
//

#pragma once
#include "../base_component/base_module.h"
#include "../core/payload/payload.h"

namespace pimsim {

class MemoryHardware : public BaseModule {
public:
    MemoryHardware(const char* name, const SimConfig& sim_config, Core* core, Clock* clk)
        : BaseModule(name, sim_config, core, clk) {}

    virtual sc_core::sc_time accessAndGetDelay(MemoryAccessPayload& payload) = 0;

    EnergyReporter getEnergyReporter() override = 0;

    [[nodiscard]] virtual int getMemoryDataWidthByte(MemoryAccessType access_type) const = 0;
    [[nodiscard]] virtual int getMemorySizeByte() const = 0;
};

}  // namespace pimsim
