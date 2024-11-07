#pragma once
#include <vector>
#include <string>
#include "args.h"
#include "input.h"
#include "types/errmsg.h"
#include "types/spirv_blob.h"
#include "types/slang.h"

namespace shdc {

// glslang SPIRV output of all shader source snippets for one shading language
struct Spirv {
    std::vector<ErrMsg> errors;
    std::vector<SpirvBlob> blobs;

    static void initialize_spirv_tools();
    static void finalize_spirv_tools();
    static Spirv compile_glsl_and_extract_bindings(Input& inp, Slang::Enum slang, const std::vector<std::string>& defines);
    bool write_to_file(const Args& args, const Input& inp, Slang::Enum slang);
    void dump_debug(const Input& inp, ErrMsg::Format err_fmt) const;
};

} // namespace shdc
