#ifndef _GLTF_SKIN_H_
#define _GLTF_SKIN_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFSkin {
    friend class GLTFAsset;
    cgltf_skin* _skin;

    explicit GLTFSkin(cgltf_skin* s) : _skin(s) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto joint_indices() const -> std::vector<size_t>;
    void add_joint(size_t node_index);

    [[nodiscard]] auto skeleton_index() const -> std::optional<size_t>;
    void set_skeleton(size_t node_index);

    [[nodiscard]] auto inverse_bind_matrices_index() const
        -> std::optional<size_t>;
    void set_inverse_bind_matrices(size_t accessor_index);

    auto handle() -> cgltf_skin*;
    [[nodiscard]] auto handle() const -> const cgltf_skin*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_