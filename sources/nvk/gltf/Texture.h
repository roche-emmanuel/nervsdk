#ifndef _GLTF_TEXTURE_H_
#define _GLTF_TEXTURE_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFTexture : public GLTFElement {
  public:
    explicit GLTFTexture(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // Sampler accessors
    [[nodiscard]] auto has_sampler() const -> bool;
    [[nodiscard]] auto sampler() const -> const GLTFSampler&;
    [[nodiscard]] auto sampler() -> GLTFSampler&;
    void set_sampler(GLTFSampler& sampler);
    void clear_sampler();

    // Source (image) accessors
    [[nodiscard]] auto has_source() const -> bool;
    [[nodiscard]] auto source() const -> const GLTFImage&;
    [[nodiscard]] auto source() -> GLTFImage&;
    void set_source(GLTFImage& image);
    void clear_source();

    // Serialization
    void read(const Json& desc);
    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    RefPtr<GLTFSampler> _sampler;
    RefPtr<GLTFImage> _source;
};

} // namespace nv

#endif // _GLTF_TEXTURE_H_