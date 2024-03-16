#pragma once
#include <string>
#include <vector>

namespace shdc {

// a SPIRV-bytecode blob with "back-link" to input_t.snippets
struct spirv_blob_t {
    int snippet_index = -1;         // index into input_t.snippets
    std::string source;             // source code this blob was compiled from
    std::vector<uint32_t> bytecode; // the resulting SPIRV blob

    spirv_blob_t(int snippet_index);
};

inline spirv_blob_t::spirv_blob_t(int snippet_index): snippet_index(snippet_index) { };

} // namespace shdc
