#pragma once
#include <array>
#include "args.h"
#include "bytecode.h"
#include "input.h"
#include "spirvcross.h"
#include "types/errmsg.h"
#include "types/slang.h"
#include "types/reflection/bindings.h"

namespace shdc::formats::bare {

ErrMsg gen(const Args& args,
    const Input& inp,
    const std::array<Spirvcross,Slang::NUM>& spirvcross,
    const std::array<Bytecode,Slang::NUM>& bytecode);

}
