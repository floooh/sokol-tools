#pragma once
#include "input.h"
#include "args.h"

namespace shdc::util {

ErrMsg write_dep_file(const std::string& dep_file_path, const Input& inp);

} // namespace shdc::util
