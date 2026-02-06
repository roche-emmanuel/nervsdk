#ifndef _GLTF_NODE_H_
#define _GLTF_NODE_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFNode : public GLTFElement {
  public:
    explicit GLTFNode(GLTFAsset& parent, U32 index)
        : GLTFElement(parent, index) {}

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // Parent node accessors
    [[nodiscard]] auto parent_node() const -> const GLTFNode*;
    [[nodiscard]] auto parent_node() -> GLTFNode*;
    void set_parent_node(GLTFNode* parent);

    // Children accessors
    [[nodiscard]] auto children_count() const -> U32;

    [[nodiscard]] auto children() const -> const Vector<RefPtr<GLTFNode>>&;

    [[nodiscard]] auto children() -> Vector<RefPtr<GLTFNode>>&;

    [[nodiscard]] auto get_child(U32 index) const -> const GLTFNode&;

    [[nodiscard]] auto get_child(U32 index) -> GLTFNode&;

    auto add_child() -> GLTFNode&;

    void add_child(RefPtr<GLTFNode> child);
    void clear_children();

    // Skin accessors
    [[nodiscard]] auto has_skin() const -> bool;
    [[nodiscard]] auto skin() const -> const GLTFSkin&;
    [[nodiscard]] auto skin() -> GLTFSkin&;
    void set_skin(GLTFSkin& skin);
    void clear_skin();

    // Mesh accessors
    [[nodiscard]] auto has_mesh() const -> bool;
    [[nodiscard]] auto mesh() const -> const GLTFMesh&;
    [[nodiscard]] auto mesh() -> GLTFMesh&;
    void set_mesh(GLTFMesh& mesh);
    void clear_mesh();

    // Camera accessors
    [[nodiscard]] auto has_camera() const -> bool;
    [[nodiscard]] auto camera() const -> const GLTFCamera&;
    [[nodiscard]] auto camera() -> GLTFCamera&;
    void set_camera(GLTFCamera& camera);
    void clear_camera();

    // Matrix accessors
    [[nodiscard]] auto has_matrix() const -> bool;
    [[nodiscard]] auto matrix() const -> const Mat4d&;
    void set_matrix(const Mat4d& matrix);
    void clear_matrix();

    // Translation accessors
    [[nodiscard]] auto has_translation() const -> bool;
    [[nodiscard]] auto translation() const -> const Vec3d&;
    void set_translation(const Vec3d& translation);
    void clear_translation();

    // Rotation accessors
    [[nodiscard]] auto has_rotation() const -> bool;
    [[nodiscard]] auto rotation() const -> const Quatd&;
    void set_rotation(const Quatd& rotation);
    void clear_rotation();

    // Scale accessors
    [[nodiscard]] auto has_scale() const -> bool;
    [[nodiscard]] auto scale() const -> const Vec3d&;
    void set_scale(const Vec3d& scale);
    void clear_scale();

    // Serialization
    void read(const Json& desc);
    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    GLTFNode* _parentNode{nullptr};
    Vector<RefPtr<GLTFNode>> _children;
    RefPtr<GLTFSkin> _skin;
    RefPtr<GLTFMesh> _mesh;
    RefPtr<GLTFCamera> _camera;
    bool _hasMatrix{false};
    Mat4d _matrix;
    bool _hasTranslation{false};
    Vec3d _translation;
    bool _hasRotation{false};
    Quatd _rotation;
    bool _hasScale{false};
    Vec3d _scale;
};

} // namespace nv

#endif // _GLTF_NODE_H_