/*
    Generate bare output in text or binary format
*/
#include "yaml.h"
#include "bare.h"
#include "util.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

ErrMsg YamlGenerator::generate(const GenInput& gen) {
    tab_width = 2;
    // first run the BareGenerator to generate shader source/blob files
    ErrMsg err = BareGenerator::generate(gen);
    if (err.valid()) {
        return err;
    }
    // next generate a YAML file with reflection info
    content.clear();
    l_open("shaders:\n");
    for (int slang_idx = 0; slang_idx < Slang::Num; slang_idx++) {
        Slang::Enum slang = Slang::from_index(slang_idx);
        if (gen.args.slang & Slang::bit(slang)) {
            // FIXME: all slang outputs will be the same, so this is kinda redundant
            l_open("-\n");
            l("slang: {}\n", Slang::to_str(slang));
            l_open("programs:\n");
            const Spirvcross& spirvcross = gen.spirvcross[slang];
            const Bytecode& bytecode = gen.bytecode[slang];
            for (const ProgramReflection& prog: gen.refl.progs) {
                l_open("-\n");
                l("name: {}\n", prog.name);
                for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                    const StageReflection& refl = prog.stages[stage_index];
                    const SpirvcrossSource* src = spirvcross.find_source_by_snippet_index(refl.snippet_index);
                    const BytecodeBlob* blob = bytecode.find_blob_by_snippet_index(refl.snippet_index);
                    const std::string file_path = shader_file_path(gen, prog.name, refl.stage_name, slang, blob != nullptr);
                    l_open("{}:\n", pystring::lower(refl.stage_name));
                    l("path: {}\n", file_path);
                    l("is_binary: {}\n", blob != nullptr);
                    l("entry_point: {}\n", refl.entry_point);
                    l_open("inputs:\n");
                    for (const auto& input: src->stage_refl.inputs) {
                        if (input.slot == -1) {
                            break;
                        }
                        gen_attr(input);
                    }
                    l_close();
                    l_open("outputs:\n");
                    for (const auto& output: src->stage_refl.outputs) {
                        if (output.slot == -1) {
                            break;
                        }
                        gen_attr(output);
                    }
                    l_close();
                    if (refl.bindings.uniform_blocks.size() > 0) {
                        l_open("uniform_blocks:\n");
                        for (const auto& uniform_block: refl.bindings.uniform_blocks) {
                            gen_uniform_block(uniform_block);
                        }
                        l_close();
                    }
                    if (refl.bindings.images.size() > 0) {
                        l_open("images:\n");
                        for (const auto& image: refl.bindings.images) {
                            gen_image(image);
                        }
                        l_close();
                    }
                    if (refl.bindings.samplers.size() > 0) {
                        l_open("samplers:\n");
                        for (const auto& sampler: refl.bindings.samplers) {
                            gen_sampler(sampler);
                        }
                        l_close();
                    }
                    if (src->stage_refl.bindings.image_samplers.size() > 0) {
                        l_open("image_sampler_pairs:\n");
                        for (const auto& image_sampler: src->stage_refl.bindings.image_samplers) {
                            gen_image_sampler(image_sampler);
                        }
                        l_close();
                    }
                    l_close();
                }
                l_close();
            }
            l_close();
            l_close();
        }
    }
    l_close();

    // write result into output file
    const std::string file_path = fmt::format("{}_{}reflection.yaml", gen.args.output, mod_prefix);
    FILE* f = fopen(file_path.c_str(), "w");
    if (!f) {
        return ErrMsg::error(gen.inp.base_path, 0, fmt::format("failed to open output file '{}'", file_path));
    }
    fwrite(content.c_str(), content.length(), 1, f);
    fclose(f);
    return ErrMsg();
}

void YamlGenerator::gen_attr(const StageAttr& att) {
    l_open("-\n");
    l("slot: {}\n", att.slot);
    l("name: {}\n", att.name);
    l("sem_name: {}\n", att.sem_name);
    l("sem_index: {}\n", att.sem_index);
    l_close();
}

void YamlGenerator::gen_uniform_block(const UniformBlock& ub) {
    l_open("-\n");
    l("slot: {}\n", ub.slot);
    l("size: {}\n", ub.struct_info.size);
    l("struct_name: {}\n", ub.struct_info.name);
    l("inst_name: {}\n", ub.inst_name);
    l_open("uniforms:\n");
    if (ub.flattened) {
        l_open("-\n");
        l("name: {}\n", ub.struct_info.struct_items[0].name);
        l("type: {}\n", flattened_uniform_type(ub.struct_info.struct_items[0].type));
        l("array_count: {}\n", roundup(ub.struct_info.struct_items[0].size, 16) / 16);
        l("offset: 0\n");
        l_close();
    } else {
        for (const Type& u: ub.struct_info.struct_items) {
            l_open("-\n");
            l("name: {}\n", u.name);
            l("type: {}\n", uniform_type(u.type));
            l("array_count: {}\n", u.array_count);
            l("offset: {}\n", u.offset);
            l_close();
        }
    }
    l_close();
    l_close();
}

void YamlGenerator::gen_image(const Image& image) {
    l_open("-\n");
    l("slot: {}\n", image.slot);
    l("name: {}\n", image.name);
    l("multisampled: {}\n", image.multisampled);
    l("type: {}\n", image_type(image.type));
    l("sample_type: {}\n", image_sample_type(image.sample_type));
    l_close();
}

void YamlGenerator::gen_sampler(const Sampler& sampler) {
    l_open("-\n");
    l("slot: {}\n", sampler.slot);
    l("name: {}\n", sampler.name);
    l("sampler_type: {}\n", sampler_type(sampler.type));
    l_close();
}

void YamlGenerator::gen_image_sampler(const ImageSampler& image_sampler) {
    l_open("-\n");
    l("slot: {}\n", image_sampler.slot);
    l("name: {}\n", image_sampler.name);
    l("image_name: {}\n", image_sampler.image_name);
    l("sampler_name: {}\n", image_sampler.sampler_name);
    l_close();
}

std::string YamlGenerator::uniform_type(Type::Enum e) {
    return Type::type_to_glsl(e);
}

std::string YamlGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return Type::type_to_glsl(Type::Float4);
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return Type::type_to_glsl(Type::Int4);
        default:
            return Type::type_to_glsl(Type::Invalid);
    }
}

std::string YamlGenerator::image_type(ImageType::Enum e) {
    return ImageType::to_str(e);
}

std::string YamlGenerator::image_sample_type(ImageSampleType::Enum e) {
    return ImageSampleType::to_str(e);
}

std::string YamlGenerator::sampler_type(SamplerType::Enum e) {
    return SamplerType::to_str(e);
}

} // namespace

