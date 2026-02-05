#include <nvk/gltf/BufferView.h>

namespace nv {
auto GLTFBufferView::name() const -> std::string_view {
    return _view->name ? std::string_view(_view->name) : std::string_view();
}
auto GLTFBufferView::offset() const -> size_t { return _view->offset; }
auto GLTFBufferView::size() const -> size_t { return _view->size; }
auto GLTFBufferView::stride() const -> size_t { return _view->stride; }
auto GLTFBufferView::type() const -> cgltf_buffer_view_type {
    return _view->type;
}
auto GLTFBufferView::handle() -> cgltf_buffer_view* { return _view; }
auto GLTFBufferView::handle() const -> const cgltf_buffer_view* {
    return _view;
}
} // namespace nv