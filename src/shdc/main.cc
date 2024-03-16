/*
    sokol-shdc main source file.
*/
#include "formats/bare.h"
#include "formats/sokol.h"
#include "formats/sokolnim.h"
#include "formats/sokolodin.h"
#include "formats/sokolrust.h"
#include "formats/sokolzig.h"
#include "formats/yaml.h"

using namespace shdc;

int main(int argc, const char** argv) {
    spirv_t::initialize_spirv_tools();

    // parse command line args
    args_t args = args_t::parse(argc, argv);
    if (args.debug_dump) {
        args.dump_debug();
    }
    if (!args.valid) {
        return args.exit_code;
    }

    // load the source and parse tagged blocks
    input_t inp = input_t::load_and_parse(args.input, args.module);
    if (args.debug_dump) {
        inp.dump_debug(args.error_format);
    }
    if (inp.out_error.has_error) {
        inp.out_error.print(args.error_format);
        return 10;
    }

    // compile source snippets to SPIRV blobs
    std::array<spirv_t,slang_t::NUM> spirv;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t)i;
        if (args.slang & slang_t::bit(slang)) {
            spirv[i] = spirv_t::compile_glsl(inp, slang, args.defines);
            if (args.debug_dump) {
                spirv[i].dump_debug(inp, args.error_format);
            }
            if (!spirv[i].errors.empty()) {
                bool has_errors = false;
                for (const ErrMsg& err: spirv[i].errors) {
                    if (err.type == ErrMsg::ERROR) {
                        has_errors = true;
                    }
                    err.print(args.error_format);
                }
                if (has_errors) {
                    return 10;
                }
            }
            if (args.save_intermediate_spirv) {
                if (!spirv[i].write_to_file(args, inp, slang)) {
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
            if (spirvcross[i].error.has_error) {
                spirvcross[i].error.print(args.error_format);
                return 10;
            }
        }
    }

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
                    for (const ErrMsg& err: bytecode[i].errors) {
                        if (err.type == ErrMsg::ERROR) {
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

    // generate output files
    ErrMsg output_err;
    switch (args.output_format) {
        case format_t::BARE:
            output_err = bare_t::gen(args, inp, spirvcross, bytecode);
            break;
        case format_t::BARE_YAML:
            output_err = yaml_t::gen(args, inp, spirvcross, bytecode);
            break;
        case format_t::SOKOL_ZIG:
            output_err = sokolzig_t::gen(args, inp, spirvcross, bytecode);
            break;
        case format_t::SOKOL_NIM:
            output_err = sokolnim_t::gen(args, inp, spirvcross, bytecode);
            break;
        case format_t::SOKOL_ODIN:
            output_err = sokolodin_t::gen(args, inp, spirvcross, bytecode);
            break;
        case format_t::SOKOL_RUST:
            output_err = sokolrust_t::gen(args, inp, spirvcross, bytecode);
            break;
        default:
            output_err = sokol_t::gen(args, inp, spirvcross, bytecode);
            break;
    }
    if (output_err.has_error) {
        output_err.print(args.error_format);
        return 10;
    }

    // success
    spirv_t::finalize_spirv_tools();
    return 0;
}
