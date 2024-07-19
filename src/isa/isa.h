//
// Created by wyk on 2024/7/19.
//

#pragma once

#include "better-enums/enum.h"

namespace pimsim {

BETTER_ENUM(InstClass, int,  // NOLINT(*-explicit-constructor)
            pim = 0b00, simd = 0b01, scalar = 0b10, transfer = 0b110, control = 0b111)

BETTER_ENUM(ScalarInstType, int,  // NOLINT(*-explicit-constructor)
            RR = 0b00, RI = 0b01, SL = 0b10, Assign = 0b11)

BETTER_ENUM(ScalarRRInstOpcode, int,  // NOLINT(*-explicit-constructor)
            add = 0b000, sub = 0b001, mul = 0b010, div = 0b011, sll = 0b100, srl = 0b101, sra = 0b110, mod = 0b111)

BETTER_ENUM(ScalarRIInstOpcode, int,  // NOLINT(*-explicit-constructor)
            addi = 0b00, muli = 0b01, lui = 0b10)

BETTER_ENUM(ScalarSLInstOpcode, int,  // NOLINT(*-explicit-constructor)
            load_local = 0b00, store_local = 0b01, load_global = 0b10, store_global = 0b11)

BETTER_ENUM(ScalarAssignInstOpcode, int,  // NOLINT(*-explicit-constructor)
            li_general = 0b00, li_special = 0b01, assign_general_to_special = 0b10, assign_special_to_general = 0b11)

}  // namespace pimsim