#pragma once
#include <vector>
#include "args.h"
#include "input.h"
#include "spirvcross.h"
#include "types/bytecode_blob.h"
#include "types/errmsg.h"
#include "types/slang.h"

namespace shdc {

struct Bytecode {
    std::vector<ErrMsg> errors;
    std::vector<BytecodeBlob> blobs;

    static Bytecode compile(const Args& args, const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang);
    int find_blob_by_snippet_index(int snippet_index) const;
    void dump_debug() const;
};

} // namespace shdc
