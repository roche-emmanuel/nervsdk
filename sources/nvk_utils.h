#ifndef _NVK_UTILS_
#define _NVK_UTILS_

#include <nvk_common.h>

#include <base/RefObject.h>
#include <base/RefPtr.h>
#include <base/std_containers.h>

namespace nv {

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
