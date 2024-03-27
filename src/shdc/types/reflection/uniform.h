#pragma once
#include <string>

namespace shdc::refl {

struct Uniform {
    static const int Num = 16;      // must be identical with SG_MAX_UB_MEMBERS
    enum Type {
        INVALID,
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        INT,
        INT2,
        INT3,
        INT4,
        MAT4,
    };
    std::string name;
    Type type = INVALID;
    int array_count = 1;
    int offset = 0;

    bool equals(const Uniform& other) const;
    const char* type_as_glsl() const;
    int size_bytes() const;

    static const char* type_to_str(Type t);
    static bool is_valid_glsl_uniform_type(const std::string& str);
    static const char* valid_glsl_uniform_types_as_str();
};

inline const char* Uniform::type_to_str(Type t) {
    switch (t) {
        case FLOAT:     return "float";
        case FLOAT2:    return "float2";
        case FLOAT3:    return "float3";
        case FLOAT4:    return "float4";
        case INT:       return "int";
        case INT2:      return "int2";
        case INT3:      return "int3";
        case INT4:      return "int4";
        case MAT4:      return "mat4";
        default:        return "invalid";
    }
}

inline bool Uniform::is_valid_glsl_uniform_type(const std::string& str) {
    return (str == "float")
        || (str == "vec2")
        || (str == "vec3")
        || (str == "vec4")
        || (str == "int")
        || (str == "int2")
        || (str == "int3")
        || (str == "int4")
        || (str == "mat4");
}

inline const char* Uniform::valid_glsl_uniform_types_as_str() {
    return "float|vec2|vec3|vec4|int|int2|int3|int4|mat4";
}

inline bool Uniform::equals(const Uniform& other) const {
    return (name == other.name) &&
           (type == other.type) &&
           (array_count == other.array_count) &&
           (offset == other.offset);
}

inline const char* Uniform::type_as_glsl() const {
    switch (type) {
        case FLOAT:     return "float";
        case FLOAT2:    return "vec2";
        case FLOAT3:    return "vec3";
        case FLOAT4:    return "vec4";
        case INT:       return "int";
        case INT2:      return "ivec2";
        case INT3:      return "ivec3";
        case INT4:      return "ivec4";
        case MAT4:      return "mat4";
        default:        return "invalid";
    }
}

inline int Uniform::size_bytes(void) const {
    if (array_count > 1) {
        switch (type) {
            case Uniform::FLOAT4:
            case Uniform::INT4:
                return 16 * array_count;
            case Uniform::MAT4:
                return 64 * array_count;
            default: return 0;
        }
    } else {
        switch (type) {
            case Uniform::FLOAT:
            case Uniform::INT:
                return 4;
            case Uniform::FLOAT2:
            case Uniform::INT2:
                return 8;
            case Uniform::FLOAT3:
            case Uniform::INT3:
                return 12;
            case Uniform::FLOAT4:
            case Uniform::INT4:
                return 16;
            case Uniform::MAT4:
                return 64;
            default:
                return 0;
        }
    }
}

} // namespace
