#include <nvk/gltf/Node.h>

namespace nv {
auto GLTFNode::name() const -> std::string_view {
    return _node->name ? std::string_view(_node->name) : std::string_view();
}
auto GLTFNode::has_matrix() const -> bool { return _node->has_matrix; }
auto GLTFNode::has_trs() const -> bool {
    return _node->has_translation || _node->has_rotation || _node->has_scale;
}
auto GLTFNode::matrix() const -> std::span<const float, 16> {
    return std::span<const float, 16>(_node->matrix, 16);
}
void GLTFNode::set_matrix(const float matrix[16]) {
    std::copy(matrix, matrix + 16, _node->matrix);
    _node->has_matrix = 1;
    _node->has_translation = _node->has_rotation = _node->has_scale = 0;
}
auto GLTFNode::translation() const -> std::span<const float, 3> {
    return std::span<const float, 3>(_node->translation, 3);
}
auto GLTFNode::rotation() const -> std::span<const float, 4> {
    return std::span<const float, 4>(_node->rotation, 4);
}
auto GLTFNode::scale() const -> std::span<const float, 3> {
    return std::span<const float, 3>(_node->scale, 3);
}
void GLTFNode::set_translation(float x, float y, float z) {
    _node->translation[0] = x;
    _node->translation[1] = y;
    _node->translation[2] = z;
    _node->has_translation = 1;
    _node->has_matrix = 0;
}
void GLTFNode::set_rotation(float x, float y, float z, float w) {
    _node->rotation[0] = x;
    _node->rotation[1] = y;
    _node->rotation[2] = z;
    _node->rotation[3] = w;
    _node->has_rotation = 1;
    _node->has_matrix = 0;
}
void GLTFNode::set_scale(float x, float y, float z) {
    _node->scale[0] = x;
    _node->scale[1] = y;
    _node->scale[2] = z;
    _node->has_scale = 1;
    _node->has_matrix = 0;
}
auto GLTFNode::handle() -> cgltf_node* { return _node; }
auto GLTFNode::handle() const -> const cgltf_node* { return _node; }
} // namespace nv