#pragma once
#include <string>
#include <vector>

namespace shdc {

// a SPIRV-bytecode blob with "back-link" to Input.snippets and some limited reflection info
struct SpirvBlob {
    int snippet_index = -1;         // index into Input.snippets
    std::string source;             // source code this blob was compiled from
    std::vector<uint32_t> bytecode; // the resulting SPIRV blob
    int num_uniform_blocks = 0;
    int num_images = 0;
    int num_samplers = 0;
    int num_storage_buffers = 0;

    SpirvBlob(int snippet_index);
};

inline SpirvBlob::SpirvBlob(int snippet_index): snippet_index(snippet_index) { };

} // namespace shdc
