#pragma once
#include <string>
#include <vector>

namespace shdc::refl {

struct Type {
    enum BaseType {
        Invalid,
        Bool,
        Int,
        UInt,
        Float,
        Half,
        Struct,
    };
    std::string name;
    BaseType base_type = Invalid;
    int vecsize = 1;
    int columns = 1;
    int offset = 0;
    int size = 0;
    bool is_array = false;
    int array_count = 0;    // NOTE: is_array == true and array_count == 0 means 'unbounded array'
    int array_stride = 0;
    std::vector<Type> struct_items;

    bool equals(const Type& other) const;
};

inline bool Type::equals(const Type& other) const {
    if (name != other.name) {
        return false;
    }
    if (base_type != other.base_type) {
        return false;
    }
    if (vecsize != other.vecsize) {
        return false;
    }
    if (columns != other.columns) {
        return false;
    }
    if (offset != other.offset) {
        return false;
    }
    if (size != other.size) {
        return false;
    }
    if (is_array != other.is_array) {
        return false;
    }
    if (array_count != other.array_count) {
        return false;
    }
    if (array_stride != other.array_stride) {
        return false;
    }
    if (struct_items.size() != other.struct_items.size()) {
        return false;
    }
    for (size_t i = 0; i < struct_items.size(); i++) {
        if (!struct_items[i].equals(other.struct_items[i])) {
            return false;
        }
    }
    return true;
}

} // namespace
