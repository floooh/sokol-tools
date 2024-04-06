#pragma once
#include <string>
#include <vector>
#include "fmt/format.h"

namespace shdc::refl {

struct Type {
    enum Enum {
        Invalid,
        Bool,
        Bool2,
        Bool3,
        Bool4,
        Int,
        Int2,
        Int3,
        Int4,
        UInt,
        UInt2,
        UInt3,
        UInt4,
        Float,
        Float2,
        Float3,
        Float4,
        Mat2x1,
        Mat2x2,
        Mat2x3,
        Mat2x4,
        Mat3x1,
        Mat3x2,
        Mat3x3,
        Mat3x4,
        Mat4x1,
        Mat4x2,
        Mat4x3,
        Mat4x4,
        Struct,
    };
    std::string name;
    std::string struct_typename;
    Enum type = Invalid;
    bool is_matrix = false;
    bool is_array = false;
    int offset = 0;
    int size = 0;
    int matrix_stride = 0;  // only set when columns > 1
    int array_count = 0;    // this can be zero for unbounded arrays
    int array_stride = 0;
    std::vector<Type> struct_items;

    bool equals(const Type& other) const;
    std::string type_as_str() const;
    std::string type_as_glsl() const;
    static std::string type_to_str(Type::Enum e);
    static std::string type_to_glsl(Type::Enum e);
    void dump_debug(const std::string& indent) const;
    static bool is_valid_glsl_type(const std::string& str);
    static std::string valid_glsl_types_as_str();
};

inline bool Type::equals(const Type& other) const {
    if (name != other.name) {
        return false;
    }
    if (type != other.type) {
        return false;
    }
    if (offset != other.offset) {
        return false;
    }
    if (size != other.size) {
        return false;
    }
    if (matrix_stride != other.matrix_stride) {
        return false;
    }
    if (is_array != other.is_array) {
        return false;
    }
    if (array_count != other.array_count) {
        return false;
    }
    if (array_stride != other.array_stride) {
        return false;
    }
    if (struct_items.size() != other.struct_items.size()) {
        return false;
    }
    for (size_t i = 0; i < struct_items.size(); i++) {
        if (!struct_items[i].equals(other.struct_items[i])) {
            return false;
        }
    }
    return true;
}

inline std::string Type::type_as_str() const {
    return type_to_str(type);
}

inline std::string Type::type_to_str(Type::Enum e) {
    switch (e) {
        case Bool:      return "Bool";
        case Bool2:     return "Bool22";
        case Bool3:     return "Bool3";
        case Bool4:     return "Bool4";
        case Int:       return "Int";
        case Int2:      return "Int2";
        case Int3:      return "Int3";
        case Int4:      return "Int4";
        case UInt:      return "UInt";
        case UInt2:     return "UInt2";
        case UInt3:     return "UInt3";
        case UInt4:     return "UInt4";
        case Float:     return "Float";
        case Float2:    return "Float2";
        case Float3:    return "Float3";
        case Float4:    return "Float4";
        case Mat2x1:    return "Mat2x1";
        case Mat2x2:    return "Mat2x2";
        case Mat2x3:    return "Mat2x3";
        case Mat2x4:    return "Mat2x4";
        case Mat3x1:    return "Mat3x1";
        case Mat3x2:    return "Mat3x2";
        case Mat3x3:    return "Mat3x2";
        case Mat3x4:    return "Mat3x4";
        case Mat4x1:    return "Mat4x1";
        case Mat4x2:    return "Mat4x2";
        case Mat4x3:    return "Mat4x3";
        case Mat4x4:    return "Mat4x4";
        case Struct:    return "Struct";
        default:        return "INVALID";
    }
}

inline std::string Type::type_as_glsl() const {
    return type_to_glsl(type);
}

inline std::string Type::type_to_glsl(Type::Enum e) {
    switch (e) {
        case Bool:      return "bool";
        case Bool2:     return "bvec2";
        case Bool3:     return "bvec3";
        case Bool4:     return "bvec4";
        case Int:       return "int";
        case Int2:      return "ivec2";
        case Int3:      return "ivec3";
        case Int4:      return "ivec4";
        case UInt:      return "uint";
        case UInt2:     return "uvec2";
        case UInt3:     return "uvec3";
        case UInt4:     return "uvec4";
        case Float:     return "float";
        case Float2:    return "vec2";
        case Float3:    return "vec3";
        case Float4:    return "vec4";
        case Mat2x1:    return "mat2x1";
        case Mat2x2:    return "mat2";
        case Mat2x3:    return "mat2x3";
        case Mat2x4:    return "mat2x4";
        case Mat3x1:    return "mat3x1";
        case Mat3x2:    return "mat3x2";
        case Mat3x3:    return "mat3";
        case Mat3x4:    return "mat3x4";
        case Mat4x1:    return "mat4x1";
        case Mat4x2:    return "mat4x2";
        case Mat4x3:    return "mat4x3";
        case Mat4x4:    return "mat4";
        default:        return "INVALID";
    }
}

inline bool Type::is_valid_glsl_type(const std::string& str) {
    return (str == "bool") ||
        (str == "bvec2") ||
        (str == "bvec3") ||
        (str == "bvec4") ||
        (str == "int") ||
        (str == "ivec2") ||
        (str == "ivec3") ||
        (str == "ivec4") ||
        (str == "uint") ||
        (str == "uvec2") ||
        (str == "uvec3") ||
        (str == "uvec4") ||
        (str == "float") ||
        (str == "vec2") ||
        (str == "vec3") ||
        (str == "vec4") ||
        (str == "mat2") ||
        (str == "mat3") ||
        (str == "mat4") ||
        (str == "mat2x1") ||
        (str == "mat2x2") ||
        (str == "mat2x3") ||
        (str == "mat2x4") ||
        (str == "mat3x1") ||
        (str == "mat3x2") ||
        (str == "mat3x3") ||
        (str == "mat3x4") ||
        (str == "mat4x1") ||
        (str == "mat4x2") ||
        (str == "mat4x3") ||
        (str == "mat4x4");
}

inline std::string Type::valid_glsl_types_as_str() {
    return "float|vec[2..4]|bool|bvec[2..4]|int|ivec[2..4]|uint|uvec[2..4]|mat[2..4]|mat[2..4]x[1..4]";
}

inline void Type::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}type: {}\n", indent2, type_to_str(type));
    fmt::print(stderr, "{}is_matrix: {}\n", indent2, is_matrix);
    fmt::print(stderr, "{}is_array: {}\n", indent2, is_array);
    fmt::print(stderr, "{}offset: {}\n", indent2, offset);
    fmt::print(stderr, "{}size: {}\n", indent2, size);
    fmt::print(stderr, "{}matrix_stride: {}\n", indent2, matrix_stride);
    fmt::print(stderr, "{}array_count: {}\n", indent2, array_count);
    fmt::print(stderr, "{}array_stride: {}\n", indent2, array_stride);
    if (type == Type::Struct) {
        fmt::print(stderr, "{}struct items:\n", indent2);
        for (const auto& struct_item: struct_items) {
            struct_item.dump_debug(indent2);
        }
    }
}

} // namespace
