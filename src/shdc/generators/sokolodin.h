#pragma once
#include "generator.h"

namespace shdc::gen {

class SokolOdinGenerator: public Generator {
public:
    virtual ErrMsg generate(const GenInput& gen);
};

} // namespace
