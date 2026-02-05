#include <nvk/gltf/Material.h>

namespace nv {
auto GLTFMaterial::name() const -> std::string_view {
    return _material->name ? std::string_view(_material->name)
                           : std::string_view();
}
auto GLTFMaterial::has_metallic_roughness() const -> bool {
    return _material->has_pbr_metallic_roughness;
}
auto GLTFMaterial::has_specular_glossiness() const -> bool {
    return _material->has_pbr_specular_glossiness;
}
auto GLTFMaterial::base_color_factor() const -> std::span<const float, 4> {
    return std::span<const float, 4>(
        _material->pbr_metallic_roughness.base_color_factor, 4);
}
void GLTFMaterial::set_base_color_factor(float r, float g, float b, float a) {
    _material->pbr_metallic_roughness.base_color_factor[0] = r;
    _material->pbr_metallic_roughness.base_color_factor[1] = g;
    _material->pbr_metallic_roughness.base_color_factor[2] = b;
    _material->pbr_metallic_roughness.base_color_factor[3] = a;
    _material->has_pbr_metallic_roughness = 1;
}
auto GLTFMaterial::metallic_factor() const -> float {
    return _material->pbr_metallic_roughness.metallic_factor;
}
auto GLTFMaterial::roughness_factor() const -> float {
    return _material->pbr_metallic_roughness.roughness_factor;
}
void GLTFMaterial::set_metallic_factor(float metallic) {
    _material->pbr_metallic_roughness.metallic_factor = metallic;
    _material->has_pbr_metallic_roughness = 1;
}
void GLTFMaterial::set_roughness_factor(float roughness) {
    _material->pbr_metallic_roughness.roughness_factor = roughness;
    _material->has_pbr_metallic_roughness = 1;
}
auto GLTFMaterial::alpha_mode() const -> cgltf_alpha_mode {
    return _material->alpha_mode;
}
auto GLTFMaterial::alpha_cutoff() const -> float {
    return _material->alpha_cutoff;
}
void GLTFMaterial::set_alpha_mode(cgltf_alpha_mode mode) {
    _material->alpha_mode = mode;
}
void GLTFMaterial::set_alpha_cutoff(float cutoff) {
    _material->alpha_cutoff = cutoff;
}
auto GLTFMaterial::double_sided() const -> bool {
    return _material->double_sided;
}
void GLTFMaterial::set_double_sided(bool double_sided) {
    _material->double_sided = double_sided;
}
auto GLTFMaterial::handle() -> cgltf_material* { return _material; }
auto GLTFMaterial::handle() const -> const cgltf_material* { return _material; }
} // namespace nv