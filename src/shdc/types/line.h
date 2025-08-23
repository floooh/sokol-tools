#pragma once
#include <string>

namespace shdc {

// mapping each line to included filename and line index
struct Line {
    std::string line;       // line content
    int filename = 0;       // index into Input filenames
    int snippet = -1;       // snippet index to which this line belongs (-1 => none)
    int index = 0;          // line index == line nr - 1

    Line();
    Line(const std::string& ln, int fn, int ix);
};

inline Line::Line() { };

inline Line::Line(const std::string& ln, int fn, int ix):
    line(ln),
    filename(fn),
    index(ix)
{ };

} // namespace shdc
