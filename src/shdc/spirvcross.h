#pragma once
#include <vector>
#include "spirv_cross.hpp"
#include "input.h"
#include "spirv.h"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/spirvcross_source.h"
#include "types/uniform_block.h"
#include "types/image.h"
#include "types/sampler.h"

namespace shdc {

// spirv-cross wrapper
struct spirvcross_t {
    ErrMsg error;
    std::vector<SpirvcrossSource> sources;
    // FIXME: those should go into reflection?
    std::vector<UniformBlock> unique_uniform_blocks;
    std::vector<Image> unique_images;
    std::vector<Sampler> unique_samplers;

    static spirvcross_t translate(const Input& inp, const spirv_t& spirv, Slang::type_t slang);
    static bool can_flatten_uniform_block(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& ub_res);
    int find_source_by_snippet_index(int snippet_index) const;
    void dump_debug(ErrMsg::msg_format_t err_fmt, Slang::type_t slang) const;
};


} // namespace shdc
