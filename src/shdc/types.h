/*
    Shared type definitions for sokol-shdc
*/
#include <string>
#include <stdint.h>

namespace shdc {

/* the output shader languages to create, can be combined */
enum slang_t {
    SLANG_GLSL330     = (1<<0),
    SLANG_GLSL100     = (1<<1),
    SLANG_GLSL300ES   = (1<<2),
    SLANG_HLSL5       = (1<<3),
    SLANG_METAL_MACOS = (1<<5),
    SLANG_METAL_IOS   = (1<<6),

    SLANG_NUM = 6
};

struct args_t {
    bool valid = false;
    int exit_code = 10;
    std::string input;      // input file path
    std::string output;     // output file path
    uint32_t slang = 0;     // combined slang_t bits
    bool byte_code = false; // output byte code (for HLSL and MetalSL)

    static args_t parse(int argc, const char** argv);
    void dump();
};

    
} // namespace shdc
