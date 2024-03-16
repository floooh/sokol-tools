#pragma once
#include <vector>
#include "spirv_cross.hpp"
#include "input.h"
#include "spirv.h"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/spirvcross_source.h"
#include "types/reflection/uniform_block.h"
#include "types/reflection/image.h"
#include "types/reflection/sampler.h"

namespace shdc {

// spirv-cross wrapper
struct Spirvcross {
    ErrMsg error;
    std::vector<SpirvcrossSource> sources;
    // FIXME: this doesn't belong here
    std::vector<refl::UniformBlock> unique_uniform_blocks;
    std::vector<refl::Image> unique_images;
    std::vector<refl::Sampler> unique_samplers;

    static Spirvcross translate(const Input& inp, const Spirv& spirv, Slang::Enum slang);
    static bool can_flatten_uniform_block(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& ub_res);
    int find_source_by_snippet_index(int snippet_index) const;
    void dump_debug(ErrMsg::Format err_fmt, Slang::Enum slang) const;
};


} // namespace shdc
