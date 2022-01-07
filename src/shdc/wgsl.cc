#include "tint.h"
#include <memory>
#include "shdc.h"

namespace shdc {
namespace wgsl {

void bla(const std::vector<uint32_t> spirv_blob) {
    std::unique_ptr<tint::Program> program;
    program = std::make_unique<tint::Program>(tint::reader::spirv::Parse(spirv_blob));
    tint::writer::wgsl::Options gen_options;
    auto result = tint::writer::wgsl::Generate(program.get(), gen_options);
    printf("%s\n", result.wgsl.c_str());
}

} // namespace sgtint
} // namespace shdc
