#ifndef _GLTF_MATERIAL_H_
#define _GLTF_MATERIAL_H_

#include <nvk/gltf/Element.h>

namespace nv {

enum GLTFAlphaMode {
    GLTF_ALPHA_OPAQUE = 0,
    GLTF_ALPHA_MASK = 1,
    GLTF_ALPHA_BLEND = 2
};

struct GLTFTextureInfo {
    RefPtr<GLTFTexture> texture = nullptr;
    U32 texCoord = 0;

    virtual ~GLTFTextureInfo() = default;

    virtual void read(const Json& desc, GLTFAsset& parent);
    [[nodiscard]] virtual auto write() const -> Json;
};

struct GLTFNormalTextureInfo : public GLTFTextureInfo {
    F32 scale = 1.0F;

    void read(const Json& desc, GLTFAsset& parent) override;
    [[nodiscard]] auto write() const -> Json override;
};

struct GLTFOcclusionTextureInfo : public GLTFTextureInfo {
    F32 strength = 1.0F;

    void read(const Json& desc, GLTFAsset& parent) override;
    [[nodiscard]] auto write() const -> Json override;
};

struct GLTFPBRMetallicRoughness {
    Vec4f baseColorFactor = Vec4f(1.0F, 1.0F, 1.0F, 1.0F);
    GLTFTextureInfo baseColorTexture;
    F32 metallicFactor = 1.0F;
    F32 roughnessFactor = 1.0F;
    GLTFTextureInfo metallicRoughnessTexture;

    void read(const Json& desc, GLTFAsset& parent);
    [[nodiscard]] auto write() const -> Json;
};

class GLTFMaterial : public GLTFElement {
  public:
    explicit GLTFMaterial(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // PBR Metallic Roughness
    [[nodiscard]] auto has_pbr_metallic_roughness() const -> bool;
    [[nodiscard]] auto pbr_metallic_roughness() const
        -> const GLTFPBRMetallicRoughness&;
    [[nodiscard]] auto pbr_metallic_roughness() -> GLTFPBRMetallicRoughness&;
    void set_pbr_metallic_roughness(const GLTFPBRMetallicRoughness& pbr);
    void clear_pbr_metallic_roughness();

    // Normal texture
    [[nodiscard]] auto has_normal_texture() const -> bool;
    [[nodiscard]] auto normal_texture() const -> const GLTFNormalTextureInfo&;
    [[nodiscard]] auto normal_texture() -> GLTFNormalTextureInfo&;
    void set_normal_texture(const GLTFNormalTextureInfo& texture);
    void clear_normal_texture();

    // Occlusion texture
    [[nodiscard]] auto has_occlusion_texture() const -> bool;
    [[nodiscard]] auto occlusion_texture() const
        -> const GLTFOcclusionTextureInfo&;
    [[nodiscard]] auto occlusion_texture() -> GLTFOcclusionTextureInfo&;
    void set_occlusion_texture(const GLTFOcclusionTextureInfo& texture);
    void clear_occlusion_texture();

    // Emissive texture
    [[nodiscard]] auto has_emissive_texture() const -> bool;
    [[nodiscard]] auto emissive_texture() const -> const GLTFTextureInfo&;
    [[nodiscard]] auto emissive_texture() -> GLTFTextureInfo&;
    void set_emissive_texture(const GLTFTextureInfo& texture);
    void clear_emissive_texture();

    // Emissive factor
    [[nodiscard]] auto emissive_factor() const -> const Vec3f&;
    void set_emissive_factor(const Vec3f& factor);

    // Alpha mode
    [[nodiscard]] auto alpha_mode() const -> GLTFAlphaMode;
    void set_alpha_mode(GLTFAlphaMode mode);

    // Alpha cutoff
    [[nodiscard]] auto alpha_cutoff() const -> F32;
    void set_alpha_cutoff(F32 cutoff);

    // Double sided
    [[nodiscard]] auto double_sided() const -> bool;
    void set_double_sided(bool doubleSided);

    // Serialization
    void read(const Json& desc);
    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    GLTFPBRMetallicRoughness _pbrMetallicRoughness;
    bool _hasPbrMetallicRoughness = false;
    GLTFNormalTextureInfo _normalTexture;
    bool _hasNormalTexture = false;
    GLTFOcclusionTextureInfo _occlusionTexture;
    bool _hasOcclusionTexture = false;
    GLTFTextureInfo _emissiveTexture;
    bool _hasEmissiveTexture = false;
    Vec3f _emissiveFactor = Vec3f(0.0F, 0.0F, 0.0F);
    GLTFAlphaMode _alphaMode = GLTF_ALPHA_OPAQUE;
    F32 _alphaCutoff = 0.5F;
    bool _doubleSided = false;
};

} // namespace nv

#endif // _GLTF_MATERIAL_H_