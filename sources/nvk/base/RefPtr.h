#ifndef NV_REFPTR_
#define NV_REFPTR_

namespace nv {

template <typename T> class RefPtr {
  public:
    using element_type = T;

    RefPtr() : _ptr(0) {}

    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    RefPtr(T* ptr) : _ptr(ptr) {
        if (_ptr) {
            _ptr->ref();
        }
    }

    RefPtr(const RefPtr& ref) : _ptr(ref._ptr) {
        if (_ptr) {
            _ptr->ref();
        }
    }

    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    template <class Other> RefPtr(const RefPtr<Other>& ref) : _ptr(ref._ptr) {
        if (_ptr) {
            _ptr->ref();
        }
    }

    // RefPtr(observer_ptr<T>& optr) : _ptr(0) { optr.lock(*this); }
    ~RefPtr() {
        if (_ptr) {
            _ptr->unref();
        }
        _ptr = 0;
    }

    auto operator=(const RefPtr& ref) -> RefPtr& {
        if (&ref != this) {
            assign(ref);
        }
        return *this;
    }

    template <class Other> auto operator=(const RefPtr<Other>& ref) -> RefPtr& {
        assign(ref);
        return *this;
    }

    // Move constructor:
    // RefPtr(RefPtr&& ref) noexcept = delete;
    RefPtr(RefPtr&& ref) noexcept {
        // NOLINTNEXTLINE
        _ptr = ref._ptr;
        // we do not change the ptr count for the ref (since we add 1 and sub 1)
        // Just reset that ref ptr:
        ref._ptr = nullptr;
    }

    // Move assignment:
    // auto operator=(RefPtr&& ref) noexcept -> RefPtr& = delete;
    auto operator=(RefPtr&& ref) noexcept -> RefPtr& {
        T* tmp_ptr = _ptr;
        // NOLINTNEXTLINE
        _ptr = ref._ptr;
        // we do not change the ptr count for the ref (since we add 1 and sub 1)
        // Just reset that ref ptr:
        ref._ptr = nullptr;
        // And unref our prev ptr:
        if (tmp_ptr) {
            tmp_ptr->unref();
        }
        return *this;
    }

    inline auto operator=(T* ptr) -> RefPtr& {
        if (_ptr == ptr) {
            return *this;
        }
        T* tmp_ptr = _ptr;
        _ptr = ptr;
        if (_ptr) {
            _ptr->ref();
        }
        // unref second to prevent any deletion of any object which might
        // be referenced by the other object. i.e rp is child of the
        // original _ptr.
        if (tmp_ptr) {
            tmp_ptr->unref();
        }
        return *this;
    }

    // comparison operators for RefPtr.
    auto operator==(const RefPtr& ref) const -> bool {
        return (_ptr == ref._ptr);
    }
    auto operator==(const T* ptr) const -> bool { return (_ptr == ptr); }
    friend auto operator==(const T* ptr, const RefPtr& ref) -> bool {
        return (ptr == ref._ptr);
    }

    auto operator!=(const RefPtr& ref) const -> bool {
        return (_ptr != ref._ptr);
    }
    auto operator!=(const T* ptr) const -> bool { return (_ptr != ptr); }
    friend auto operator!=(const T* ptr, const RefPtr& ref) -> bool {
        return (ptr != ref._ptr);
    }

    auto operator<(const RefPtr& ref) const -> bool {
        return (_ptr < ref._ptr);
    }

    // follows is an implmentation of the "safe bool idiom", details can be
    // found at:
    //   http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Safe_bool
    //   http://lists.boost.org/Archives/boost/2003/09/52856.php

    //   private:
    //     using unspecified_bool_type = T* RefPtr::*;

    //   public:
    //     // safe bool conversion
    //     explicit operator unspecified_bool_type() const {
    //         return valid() ? &RefPtr::_ptr : 0;
    //     }
    // Update C++11 explicit bool conversion:
    // cf. https://www.kdab.com/explicit-operator-bool/
    explicit operator bool() const { return valid(); }

    auto operator*() const -> T& {
        // #ifndef NDEBUG
        //         NVCHK(_ptr != nullptr, "RefPtr: invalid pointer
        //         dereference.");
        // #endif
        return *_ptr;
    }
    auto operator->() const -> T* { return _ptr; }
    auto get() const -> T* { return _ptr; }

    auto operator!() const -> bool { return _ptr == 0; } // not required
    [[nodiscard]] auto valid() const -> bool { return _ptr != 0; }

    auto release() -> T* {
        T* tmp = _ptr;
        if (_ptr) {
            _ptr->unref_nodelete();
        }
        _ptr = 0;
        return tmp;
    }

    void swap(RefPtr& ref) {
        T* tmp = _ptr;
        _ptr = ref._ptr;
        ref._ptr = tmp;
    }

    void reset(T* ptr = 0) { *this = ptr; }

  private:
    template <class Other> void assign(const RefPtr<Other>& ref) {
        if (_ptr == ref._ptr) {
            return;
        }
        T* tmp_ptr = _ptr;
        _ptr = ref._ptr;
        if (_ptr) {
            _ptr->ref();
        }
        // unref second to prevent any deletion of any object which might
        // be referenced by the other object. i.e rp is child of the
        // original _ptr.
        if (tmp_ptr) {
            tmp_ptr->unref();
        }
    }

    template <class Other> friend class RefPtr;

    T* _ptr{nullptr};
};

}; // namespace nv

#endif
