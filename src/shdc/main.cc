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
        inp.dump_debug(args.error_format);
    }
    if (inp.out_error.valid) {
        inp.out_error.print(args.error_format);
        return 10;
    }

    // compile source snippets to SPIRV blobs
    spirv_t::initialize_spirv_tools();
    std::array<spirv_t,slang_t::NUM> spirv;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t)i;
        if (args.slang & slang_t::bit(slang)) {
            spirv[i] = spirv_t::compile_glsl(inp, slang);
            if (args.debug_dump) {
                spirv[i].dump_debug(inp, args.error_format);
            }
            if (!spirv[i].errors.empty()) {
                bool has_errors = false;
                for (const errmsg_t& err: spirv[i].errors) {
                    if (err.type == errmsg_t::ERROR) {
                        has_errors = true;
                    }
                    err.print(args.error_format);
                }
                if (has_errors) {
                    return 10;
                }
            }
        }
    }

    // cross-translate SPIRV to shader dialects
    std::array<spirvcross_t,slang_t::NUM> spirvcross;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t)i;
        if (args.slang & slang_t::bit(slang)) {
            spirvcross[i] = spirvcross_t::translate(inp, spirv[i], slang);
            if (args.debug_dump) {
                spirvcross[i].dump_debug(args.error_format, slang);
            }
            if (spirvcross[i].error.valid) {
                spirvcross[i].error.print(args.error_format);
                return 10;
            }
        }
    }
    spirv_t::finalize_spirv_tools();

    // compile shader-byte code if requested (HLSL / Metal)
    std::array<bytecode_t, slang_t::NUM> bytecode;
    if (args.byte_code) {
        for (int i = 0; i < slang_t::NUM; i++) {
            slang_t::type_t slang = (slang_t::type_t)i;
            if (args.slang & slang_t::bit(slang)) {
                bytecode[i] = bytecode_t::compile(args, inp, spirvcross[i], slang);
                if (args.debug_dump) {
                    bytecode[i].dump_debug();
                }
                if (!bytecode[i].errors.empty()) {
                    bool has_errors = false;
                    for (const errmsg_t& err: bytecode[i].errors) {
                        if (err.type == errmsg_t::ERROR) {
                            has_errors = true;
                        }
                        err.print(args.error_format);
                    }
                    if (has_errors) {
                        return 10;
                    }
                }
            }
        }
    }

    if (args.output_format == format_t::BARE) {
        errmsg_t err = bare_t::gen(args, inp, spirvcross, bytecode);
        if (err.valid) {
            err.print(args.error_format);
            return 10;
        }
    }
    else {
        // generate the output C header
        errmsg_t err = sokol_t::gen(args, inp, spirvcross, bytecode);
        if (err.valid) {
            err.print(args.error_format);
            return 10;
        }
    }

    // success
    return 0;
}

