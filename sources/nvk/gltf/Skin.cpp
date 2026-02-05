#include <nvk/gltf/Skin.h>

namespace nv {
auto GLTFSkin::name() const -> std::string_view {
    return _skin->name ? std::string_view(_skin->name) : std::string_view();
}
auto GLTFSkin::handle() -> cgltf_skin* { return _skin; }
auto GLTFSkin::handle() const -> const cgltf_skin* { return _skin; }
} // namespace nv