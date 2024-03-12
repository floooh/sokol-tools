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
        WGSL,
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
            case WGSL:          return "wgsl";
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
    static bool is_wgsl(type_t c) {
        return WGSL == c;
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
        SOKOL_RUST,
        BARE,
        BARE_YAML,
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
            case SOKOL_RUST:    return "sokol_rust";
            case BARE:          return "bare";
            case BARE_YAML:     return "bare_yaml";
            default:            return "<invalid>";
        }
    }
    static type_t from_str(const std::string& str) {
        if (str == "sokol") {
            return SOKOL;
        } else if (str == "sokol_decl") {
            return SOKOL_DECL;
        } else if (str == "sokol_impl") {
            return SOKOL_IMPL;
        } else if (str == "sokol_zig") {
            return SOKOL_ZIG;
        } else if (str == "sokol_nim") {
            return SOKOL_NIM;
        } else if (str == "sokol_odin") {
            return SOKOL_ODIN;
        } else if (str == "sokol_rust") {
            return SOKOL_RUST;
        } else if (str == "bare") {
            return BARE;
        } else if (str == "bare_yaml") {
            return BARE_YAML;
        } else {
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
        } else {
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
    bool save_intermediate_spirv = false;   // save intermediate SPIRV bytecode (glslangvalidator output)
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
    };

    static type_t from_string(const std::string& str) {
        if (str == "fixup_clipspace") {
            return FIXUP_CLIPSPACE;
        } else if (str == "flip_vert_y") {
            return FLIP_VERT_Y;
        } else {
            return INVALID;
        }
    }
};

// image-type (see sg_image_type)
struct image_type_t {
    enum type_t {
        INVALID,
        _2D,
        CUBE,
        _3D,
        ARRAY
    };

    static const char* to_str(type_t t) {
        switch (t) {
            case _2D:   return "2d";
            case CUBE:  return "cube";
            case _3D:   return "3d";
            case ARRAY: return "array";
            default:    return "invalid";
        }
    }
};

// image-sample-type (see sg_image_sample_type)
struct image_sample_type_t {
    enum type_t {
        INVALID,
        FLOAT,
        SINT,
        UINT,
        DEPTH,
        UNFILTERABLE_FLOAT,
    };
    static const char* to_str(type_t t) {
        switch (t) {
            case FLOAT:  return "float";
            case SINT:   return "sint";
            case UINT:   return "uint";
            case DEPTH:  return "depth";
            case UNFILTERABLE_FLOAT: return "unfilterable_float";
            default:     return "invalid";
        }
    }
    static image_sample_type_t::type_t from_str(const std::string& str) {
        if (str == "float") return FLOAT;
        else if (str == "sint") return SINT;
        else if (str == "uint") return UINT;
        else if (str == "depth") return DEPTH;
        else if (str == "unfilterable_float") return UNFILTERABLE_FLOAT;
        else return INVALID;
    }
    static bool is_valid_str(const std::string& str) {
        return (str == "float")
            || (str == "sint")
            || (str == "uint")
            || (str == "depth")
            || (str == "unfilterable_float");
    }
    static const char* valid_image_sample_types_as_str() {
        return "float|sint|uint|depth|unfilterable_float";
    }
};

struct sampler_type_t {
    enum type_t {
        INVALID,
        FILTERING,
        COMPARISON,
        NONFILTERING,
    };
    static const char* to_str(type_t t) {
        switch (t) {
            case FILTERING:    return "filtering";
            case COMPARISON:   return "comparison";
            case NONFILTERING: return "nonfiltering";
            default:           return "invalid";
        }
    }
    static sampler_type_t::type_t from_str(const std::string& str) {
        if (str == "filtering") return FILTERING;
        else if (str == "comparison") return COMPARISON;
        else if (str == "nonfiltering") return NONFILTERING;
        else return INVALID;
    }
    static bool is_valid_str(const std::string& str) {
        return (str == "filtering")
            || (str == "comparison")
            || (str == "nonfiltering");
    }
    static const char* valid_sampler_types_as_str() {
        return "filtering|comparison|nonfiltering";
    }
};

struct image_sample_type_tag_t {
    std::string tex_name;
    image_sample_type_t::type_t type = image_sample_type_t::INVALID;
    int line_index = 0;
    image_sample_type_tag_t() { };
    image_sample_type_tag_t(const std::string& n, image_sample_type_t::type_t t, int l): tex_name(n), type(t), line_index(l) { };
};

