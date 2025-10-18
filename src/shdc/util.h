#pragma once
#include "input.h"
#include "args.h"
#include "types/errmsg.h"

namespace shdc::util {

ErrMsg write_dep_file(const Args& args, const Input& inp);
int first_snippet_line_index_skipping_include_blocks(const Input& inp, const Snippet& snippet);
void infolog_to_errors(const std::string& log, const Input& inp, int snippet_index, int linenr_offset, std::vector<ErrMsg>& out_errors);

} // namespace shdc::util
