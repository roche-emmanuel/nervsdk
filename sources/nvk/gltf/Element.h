#ifndef _GLTF_OBJECT_H_
#define _GLTF_OBJECT_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFElement : public RefObject {

  public:
    explicit GLTFElement(GLTFAsset& parent, U32 index);

  protected:
    GLTFAsset& _parent;
    U32 _index;
};

} // namespace nv

#endif // _GLTF_ASSET_H_