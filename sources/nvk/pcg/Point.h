#ifndef _NV_PCG_POINT_H_
#define _NV_PCG_POINT_H_

#include <nvk/pcg/PointArray.h>

namespace nv {

// PCGPointRef - provides reference semantics (modifies underlying PointArray)
class PCGPointRef {
  public:
    PCGPointRef(PointArray* array, U64 index) : _array(array), _index(index) {}

    // Get attribute value
    template <typename T>
    [[nodiscard]] auto get(const String& name) const -> T {
        const auto& attr = _array->get_attribute(name);
        return attr.get_value<T>(_index);
    }

    // Set attribute value (modifies underlying array)
    template <typename T> void set(const String& name, const T& value) {
        auto& attr = _array->get_attribute(name);
        attr.set_value<T>(_index, T(value));
    }

    // Convenience accessors for standard attributes
    [[nodiscard]] auto position() const -> Vec3d;
    void set_position(const Vec3d& p);

    [[nodiscard]] auto rotation() const -> Vec3d;
    void set_rotation(const Vec3d& r);

    [[nodiscard]] auto scale() const -> Vec3d;
    void set_scale(const Vec3d& s);

    [[nodiscard]] auto index() const -> U64;
    [[nodiscard]] auto array() const -> const PointArray*;
    auto array() -> PointArray*;

    // Convert to value copy
    [[nodiscard]] auto copy() const -> PCGPoint;

  private:
    PointArray* _array;
    U64 _index;
};

// Point - provides value semantics (independent copy)
class PCGPoint {
  public:
    PCGPoint() = default;

    // Construct from a PCGPointRef (makes a copy)
    explicit PCGPoint(const PCGPointRef& ref);

    // Get attribute value
    template <typename T>
    [[nodiscard]] auto get(const String& name) const -> const T& {
        auto it = _values.find(name);
        NVCHK(it != _values.end(), "Point::get: attribute '{}' not found",
              name);
        return std::any_cast<const T&>(it->second);
    }

    // Set attribute value (only affects this copy)
    template <typename T> void set(const String& name, T&& value) {
        _values[name] = std::forward<T>(value);
    }

    // Check if attribute exists
    [[nodiscard]] auto has(const String& name) const -> bool;

    // Convenience accessors
    [[nodiscard]] auto position() const -> Vec3d;
    void set_position(const Vec3d& p);

    [[nodiscard]] auto rotation() const -> Vec3d;
    void set_rotation(const Vec3d& r);

    [[nodiscard]] auto scale() const -> Vec3d;
    void set_scale(const Vec3d& s);

    // Get all attribute names
    [[nodiscard]] auto get_attribute_names() const -> Vector<String>;

    // Apply this point's values back to a PCGPointRef
    void apply_to(PCGPointRef& ref) const;

  private:
    UnorderedMap<String, std::any> _values;

    void copy_attribute_value(const String& name, const PointAttribute& attr,
                              U64 idx);

    void apply_value_to_ref(const String& name, const std::any& value,
                            PCGPointRef& ref) const;
};
}; // namespace nv

#endif
