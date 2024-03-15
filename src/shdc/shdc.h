/*
    Shared type definitions for sokol-shdc
*/
#include <string>
#include <stdint.h>
#include <vector>
#include <array>
#include <map>
#include "fmt/format.h"
#include "spirv_cross.hpp"
#include "reflection.h"
#include "types/format.h"
#include "types/errmsg.h"
#include "types/line.h"
#include "types/snippet.h"
#include "types/program.h"
#include "types/option.h"

namespace shdc {

// result of command-line-args parsing
struct args_t {
    bool valid = false;
    std::string cmdline;
    int exit_code = 10;
    std::string input;                  // input file path
    std::string output;                 // output file path
    std::string tmpdir;                 // directory for temporary files
    std::string module;                 // optional @module name override
    std::vector<std::string> defines;   // additional preprocessor defines
    uint32_t slang = 0;                 // combined slang_t bits
    bool byte_code = false;             // output byte code (for HLSL and MetalSL)
    bool reflection = false;            // if true, generate runtime reflection functions
    format_t::type_t output_format = format_t::SOKOL; // output format
    bool debug_dump = false;            // print debug-dump info
    bool ifdef = false;                 // wrap backend specific shaders into #ifdefs (SOKOL_D3D11 etc...)
    bool save_intermediate_spirv = false;   // save intermediate SPIRV bytecode (glslangvalidator output)
    int gen_version = 1;                // generator-version stamp
    errmsg_t::msg_format_t error_format = errmsg_t::GCC;  // format for error messages

    static args_t parse(int argc, const char** argv);
    void dump_debug() const;
};

// pre-parsed GLSL source file, with content split into snippets
struct input_t {
    errmsg_t out_error;
    std::string base_path;              // path to base file
    std::string module;                 // optional module name
    std::vector<std::string> filenames; // all source files, base is first entry
    std::vector<line_t> lines;          // input source files split into lines
    std::vector<snippet_t> snippets;    // @block, @vs and @fs snippets
    std::map<std::string, std::string> ctype_map;    // @ctype uniform type definitions
    std::vector<std::string> headers;       // @header statements
    std::map<std::string, int> snippet_map; // name-index mapping for all code snippets
    std::map<std::string, int> block_map;   // name-index mapping for @block snippets
    std::map<std::string, int> vs_map;      // name-index mapping for @vs snippets
    std::map<std::string, int> fs_map;      // name-index mapping for @fs snippets
    std::map<std::string, program_t> programs;    // all @program definitions

    input_t() { };
    static input_t load_and_parse(const std::string& path, const std::string& module_override);
    void dump_debug(errmsg_t::msg_format_t err_fmt) const;

    errmsg_t error(int index, const std::string& msg) const {
        if (index < (int)lines.size()) {
            const line_t& line = lines[index];
            return errmsg_t::error(filenames[line.filename], line.index, msg);
        } else {
            return errmsg_t::error(base_path, 0, msg);
        }
    };
    errmsg_t warning(int index, const std::string& msg) const {
        if (index < (int)lines.size()) {
            const line_t& line = lines[index];
            return errmsg_t::warning(filenames[line.filename], line.index, msg);
        } else {
            return errmsg_t::warning(base_path, 0, msg);
        }
    };
};

// a SPIRV-bytecode blob with "back-link" to input_t.snippets
struct spirv_blob_t {
    int snippet_index = -1;         // index into input_t.snippets
    std::string source;             // source code this blob was compiled from
    std::vector<uint32_t> bytecode; // the resulting SPIRV blob

    spirv_blob_t(int snippet_index): snippet_index(snippet_index) { };
};

// glsl-to-spirv compiler wrapper
struct spirvcross_t;
struct spirv_t {
    std::vector<errmsg_t> errors;
    std::vector<spirv_blob_t> blobs;

    static void initialize_spirv_tools();
    static void finalize_spirv_tools();
    static spirv_t compile_glsl(const input_t& inp, slang_t::type_t slang, const std::vector<std::string>& defines);
    bool write_to_file(const args_t& args, const input_t& inp, slang_t::type_t slang);
    void dump_debug(const input_t& inp, errmsg_t::msg_format_t err_fmt) const;
};

// result of a spirv-cross compilation
struct spirvcross_source_t {
    bool valid = false;
    int snippet_index = -1;
    std::string source_code;
    errmsg_t error;
    reflection_t refl;
};

// spirv-cross wrapper
struct spirvcross_t {
    errmsg_t error;
    std::vector<spirvcross_source_t> sources;
    std::vector<uniform_block_t> unique_uniform_blocks;
    std::vector<image_t> unique_images;
    std::vector<sampler_t> unique_samplers;

    static spirvcross_t translate(const input_t& inp, const spirv_t& spirv, slang_t::type_t slang);
    static bool can_flatten_uniform_block(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& ub_res);
    int find_source_by_snippet_index(int snippet_index) const;
    void dump_debug(errmsg_t::msg_format_t err_fmt, slang_t::type_t slang) const;
};

// HLSL/Metal to bytecode compiler wrapper
struct bytecode_blob_t {
    bool valid = false;
    int snippet_index = -1;
    std::vector<uint8_t> data;
};

struct bytecode_t {
    std::vector<errmsg_t> errors;
    std::vector<bytecode_blob_t> blobs;

    static bytecode_t compile(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang);
    int find_blob_by_snippet_index(int snippet_index) const;
    void dump_debug() const;
};

// C header-generator for sokol_gfx.h
struct sokol_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// Zig module-generator for sokol-zig
struct sokolzig_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// Nim module-generator for sokol-nim
struct sokolnim_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// Odin module-generator for sokol-odin
struct sokolodin_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// Rust module-generator for sokol-rust
struct sokolrust_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// bare format generator
struct bare_t {
    static const char* slang_file_extension(slang_t::type_t c, bool binary);
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// yaml reflection format generator
struct yaml_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// utility functions for generators
namespace util {
    errmsg_t check_errors(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang);
    const char* uniform_type_str(uniform_t::type_t type);
    int uniform_size(uniform_t::type_t type, int array_size);
    int roundup(int val, int round_to);
    std::string mod_prefix(const input_t& inp);
    const spirvcross_source_t* find_spirvcross_source_by_shader_name(const std::string& shader_name, const input_t& inp, const spirvcross_t& spirvcross);
    const bytecode_blob_t* find_bytecode_blob_by_shader_name(const std::string& shader_name, const input_t& inp, const bytecode_t& bytecode);
    std::string to_camel_case(const std::string& str);
    std::string to_pascal_case(const std::string& str);
    std::string to_ada_case(const std::string& str);
    std::string to_upper_case(const std::string& str);
    std::string replace_C_comment_tokens(const std::string& str);
};

} // namespace shdc
