#pragma once
#include <vector>
#include <stdint.h>

namespace shdc {

// HLSL/Metal to bytecode compiler wrapper
struct BytecodeBlob {
    bool valid = false;
    int snippet_index = -1;
    std::vector<uint8_t> data;
};

} // namespace shdc
