/*
    Generate bare output in text or binary format
*/
#include "bare.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

static ErrMsg write_file(const std::string& file_path, const SpirvcrossSource* src, const BytecodeBlob* blob) {
    FILE* f = fopen(file_path.c_str(), "wb");
    if (f == nullptr) {
        return ErrMsg::error(file_path, 0, fmt::format("failed to open output file '{}'", file_path));
    }
    const void* write_data;
    size_t write_count;
    if (blob) {
        write_data = blob->data.data();
        write_count = blob->data.size();
    } else {
        assert(src);
        write_data = src->source_code.data();
        write_count = src->source_code.length();
    }
    size_t written = fwrite(write_data, 1, write_count, f);
    if (written != write_count) {
        fclose(f);
        return ErrMsg::error(file_path, 0, fmt::format("failed to write output file '{}'", file_path));
    }
    fclose(f);
    return ErrMsg();
}

// completely override the generate function since there's no overlap with code-generators
ErrMsg BareGenerator::generate(const GenInput& gen) {
    mod_prefix = gen.inp.module.empty() ? "" : fmt::format("{}_", gen.inp.module);
    ErrMsg err = check_errors(gen);
    if (err.valid()) {
        return err;
    }
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            const Spirvcross& spirvcross = gen.spirvcross[slang];
            const Bytecode& bytecode = gen.bytecode[slang];
            for (const ProgramReflection& prog: gen.refl.progs) {
                for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                    const StageReflection& refl = prog.stages[stage_index];
                    const SpirvcrossSource* src = spirvcross.find_source_by_snippet_index(refl.snippet_index);
                    const BytecodeBlob* blob = bytecode.find_blob_by_snippet_index(refl.snippet_index);
                    const std::string file_path = shader_file_path(gen, prog.name, refl.stage_name, slang, blob != nullptr);
                    err = write_file(file_path, src, blob);
                    if (err.valid()) {
                        return err;
                    }
                }
            }
        }
    }
    return ErrMsg();
}

static const char* slang_file_extension(Slang::Enum c, bool binary) {
    switch (c) {
        case Slang::GLSL410:
        case Slang::GLSL430:
        case Slang::GLSL300ES:
            return ".glsl";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return binary ? ".fxc" : ".hlsl";
        case Slang::METAL_MACOS:
        case Slang::METAL_IOS:
        case Slang::METAL_SIM:
            return binary ? ".metallib" : ".metal";
        case Slang::WGSL:
            return ".wgsl";
        default:
            return "";
    }
}

std::string BareGenerator::shader_file_path(const GenInput& gen, const std::string& prog_name, const std::string& stage_name, Slang::Enum slang, bool is_binary) {
    return fmt::format("{}_{}{}_{}_{}{}",
        gen.args.output,
        mod_prefix,
        prog_name,
        Slang::to_str(slang),
        pystring::lower(stage_name),
        slang_file_extension(slang, is_binary));
}

} // namespace
