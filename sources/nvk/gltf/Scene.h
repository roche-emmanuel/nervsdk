#ifndef _GLTF_SCENE_H_
#define _GLTF_SCENE_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFScene : public GLTFElement {
  public:
    explicit GLTFScene(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // Nodes accessors
    [[nodiscard]] auto nodes_count() const -> U32;

    [[nodiscard]] auto nodes() const -> const Vector<RefPtr<GLTFNode>>&;

    [[nodiscard]] auto nodes() -> Vector<RefPtr<GLTFNode>>&;

    [[nodiscard]] auto get_node(U32 index) const -> const GLTFNode&;

    [[nodiscard]] auto get_node(U32 index) -> GLTFNode&;

    void add_node(GLTFNode& node);

    void clear_nodes();

    // Serialization
    void read(const Json& desc);

    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    Vector<RefPtr<GLTFNode>> _nodes;
};

} // namespace nv

#endif // _GLTF_SCENE_H_