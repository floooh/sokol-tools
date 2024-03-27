#pragma once
#include "bare.h"

namespace shdc::gen {

class YamlGenerator: public BareGenerator {
public:
    virtual ErrMsg generate(const GenInput& gen);
};

} // namespace
