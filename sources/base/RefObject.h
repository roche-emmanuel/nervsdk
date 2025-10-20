#ifndef NV_REFOBJECT_
#define NV_REFOBJECT_

#include <nvk_config.h>
#include <nvk_macros.h>

namespace nv {

class Allocator;

class RefObject {
    NV_DECLARE_NO_COPY(RefObject)
    NV_DECLARE_NO_MOVE(RefObject)

  public:
    // Default constructor
    RefObject();

    // Destructor
    virtual ~RefObject();

    static void check_memory_refs();

    /**
      Increment the reference count.
    */
    void ref();

    /**
    Decrement the reference count.
    */
    void unref();

    /** Decrement the reference count by one, indicating that
            a pointer to this object is no longer referencing it.  However, do
            not delete it, even if ref count goes to 0.  Warning,
       unref_nodelete() should only be called if the user knows exactly who will
            be responsible for, one should prefer unref() over unref_nodelete()
            as the latter can lead to memory leaks.*/
    void unref_nodelete();

    /**
    Retrieve the current reference count.
    */
    [[nodiscard]] inline auto ref_count() const -> U64 {
        return _refCount.load(std::memory_order_acquire);
    }

    template <class U> inline auto cast() -> U* {
        return dynamic_cast<U*>(this);
    }

    [[nodiscard]] virtual auto get_class_name() const -> const char* {
        return "RefObject";
    }

    [[nodiscard]] virtual auto get_class_id() const -> StringID;

    virtual auto cast(StringID tid) -> RefObject*;

    template <typename T> inline auto cast_to() -> T* {
        return (T*)cast(T::class_id);
    }

    friend class Allocator;

  private:
    std::atomic<I64> _refCount;

#if !NV_USE_STD_MEMORY
    /** The allocator that was used to create this object */
    Allocator* _allocator{nullptr};

  protected:
    /** Notify the creation and assign the allocator for this object.
    This should only be called once by the allocator that created this object
    itself. */
    virtual void set_allocator(Allocator* alloc);
#endif
};

} // namespace nv

#endif
