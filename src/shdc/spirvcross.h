#pragma once
#include <vector>
#include "spirv_cross.hpp"
#include "input.h"
#include "spirv.h"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/spirvcross_source.h"
#include "types/reflection/bindings.h"

namespace shdc {

// SPIRVCross output for all shader snippets of one target language
struct Spirvcross {
    ErrMsg error;
    std::vector<SpirvcrossSource> sources;

    static Spirvcross translate(const Input& inp, const Spirv& spirv, Slang::Enum slang);
    static bool can_flatten_uniform_block(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& ub_res);
    const SpirvcrossSource* find_source_by_snippet_index(int snippet_index) const;
    void dump_debug(ErrMsg::Format err_fmt, Slang::Enum slang) const;
};


} // namespace shdc
