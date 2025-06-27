#pragma once
#include "input.h"
#include "args.h"

namespace shdc::util {

ErrMsg write_dep_file(const Args& args, const Input& inp);

} // namespace shdc::util
