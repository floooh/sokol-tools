/*
    Compile HLSL / Metal source code to bytecode, HLSL only works
    when running on Windos, Metal only works when running on macOS.

    Uses d3dcompiler.dll for HLSL, and for Metal, invokes the Metal
    compiler toolchain commandline tools.
*/
#include "types.h"
#include "fmt/format.h"

namespace shdc {

bytecode_t bytecode_t::compile(const input_t& inp, const spirvcross_t& spirvcross, bool gen_bytecode) {
    fmt::print(stderr, "bytecode_t::compile(): FIXME!\n");
    return bytecode_t();
}

void bytecode_t::dump_debug() const {
    fmt::print(stderr, "bytecode_t::dump_debug(): FIXME!\n");
}

} // namespace shdc

