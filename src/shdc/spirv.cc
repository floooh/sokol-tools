/*
    compile GLSL to SPIRV, wrapper around https://github.com/KhronosGroup/glslang
*/
#include "types.h"
#include "fmt/format.h"

namespace shdc {

spirv_t spirv_t::compile_glsl(const input_t& inp) {
    fmt::print(stderr, "spirv_t::compile_glsl(): FIXME!\n");
    return spirv_t();
}

void spirv_t::dump_debug() const {
    fmt::print(stderr, "spirv_t::dump_debug(): FIXME!\n");
}

} // namespace shdc
