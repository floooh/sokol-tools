#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "types/errmsg.h"
#include "types/format.h"

namespace shdc {

// result of command-line-args parsing
struct Args {
    bool valid = false;
    std::string cmdline;
    int exit_code = 10;
    std::string input;                  // input file path
    std::string output;                 // output file path
    std::string tmpdir;                 // directory for temporary files
    std::string module;                 // optional @module name override
    std::vector<std::string> defines;   // additional preprocessor defines
    uint32_t slang = 0;                 // combined Slang bits
    bool byte_code = false;             // output byte code (for HLSL and MetalSL)
    bool reflection = false;            // if true, generate runtime reflection functions
    Format::Enum output_format = Format::SOKOL; // output format
    bool debug_dump = false;            // print debug-dump info
    bool ifdef = false;                 // wrap backend specific shaders into #ifdefs (SOKOL_D3D11 etc...)
    bool save_intermediate_spirv = false;   // save intermediate SPIRV bytecode (glslangvalidator output)
    bool no_log_cmdline = false;        // if true, no cmdline will be logged to the generated output code
    int gen_version = 1;                // generator-version stamp
    ErrMsg::Format error_format = ErrMsg::GCC;  // format for error messages

    static Args parse(int argc, const char** argv);
    void dump_debug() const;
};

} // namespace shdc
