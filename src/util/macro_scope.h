//
// Created by wyk on 2024/6/19.
//

#pragma once

#include "nlohmann/json.hpp"

namespace pimsim {

#define DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(Type)     \
    friend void to_json(nlohmann::ordered_json&, const Type&); \
    friend void from_json(const nlohmann::ordered_json&, Type&);

#define DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(Type) \
    void to_json(nlohmann::ordered_json&, const Type&);        \
    void from_json(const nlohmann::ordered_json&, Type&);

#define DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(Type, ...)                               \
    void to_json(nlohmann::ordered_json& nlohmann_json_j, const Type& nlohmann_json_t) {        \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                \
    }                                                                                           \
    void from_json(const nlohmann::ordered_json& nlohmann_json_j, Type& nlohmann_json_t) {      \
        const Type nlohmann_json_default_obj{};                                                 \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_WITH_DEFAULT, __VA_ARGS__)) \
    }

#define DEFINE_TYPE_FROM_JSON_FUNCTION_WITH_DEFAULT(Type, ...)                                  \
    void from_json(const nlohmann::ordered_json& nlohmann_json_j, Type& nlohmann_json_t) {      \
        const Type nlohmann_json_default_obj{};                                                 \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_WITH_DEFAULT, __VA_ARGS__)) \
    }

#define DEFINE_ENUM_FROM_TO_JSON_FUNCTION(EnumType, type1, type2, type_other) \
    void to_json(nlohmann::ordered_json& j, const EnumType& m) {              \
        j = m._to_string();                                                   \
    }                                                                         \
    void from_json(const nlohmann::ordered_json& j, EnumType& m) {            \
        const auto str = j.get<std::string>();                                \
        if (str == #type1) {                                                  \
            m = EnumType::type1;                                              \
        } else if (str == #type2) {                                           \
            m = EnumType::type2;                                              \
        } else {                                                              \
            m = EnumType::type_other;                                         \
        }                                                                     \
    }

}  // namespace pimsim
