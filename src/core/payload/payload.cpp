//
// Created by wyk on 2024/7/8.
//

#include "payload.h"

namespace pimsim {

bool InstructionPayload::valid() const {
    return pc != -1;
}

}
