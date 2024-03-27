#pragma once
#include "generator.h"

namespace shdc::gen {

class BareGenerator: public Generator {
public:
    virtual ErrMsg generate(const GenInput& gen);
private:
    ErrMsg gen_shader_sources_and_blobs(const GenInput& gen, Slang::Enum slang);
};

} // namespace
