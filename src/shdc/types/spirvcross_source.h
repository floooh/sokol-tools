#pragma once
#include "errmsg.h"
// FIXME! reflection shouldn't be nested in SpirvcrossSource
#include "reflection.h"

namespace shdc {

// result of a spirv-cross compilation
struct SpirvcrossSource {
    bool valid = false;
    int snippet_index = -1;
    std::string source_code;
    ErrMsg error;
    // FIXME: this doesn't belong here
    refl::Reflection refl;
};

} // namespace shdc
