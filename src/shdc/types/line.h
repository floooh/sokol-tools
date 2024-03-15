#pragma once
#include <string>

namespace shdc {

// mapping each line to included filename and line index
struct line_t {
    std::string line;       // line content
    int filename = 0;       // index into input_t filenames
    int index = 0;          // line index == line nr - 1

    line_t();
    line_t(const std::string& ln, int fn, int ix);
};

inline line_t::line_t() { };

inline line_t::line_t(const std::string& ln, int fn, int ix):
    line(ln),
    filename(fn),
    index(ix)
{ };

} // namespace shdc
