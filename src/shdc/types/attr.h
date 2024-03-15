#pragma once
#include <string>

namespace shdc {

struct attr_t {
    static const int NUM = 16;      // must be identical with NUM_VERTEX_ATTRS
    int slot = -1;
    std::string name;
    std::string sem_name;
    int sem_index = 0;

    bool equals(const attr_t& rhs) const;
};

inline bool attr_t::equals(const attr_t& rhs) const {
    return (slot == rhs.slot) &&
           (name == rhs.name) &&
           (sem_name == rhs.sem_name) &&
           (sem_index == rhs.sem_index);
}

} // namespace shdc
