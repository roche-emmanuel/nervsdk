#ifndef _NVK_UTILS_
#define _NVK_UTILS_

#include <nvk_common.h>

#include <nvk/base/RefObject.h>
#include <nvk/base/RefPtr.h>
#include <nvk/base/std_containers.h>

namespace nv {

// Returns current physical RAM usage in bytes, or 0 on failure.
auto get_current_rss() -> U64;

auto base64_encode(const U8* data, size_t len) -> String;
auto base64_encode(const Vector<U8>& data) -> String;
auto base64_decode(const String& encoded_string) -> Vector<U8>;

auto toHex(const U8Vector& data) -> String;
auto fromHex(const String& hex) -> U8Vector;

// Helper function to convert glob pattern to regex
auto glob_to_regex(const String& pattern) -> String;

// Helper function to expand file wildcards
auto expand_files_wildcard(const String& sourceDir, const String& pattern)
    -> Set<String>;

auto get_relative_path(const String& filepath, const String& parent) -> String;

// Sleep functions
void sleep_s(U32 secs);
void sleep_ms(U32 msecs);
void sleep_us(U32 usecs);

// Convert a string to upper characters:
auto to_upper(const String& str) -> String;

// Convert a string to lower characters:
auto to_lower(const String& str) -> String;

auto system_file_exists(const char* fname) -> bool;

inline auto system_file_exists(const String& fname) -> bool {
    return system_file_exists(fname.c_str());
}

auto system_dir_exists(const char* path) -> bool;

/** Check if a virtual file exists. */
auto virtual_file_exists(const String& fname, bool forceAllowSystem = false)
    -> bool;

auto get_file_last_write_time(const String& fname) -> std::time_t;

auto is_absolute_path(const String& path) -> bool;
auto get_absolute_path(const String& path) -> String;

auto read_system_file(const char* fname) -> String;
auto read_system_binary_file(const char* fname) -> U8Vector;

auto read_virtual_file(const String& fname, bool forceAllowSystem = false)
    -> String;

auto read_virtual_binary_file(const String& fname,
                              bool forceAllowSystem = false) -> U8Vector;

auto get_virtual_files(const String& directory, const std::regex& pattern,
                       bool recursive) -> Vector<String>;

void write_file(const char* fname, const String& content,
                bool createFolders = true);

void write_binary_file(const char* fname, const U8Vector& content,
                       bool createFolders = true);

void remove_file(const char* fname);

auto read_json_string(const String& content) -> Json;

auto read_json_file(const String& fname, bool forceAllowSystem = false) -> Json;

auto read_ordered_json_string(const String& content) -> OrderedJson;

auto read_ordered_json_file(const String& fname, bool forceAllowSystem = false)
    -> OrderedJson;

void write_json_file(const char* fname, const Json& content, I32 indent = 2);

auto read_yaml_string(const String& content) -> Json;

auto read_yaml_file(const String& fname, bool forceAllowSystem = false) -> Json;

auto read_config_file(const String& fname, bool forceAllowSystem = false)
    -> Json;

auto get_file_extension(const char* filename) -> String;

auto is_yaml_file(const char* filename) -> bool;
auto is_yaml_file(const String& filename) -> bool;

auto is_json_file(const char* filename) -> bool;
auto is_json_file(const String& filename) -> bool;

auto create_folders(const String& fullpath) -> bool;

auto get_path_extension(const String& fname) -> String;

auto get_parent_folder(const char* fname) -> String;

auto get_parent_folder(const String& fname) -> String;

auto get_filename(const String& full_path, bool withExt = true) -> String;

// Convert wide string to String:
auto toString(const std::wstring& wstr) -> String;

// Convert string to String:
auto toString(const std::string& wstr) -> String;

// Get the current working directory
auto get_cwd() -> String;

// Get the user home directory
auto get_home_dir() -> String;

auto copy_file(const char* source_path, const char* dest_path,
               U32 buffer_size = 64 * 1024, bool createFolders = true) -> bool;

auto copy_file(const String& source_path, const String& dest_path,
               U32 buffer_size = 64 * 1024, bool createFolders = true) -> bool;

auto get_files(const String& directory, bool recursive = false)
    -> Vector<String>;

auto get_files(const String& directory, const std::regex& pattern,
               bool recursive) -> Vector<String>;

auto make_extensions_regex(const Vector<String>& extensions) -> std::regex;

// Replace all instances of a given string inside another string:
void replace_all(String& str, const String& old_value, const String& new_value);

auto ecefToLLA(const Vec3d& xyz, F64 radius = MEAN_EARTH_RADIUS) -> Vec3d;

auto llaToECEF(const Vec3d& lla, F64 radius = MEAN_EARTH_RADIUS) -> Vec3d;

// Support to concatenate path elements:
template <typename T, typename V>
auto get_path(const T& arg0, const V& arg1) -> String {
    String p1(arg0);
    if (p1.back() == '/') {
        return p1 + String(arg1);
    }

    return p1 + "/" + String(arg1);
}

template <typename T, typename V, typename... Args>
auto get_path(const T& arg0, const V& arg1, const Args&... args) -> String {
    String p1(arg0);
    if (p1.back() == '/') {
        return get_path(p1 + String(arg1), args...);
    }

    return get_path(p1 + "/" + String(arg1), args...);
}

template <typename T> auto get_path(const T& arg) -> String {
    return String(arg);
};

inline auto get_path(String&& arg) -> String { return std::move(arg); };

template <typename... Args>
auto format_string(const String& format, Args... args) -> String {
    size_t size = snprintf(nullptr, 0, format.c_str(), args...) +
                  1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return String(buf.get(),
                  buf.get() + size - 1); // We don't want the '\0' inside
};

template <typename T>
auto remove_vector_element(Vector<T>& vec, const T& val) -> bool {
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if ((*it) == val) {
            vec.erase(it);
            return true;
        }
    }
    return false;
}

template <typename T>
auto remove_vector_element(Vector<RefPtr<T>>& vec, T* val) -> bool {
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if ((*it) == val) {
            vec.erase(it);
            return true;
        }
    }
    return false;
}

template <typename T>
auto vector_contains(const Vector<T>& vec, const T& val) -> bool {
    return std::find(vec.begin(), vec.end(), val) != vec.end();
}

#if !NV_USE_STD_MEMORY
template <typename T>
auto remove_vector_element(std::vector<RefPtr<T>>& vec, T* val) -> bool {
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if ((*it) == val) {
            vec.erase(it);
            return true;
        }
    }
    return false;
}
#endif

} // namespace nv

#endif
