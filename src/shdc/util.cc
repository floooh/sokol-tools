#include "util.h"
#include "pystring.h"

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

// convert a glslang info-log string to ErrMsg's and append to out_errors
void infolog_to_errors(const std::string& log, const Input& inp, int snippet_index, int linenr_offset, std::vector<ErrMsg>& out_errors) {
    /*
        format for errors is "[ERROR|WARNING]: [pos=0?]:[line]: message"
        And a last line we need to ignore: "ERROR: N compilation errors. ..."
    */
    const Snippet& snippet = inp.snippets[snippet_index];

    std::vector<std::string> lines;
    pystring::splitlines(log, lines);
    std::vector<std::string> tokens;
    static const std::string colon = ":";
    for (const std::string& src_line: lines) {
        // ignore the "compilation terminated" message, nothing useful in there
        if (pystring::find(src_line, ": compilation terminated") >= 0) {
            continue;
        }
        // special hack for absolute Windows paths starting with a device letter
        std::string line = pystring::replace(src_line, ":/", "\01\02", 1);
        line = pystring::replace(line, ":\\", "\03\04", 1);

        // split by colons
        pystring::split(line, tokens, colon);
        if ((tokens.size() > 2) && ((tokens[0] == "ERROR") || (tokens[0] == "WARNING"))) {
            bool ok = false;
            int line_index = 0;
            std::string msg;
            if (tokens.size() >= 4) {
                // extract line index and message
                int snippet_line_index = atoi(tokens[2].c_str());
                // correct for 1-based and additional #defines that had been injected in front of actual snippet source
                snippet_line_index -= (linenr_offset + 1);
                if (snippet_line_index < 0) {
                    snippet_line_index = 0;
                }
                // everything after the 3rd colon is 'msg'
                for (int i = 3; i < (int)tokens.size(); i++) {
                    if (msg.empty()) {
                        msg = tokens[i];
                    } else {
                        msg = fmt::format("{}:{}", msg, tokens[i]);
                    }
                }
                msg = pystring::strip(msg);
                // NOTE: The absolute file path isn't part of the message, so this replacement
                // seems redundant. It only kicks in if a :/ or :\\ was actually part of the
                // error message (highly unlikely)
                msg = pystring::replace(msg, "\01\02", ":/", 1);
                msg = pystring::replace(msg, "\03\04", ":\\", 1);
                // snippet-line-index to input source line index
                if ((snippet_line_index >= 0) && (snippet_line_index < (int)snippet.lines.size())) {
                    line_index = snippet.lines[snippet_line_index];
                    ok = true;
                }
            }
            if (ok) {
                if (tokens[0] == "ERROR") {
                    out_errors.push_back(inp.error(line_index, msg));
                } else {
                    out_errors.push_back(inp.warning(line_index, msg));
                }
            } else {
                // some error during parsing, still create an error object so the error isn't lost in the void
                out_errors.push_back(inp.error(0, line));
            }
        }
    }
}

} // namespace shdc::util
