#include <nvk/base/WeakRefObject.h>

namespace nv {
RefControlBlock::RefControlBlock() : _strongCount(0), _weakCount(1) {}
void RefControlBlock::reset() {
    _strongCount.store(0, std::memory_order_relaxed);
    _weakCount.store(1, std::memory_order_relaxed);
}
void RefControlBlock::add_strong_ref() {
    _strongCount.fetch_add(1, std::memory_order_relaxed);
}
auto RefControlBlock::release_strong_ref() -> bool {
    return _strongCount.fetch_sub(1, std::memory_order_acq_rel) == 1;
}
void RefControlBlock::add_weak_ref() {
    _weakCount.fetch_add(1, std::memory_order_relaxed);
}
auto RefControlBlock::release_weak_ref() -> bool {
    return _weakCount.fetch_sub(1, std::memory_order_acq_rel) == 1;
}
auto RefControlBlock::try_add_strong_ref() -> bool {
    I64 count = _strongCount.load(std::memory_order_relaxed);
    do {
        if (count == 0) {
            return false;
        }
    } while (!_strongCount.compare_exchange_weak(count, count + 1,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_relaxed));
    return true;
}
auto RefControlBlock::strong_count() const -> I64 {
    return _strongCount.load(std::memory_order_acquire);
}
auto RefControlBlock::weak_count() const -> I64 {
    return _weakCount.load(std::memory_order_acquire);
}
auto RefControlBlockPool::instance() -> RefControlBlockPool& {
    static RefControlBlockPool pool;
    return pool;
}
auto RefControlBlockPool::acquire() -> RefControlBlock* {
    U32 index = _provider.acquire_slot();
    RefControlBlock* block = &_provider[index];
    // Reset atomics to initial state (much faster than placement new)
    block->reset();
    // Store the index in the block for later release
    _indexMap[block] = index;
    return block;
}
void RefControlBlockPool::release(RefControlBlock* block) {
    auto it = _indexMap.find(block);
    if (it != _indexMap.end()) {
        U32 index = it->second;
        _indexMap.erase(it);
        _provider.release_slot(index);
    }
}
auto RefControlBlockPool::allocated_count() const -> U32 {
    return _provider.used_count();
}
auto RefControlBlockPool::total_capacity() const -> U32 {
    return _provider.size();
}
WeakRefObject::WeakRefObject()
    : _controlBlock(RefControlBlockPool::instance().acquire()) {}
WeakRefObject::~WeakRefObject() {
    if (_controlBlock && _controlBlock->release_weak_ref()) {
        RefControlBlockPool::instance().release(_controlBlock);
    }
}
void WeakRefObject::ref() { _controlBlock->add_strong_ref(); }
void WeakRefObject::unref() {
    if (_controlBlock->release_strong_ref()) {
        delete_object();
    }
}
void WeakRefObject::unref_nodelete() { _controlBlock->release_strong_ref(); }
} // namespace nv
