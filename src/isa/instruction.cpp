//
// Created by wyk on 2024/8/8.
//

#include "instruction.h"

namespace pimsim {

void from_json(const nlohmann::ordered_json& j, Instruction& t) {
    const Instruction obj{};
    t.class_code = j.value("class_code", obj.class_code);
    t.type = j.value("type", obj.type);
    t.opcode = j.value("opcode", obj.opcode);

    if (j.contains("rs1")) {
        t.rs1 = j["rs1"];
    } else if (j.contains("rs")) {
        t.rs1 = j["rs"];
    } else {
        t.rs1 = obj.rs1;
    }

    t.rs2 = j.value("rs2", obj.rs2);
    t.rs3 = j.value("rs3", obj.rs3);
    t.rs4 = j.value("rs4", obj.rs4);
    t.rd = j.value("rd", obj.rd);
    t.imm = j.value("imm", obj.imm);
    t.offset = j.value("offset", obj.offset);
    t.value_sparse = j.value("value_sparse", obj.value_sparse);
    t.bit_sparse = j.value("bit_sparse", obj.bit_sparse);
    t.group = j.value("group", obj.group);
    t.group_input_mode = j.value("group_input_mode", obj.group_input_mode);
    t.group_broadcast = j.value("group_broadcast", obj.group_broadcast);
    t.outsum_move = j.value("outsum_move", obj.outsum_move);
    t.outsum = j.value("outsum", obj.outsum);
    t.input_num = j.value("input_num", obj.input_num);
    t.offset_mask = j.value("offset_mask", obj.offset_mask);
}

DEFINE_TYPE_TO_JSON_FUNCTION_WITH_DEFAULT(Instruction, class_code, type, opcode, rs1, rs2, rs3, rs4, rd, imm, offset,
                                          value_sparse, bit_sparse, group, group_input_mode, group_broadcast,
                                          outsum_move, outsum, input_num, offset_mask)

}  // namespace pimsim