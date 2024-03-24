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

} // namespace
