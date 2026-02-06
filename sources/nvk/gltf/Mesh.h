#ifndef _GLTF_MESH_H_
#define _GLTF_MESH_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFMesh : public GLTFElement {
  public:
    explicit GLTFMesh(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // Primitives accessors
    [[nodiscard]] auto primitives_count() const -> U32;

    [[nodiscard]] auto primitives() const
        -> const Vector<RefPtr<GLTFPrimitive>>&;

    [[nodiscard]] auto primitives() -> Vector<RefPtr<GLTFPrimitive>>&;

    [[nodiscard]] auto get_primitive(U32 index) const -> const GLTFPrimitive&;

    [[nodiscard]] auto get_primitive(U32 index) -> GLTFPrimitive&;

    auto add_primitive(GLTFPrimitiveType ptype = GLTF_PRIM_TRIANGLES)
        -> GLTFPrimitive&;

    void clear_primitives();

    // Weights accessors
    [[nodiscard]] auto weights_count() const -> U32;

    [[nodiscard]] auto weights() const -> const F32Vector&;

    [[nodiscard]] auto weights() -> F32Vector&;

    void set_weights(F32Vector weights);

    void clear_weights();

    // Serialization
    void read(const Json& desc);

    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    Vector<RefPtr<GLTFPrimitive>> _primitives;
    F32Vector _weights;
    // StringVector _targetNames; // Uncomment and add accessors if needed
};

} // namespace nv

#endif // _GLTF_MESH_H_