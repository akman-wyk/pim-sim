//
// Created by wyk on 2024/7/4.
//

#include "local_memory_unit.h"

#include "fmt/core.h"

namespace pimsim {

LocalMemoryUnit::LocalMemoryUnit(const char *name, const pimsim::LocalMemoryUnitConfig &config,
                                 const pimsim::SimConfig &sim_config, pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config) {
    for (const auto &local_memory_config : config_.local_memory_list) {
        local_memory_list_.emplace_back(
            new LocalMemory{local_memory_config.name.c_str(), local_memory_config, sim_config, core, clk});
    }
}

std::vector<uint8_t> LocalMemoryUnit::read_data(const pimsim::InstructionPayload &ins, int address_byte, int size_byte,
                                                sc_core::sc_event &finish_access) {
    auto *local_memory = getLocalMemoryByAddress(address_byte);
    if (local_memory == nullptr) {
        std::cerr
            << fmt::format(
                   "Invalid memory read with ins NO.'{}': address does not match any local memory's address space",
                   ins.pc)
            << std::endl;
        return {};
    }

    MemoryAccessPayload payload{.ins = ins,
                                .access_type = MemoryAccessType::read,
                                .address_byte = address_byte - local_memory->getAddressSpaceBegin(),
                                .size_byte = size_byte,
                                .finish_access = finish_access};
    local_memory->access(&payload);
    wait(payload.finish_access);

    return std::move(payload.data);
}

void LocalMemoryUnit::write_data(const pimsim::InstructionPayload &ins, int address_byte, int size_byte,
                                 std::vector<uint8_t> data, sc_core::sc_event &finish_access) {
    auto *local_memory = getLocalMemoryByAddress(address_byte);
    if (local_memory == nullptr) {
        std::cerr
            << fmt::format(
                   "Invalid memory write with ins NO.'{}': address does not match any local memory's address space",
                   ins.pc)
            << std::endl;
        return;
    }

    MemoryAccessPayload payload{.ins = ins,
                                .access_type = MemoryAccessType::write,
                                .address_byte = address_byte - local_memory->getAddressSpaceBegin(),
                                .size_byte = size_byte,
                                .data = std::move(data),
                                .finish_access = finish_access};
    local_memory->access(&payload);
    wait(payload.finish_access);
}

EnergyReporter LocalMemoryUnit::getEnergyReporter() {
    EnergyReporter local_memory_unit_reporter;
    for (auto *local_memory : local_memory_list_) {
        local_memory_unit_reporter.addSubModule(local_memory->getName(), local_memory->getEnergyReporter());
    }
    return std::move(local_memory_unit_reporter);
}

int LocalMemoryUnit::getLocalMemoryIdByAddress(int address_byte) const {
    for (int i = 0; i < local_memory_list_.size(); i++) {
        auto *local_memory = local_memory_list_[i];
        if (local_memory->getAddressSpaceBegin() <= address_byte && address_byte < local_memory->getAddressSpaceEnd()) {
            return i;
        }
    }
    return -1;
}

LocalMemory *LocalMemoryUnit::getLocalMemoryByAddress(int address_byte) {
    for (auto *local_memory : local_memory_list_) {
        if (local_memory->getAddressSpaceBegin() <= address_byte && address_byte < local_memory->getAddressSpaceEnd()) {
            return local_memory;
        }
    }
    return nullptr;
}

}  // namespace pimsim
