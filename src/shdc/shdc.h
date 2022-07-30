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

// the output shader languages to create
struct slang_t {
    enum type_t {
        GLSL330 = 0,
        GLSL100,
        GLSL300ES,
        HLSL4,
        HLSL5,
        METAL_MACOS,
        METAL_IOS,
        METAL_SIM,
        WGPU,
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
            case HLSL4:         return "hlsl4";
            case HLSL5:         return "hlsl5";
            case METAL_MACOS:   return "metal_macos";
            case METAL_IOS:     return "metal_ios";
            case METAL_SIM:     return "metal_sim";
            case WGPU:          return "wgpu";
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
    static bool is_glsl(type_t c) {
        switch (c) {
            case GLSL330:
            case GLSL100:
            case GLSL300ES:
                return true;
            default:
                return false;
        }
    }
    static bool is_hlsl(type_t c) {
        switch (c) {
            case HLSL4:
            case HLSL5:
                return true;
            default:
                return false;
        }
    }
    static bool is_msl(type_t c) {
        switch (c) {
            case METAL_MACOS:
            case METAL_IOS:
            case METAL_SIM:
                return true;
            default:
                return false;
        }
    }
    static bool is_wgpu(type_t c) {
        return WGPU == c;
    }
    static slang_t::type_t first_valid(uint32_t mask) {
        int i = 0;
        for (i = 0; i < NUM; i++) {
            if (0 != (mask & (1<<i))) {
                break;
            }
        }
        return (slang_t::type_t)i;
    }
};

// the output format
struct format_t	{
    enum type_t {
        SOKOL = 0,
        SOKOL_DECL,
        SOKOL_IMPL,
        SOKOL_ZIG,
        SOKOL_NIM,
        SOKOL_ODIN,
        BARE,
        NUM,
        INVALID,
    };

    static const char* to_str(type_t f) {
        switch (f) {
            case SOKOL:         return "sokol";
            case SOKOL_DECL:    return "sokol_decl";
            case SOKOL_IMPL:    return "sokol_impl";
            case SOKOL_ZIG:     return "sokol_zig";
            case SOKOL_NIM:     return "sokol_nim";
            case SOKOL_ODIN:    return "sokol_odin";
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
        else if (str == "sokol_zig") {
            return SOKOL_ZIG;
        }
        else if (str == "sokol_nim") {
            return SOKOL_NIM;
        }
        else if (str == "sokol_odin") {
            return SOKOL_ODIN;
        }
        else if (str == "bare") {
            return BARE;
        }
        else {
            return INVALID;
        }
    }
};

// an error message object with filename, line number and message
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
            return fmt::format("{}({}): {}: {}", file, line_index, (type==ERROR)?"error":"warning", msg);
        }
        else {
            return fmt::format("{}:{}:0: {}: {}", file, line_index, (type==ERROR)?"error":"warning", msg);
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
    int gen_version = 1;                // generator-version stamp
    errmsg_t::msg_format_t error_format = errmsg_t::GCC;  // format for error messages

    static args_t parse(int argc, const char** argv);
    void dump_debug() const;
};

// per-shader compile options
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

// a named code-snippet (@block, @vs or @fs) in the input source file
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

// a vertex-/fragment-shader pair (@program)
struct program_t {
    std::string name;
    std::string vs_name;    // name of vertex shader snippet
    std::string fs_name;    // name of fragment shader snippet
    int line_index = -1;    // line index in input source (zero-based)

    program_t() { };
    program_t(const std::string& n, const std::string& vs, const std::string& fs, int l): name(n), vs_name(vs), fs_name(fs), line_index(l) { };
};

// mapping each line to included filename and line index
struct line_t {
    std::string line;       // line content
    int filename = 0;       // index into input_t filenames
    int index = 0;          // line index == line nr - 1

