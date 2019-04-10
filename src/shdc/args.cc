/*
    parse command line arguments
*/
#include <stdio.h>
#include "types.h"
#include "getopt/getopt.h"

namespace shdc {

static const getopt_option_t option_list[] = {
    { "help", 'h', GETOPT_OPTION_TYPE_NO_ARG, 0, 'h', "print this help text", 0},
    GETOPT_OPTIONS_END
};

static void print_help_string(getopt_context_t& ctx) {
    char buf[2048];
    fprintf(stderr, "%s\n", getopt_create_help_string(&ctx, buf, sizeof(buf)));
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

} // namespace shdc
