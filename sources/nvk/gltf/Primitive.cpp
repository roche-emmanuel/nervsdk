#include <nvk_gltf.h>

namespace nv {
auto GLTFPrimitive::index() const -> U32 { return _index; }
auto GLTFPrimitive::type() const -> GLTFPrimitiveType { return _type; }
void GLTFPrimitive::set_type(GLTFPrimitiveType type) { _type = type; }
auto GLTFPrimitive::has_material() const -> bool {
    return _material != nullptr;
}
auto GLTFPrimitive::material() const -> GLTFMaterial& { return *_material; }
void GLTFPrimitive::set_material(GLTFMaterial& material) {
    _material = &material;
}
void GLTFPrimitive::clear_material() { _material = nullptr; }
auto GLTFPrimitive::has_indices() const -> bool { return _indices != nullptr; }
auto GLTFPrimitive::indices() const -> GLTFAccessor& { return *_indices; }
void GLTFPrimitive::set_indices(GLTFAccessor& accessor) {
    _indices = &accessor;
}
void GLTFPrimitive::clear_indices() { _indices = nullptr; }
auto GLTFPrimitive::attributes_count() const -> U32 {
    return static_cast<U32>(_attributes.size());
}
auto GLTFPrimitive::attributes() const -> const AttribMap& {
    return _attributes;
}
auto GLTFPrimitive::attributes() -> AttribMap& { return _attributes; }
auto GLTFPrimitive::has_attribute(GLTFAttributeType atype) const -> bool {
    return _attributes.find(atype) != _attributes.end();
}
auto GLTFPrimitive::attribute(GLTFAttributeType atype) const -> GLTFAccessor& {
    return *_attributes.at(atype);
}
void GLTFPrimitive::set_attribute(GLTFAttributeType atype,
                                  GLTFAccessor& accessor) {
    _attributes[atype] = &accessor;
}
void GLTFPrimitive::remove_attribute(GLTFAttributeType atype) {
    _attributes.erase(atype);
}
void GLTFPrimitive::clear_attributes() { _attributes.clear(); }
void GLTFPrimitive::read(const Json& desc) {
    // Read primitive type (default is triangles)
    if (desc.contains("mode")) {
        _type = static_cast<GLTFPrimitiveType>(desc["mode"].get<U32>());
    } else {
        _type = GLTF_PRIM_TRIANGLES;
    }

    // Read material reference
    if (desc.contains("material")) {
        U32 materialIndex = desc["material"].get<U32>();
        _material = &_parent.get_material(materialIndex);
    }

    // Read indices reference
    if (desc.contains("indices")) {
        U32 indicesIndex = desc["indices"].get<U32>();
        _indices = &_parent.get_accessor(indicesIndex);
    }

    // Read attributes
    if (desc.contains("attributes") && desc["attributes"].is_object()) {
        for (const auto& [name, value] : desc["attributes"].items()) {
            U32 accessorIndex = value.get<U32>();
            auto atype = gltf::to_attribute_type(name);
            _attributes[atype] = &_parent.get_accessor(accessorIndex);
        }
    }

#if 0
    // Read morph targets
    if (desc.contains("targets") && desc["targets"].is_array()) {
        for (const auto& targetDesc : desc["targets"]) {
            AttribMap target;
            for (const auto& [name, value] : targetDesc.items()) {
                U32 accessorIndex = value.get<U32>();
                target[name] =
                    RefPtr<GLTFAccessor>(&_parent.accessor(accessorIndex));
            }
            _targets.push_back(std::move(target));
        }
    }
#endif
}
auto GLTFPrimitive::write() const -> Json {
    Json desc;

    // Write primitive type (only if not default triangles)
    if (_type != GLTF_PRIM_TRIANGLES) {
        desc["mode"] = static_cast<U32>(_type);
    }

    // Write material reference
    if (_material != nullptr) {
        desc["material"] = _material->index();
    }

    // Write indices reference
    if (_indices != nullptr) {
        desc["indices"] = _indices->index();
    }

    // Write attributes
    if (!_attributes.empty()) {
        Json attrs = Json::object();
        for (const auto& [atype, accessor] : _attributes) {
            auto name = gltf::to_string(atype);
            attrs[name] = accessor->index();
        }
        desc["attributes"] = attrs;
    }

#if 0
    // Write morph targets
    if (!_targets.empty()) {
        Json targets = Json::array();
        for (const auto& target : _targets) {
            Json targetJson = Json::object();
            for (const auto& [name, accessor] : target) {
                targetJson[name] = accessor->index();
            }
            targets.push_back(targetJson);
        }
        desc["targets"] = targets;
    }
#endif

    return desc;
}
auto GLTFPrimitive::add_material(String name) -> GLTFMaterial& {
    auto& mat = _parent.add_material(std::move(name));
    set_material(mat);
    return mat;
};
} // namespace nv