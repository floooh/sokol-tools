#pragma once
#include <array>
#include "stage_reflection.h"

namespace shdc::refl {

struct ProgramReflection {
    std::string name;
    std::array<StageReflection, ShaderStage::Num> stages;

    const StageReflection& stage(ShaderStage::Enum s) const;
    const StageReflection& vs() const;
    const StageReflection& fs() const;
    const std::string& vs_name() const;
    const std::string& fs_name() const;
    void dump_debug(const std::string& indent) const;
};

inline const StageReflection& ProgramReflection::stage(ShaderStage::Enum s) const {
    assert((s >= 0) && (s < ShaderStage::Num));
    return stages[s];
}

inline const StageReflection& ProgramReflection::vs() const {
    return stages[ShaderStage::Vertex];
}

inline const StageReflection& ProgramReflection::fs() const {
    return stages[ShaderStage::Fragment];
}

inline const std::string& ProgramReflection::vs_name() const {
    return stages[ShaderStage::Vertex].snippet_name;
}

inline const std::string& ProgramReflection::fs_name() const {
    return stages[ShaderStage::Fragment].snippet_name;
}

inline void ProgramReflection::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}stages:\n", indent2);
    for (const auto& stage: stages) {
        stage.dump_debug(indent2);
    }
}

} // namespace
