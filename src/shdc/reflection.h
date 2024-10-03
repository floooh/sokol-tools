#pragma once
#include <string>
#include <array>
#include <vector>
#include "spirv_cross.hpp"
#include "spirvcross.h"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/snippet.h"
#include "types/reflection/stage_attr.h"
#include "types/reflection/stage_reflection.h"
#include "types/reflection/program_reflection.h"
#include "types/reflection/type.h"

namespace shdc::refl {

struct Reflection {
    std::vector<ProgramReflection> progs;
    std::vector<StageAttr> unique_vs_inputs;
    Bindings bindings;
    ErrMsg error;

    // build merged reflection object from per-slang / per-snippet reflections, error will be in .error
    static Reflection build(const Args& args, const Input& inp, const std::array<Spirvcross,Slang::Num>& spirvcross);
    // parse per-snippet reflection info for a compiled shader source
    static StageReflection parse_snippet_reflection(const spirv_cross::Compiler& compiler, const Snippet& snippet, ErrMsg& out_error);

FIXME: add a static helper function which returns binding base by slang, stage and resource type

    // print a debug dump to stderr
    void dump_debug(ErrMsg::Format err_fmt) const;

private:
    // create a set of unique vertex shader inputs across all programs
    static std::vector<StageAttr> merge_vs_inputs(const std::vector<ProgramReflection>& progs, ErrMsg& out_error);
    // create a set of unique resource bindings from shader snippet input bindings
    static Bindings merge_bindings(const std::vector<Bindings>& in_bindings, ErrMsg& out_error);
    // parse a struct
    static Type parse_toplevel_struct(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& res, ErrMsg& out_error);
    // parse a struct item
    static Type parse_struct_item(const spirv_cross::Compiler& compiler, const spirv_cross::TypeID& type_id, const spirv_cross::TypeID& base_type_id, uint32_t item_index, ErrMsg& out_error);
};

} // namespace reflection
