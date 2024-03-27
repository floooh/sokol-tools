#pragma once

namespace shdc::refl {

// a shader stage
struct ShaderStage {
    enum Enum {
        Vertex = 0,
        Fragment,
        Num,
        Invalid,
    };
    static const char* to_str(Enum e);
    static Enum from_index(int idx);
    static bool is_vs(Enum e);
    static bool is_fs(Enum e);
};

inline const char* ShaderStage::to_str(ShaderStage::Enum e) {
    switch (e) {
        case Vertex: return "VS";
        case Fragment: return "FS";
        default: return "INVALID";
    }
}

inline ShaderStage::Enum ShaderStage::from_index(int idx) {
    assert((idx >= 0) && (idx < Num));
    return (ShaderStage::Enum)idx;
}

inline bool ShaderStage::is_vs(ShaderStage::Enum e) {
    return Vertex == e;
}

inline bool ShaderStage::is_fs(ShaderStage::Enum e) {
    return Fragment == e;
}

} // namespace
