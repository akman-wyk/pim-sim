//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <array>
#include <cstdint>
#include <sstream>
#include <vector>

#include "payload_enum.h"
#include "systemc.h"
#include "util/macro_scope.h"
#include "config/config.h"

namespace pimsim {

struct InstructionPayload {
    int pc;

    [[nodiscard]] bool valid() const;

    friend std::ostream& operator<<(std::ostream& out, const InstructionPayload* ins) {
        out << "pc: " << ins->pc << "\n";
        return out;
    }
};

struct MemoryAccessPayload {
    const InstructionPayload* ins;

    MemoryAccessType access_type;
    int address_byte;  // byte
    int size_byte;     // byte
    std::vector<uint8_t> data;
    sc_core::sc_event& finish_access;
};

template <class V, int N>
std::ostream& operator<<(std::ostream& out, const std::array<V, N>& arr) {
    if (!arr.empty()) {
        out << arr[0];
        for (int i = 1; i < arr.size(); i++) {
            out << ", " << arr[i];
        }
    }
    return out;
}

struct SIMDInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(SIMDInsPayload)

    const InstructionPayload* ins{nullptr};

    // compute type info
    unsigned int input_cnt{2};
    unsigned int opcode{0x00};

    // data width info
    std::array<int, SIMD_MAX_INPUT_NUM> inputs_bit_width{0, 0, 0, 0};
    int output_bit_width{0};

    // data address info
    std::array<int, SIMD_MAX_INPUT_NUM> inputs_address_byte{0, 0, 0, 0};
    int output_address_byte{0};

    // vector length info
    int len{0};

    // instruction's execution can be pipelined
    bool pipelined{false};

    DEFINE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width, output_bit_width,
                                 inputs_address_byte, output_address_byte, len, pipelined)
};

}  // namespace pimsim
