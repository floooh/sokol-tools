/*
    sokol-shdc main source file.
*/
#include "spirv.h"
#include "args.h"
#include "input.h"
#include "spirvcross.h"
#include "bytecode.h"
#include "generators/generate.h"

using namespace shdc;
using namespace shdc::refl;
using namespace shdc::gen;

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

    // find and merge identical binding reflections across all compiled shader snippets
    std::vector<Bindings> snippet_bindings;
    for (int i = 0; i < Slang::NUM; i++) {
        Slang::Enum slang = (Slang::Enum)i;
        if (args.slang & Slang::bit(slang)) {
            for (const SpirvcrossSource& src: spirvcross[i].sources) {
                snippet_bindings.push_back(src.refl.bindings);
            }
        }
    }
    ErrMsg merge_error;
    const Bindings merged_bindings = Reflection::merge_bindings(snippet_bindings, inp.base_path, merge_error);
    if (merge_error.valid()) {
        merge_error.print(args.error_format);
        return 10;
    }

    // generate output files
    const GenInput gen_input(args, inp, spirvcross, bytecode, merged_bindings);
    ErrMsg gen_error = generate(args.output_format, gen_input);
    if (gen_error.valid()) {
        gen_error.print(args.error_format);
        return 10;
    }

    // success
    Spirv::finalize_spirv_tools();
    return 0;
}
