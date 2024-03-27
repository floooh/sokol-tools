#pragma once
#include <string>
#include "input.h"
#include "spirvcross.h"
#include "bytecode.h"
#include "types/slang.h"
#include "types/reflection/uniform.h"

namespace shdc::gen::util {

// utility functions for generators
ErrMsg check_errors(const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang);
const char* slang_file_extension(Slang::Enum c, bool binary);
int roundup(int val, int round_to);
std::string mod_prefix(const Input& inp);
const SpirvcrossSource* find_spirvcross_source_by_shader_name(const std::string& shader_name, const Input& inp, const Spirvcross& spirvcross);
const BytecodeBlob* find_bytecode_blob_by_shader_name(const std::string& shader_name, const Input& inp, const Bytecode& bytecode);

} // namespace
