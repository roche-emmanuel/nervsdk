#ifndef _NV_SPLOTPROVIDER_H_
#define _NV_SPLOTPROVIDER_H_

#include <nvk/base/std_containers.h>

namespace nv {

template <typename T, typename Index = U32, typename ContType = Vector<T>>
class SlotProvider {
  public:
    // Constructor to initialize the container
    SlotProvider() = default;

    // Acquire one slot
    auto acquire_slot() -> Index {
        if (_freeSlots.empty()) {
            _slots.emplace_back();
            return _slots.size() - 1;
        }

        // Take the smallest available index (first element in set)
        Index index = *_freeSlots.begin();
        _freeSlots.erase(_freeSlots.begin());
        return index;
    }

    // Acquire multiple slots
    auto acquire_slots(Index count) -> Vector<Index> {
        Vector<Index> indices;
        indices.reserve(count);

        // Reuse free slots if available (take smallest indices first)
        while (!_freeSlots.empty() && indices.size() < count) {
            indices.push_back(*_freeSlots.begin());
            _freeSlots.erase(_freeSlots.begin());
        }

        // Allocate new slots if needed
        if (indices.size() < count) {
            Index newSlotsNeeded = count - indices.size();
            Index curSize = _slots.size();
            _slots.resize(curSize + newSlotsNeeded);
            for (Index i = 0; i < newSlotsNeeded; ++i) {
                indices.push_back(curSize + i);
            }
        }

        return indices;
    }

    // Release a slot
    void release_slot(Index index) {
        NVCHK(index < _slots.size(),
              "SlotProvider::release_slot: Index out of range: {} >= {}", index,
              _slots.size());

        // Check if already released
        NVCHK(!_freeSlots.contains(index),
              "SlotProvider::release_slot: Index {} already released", index);

        // Insert into set (automatically maintains sorted order)
        _freeSlots.insert(index);
    }

    // Release multiple slots
    void release_slots(const Vector<Index>& indices) {
        for (Index index : indices) {
            NVCHK(index < _slots.size(),
                  "SlotProvider::release_slots: Index out of range: {} >= {}",
                  index, _slots.size());

            // Check if already released
            NVCHK(!_freeSlots.contains(index),
                  "SlotProvider::release_slots: Index {} already released",
                  index);

            // Insert into set (automatically maintains sorted order)
            _freeSlots.insert(index);
        }
    }

    // Access a slot
    auto operator[](Index index) -> T& {
        NVCHK(index < _slots.size(),
              "SlotProvider::operator[]: Index out of range: {} >= {}", index,
              _slots.size());
        return _slots[index];
    }

    // Access a slot (const version)
    auto operator[](Index index) const -> const T& {
        NVCHK(index < _slots.size(),
              "SlotProvider::operator[]: Index out of range: {} >= {}", index,
              _slots.size());
        return _slots[index];
    }

    // Get the vector of slots
    auto get_slots() const -> const ContType& { return _slots; }
    auto get_slots() -> ContType& { return _slots; }

    // Get the current size of the internal vector
    [[nodiscard]] auto size() const -> Index { return _slots.size(); }

    // Get number of free slots
    [[nodiscard]] auto free_count() const -> Index { return _freeSlots.size(); }

    // Get number of used slots
    [[nodiscard]] auto used_count() const -> Index {
        return _slots.size() - _freeSlots.size();
    }

    // Clear all slots
    void clear() {
        _slots.clear();
        _freeSlots.clear();
    }

    // Resize the vector to a given size
    void resize(size_t size) {
        NVCHK(size <= _slots.size(), "Can only resize to remove elements.");

        if (size < _slots.size()) {
            // Remove free slots that are now out of bounds
            auto it = _freeSlots.lower_bound(size);
            _freeSlots.erase(it, _freeSlots.end());
        }
        _slots.resize(size);
    }

    // Check if the free slots are properly sorted (for debugging)
    [[nodiscard]] auto is_free_slots_sorted() const -> bool {
        // std::set is always sorted by definition
        return true;
    }

  private:
    ContType _slots;
    std::set<Index> _freeSlots; // Sorted set of free slot indices (min to max)
};

} // namespace nv

#endif