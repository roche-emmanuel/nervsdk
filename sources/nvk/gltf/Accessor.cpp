#include <nvk/gltf/Accessor.h>

namespace nv {
auto GLTFAccessor::name() const -> std::string_view {
    return _accessor->name ? std::string_view(_accessor->name)
                           : std::string_view();
}
auto GLTFAccessor::type() const -> cgltf_type { return _accessor->type; }
auto GLTFAccessor::component_type() const -> cgltf_component_type {
    return _accessor->component_type;
}
auto GLTFAccessor::count() const -> size_t { return _accessor->count; }
auto GLTFAccessor::offset() const -> size_t { return _accessor->offset; }
auto GLTFAccessor::normalized() const -> bool { return _accessor->normalized; }
void GLTFAccessor::set_offset(size_t offset) { _accessor->offset = offset; }
void GLTFAccessor::set_normalized(bool normalized) {
    _accessor->normalized = normalized;
}
auto GLTFAccessor::has_min() const -> bool { return _accessor->has_min; }
auto GLTFAccessor::has_max() const -> bool { return _accessor->has_max; }
auto GLTFAccessor::min() const -> std::span<const float> {
    return _accessor->has_min ? std::span<const float>(_accessor->min, 16)
                              : std::span<const float>();
}
auto GLTFAccessor::max() const -> std::span<const float> {
    return _accessor->has_max ? std::span<const float>(_accessor->max, 16)
                              : std::span<const float>();
}
auto GLTFAccessor::handle() -> cgltf_accessor* { return _accessor; }
auto GLTFAccessor::handle() const -> const cgltf_accessor* { return _accessor; }
} // namespace nv