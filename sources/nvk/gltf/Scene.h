#ifndef _GLTF_SCENE_H_
#define _GLTF_SCENE_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {
class GLTFScene {
    friend class GLTFAsset;
    cgltf_scene* _scene;

    explicit GLTFScene(cgltf_scene* s) : _scene(s) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto node_indices() const -> std::vector<size_t>;
    void add_node(size_t node_index);

    auto handle() -> cgltf_scene*;
    [[nodiscard]] auto handle() const -> const cgltf_scene*;
};
} // namespace nv

#endif // _GLTF_ASSET_H_