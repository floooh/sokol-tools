/*
    Generates the final sokol_gfx.h compatible C header with
    embedded shader source/byte code, uniform block structs
    and sg_shader_desc structs.
*/
#include "types.h"
#include "fmt/format.h"

namespace shdc {

header_t header_t::build(const input_t& inp, const spirvcross_t& spirvcross, const bytecode_t& bytecode, const std::string path) {
    fmt::print(stderr, "header_t::build(): FIXME!\n");
    return header_t();
}

void header_t::dump_debug() const {
    fmt::print(stderr, "header_t::dump_debug(): FIXME!\n");
}

} // namespace shdc
