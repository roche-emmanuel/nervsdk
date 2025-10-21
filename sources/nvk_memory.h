#ifndef NV_CORE_MEMORY_
#define NV_CORE_MEMORY_

#if !NV_USE_STD_MEMORY
#include <nvk/base/memory/MemoryManager.h>
#endif

#include <nvk/base/RefPtr.h>

namespace nv {

#if NV_USE_STD_MEMORY
template <class T, class... Args> auto std_mem_create(Args&&... args) -> T* {
    return new T(std::forward<Args>(args)...);
}

template <typename T> inline void destroy_object(T* ptr) { delete ptr; }

template <typename T, class... Args>
auto create_ref_object(Args&&... args) -> RefPtr<T> {
    // cf.
    // https://stackoverflow.com/questions/2821223/how-would-one-call-stdforward-on-all-arguments-in-a-variadic-function
    return std_mem_create<T>(std::forward<Args>(args)...);
}

template <typename T, class... Args> auto create(Args&&... args) -> RefPtr<T> {
    // cf.
    // https://stackoverflow.com/questions/2821223/how-would-one-call-stdforward-on-all-arguments-in-a-variadic-function
    return std_mem_create<T>(std::forward<Args>(args)...);
}

template <typename T, class... Args> auto create_object(Args&&... args) -> T* {
    // cf.
    // https://stackoverflow.com/questions/2821223/how-would-one-call-stdforward-on-all-arguments-in-a-variadic-function
    return std_mem_create<T>(std::forward<Args>(args)...);
}

#else
template <typename T, class... Args>
auto create_ref_object(Args&&... args) -> RefPtr<T> {
    // cf.
    // https://stackoverflow.com/questions/2821223/how-would-one-call-stdforward-on-all-arguments-in-a-variadic-function
    return MemoryManager::get_root_allocator().create<T>(
        std::forward<Args>(args)...);
}

template <typename T, class... Args> auto create(Args&&... args) -> RefPtr<T> {
    // cf.
    // https://stackoverflow.com/questions/2821223/how-would-one-call-stdforward-on-all-arguments-in-a-variadic-function
    return MemoryManager::get_root_allocator().create<T>(
        std::forward<Args>(args)...);
}

template <typename T, class... Args> auto create_object(Args&&... args) -> T* {
    // cf.
    // https://stackoverflow.com/questions/2821223/how-would-one-call-stdforward-on-all-arguments-in-a-variadic-function
    return MemoryManager::get_root_allocator().create_ptr<T>(
        std::forward<Args>(args)...);
}

inline void destroy_object(void* ptr) {
    MemoryManager::get_root_allocator().free(ptr);
}
#endif

} // namespace nv

#endif
