#ifndef NV_REFPTR_
#define NV_REFPTR_

namespace nv {

template <typename T> class WeakPtr;

template <typename T> class RefPtr {
  public:
    using element_type = T;

    RefPtr() : _ptr(nullptr) {}

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

    // Construct from WeakPtr
    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    explicit RefPtr(const WeakPtr<T>& weak);

    ~RefPtr() {
        if (_ptr) {
            _ptr->unref();
        }
        _ptr = nullptr;
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

    // Move constructor
    RefPtr(RefPtr&& ref) noexcept {
        // NOLINTNEXTLINE
        _ptr = ref._ptr;
        ref._ptr = nullptr;
    }

    // Move assignment
    auto operator=(RefPtr&& ref) noexcept -> RefPtr& {
        T* tmp_ptr = _ptr;
        // NOLINTNEXTLINE
        _ptr = ref._ptr;
        ref._ptr = nullptr;
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

    explicit operator bool() const { return valid(); }

    auto operator*() const -> T& { return *_ptr; }
    auto operator->() const -> T* { return _ptr; }
    auto get() const -> T* { return _ptr; }

    auto operator!() const -> bool { return _ptr == nullptr; }
    [[nodiscard]] auto valid() const -> bool { return _ptr != nullptr; }

    auto release() -> T* {
        T* tmp = _ptr;
        if (_ptr) {
            _ptr->unref_nodelete();
        }
        _ptr = nullptr;
        return tmp;
    }

    void swap(RefPtr& ref) {
        T* tmp = _ptr;
        _ptr = ref._ptr;
        ref._ptr = tmp;
    }

    void reset(T* ptr = nullptr) { *this = ptr; }

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
        if (tmp_ptr) {
            tmp_ptr->unref();
        }
    }

    template <class Other> friend class RefPtr;
    template <class Other> friend class WeakPtr;

    T* _ptr{nullptr};

    struct AdoptTag {};
    static constexpr AdoptTag Adopt{};

    RefPtr(T* ptr, AdoptTag) : _ptr(ptr) {
        // Don't call ref() - we're adopting an existing reference
    }
};

} // namespace nv

#endif
