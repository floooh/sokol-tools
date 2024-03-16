#pragma once
#include <vector>
#include "args.h"
#include "input.h"
#include "spirvcross.h"
#include "types/bytecode_blob.h"
#include "types/errmsg.h"
#include "types/slang.h"

namespace shdc {

struct bytecode_t {
    std::vector<ErrMsg> errors;
    std::vector<BytecodeBlob> blobs;

    static bytecode_t compile(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross, Slang::type_t slang);
    int find_blob_by_snippet_index(int snippet_index) const;
    void dump_debug() const;
};



} // namespace shdc
