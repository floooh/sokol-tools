#pragma once
#include <string>

namespace shdc::refl {

struct StageAttr {
    static const int Num = 16;
    int slot = -1;
    std::string name;
    std::string sem_name;
    int sem_index = 0;

    bool equals(const StageAttr& rhs) const;
};

inline bool StageAttr::equals(const StageAttr& rhs) const {
    return (slot == rhs.slot) &&
           (name == rhs.name) &&
           (sem_name == rhs.sem_name) &&
           (sem_index == rhs.sem_index);
}

} // namespace
