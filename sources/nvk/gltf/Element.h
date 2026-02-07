#ifndef _GLTF_ELEMENT_H_
#define _GLTF_ELEMENT_H_

#include <nvk_common.h>

namespace nv {

class GLTFElement : public RefObject {

  public:
    explicit GLTFElement(GLTFAsset& parent, U32 index);

    auto parent() const -> GLTFAsset& { return _parent; }
    auto index() const -> U32 { return _index; }

  protected:
    GLTFAsset& _parent;
    U32 _index;
};

} // namespace nv

#endif // _GLTF_ASSET_H_