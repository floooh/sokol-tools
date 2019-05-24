/*
    parse command line arguments
*/
#include <vector>
#include <stdio.h>
#include "fmt/format.h"
#include "shdc.h"
#include "getopt/getopt.h"
#include "pystring.h"

namespace shdc {

static const getopt_option_t option_list[] = {
    { "help", 'h',  GETOPT_OPTION_TYPE_NO_ARG, 0, 'h', "print this help text", 0},
    { "input", 'i', GETOPT_OPTION_TYPE_REQUIRED, 0, 'i', "input source file", "GLSL file" },
    { "output", 'o', GETOPT_OPTION_TYPE_REQUIRED, 0, 'o', "output source file", "C header" },
    { "slang", 'l', GETOPT_OPTION_TYPE_REQUIRED, 0, 'l', "output shader language(s)", "glsl330:glsl100:glsl300es:hlsl5:metal_macos:metal_ios" },
    { "bytecode", 'b', GETOPT_OPTION_TYPE_NO_ARG, 0, 'b', "output bytecode (HLSL and Metal)"},
    { "errfmt", 'e', GETOPT_OPTION_TYPE_REQUIRED, 0, 'e', "error message format (default: gcc)", "[gcc|msvc]"},
    { "dump", 'd', GETOPT_OPTION_TYPE_NO_ARG, 0, 'd', "dump debugging information to stderr"},
    { "genver", 'g', GETOPT_OPTION_TYPE_REQUIRED, 0, 'g', "version-stamp for code-generation", "[int]"},
    { "noifdef", 'n', GETOPT_OPTION_TYPE_NO_ARG, 0, 'n', "don't emit #ifdef SOKOL_XXX"},
    { "tmpdir", 't', GETOPT_OPTION_TYPE_REQUIRED, 0, 't', "directory for temporary files (use output dir if not specified)", "[dir]"},
    GETOPT_OPTIONS_END
};

static void print_help_string(getopt_context_t& ctx) {
    fmt::print(stderr, 
        "Shader compiler / code generator for sokol_gfx.h based on GLslang + SPIRV-Cross\n"
        "https://github.com/floooh/sokol-tools\n\n"
        "Usage: sokol-shdc -i input [-o output] [options]\n\n"
        "Where [input] is exactly one .glsl file, and [output] is a C header\n"
        "with embedded shader source code and/or byte code and code-generated\n"
        "uniform-block and shader-descripton C structs ready for use with sokol_gfx.h\n\n"
        "The input source file contains custom '@-tags' to group the\n"
        "source code for several shaders and shared code blocks into one file:\n\n"
        "  - @module name: optional shader module name, will be used as prefix in generated code\n"
        "  - @ctype type ctype: a type-alias for generated uniform block structs\n"
        "    (where type is float, vec2, vec3, vec4 or mat4)\n"
        "  - @block name: a general reusable code block\n"
        "  - @vs name: a named vertex shader code block\n"
        "  - @fs name: a named fragment shader code block\n"
        "  - @glsl_options options...: GLSL specific compile options\n"
        "  - @hlsl_options options...: HLSL specific compile options\n"
        "  - @msl_options options...: MSL (Metal) specific compile options\n"
        "    valid options are: flip_vert_y and fixup_clipspace\n"
        "  - @end: ends a @vs, @fs or @block code block\n"
        "  - @include_block block_name: include a code block in a @vs or @fs block\n"
        "  - @program name vs_name fs_name: a named, linked shader program\n\n"
        "An input file must contain at least one @vs block, one @fs block\n"
        "and one @program declaration.\n\n"
        "Options:\n\n");
    char buf[2048];
    fmt::print(stderr, getopt_create_help_string(&ctx, buf, sizeof(buf)));
}

/* parse string of format 'hlsl|glsl100|...' args.slang bitmask */
static bool parse_slang(args_t& args, const char* str) {
    args.slang = 0;
    std::vector<std::string> splits;
    pystring::split(str, splits, ":");
    for (const auto& item : splits) {
        bool item_valid = false;
        for (int i = 0; i < slang_t::NUM; i++) {
            if (item == slang_t::to_str((slang_t::type_t)i)) {
                args.slang |= slang_t::bit((slang_t::type_t)i);
                item_valid = true;
                break;
            }
        }
        if (!item_valid) {
            fmt::print(stderr, "sokol-shdc: unknown shader language '{}'\n", item);
            args.valid = false;
            args.exit_code = 10;
            return false;
        }
    }
    return true;
}

static void validate(args_t& args) {
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

args_t args_t::parse(int argc, const char** argv) {
    args_t args;
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
                case 'i':
                    args.input = ctx.current_opt_arg;
                    break;
                case 'o':
                    args.output = ctx.current_opt_arg;
                    break;
                case 't':
                    args.tmpdir = ctx.current_opt_arg;
                    break;
                case 'b':
                    args.byte_code = true;
                    break;
                case 'd':
                    args.debug_dump = true;
                    break;
                case 'l':
                    if (!parse_slang(args, ctx.current_opt_arg)) {
                        /* error details have been filled by parse_slang() */
                        return args;
                    }
                    break;
                case 'e':
                    if (0 == strcmp("gcc", ctx.current_opt_arg)) {
                        args.error_format = errmsg_t::GCC;
                    }
                    else if (0 == strcmp("msvc", ctx.current_opt_arg)) {
                        args.error_format = errmsg_t::MSVC;
                    }
                    else {
                        fmt::print(stderr, "sokol-shdc: unknown error format {}, must be 'gcc' or 'msvc'\n", ctx.current_opt_arg);
                        args.valid = false;
                        args.exit_code = 10;
                        return args;
                    }
                    break;
                case 'g':
                    args.gen_version = atoi(ctx.current_opt_arg);
                    break;
                case 'n':
                    args.no_ifdef = true;
                    break;
                case 'h':
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

void args_t::dump_debug() const {
    fmt::print(stderr, "args_t:\n");
    fmt::print(stderr, "  valid: {}\n", valid);
    fmt::print(stderr, "  exit_code: {}\n", exit_code);
    fmt::print(stderr, "  input:  '{}'\n", input);
    fmt::print(stderr, "  output: '{}'\n", output);
    fmt::print(stderr, "  tmpdir: '{}'\n", tmpdir);
    fmt::print(stderr, "  slang:  '{}'\n", slang_t::bits_to_str(slang));
    fmt::print(stderr, "  byte_code: {}\n", byte_code);
    fmt::print(stderr, "  debug_dump: {}\n", debug_dump);
    fmt::print(stderr, "  no_ifdef: {}\n", no_ifdef);
    fmt::print(stderr, "  gen_version: {}\n", gen_version);
    fmt::print(stderr, "  error_format: {}\n", errmsg_t::msg_format_to_str(error_format));
    fmt::print(stderr, "\n");
}

} // namespace shdc
