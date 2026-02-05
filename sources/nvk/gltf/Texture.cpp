#include <nvk/gltf/Texture.h>

namespace nv {
auto GLTFTexture::name() const -> std::string_view {
    return _texture->name ? std::string_view(_texture->name)
                          : std::string_view();
}
auto GLTFTexture::handle() -> cgltf_texture* { return _texture; }
auto GLTFTexture::handle() const -> const cgltf_texture* { return _texture; }
} // namespace nv