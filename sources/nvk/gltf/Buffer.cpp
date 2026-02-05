#include <nvk/gltf/Buffer.h>

namespace nv {
auto GLTFBuffer::name() const -> std::string_view {
    return _buffer->name ? std::string_view(_buffer->name) : std::string_view();
}
auto GLTFBuffer::uri() const -> std::string_view {
    return _buffer->uri ? std::string_view(_buffer->uri) : std::string_view();
}
auto GLTFBuffer::size() const -> size_t { return _buffer->size; }
auto GLTFBuffer::data() const -> std::span<const uint8_t> {
    if (_buffer->data) {
        return {static_cast<const uint8_t*>(_buffer->data), _buffer->size};
    }
    return {};
}
auto GLTFBuffer::data() -> std::span<uint8_t> {
    if (_data_storage) {
        return *_data_storage;
    }
    if (_buffer->data) {
        return {static_cast<uint8_t*>(_buffer->data), _buffer->size};
    }
    return {};
}
auto GLTFBuffer::handle() -> cgltf_buffer* { return _buffer; }
auto GLTFBuffer::handle() const -> const cgltf_buffer* { return _buffer; }
} // namespace nv