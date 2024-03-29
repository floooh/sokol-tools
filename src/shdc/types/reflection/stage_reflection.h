#pragma once
#include <string>
#include <array>
#include "shader_stage.h"
#include "stage_attr.h"
#include "bindings.h"

namespace shdc::refl {

struct StageReflection {
    int snippet_index = -1;
    std::string snippet_name;
    ShaderStage::Enum stage = ShaderStage::Invalid;
    std::string stage_name;                             // same as ShaderStage::to_str(stage)
    std::string entry_point;
    std::array<StageAttr, StageAttr::Num> inputs;       // index == attribute slot
    std::array<StageAttr, StageAttr::Num> outputs;      // index == attribute slot
    Bindings bindings;
};

} // namespace
