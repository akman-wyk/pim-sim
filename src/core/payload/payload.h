//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <cstdint>
#include <vector>

#include "payload_enum.h"
#include "systemc.h"

namespace pimsim {

struct InstructionPayload {
    int pc;
};

struct MemoryAccessPayload {
    const InstructionPayload& ins;

    MemoryAccessType access_type;
    int address_byte;  // byte
    int size_byte;     // byte
    std::vector<uint8_t> data;
    sc_core::sc_event& finish_access;
};

}  // namespace pimsim
