//
// Created by wyk on 2024/6/19.
//

#pragma once

#include "nlohmann/json.hpp"

namespace pimsim {

#define DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(Type)                             \
    friend void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t); \
    friend void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t);

#define DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(Type)                  \
    void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t); \
    void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t);

}  // namespace pimsim
