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

bool DataConflictPayload::checkMemoryConflict(const pimsim::DataConflictPayload& ins_conflict_payload,
                                              const pimsim::DataConflictPayload& unit_conflict_payload,
                                              bool has_unit_conflict) {
    if (has_unit_conflict) {
        return SetsIntersection(unit_conflict_payload.write_memory_id, ins_conflict_payload.read_memory_id);
    } else {
        return SetsIntersection(unit_conflict_payload.used_memory_id, ins_conflict_payload.used_memory_id);
    }
}

bool DataConflictPayload::checkRegisterConflict(const pimsim::DataConflictPayload& ins_conflict_payload,
                                                const pimsim::DataConflictPayload& unit_conflict_payload) {
    return ins_conflict_payload.read_reg_id.find(unit_conflict_payload.write_reg_id) !=
           ins_conflict_payload.read_reg_id.end();
}

bool DataConflictPayload::checkDataConflict(const DataConflictPayload& ins_conflict_payload,
                                            const DataConflictPayload& unit_conflict_payload, bool has_unit_conflict) {
    return checkMemoryConflict(ins_conflict_payload, unit_conflict_payload, has_unit_conflict) ||
           checkRegisterConflict(ins_conflict_payload, unit_conflict_payload);
}

DEFINE_PIM_PAYLOAD_FUNCTIONS(DataConflictPayload, pc, read_memory_id, write_memory_id, used_memory_id)

DEFINE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width, output_bit_width,
                             inputs_address_byte, output_address_byte, len)

DEFINE_PIM_PAYLOAD_FUNCTIONS(TransferInsPayload, ins, src_address_byte, dst_address_byte, size_byte)

DEFINE_PIM_PAYLOAD_FUNCTIONS(ScalarInsPayload, ins, op, src1_value, src2_value, offset, dst_reg, access_global_memory,
                             write_special_register)

DEFINE_PIM_PAYLOAD_FUNCTIONS(RegUnitWriteRequest, reg_id, reg_value, write_special_register)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(InstructionPayload, pc)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width,
                                               output_bit_width, inputs_address_byte, output_address_byte, len)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferInsPayload, ins, src_address_byte, dst_address_byte, size_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ScalarInsPayload, ins, op, src1_value, src2_value, offset, dst_reg,
                                               access_global_memory, write_special_register)

}  // namespace pimsim
