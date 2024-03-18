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
#include <memory>

namespace shdc::gen {

std::unique_ptr<Generator> make_generator(Format::Enum format) {
    switch (format) {
        case Format::BARE:
            return std::make_unique<BareGenerator>();
        case Format::BARE_YAML:
            return std::make_unique<YamlGenerator>();
        case Format::SOKOL_ZIG:
            return std::make_unique<SokolZigGenerator>();
        case Format::SOKOL_NIM:
            return std::make_unique<SokolNimGenerator>();
        case Format::SOKOL_ODIN:
            return std::make_unique<SokolOdinGenerator>();
        case Format::SOKOL_RUST:
            return std::make_unique<SokolRustGenerator>();
        default:
            return std::make_unique<SokolGenerator>();
    }
}

ErrMsg generate(Format::Enum format, const GenInput& gen_input) {
    return make_generator(format)->generate(gen_input);
}

} // namespace