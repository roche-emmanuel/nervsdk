#ifndef _NVK_UTILS_
#define _NVK_UTILS_

#include <nvk_common.h>

#include <nvk/base/RefObject.h>
#include <nvk/base/RefPtr.h>
#include <nvk/base/std_containers.h>

namespace nv {

auto system_file_exists(const char* fname) -> bool;
auto system_dir_exists(const char* path) -> bool;

// Replace all instances of a given string inside another string:
void replace_all(String& str, const String& old_value, const String& new_value);

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
