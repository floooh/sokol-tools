#pragma once
#include <string>

namespace shdc {

// a vertex-/fragment-shader pair (@program)
struct Program {
    std::string name;
    std::string vs_name;    // name of vertex shader snippet
    std::string fs_name;    // name of fragment shader snippet
    std::string cs_name;    // name of compute shader snippet
    int line_index = -1;    // line index in input source (zero-based)

    Program();
    Program(const std::string& n, const std::string& vs, const std::string& fs, int l);
    Program(const std::string& n, const std::string& cs, int l);
};

inline Program::Program() { };

inline Program::Program(const std::string& n, const std::string& vs, const std::string& fs, int l):
    name(n),
    vs_name(vs),
    fs_name(fs),
    line_index(l)
{ };

inline Program::Program(const std::string& n, const std::string& cs, int l):
    name(n),
    cs_name(cs),
    line_index(l)
{ };

} // namespace shdc
