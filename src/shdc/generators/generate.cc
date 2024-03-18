/*
    Top-level wrapper for code-generators.
*/
#include "generate.h"
#include "types/format.h"
#include "bare.h"
#include "sokol.h"
#include "sokolnim.h"
#include "sokolodin.h"
#include "sokolrust.h"
#include "sokolzig.h"
#include "yaml.h"

namespace shdc::gen {

ErrMsg generate(Format::Enum format, const GenInput& gen_input) {
    switch (format) {
        case Format::BARE:
            return bare::generate(gen_input);
        case Format::BARE_YAML:
            return yaml::generate(gen_input);
        case Format::SOKOL_ZIG:
            return sokolzig::generate(gen_input);
        case Format::SOKOL_NIM:
            return sokolnim::generate(gen_input);
        case Format::SOKOL_ODIN:
            return sokolodin::generate(gen_input);
        case Format::SOKOL_RUST:
            return sokolrust::generate(gen_input);
        default:
            return sokol::generate(gen_input);
    }
}

} // namespace