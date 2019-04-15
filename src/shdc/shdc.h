/*
    Shared type definitions for sokol-shdc
*/
#include <string>
#include <stdint.h>
#include <vector>
#include <map>
#include "fmt/format.h"

namespace shdc {

/* the output shader languages to create */
struct slang_t {
    enum type_t {
        INVALID,
        GLSL330,
        GLSL100,
        GLSL300ES,
        HLSL5,
        METAL_MACOS,
        METAL_IOS,
        NUM
    };

    static uint32_t bit(type_t c) {
        return (1<<c);
    }
    static const char* to_str(type_t c) {
        switch (c) {
            case GLSL330:       return "glsl330";
            case GLSL100:       return "glsl100";
            case GLSL300ES:     return "glsl300es";
            case HLSL5:         return "hlsl5";
            case METAL_MACOS:   return "metal_macos";
            case METAL_IOS:     return "metal_ios";
            default:            return "<invalid>";
        }
    }
    static std::string bits_to_str(uint32_t mask) {
        std::string res;
        bool sep = false;
        for (int i = 0; i < slang_t::NUM; i++) {
            if (mask & slang_t::bit((type_t)i)) {
                if (sep) {
                    res += ":";
                }
                res += to_str((type_t)i);
                sep = true;
            }
        }
        return res;
    }
};

/* an error object with filename, line number and message */
struct error_t {
    std::string file;
    std::string msg;
    int line_index = -1;      // line_index is zero-based!
    bool valid = false;
    // format for error message
    enum msg_format_t {
        GCC,
        VSTUDIO
    };

    error_t() { };
    error_t(const std::string& f, int l, const std::string& m): file(f), msg(m), line_index(l), valid(true) { };
    error_t(const std::string& m): msg(m), valid(true) { };
    std::string as_string(msg_format_t fmt) const {
        if (fmt == VSTUDIO) {
            return fmt::format("{}({}): error: {}", file, line_index+1, msg);
        }
        else {
            return fmt::format("{}:{}:0: error: {}", file, line_index+1, msg);
        }
    }
    // print error to stdout
    void print(msg_format_t fmt) const {
        fmt::print("{}\n", as_string(fmt));
    }
    // convert msg_format to string
    static const char* msg_format_to_str(msg_format_t fmt) {
        switch (fmt) {
            case GCC: return "gcc";
            case VSTUDIO: return "vstudio";
            default: return "<invalid>";
        }
    }
};

/* result of command-line-args parsing */
struct args_t {
    bool valid = false;
    int exit_code = 10;
    std::string input;                  // input file path
    std::string output;                 // output file path
    uint32_t slang = 0;                 // combined slang_t bits
    bool byte_code = false;             // output byte code (for HLSL and MetalSL)
    bool debug_dump = false;            // print debug-dump info
    error_t::msg_format_t error_format = error_t::GCC;  // format for error messages

    static args_t parse(int argc, const char** argv);
    void dump_debug() const;
};

/* a named code-snippet (@block, @vs or @fs) in the input source file */
struct snippet_t {
    enum type_t {
        INVALID,
        BLOCK,
        VS,
        FS
    };
    type_t type = INVALID;
    std::string name;
    std::vector<int> lines; // resolved zero-based line-indices (including @include_block)

    snippet_t() { };
    snippet_t(type_t t, const std::string& n): type(t), name(n) { };

    static const char* type_to_str(type_t t) {
        switch (t) {
            case BLOCK: return "block";
            case VS: return "vs";
            case FS: return "fs";
            default: return "<invalid>";
        }
    }
};

/* a vertex-/fragment-shader pair (@program) */
struct program_t {
    std::string name;
    std::string vs_name;    // name of vertex shader snippet
    std::string fs_name;    // name of fragment shader snippet
    int line_index = -1;    // line index in input source (zero-based)

    program_t() { };
    program_t(const std::string& n, const std::string& vs, const std::string& fs, int l): name(n), vs_name(vs), fs_name(fs), line_index(l) { };
};

/* pre-parsed GLSL source file, with content split into snippets */
struct input_t {
    error_t error;
    std::string path;                   // filesystem
    std::vector<std::string> lines;     // input source file split into lines
    std::vector<snippet_t> snippets;    // @block, @vs and @fs snippets
    std::map<std::string, int> snippet_map; // name-index mapping for all code snippets
    std::map<std::string, int> block_map;   // name-index mapping for @block snippets
    std::map<std::string, int> vs_map;      // name-index mapping for @vs snippets
    std::map<std::string, int> fs_map;      // name-index mapping for @fs snippets
    std::map<std::string, program_t> programs;    // all @program definitions

    input_t() { };
    static input_t load_and_parse(const std::string& path);
    void dump_debug() const;
};

/* a SPIRV-bytecode blob with "back-link" to input_t.snippets */
struct spirv_blob_t {
    int snippet_index = -1;         // index into input_t.snippets
    std::vector<uint32_t> bytecode; // the SPIRV blob

    spirv_blob_t(int snippet_index): snippet_index(snippet_index) { };
};

/* glsl-to-spirv compiler wrapper */
struct spirv_t {
    std::vector<error_t> errors;
    std::vector<spirv_blob_t> blobs;

    static void initialize_spirv_tools();
    static void finalize_spirv_tools();
    static spirv_t compile_glsl(const input_t& inp);
    void dump_debug(const input_t& inp, error_t::msg_format_t err_fmt) const;
};

/* spirv-cross wrapper */
struct spirvcross_t {
    error_t error;

    static spirvcross_t translate(const input_t& inp, const spirv_t& spirv, uint32_t slang_mask);
    void dump_debug() const;
};

/* HLSL/Metal to bytecode compiler wrapper */
struct bytecode_t {
    error_t error;

    static bytecode_t compile(const input_t& inp, const spirvcross_t& spirvcross, bool gen_bytecode);
    void dump_debug() const;
};

/* C header-generator wrapper */
struct header_t {
    error_t error;

    static header_t build(const input_t& inp, const spirvcross_t& spirvcross, const bytecode_t& bytecode, const std::string path);
    void dump_debug() const;
};

} // namespace shdc
