#pragma once
#include "errmsg.h"
#include "reflection/stage_reflection.h"

namespace shdc {

// spirv-cross output for one shader source snippet
struct SpirvcrossSource {
    bool valid = false;
    int snippet_index = -1;
    std::string source_code;
    ErrMsg error;
    refl::StageReflection stage_refl;
};

} // namespace shdc
