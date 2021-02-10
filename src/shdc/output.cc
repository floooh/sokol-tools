/*
    Utility functions shared by output generators
 */
#include "shdc.h"

namespace shdc {
namespace output {

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

int uniform_type_size(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT:  return 4;
        case uniform_t::FLOAT2: return 8;
        case uniform_t::FLOAT3: return 12;
        case uniform_t::FLOAT4: return 16;
        case uniform_t::MAT4:   return 64;
        default: return 0;
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

} // namespace output
} // namespace shdc
