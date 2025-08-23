#pragma once
#include <array>
#include "stage_reflection.h"

namespace shdc::refl {

struct ProgramReflection {
    std::string name;
    std::array<StageReflection, ShaderStage::Num> stages;
    Bindings bindings;  // merged stage bindings

    const StageReflection& stage(ShaderStage::Enum s) const;
    bool has_vs() const;
    bool has_fs() const;
    bool has_cs() const;
    const StageReflection& vs() const;
    const StageReflection& fs() const;
    const StageReflection& cs() const;
    const std::string& vs_name() const;
    const std::string& fs_name() const;
    const std::string& cs_name() const;
    void dump_debug(const std::string& indent) const;
};

inline const StageReflection& ProgramReflection::stage(ShaderStage::Enum s) const {
    assert((s >= 0) && (s < ShaderStage::Num));
    return stages[s];
}

inline bool ProgramReflection::has_vs() const {
    return stages[ShaderStage::Vertex].stage != ShaderStage::Invalid;
}

inline bool ProgramReflection::has_fs() const {
    return stages[ShaderStage::Fragment].stage != ShaderStage::Invalid;
}

inline bool ProgramReflection::has_cs() const {
    return stages[ShaderStage::Compute].stage != ShaderStage::Invalid;
}

inline const StageReflection& ProgramReflection::vs() const {
    return stages[ShaderStage::Vertex];
}

inline const StageReflection& ProgramReflection::fs() const {
    return stages[ShaderStage::Fragment];
}

inline const StageReflection& ProgramReflection::cs() const {
    return stages[ShaderStage::Compute];
}

inline const std::string& ProgramReflection::vs_name() const {
    return stages[ShaderStage::Vertex].snippet_name;
}

inline const std::string& ProgramReflection::fs_name() const {
    return stages[ShaderStage::Fragment].snippet_name;
}

inline const std::string& ProgramReflection::cs_name() const {
    return stages[ShaderStage::Compute].snippet_name;
}

inline void ProgramReflection::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}stages:\n", indent2);
    bindings.dump_debug(indent2);
    if (has_vs()) {
        vs().dump_debug(indent2);
    }
    if (has_fs()) {
        fs().dump_debug(indent2);
    }
    if (has_cs()) {
        cs().dump_debug(indent2);
    }
}

} // namespace
