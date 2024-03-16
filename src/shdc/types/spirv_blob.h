#pragma once
#include <string>
#include <vector>

namespace shdc {

// a SPIRV-bytecode blob with "back-link" to input_t.snippets
struct SpirvBlob {
    int snippet_index = -1;         // index into input_t.snippets
    std::string source;             // source code this blob was compiled from
    std::vector<uint32_t> bytecode; // the resulting SPIRV blob

    SpirvBlob(int snippet_index);
};

inline SpirvBlob::SpirvBlob(int snippet_index): snippet_index(snippet_index) { };

} // namespace shdc