    line_t() { };
    line_t(const std::string& ln, int fn, int ix): line(ln), filename(fn), index(ix) { };
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
    static spirv_t compile_input_glsl(const input_t& inp, slang_t::type_t slang, const std::vector<std::string>& defines);
    static spirv_t compile_spirvcross_glsl(const input_t& inp, slang_t::type_t slang, const spirvcross_t* spirvcross);
    void dump_debug(const input_t& inp, errmsg_t::msg_format_t err_fmt) const;
};

// reflection info
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
        INT,
        INT2,
        INT3,
        INT4,
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
            case INT:       return "INT";
            case INT2:      return "INT2";
            case INT3:      return "INT3";
            case INT4:      return "INT4";
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
    std::string struct_name;
    std::string inst_name;
    std::vector<uniform_t> uniforms;
    int unique_index = -1;      // index into spirvcross_t.unique_uniform_blocks
    bool flattened = false;

    // FIXME: hmm is this correct??
    bool equals(const uniform_block_t& other) const {
        if ((slot != other.slot) ||
            (size != other.size) ||
            (struct_name != other.struct_name) ||
            (uniforms.size() != other.uniforms.size()) ||
            (flattened != other.flattened))
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
        IMAGE_TYPE_INVALID,
        IMAGE_TYPE_2D,
        IMAGE_TYPE_CUBE,
        IMAGE_TYPE_3D,
        IMAGE_TYPE_ARRAY
    };
    enum basetype_t {
        IMAGE_BASETYPE_INVALID,
        IMAGE_BASETYPE_FLOAT,
        IMAGE_BASETYPE_SINT,
        IMAGE_BASETYPE_UINT
    };
    int slot = -1;
    std::string name;
    type_t type = IMAGE_TYPE_INVALID;
    basetype_t base_type = IMAGE_BASETYPE_INVALID;
    int unique_index = -1;      // index into spirvcross_t.unique_images

    static const char* type_to_str(type_t t) {
        switch (t) {
            case IMAGE_TYPE_2D:     return "IMAGE_TYPE_2D";
            case IMAGE_TYPE_CUBE:   return "IMAGE_TYPE_CUBE";
            case IMAGE_TYPE_3D:     return "IMAGE_TYPE_3D";
            case IMAGE_TYPE_ARRAY:  return "IMAGE_TYPE_ARRAY";
            default:                return "IMAGE_TYPE_INVALID";
        }
    }
    static const char* basetype_to_str(basetype_t t) {
        switch (t) {
            case IMAGE_BASETYPE_FLOAT:  return "IMAGE_BASETYPE_FLOAT";
            case IMAGE_BASETYPE_SINT:   return "IMAGE_BASETYPE_SINT";
            case IMAGE_BASETYPE_UINT:   return "IMAGE_BASETYPE_UINT";
            default:                    return "IMAGE_BASETYPE_INVALID";
        }
    }

    bool equals(const image_t& other) {
        return (slot == other.slot) && (name == other.name) && (type == other.type) && (base_type == other.base_type);
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

// result of a spirv-cross compilation
struct spirvcross_source_t {
    bool valid = false;
    int snippet_index = -1;
    std::string source_code;
    spirvcross_refl_t refl;
};

// spirv-cross wrapper
struct spirvcross_t {
    errmsg_t error;
    std::vector<spirvcross_source_t> sources;
    std::vector<uniform_block_t> unique_uniform_blocks;
    std::vector<image_t> unique_images;

    static spirvcross_t translate(const input_t& inp, const spirv_t& spirv, slang_t::type_t slang);
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

// bare format generator
struct bare_t {
    static errmsg_t gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode);
};

// utility functions for generators
namespace util {
    errmsg_t check_errors(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang);
    const char* uniform_type_str(uniform_t::type_t type);
    int uniform_size(uniform_t::type_t type, int array_size);
    int roundup(int val, int round_to);
    std::string mod_prefix(const input_t& inp);
    const uniform_block_t* find_uniform_block(const spirvcross_refl_t& refl, int slot);
    const image_t* find_image(const spirvcross_refl_t& refl, int slot);
    const spirvcross_source_t* find_spirvcross_source_by_shader_name(const std::string& shader_name, const input_t& inp, const spirvcross_t& spirvcross);
    const bytecode_blob_t* find_bytecode_blob_by_shader_name(const std::string& shader_name, const input_t& inp, const bytecode_t& bytecode);
    std::string to_camel_case(const std::string& str);
    std::string to_pascal_case(const std::string& str);
    std::string to_ada_case(const std::string& str);
    std::string replace_C_comment_tokens(const std::string& str);
};

} // namespace shdc
