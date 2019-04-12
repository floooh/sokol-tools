#include <stdio.h>
#include "types.h"
#include "fmt/format.h"

using namespace shdc;

int main(int argc, const char** argv) {
    /* parse command line arguments */
    args_t args = args_t::parse(argc, argv);
    if (args.debug_dump) {
        args.dump();
    }
    if (!args.valid) {
        return args.exit_code;
    }

    /* load and parse input source */
    input_t inp = input_t::load(args.input);
    if (args.debug_dump) {
        inp.dump();
    }
    if (inp.error.valid) {
        fmt::print(stderr, "error: {}\n", inp.error.msg);
        return 10;
    }

    /* success */
    return 0;
}

