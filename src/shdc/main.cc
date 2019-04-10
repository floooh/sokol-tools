#include <stdio.h>
#include "types.h"

int main(int argc, const char** argv) {
    shdc::args_t args = shdc::args_t::parse(argc, argv);
    if (!args.valid) {
        return args.exit_code;
    }
    return 0;
}

