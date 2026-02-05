#ifndef _GLTF_ANIMATION_H_
#define _GLTF_ANIMATION_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFAnimation {
    friend class GLTFAsset;
    cgltf_animation* _animation;

    explicit GLTFAnimation(cgltf_animation* a) : _animation(a) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto sampler_count() const -> size_t;
    [[nodiscard]] auto channel_count() const -> size_t;

    auto handle() -> cgltf_animation*;
    [[nodiscard]] auto handle() const -> const cgltf_animation*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_