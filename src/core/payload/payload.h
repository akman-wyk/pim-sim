//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <array>
#include <cstdint>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "config/config.h"
#include "isa/isa.h"
#include "payload_enum.h"
#include "systemc.h"
#include "util/macro_scope.h"

namespace pimsim {

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr);

std::stringstream& operator<<(std::stringstream& out, const std::unordered_set<int>& set);

struct InstructionPayload {
    int pc{-1};

    [[nodiscard]] bool valid() const;

    friend std::ostream& operator<<(std::ostream& out, const InstructionPayload& ins) {
        out << "pc: " << ins.pc << "\n";
        return out;
    }

    bool operator==(const InstructionPayload& another) const {
        return pc == another.pc;
    }

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(InstructionPayload)
};

struct MemoryAccessPayload {
    InstructionPayload ins{};

    MemoryAccessType access_type;
    int address_byte;  // byte
    int size_byte;     // byte
    std::vector<uint8_t> data;
    sc_core::sc_event& finish_access;
};

struct DataConflictPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(DataConflictPayload)

    int pc{-1};

    std::unordered_set<int> read_memory_id;
    std::unordered_set<int> write_memory_id;
    std::unordered_set<int> used_memory_id;

    std::unordered_set<int> read_reg_id;
    int write_reg_id{-1};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(DataConflictPayload)

    static bool checkMemoryConflict(const DataConflictPayload& ins_conflict_payload,
                                    const DataConflictPayload& unit_conflict_payload, bool has_unit_conflict);

    static bool checkRegisterConflict(const DataConflictPayload& ins_conflict_payload,
                                      const DataConflictPayload& unit_conflict_payload);

    static bool checkDataConflict(const DataConflictPayload& ins_conflict_payload,
                                  const DataConflictPayload& unit_conflict_payload, bool has_unit_conflict);
};

struct SIMDInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(SIMDInsPayload)

    InstructionPayload ins{};

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

    DECLARE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload)
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInsPayload)
};

struct TransferInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(TransferInsPayload)

    InstructionPayload ins{};

    int src_address_byte{0};
    int dst_address_byte{0};
    int size_byte{0};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(TransferInsPayload)
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(TransferInsPayload)
};

struct ScalarInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ScalarInsPayload)

    InstructionPayload ins{};

    ScalarOperator op{};

    int src1_value{0}, src2_value{0}, offset{0};
    ;
    int dst_reg{0};

    bool access_global_memory{false};
    bool write_special_register{false};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(ScalarInsPayload)
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarInsPayload)
};

struct RegUnitWriteRequest {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(RegUnitWriteRequest)

    int reg_id{0}, reg_value{0};
    bool write_special_register{false};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(RegUnitWriteRequest)
};

}  // namespace pimsim
