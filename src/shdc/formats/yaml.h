#pragma once
#include <array>
#include "args.h"
#include "bytecode.h"
#include "input.h"
#include "spirvcross.h"
#include "types/errmsg.h"
#include "types/slang.h"

namespace shdc {

struct yaml_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

} // namespace shdc
