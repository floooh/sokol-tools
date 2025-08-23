#include "util.h"

namespace shdc::util {

ErrMsg write_dep_file(const Args& args, const Input& inp) {
    std::string content;
    content.append(fmt::format("{}: {}", args.output, inp.filenames[0]));
    for (size_t i = 1; i < inp.filenames.size(); i++) {
        const std::string& fn = inp.filenames[i];
        content.append(fmt::format(" \\\n  {}", fn));
    }
    content.append("\n");
    FILE* f = fopen(args.dependency_file.c_str(), "w");
    if (!f) {
        return ErrMsg::error(inp.base_path, 0, fmt::format("failed to open dependency output file '{}'", args.dependency_file));
    }
    fwrite(content.c_str(), content.length(), 1, f);
    fclose(f);
    return ErrMsg();
}

// this returns the first line index of a snippet which actually belong to the snippet,
// skipping any included blocks - used for error messages which should be positioned
// at the start of a snippet (if the snippet started with an @include_block that first
// line would point in the @block instead)
int first_snippet_line_index_skipping_include_blocks(const Input& inp, const Snippet& snippet) {
    for (int i: snippet.lines) {
        assert(i < (int)inp.lines.size());
        if ((inp.lines[i].snippet != -1) && (inp.lines[i].snippet == snippet.index)) {
            return i;
        }
    }
    // hmm, this shouldn't actually happen
    return snippet.lines[0];
}

} // namespace shdc::util
