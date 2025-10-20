// Implementation for RefObject

#if !NV_USE_STD_MEMORY
#include <nvk/base/memory/Allocator.h>
#endif

#include <nvk/base/RefObject.h>
#include <nvk/base/SpinLock.h>
#include <nvk/base/string_id.h>

namespace nv {

#if NV_CHECK_MEMORY_LEAKS
/** Debug section to check for memory leaks. */
using PointerMap = std::unordered_map<RefObject*, std::atomic<I64>>;
static auto get_object_refs() -> PointerMap& {
    static PointerMap sObjectRefs;
    return sObjectRefs;
}

static SpinLock refSP;

static auto get_object_refs(RefObject* ptr) -> std::atomic<I64>& {
    auto& refs = get_object_refs();
    WITH_SPINLOCK(refSP);
    auto it = refs.find(ptr);
    if (it == refs.end()) {
        refs[ptr] = 0;
        return refs[ptr];
    }

    return it->second;
}

static void remove_object_refs(RefObject* ptr) {
    auto& refs = get_object_refs();
    WITH_SPINLOCK(refSP);

    auto it = refs.find(ptr);
    if (it == refs.end()) {
        std::cout << "MEMWARN: Entry not found in refs during deletion of '"
                  << ptr->get_class_name()
                  << "' object:"
                     "assuming on stack."
                  << std::endl;
        // std::cout << "Stacktrace: " << getStackTrace() << std::endl;
    } else {
        I64 count = it->second.load(std::memory_order_acquire);
        if (count != 0) {
            std::cout << "MEMFATAL: Invalid object deletion of '"
                      << ptr->get_class_name() << "': invalid count: " << count
                      << std::endl;
            // std::cout << "Stacktrace: " << getStackTrace() << std::endl;
        }
        refs.erase(it);
    }
}

#endif

/** allocated count. */
static auto get_allocated_count() -> std::atomic<I64>& {
    static std::atomic<I64> allocatedCount{0};
    return allocatedCount;
}

RefObject::RefObject() : _refCount(0) {
#if NV_CHECK_MEMORY_LEAKS
    auto& count = get_object_refs(this);

    if (count.load(std::memory_order_acquire) != 0) {
        std::cout << "MEMFATAL: Reallocating in-use address"
                  << (const void*)this << std::endl;
        // std::cout << "Stacktrace: " << getStackTrace() << std::endl;
    }

    // std::cout << "Creating RefObject " << (const void*)this << std::endl;

    get_allocated_count().fetch_add(1);
    // std::cout << "Allocated count: " << get_allocated_count() << std::endl;
#endif
};

RefObject::~RefObject() {
#if NV_CHECK_MEMORY_LEAKS
    remove_object_refs(this);

    // std::cout << "Releasing RefObject " << (const void*)this << std::endl;
    if (get_allocated_count().load(std::memory_order_acquire) == 0) {
        std::cout << "MEMFATAL: allocated count is already at zero!!!"
                  << std::endl;
        // std::cout << "Stacktrace: " << getStackTrace() << std::endl;
        return;
    }
    get_allocated_count().fetch_sub(1);
    // std::cout << "Allocated count: " << get_allocated_count() << std::endl;
#endif
};

void RefObject::check_memory_refs() {

#if NV_CHECK_MEMORY_LEAKS
    if (get_allocated_count().load(std::memory_order_acquire) > 0) {
        std::cout << "[ERROR] Detected " << get_allocated_count()
                  << " remaining allocated objects." << std::endl;
    }

    std::cout << "Looking for memory leaks..." << std::endl;
    auto& refs = get_object_refs();
    WITH_SPINLOCK(refSP);

    if (!refs.empty()) {
        std::cout << "[FATAL] Found " << refs.size() << " memory leaks!!!"
                  << std::endl;
        // Force deleting those object:
        for (auto& obj : refs) {
            std::cout << "[FATAL] Leaking object " << (const void*)(obj.first)
                      << " with count: " << obj.second << std::endl;
            // std::cout << "[FATAL]Object desc: "<< obj.first->toString() <<
            // std::endl; delete obj.first;
        }

        refs.clear();
    } else {
        std::cout << "No memory leak detected." << std::endl;
    }
#endif
}

void RefObject::ref() {
    _refCount.fetch_add(1, std::memory_order_release);
#if NV_CHECK_MEMORY_LEAKS
    get_object_refs(this).fetch_add(1, std::memory_order_release);
    ;
#endif
}

void RefObject::unref() {
#if NV_CHECK_MEMORY_LEAKS
    get_object_refs(this).fetch_add(-1, std::memory_order_release);
#endif

    if (_refCount.fetch_add(-1, std::memory_order_acq_rel) == 1) {
        // std::cout << "Deleting RefObject " << toString() << std::endl;
        // if we have an allocator, then we should use it
        // to free the memory we are currently using:
#if NV_USE_STD_MEMORY
        delete this;
#else
        if (_allocator != nullptr) {
            // We manually call the destructor:
            this->~RefObject();
            // then we free the memory:
            _allocator->free(this);
        } else {
            // otherwise we can still delete normaly:
            delete this;
        }
#endif
    }
}

void RefObject::unref_nodelete() {
#if NV_CHECK_MEMORY_LEAKS
    get_object_refs(this).fetch_add(-1, std::memory_order_release);
#endif

    _refCount.fetch_add(-1, std::memory_order_release);
}

#if !NV_USE_STD_MEMORY
void RefObject::set_allocator(Allocator* alloc) {
    // We should not have an allocator yet:
    NVCHK(_allocator == nullptr, "Allocator was already assigned.");

    // The newly provided allocator should always be valid:
    NVCHK(alloc != nullptr, "Invalid allocator.");

    // Assign the provided allocator:
    _allocator = alloc;
}
#endif

auto RefObject::cast(StringID tid) -> RefObject* {
    switch (tid) {
    case SID("RefObject"):
    case SID("nv::RefObject"):
        return this;
    default:
        return nullptr;
    }
}

auto RefObject::get_class_id() const -> StringID { return SID("RefObject"); }

} // namespace nv
