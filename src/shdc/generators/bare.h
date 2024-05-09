#pragma once
#include "generator.h"

namespace shdc::gen {

class BareGenerator: public Generator {
public:
    virtual ErrMsg generate(const GenInput& gen);
protected:
    std::string mod_prefix;
    std::string shader_file_path(const GenInput& gen, const std::string& prog_name, const std::string& stage_name, Slang::Enum slang, bool is_blob);
private:
    ErrMsg gen_shader_sources_and_blobs(const GenInput& gen, Slang::Enum slang);
};

} // namespace
