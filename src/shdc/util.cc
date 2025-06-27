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

} // namespace shdc::util
