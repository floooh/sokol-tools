#pragma once
#include <string>
#include <vector>
#include "bind_slot_map.h"

namespace shdc {

// a SPIRV-bytecode blob with "back-link" to Input.snippets and some limited reflection info
struct SpirvBlob {
    int snippet_index = -1;         // index into Input.snippets
    std::string source;             // source code this blob was compiled from
    std::vector<uint32_t> bytecode; // the resulting SPIRV blob
    BindSlotMap bindings;           // extracted bindings

    SpirvBlob(int snippet_index);
};

inline SpirvBlob::SpirvBlob(int snippet_index): snippet_index(snippet_index) { };

} // namespace shdc
