#pragma once
#include <string>
#include <array>
#include <vector>
#include "spirv_cross.hpp"
#include "spirvcross.h"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/snippet.h"
#include "types/reflection/stage_reflection.h"
#include "types/reflection/program_reflection.h"

namespace shdc::refl {

struct Reflection {
    std::vector<ProgramReflection> progs;
    Bindings bindings;
    ErrMsg error;

    // build merged reflection object from per-slang / per-snippet reflections, error will be in .error
    static Reflection build(const Args& args, const Input& inp, const std::array<Spirvcross,Slang::Num>& spirvcross);
    // parse per-snippet reflection info for a compiled shader source
    static StageReflection parse_snippet_reflection(const spirv_cross::Compiler& compiler, const Snippet& snippet, Slang::Enum slang);

private:
    // create a set of unique resource bindings from shader snippet input bindings
    static Bindings merge_bindings(const std::vector<Bindings>& in_bindings, const std::string& inp_base_path, ErrMsg& out_error);
};

} // namespace reflection
