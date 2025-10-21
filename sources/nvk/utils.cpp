
#include <nvk/utils.h>

#define BUFSIZE 4096

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

auto is_absolute_path(const String& path) -> bool {
    return std::filesystem::path(path).is_absolute();
}

auto get_absolute_path(const String& path) -> String {
#ifdef _WIN32
    DWORD retval = 0;
    std::array<char, BUFSIZE> buffer{0};
    char** lppPart = {nullptr};

    retval = GetFullPathName(path.c_str(), BUFSIZE, buffer.data(), lppPart);

    NVCHK(retval != 0, "Cannot build fullpath variable.");

    String result = buffer.data();
    return result;
#else
    char resolved_path[PATH_MAX];
    realpath(path.c_str(), resolved_path);
    return String(resolved_path);
#endif
}

// Read file content as string:
auto read_system_file(const char* fname) -> String {
    std::ifstream t(fname, std::ios::in | std::ios::binary);
    String res;
    NVCHK(t.is_open(), "File {} doesn't exist.", fname);

    t.seekg(0, std::ios::end);
    auto size = t.tellg();
    // res.reserve(size); // Should not use reserve but resize instead here.
    res.resize(size);
    t.seekg(0, std::ios::beg);

    // logDEBUG("Reading file "<<fname<<", size: "<<size);
    t.read(res.data(), size);
    return res;
}

auto read_system_binary_file(const char* fname) -> U8Vector {
    std::ifstream t(fname, std::ios::in | std::ios::binary);
    U8Vector res;
    NVCHK(t.is_open(), "File {} doesn't exist.", fname);

    t.seekg(0, std::ios::end);
    auto size = t.tellg();
    res.resize(size);
    t.seekg(0, std::ios::beg);

    // logDEBUG("Reading file "<<fname<<", size: "<<size);
    t.read(reinterpret_cast<char*>(res.data()), size);
    return res;
}

} // namespace nv
