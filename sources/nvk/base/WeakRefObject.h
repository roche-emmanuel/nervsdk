#ifndef NV_WEAKREFOBJECT_
#define NV_WEAKREFOBJECT_

#include <nvk/base/RefObject.h>
#include <nvk/base/SlotProvider.h>

namespace nv {

// Lightweight control block for weak pointer support
class RefControlBlock {
  public:
    RefControlBlock();

    // Reset to initial state (called when reusing from pool)
    void reset();

    void add_strong_ref();

    auto release_strong_ref() -> bool;

    void add_weak_ref();

    auto release_weak_ref() -> bool;

    auto try_add_strong_ref() -> bool;

    [[nodiscard]] auto strong_count() const -> I64;

    [[nodiscard]] auto weak_count() const -> I64;

  private:
    std::atomic<I64> _strongCount;
    std::atomic<I64> _weakCount;
};

// Pool-based allocator for control blocks
class RefControlBlockPool {
  public:
    static auto instance() -> RefControlBlockPool&;

    auto acquire() -> RefControlBlock*;

    void release(RefControlBlock* block);

    // Statistics
    [[nodiscard]] auto allocated_count() const -> U32;

    [[nodiscard]] auto total_capacity() const -> U32;

  private:
    RefControlBlockPool() = default;

    SlotProvider<RefControlBlock, U32, Deque<RefControlBlock>> _provider;
    UnorderedMap<RefControlBlock*, U32> _indexMap;
};

// Implementation for objects WITH weak pointer support
// Uses pooled control block
class WeakRefObject : public RefObject {
  public:
    WeakRefObject();

    ~WeakRefObject() override;

    void ref() final;

    void unref() final;

    void unref_nodelete() final;

    [[nodiscard]] auto ref_count() const -> U64 final {
        return static_cast<U64>(_controlBlock->strong_count());
    }

    [[nodiscard]] auto supports_weak_refs() const -> bool final { return true; }

    auto get_control_block() const -> RefControlBlock* { return _controlBlock; }

  private:
    RefControlBlock* _controlBlock;
    template <typename T> friend class WeakPtr;
};

} // namespace nv

#endif