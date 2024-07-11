//
// Created by wyk on 2024/7/8.
//

#include "payload.h"

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

DEFINE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width, output_bit_width,
                             inputs_address_byte, output_address_byte, len, pipelined)

DEFINE_PIM_PAYLOAD_FUNCTIONS(SIMDInsDataConflictPayload, inputs_address_byte, output_address_byte)

}  // namespace pimsim
