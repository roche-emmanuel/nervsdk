#ifndef _GLTF_BUFFERVIEW_H_
#define _GLTF_BUFFERVIEW_H_

#include <nvk/gltf/Element.h>

namespace nv {

// typedef struct cgltf_meshopt_compression {
//     cgltf_buffer* buffer;
//     cgltf_size offset;
//     cgltf_size size;
//     cgltf_size stride;
//     cgltf_size count;
//     cgltf_meshopt_compression_mode mode;
//     cgltf_meshopt_compression_filter filter;
//     cgltf_bool is_khr;
// } cgltf_meshopt_compression;

// typedef enum cgltf_buffer_view_type {
//     cgltf_buffer_view_type_invalid,
//     cgltf_buffer_view_type_indices,
//     cgltf_buffer_view_type_vertices,
//     cgltf_buffer_view_type_max_enum
// } cgltf_buffer_view_type;

// typedef struct cgltf_extras {
//     cgltf_size start_offset; /* this field is deprecated and will be removed
//     in
//                                 the future; use data instead */
//     cgltf_size end_offset;   /* this field is deprecated and will be removed
//     in
//                                 the future; use data instead */

//     char* data;
// } cgltf_extras;

// typedef struct cgltf_extension {
//     char* name;
//     char* data;
// } cgltf_extension;

// char* name;
// cgltf_buffer* buffer;
// cgltf_size offset;
// cgltf_size size;
// cgltf_size stride; /* 0 == automatically determined by accessor */
// cgltf_buffer_view_type type;
// void* data; /* overrides buffer->data if present, filled by extensions */
// cgltf_bool has_meshopt_compression;
// cgltf_meshopt_compression meshopt_compression;
// cgltf_extras extras;
// cgltf_size extensions_count;
// cgltf_extension* extensions;

class GLTFBufferView : public GLTFElement {
  public:
    explicit GLTFBufferView(GLTFAsset& parent, U32 index);

    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    [[nodiscard]] auto offset() const -> U32;
    void set_offset(U32 offset);

    [[nodiscard]] auto size() const -> U32;
    void set_size(U32 size);

    [[nodiscard]] auto stride() const -> U32;
    void set_stride(U32 stride);

    [[nodiscard]] auto type() const -> U32;
    void set_type(U32 type);

    void set_buffer(GLTFBuffer& buf);

    void read(const Json& desc);
    auto write() const -> Json;

    auto data() -> U8*;
    auto data() const -> const U8*;
    auto add_accessor(GLTFElementType etype, GLTFComponentType ctype, U32 count,
                      U32 offset = 0) -> GLTFAccessor&;

  protected:
    String _name;
    RefPtr<GLTFBuffer> _buffer;
    U32 _offset;
    U32 _size;
    U32 _stride;
    U32 _type;
    // U8Vector _override;
};

} // namespace nv

#endif // _GLTF_ASSET_H_