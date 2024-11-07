#pragma once
#include <string>
#include "type.h"

namespace shdc::refl {

struct StageAttr {
    static const int Num = 16;
    int slot = -1;
    std::string name;
    std::string sem_name;
    int sem_index = 0;
    Type type_info;

    bool equals(const StageAttr& rhs) const;
    void dump_debug(const std::string& indent) const;
};

inline bool StageAttr::equals(const StageAttr& rhs) const {
    return (slot == rhs.slot) &&
           (name == rhs.name) &&
           (sem_name == rhs.sem_name) &&
           (sem_index == rhs.sem_index);
}

inline void StageAttr::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}slot: {}\n", indent2, slot);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}sem_name: {}\n", indent2, sem_name);
    fmt::print(stderr, "{}sem_index: {}\n", indent2, sem_index);
}

} // namespace
