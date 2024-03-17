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
    Spirv::initialize_spirv_tools();

    // parse command line args
    const Args args = Args::parse(argc, argv);
    if (args.debug_dump) {
        args.dump_debug();
    }
    if (!args.valid) {
        return args.exit_code;
    }

    // load the source and parse tagged blocks
    const Input inp = Input::load_and_parse(args.input, args.module);
    if (args.debug_dump) {
        inp.dump_debug(args.error_format);
    }
    if (inp.out_error.valid()) {
        inp.out_error.print(args.error_format);
        return 10;
    }

    // compile source snippets to SPIRV blobs (multiple compilations is necessary
    // because of conditional compilation by target language)
    std::array<Spirv,Slang::NUM> spirv;
    for (int i = 0; i < Slang::NUM; i++) {
        Slang::Enum slang = (Slang::Enum)i;
        if (args.slang & Slang::bit(slang)) {
            spirv[i] = Spirv::compile_glsl(inp, slang, args.defines);
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
    std::array<Spirvcross,Slang::NUM> spirvcross;
    for (int i = 0; i < Slang::NUM; i++) {
        Slang::Enum slang = (Slang::Enum)i;
        if (args.slang & Slang::bit(slang)) {
            spirvcross[i] = Spirvcross::translate(inp, spirv[i], slang);
            if (args.debug_dump) {
                spirvcross[i].dump_debug(args.error_format, slang);
            }
            if (spirvcross[i].error.valid()) {
                spirvcross[i].error.print(args.error_format);
                return 10;
            }
        }
    }

    // compile shader-byte code if requested (HLSL / Metal)
    std::array<Bytecode, Slang::NUM> bytecode;
    if (args.byte_code) {
        for (int i = 0; i < Slang::NUM; i++) {
            Slang::Enum slang = (Slang::Enum)i;
            if (args.slang & Slang::bit(slang)) {
                bytecode[i] = Bytecode::compile(args, inp, spirvcross[i], slang);
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
        case Format::BARE:
            output_err = formats::bare::gen(args, inp, spirvcross, bytecode);
            break;
        case Format::BARE_YAML:
            output_err = formats::yaml::gen(args, inp, spirvcross, bytecode);
            break;
        case Format::SOKOL_ZIG:
            output_err = formats::sokolzig::gen(args, inp, spirvcross, bytecode);
            break;
        case Format::SOKOL_NIM:
            output_err = formats::sokolnim::gen(args, inp, spirvcross, bytecode);
            break;
        case Format::SOKOL_ODIN:
            output_err = formats::sokolodin::gen(args, inp, spirvcross, bytecode);
            break;
        case Format::SOKOL_RUST:
            output_err = formats::sokolrust::gen(args, inp, spirvcross, bytecode);
            break;
        default:
            output_err = formats::sokol::gen(args, inp, spirvcross, bytecode);
            break;
    }
    if (output_err.valid()) {
        output_err.print(args.error_format);
        return 10;
    }

    // success
    Spirv::finalize_spirv_tools();
    return 0;
}
