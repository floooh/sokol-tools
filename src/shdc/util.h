#pragma once
#include "input.h"
#include "args.h"

namespace shdc::util {

ErrMsg write_dep_file(const Args& args, const Input& inp);
int first_snippet_line_index_skipping_include_blocks(const Input& inp, const Snippet& snippet);

} // namespace shdc::util
