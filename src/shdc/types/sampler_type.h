#pragma once
#include  <string>

namespace shdc {

struct SamplerType {
    enum type_t {
        INVALID,
        FILTERING,
        COMPARISON,
        NONFILTERING,
    };
    static const char* to_str(type_t t);
    static SamplerType::type_t from_str(const std::string& str);
    static bool is_valid_str(const std::string& str);
    static const char* valid_sampler_types_as_str();
};

inline const char* SamplerType::to_str(type_t t) {
    switch (t) {
        case FILTERING:    return "filtering";
        case COMPARISON:   return "comparison";
        case NONFILTERING: return "nonfiltering";
        default:           return "invalid";
    }
}

inline SamplerType::type_t SamplerType::from_str(const std::string& str) {
    if (str == "filtering") return FILTERING;
    else if (str == "comparison") return COMPARISON;
    else if (str == "nonfiltering") return NONFILTERING;
    else return INVALID;
}

inline bool SamplerType::is_valid_str(const std::string& str) {
    return (str == "filtering")
        || (str == "comparison")
        || (str == "nonfiltering");
}

inline const char* SamplerType::valid_sampler_types_as_str() {
    return "filtering|comparison|nonfiltering";
}

} // namespace shdc