#include "nvk/geometry/geometry2d.h"
#include <nvk/pcg/Point.h>
#include <nvk/pcg/PointArray.h>

namespace nv {
PointArray::PointArray(Traits traits) : _traits(std::move(traits)) {};

PointArray::~PointArray() = default;

auto PointArray::create(I32 numPoints, Traits traits) -> RefPtr<PointArray> {
    auto arr = nv::create<PointArray>(std::move(traits));
    if (numPoints >= 0) {
        arr->resize(numPoints);
    }
    return arr;
}

PointArray::PointArray(const PointAttributeVector& attribs, Traits traits)
    : _traits(std::move(traits)) {
    for (const auto& attr : attribs) {
        add_attribute(attr);
        // auto res = _attributes.insert(std::make_pair(attr->name(), attr));
        // NVCHK(res.second, "Could not insert attribute {} in PointArray.",
        //       attr->name());
    }
    validate_attributes();
};

void PointArray::validate_attributes() {
    if (_attributes.size() <= 1) {
        // Nothing to validate in this case:
        return;
    }

    // Check that all attributes have the same length:
    U32 size = _attributes.begin()->second->size();
    for (const auto& it : _attributes) {
        U32 size2 = it.second->size();
        if (size != size2) {
            THROW_MSG("Mismatch in attribute {} num points: {} != {}", it.first,
                      size2, size);
        }
    }
};

auto PointArray::create(const PointAttributeVector& attribs, Traits traits)
    -> RefPtr<PointArray> {
    return nv::create<PointArray>(attribs, std::move(traits));
}

auto PointArray::get_num_attributes() const -> U32 {
    return _attributes.size();
}

auto PointArray::get_scale_attribute() const -> const PointAttribute& {
    return get_attribute(pt_scale_attr);
}
auto PointArray::get_rotation_attribute() const -> const PointAttribute& {
    return get_attribute(pt_rotation_attr);
}
auto PointArray::get_position_attribute() const -> const PointAttribute& {
    return get_attribute(pt_position_attr);
}
auto PointArray::get_attribute(const String& name) const
    -> const PointAttribute& {
    const auto* attrib = find_attribute(name);
    NVCHK(attrib != nullptr, "Invalid attribute with name {}", name);
    return *attrib;
}
auto PointArray::get_attribute(const String& name) -> PointAttribute& {
    auto* attrib = find_attribute(name);
    NVCHK(attrib != nullptr, "Invalid attribute with name {}", name);
    return *attrib;
}
auto PointArray::find_attribute(const String& name) const
    -> const PointAttribute* {
    auto it = _attributes.find(name);
    if (it != _attributes.end()) {
        return it->second.get();
    }

    return nullptr;
}
auto PointArray::find_attribute(const String& name) -> PointAttribute* {
    auto it = _attributes.find(name);
    if (it != _attributes.end()) {
        return it->second.get();
    }

    return nullptr;
}
auto PointArray::get_num_points() const -> U32 {
    return _numPoints < 0 ? 0 : _numPoints;
}

auto PointArray::get_num_segments() const -> U32 {
    U32 nPoints = get_num_points();
    if (nPoints <= 1)
        return 0;

    return _traits.closedLoop ? nPoints : nPoints - 1;
}

void PointArray::add_attribute(RefPtr<PointAttribute> attr) {
    NVCHK(attr != nullptr, "Invalid attribute.");

    if (_numPoints >= 0 && attr->size() != _numPoints) {
        THROW_MSG("Attribute size doesn't match num points: {} != {}",
                  attr->size(), _numPoints);
        // logWARN("Resizing attribute to PointArray size {}", _numPoints);
        // attr->resize(_numPoints);
    }
    _numPoints = (I32)attr->size();

    auto res =
        _attributes.insert(std::make_pair(attr->name(), std::move(attr)));
    NVCHK(res.second, "Attribute {} was already inserted in PointArray.",
          attr->name());
}

auto PointArray::create(const Vector<AttribDesc>& attribs, U32 numPoints,
                        Traits traits) -> RefPtr<PointArray> {
    auto arr = create((I32)numPoints, traits);
    arr->add_attributes(attribs);

    return arr;
};
auto PointArray::get_attributes() const -> const PointAttributeMap& {
    return _attributes;
}

void PointArray::add_std_attributes() {
    static std::vector<AttribDesc> stdAttribs = {
        {.name = pt_position_attr, .type = DTYPE_VEC3D},
        {.name = pt_rotation_attr, .type = DTYPE_VEC3D},
        {.name = pt_scale_attr, .type = DTYPE_VEC3D},
        {.name = pt_boundsmin_attr, .type = DTYPE_VEC3D},
        {.name = pt_boundsmax_attr, .type = DTYPE_VEC3D},
        {.name = pt_color_attr, .type = DTYPE_VEC4D},
        {.name = pt_density_attr, .type = DTYPE_F32},
        {.name = pt_steepness_attr, .type = DTYPE_F32},
        {.name = pt_seed_attr, .type = DTYPE_I32},
    };

    add_attributes(stdAttribs);
};

void PointArray::add_attributes(const Vector<AttribDesc>& attribs) {
    for (const auto& adesc : attribs) {
        switch (adesc.type) {
        case DTYPE_BOOL:
            add_attribute<bool>(adesc.name);
            break;
        case DTYPE_I32:
            add_attribute<I32>(adesc.name);
            break;
        case DTYPE_F32:
            add_attribute<F32>(adesc.name);
            break;
        case DTYPE_F64:
            add_attribute<F64>(adesc.name);
            break;
        case DTYPE_VEC2F:
            add_attribute<Vec2f>(adesc.name);
            break;
        case DTYPE_VEC3F:
            add_attribute<Vec3f>(adesc.name);
            break;
        case DTYPE_VEC4F:
            add_attribute<Vec4f>(adesc.name);
            break;
        case DTYPE_VEC2D:
            add_attribute<Vec2d>(adesc.name);
            break;
        case DTYPE_VEC3D:
            add_attribute<Vec3d>(adesc.name);
            break;
        case DTYPE_VEC4D:
            add_attribute<Vec4d>(adesc.name);
            break;
        case DTYPE_MAT4F:
            add_attribute<Mat4f>(adesc.name);
            break;
        case DTYPE_MAT4D:
            add_attribute<Mat4d>(adesc.name);
            break;
        default:
            THROW_MSG("Unsupported PointArray attribute type: {}", adesc.type);
        }
    }
};

void PointArray::randomize_all_attributes(UnorderedMap<String, Box4d> ranges) {
    for (auto& it : _attributes) {
        auto it2 = ranges.find(it.first);
        if (it2 == ranges.end()) {
            it.second->randomize();
        } else {
            it.second->randomize_values(it2->second);
        }
    }
}

auto PointArray::clone() const -> RefPtr<PointArray> {
    auto arr = create(_numPoints, _traits);

    for (const auto& it : _attributes) {
        arr->add_attribute(it.second->clone());
    }

    return arr;
};

void PointArray::collect_attribute_types(PointAttributeTypeMap& atypes) {
    for (const auto& it : _attributes) {
        auto it2 = atypes.find(it.first);
        auto atype = it.second->get_type_id();

        if (it2 == atypes.end()) {
            // Add a new entry for this attribute:
            auto res = atypes.insert(std::make_pair(it.first, atype));
            NVCHK(res.second, "Cannot insert attribute {}", it.first);
        } else {
            // Check that we have the same type:
            NVCHK(it2->second == it.second->get_type_id(),
                  "collect_attribute_types: attribute type mismatch {} != {}",
                  it2->second, atype);
        }
    }
};

auto PointArray::collect_all_attribute_types(
    const Vector<RefPtr<PointArray>>& arrays) -> Vector<AttribDesc> {
    PointAttributeTypeMap atypes;

    for (const auto& arr : arrays) {
        arr->collect_attribute_types(atypes);
    }

    Vector<PointArray::AttribDesc> adescs;
    for (const auto& it : atypes) {
        adescs.emplace_back(
            PointArray::AttribDesc{.name = it.first, .type = it.second});
    }

    return adescs;
};

void PointArray::set_point(I64 index, const PCGPoint& point) {
    auto ref = get_point(index);
    point.apply_to(ref);
}

auto PointArray::copy_point(I64 index) const -> PCGPoint {
    return PCGPoint(get_point(index));
}

auto PointArray::get_point(I64 index) const -> PCGPointRef {
    if (index < 0) {
        index += get_num_points();
    }
    NVCHK(0 <= index && index < get_num_points(),
          "PointArray::get_point: index {} out of bounds", index);
    return {const_cast<PointArray*>(this), U64(index)};
}

auto PointArray::get_point(I64 index) -> PCGPointRef {
    if (index < 0) {
        index += get_num_points();
    }
    NVCHK(0 <= index && index < get_num_points(),
          "PointArray::get_point: index {} out of bounds", index);
    return {this, U64(index)};
}
auto PointArray::get_attribute_names() const -> Vector<String> {
    StringVector res;
    for (const auto& it : _attributes) {
        res.emplace_back(it.first);
    }
    return res;
};

auto PointArray::has_attribute(const String& name) const -> bool {
    return _attributes.contains(name);
};

auto PointArray::get_seg_start_point(I64 segId) -> PCGPointRef {
    if (segId < 0) {
        segId += get_num_segments();
    }

    NVCHK(0 <= segId && segId < get_num_segments(),
          "PointArray::get_seg_start_point: segId {} out of bounds", segId);
    return get_point(segId);
};

auto PointArray::get_seg_end_point(I64 segId) -> PCGPointRef {
    if (segId < 0) {
        segId += get_num_segments();
    }

    NVCHK(0 <= segId && segId < get_num_segments(),
          "PointArray::get_seg_end_point: segId {} out of bounds", segId);

    auto ptId =
        (is_closed_loop() && segId == (get_num_segments() - 1)) ? 0 : segId + 1;

    return get_point(ptId);
};

auto PointArray::get_tags() const -> const Set<String>& { return _tags; }
void PointArray::add_tags(const Set<String>& tags) {
    _tags.insert(tags.begin(), tags.end());
}

auto PointArray::add_tag(const String& tag) -> bool {
    return _tags.insert(tag).second;
}

void PointArray::add_point(const PCGPoint& pt) {
    auto ref = add_point();
    pt.apply_to(ref);
};

void PointArray::add_point(const PCGPointRef& pt) {
    auto ref = add_point();
    pt.copy().apply_to(ref);
};

auto PointArray::add_point() -> PCGPointRef {
    U32 num = get_num_points() + 1;
    resize(num);
    return get_point(-1);
};

auto PointArray::create_like(const RefPtr<PointArray>& array, I32 numPoints)
    -> RefPtr<PointArray> {

    auto res = PointArray::create(numPoints, array->_traits);
    auto adescs = collect_all_attribute_types({array});

    res->add_attributes(adescs);
    return res;
};

void PointArray::set_point(I64 index, const PCGPointRef& point) {
    auto ref = get_point(index);
    point.copy().apply_to(ref);
};

auto PointArray::compute_area() const -> F64 {
    if (!is_closed_loop()) {
        return 0.0;
    }

    const auto* positions = find<Vec3d>(pt_position_attr);
    return polygon_signed_area_xy(positions->data(), positions->size());
};

} // namespace nv
