#include <nvk/gltf/Primitive.h>

namespace nv {
auto GLTFPrimitive::type() const -> cgltf_primitive_type { return _prim->type; }
void GLTFPrimitive::set_type(cgltf_primitive_type type) { _prim->type = type; }
auto GLTFPrimitive::handle() -> cgltf_primitive* { return _prim; }
auto GLTFPrimitive::handle() const -> const cgltf_primitive* { return _prim; }
} // namespace nv