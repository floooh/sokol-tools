#pragma once
#include <string>

namespace shdc::refl {

struct VertexAttr {
    static const int NUM = 16;      // must be identical with NUM_VERTEX_ATTRS
    int slot = -1;
    std::string name;
    std::string sem_name;
    int sem_index = 0;

    bool equals(const VertexAttr& rhs) const;
};

inline bool VertexAttr::equals(const VertexAttr& rhs) const {
    return (slot == rhs.slot) &&
           (name == rhs.name) &&
           (sem_name == rhs.sem_name) &&
           (sem_index == rhs.sem_index);
}

} // namespace
