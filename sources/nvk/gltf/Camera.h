#ifndef _GLTF_CAMERA_H_
#define _GLTF_CAMERA_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFCamera : public GLTFElement {
  public:
    GLTFCamera(GLTFAsset& parent, U32 index) : GLTFElement(parent, index) {};
};

} // namespace nv

#endif // _GLTF_ASSET_H_