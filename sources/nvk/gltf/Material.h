#ifndef _GLTF_MATERIAL_H_
#define _GLTF_MATERIAL_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFMaterial {
    friend class GLTFAsset;
    cgltf_material* _material;

    explicit GLTFMaterial(cgltf_material* m) : _material(m) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto has_metallic_roughness() const -> bool;
    [[nodiscard]] auto has_specular_glossiness() const -> bool;

    // PBR Metallic Roughness
    [[nodiscard]] auto base_color_factor() const -> std::span<const float, 4>;

    void set_base_color_factor(float r, float g, float b, float a);

    [[nodiscard]] auto metallic_factor() const -> float;

    [[nodiscard]] auto roughness_factor() const -> float;

    void set_metallic_factor(float metallic);

    void set_roughness_factor(float roughness);

    // Alpha mode
    [[nodiscard]] auto alpha_mode() const -> cgltf_alpha_mode;
    [[nodiscard]] auto alpha_cutoff() const -> float;

    void set_alpha_mode(cgltf_alpha_mode mode);
    void set_alpha_cutoff(float cutoff);

    // Double sided
    [[nodiscard]] auto double_sided() const -> bool;
    void set_double_sided(bool double_sided);

    auto handle() -> cgltf_material*;
    [[nodiscard]] auto handle() const -> const cgltf_material*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_