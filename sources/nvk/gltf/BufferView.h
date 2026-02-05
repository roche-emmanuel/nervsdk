#ifndef _GLTF_BUFFERVIEW_H_
#define _GLTF_BUFFERVIEW_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFBufferView {
    friend class GLTFAsset;
    cgltf_buffer_view* _view;

    explicit GLTFBufferView(cgltf_buffer_view* v) : _view(v) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto buffer_index() const -> size_t;
    [[nodiscard]] auto offset() const -> size_t;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto stride() const -> size_t;

    [[nodiscard]] auto type() const -> cgltf_buffer_view_type;
    void set_type(cgltf_buffer_view_type type) { _view->type = type; }

    void set_buffer(size_t buffer_index);
    void set_offset(size_t offset) { _view->offset = offset; }
    void set_size(size_t size) { _view->size = size; }
    void set_stride(size_t stride) { _view->stride = stride; }

    auto handle() -> cgltf_buffer_view*;
    [[nodiscard]] auto handle() const -> const cgltf_buffer_view*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_