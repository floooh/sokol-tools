#pragma once
#include <string>

namespace shdc::refl {

// image-sample-type (see sg_image_sample_type)
struct ImageSampleType {
    enum Enum {
        INVALID,
        FLOAT,
        SINT,
        UINT,
        DEPTH,
        UNFILTERABLE_FLOAT,
    };

    static const char* to_str(Enum e);
    static ImageSampleType::Enum from_str(const std::string& str);
    static bool is_valid_str(const std::string& str);
    static const char* valid_image_sample_types_as_str();
};

inline const char* ImageSampleType::to_str(Enum e) {
    switch (e) {
        case FLOAT:  return "float";
        case SINT:   return "sint";
        case UINT:   return "uint";
        case DEPTH:  return "depth";
        case UNFILTERABLE_FLOAT: return "unfilterable_float";
        default:     return "invalid";
    }
}

inline ImageSampleType::Enum ImageSampleType::from_str(const std::string& str) {
    if (str == "float") return FLOAT;
    else if (str == "sint") return SINT;
    else if (str == "uint") return UINT;
    else if (str == "depth") return DEPTH;
    else if (str == "unfilterable_float") return UNFILTERABLE_FLOAT;
    else return INVALID;
}

inline bool ImageSampleType::is_valid_str(const std::string& str) {
    return (str == "float")
        || (str == "sint")
        || (str == "uint")
        || (str == "depth")
        || (str == "unfilterable_float");
}

inline const char* ImageSampleType::valid_image_sample_types_as_str() {
    return "float|sint|uint|depth|unfilterable_float";
}

} // namespace
