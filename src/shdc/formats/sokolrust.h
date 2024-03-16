#pragma once
#include <array>
#include "args.h"
#include "bytecode.h"
#include "input.h"
#include "spirvcross.h"
#include "types/errmsg.h"
#include "types/slang.h"

namespace shdc {

struct sokolrust_t {
    static ErrMsg gen(const Args& args, const Input& inp, const std::array<spirvcross_t,Slang::NUM>& spirvcross, const std::array<Bytecode,Slang::NUM>& bytecode);
};

} // namespace shdc
