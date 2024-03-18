#pragma once
#include "generator.h"

namespace shdc::gen {

class SokolRustGenerator: public Generator {
public:
    virtual ErrMsg generate(const GenInput& gen);
};

} // namespace
