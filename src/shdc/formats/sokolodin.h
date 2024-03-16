#pragma once
#include <array>
#include "args.h"
#include "bytecode.h"
#include "input.h"
#include "spirvcross.h"
#include "types/errmsg.h"
#include "types/slang.h"

namespace shdc {

struct sokolodin_t {
    static ErrMsg gen(const Args& args, const input_t& inp, const std::array<spirvcross_t,Slang::NUM>& spirvcross, const std::array<bytecode_t,Slang::NUM>& bytecode);
};

} // namespace shdc
