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
    static Program from_vs_fs(const std::string& name, const std::string& vs_name, const std::string& fs_name, int line_index);
    static Program from_cs(const std::string& name, const std::string& cs_name, int line_index);
    bool has_vs() const;
    bool has_fs() const;
    bool has_cs() const;
    bool has_vs_fs() const;
};

inline Program::Program() { }

inline Program Program::from_vs_fs(const std::string& name, const std::string& vs_name, const std::string& fs_name, int line_index) {
    Program p;
    p.name = name;
    p.vs_name = vs_name;
    p.fs_name = fs_name;
    p.line_index = line_index;
    return p;
}

inline Program Program::from_cs(const std::string& name, const std::string& cs_name, int line_index) {
    Program p;
    p.name = name;
    p.cs_name = cs_name;
    p.line_index = line_index;
    return p;
}

inline bool Program::has_vs() const {
    return vs_name.size() > 0;
}

inline bool Program::has_fs() const {
    return fs_name.size() > 0;
}

inline bool Program::has_cs() const {
    return cs_name.size() > 0;
}

inline bool Program::has_vs_fs() const {
    return has_vs() && has_fs();
}
} // namespace shdc
