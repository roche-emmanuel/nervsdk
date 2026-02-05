#ifndef _GLTF_NODE_H_
#define _GLTF_NODE_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFNode {
    friend class GLTFAsset;
    cgltf_node* _node;

    explicit GLTFNode(cgltf_node* n) : _node(n) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    // Transform
    [[nodiscard]] auto has_matrix() const -> bool;
    [[nodiscard]] auto has_trs() const -> bool;

    [[nodiscard]] auto matrix() const -> std::span<const float, 16>;

    void set_matrix(const float matrix[16]);

    [[nodiscard]] auto translation() const -> std::span<const float, 3>;

    [[nodiscard]] auto rotation() const -> std::span<const float, 4>;

    [[nodiscard]] auto scale() const -> std::span<const float, 3>;

    void set_translation(float x, float y, float z);

    void set_rotation(float x, float y, float z, float w);

    void set_scale(float x, float y, float z);

    // Hierarchy
    [[nodiscard]] auto child_indices() const -> std::vector<size_t>;
    void add_child(size_t node_index);
    [[nodiscard]] auto parent_index() const -> std::optional<size_t>;

    // Attachments
    [[nodiscard]] auto mesh_index() const -> std::optional<size_t>;
    void set_mesh(size_t mesh_index);

    [[nodiscard]] auto skin_index() const -> std::optional<size_t>;
    void set_skin(size_t skin_index);

    [[nodiscard]] auto camera_index() const -> std::optional<size_t>;
    void set_camera(size_t camera_index);

    auto handle() -> cgltf_node*;
    [[nodiscard]] auto handle() const -> const cgltf_node*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_