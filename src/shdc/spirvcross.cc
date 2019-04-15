/*
    translate SPIRV bytecode to shader sources and generate
    uniform block reflection information, wrapper around
    https://github.com/KhronosGroup/SPIRV-Cross
*/
#include "shdc.h"
#include "fmt/format.h"

namespace shdc {

spirvcross_t spirvcross_t::translate(const input_t& inp, const spirv_t& spirv, uint32_t slang_mask) {
    fmt::print(stderr, "spirvcross_t::translate(): FIXME!\n");
    return spirvcross_t();
}

void spirvcross_t::dump_debug() const {
    fmt::print(stderr, "spirvcross_t::dump_debug(): FIXME!\n");
}

} // namespace shdc

