#ifndef _NV_SLOTMAP_H_
#define _NV_SLOTMAP_H_

namespace nv {

/**
 * A collection of named, type-safe data slots.
 * Can be used for inputs, outputs, or any other collection of heterogeneous
 * named data.
 */
class SlotMap : public RefObject {
    NV_DECLARE_NO_COPY(SlotMap)
    NV_DECLARE_NO_MOVE(SlotMap)

  public:
    template <typename T> class SlotHolder;

    class Slot : public RefObject {
        NV_DECLARE_NO_COPY(Slot)
        NV_DECLARE_NO_MOVE(Slot)
      public:
        Slot() = default;
        virtual ~Slot() = default;

        auto get_type_index() const -> std::type_index { return _typeIndex; }

        template <typename T> auto is_a() const -> bool {
            return _typeIndex == std::type_index(typeid(T));
        }

        template <typename T> void set_value(T&& val) {
            using CleanT = std::decay_t<T>;
            NVCHK(_typeIndex == std::type_index(typeid(CleanT)),
                  "Slot::set_value: type mismatch.");
            // Forward with the original T, not CleanT
            static_cast<SlotHolder<CleanT>*>(this)->assign_value(
                std::forward<T>(val));
        }

        template <typename T> auto get_value() const -> const T& {
            NVCHK(_typeIndex == std::type_index(typeid(T)),
                  "Slot::get_value: type mismatch.");
            return static_cast<const SlotHolder<T>*>(this)->retrieve_value();
        }
        template <typename T> auto get_value() -> T& {
            NVCHK(_typeIndex == std::type_index(typeid(T)),
                  "Slot::get_value: type mismatch.");
            return static_cast<SlotHolder<T>*>(this)->retrieve_value();
        }

        template <typename T> static auto create() -> RefPtr<Slot> {
            return nv::create<SlotHolder<std::decay_t<T>>>();
        }

        template <typename T> auto as_vector() const -> Vector<T> {
            if (is_a<T>()) {
                return Vector<T>{get_value<T>()};
            }
            if (is_a<Vector<T>>()) {
                return get_value<Vector<T>>();
            }
            THROW_MSG("Cannot convert slot to vector");
            return {};
        }

      protected:
        std::type_index _typeIndex{typeid(void)};
    };

    template <typename T> class SlotHolder : public Slot {
      protected:
        T _value{};

      public:
        SlotHolder() { _typeIndex = std::type_index(typeid(T)); }

        template <typename U> void assign_value(U&& val) {
            _value = std::forward<U>(val);
        }

        auto retrieve_value() const -> const T& { return _value; }
        auto retrieve_value() -> T& { return _value; }
    };

    SlotMap() = default;
    ~SlotMap() override = default;

    static auto create() -> RefPtr<SlotMap>;

    auto find_raw_slot(const String& slotName) const -> Slot* {
        auto it = _slots.find(slotName);
        if (it != _slots.end()) {
            return it->second.get();
        }
        return nullptr;
    }
    auto get_raw_slot(const String& slotName) const -> Slot& {
        auto* slot = find_raw_slot(slotName);
        NVCHK(slot != nullptr, "Invalid slot with name {}", slotName);
        return *slot;
    }

    // Find a slot by name (returns nullptr if not found)
    template <typename T>
    auto find_slot(const String& slotName) const -> Slot* {
        auto it = _slots.find(slotName);
        if (it != _slots.end()) {
            NVCHK(it->second->get_type_index() == std::type_index(typeid(T)),
                  "Slot '{}' exists but has different type: {} != {}", slotName,
                  it->second->get_type_index().name(),
                  std::type_index(typeid(T)).name());
            return it->second.get();
        }
        return nullptr;
    }

    // Get a slot by name (throws if not found)
    template <typename T> auto get_slot(const String& slotName) const -> Slot& {
        auto* slot = find_slot<T>(slotName);
        NVCHK(slot != nullptr, "Slot '{}' not found.", slotName);
        return *slot;
    }

    // Get or create a slot
    template <typename T> auto get_or_create_slot(String slotName) -> Slot& {
        auto* slot = find_slot<T>(slotName);
        if (slot != nullptr) {
            return *slot;
        }
        auto res = _slots.insert(
            std::make_pair(std::move(slotName), Slot::create<T>()));
        NVCHK(res.second, "Failed to insert slot '{}'.", slotName);
        return *res.first->second;
    }

    // Set a value (template type deduced from value)
    template <typename T> auto set(String slotName, T&& value) -> SlotMap& {
        using CleanT = std::decay_t<T>;
        auto& slot = get_or_create_slot<CleanT>(std::move(slotName));
        slot.set_value(std::forward<T>(value));
        return *this;
    }

    // Get a value (template type must be specified)
    template <typename T> auto get(const String& slotName) const -> T& {
        auto& slot = get_slot<T>(slotName);
        return slot.template get_value<T>();
    }

    // Get a value with default fallback (type deduced from default value)
    template <typename T>
    auto get(const String& slotName, T&& defaultValue) const -> T {
        using CleanT = std::decay_t<T>;
        auto it = _slots.find(slotName);
        if (it == _slots.end()) {
            return std::forward<T>(defaultValue);
        }
        NVCHK(it->second->get_type_index() == std::type_index(typeid(CleanT)),
              "Slot '{}' exists but has type mismatch (expected {}, got {}).",
              slotName, typeid(CleanT).name(),
              it->second->get_type_index().name());
        return it->second->template get_value<CleanT>();
    }

    template <typename T> auto is_a(const String& slotName) const -> bool {
        auto it = _slots.find(slotName);
        if (it == _slots.end()) {
            return false;
        }

        return it->second->is_a<T>();
    }

    // Type-deducing getter via conversion operator proxy
    class GetProxy {
        const SlotMap* _map;
        String _slotName;

      public:
        GetProxy(const SlotMap* map, String slotName)
            : _map(map), _slotName(std::move(slotName)) {}

        // template <typename T> operator T() const {
        //     return _map->get<T>(_slotName);
        // }

        template <typename T> operator T&() const {
            return _map->get<T>(_slotName);
        }
    };

    // Type-deducing get (works via assignment: double val = slots.get("name"))
    auto get(String slotName) const -> GetProxy;

    // Check if a slot exists
    auto has_slot(const String& slotName) const -> bool;

    // Remove a slot
    auto remove_slot(const String& slotName) -> bool;

    // Clear all slots
    void clear();

    // Get number of slots
    auto size() const -> size_t;

    // Iterate over slot names
    auto get_slot_names() const -> Vector<String>;

  protected:
    UnorderedMap<String, RefPtr<Slot>> _slots;
};

} // namespace nv

#endif
