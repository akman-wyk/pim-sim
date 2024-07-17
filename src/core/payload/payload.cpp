//
// Created by wyk on 2024/7/8.
//

#include "payload.h"

#include "util/util.h"

namespace pimsim {

bool InstructionPayload::valid() const {
    return pc != -1;
}

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr) {
    out << arr[0];
    for (int i = 1; i < arr.size(); i++) {
        out << ", " << arr[i];
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::unordered_set<int>& set) {
    for (auto it = set.begin(); it != set.end(); ++it) {
        if (it != set.begin()) {
            out << ", ";
        }
        out << *it;
    }
    return out;
}

bool MemoryConflictPayload::checkMemoryConflict(const pimsim::MemoryConflictPayload& ins_conflict_payload,
                                                const pimsim::MemoryConflictPayload& unit_conflict_payload,
                                                bool has_unit_conflict) {
    if (has_unit_conflict) {
        return SetsIntersection(unit_conflict_payload.write_memory_id, ins_conflict_payload.read_memory_id);
    } else {
        return SetsIntersection(unit_conflict_payload.used_memory_id, ins_conflict_payload.used_memory_id);
    }
}

DEFINE_PIM_PAYLOAD_FUNCTIONS(MemoryConflictPayload, pc, read_memory_id, write_memory_id, used_memory_id)

DEFINE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width, output_bit_width,
                             inputs_address_byte, output_address_byte, len)

DEFINE_PIM_PAYLOAD_FUNCTIONS(TransferInsPayload, ins, src_address_byte, dst_address_byte, size_byte)

}  // namespace pimsim
