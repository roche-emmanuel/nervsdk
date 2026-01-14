#ifndef _NV_POINTATTRIBUTE_H_
#define _NV_POINTATTRIBUTE_H_

#include <nvk_common.h>

namespace nv {

class PointAttribute : public RefObject {
    NV_DECLARE_NO_COPY(PointAttribute)
    NV_DECLARE_NO_MOVE(PointAttribute)
  public:
    struct Traits {};

    template <typename T> class AttributeHolder;

    explicit PointAttribute(String name, Traits traits);
    ~PointAttribute() override;
    virtual void resize(U32 size) = 0;
    virtual auto size() const -> U64 = 0;
    virtual auto element_size() const -> U32 = 0;

    auto name() const -> const String& { return _name; }
    auto get_type_index() const -> std::type_index { return _typeIndex; }

    template <typename T> auto is_type() const -> bool {
        return _typeIndex == std::type_index(typeid(T));
    }

    // Set values (with type checking)
    template <typename T> void set_values(Vector<T>&& values) {
        NVCHK(_typeIndex == std::type_index(typeid(T)),
              "PointAttribute::set_values: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->assign_values(
            std::move(values));
    }

    template <typename T> void set_values(const Vector<T>& values) {
        NVCHK(_typeIndex == std::type_index(typeid(T)),
              "PointAttribute::set_values: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->assign_values(values);
    }

    // Get values (with type checking)
    template <typename T> auto get_values() -> Vector<T>& {
        NVCHK(_typeIndex == std::type_index(typeid(T)),
              "PointAttribute::get_values: type mismatch.");
        return static_cast<AttributeHolder<T>*>(this)->retrieve_values();
    }

    template <typename T> auto get_values() const -> const Vector<T>& {
        NVCHK(_typeIndex == std::type_index(typeid(T)),
              "PointAttribute::get_values: type mismatch.");
        return static_cast<const AttributeHolder<T>*>(this)->retrieve_values();
    }

    // Get single value at index
    template <typename T> auto get_value(U64 index) const -> const T& {
        NVCHK(_typeIndex == std::type_index(typeid(T)),
              "PointAttribute::get_value: type mismatch.");
        return static_cast<const AttributeHolder<T>*>(this)->retrieve_value(
            index);
    }

    // Set single value at index
    template <typename T> void set_value(U64 index, T&& value) {
        NVCHK(_typeIndex == std::type_index(typeid(T)),
              "PointAttribute::set_value: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->assign_value(
            index, std::forward<T>(value));
    }

    // Factory method
    template <typename T>
    static auto create(String name, U32 size, const T& value = T{},
                       Traits traits = {}) -> RefPtr<PointAttribute> {
        return nv::create<AttributeHolder<T>>(std::move(name), size, value,
                                              std::move(traits));
    }

    template <typename T>
    static auto create(String name, Vector<T> values, Traits traits = {})
        -> RefPtr<PointAttribute> {
        return nv::create<AttributeHolder<T>>(
            std::move(name), std::move(values), std::move(traits));
    }

  protected:
    Traits _traits;
    String _name;
    std::type_index _typeIndex{typeid(void)};
};

template <typename T>
class PointAttribute::AttributeHolder : public PointAttribute {
  public:
    AttributeHolder(String name, U32 size, const T& value, Traits traits)
        : PointAttribute(std::move(name), std::move(traits)),
          _values(size, value) {
        _typeIndex = std::type_index(typeid(T));
    }

    AttributeHolder(String name, Vector<T> values, Traits traits)
        : PointAttribute(std::move(name), std::move(traits)),
          _values(std::move(values)) {
        _typeIndex = std::type_index(typeid(T));
    }

    void assign_values(Vector<T>&& values) { _values = std::move(values); }
    void assign_values(const Vector<T>& values) { _values = values; }

    auto retrieve_values() -> Vector<T>& { return _values; }
    auto retrieve_values() const -> const Vector<T>& { return _values; }

    auto retrieve_value(U64 index) const -> const T& {
        NVCHK(index < _values.size(),
              "PointAttribute::retrieve_value: index {} out of bounds (size: "
              "{})",
              index, _values.size());
        return _values[index];
    }

    void assign_value(U64 index, T&& value) {
        NVCHK(index < _values.size(),
              "PointAttribute::assign_value: index {} out of bounds (size: {})",
              index, _values.size());
        _values[index] = std::forward<T>(value);
    }

    void resize(U32 size) override { _values.resize(size); }
    auto size() const -> U64 override { return _values.size(); }
    auto element_size() const -> U32 override { return sizeof(T); }

  protected:
    Vector<T> _values;
};

using PointAttributeVector = Vector<RefPtr<PointAttribute>>;
using PointAttributeMap = UnorderedMap<String, RefPtr<PointAttribute>>;

// Convenience type aliases (optional - can cast from RefPtr<PointAttribute>)
template <typename T>
using TypedPointAttribute = PointAttribute::AttributeHolder<T>;
using F32PointAttribute = TypedPointAttribute<F32>;
using F64PointAttribute = TypedPointAttribute<F64>;
using Vec3fPointAttribute = TypedPointAttribute<Vec3f>;
using Vec3dPointAttribute = TypedPointAttribute<Vec3d>;

} // namespace nv

#endif