#ifndef _NVK_UTILS_
#define _NVK_UTILS_

#include <nvk_common.h>

#include <nvk/base/RefObject.h>
#include <nvk/base/RefPtr.h>
#include <nvk/base/std_containers.h>

namespace nv {

// Helper function to convert glob pattern to regex
auto glob_to_regex(const String& pattern) -> String;

// Helper function to expand file wildcards
auto expand_files_wildcard(const std::string& pattern) -> Set<String>;

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

auto is_absolute_path(const String& path) -> bool;
auto get_absolute_path(const String& path) -> String;

auto read_system_file(const char* fname) -> String;
auto read_system_binary_file(const char* fname) -> U8Vector;

auto read_virtual_file(const String& fname, bool forceAllowSystem = false)
    -> String;

auto read_json_string(const String& content) -> Json;

auto read_json_file(const String& fname, bool forceAllowSystem = false) -> Json;

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

// Replace all instances of a given string inside another string:
void replace_all(String& str, const String& old_value, const String& new_value);

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
