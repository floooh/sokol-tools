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
struct Snippet {
    enum Type {
        INVALID,
        BLOCK,
        VS,
        FS,
        CS,
    };
    int index = -1;
    Type type = INVALID;
    std::array<uint32_t, Slang::Num> options = { };
    std::string name;
    std::vector<int> lines; // resolved zero-based line-indices (including @include_block)

    Snippet();
    Snippet(Type t, const std::string& n);
    static const char* type_to_str(Type t);
    static bool is_vs(Type t);
    static bool is_fs(Type t);
    static bool is_cs(Type t);
};

inline Snippet::Snippet() { };

inline Snippet::Snippet(Type t, const std::string& n): type(t), name(n) { };

inline const char* Snippet::type_to_str(Type t) {
    switch (t) {
        case BLOCK: return "block";
        case VS: return "vs";
        case FS: return "fs";
        case CS: return "cs";
        default: return "<invalid>";
    }
}

inline bool Snippet::is_vs(Type t) {
    return VS == t;
}

inline bool Snippet::is_fs(Type t) {
    return FS == t;
}

inline bool Snippet::is_cs(Type t) {
    return CS == t;
}

} // namespace shdc
