#ifndef _GLTF_SKIN_H_
#define _GLTF_SKIN_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFSkin : public GLTFElement {
  public:
    GLTFSkin(GLTFAsset& parent, U32 index) : GLTFElement(parent, index) {};
};

} // namespace nv

#endif // _GLTF_ASSET_H_