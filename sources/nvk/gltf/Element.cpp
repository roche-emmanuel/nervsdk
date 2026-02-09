#include <nvk/gltf/Element.h>

namespace nv {
GLTFElement::GLTFElement(GLTFAsset& parent, U32 index)
    : _parent(parent), _index(index) {}
} // namespace nv