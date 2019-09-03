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

namespace shdc {

/* the output shader languages to create */
struct slang_t {
    enum type_t {
        GLSL330 = 0,
        GLSL100,
        GLSL300ES,
        HLSL5,
        METAL_MACOS,
        METAL_IOS,
        METAL_SIM,
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
            case METAL_SIM:     return "metal_sim";
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

/* the output format */
struct format_t	{
    enum type_t {
        SOKOL = 0,
        SOKOL_DECL,
        SOKOL_IMPL,
        BARE,
        NUM,
        INVALID,
    };

    static const char* to_str(type_t f) {
        switch (f) {
            case SOKOL:         return "sokol";
            case SOKOL_DECL:    return "sokol_decl";
            case SOKOL_IMPL:    return "sokol_impl";
            case BARE:          return "bare";
            default:            return "<invalid>";
        }
    }
    static type_t from_str(const std::string& str) {
        if (str == "sokol") {
            return SOKOL;
        }
        else if (str == "sokol_decl") {
            return SOKOL_DECL;
        }
        else if (str == "sokol_impl") {
            return SOKOL_IMPL;
        }
        else if (str == "bare") {
            return BARE;
        }
        else {
            return INVALID;
        }
    }
};

/* an error message object with filename, line number and message */
struct errmsg_t {
    enum type_t {
        ERROR,
        WARNING,
    };
    type_t type = ERROR;
    std::string file;
    std::string msg;
    int line_index = -1;      // line_index is zero-based!
    bool valid = false;
    // format for error message
    enum msg_format_t {
        GCC,
        MSVC
    };

    static errmsg_t error(const std::string& file, int line, const std::string& msg) {
        errmsg_t err;
        err.type = ERROR;
        err.file = file;
        err.msg = msg;
        err.line_index = line;
        err.valid = true;
        return err;
    }
    static errmsg_t warning(const std::string& file, int line, const std::string& msg) {
        errmsg_t err;
        err.type = WARNING;
        err.file = file;
        err.msg = msg;
        err.line_index = line;
        err.valid = true;
        return err;
    }

