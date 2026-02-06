#include <nvk_gltf.h>

namespace nv {
auto GLTFNode::name() const -> const String& { return _name; }
void GLTFNode::set_name(String name) { _name = std::move(name); }
auto GLTFNode::parent_node() const -> const GLTFNode* { return _parentNode; }
auto GLTFNode::parent_node() -> GLTFNode* { return _parentNode; }
void GLTFNode::set_parent_node(GLTFNode* parent) { _parentNode = parent; }
auto GLTFNode::children_count() const -> U32 {
    return static_cast<U32>(_children.size());
}
auto GLTFNode::children() const -> const Vector<RefPtr<GLTFNode>>& {
    return _children;
}
auto GLTFNode::children() -> Vector<RefPtr<GLTFNode>>& { return _children; }
auto GLTFNode::get_child(U32 index) const -> const GLTFNode& {
    NVCHK(index < _children.size(), "Out of range child index: {}", index);
    return *_children[index];
}
auto GLTFNode::get_child(U32 index) -> GLTFNode& {
    NVCHK(index < _children.size(), "Out of range child index: {}", index);
    return *_children[index];
}
auto GLTFNode::add_child() -> GLTFNode& {
    auto& obj = _parent.add_node();
    obj.set_parent_node(this);
    _children.emplace_back(&obj);
    return obj;
}
void GLTFNode::add_child(RefPtr<GLTFNode> child) {
    _children.push_back(std::move(child));
}
void GLTFNode::clear_children() { _children.clear(); }
auto GLTFNode::has_skin() const -> bool { return _skin != nullptr; }
auto GLTFNode::skin() const -> const GLTFSkin& {
    NVCHK(_skin != nullptr, "Invalid skin.");
    return *_skin;
}
auto GLTFNode::skin() -> GLTFSkin& {
    NVCHK(_skin != nullptr, "Invalid skin.");
    return *_skin;
}
void GLTFNode::set_skin(GLTFSkin& skin) { _skin = &skin; }
void GLTFNode::clear_skin() { _skin = nullptr; }
auto GLTFNode::has_mesh() const -> bool { return _mesh != nullptr; }
auto GLTFNode::mesh() const -> const GLTFMesh& {
    NVCHK(_mesh != nullptr, "Invalid mesh.");
    return *_mesh;
}
auto GLTFNode::mesh() -> GLTFMesh& {
    NVCHK(_mesh != nullptr, "Invalid mesh.");
    return *_mesh;
}
void GLTFNode::set_mesh(GLTFMesh& mesh) { _mesh = &mesh; }
void GLTFNode::clear_mesh() { _mesh = nullptr; }
auto GLTFNode::has_camera() const -> bool { return _camera != nullptr; }
auto GLTFNode::camera() const -> const GLTFCamera& {
    NVCHK(_camera != nullptr, "Invalid camera.");
    return *_camera;
}
auto GLTFNode::camera() -> GLTFCamera& {
    NVCHK(_camera != nullptr, "Invalid camera.");
    return *_camera;
}
void GLTFNode::set_camera(GLTFCamera& camera) { _camera = &camera; }
void GLTFNode::clear_camera() { _camera = nullptr; }
auto GLTFNode::has_matrix() const -> bool { return _hasMatrix; }
auto GLTFNode::matrix() const -> const Mat4d& { return _matrix; }
void GLTFNode::set_matrix(const Mat4d& matrix) {
    _matrix = matrix;
    _hasMatrix = true;
}
void GLTFNode::clear_matrix() { _hasMatrix = false; }
auto GLTFNode::has_translation() const -> bool { return _hasTranslation; }
auto GLTFNode::translation() const -> const Vec3d& { return _translation; }
void GLTFNode::set_translation(const Vec3d& translation) {
    _translation = translation;
    _hasTranslation = true;
}
void GLTFNode::clear_translation() { _hasTranslation = false; }
auto GLTFNode::has_rotation() const -> bool { return _hasRotation; }
auto GLTFNode::rotation() const -> const Quatd& { return _rotation; }
void GLTFNode::set_rotation(const Quatd& rotation) {
    _rotation = rotation;
    _hasRotation = true;
}
void GLTFNode::clear_rotation() { _hasRotation = false; }
auto GLTFNode::has_scale() const -> bool { return _hasScale; }
auto GLTFNode::scale() const -> const Vec3d& { return _scale; }
void GLTFNode::set_scale(const Vec3d& scale) {
    _scale = scale;
    _hasScale = true;
}
void GLTFNode::clear_scale() { _hasScale = false; }

void GLTFNode::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("children") && desc["children"].is_array()) {
        const auto& children = desc["children"];
        _children.reserve(children.size());
        for (const auto& childIndex : children) {
            U32 idx = childIndex.get<U32>();
            _children.emplace_back(&_parent.get_node(idx));
        }
    }

    // if (desc.contains("skin") && desc["skin"].is_number_unsigned()) {
    //     U32 skinIndex = desc["skin"].get<U32>();
    //     _skin = _parent.get_skin(skinIndex);
    // }

    if (desc.contains("mesh") && desc["mesh"].is_number_unsigned()) {
        U32 meshIndex = desc["mesh"].get<U32>();
        _mesh = &_parent.get_mesh(meshIndex);
    }

    // if (desc.contains("camera") && desc["camera"].is_number_unsigned()) {
    //     U32 cameraIndex = desc["camera"].get<U32>();
    //     _camera = _parent.get_camera(cameraIndex);
    // }

    if (desc.contains("matrix") && desc["matrix"].is_array()) {
        const auto& matrix = desc["matrix"];
        NVCHK(matrix.size() == 16, "Invalid matrix size: {}", matrix.size());

        I32 i = 0;
        for (I32 c = 0; c < 4; ++c) {
            for (I32 r = 0; r < 4; ++r) {
                _matrix(r, c) = matrix[i++].get<F64>();
            }
        }
        _hasMatrix = true;
    }

    if (desc.contains("translation") && desc["translation"].is_array()) {
        const auto& translation = desc["translation"];
        NVCHK(translation.size() == 3, "Invalid translation size: {}",
              translation.size());
        _translation.x() = translation[0].get<F64>();
        _translation.y() = translation[1].get<F64>();
        _translation.z() = translation[2].get<F64>();
        _hasTranslation = true;
    }

    if (desc.contains("rotation") && desc["rotation"].is_array()) {
        const auto& rotation = desc["rotation"];
        NVCHK(rotation.size() == 4, "Invalid rotation size: {}",
              rotation.size());

        _rotation.x() = rotation[0].get<F64>();
        _rotation.y() = rotation[1].get<F64>();
        _rotation.z() = rotation[2].get<F64>();
        _rotation.w() = rotation[3].get<F64>();
        _hasRotation = true;
    }

    if (desc.contains("scale") && desc["scale"].is_array()) {
        const auto& scale = desc["scale"];
        NVCHK(scale.size() == 3, "Invalid scale size: {}", scale.size());

        _scale.x() = scale[0].get<F64>();
        _scale.y() = scale[1].get<F64>();
        _scale.z() = scale[2].get<F64>();
        _hasScale = true;
    }
}

auto GLTFNode::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (!_children.empty()) {
        Json children = Json::array();
        for (const auto& child : _children) {
            children.push_back(child->index());
        }
        json["children"] = children;
    }

    // if (_skin) {
    //     json["skin"] = _skin->index();
    // }

    if (_mesh) {
        json["mesh"] = _mesh->index();
    }

    // if (_camera) {
    //     json["camera"] = _camera->index();
    // }

    if (_hasMatrix) {
        Json matrix = Json::array();
        for (I32 c = 0; c < 4; ++c) {
            for (I32 r = 0; r < 4; ++r) {
                matrix.push_back(_matrix(r, c));
            }
        }
        json["matrix"] = matrix;
    }

    if (_hasTranslation) {
        json["translation"] =
            Json::array({_translation.x(), _translation.y(), _translation.z()});
    }

    if (_hasRotation) {
        json["rotation"] = Json::array(
            {_rotation.x(), _rotation.y(), _rotation.z(), _rotation.w()});
    }

    if (_hasScale) {
        json["scale"] = Json::array({_scale.x(), _scale.y(), _scale.z()});
    }

    return json;
}

} // namespace nv