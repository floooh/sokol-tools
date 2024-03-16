#pragma once
#include <vector>
#include <string>
#include "args.h"
#include "input.h"
#include "types/errmsg.h"
#include "types/spirv_blob.h"
#include "types/slang.h"

namespace shdc {

// glsl-to-spirv compiler wrapper
struct spirv_t {
    std::vector<ErrMsg> errors;
    std::vector<SpirvBlob> blobs;

    static void initialize_spirv_tools();
    static void finalize_spirv_tools();
    static spirv_t compile_glsl(const Input& inp, Slang::type_t slang, const std::vector<std::string>& defines);
    bool write_to_file(const Args& args, const Input& inp, Slang::type_t slang);
    void dump_debug(const Input& inp, ErrMsg::msg_format_t err_fmt) const;
};

} // namespace shdc
