/*
    Utility functions shared by output generators
 */
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"

namespace shdc {
namespace util {

errmsg_t check_errors(const input_t& inp,
                      const spirvcross_t& spirvcross,
                      slang_t::type_t slang)
{
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        if (vs_src_index < 0) {
            return inp.error(inp.snippets[vs_snippet_index].lines[0],
                fmt::format("no generated '{}' source for vertex shader '{}' in program '{}'",
                    slang_t::to_str(slang), prog.vs_name, prog.name));
        }
        if (fs_src_index < 0) {
            return inp.error(inp.snippets[vs_snippet_index].lines[0],
                fmt::format("no generated '{}' source for fragment shader '{}' in program '{}'",
                    slang_t::to_str(slang), prog.fs_name, prog.name));
        }
    }
    // all ok
    return errmsg_t();
}

const char* uniform_type_str(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT: return "float";
        case uniform_t::FLOAT2: return "vec2";
        case uniform_t::FLOAT3: return "vec3";
        case uniform_t::FLOAT4: return "vec4";
        case uniform_t::MAT4: return "mat4";
        default: return "FIXME";
    }
}

int uniform_size(uniform_t::type_t type, int array_size) {
    if (array_size > 1) {
        switch (type) {
            case uniform_t::FLOAT4:
            case uniform_t::INT4:
                return 16 * array_size;
            case uniform_t::MAT4:
                return 64 * array_size;
            default: return 0;
        }
    }
    else {
        switch (type) {
            case uniform_t::FLOAT:
            case uniform_t::INT:
                return 4;
            case uniform_t::FLOAT2:
            case uniform_t::INT2:
                return 8;
            case uniform_t::FLOAT3:
            case uniform_t::INT3:
                return 12;
            case uniform_t::FLOAT4:
            case uniform_t::INT4:
                return 16;
            case uniform_t::MAT4:
                return 64;
            default: return 0;
        }
    }
}

int roundup(int val, int round_to) {
    return (val + (round_to - 1)) & ~(round_to - 1);
}

std::string mod_prefix(const input_t& inp) {
    if (inp.module.empty()) {
        return "";
    }
    else {
        return fmt::format("{}_", inp.module);
    }
}

const uniform_block_t* find_uniform_block(const spirvcross_refl_t& refl, int slot) {
    for (const uniform_block_t& ub: refl.uniform_blocks) {
        if (ub.slot == slot) {
            return &ub;
        }
    }
    return nullptr;
}

const image_t* find_image(const spirvcross_refl_t& refl, int slot) {
    for (const image_t& img: refl.images) {
        if (img.slot == slot) {
            return &img;
        }
    }
    return nullptr;
}

const spirvcross_source_t* find_spirvcross_source_by_shader_name(const std::string& shader_name, const input_t& inp, const spirvcross_t& spirvcross) {
    assert(!shader_name.empty());
    int snippet_index = inp.snippet_map.at(shader_name);
    int src_index = spirvcross.find_source_by_snippet_index(snippet_index);
    if (src_index >= 0) {
        return &spirvcross.sources[src_index];
    }
    else {
        return nullptr;
    }
}

const bytecode_blob_t* find_bytecode_blob_by_shader_name(const std::string& shader_name, const input_t& inp, const bytecode_t& bytecode) {
    assert(!shader_name.empty());
    int snippet_index = inp.snippet_map.at(shader_name);
    int blob_index = bytecode.find_blob_by_snippet_index(snippet_index);
    if (blob_index >= 0) {
        return &bytecode.blobs[blob_index];
    }
    else {
        return nullptr;
    }
}

std::string to_pascal_case(const std::string& str) {
    std::vector<std::string> splits;
    pystring::split(str, splits, "_");
    std::vector<std::string> parts;
    for (const auto& part: splits) {
        parts.push_back(pystring::capitalize(part));
    }
    return pystring::join("", parts);
}

std::string to_ada_case(const std::string& str) {
    std::vector<std::string> splits;
    pystring::split(str, splits, "_");
    std::vector<std::string> parts;
    for (const auto& part: splits) {
        parts.push_back(pystring::capitalize(part));
    }
    return pystring::join("_", parts);
}

std::string to_camel_case(const std::string& str) {
    std::string res = to_pascal_case(str);
    res[0] = tolower(res[0]);
    return res;
}

std::string replace_C_comment_tokens(const std::string& str) {
    static const std::string comment_start_old = "/*";
    static const std::string comment_start_new = "/_";
    static const std::string comment_end_old = "*/";
    static const std::string comment_end_new = "_/";
    std::string s = pystring::replace(str, comment_start_old, comment_start_new);
    s = pystring::replace(s, comment_end_old, comment_end_new);
    return s;
}

} // namespace util
} // namespace shdc
