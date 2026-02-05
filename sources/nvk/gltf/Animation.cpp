#include <nvk/gltf/Animation.h>

namespace nv {
auto GLTFAnimation::name() const -> std::string_view {
    return _animation->name ? std::string_view(_animation->name)
                            : std::string_view();
}
auto GLTFAnimation::sampler_count() const -> size_t {
    return _animation->samplers_count;
}
auto GLTFAnimation::channel_count() const -> size_t {
    return _animation->channels_count;
}
auto GLTFAnimation::handle() -> cgltf_animation* { return _animation; }
auto GLTFAnimation::handle() const -> const cgltf_animation* {
    return _animation;
}
} // namespace nv