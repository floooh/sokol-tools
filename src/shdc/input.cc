/*
    code for loading and parsing the input .glsl file with custom-tags
*/
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include "fmt/format.h"
#include "pystring.h"

namespace shdc {

static std::string load_file_into_str(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        return std::string();
    }
    fseek(f, 0, SEEK_END);
    const int file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*) malloc(file_size + 1);
    fread((void*)buf, file_size, 1, f);
    buf[file_size] = 0;
    std::string str(buf);
    free((void*)buf);
    return str;
}

/* removes comments from string
    - FIXME: doesn't detect block-comment in block-comment bugs
    - also removes comments in string literals (no problem for shader langs)
*/
static bool remove_comments(std::string& str) {
    bool in_winged_comment = false;
    bool in_block_comment = false;
    bool maybe_start = false;
    bool maybe_end = false;
    const int len = str.length();
    for (int pos = 0; pos < len; pos++) {
        const char c = str[pos];
        if (!(in_winged_comment || in_block_comment)) {
            // not currently in a comment
            if (maybe_start) {
                if (c == '/') {
                    // start of a winged comment
                    in_winged_comment = true;
                    str[pos-1] = ' ';
                    str[pos] = ' ';
                }
                else if (c == '*') {
                    // start of a block comment
                    in_block_comment = true;
                    str[pos - 1] = ' ';
                    str[pos] = ' ';
                }
                maybe_start = false;
            }
            else {
                if (c == '/') {
                    // maybe start of a new comment
                    maybe_start = true;
                }
            }
        }
        else if (in_winged_comment || in_block_comment) {
            if (in_winged_comment) {
                if ((c == '\r') || (c == '\n')) {
                    // end of line reached
                    in_winged_comment = false;
                }
                else {
                    str[pos] = ' ';
                }
            }
            else {
                // in block comment (preserve newlines)
                if ((c != '\r') && (c != '\n')) {
                    str[pos] = ' ';
                }
                if (maybe_end) {
                    if (c == '/') {
                        // end of block comment
                        in_block_comment = false;
                    }
                    maybe_end = false;
                }
                else {
                    if (c == '*') {
                        // potential end of block comment
                        maybe_end = true;
                    }
                }
            }
        }
    }
    return true;
}

/* This parses the splitted input line array for custom tags (@vs, @fs, @block,
    @end and @program), and fills the respective members. If a parsing error
    happens, the inp.error object is setup accordingly.
*/
static bool parse(input_t& inp) {
    bool in_snippet = false;
    snippet_t cur_snippet;
    const std::string vs_tag = "@vs";
    const std::string fs_tag = "@fs";
    const std::string block_tag = "@block";
    const std::string end_tag = "@end";
    const std::string prog_tag = "@program";
    std::vector<std::string> tokens;
    int line_index = 0;
    for (const std::string& line : inp.lines) {
        if (pystring::find(line, block_tag) >= 0) {
            pystring::split(line, tokens);
            if (tokens[0] != block_tag) {
                inp.error = error_t(inp.path, line_index, "@block tag must be first word in line.");
                return false;   
            }
            if (tokens.size() != 2) {
                inp.error = error_t(inp.path, line_index, "@block tag must have exactly one arg (@block name).");
                return false;
            }
            if (in_snippet) {
                inp.error = error_t(inp.path, line_index, "@block tag cannot be inside other tag block (missing @end?).");
                return false;
            }
            if (inp.blocks.count(tokens[1]) > 0) {
                inp.error = error_t(inp.path, line_index, fmt::format("@block '{}' already exists.", tokens[1]));
                return false;
            }
            cur_snippet.start(snippet_t::BLOCK, tokens[1], line_index);
            in_snippet = true;
        }
        else if (pystring::find(line, vs_tag) >= 0) {
            pystring::split(line, tokens);
            if (tokens[0] != vs_tag) {
                inp.error = error_t(inp.path, line_index, "@vs tag must be first word in line.");
                return false;
            }
            if (tokens.size() != 2) {
                inp.error = error_t(inp.path, line_index, "@vs tag must have exactly one arg (@vs name).");
                return false;
            }
            if (in_snippet) {
                inp.error = error_t(inp.path, line_index, "@vs tag cannot be inside other tag block (missing @end?).");
                return false;
            }
            if (inp.vs.count(tokens[1]) > 0) {
                inp.error = error_t(inp.path, line_index, fmt::format("@vs '{}' already exists.", tokens[1]));
                return false;
            }
            cur_snippet.start(snippet_t::VS, tokens[1], line_index);
            in_snippet = true;
        }
        else if (pystring::find(line, fs_tag) >= 0) {
            pystring::split(line, tokens);
            if (tokens[0] != fs_tag) {
                inp.error = error_t(inp.path, line_index, "@fs tag must be first word in line.");
                return false;
            }
            if (tokens.size() != 2) {
                inp.error = error_t(inp.path, line_index, "@fs tag must have exactly one arg (@fs name).");
                return false;
            }
            if (in_snippet) {
                inp.error = error_t(inp.path, line_index, "@fs tag cannot be inside other tag block (missing @end?).");
                return false;
            }
            if (inp.vs.count(tokens[1]) > 0) {
                inp.error = error_t(inp.path, line_index, fmt::format("@fs '{}' already exists.", tokens[1]));
                return false;
            }
            cur_snippet.start(snippet_t::FS, tokens[1], line_index);
            in_snippet = true;
        }
        else if (pystring::find(line, end_tag) >= 0) {
            pystring::split(line, tokens);
            if (tokens.size() != 1) {
                inp.error = error_t(inp.path, line_index, "@end tag must be the only word in a line.");
                return false;
            }
            if (!in_snippet) {
                inp.error = error_t(inp.path, line_index, "@end tag must come after a @block, @vs or @fs tag.");
                return false;
            }
            cur_snippet.end(line_index);
            in_snippet = false;
            switch (cur_snippet.type) {
                case snippet_t::BLOCK:
                    inp.blocks[cur_snippet.name] = inp.snippets.size();
                    break;
                case snippet_t::VS:
                    inp.vs[cur_snippet.name] = inp.snippets.size();
                    break;
                case snippet_t::FS:
                    inp.fs[cur_snippet.name] = inp.snippets.size();
                    break;
                default: break;
            }
            inp.snippets.push_back(std::move(cur_snippet));
        }
        else if (pystring::find(line, prog_tag) >= 0) {
            pystring::split(line, tokens);
            if (tokens[0] != prog_tag) {
                inp.error = error_t(inp.path, line_index, "@program tag must be the first word in line.");
                return false;
            }
            if (tokens.size() != 4) {
                inp.error = error_t(inp.path, line_index, "@program tag must have exactly 3 args (@program name vs_name fs_name).");
                return false;
            }
            if (in_snippet) {
                inp.error = error_t(inp.path, line_index, "@program tag cannot be inside a block tag.");
                return false;
            }
            if (inp.programs.count(tokens[1]) > 0) {
                inp.error = error_t(inp.path, line_index, fmt::format("@program '{}' already exists.", tokens[1]));
                return false;
            }
            if (inp.vs.count(tokens[2]) != 1) {
                inp.error = error_t(inp.path, line_index, fmt::format("@vs '{}' not found.", tokens[2]));
                return false;
            }
            if (inp.fs.count(tokens[3]) != 1) {
                inp.error = error_t(inp.path, line_index, fmt::format("@fs '{}' not found.", tokens[3]));
                return false;
            }
            inp.programs[tokens[1]] = program_t(tokens[1], tokens[2], tokens[3], line_index);
        }
        line_index++;
    }
    if (in_snippet) {
        inp.error = error_t(inp.path, line_index, "final @end missing.");
        return false;
    }
    return true;
}

