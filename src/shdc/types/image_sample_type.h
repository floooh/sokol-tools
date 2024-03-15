#pragma once
#include <string>

namespace shdc {

// image-sample-type (see sg_image_sample_type)
struct image_sample_type_t {
    enum type_t {
        INVALID,
        FLOAT,
        SINT,
        UINT,
        DEPTH,
        UNFILTERABLE_FLOAT,
    };

    static const char* to_str(type_t t);
    static image_sample_type_t::type_t from_str(const std::string& str);
    static bool is_valid_str(const std::string& str);
    static const char* valid_image_sample_types_as_str();
};

inline const char* image_sample_type_t::to_str(type_t t) {
    switch (t) {
        case FLOAT:  return "float";
        case SINT:   return "sint";
        case UINT:   return "uint";
        case DEPTH:  return "depth";
        case UNFILTERABLE_FLOAT: return "unfilterable_float";
        default:     return "invalid";
    }
}

inline image_sample_type_t::type_t image_sample_type_t::from_str(const std::string& str) {
    if (str == "float") return FLOAT;
    else if (str == "sint") return SINT;
    else if (str == "uint") return UINT;
    else if (str == "depth") return DEPTH;
    else if (str == "unfilterable_float") return UNFILTERABLE_FLOAT;
    else return INVALID;
}

inline bool image_sample_type_t::is_valid_str(const std::string& str) {
    return (str == "float")
        || (str == "sint")
        || (str == "uint")
        || (str == "depth")
        || (str == "unfilterable_float");
}

inline const char* image_sample_type_t::valid_image_sample_types_as_str() {
    return "float|sint|uint|depth|unfilterable_float";
}

} // namespace shdc
