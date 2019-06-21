/*
    Generate bare output in text or binary format
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

static const char* slang_file_extension(slang_t::type_t c, bool binary) {
    switch (c) {
        case slang_t::GLSL330:
        case slang_t::GLSL100:
        case slang_t::GLSL300ES:
            return ".glsl";
        case slang_t::HLSL5:
            return binary ? ".fxc" : ".hlsl";
        case slang_t::METAL_MACOS:
        case slang_t::METAL_IOS:
        case slang_t::METAL_SIM:
            return binary ? ".metallib" : ".metal";
        default:
            return "";
    }
}

static void write_shader_sources_and_blobs(const input_t& inp,
                                           const spirvcross_t& spirvcross,
                                           const bytecode_t& bytecode,
                                           slang_t::type_t slang,
                                           const std::string& output)
{
    for (int snippet_index = 0; snippet_index < (int)inp.snippets.size(); snippet_index++) {
        const snippet_t& snippet = inp.snippets[snippet_index];
        if ((snippet.type != snippet_t::VS) && (snippet.type != snippet_t::FS)) {
            continue;
        }
        int src_index = spirvcross.find_source_by_snippet_index(snippet_index);
        assert(src_index >= 0);
        const spirvcross_source_t& src = spirvcross.sources[src_index];
        int blob_index = bytecode.find_blob_by_snippet_index(snippet_index);
        const bytecode_blob_t* blob = 0;
        if (blob_index != -1) {
            blob = &bytecode.blobs[blob_index];
        }

        // output file name
        std::string file_path(output);
        file_path += ".";
        file_path += snippet_t::type_to_str(snippet.type);
        file_path += slang_file_extension(slang, blob);

        // write text or binary to output file
        FILE* f = fopen(file_path.c_str(), "wb");
        if (blob) {
            fwrite(blob->data.data(), 1, blob->data.size(), f);
        }
        else {
            fwrite(src.source_code.data(), 1, src.source_code.length(), f);
        }
        fflush(f);
        fclose(f);
    }
}

errmsg_t bare_t::gen(const args_t& args, const input_t& inp,
                     const std::array<spirvcross_t,slang_t::NUM>& spirvcross,
                     const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            /*errmsg_t err = check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }*/
            write_shader_sources_and_blobs(inp, spirvcross[i], bytecode[i], slang, args.output);
        }
    }

    return errmsg_t();
}

} // namespace shdc