/* load file and parse into an input_t object, check valid and error
    fields in returned object
*/
input_t input_t::load(const std::string& path) {
    input_t inp;
    inp.path = path;

    std::string str = load_file_into_str(path);
    if (str.empty()) {
        inp.error = error_t(fmt::format("Failed to open input file '{}'", path));
        return inp;
    }
    
    // remove comments before splitting into lines
    if (!remove_comments(str)) {
        inp.error = error_t(fmt::format("(FIXME) Error during removing comments in '{}'", path));
    }

    // split source file into lines and parse tags */
    pystring::splitlines(str, inp.lines);
    if (!parse(inp)) {
        return inp;
    }

    return inp;
}

/* print a debug-dump of content to stderr */
void input_t::dump() {
    fmt::print(stderr, "input_t:\n");
    if (error.valid) {
        fmt::print(stderr, "  error: {}:{}:{}\n", error.file, error.line_index, error.msg);
    }
    else {
        fmt::print(stderr, "  error: not set\n");
    }
    int line_nr = 1;
    fmt::print(stderr, "  lines:\n");
    for (const std::string& line : lines) {
        fmt::print(stderr, "    {}: {}\n", line_nr++, line);
    }
    int snippet_nr = 0;
    fmt::print(stderr, "  snippets:\n");
    for (const snippet_t& snippet : snippets) {
        fmt::print(stderr, "    snippet {}:\n", snippet_nr++);
        fmt::print(stderr, "      name: {}\n", snippet.name);
        fmt::print(stderr, "      type: {}\n", snippet_t::type_to_str(snippet.type));
        fmt::print(stderr, "      line_index: {}\n", snippet.line_index);
        fmt::print(stderr, "      num_lines: {}\n", snippet.num_lines);
    }
    fmt::print(stderr, "  blocks:\n");
    for (const auto& item : blocks) {
        fmt::print(stderr, "    block {} => snippet {}\n", item.first, item.second);
    }
    fmt::print(stderr, "  vertex shaders:\n");
    for (const auto& item : vs) {
        fmt::print(stderr, "    vs {} => snippet {}\n", item.first, item.second);
    }
    fmt::print(stderr, "  fragment shaders:\n");
    for (const auto& item : fs) {
        fmt::print(stderr, "    fs {} => snippet {}\n", item.first, item.second);
    }
    fmt::print(stderr, "  programs:\n");
    for (const auto& item : programs) {
        const std::string& key = item.first;
        const program_t& prog = item.second;
        fmt::print(stderr, "    program {}:\n", key);
        fmt::print(stderr, "      name: {}\n", prog.name);
        fmt::print(stderr, "      vs: {}\n", prog.vs_name);
        fmt::print(stderr, "      fs: {}\n", prog.fs_name);
        fmt::print(stderr, "      line_index: {}\n", prog.line_index);
    }
}

} // namespace shdc
