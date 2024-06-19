//
// Created by wyk on 24-6-17.
//

#include "config_enum.h"

#include <string>

namespace pimsim {
void to_json(nlohmann::json& j, const SimMode& m) {
    j = m._to_string();
}

void from_json(const nlohmann::json& j, SimMode& m) {
    const auto str = j.get<std::string>();
    if (str == "run_until_time") {
        m = SimMode::run_until_time;
    } else if (str == "run_one_round") {
        m = SimMode::run_one_round;
    } else {
        m = SimMode::other;
    }
}

void to_json(nlohmann::json& j, const DataMode& m) {
    j = m._to_string();
}

void from_json(const nlohmann::json& j, DataMode& m) {
    const auto str = j.get<std::string>();
    if (str == "real_data") {
        m = DataMode::real_data;
    } else if (str == "not_real_data") {
        m = DataMode::not_real_data;
    } else {
        m = DataMode::other;
    }
}

void to_json(nlohmann::json& j, const LocalMemoryType& m) {
    j = m._to_string();
}

void from_json(const nlohmann::json& j, LocalMemoryType& m) {
    const auto str = j.get<std::string>();
    if (str == "ram") {
        m = LocalMemoryType::ram;
    } else if (str == "reg_buffer") {
        m = LocalMemoryType::reg_buffer;
    } else {
        m = LocalMemoryType::other;
    }
}

void to_json(nlohmann::json& j, const SIMDInputType& m) {
    j = m._to_string();
}

void from_json(const nlohmann::json& j, SIMDInputType& m) {
    const auto str = j.get<std::string>();
    if (str == "vector") {
        m = SIMDInputType::vector;
    } else if (str == "scalar") {
        m = SIMDInputType::scalar;
    } else {
        m = SIMDInputType::other;
    }
}

void to_json(nlohmann::json& j, const PimSRAMAddressSpaceContinuousMode& m) {
    j = m._to_string();
}

void from_json(const nlohmann::json& j, PimSRAMAddressSpaceContinuousMode& m) {
    const auto str = j.get<std::string>();
    if (str == "intergroup") {
        m = PimSRAMAddressSpaceContinuousMode::intergroup;
    } else if (str == "intragroup") {
        m = PimSRAMAddressSpaceContinuousMode::intragroup;
    } else {
        m = PimSRAMAddressSpaceContinuousMode::other;
    }
}

}  // namespace pimsim