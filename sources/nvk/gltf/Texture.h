#ifndef _GLTF_TEXTURE_H_
#define _GLTF_TEXTURE_H_

#include <nvk/gltf/Element.h>

namespace nv {
class GLTFTexture : public GLTFElement {
  public:
    GLTFTexture(GLTFAsset& parent, U32 index) : GLTFElement(parent, index) {};

    [[nodiscard]] auto name() const -> const String& { return _name; };
    void set_name(String name) { _name = std::move(name); };

  protected:
    String _name;
};

} // namespace nv

#endif // _GLTF_ASSET_H_