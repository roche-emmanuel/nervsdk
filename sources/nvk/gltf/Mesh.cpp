#include <nvk_gltf.h>

namespace nv {
GLTFMesh::GLTFMesh(GLTFAsset& parent, U32 index) : GLTFElement(parent, index) {}
auto GLTFMesh::name() const -> const String& { return _name; }
void GLTFMesh::set_name(String name) { _name = std::move(name); }
auto GLTFMesh::primitives_count() const -> U32 {
    return static_cast<U32>(_primitives.size());
}
auto GLTFMesh::primitives() const -> const Vector<RefPtr<GLTFPrimitive>>& {
    return _primitives;
}
auto GLTFMesh::primitives() -> Vector<RefPtr<GLTFPrimitive>>& {
    return _primitives;
}
auto GLTFMesh::get_primitive(U32 index) const -> const GLTFPrimitive& {
    NVCHK(index < _primitives.size(), "Out of range primitive index {}", index);
    return *_primitives[index];
}
auto GLTFMesh::get_primitive(U32 index) -> GLTFPrimitive& {
    NVCHK(index < _primitives.size(), "Out of range primitive index {}", index);
    return *_primitives[index];
}
auto GLTFMesh::add_primitive(GLTFPrimitiveType ptype) -> GLTFPrimitive& {
    auto obj = nv::create<GLTFPrimitive>(_parent, *this, _primitives.size());
    obj->set_type(ptype);
    _primitives.emplace_back(obj);
    return *obj;
}
void GLTFMesh::clear_primitives() { _primitives.clear(); }
auto GLTFMesh::weights_count() const -> U32 {
    return static_cast<U32>(_weights.size());
}
auto GLTFMesh::weights() const -> const F32Vector& { return _weights; }
auto GLTFMesh::weights() -> F32Vector& { return _weights; }
void GLTFMesh::set_weights(F32Vector weights) { _weights = std::move(weights); }
void GLTFMesh::clear_weights() { _weights.clear(); }
void GLTFMesh::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("primitives") && desc["primitives"].is_array()) {
        const auto& prims = desc["primitives"];
        _primitives.reserve(prims.size());
        for (const auto& prim : prims) {
            auto& primitive = add_primitive();
            primitive.read(prim);
        }
    }

    if (desc.contains("weights") && desc["weights"].is_array()) {
        const auto& weights = desc["weights"];
        _weights.reserve(weights.size());
        for (const auto& weight : weights) {
            _weights.push_back(weight.get<F32>());
        }
    }
}
auto GLTFMesh::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (!_primitives.empty()) {
        Json prims = Json::array();
        for (const auto& primitive : _primitives) {
            prims.push_back(primitive->write());
        }
        json["primitives"] = prims;
    }

    if (!_weights.empty()) {
        Json weights = Json::array();
        for (F32 weight : _weights) {
            weights.push_back(weight);
        }
        json["weights"] = weights;
    }

    return json;
}
} // namespace nv