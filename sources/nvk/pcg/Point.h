#ifndef _NV_PCG_POINT_H_
#define _NV_PCG_POINT_H_

#include <nvk/pcg/PointArray.h>

namespace nv {

// Forward declarations
class PCGPoint;
class PCGPointRef;

// Trait to determine if a type supports weighted averaging
template <typename T> struct WeightedAverageTraits {
    static constexpr bool supported = false;
    using AccumType = T;
    static auto accumulate(const T& a, F64 wa) -> T { return T{}; }
    static auto divide(const T& sum, F64 total_weight) -> T { return T{}; }
};

// Specialization for scalar types
template <> struct WeightedAverageTraits<F32> {
    static constexpr bool supported = true;
    using AccumType = F64;
    static auto accumulate(const F32& a, F64 wa) -> F64 { return F64(a) * wa; }
    static auto divide(const F64& sum, F64 total_weight) -> F32 {
        return static_cast<F32>(sum / total_weight);
    }
};

template <> struct WeightedAverageTraits<F64> {
    static constexpr bool supported = true;
    using AccumType = F64;
    static auto accumulate(const F64& a, F64 wa) -> F64 { return a * wa; }
    static auto divide(const F64& sum, F64 total_weight) -> F64 {
        return sum / total_weight;
    }
};

template <> struct WeightedAverageTraits<I32> {
    static constexpr bool supported = true;
    using AccumType = F64;
    static auto accumulate(const I32& a, F64 wa) -> F64 { return F64(a) * wa; }
    static auto divide(const F64& sum, F64 total_weight) -> I32 {
        return static_cast<I32>(std::round(sum / total_weight));
    }
};

template <> struct WeightedAverageTraits<I64> {
    static constexpr bool supported = true;
    using AccumType = F64;
    static auto accumulate(const I64& a, F64 wa) -> F64 { return F64(a) * wa; }
    static auto divide(const F64& sum, F64 total_weight) -> I64 {
        return static_cast<I64>(std::round(sum / total_weight));
    }
};

// Specialization for vector types
template <> struct WeightedAverageTraits<Vec2d> {
    static constexpr bool supported = true;
    using AccumType = Vec2d;
    static auto accumulate(const Vec2d& a, F64 wa) -> Vec2d { return a * wa; }
    static auto divide(const Vec2d& sum, F64 total_weight) -> Vec2d {
        return sum / total_weight;
    }
};

template <> struct WeightedAverageTraits<Vec3d> {
    static constexpr bool supported = true;
    using AccumType = Vec3d;
    static auto accumulate(const Vec3d& a, F64 wa) -> Vec3d { return a * wa; }
    static auto divide(const Vec3d& sum, F64 total_weight) -> Vec3d {
        return sum / total_weight;
    }
};

template <> struct WeightedAverageTraits<Vec4d> {
    static constexpr bool supported = true;
    using AccumType = Vec4d;
    static auto accumulate(const Vec4d& a, F64 wa) -> Vec4d { return a * wa; }
    static auto divide(const Vec4d& sum, F64 total_weight) -> Vec4d {
        return sum / total_weight;
    }
};

struct WeightedPoint;

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

    // Compute weighted average from multiple points
    void set_weighted_average(const Vector<WeightedPoint>& weighted_points,
                              const UnorderedSet<String>& skip_attributes = {});

  private:
    PointArray* _array;
    U64 _index;

    template <typename T>
    void compute_weighted_average_for_attribute(
        const String& attr_name, const Vector<WeightedPoint>& weighted_points);
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
    template <typename T> void set(const String& name, const T& value) {
        _values[name] = value;
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

    // Compute weighted average from multiple points
    void set_weighted_average(const Vector<WeightedPoint>& weighted_points,
                              const UnorderedSet<String>& skip_attributes = {});

  private:
    UnorderedMap<String, std::any> _values;

    void copy_attribute_value(const String& name, const PointAttribute& attr,
                              U64 idx);

    void apply_value_to_ref(const String& name, const std::any& value,
                            PCGPointRef& ref) const;

    template <typename T>
    void compute_weighted_average_for_attribute(
        const String& attr_name, const Vector<WeightedPoint>& weighted_points);
};

// Weighted point structure
struct WeightedPoint {
    std::variant<PCGPoint, PCGPointRef> point;
    F64 weight;

    WeightedPoint(const PCGPoint& p, F64 w) : point(p), weight(w) {}
    WeightedPoint(const PCGPointRef& p, F64 w) : point(p), weight(w) {}
    WeightedPoint(PCGPoint&& p, F64 w) : point(std::move(p)), weight(w) {}
};

// Helper function to get attribute value from variant point
template <typename T>
auto get_point_attribute(const std::variant<PCGPoint, PCGPointRef>& point,
                         const String& name) -> T {
    if (std::holds_alternative<PCGPoint>(point)) {
        return std::get<PCGPoint>(point).get<T>(name);
    }

    return std::get<PCGPointRef>(point).get<T>(name);
}

// Template implementation for PCGPointRef
template <typename T>
void PCGPointRef::compute_weighted_average_for_attribute(
    const String& attr_name, const Vector<WeightedPoint>& weighted_points) {

    if (!WeightedAverageTraits<T>::supported) {
        return; // Skip unsupported types
    }

    if (weighted_points.empty()) {
        return;
    }

    using AccumType = typename WeightedAverageTraits<T>::AccumType;

    // Accumulate weighted values
    AccumType accumulated{};
    F64 total_weight = 0.0;

    for (const auto& wp : weighted_points) {
        T value = get_point_attribute<T>(wp.point, attr_name);
        accumulated += WeightedAverageTraits<T>::accumulate(value, wp.weight);
        total_weight += wp.weight;
    }

    if (total_weight > 0.0) {
        T result = WeightedAverageTraits<T>::divide(accumulated, total_weight);
        set<T>(attr_name, result);
    }
}

// Template implementation for PCGPoint
template <typename T>
void PCGPoint::compute_weighted_average_for_attribute(
    const String& attr_name, const Vector<WeightedPoint>& weighted_points) {

    if (!WeightedAverageTraits<T>::supported) {
        return; // Skip unsupported types
    }

    if (weighted_points.empty()) {
        return;
    }

    using AccumType = typename WeightedAverageTraits<T>::AccumType;

    // Accumulate weighted values
    AccumType accumulated{};
    F64 total_weight = 0.0;

    for (const auto& wp : weighted_points) {
        T value = get_point_attribute<T>(wp.point, attr_name);
        accumulated += WeightedAverageTraits<T>::accumulate(value, wp.weight);
        total_weight += wp.weight;
    }

    T result = WeightedAverageTraits<T>::divide(
        accumulated, total_weight == 0.0 ? 1.0 : total_weight);
    set<T>(attr_name, result);
}

} // namespace nv

#endif