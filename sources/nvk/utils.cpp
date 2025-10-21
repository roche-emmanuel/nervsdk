
#include <nvk/utils.h>

namespace nv {

auto system_dir_exists(const char* path) -> bool {
    // Check if path exists and is a directory
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

auto system_file_exists(const char* fname) -> bool {
    // cf.
    // http://stackoverflow.com/questions/268023/what-s-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
    return std::filesystem::exists(fname) &&
           std::filesystem::is_regular_file(fname);
}

void replace_all(String& str, const String& old_value,
                 const String& new_value) {
    size_t pos = 0;
    while ((pos = str.find(old_value, pos)) != std::string::npos) {
        str.replace(pos, old_value.length(), new_value);
        pos += new_value.length();
    }
}

} // namespace nv
