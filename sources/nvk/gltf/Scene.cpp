#include <nvk/gltf/Scene.h>

namespace nv {
auto GLTFScene::name() const -> std::string_view {
    return _scene->name ? std::string_view(_scene->name) : std::string_view();
}
auto GLTFScene::handle() -> cgltf_scene* { return _scene; }
auto GLTFScene::handle() const -> const cgltf_scene* { return _scene; }
} // namespace nv