#pragma once
#include <array>
#include <map>
#include <vector>
#include <string>
#include "slang.h"
#include "image_sample_type_tag.h"
#include "sampler_type_tag.h"

namespace shdc {

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
    std::map<std::string, ImageSampleTypeTag> image_sample_type_tags;
    std::map<std::string, SamplerTypeTag> sampler_type_tags;
    std::string name;
    std::vector<int> lines; // resolved zero-based line-indices (including @include_block)

    snippet_t();
    snippet_t(type_t t, const std::string& n);
    const ImageSampleTypeTag* lookup_image_sample_type_tag(const std::string& tex_name) const;
    const SamplerTypeTag* lookup_sampler_type_tag(const std::string& smp_name) const;
    static const char* type_to_str(type_t t);
    static bool is_vs(type_t t);
    static bool is_fs(type_t t);
};

inline snippet_t::snippet_t() { };

inline snippet_t::snippet_t(type_t t, const std::string& n): type(t), name(n) { };

inline const ImageSampleTypeTag* snippet_t::lookup_image_sample_type_tag(const std::string& tex_name) const {
    auto it = image_sample_type_tags.find(tex_name);
    if (it != image_sample_type_tags.end()) {
        return &image_sample_type_tags.at(tex_name);
    } else {
        return nullptr;
    }
}

inline const SamplerTypeTag* snippet_t::lookup_sampler_type_tag(const std::string& smp_name) const {
    auto it = sampler_type_tags.find(smp_name);
    if (it != sampler_type_tags.end()) {
        return &sampler_type_tags.at(smp_name);
    } else {
        return nullptr;
    }
}

inline const char* snippet_t::type_to_str(type_t t) {
    switch (t) {
        case BLOCK: return "block";
        case VS: return "vs";
        case FS: return "fs";
        default: return "<invalid>";
    }
}

inline bool snippet_t::is_vs(type_t t) {
    return VS == t;
}

inline bool snippet_t::is_fs(type_t t) {
    return FS == t;
}

} // namespace shdc
