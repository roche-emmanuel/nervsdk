#ifndef _GLTF_MATERIAL_H_
#define _GLTF_MATERIAL_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFMaterial : public GLTFElement {
  public:
    GLTFMaterial(GLTFAsset& parent, U32 index) : GLTFElement(parent, index) {};
};

} // namespace nv

#endif // _GLTF_ASSET_H_