    std::string as_string(msg_format_t fmt) const {
        if (fmt == MSVC) {
            return fmt::format("{}({}): {}: {}", file, line_index+1, (type==ERROR)?"error":"warning", msg);
        }
        else {
            return fmt::format("{}:{}:0: {}: {}", file, line_index+1, (type==ERROR)?"error":"warning", msg);
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
            case MSVC: return "msvc";
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
    std::string tmpdir;                 // directory for temporary files
    uint32_t slang = 0;                 // combined slang_t bits
    bool byte_code = false;             // output byte code (for HLSL and MetalSL)
    format_t::type_t output_format = format_t::SOKOL; // output format
    bool debug_dump = false;            // print debug-dump info
    bool no_ifdef = false;              // don't emit platform #ifdefs (SOKOL_D3D11 etc...)
    int gen_version = 1;                // generator-version stamp
    errmsg_t::msg_format_t error_format = errmsg_t::GCC;  // format for error messages

    static args_t parse(int argc, const char** argv);
    void dump_debug() const;
};

/* per-shader compile options */
struct option_t {
    enum type_t {
        INVALID = 0,
        FIXUP_CLIPSPACE = (1<<0),
        FLIP_VERT_Y = (1<<1),

        NUM
    };

    static type_t from_string(const std::string& str) {
        if (str == "fixup_clipspace") {
            return FIXUP_CLIPSPACE;
        }
        else if (str == "flip_vert_y") {
            return FLIP_VERT_Y;
        }
        else {
            return INVALID;
        }
    }
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
    std::array<uint32_t, slang_t::NUM> options = { };
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

/* mapping each line to included filename and line index */
struct line_t {
    std::string line;       // line content
    int filename = 0;       // index into input_t filenames
    int index = 0;          // line index == line nr - 1

    line_t() { };
    line_t(const std::string& ln, int fn, int ix): line(ln), filename(fn), index(ix) { };
};

/* pre-parsed GLSL source file, with content split into snippets */
struct input_t {
    errmsg_t out_error;
    std::string base_path;              // path to base file
    std::string module;                 // optional module name
    std::vector<std::string> filenames; // all source files, base is first entry
    std::vector<line_t> lines;          // input source files split into lines
    std::vector<snippet_t> snippets;    // @block, @vs and @fs snippets
    std::map<std::string, std::string> type_map;    // @type uniform type definitions
    std::map<std::string, int> snippet_map; // name-index mapping for all code snippets
    std::map<std::string, int> block_map;   // name-index mapping for @block snippets
    std::map<std::string, int> vs_map;      // name-index mapping for @vs snippets
    std::map<std::string, int> fs_map;      // name-index mapping for @fs snippets
    std::map<std::string, program_t> programs;    // all @program definitions

    input_t() { };
    static input_t load_and_parse(const std::string& path);
    void dump_debug(errmsg_t::msg_format_t err_fmt) const;

    errmsg_t error(int index, const std::string& msg) const {
        if (index < (int)lines.size()) {
            const line_t& line = lines[index];
            return errmsg_t::error(filenames[line.filename], line.index, msg);
        }
        else {
            return errmsg_t::error(base_path, 0, msg);
        }
    };
    errmsg_t warning(int index, const std::string& msg) const {
        if (index < (int)lines.size()) {
            const line_t& line = lines[index];
            return errmsg_t::warning(filenames[line.filename], line.index, msg);
        }
        else {
            return errmsg_t::warning(base_path, 0, msg);
        }
    };
};

/* a SPIRV-bytecode blob with "back-link" to input_t.snippets */
struct spirv_blob_t {
    int snippet_index = -1;         // index into input_t.snippets
    std::vector<uint32_t> bytecode; // the SPIRV blob

    spirv_blob_t(int snippet_index): snippet_index(snippet_index) { };
};

/* glsl-to-spirv compiler wrapper */
struct spirv_t {
    std::vector<errmsg_t> errors;
    std::vector<spirv_blob_t> blobs;

    static void initialize_spirv_tools();
    static void finalize_spirv_tools();
    static spirv_t compile_glsl(const input_t& inp, slang_t::type_t slang);
    void dump_debug(const input_t& inp, errmsg_t::msg_format_t err_fmt) const;
};

/* reflection info */
struct attr_t {
    static const int NUM = 16;      // must be identical with NUM_VERTEX_ATTRS
    int slot = -1;
    std::string name;
    std::string sem_name;
    int sem_index = 0;

    bool equals(const attr_t& rhs) const {
        return (slot == rhs.slot) &&
               (name == rhs.name) &&
               (sem_name == rhs.sem_name) &&
               (sem_index == rhs.sem_index);
    }
};

struct uniform_t {
    static const int NUM = 16;      // must be identical with SG_MAX_UB_MEMBERS
    enum type_t {
        INVALID,
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        MAT4,
    };
    std::string name;
    type_t type = INVALID;
    int array_count = 1;
    int offset = 0;

    static const char* type_to_str(type_t t) {
        switch (t) {
            case FLOAT:     return "FLOAT";
            case FLOAT2:    return "FLOAT2";
            case FLOAT3:    return "FLOAT3";
            case FLOAT4:    return "FLOAT4";
            case MAT4:      return "MAT4";
            default:        return "INVALID";
        }
    }

    bool equals(const uniform_t& other) const {
        return (name == other.name) &&
               (type == other.type) &&
               (array_count == other.array_count) &&
               (offset == other.offset);
    }
};

struct uniform_block_t {
    static const int NUM = 4;     // must be identical with SG_MAX_SHADERSTAGE_UBS
    int slot = -1;
    int size = 0;
    std::string name;
    std::vector<uniform_t> uniforms;
    int unique_index = -1;      // index into spirvcross_t.unique_uniform_blocks

    bool equals(const uniform_block_t& other) const {
        if ((slot != other.slot) ||
            (size != other.size) ||
            (name != other.name) ||
            (uniforms.size() != other.uniforms.size()))
        {
            return false;
        }
        for (int i = 0; i < (int)uniforms.size(); i++) {
            if (!uniforms[i].equals(other.uniforms[i])) {
                return false;
            }
        }
        return true;
    }
};

struct image_t {
    static const int NUM = 12;        // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    enum type_t {
        INVALID,
        IMAGE_2D,
        IMAGE_CUBE,
        IMAGE_3D,
        IMAGE_ARRAY
    };
    int slot = -1;
    std::string name;
    type_t type = INVALID;
    int unique_index = -1;      // index into spirvcross_t.unique_images

    static const char* type_to_str(type_t t) {
        switch (t) {
            case IMAGE_2D:      return "IMAGE_2D";
            case IMAGE_CUBE:    return "IMAGE_CUBE";
            case IMAGE_3D:      return "IMAGE_3D";
            case IMAGE_ARRAY:   return "IMAGE_ARRAY";
            default:            return "INVALID";
        }
    }

    bool equals(const image_t& other) {
        return (slot == other.slot) && (name == other.name) && (type == other.type);
    }
};

struct stage_t {
    enum type_t {
        INVALID,
        VS,
        FS,
    };
    static const char* to_str(type_t t) {
        switch (t) {
            case VS: return "VS";
            case FS: return "FS";
            default: return "INVALID";
        }
    }
};

struct spirvcross_refl_t {
    stage_t::type_t stage = stage_t::INVALID;
    std::string entry_point;
    std::array<attr_t, attr_t::NUM> inputs;
    std::array<attr_t, attr_t::NUM> outputs;
    std::vector<uniform_block_t> uniform_blocks;
    std::vector<image_t> images;
};

/* result of a spirv-cross compilation */
struct spirvcross_source_t {
    bool valid = false;
    int snippet_index = -1;
    std::string source_code;
    spirvcross_refl_t refl;
};

/* spirv-cross wrapper */
struct spirvcross_t {
    errmsg_t error;
    std::vector<spirvcross_source_t> sources;
    std::vector<uniform_block_t> unique_uniform_blocks;
    std::vector<image_t> unique_images;

    static spirvcross_t translate(const input_t& inp, const spirv_t& spirv, slang_t::type_t slang);
    int find_source_by_snippet_index(int snippet_index) const;
    void dump_debug(errmsg_t::msg_format_t err_fmt, slang_t::type_t slang) const;
};

/* HLSL/Metal to bytecode compiler wrapper */
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

/* C header-generator for sokol_gfx.h */
struct sokol_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

/* bare format generator */
struct bare_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

/* utility functions for generators */
inline std::string mod_prefix(const input_t& inp) {
    if (inp.module.empty()) {
        return "";
    }
    else {
        return fmt::format("{}_", inp.module);
    }
}

struct output_t {
    static errmsg_t check_errors(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang);
};

} // namespace shdc