struct sampler_type_tag_t {
    std::string smp_name;
    sampler_type_t::type_t type = sampler_type_t::INVALID;
    int line_index = 0;
    sampler_type_tag_t() { };
    sampler_type_tag_t(const std::string& n, sampler_type_t::type_t t, int l): smp_name(n), type(t), line_index(l) { };
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
    std::map<std::string, image_sample_type_tag_t> image_sample_type_tags;
    std::map<std::string, sampler_type_tag_t> sampler_type_tags;
    std::string name;
    std::vector<int> lines; // resolved zero-based line-indices (including @include_block)

    snippet_t() { };
    snippet_t(type_t t, const std::string& n): type(t), name(n) { };

    const image_sample_type_tag_t* lookup_image_sample_type_tag(const std::string& tex_name) const {
        auto it = image_sample_type_tags.find(tex_name);
        if (it != image_sample_type_tags.end()) {
            return &image_sample_type_tags.at(tex_name);
        } else {
            return nullptr;
        }
    }
    const sampler_type_tag_t* lookup_sampler_type_tag(const std::string& smp_name) const {
        auto it = sampler_type_tags.find(smp_name);
        if (it != sampler_type_tags.end()) {
            return &sampler_type_tags.at(smp_name);
        } else {
            return nullptr;
        }
    }
    static const char* type_to_str(type_t t) {
        switch (t) {
            case BLOCK: return "block";
            case VS: return "vs";
            case FS: return "fs";
            default: return "<invalid>";
        }
    }
    static bool is_vs(type_t t) {
        return VS == t;
    }
    static bool is_fs(type_t t) {
        return FS == t;
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
            case FLOAT:     return "float";
            case FLOAT2:    return "float2";
            case FLOAT3:    return "float3";
            case FLOAT4:    return "float4";
            case INT:       return "int";
            case INT2:      return "int2";
            case INT3:      return "int3";
            case INT4:      return "int4";
            case MAT4:      return "mat4";
            default:        return "invalid";
        }
    }
    static bool is_valid_glsl_uniform_type(const std::string& str) {
        return (str == "float")
            || (str == "vec2")
            || (str == "vec3")
            || (str == "vec4")
            || (str == "int")
            || (str == "int2")
            || (str == "int3")
            || (str == "int4")
            || (str == "mat4");
    }
    static const char* valid_glsl_uniform_types_as_str() {
        return "float|vec2|vec3|vec4|int|int2|int3|int4|mat4";
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
    int slot = -1;
    std::string name;
    image_type_t::type_t type = image_type_t::INVALID;
    image_sample_type_t::type_t sample_type = image_sample_type_t::INVALID;
    bool multisampled = false;
    int unique_index = -1;      // index into spirvcross_t.unique_images

    bool equals(const image_t& other) {
        return (slot == other.slot)
            && (name == other.name)
            && (type == other.type)
            && (sample_type == other.sample_type)
            && (multisampled == other.multisampled);
    }
};

struct sampler_t {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_SAMPLERS
    int slot = -1;
    std::string name;
    sampler_type_t::type_t type = sampler_type_t::INVALID;
    int unique_index = -1;          // index into spirvcross_t.unique_samplers

    bool equals(const sampler_t& other) {
        return (slot == other.slot)
            && (name == other.name)
            && (type == other.type);
    }
};

// special combined-image-samplers for GLSL output with GL semantics
struct image_sampler_t {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    int slot = -1;
    std::string name;
    std::string image_name;
    std::string sampler_name;
    int unique_index = -1;

    bool equals(const image_sampler_t& other) {
        return (slot == other.slot)
            && (name == other.name)
            && (image_name == other.image_name)
            && (sampler_name == other.sampler_name);
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
    static bool is_vs(type_t t) {
        return VS == t;
    }
    static bool is_fs(type_t t) {
        return FS == t;
    }
};

struct reflection_t {
    stage_t::type_t stage = stage_t::INVALID;
    std::string entry_point;
    std::array<attr_t, attr_t::NUM> inputs;
    std::array<attr_t, attr_t::NUM> outputs;
    std::vector<uniform_block_t> uniform_blocks;
    std::vector<image_t> images;
    std::vector<sampler_t> samplers;
    std::vector<image_sampler_t> image_samplers;

    static reflection_t parse(const spirv_cross::Compiler& compiler, const snippet_t& snippet, slang_t::type_t slang);

    const uniform_block_t* find_uniform_block_by_slot(int slot) const;
    const image_t* find_image_by_slot(int slot) const;
    const image_t* find_image_by_name(const std::string& name) const;
    const sampler_t* find_sampler_by_slot(int slot) const;
    const sampler_t* find_sampler_by_name(const std::string& name) const;
    const image_sampler_t* find_image_sampler_by_slot(int slot) const;
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
