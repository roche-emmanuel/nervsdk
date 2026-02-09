#ifndef _GLTF_PRIMITIVE_H_
#define _GLTF_PRIMITIVE_H_

#include <nvk_common.h>

namespace nv {

class GLTFPrimitive : public RefObject {
  public:
    using AttribMap = UnorderedMap<GLTFAttributeType, RefPtr<GLTFAccessor>>;

    GLTFPrimitive(GLTFAsset& parent, GLTFMesh& mesh, U32 index)
        : _parent(parent), _mesh(&mesh), _index(index) {}

    // Index accessor
    [[nodiscard]] auto index() const -> U32;

    // Primitive type accessors
    [[nodiscard]] auto type() const -> GLTFPrimitiveType;
    void set_type(GLTFPrimitiveType type);

    // Material accessor
    [[nodiscard]] auto has_material() const -> bool;
    [[nodiscard]] auto material() const -> GLTFMaterial&;
    void set_material(GLTFMaterial& material);
    void clear_material();

    // Indices accessor
    [[nodiscard]] auto has_indices() const -> bool;
    [[nodiscard]] auto indices() const -> GLTFAccessor&;
    void set_indices(GLTFAccessor& accessor);
    void clear_indices();

    // Attributes accessors
    [[nodiscard]] auto attributes_count() const -> U32;

    [[nodiscard]] auto attributes() const -> const AttribMap&;
    [[nodiscard]] auto attributes() -> AttribMap&;

    [[nodiscard]] auto has_attribute(GLTFAttributeType atype) const -> bool;

    [[nodiscard]] auto attribute(GLTFAttributeType atype) const
        -> GLTFAccessor&;

    void set_attribute(GLTFAttributeType atype, GLTFAccessor& accessor);
    void remove_attribute(GLTFAttributeType atype);
    void clear_attributes();

#if 0
    // Morph targets accessors
    [[nodiscard]] auto targets_count() const -> U32 {
        return static_cast<U32>(_targets.size());
    }

    [[nodiscard]] auto targets() const -> const Vector<AttribMap>& {
        return _targets;
    }

    [[nodiscard]] auto targets() -> Vector<AttribMap>& { return _targets; }

    [[nodiscard]] auto target(U32 index) const -> const AttribMap& {
        return _targets[index];
    }

    [[nodiscard]] auto target(U32 index) -> AttribMap& {
        return _targets[index];
    }

    void add_target(AttribMap target) { _targets.push_back(std::move(target)); }

    void clear_targets() { _targets.clear(); }
#endif

    // Serialization
    void read(const Json& desc);

    [[nodiscard]] auto write() const -> Json;

  protected:
    GLTFAsset& _parent;
    GLTFMesh* _mesh;
    U32 _index{0};
    GLTFPrimitiveType _type{GLTF_PRIM_TRIANGLES};
    RefPtr<GLTFMaterial> _material{nullptr};
    RefPtr<GLTFAccessor> _indices{nullptr};
    AttribMap _attributes;
    // Vector<AttribMap> _targets;
};

} // namespace nv

#endif // _GLTF_PRIMITIVE_H_