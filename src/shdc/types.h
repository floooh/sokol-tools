/*
    Shared type definitions for sokol-shdc
*/
#include <string>

namespace shdc {

struct args_t {
    bool valid = false;
    int exit_code = 10;

    static args_t parse(int argc, const char** argv);
};

    
} // namespace shdc
