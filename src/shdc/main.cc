/*
    sokol-shdc main source file.
*/
#include "shdc.h"

using namespace shdc;

int main(int argc, const char** argv) {

    // parse command line args
    args_t args = args_t::parse(argc, argv);
    if (args.debug_dump) {
        args.dump_debug();
    }
    if (!args.valid) {
        return args.exit_code;
    }

    // load the source and parse tagged blocks
    input_t inp = input_t::load_and_parse(args.input);
    if (args.debug_dump) {
        inp.dump_debug();
    }
    if (inp.error.valid) {
        inp.error.print(args.error_format);
        return 10;
    }

    // compile source snippets to SPIRV blobs
    spirv_t::initialize_spirv_tools();
    spirv_t spirv = spirv_t::compile_glsl(inp);
    if (args.debug_dump) {
        spirv.dump_debug(inp);
    }
    if (!spirv.errors.empty()) {
        for (const error_t& err : spirv.errors) {
            err.print(args.error_format);
        }
        return 10;
    }
    spirv_t::finalize_spirv_tools();

    // cross-translate SPIRV to shader dialects
    spirvcross_t spirvcross = spirvcross_t::translate(inp, spirv, args.slang);
    if (args.debug_dump) {
        spirvcross.dump_debug();
    }
    if (spirvcross.error.valid) {
        spirvcross.error.print(args.error_format);
        return 10;
    }

    // compile shader-byte code if requested (HLSL / Metal)
    bytecode_t bytecode = bytecode_t::compile(inp, spirvcross, args.byte_code);
    if (args.debug_dump) {
        bytecode.dump_debug();
    }
    if (bytecode.error.valid) {
        bytecode.error.print(args.error_format);
        return 10;
    }

    // generate the output C header
    header_t header = header_t::build(inp, spirvcross, bytecode, args.output);
    if (args.debug_dump) {
        header.dump_debug();
    }
    if (header.error.valid) {
        header.error.print(args.error_format);
        return 10;
    }

    // success
    return 0;
}

