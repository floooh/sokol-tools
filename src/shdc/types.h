/*
    Shared type definitions for sokol-shdc
*/
#include <string>
#include <string.h> /* strcmp */
#include <stdint.h>
#include <vector>
#include <map>

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
    static type_t to_code(const char* str) {
        if      (0 == strcmp(str, "glsl330")) return GLSL330;
        else if (0 == strcmp(str, "glsl100")) return GLSL100;
        else if (0 == strcmp(str, "glsl300es")) return GLSL300ES;
        else if (0 == strcmp(str, "hlsl5")) return HLSL5;
        else if (0 == strcmp(str, "metal_macos")) return METAL_MACOS;
        else if (0 == strcmp(str, "metal_ios")) return METAL_IOS;
        else return INVALID;
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

    error_t() { };
    error_t(const std::string& f, int l, const std::string& m): file(f), msg(m), line_index(l), valid(true) { };
    error_t(const std::string& m): msg(m), valid(true) { };
};

/* result of command-line-args parsing */
struct args_t {
    bool valid = false;
    int exit_code = 10;
    std::string input;          // input file path
    std::string output;         // output file path
    uint32_t slang = 0;         // combined slang_t bits
    bool byte_code = false;     // output byte code (for HLSL and MetalSL)
    bool debug_dump = true;    // print debug-dump info

    static args_t parse(int argc, const char** argv);
    void dump();
};

/* a code-snippet-range in the source file (used for @vs, @fs, @block code blocks) */
struct snippet_t {
    enum type_t {
        INVALID,
        BLOCK,
        VS,
        FS
    };
    type_t type = INVALID;
    std::string name;
    int line_index = -1;    // first line of the snippet in input source (zero-based)
    int num_lines = 0;      // number of lines of snippet

    void start(snippet_t::type_t type, const std::string& name, int line_index) {
        this->type = type;
        this->name = name;
        this->line_index = line_index;
    }
    void end(int end_line_index) {
        this->num_lines = end_line_index - this->line_index;
        assert(this->num_lines > 0);
    }
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

/* pre-parsed GLSL source file */
struct input_t {
    error_t error;                      // error object for parsing errors
    std::string path;                   // filesystem
    std::vector<std::string> lines;     // input source file split into lines
    std::vector<snippet_t> snippets;    // snippet-ranges
    std::map<std::string, int> blocks;  // name-index mapping of code block snippets
    std::map<std::string, int> vs;      // name-index mapping of vertex shader snippets
    std::map<std::string, int> fs;      // name-index mapping of fragment shader snippets
    std::map<std::string, program_t> programs;    // parsed shader program definitions

    input_t() { };
    static input_t load(const std::string& path);    // load file and parse into input_t
    void dump();                        // debug-dump content
};

} // namespace shdc
