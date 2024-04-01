#pragma once
#include <string>
#include <vector>
#include "fmt/format.h"

namespace shdc::refl {

struct Type {
    enum BaseType {
        Invalid,
        Bool,
        Int,
        UInt,
        Float,
        Half,
        Struct,
    };
    std::string name;
    BaseType base_type = Invalid;
    int vecsize = 1;
    int columns = 1;
    int offset = 0;
    int size = 0;
    bool is_array = false;
    int array_count = 0;    // NOTE: is_array == true and array_count == 0 means 'unbounded array'
    int array_stride = 0;
    std::vector<Type> struct_items;

    bool equals(const Type& other) const;
    std::string type_as_glsl() const;
    void dump_debug(const std::string& indent) const;
    static const char* base_type_to_str(BaseType bt);
    static bool is_valid_glsl_type(const std::string& str);
    static std::string valid_glsl_types_as_str();
};

inline bool Type::equals(const Type& other) const {
    if (name != other.name) {
        return false;
    }
    if (base_type != other.base_type) {
        return false;
    }
    if (vecsize != other.vecsize) {
        return false;
    }
    if (columns != other.columns) {
        return false;
    }
    if (offset != other.offset) {
        return false;
    }
    if (size != other.size) {
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

inline const char* Type::base_type_to_str(BaseType bt) {
    switch (bt) {
        case Invalid: return "Invalid";
        case Bool: return "Bool";
        case Int: return "Int";
        case UInt: return "UInt";
        case Float: return "Float";
        case Half: return "Half";
        case Struct: return "Struct";
    }
}

inline std::string Type::type_as_glsl() const {
    switch (base_type) {
        case Bool:
            if (columns == 1) {
                if (vecsize == 1) {
                    return "bool";
                } else {
                    return fmt::format("bvec{}", vecsize);
                }
            }
            break;
        case Int:
            if (columns == 1) {
                if (vecsize == 1) {
                    return "int";
                } else {
                    return fmt::format("ivec{}", vecsize);
                }
            }
            break;
        case UInt:
            if (columns == 1) {
                if (vecsize == 1) {
                    return "uint";
                } else {
                    return fmt::format("uvec{}", vecsize);
                }
            }
            break;
        case Float:
            if (columns == 1) {
                if (vecsize == 1) {
                    return "float";
                } else {
                    return fmt::format("vec{}", vecsize);
                }
            } else {
                if (columns == vecsize) {
                    return fmt::format("mat{}", vecsize);
                } else {
                    // FIXME: order?
                    return fmt::format("mat{}x{}", vecsize, columns);
                }
            }
            break;
        case Half:
            // see https://github.com/KhronosGroup/GLSL/blob/main/extensions/ext/GL_EXT_shader_explicit_arithmetic_types.txt
            if (columns == 1) {
                if (vecsize == 1) {
                    return "float16_t";
                } else {
                    return fmt::format("f16vec{}", vecsize);
                }
            }
            break;
        default:
            break;
    }
    return "INVALID";
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
        (str == "mat2x2") ||
        (str == "mat2x3") ||
        (str == "mat2x4") ||
        (str == "mat3x2") ||
        (str == "mat4x2") ||
        (str == "mat3x3") ||
        (str == "mat3x4") ||
        (str == "mat4x3") ||
        (str == "mat4x4") ||
        (str == "float16_t") ||
        (str == "f16vec2") ||
        (str == "f16vec3") ||
        (str == "f16vec4");
}

inline std::string Type::valid_glsl_types_as_str() {
    return "float|vec[2..4]|bool|bvec[2..4]|int|ivec[2..4]|uint|uvec[2..4]|mat[2..4]|mat[2..4]x[2..4]|float16_t|f16vec[2..4]";
}

inline void Type::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}base_type: {}\n", indent2, Type::base_type_to_str(base_type));
    fmt::print(stderr, "{}vecsize: {}\n", indent2, vecsize);
    fmt::print(stderr, "{}columns: {}\n", indent2, columns);
    fmt::print(stderr, "{}offset: {}\n", indent2, offset);
    fmt::print(stderr, "{}size: {}\n", indent2, size);
    fmt::print(stderr, "{}is_array: {}\n", indent2, is_array);
    fmt::print(stderr, "{}array_count: {}\n", indent2, array_count);
    fmt::print(stderr, "{}array_stride: {}\n", indent2, array_stride);
    if (base_type == Type::Struct) {
        fmt::print(stderr, "{}struct items:\n", indent2);
        for (const auto& struct_item: struct_items) {
            struct_item.dump_debug(indent2);
        }
    }
}

} // namespace
