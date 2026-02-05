#ifndef _GLTF_TEXTURE_H_
#define _GLTF_TEXTURE_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {
class GLTFTexture {
    friend class GLTFAsset;
    cgltf_texture* _texture;

    explicit GLTFTexture(cgltf_texture* t) : _texture(t) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto image_index() const -> std::optional<size_t>;
    void set_image(size_t image_index);

    [[nodiscard]] auto sampler_index() const -> std::optional<size_t>;
    void set_sampler(size_t sampler_index);

    auto handle() -> cgltf_texture*;
    [[nodiscard]] auto handle() const -> const cgltf_texture*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_