#pragma once
#include <string>
#include <vector>
#include <map>
#include "types/errmsg.h"
#include "types/line.h"
#include "types/snippet.h"
#include "types/program.h"

namespace shdc {

// pre-parsed GLSL source file, with content split into snippets
struct input_t {
    ErrMsg out_error;
    std::string base_path;              // path to base file
    std::string module;                 // optional module name
    std::vector<std::string> filenames; // all source files, base is first entry
    std::vector<Line> lines;          // input source files split into lines
    std::vector<snippet_t> snippets;    // @block, @vs and @fs snippets
    std::map<std::string, std::string> ctype_map;    // @ctype uniform type definitions
    std::vector<std::string> headers;       // @header statements
    std::map<std::string, int> snippet_map; // name-index mapping for all code snippets
    std::map<std::string, int> block_map;   // name-index mapping for @block snippets
    std::map<std::string, int> vs_map;      // name-index mapping for @vs snippets
    std::map<std::string, int> fs_map;      // name-index mapping for @fs snippets
    std::map<std::string, Program> programs;    // all @program definitions

    static input_t load_and_parse(const std::string& path, const std::string& module_override);
    ErrMsg error(int index, const std::string& msg) const;
    ErrMsg warning(int index, const std::string& msg) const;
    void dump_debug(ErrMsg::msg_format_t err_fmt) const;
};

} // namespace shdc
