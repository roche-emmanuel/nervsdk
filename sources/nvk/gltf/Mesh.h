#ifndef _GLTF_MESH_H_
#define _GLTF_MESH_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFMesh {
    friend class GLTFAsset;
    cgltf_mesh* _mesh;

    explicit GLTFMesh(cgltf_mesh* m) : _mesh(m) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;

    void set_name(std::string_view name);

    [[nodiscard]] auto primitives() const -> std::span<const GLTFPrimitive>;
    auto primitives() -> std::span<GLTFPrimitive>;

    auto add_primitive() -> GLTFPrimitive&;

    [[nodiscard]] auto primitive_count() const -> size_t;

    // Weights for morph targets
    [[nodiscard]] auto weights() const -> std::span<const float>;

    auto handle() -> cgltf_mesh*;
    [[nodiscard]] auto handle() const -> const cgltf_mesh*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_