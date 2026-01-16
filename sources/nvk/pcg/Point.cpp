#include <nvk/pcg/Point.h>

namespace nv {

auto PCGPointRef::copy() const -> PCGPoint { return PCGPoint(*this); }

auto PCGPointRef::position() const -> Vec3d {
    return get<Vec3d>(pt_position_attr);
}
void PCGPointRef::set_position(const Vec3d& p) { set(pt_position_attr, p); }
auto PCGPointRef::rotation() const -> Vec3d {
    return get<Vec3d>(pt_rotation_attr);
}
void PCGPointRef::set_rotation(const Vec3d& r) { set(pt_rotation_attr, r); }
auto PCGPointRef::scale() const -> Vec3d { return get<Vec3d>(pt_scale_attr); }
void PCGPointRef::set_scale(const Vec3d& s) { set(pt_scale_attr, s); }
auto PCGPointRef::index() const -> U64 { return _index; }
auto PCGPointRef::array() const -> const PointArray* { return _array; }
auto PCGPointRef::array() -> PointArray* { return _array; }
PCGPoint::PCGPoint(const PCGPointRef& ref) {
    const auto& attribs = ref.array()->get_attributes();
    for (const auto& [name, attr] : attribs) {
        copy_attribute_value(name, *attr, ref.index());
    }
}
auto PCGPoint::has(const String& name) const -> bool {
    return _values.find(name) != _values.end();
}
auto PCGPoint::position() const -> Vec3d {
    return get<Vec3d>(pt_position_attr);
}
void PCGPoint::set_position(const Vec3d& p) { set(pt_position_attr, p); }
auto PCGPoint::rotation() const -> Vec3d {
    return get<Vec3d>(pt_rotation_attr);
}
void PCGPoint::set_rotation(const Vec3d& r) { set(pt_rotation_attr, r); }
auto PCGPoint::scale() const -> Vec3d { return get<Vec3d>(pt_scale_attr); }
void PCGPoint::set_scale(const Vec3d& s) { set(pt_scale_attr, s); }
auto PCGPoint::get_attribute_names() const -> Vector<String> {
    Vector<String> names;
    names.reserve(_values.size());
    for (const auto& [name, _] : _values) {
        names.push_back(name);
    }
    return names;
}
void PCGPoint::apply_to(PCGPointRef& ref) const {
    for (const auto& [name, value] : _values) {
        apply_value_to_ref(name, value, ref);
    }
}
void PCGPoint::copy_attribute_value(const String& name,
                                    const PointAttribute& attr, U64 idx) {
    StringID typeId = attr.get_type_id();

    // Handle common types
    if (typeId == TypeId<I32>::id) {
        _values[name] = attr.get_value<I32>(idx);
    } else if (typeId == TypeId<I64>::id) {
        _values[name] = attr.get_value<I64>(idx);
    } else if (typeId == TypeId<F32>::id) {
        _values[name] = attr.get_value<F32>(idx);
    } else if (typeId == TypeId<F64>::id) {
        _values[name] = attr.get_value<F64>(idx);
    } else if (typeId == TypeId<Vec2d>::id) {
        _values[name] = attr.get_value<Vec2d>(idx);
    } else if (typeId == TypeId<Vec3d>::id) {
        _values[name] = attr.get_value<Vec3d>(idx);
    } else if (typeId == TypeId<Vec4d>::id) {
        _values[name] = attr.get_value<Vec4d>(idx);
    } else {
        NVCHK(false,
              "copy_attribute_value: unsupported type for attribute '{}'",
              name);
    }
}
void PCGPoint::apply_value_to_ref(const String& name, const std::any& value,
                                  PCGPointRef& ref) const {
    const auto& type = value.type();

    if (type == typeid(I32)) {
        ref.set(name, std::any_cast<I32>(value));
    } else if (type == typeid(I64)) {
        ref.set(name, std::any_cast<I64>(value));
    } else if (type == typeid(F32)) {
        ref.set(name, std::any_cast<F32>(value));
    } else if (type == typeid(F64)) {
        ref.set(name, std::any_cast<F64>(value));
    } else if (type == typeid(Vec2d)) {
        ref.set(name, std::any_cast<Vec2d>(value));
    } else if (type == typeid(Vec3d)) {
        ref.set(name, std::any_cast<Vec3d>(value));
    } else if (type == typeid(Vec4d)) {
        ref.set(name, std::any_cast<Vec4d>(value));
    } else {
        THROW_MSG("apply_value_to_ref: Unsupported any type {}", type.name());
    }
}

void PCGPointRef::set_weighted_average(
    const Vector<WeightedPoint>& weighted_points,
    const UnorderedSet<String>& skip_attributes) {

    if (weighted_points.empty()) {
        return;
    }

    // Get the list of attributes from the array
    const auto& attributes = _array->get_attributes();

    for (const auto& [attr_name, attr] : attributes) {
        // Skip if in skip list
        if (skip_attributes.contains(attr_name)) {
            continue;
        }

        StringID type_id = attr->get_type_id();

        // Dispatch to appropriate typed version based on attribute type
        if (type_id == TypeId<I32>::id) {
            compute_weighted_average_for_attribute<I32>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<I64>::id) {
            compute_weighted_average_for_attribute<I64>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<F32>::id) {
            compute_weighted_average_for_attribute<F32>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<F64>::id) {
            compute_weighted_average_for_attribute<F64>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<Vec2d>::id) {
            compute_weighted_average_for_attribute<Vec2d>(attr_name,
                                                          weighted_points);
        } else if (type_id == TypeId<Vec3d>::id) {
            compute_weighted_average_for_attribute<Vec3d>(attr_name,
                                                          weighted_points);
        } else if (type_id == TypeId<Vec4d>::id) {
            compute_weighted_average_for_attribute<Vec4d>(attr_name,
                                                          weighted_points);
        }
        // Unsupported types are silently skipped
    }
}

// PCGPoint implementation
void PCGPoint::set_weighted_average(
    const Vector<WeightedPoint>& weighted_points,
    const UnorderedSet<String>& skip_attributes) {

    if (weighted_points.empty()) {
        return;
    }

    // Get attribute names from the first point
    Vector<String> attr_names;
    if (std::holds_alternative<PCGPoint>(weighted_points[0].point)) {
        attr_names =
            std::get<PCGPoint>(weighted_points[0].point).get_attribute_names();
    } else {
        attr_names = std::get<PCGPointRef>(weighted_points[0].point)
                         .array()
                         ->get_attribute_names();
    }

    for (const auto& attr_name : attr_names) {
        // Skip if in skip list
        if (skip_attributes.contains(attr_name)) {
            continue;
        }

        // Get type from first point's attribute
        StringID type_id{0};

        if (std::holds_alternative<PCGPoint>(weighted_points[0].point)) {
            const auto& first_point =
                std::get<PCGPoint>(weighted_points[0].point);
            auto it = first_point._values.find(attr_name);
            if (it == first_point._values.end()) {
                THROW_MSG("set_weighted_average: No attribute with name {} in "
                          "input point.",
                          attr_name);
            }

            const auto& type = it->second.type();
            if (type == typeid(I32))
                type_id = TypeId<I32>::id;
            else if (type == typeid(I64))
                type_id = TypeId<I64>::id;
            else if (type == typeid(F32))
                type_id = TypeId<F32>::id;
            else if (type == typeid(F64))
                type_id = TypeId<F64>::id;
            else if (type == typeid(Vec2d))
                type_id = TypeId<Vec2d>::id;
            else if (type == typeid(Vec3d))
                type_id = TypeId<Vec3d>::id;
            else if (type == typeid(Vec4d))
                type_id = TypeId<Vec4d>::id;
            else {
                THROW_MSG("set_weighted_average: Unsupported type {}",
                          type.name());
            }
        } else {
            const auto& ref = std::get<PCGPointRef>(weighted_points[0].point);
            const auto* attr = ref.array()->find_attribute(attr_name);
            if (attr == nullptr) {
                THROW_MSG(
                    "set_weighted_average: Invalid attribute with name {}",
                    attr_name);
            }

            type_id = attr->get_type_id();
        }

        // Dispatch based on type
        if (type_id == TypeId<I32>::id) {
            compute_weighted_average_for_attribute<I32>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<I64>::id) {
            compute_weighted_average_for_attribute<I64>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<F32>::id) {
            compute_weighted_average_for_attribute<F32>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<F64>::id) {
            compute_weighted_average_for_attribute<F64>(attr_name,
                                                        weighted_points);
        } else if (type_id == TypeId<Vec2d>::id) {
            compute_weighted_average_for_attribute<Vec2d>(attr_name,
                                                          weighted_points);
        } else if (type_id == TypeId<Vec3d>::id) {
            compute_weighted_average_for_attribute<Vec3d>(attr_name,
                                                          weighted_points);
        } else if (type_id == TypeId<Vec4d>::id) {
            compute_weighted_average_for_attribute<Vec4d>(attr_name,
                                                          weighted_points);
        }
    }
}

auto PCGPoint::mix_from(const PCGVariantPoint& pt0, const PCGVariantPoint& pt1,
                        F64 ratio) -> PCGPoint& {
    set_weighted_average({{pt0, 1.0 - ratio}, {pt1, ratio}});
    return *this;
};

auto PCGPointRef::mix_from(const PCGVariantPoint& pt0,
                           const PCGVariantPoint& pt1, F64 ratio)
    -> PCGPointRef& {
    set_weighted_average({{pt0, 1.0 - ratio}, {pt1, ratio}});
    return *this;
};

auto PCGPoint::mix(const PCGVariantPoint& pt0, const PCGVariantPoint& pt1,
                   F64 ratio) -> PCGPoint {
    PCGPoint pt;
    pt.set_weighted_average({{pt0, 1.0 - ratio}, {pt1, ratio}});
    return pt;
};

auto WeightedPoint::has_attribute(const String& aname) const -> bool {

    if (std::holds_alternative<PCGPoint>(point)) {
        return std::get<PCGPoint>(point).has(aname);
    }

    return std::get<PCGPointRef>(point).array()->has_attribute(aname);
};

} // namespace nv
