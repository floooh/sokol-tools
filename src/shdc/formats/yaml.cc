/*
    Generate bare output in text or binary format
*/
#include "yaml.h"
#include "bare.h"
#include "util.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::formats::yaml {

using namespace util;
using namespace refl;

static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

static Uniform as_flattened_uniform(const UniformBlock& block) {
    assert(block.flattened);
    assert(!block.uniforms.empty());

    const Uniform::Type type = [&block](){
        switch (block.uniforms[0].type) {
            case Uniform::FLOAT:
            case Uniform::FLOAT2:
            case Uniform::FLOAT3:
            case Uniform::FLOAT4:
            case Uniform::MAT4:
                return Uniform::FLOAT4;
            case Uniform::INT:
            case Uniform::INT2:
            case Uniform::INT3:
            case Uniform::INT4:
                return Uniform::INT4;
            default:
                assert(false);
                return Uniform::INVALID;
        }
    }();
    Uniform uniform;
    uniform.name = block.struct_name;
    uniform.type = type;
    uniform.array_count = roundup(block.size, 16) / 16;
    return uniform;
}

static void write_attribute(const VertexAttr& att) {
    L("            -\n");
    L("              slot: {}\n", att.slot);
    L("              name: {}\n", att.name);
    L("              sem_name: {}\n", att.sem_name);
    L("              sem_index: {}\n", att.sem_index);
}

static void write_uniform(const Uniform& uniform);
static void write_uniform_block(const UniformBlock& uniform_block) {
    L("            -\n");
    L("              slot: {}\n", uniform_block.slot);
    L("              size: {}\n", uniform_block.size);
    L("              struct_name: {}\n", uniform_block.struct_name);
    L("              inst_name: {}\n", uniform_block.inst_name);
    L("              uniforms:\n");


    if (uniform_block.flattened && !uniform_block.uniforms.empty()) {
        write_uniform(as_flattened_uniform(uniform_block));
        return;
    }

    for (const auto& uniform: uniform_block.uniforms) {
        write_uniform(uniform);
    }
}

static void write_uniform(const Uniform& uniform) {
    L("                -\n");
    L("                  name: {}\n", uniform.name);
    L("                  type: {}\n", Uniform::type_to_str(uniform.type));
    L("                  array_count: {}\n", uniform.array_count);
    L("                  offset: {}\n", uniform.offset);
}

static void write_image(const Image& image) {
    L("            -\n");
    L("              slot: {}\n", image.slot);
    L("              name: {}\n", image.name);
    L("              multisampled: {}\n", image.multisampled);
    L("              type: {}\n", ImageType::to_str(image.type));
    L("              sample_type: {}\n", ImageSampleType::to_str(image.sample_type));
}

static void write_sampler(const Sampler& sampler) {
    L("            -\n");
    L("              slot: {}\n", sampler.slot);
    L("              name: {}\n", sampler.name);
    L("              sampler_type: {}\n", SamplerType::to_str(sampler.type));
}

static void write_image_sampler(const ImageSampler& image_sampler) {
    L("            -\n");
    L("              slot: {}\n", image_sampler.slot);
    L("              name: {}\n", image_sampler.name);
    L("              image_name: {}\n", image_sampler.image_name);
    L("              sampler_name: {}\n", image_sampler.sampler_name);
}

static void write_source_reflection(const SpirvcrossSource* src) {
    L("          entry_point: {}\n", src->refl.entry_point);
    L("          inputs:\n");
    for (const auto& input: src->refl.inputs) {
        if (input.slot == -1) {
            break;
        }
        write_attribute(input);
    }
    L("          outputs:\n");
    for (const auto& output: src->refl.outputs) {
        if (output.slot == -1) {
            break;
        }
        write_attribute(output);
    }
    if (src->refl.bindings.uniform_blocks.size() > 0) {
        L("          uniform_blocks:\n");
        for (const auto& uniform_block: src->refl.bindings.uniform_blocks) {
            if (uniform_block.slot == -1) {
                break;
            }
            write_uniform_block(uniform_block);
        }
    }
    if (src->refl.bindings.images.size() > 0) {
        L("          images:\n");
        for (const auto& image: src->refl.bindings.images) {
            if (image.slot == -1) {
                break;
            }
            write_image(image);
        }
    }
    if (src->refl.bindings.samplers.size() > 0) {
        L("          samplers:\n");
        for (const auto& sampler: src->refl.bindings.samplers) {
            if (sampler.slot == -1) {
                break;
            }
            write_sampler(sampler);
        }
    }
    if (src->refl.bindings.image_samplers.size() > 0) {
        L("          image_sampler_pairs:\n");
        for (const auto& image_sampler: src->refl.bindings.image_samplers) {
            if (image_sampler.slot == -1) {
                break;
            }
            write_image_sampler(image_sampler);
        }
    }
}

static ErrMsg write_shader_sources_and_blobs(const Args& args,
                                               const Input& inp,
                                               const Spirvcross& spirvcross,
                                               const Bytecode& bytecode,
                                               Slang::Enum slang)
{
    L("    programs:\n");
    for (const auto& item: inp.programs) {
        const Program& prog = item.second;
        const SpirvcrossSource* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const SpirvcrossSource* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        const BytecodeBlob* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
        const BytecodeBlob* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);

        const std::string file_path_base = fmt::format("{}_{}{}_{}", args.output, mod_prefix(inp), prog.name, Slang::to_str(slang));
        const std::string file_path_vs = fmt::format("{}_vs{}", file_path_base, util::slang_file_extension(slang, vs_blob != nullptr));
        const std::string file_path_fs = fmt::format("{}_fs{}", file_path_base, util::slang_file_extension(slang, fs_blob != nullptr));

        L("      -\n");
        L("        name: {}\n", prog.name);
        L("        vs:\n");
        L("          path: {}\n", file_path_vs);
        L("          is_binary: {}\n", vs_blob != nullptr);
        write_source_reflection(vs_src);

        L("        fs:\n");
        L("          path: {}\n", file_path_fs);
        L("          is_binary: {}\n", fs_blob != nullptr);
        write_source_reflection(fs_src);
    }

    return ErrMsg();
}

ErrMsg gen(const Args& args, const Input& inp, const std::array<Spirvcross,Slang::NUM>& spirvcross, const std::array<Bytecode,Slang::NUM>& bytecode) {
    // first generate the bare-output files
    ErrMsg output_err = bare::gen(args, inp, spirvcross, bytecode);
    if (output_err.valid()) {
        return output_err;
    }

    file_content.clear();

    L("shaders:\n");

    for (int i = 0; i < Slang::NUM; i++) {
        Slang::Enum slang = (Slang::Enum) i;
        if (args.slang & Slang::bit(slang)) {
            ErrMsg err = check_errors(inp, spirvcross[i], slang);
            if (err.valid()) {
                return err;
            }

            L("  -\n");
            L("    slang: {}\n", Slang::to_str(slang));
            err = write_shader_sources_and_blobs(args, inp, spirvcross[i], bytecode[i], slang);
            if (err.valid()) {
                return err;
            }
        }
    }

    // write result into output file
    const std::string file_path = fmt::format("{}_{}reflection.yaml", args.output, mod_prefix(inp));
    FILE* f = fopen(file_path.c_str(), "w");
    if (!f) {
        return ErrMsg::error(inp.base_path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);

    return ErrMsg();
}

}
