/*
    Error message creation and conversion into IDE-compatible format.
*/
#include "shdc.h"
#include "fmt/format.h"

namespace shdc {

void error_t::print(msg_format_t fmt) const {
    if (fmt == VSTUDIO) {
        fmt::print("{}({}): error: {}\n", file, line_index+1, msg);
    }
    else {
        fmt::print("{}:{}:0: error: {}\n", file, line_index+1, msg);
    }
}

} // namespace shadc

