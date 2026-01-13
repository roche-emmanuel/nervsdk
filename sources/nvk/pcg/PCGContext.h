#ifndef _NV_PCGCONTEXT_H_
#define _NV_PCGCONTEXT_H_

#include <nvk/pcg/PointArray.h>
#include <nvk_type_ids.h>

namespace nv {

class PCGContext : public RefObject {
    NV_DECLARE_NO_COPY(PCGContext)
    NV_DECLARE_NO_MOVE(PCGContext)

  public:
    template <typename T> class InputSlotHolder;

    class InputSlot : public RefObject {
        NV_DECLARE_NO_COPY(InputSlot)
        NV_DECLARE_NO_MOVE(InputSlot)

      public:
        InputSlot() {};

        auto get_data_type_id() const -> StringID { return _dataTypeId; }

        template <typename T> void set_value(T val) {
            NVCHK(_dataTypeId == TypeId<T>::id,
                  "InputSlot::set_value: mismatch in type ids.");

            ((InputSlotHolder<T>*)this)->assign_value(val);
        };

        template <typename T> static auto create() -> RefPtr<InputSlot> {
            return nv::create<InputSlotHolder<T>>();
        }

      protected:
        StringID _dataTypeId{0};
    };

    template <typename T> class InputSlotHolder : public InputSlot {
      protected:
        T _value{};

      public:
        InputSlotHolder() { _dataTypeId = TypeId<T>::id; };
        void assign_value(T val) { _value = std::forward<T>(val); }
    };

    struct Traits {};

    explicit PCGContext(Traits traits);
    ~PCGContext() override;

    static auto create(Traits traits = {}) -> RefPtr<PCGContext>;

    template <typename T>
    auto get_or_create_input_slot(String slotName) -> InputSlot& {
        auto it = _inputs.find(slotName);
        if (it != _inputs.end()) {
            // Check the type of this socket is correct:
            NVCHK(it->second->get_data_type_id() == TypeId<T>::id,
                  "Mismatch in input slot data type id.");
            return *it->second;
        }

        // Create a new slot with this data type:
        auto slot = InputSlot::create<T>();
        auto res = _inputs.insert(std::make_pair(std::move(slotName), slot));
        NVCHK(res.second, "Could not insert input {}", slotName);
        return *res.first->second;
    };

    template <typename T>
    auto set_input(String slotName, T value) -> InputSlot& {
        auto& socket = get_or_create_input_slot<T>(std::move(slotName));

        socket.set_value(std::move(value));
        return socket;
    };

  protected:
    Traits _traits;

    UnorderedMap<String, RefPtr<InputSlot>> _inputs;
};

}; // namespace nv

#endif
