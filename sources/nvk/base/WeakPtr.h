#ifndef NV_WEAKPTR_
#define NV_WEAKPTR_

#include <nvk/base/RefPtr.h>
#include <nvk/base/WeakRefObject.h>

namespace nv {

template <typename T> class WeakPtr {
  public:
    using element_type = T;

    WeakPtr() : _ptr(nullptr), _controlBlock(nullptr) {}

    // Construct from RefPtr
    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    WeakPtr(const RefPtr<T>& ref) : _ptr(ref._ptr), _controlBlock(nullptr) {
        if (_ptr) {
            // Check at runtime - only WeakRefObject supports weak refs
            NVCHK(_ptr->supports_weak_refs(),
                  "WeakPtr: Object does not support weak references. "
                  "Inherit from WeakRefObject instead of RefObject.");

            // Cast to WeakRefObject to get control block
            auto* weak_obj = static_cast<WeakRefObject*>(_ptr);
            _controlBlock = weak_obj->get_control_block();
            if (_controlBlock != nullptr) {
                _controlBlock->add_weak_ref();
            }
        }
    }

    WeakPtr(const WeakPtr& other)
        : _ptr(other._ptr), _controlBlock(other._controlBlock) {
        if (_controlBlock != nullptr) {
            _controlBlock->add_weak_ref();
        }
    }

    template <class Other>
    WeakPtr(const WeakPtr<Other>& other)
        : _ptr(other._ptr), _controlBlock(other._controlBlock) {
        if (_controlBlock != nullptr) {
            _controlBlock->add_weak_ref();
        }
    }

    ~WeakPtr() {
        if (_controlBlock != nullptr && _controlBlock->release_weak_ref()) {
            RefControlBlockPool::instance().release(_controlBlock);
        }
        _ptr = nullptr;
        _controlBlock = nullptr;
    }

    auto operator=(const WeakPtr& other) -> WeakPtr& {
        if (this != &other) {
            if (_controlBlock != nullptr && _controlBlock->release_weak_ref()) {
                RefControlBlockPool::instance().release(_controlBlock);
            }

            _ptr = other._ptr;
            _controlBlock = other._controlBlock;
            if (_controlBlock != nullptr) {
                _controlBlock->add_weak_ref();
            }
        }
        return *this;
    }

    auto operator=(const RefPtr<T>& ref) -> WeakPtr& {
        if (_controlBlock != nullptr && _controlBlock->release_weak_ref()) {
            RefControlBlockPool::instance().release(_controlBlock);
        }

        _ptr = ref._ptr;
        _controlBlock = nullptr;

        if (_ptr) {
            NVCHK(_ptr->supports_weak_refs(),
                  "WeakPtr: Object does not support weak references. "
                  "Inherit from WeakRefObject instead of RefObjectSimple.");

            auto* weak_obj = static_cast<WeakRefObject*>(_ptr);
            _controlBlock = weak_obj->get_control_block();
            if (_controlBlock != nullptr) {
                _controlBlock->add_weak_ref();
            }
        }

        return *this;
    }

    WeakPtr(WeakPtr&& other) noexcept
        : _ptr(other._ptr), _controlBlock(other._controlBlock) {
        other._ptr = nullptr;
        other._controlBlock = nullptr;
    }

    auto operator=(WeakPtr&& other) noexcept -> WeakPtr& {
        if (this != &other) {
            if (_controlBlock != nullptr && _controlBlock->release_weak_ref()) {
                RefControlBlockPool::instance().release(_controlBlock);
            }

            _ptr = other._ptr;
            _controlBlock = other._controlBlock;
            other._ptr = nullptr;
            other._controlBlock = nullptr;
        }
        return *this;
    }

    [[nodiscard]] auto lock() const -> RefPtr<T> {
        if (_controlBlock != nullptr && _controlBlock->try_add_strong_ref()) {
            return RefPtr<T>(_ptr, RefPtr<T>::Adopt); // Use adopt constructor
        }
        return RefPtr<T>();
    }

    [[nodiscard]] auto expired() const -> bool {
        return _controlBlock == nullptr || _controlBlock->strong_count() == 0;
    }

    [[nodiscard]] auto use_count() const -> I64 {
        return _controlBlock != nullptr ? _controlBlock->strong_count() : 0;
    }

    void reset() {
        if (_controlBlock != nullptr && _controlBlock->release_weak_ref()) {
            RefControlBlockPool::instance().release(_controlBlock);
        }
        _ptr = nullptr;
        _controlBlock = nullptr;
    }

    void swap(WeakPtr& other) noexcept {
        T* tmp_ptr = _ptr;
        RefControlBlock* tmp_block = _controlBlock;

        _ptr = other._ptr;
        _controlBlock = other._controlBlock;

        other._ptr = tmp_ptr;
        other._controlBlock = tmp_block;
    }

  private:
    template <class Other> friend class WeakPtr;
    template <class Other> friend class RefPtr;

    T* _ptr{nullptr};
    RefControlBlock* _controlBlock{nullptr};
};

template <typename T>
RefPtr<T>::RefPtr(const WeakPtr<T>& weak) : _ptr(nullptr) {
    if (weak._controlBlock != nullptr &&
        weak._controlBlock->try_add_strong_ref()) {
        _ptr = weak._ptr;
    }
}

} // namespace nv

#endif