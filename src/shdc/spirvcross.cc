/*
    translate SPIRV bytecode to shader sources and generate
    uniform block reflection information, wrapper around
    https://github.com/KhronosGroup/SPIRV-Cross
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"

using namespace spirv_cross;

namespace shdc {

static void fix_ub_matrix_force_colmajor(Compiler& compiler) {
    /* go though all uniform block matrixes and decorate them with
        column-major, this is needed in the HLSL backend to fix the
        multiplication order    
    */
    ShaderResources res = compiler.get_shader_resources();
    for (const Resource& ub_res: res.uniform_buffers) {
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            if ((m_type.basetype == SPIRType::Float) && (m_type.vecsize > 1) && (m_type.columns > 1)) {
                compiler.set_member_decoration(ub_res.base_type_id, m_index, spv::DecorationColMajor);
            }
        }
    }
}

static void flatten_uniform_blocks(CompilerGLSL& compiler) {
    /* this flattens each uniform block into a vec4 array, in WebGL/GLES2 this
        allows more efficient uniform updates
    */
    ShaderResources res = compiler.get_shader_resources();
    for (const Resource& ub_res: res.uniform_buffers) {
        compiler.flatten_buffer_block(ub_res.id);
    }
}

static spirvcross_source_t to_glsl(const spirv_blob_t& blob, int glsl_version, bool is_gles) {
    CompilerGLSL compiler(blob.bytecode);
    CompilerGLSL::Options options;
    options.version = glsl_version;
    options.es = is_gles;
    options.enable_420pack_extension = false;
    options.vertex.fixup_clipspace = false;    /* ??? */
    compiler.set_common_options(options);
    fix_ub_matrix_force_colmajor(compiler);
    flatten_uniform_blocks(compiler);
    std::string src = compiler.compile();
    spirvcross_source_t res;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
    }
    return res;
}

static spirvcross_source_t to_hlsl5(const spirv_blob_t& blob) {
    CompilerHLSL compiler(blob.bytecode);
    CompilerHLSL::Options options;
    options.shader_model = 50;
    options.point_size_compat = true;
    compiler.set_hlsl_options(options);
    fix_ub_matrix_force_colmajor(compiler);
    std::string src = compiler.compile();
    spirvcross_source_t res;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
    }
    return res;
}

static spirvcross_source_t to_msl(const spirv_blob_t& blob, CompilerMSL::Options::Platform plat) {
    CompilerMSL compiler(blob.bytecode);
    CompilerMSL::Options options;
    options.platform = plat;
    compiler.set_msl_options(options);
    std::string src = compiler.compile();
    spirvcross_source_t res;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
    }
    return res;
}

spirvcross_t spirvcross_t::translate(const input_t& inp, const spirv_t& spirv, uint32_t slang_mask) {
    spirvcross_t spv_cross;
    for (const auto& blob: spirv.blobs) {
        for (int slang = 0; slang < slang_t::NUM; slang++) {
            spirvcross_source_t src;
            if (slang_mask & slang_t::bit((slang_t::type_t)slang)) {
                switch (slang) {
                    case slang_t::GLSL330:
                        src = to_glsl(blob, 330, false);
                        break;
                    case slang_t::GLSL100:
                        src = to_glsl(blob, 100, true);
                        break;
                    case slang_t::GLSL300ES:
                        src = to_glsl(blob, 300, true);
                        break;
                    case slang_t::HLSL5:
                        src = to_hlsl5(blob);
                        break;
                    case slang_t::METAL_MACOS:
                        src = to_msl(blob, CompilerMSL::Options::macOS);
                        break;
                    case slang_t::METAL_IOS:
                        src = to_msl(blob, CompilerMSL::Options::iOS);
                        break;
                }
                if (src.valid) {
                    src.snippet_index = blob.snippet_index;
                    spv_cross.sources[slang].push_back(std::move(src));
                }
                else {
                    const int line_num = inp.snippets[blob.snippet_index].lines[0];
                    std::string err_msg = fmt::format("Failed to cross-compile to {}.", slang_t::to_str((slang_t::type_t)slang));
                    spv_cross.error = error_t(inp.path, line_num, err_msg);
                    return spv_cross;
                }
            }
        }
    }
    return spv_cross;
}

void spirvcross_t::dump_debug(error_t::msg_format_t err_fmt) const {
    fmt::print(stderr, "spirvcross_t:\n");
    if (error.valid) {
        fmt::print(stderr, "  error: {}\n", error.as_string(err_fmt));
    }
    else {
        fmt::print(stderr, "  error: not set\n");
    }
    std::vector<std::string> lines;
    for (int slang = 0; slang < slang_t::NUM; slang++) {
        if (sources[slang].size() > 0) {
            fmt::print(stderr, "  sources for {}:\n", slang_t::to_str((slang_t::type_t)slang));
            for (const spirvcross_source_t& source: sources[slang]) {
                fmt::print(stderr, "    snippet {}:\n", source.snippet_index);
                pystring::splitlines(source.source_code, lines);
                for (const std::string& line: lines) {
                    fmt::print(stderr, "      {}\n", line);
                }
            }
        }
    }
}

} // namespace shdc

