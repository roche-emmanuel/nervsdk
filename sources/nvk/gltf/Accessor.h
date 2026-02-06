#ifndef _GLTF_ACCESSOR_H_
#define _GLTF_ACCESSOR_H_

#include <nvk/gltf/Element.h>

// typedef enum cgltf_component_type {
//     cgltf_component_type_invalid,
//     cgltf_component_type_r_8,   /* BYTE */
//     cgltf_component_type_r_8u,  /* UNSIGNED_BYTE */
//     cgltf_component_type_r_16,  /* SHORT */
//     cgltf_component_type_r_16u, /* UNSIGNED_SHORT */
//     cgltf_component_type_r_32u, /* UNSIGNED_INT */
//     cgltf_component_type_r_32f, /* FLOAT */
//     cgltf_component_type_max_enum
// } cgltf_component_type;

// typedef enum cgltf_type {
//     cgltf_type_invalid,
//     cgltf_type_scalar,
//     cgltf_type_vec2,
//     cgltf_type_vec3,
//     cgltf_type_vec4,
//     cgltf_type_mat2,
//     cgltf_type_mat3,
//     cgltf_type_mat4,
//     cgltf_type_max_enum
// } cgltf_type;

// typedef struct cgltf_accessor_sparse {
//     cgltf_size count;
//     cgltf_buffer_view* indices_buffer_view;
//     cgltf_size indices_byte_offset;
//     cgltf_component_type indices_component_type;
//     cgltf_buffer_view* values_buffer_view;
//     cgltf_size values_byte_offset;
// } cgltf_accessor_sparse;

// typedef struct cgltf_accessor {
//     char* name;
//     cgltf_component_type component_type;
//     cgltf_bool normalized;
//     cgltf_type type;
//     cgltf_size offset;
//     cgltf_size count;
//     cgltf_size stride;
//     cgltf_buffer_view* buffer_view;
//     cgltf_bool has_min;
//     cgltf_float min[16];
//     cgltf_bool has_max;
//     cgltf_float max[16];
//     cgltf_bool is_sparse;
//     cgltf_accessor_sparse sparse;
//     cgltf_extras extras;
//     cgltf_size extensions_count;
//     cgltf_extension* extensions;
// } cgltf_accessor;

namespace nv {

class GLTFAccessor : public GLTFElement {

  public:
    explicit GLTFAccessor(GLTFAsset& parent, U32 index);

    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

  protected:
    String _name;
    U32 _componentType{GLTF_COMP_UNKNOWN};
    bool _normalized{false};
    U32 _elementType{GLTF_ELEM_UNKNOWN};
    U32 _offset{0};
    U32 _count{0};
    U32 _stride{0};
    RefPtr<GLTFBufferView> _bufferView;
    bool _hasMin;
    std::array<F32, 16> _min;
    bool _hasMax;
    std::array<F32, 16> _max;
};

} // namespace nv

#endif // _GLTF_ASSET_H_