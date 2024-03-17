#pragma once
#include <string>
#include <array>
#include <vector>
#include "spirv_cross.hpp"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/snippet.h"
#include "types/reflection/shader_stage.h"
#include "types/reflection/vertex_attr.h"
#include "types/reflection/bindings.h"

namespace shdc::refl {

struct Reflection {
    ShaderStage::Enum stage = ShaderStage::INVALID;
    std::string entry_point;
    std::array<VertexAttr, VertexAttr::NUM> inputs;
    std::array<VertexAttr, VertexAttr::NUM> outputs;
    Bindings bindings;

    // parse reflection info for a compiled shader source
    static Reflection parse(const spirv_cross::Compiler& compiler, const Snippet& snippet, Slang::Enum slang);
    // create a set of unique resource bindings from shader snippet input bindings
    static bool merge_bindings(const std::vector<Bindings>& in_bindings, const std::string& inp_base_path, Bindings& out_bindings, ErrMsg& out_error);
};

} // namespace reflection
