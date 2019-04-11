/*
    parse command line arguments
*/
#include <vector>
#include <stdio.h>
#include "types.h"
#include "getopt/getopt.h"
#include "pystring.h"

namespace shdc {

static const getopt_option_t option_list[] = {
    { "help", 'h',  GETOPT_OPTION_TYPE_NO_ARG, 0, 'h', "print this help text", 0},
    { "input", 'i', GETOPT_OPTION_TYPE_REQUIRED, 0, 'i', "input source file", "GLSL file" },
    { "output", 'o', GETOPT_OPTION_TYPE_REQUIRED, 0, 'o', "output source file", "C header" },
    { "slang", 'l', GETOPT_OPTION_TYPE_REQUIRED, 0, 'l', "output shader language(s)", "glsl330:glsl100:glsl300es:hlsl5:metal_macos:metal_ios" },
    { "bytecode", 'b', GETOPT_OPTION_TYPE_NO_ARG, 0, 'b', "output bytecode (HLSL and Metal)"},
    GETOPT_OPTIONS_END
};

static void print_help_string(getopt_context_t& ctx) {
    fprintf(stderr, 
        "Shader compiler / code generator for sokol_gfx.h based on GLslang + SPIRV-Cross\n"
        "https://github.com/floooh/sokol-tools\n\n"
        "Usage: sokol-shdc -i input [-o output] [options]\n\n"
        "Where [input] is exactly one .glsl file, and [output] is a C header\n"
        "with embedded shader source code and/or byte code, and code-generated\n"
        "uniform-block and shader-descripton C structs ready for use with sokol_gfx.h\n\n"
        "The input source file may contains custom '@-tags' to group the\n"
        "source code for several shaders and reusable code blocks into one file:\n\n"
        "  - @block name: a general reusable code block\n"
        "  - @vs name: a named vertex shader code block\n"
        "  - @fs name: a named fragment shader code block\n"
        "  - @end: ends a @vs, @fs or @block code block\n"
        "  - @include block_name: include a code block in a @vs or @fs block\n"
        "  - @program name vs_name fs_name: a named, linked shader program\n\n"
        "An input file must contain at least one @vs block, one @fs block\n"
        "and one @program declaration.\n\n"
        "Options:\n\n");
    char buf[2048];
    fprintf(stderr, "%s\n", getopt_create_help_string(&ctx, buf, sizeof(buf)));
}

static const char* slang_names[SLANG_NUM] = { "glsl330", "glsl100", "glsl300es", "hlsl5", "metal_macos", "metal_ios" };

/* parse string of format 'hlsl|glsl100|...' args.slang bitmask */
static bool parse_slang(args_t& args, const char* str) {
    args.slang = 0;
    std::vector<std::string> splits;
    pystring::split(str, splits, ":");
    for (const auto& item : splits) {
        bool item_valid = false;
        for (int i = 0; i < SLANG_NUM; i++) {
            if (item == slang_names[i]) {
                args.slang |= (1<<i);
                item_valid = true;
                break;
            }
        }
        if (!item_valid) {
            fprintf(stderr, "error: unknown shader language '%s'\n", item.c_str());
            args.valid = false;
            args.exit_code = 10;
            return false;
        }
    }
    return true;
}

static std::string slang_to_str(uint32_t slang) {
    std::string res;
    bool sep = false;
    for (int i = 0; i < SLANG_NUM; i++) {
        if (slang & (1<<i)) {
            if (sep) {
                res += ":";
            }
            res += slang_names[i];
            sep = true;
        }
    }
    return res;
}

args_t args_t::parse(int argc, const char** argv) {
    args_t args;
    getopt_context_t ctx;
    if (getopt_create_context(&ctx, argc, argv, option_list) < 0) {
        fprintf(stderr, "error in getopt_create_context()\n");
    }
    else {
        int opt = 0;
        while ((opt = getopt_next(&ctx)) != -1) {
            switch (opt) {
                case '+':
                    fprintf(stderr, "got argument without flag: %s\n", ctx.current_opt_arg);
                    break;
                case '?':
                    fprintf(stderr, "unknown flag %s\n", ctx.current_opt_arg);
                    break;
                case '!':
                    fprintf(stderr, "invalid use of flag %s\n", ctx.current_opt_arg);
                    break;
                case 'i':
                    args.input = ctx.current_opt_arg;
                    break;
                case 'o':
                    args.output = ctx.current_opt_arg;
                    break;
                case 'b':
                    args.output = true;
                    break;
                case 'l':
                    if (!parse_slang(args, ctx.current_opt_arg)) {
                        /* error details have been filled by parse_slang() */
                        return args;
                    }
                    break;
                case 'h':
                    print_help_string(ctx);
                    args.exit_code = 0;
                    break;
                default:
                    break;
            }
        }
    }
    return args;
}

void args_t::dump() {
    std::string slang_str = slang_to_str(slang);
    if (!slang_str[0]) {
        slang_str = "<none>";
    }
    std::string input_str = input[0] ? input : "<none>";
    std::string output_str = output[0] ? output : "<none>";
    fprintf(stderr,
        "\nargs:\n  input: %s\n  output: %s\n  slang: %s\n  byte_code: %s\n\n",
        input_str.c_str(), output_str.c_str(), slang_str.c_str(), byte_code ? "true":"false");
}

void args_t::validate() {
    bool err = false;
    if (input.empty()) {
        fprintf(stderr, "error: no input file (--input [path])\n");
        err = true;
    }
    if (output.empty()) {
        fprintf(stderr, "error: no output file (--output [path])\n");
        err = true;
    }
    if (slang == 0) {
        fprintf(stderr, "error: no shader languages (--slang ...)\n");
        err = true;
    }
    if (err) {
        valid = false;
        exit_code = 10;
    }
}

} // namespace shdc
