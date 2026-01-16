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
} // namespace nv
