/*
    parse command line arguments
*/
#include "args.h"
#include "types/slang.h"
#include <vector>
#include <stdio.h>
#include "fmt/format.h"
#include "getopt/getopt.h"
#include "pystring.h"

namespace shdc {

typedef enum {
    OPTION_HELP = 1,
    OPTION_INPUT,
    OPTION_OUTPUT,
    OPTION_SLANG,
    OPTION_BYTECODE,
    OPTION_DEFINES,
    OPTION_MODULE,
    OPTION_FORMAT,
    OPTION_ERRFMT,
    OPTION_DUMP,
    OPTION_GENVER,
    OPTION_TMPDIR,
    OPTION_IFDEF,
    OPTION_NOIFDEF,
    OPTION_REFLECTION,
    OPTION_SAVE_INTERMEDIATE_SPIRV,
} arg_option_t;

static const getopt_option_t option_list[] = {
    { "help",               'h', GETOPT_OPTION_TYPE_NO_ARG,     0, OPTION_HELP,         "print this help text", 0},
    { "input",              'i', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_INPUT,        "input source file", "GLSL file" },
    { "output",             'o', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_OUTPUT,       "output source file", "C header" },
    { "slang",              'l', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_SLANG,        "output shader language(s), see above for list", "glsl430:glsl300es..." },
    { "defines",            0,   GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_DEFINES,      "optional preprocessor defines", "define1:define2..." },
    { "module",             'm', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_MODULE,       "optional @module name override" },
    { "reflection",         'r', GETOPT_OPTION_TYPE_NO_ARG,     0, OPTION_REFLECTION,   "generate runtime reflection functions" },
    { "bytecode",           'b', GETOPT_OPTION_TYPE_NO_ARG,     0, OPTION_BYTECODE,     "output bytecode (HLSL and Metal)"},
    { "format",             'f', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_FORMAT,       "output format (default: sokol)", "[sokol|sokol_decl|sokol_impl|sokol_zig|sokol_nim|sokol_odin|sokol_rust|bare|bare_yaml]" },
    { "errfmt",             'e', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_ERRFMT,       "error message format (default: gcc)", "[gcc|msvc]"},
    { "dump",               'd', GETOPT_OPTION_TYPE_NO_ARG,     0, OPTION_DUMP,         "dump debugging information to stderr"},
    { "genver",             'g', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_GENVER,       "version-stamp for code-generation", "[int]"},
    { "tmpdir",             't', GETOPT_OPTION_TYPE_REQUIRED,   0, OPTION_TMPDIR,       "directory for temporary files (use output dir if not specified)", "[dir]"},
    { "ifdef",              0,   GETOPT_OPTION_TYPE_NO_ARG,     0, OPTION_IFDEF,        "wrap backend-specific generated code in #ifdef/#endif"},
    { "noifdef",            'n', GETOPT_OPTION_TYPE_NO_ARG,     0, OPTION_NOIFDEF,      "obsolete, superseded by --ifdef"},
    { "save-intermediate-spirv", 0, GETOPT_OPTION_TYPE_NO_ARG,  0, OPTION_SAVE_INTERMEDIATE_SPIRV, "save intermediate SPIRV bytecode (for debug inspection)"},
    GETOPT_OPTIONS_END
};

static void print_help_string(getopt_context_t& ctx) {
    fmt::print(stderr,
        "Shader compiler / code generator for sokol_gfx.h based on GLslang + SPIRV-Cross\n"
        "https://github.com/floooh/sokol-tools\n\n"
        "Usage: sokol-shdc -i input [-o output] [options]\n\n"
        "Where [input] is exactly one .glsl file in Vulkan syntax (separate texture and sampler uniforms),\n"
        "and [output] is a C header with embedded shader source code and/or byte code and\n"
        "code-generated uniform-block and shader-description C structs ready for use with sokol_gfx.h\n\n"
        "Please refer to the documentation to learn about the 'annotated GLSL syntax':\n\n"
        "  https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md\n\n\n"
        "Target shader languages (used with -l --slang):\n"
        "  - glsl410        desktop OpenGL backend (SOKOL_GLCORE)\n"
        "  - glsl430        desktop OpenGL backend (SOKOL_GLCORE)\n"
        "  - glsl300es      OpenGLES3 and WebGL2 (SOKOL_GLES3)\n"
        "  - hlsl4          Direct3D11 with HLSL4 (SOKOL_D3D11)\n"
        "  - hlsl5          Direct3D11 with HLSL5 (SOKOL_D3D11)\n"
        "  - metal_macos    Metal on macOS (SOKOL_METAL)\n"
        "  - metal_ios      Metal on iOS devices (SOKOL_METAL)\n"
        "  - metal_sim      Metal on iOS simulator (SOKOL_METAL)\n"
        "  - wgsl           WebGPU (SOKOL_WGPU)\n\n"
        "Output formats (used with -f --format):\n"
        "  - sokol          C header which includes both decl and inlined impl\n"
        "  - sokol_decl     C header with SOKOL_SHDC_DECL wrapped decl and inlined impl\n"
        "  - sokol_impl     C header with STB-style SOKOL_SHDC_IMPL wrapped impl\n"
        "  - sokol_zig      Zig module file\n"
        "  - sokol_nim      Nim module file\n"
        "  - sokol_odin     Odin module file\n"
        "  - sokol_rust     Rust module file\n"
        "  - bare           raw output of SPIRV-Cross compiler, in text or binary format\n"
        "  - bare_yaml      like bare, but with reflection file in YAML format\n\n"
        "Options:\n\n");
    char buf[4096];
    fmt::print(stderr, "{}", getopt_create_help_string(&ctx, buf, sizeof(buf)));
}

/* parse string of format 'hlsl4|glsl100|...' args.slang bitmask */
static bool parse_slang(Args& args, const char* str) {
    args.slang = 0;
    std::vector<std::string> splits;
    pystring::split(str, splits, ":");
    for (const auto& item : splits) {
        bool item_valid = false;
        for (int i = 0; i < Slang::NUM; i++) {
            if (item == Slang::to_str((Slang::Enum)i)) {
                args.slang |= Slang::bit((Slang::Enum)i);
                item_valid = true;
                break;
            }
        }
        if (!item_valid) {
            fmt::print(stderr, "sokol-shdc: unknown shader language '{}' (valid: {})\n", item, Slang::bits_to_str(0xFFFF, " "));
            args.valid = false;
            args.exit_code = 10;
            return false;
        }
    }
    if ((args.slang & Slang::bit(Slang::HLSL4)) && (args.slang & Slang::bit(Slang::HLSL5))) {
        fmt::print(stderr, "sokol-shdc: hlsl4 and hlsl5 output cannot be active at the same time!\n");
        args.valid = false;
        args.exit_code = 10;
        return false;
    }
    if ((args.slang & Slang::bit(Slang::GLSL410)) && (args.slang & Slang::bit(Slang::GLSL430))) {
        fmt::print(stderr, "sokol-shdc: glsl410 and glsl430 output cannot be active at the same time!\n");
        args.valid = false;
        args.exit_code = 10;
        return false;
    }
    return true;
}

static void validate(Args& args) {
    bool err = false;
    if (args.input.empty()) {
        fmt::print(stderr, "sokol-shdc: no input file (--input [path])\n");
        err = true;
    }
    if (args.output.empty()) {
        fmt::print(stderr, "sokol-shdc: no output file (--output [path])\n");
        err = true;
    }
    if (args.slang == 0) {
        fmt::print(stderr, "sokol-shdc: no shader languages (--slang ...)\n");
        err = true;
    }
    if (args.tmpdir.empty()) {
        std::string tail;
        pystring::os::path::split(args.tmpdir, tail, args.output);
        if (!args.tmpdir.empty()) {
            args.tmpdir += "/";
        }
    }
    else {
        if (!pystring::endswith(args.tmpdir, "/")) {
            args.tmpdir += "/";
        }
    }
    if (err) {
        args.valid = false;
        args.exit_code = 10;
    }
    else {
        args.valid = true;
        args.exit_code = 0;
    }
}

Args Args::parse(int argc, const char** argv) {
    Args args;

    // store the original command line args
    args.cmdline = "sokol-shdc";
    for (int i = 1; i < argc; i++) {
        args.cmdline.append(" ");
        args.cmdline.append(argv[i]);
    }

    getopt_context_t ctx;
    if (getopt_create_context(&ctx, argc, argv, option_list) < 0) {
        fmt::print(stderr, "error in getopt_create_context()\n");
    }
    else {
        int opt = 0;
        while ((opt = getopt_next(&ctx)) != -1) {
            switch (opt) {
                case '+':
                    fmt::print(stderr, "sokol-shdc: got argument without flag: {}\n", ctx.current_opt_arg);
                    args.valid = false;
                    return args;
                case '?':
                    fmt::print(stderr, "sokol-shdc: unknown flag {}\n", ctx.current_opt_arg);
                    args.valid = false;
                    return args;
                case '!':
                    fmt::print(stderr, "sokol-shdc: invalid use of flag {}\n", ctx.current_opt_arg);
                    args.valid = false;
                    return args;
                case OPTION_INPUT:
                    args.input = ctx.current_opt_arg;
                    break;
                case OPTION_OUTPUT:
                    args.output = ctx.current_opt_arg;
                    break;
                case OPTION_TMPDIR:
                    args.tmpdir = ctx.current_opt_arg;
                    break;
                case OPTION_BYTECODE:
                    args.byte_code = true;
                    break;
                case OPTION_DEFINES:
                    pystring::split(ctx.current_opt_arg, args.defines, ":");
                    break;
                case OPTION_MODULE:
                    args.module = ctx.current_opt_arg;
                    break;
                case OPTION_REFLECTION:
                    args.reflection = true;
                    break;
                case OPTION_FORMAT:
                    args.output_format = Format::from_str(ctx.current_opt_arg);
                    if (args.output_format == Format::INVALID) {
                        fmt::print(stderr, "sokol-shdc: unknown output format {}, must be [sokol|sokol_decl|sokol_impl|sokol_zig|sokol_nim|sokol_odin|bare]\n", ctx.current_opt_arg);
                        args.valid = false;
                        args.exit_code = 10;
                        return args;
                    }
                    break;
                case OPTION_DUMP:
                    args.debug_dump = true;
                    break;
                case OPTION_SAVE_INTERMEDIATE_SPIRV:
                    args.save_intermediate_spirv = true;
                    break;
                case OPTION_SLANG:
                    if (!parse_slang(args, ctx.current_opt_arg)) {
                        /* error details have been filled by parse_slang() */
                        return args;
                    }
                    break;
                case OPTION_ERRFMT:
                    if (0 == strcmp("gcc", ctx.current_opt_arg)) {
                        args.error_format = ErrMsg::GCC;
                    }
                    else if (0 == strcmp("msvc", ctx.current_opt_arg)) {
                        args.error_format = ErrMsg::MSVC;
                    }
                    else {
                        fmt::print(stderr, "sokol-shdc: unknown error format {}, must be 'gcc' or 'msvc'\n", ctx.current_opt_arg);
                        args.valid = false;
                        args.exit_code = 10;
                        return args;
                    }
                    break;
                case OPTION_GENVER:
                    args.gen_version = atoi(ctx.current_opt_arg);
                    break;
                case OPTION_IFDEF:
                    args.ifdef = true;
                    break;
                case OPTION_NOIFDEF:
                    // obsolete, but keep for backwards compatibility
                    args.ifdef = false;
                    break;
                case OPTION_HELP:
                    print_help_string(ctx);
                    args.valid = false;
                    args.exit_code = 0;
                    return args;
                default:
                    break;
            }
        }
    }
    validate(args);
    return args;
}

void Args::dump_debug() const {
    fmt::print(stderr, "Args:\n");
    fmt::print(stderr, "  valid: {}\n", valid);
    fmt::print(stderr, "  exit_code: {}\n", exit_code);
    fmt::print(stderr, "  input: '{}'\n", input);
    fmt::print(stderr, "  output: '{}'\n", output);
    fmt::print(stderr, "  tmpdir: '{}'\n", tmpdir);
    fmt::print(stderr, "  slang: '{}'\n", Slang::bits_to_str(slang, ":"));
    fmt::print(stderr, "  byte_code: {}\n", byte_code);
    fmt::print(stderr, "  module: '{}'\n", module);
    fmt::print(stderr, "  defines: '{}'\n", pystring::join(":", defines));
    fmt::print(stderr, "  output_format: '{}'\n", Format::to_str(output_format));
    fmt::print(stderr, "  debug_dump: {}\n", debug_dump);
    fmt::print(stderr, "  ifdef: {}\n", ifdef);
    fmt::print(stderr, "  gen_version: {}\n", gen_version);
    fmt::print(stderr, "  error_format: {}\n", ErrMsg::msg_format_to_str(error_format));
    fmt::print(stderr, "\n");
}

} // namespace shdc
