//
// Created by wyk on 2024/7/19.
//

#pragma once

namespace pimsim {

class RegUnit;

class RegUnitSocket {
public:
    RegUnitSocket() = default;

    void bindRegUnit(RegUnit* reg_unit);

    int getSpecialBoundGeneralId(int special_id);

private:
    RegUnit* reg_unit_{nullptr};
};

}  // namespace pimsim
