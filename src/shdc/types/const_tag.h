#pragma once
#include <string>
#include <cstdint>

namespace shdc {

// a '@const [type] [name] [value]' tag
struct ConstTag {
    enum Type {
        INVALID,
        INT,
        UINT,
        BOOL,
    };
    Type type = INVALID;
    std::string name;
    int64_t value = 0;
    int line_index = 0;

    ConstTag();
    ConstTag(Type t, const std::string& n, int64_t v, int l);
    static Type type_from_str(const std::string& str);
    static const char* type_to_str(Type t);
    static const char* valid_types_as_str();
    // value as GLSL literal
    std::string value_as_glsl() const;
    // value as plain decimal or 'true'/'false' (for typed const declarations)
    std::string value_as_str() const;
};

inline ConstTag::ConstTag() { };

inline ConstTag::ConstTag(Type t, const std::string& n, int64_t v, int l):
    type(t),
    name(n),
    value(v),
    line_index(l)
{ };

inline ConstTag::Type ConstTag::type_from_str(const std::string& str) {
    if (str == "int") {
        return INT;
    } else if (str == "uint") {
        return UINT;
    } else if (str == "bool") {
        return BOOL;
    } else {
        return INVALID;
    }
}

inline const char* ConstTag::type_to_str(Type t) {
    switch (t) {
        case INT: return "int";
        case UINT: return "uint";
        case BOOL: return "bool";
        default: return "invalid";
    }
}

inline const char* ConstTag::valid_types_as_str() {
    return "int, uint, bool";
}

inline std::string ConstTag::value_as_glsl() const {
    switch (type) {
        case INT:
            return std::to_string(value);
        case UINT:
            return std::to_string(value) + "u";
        case BOOL:
            return value ? "true" : "false";
        default:
            return "INVALID";
    }
}

inline std::string ConstTag::value_as_str() const {
    if (type == BOOL) {
        return value ? "true" : "false";
    } else {
        return std::to_string(value);
    }
}

} // namespace shdc
