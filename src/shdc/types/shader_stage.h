#pragma once

namespace shdc {

// a shader stage
struct ShaderStage {
    enum type_t {
        INVALID,
        VS,
        FS,
    };
    static const char* to_str(type_t t);
    static bool is_vs(type_t t);
    static bool is_fs(type_t t);
};

inline const char* ShaderStage::to_str(ShaderStage::type_t t) {
    switch (t) {
        case VS: return "VS";
        case FS: return "FS";
        default: return "INVALID";
    }
}

inline bool ShaderStage::is_vs(ShaderStage::type_t t) {
    return VS == t;
}

inline bool ShaderStage::is_fs(ShaderStage::type_t t) {
    return FS == t;
}

} // namespace shdc