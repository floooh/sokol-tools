#pragma once
#include "errmsg.h"
// FIXME! reflection shouldn't be nested in spirvcross_source_t
#include "reflection.h"

namespace shdc {

// result of a spirv-cross compilation
struct spirvcross_source_t {
    bool valid = false;
    int snippet_index = -1;
    std::string source_code;
    errmsg_t error;
    reflection_t refl;
};

} // namespace shdc
