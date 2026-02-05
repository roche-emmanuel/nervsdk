#include <nvk/gltf/Mesh.h>

namespace nv {
auto GLTFMesh::name() const -> std::string_view {
    return _mesh->name ? std::string_view(_mesh->name) : std::string_view();
}
auto GLTFMesh::primitive_count() const -> size_t {
    return _mesh->primitives_count;
}
auto GLTFMesh::weights() const -> std::span<const float> {
    return {_mesh->weights, _mesh->weights_count};
}
auto GLTFMesh::handle() const -> const cgltf_mesh* { return _mesh; }
auto GLTFMesh::handle() -> cgltf_mesh* { return _mesh; }
} // namespace nv