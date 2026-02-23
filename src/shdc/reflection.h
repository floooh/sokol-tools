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
    Bindings bindings;
    std::vector<Type> sbuf_structs;
    ErrMsg error;

    // build merged reflection object from per-slang / per-snippet reflections, error will be in .error
    static Reflection build(const Args& args, const Input& inp, const std::array<Spirvcross,Slang::Num>& spirvcross);
    // parse per-snippet reflection info for a compiled shader source
    static StageReflection parse_snippet_reflection(const spirv_cross::Compiler& compiler, const Snippet& snippet, const Input& inp, const BindSlotMap& bindslot_map, ErrMsg& out_error);
    // print a debug dump to stderr
    void dump_debug(ErrMsg::Format err_fmt) const;

    // helper: convert SPIRType to ImageType
    static ImageType::Enum spirtype_to_image_type(const spirv_cross::SPIRType& type);

private:
    // create a set of unique resource bindings from shader snippet input bindings
    static Bindings merge_bindings(const std::vector<Bindings>& in_bindings, bool to_prog_bindings, ErrMsg& out_error);
    // create a set of unique storage buffer structs from merged bindings (result of merge_bindings())
    static std::vector<Type> merge_storagebuffer_structs(const Bindings& merged_bindings, ErrMsg& out_error);
    // parse a struct
    static Type parse_toplevel_struct(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& res, ErrMsg& out_error);
    // parse a struct item
    static Type parse_struct_item(const spirv_cross::Compiler& compiler, const spirv_cross::TypeID& type_id, const spirv_cross::TypeID& base_type_id, uint32_t item_index, ErrMsg& out_error);
    // check for valid sokol bindslot ranges and binding collisions across shader stages
    static ErrMsg validate_program_bindings(const Bindings& bindings);
    // helper function to find a
    static const Type* find_struct_by_typename(const std::vector<Type>& structs, const std::string& struct_typename);
};

} // namespace reflection
