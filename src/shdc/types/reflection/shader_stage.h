#pragma once
#include "types/snippet.h"

namespace shdc::refl {

// a shader stage
struct ShaderStage {
    enum Enum {
        Vertex = 0,
        Fragment,
        Compute,
        Num,
        Invalid,
    };
    static const char* to_str(Enum e);
    static Enum from_index(int idx);
    static Enum from_snippet_type(Snippet::Type t);
    static bool is_vs(Enum e);
    static bool is_fs(Enum e);
    static bool is_cs(Enum e);
};

inline const char* ShaderStage::to_str(ShaderStage::Enum e) {
    switch (e) {
        case Vertex: return "vertex";
        case Fragment: return "fragment";
        case Compute: return "compute";
        default: return "invalid";
    }
}

inline ShaderStage::Enum ShaderStage::from_snippet_type(Snippet::Type t) {
    switch (t) {
        case Snippet::Type::VS: return ShaderStage::Vertex;
        case Snippet::Type::FS: return ShaderStage::Fragment;
        case Snippet::Type::CS: return ShaderStage::Compute;
        default: return ShaderStage::Vertex;
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

inline bool ShaderStage::is_cs(ShaderStage::Enum e) {
    return Compute == e;
}

} // namespace
