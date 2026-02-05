#ifndef _GLTF_PRIMITIVE_H_
#define _GLTF_PRIMITIVE_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFPrimitive {
    friend class GLTFMesh;
    cgltf_primitive* _prim;

    explicit GLTFPrimitive(cgltf_primitive* p) : _prim(p) {}

  public:
    [[nodiscard]] auto type() const -> cgltf_primitive_type;
    void set_type(cgltf_primitive_type type);

    // Indices
    [[nodiscard]] auto indices() const -> std::optional<GLTFAccessor>;
    void set_indices(size_t accessor_index);

    // Attributes
    struct Attribute {
        std::string_view name;
        size_t accessor_index;
        cgltf_attribute_type type;
    };

    [[nodiscard]] auto attributes() const -> std::vector<Attribute>;
    void add_attribute(std::string_view name, size_t accessor_index,
                       cgltf_attribute_type type);
    void add_attribute(cgltf_attribute_type type, size_t accessor_index);

    // Material
    [[nodiscard]] auto material_index() const -> std::optional<size_t>;
    void set_material(size_t material_index);

    auto handle() -> cgltf_primitive*;
    [[nodiscard]] auto handle() const -> const cgltf_primitive*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_