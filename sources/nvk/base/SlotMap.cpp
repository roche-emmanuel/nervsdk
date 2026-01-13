#include <nvk/base/SlotMap.h>

namespace nv {
auto SlotMap::create() -> RefPtr<SlotMap> { return nv::create<SlotMap>(); }
auto SlotMap::get_slot_names() const -> Vector<String> {
    Vector<String> names;
    names.reserve(_slots.size());
    for (const auto& pair : _slots) {
        names.push_back(pair.first);
    }
    return names;
}
auto SlotMap::size() const -> size_t { return _slots.size(); }
void SlotMap::clear() { _slots.clear(); }
auto SlotMap::remove_slot(const String& slotName) -> bool {
    return _slots.erase(slotName) > 0;
}
auto SlotMap::has_slot(const String& slotName) const -> bool {
    return _slots.contains(slotName);
}
auto SlotMap::get(String slotName) const -> GetProxy {
    return {this, std::move(slotName)};
}
} // namespace nv
