#include <nvk/gltf/Asset.h>
#include <nvk/gltf/Buffer.h>
#include <nvk/gltf/BufferView.h>

namespace nv {
auto GLTFBufferView::name() const -> const String& { return _name; };
void GLTFBufferView::set_name(String name) { _name = std::move(name); };
auto GLTFBufferView::offset() const -> U32 { return _offset; };
void GLTFBufferView::set_offset(U32 offset) { _offset = offset; };
auto GLTFBufferView::size() const -> U32 { return _size; };
void GLTFBufferView::set_size(U32 size) { _size = size; };
auto GLTFBufferView::type() const -> U32 { return _type; };
void GLTFBufferView::set_type(U32 type) { _type = type; };

void GLTFBufferView::read(const Json& desc) {
    // Optional name
    if (desc.contains("name")) {
        _name = desc["name"].get<String>();
    }

    // Required buffer index
    const U32 bufferIndex = desc.at("buffer").get<U32>();
    _buffer = &_parent.get_buffer(bufferIndex);

    // Optional byteOffset (defaults to 0)
    _offset = desc.value("byteOffset", 0U);

    // Optional byteStride (defaults to 0)
    _stride = desc.value("byteStride", 0U);

    // Required byteLength
    _size = desc.at("byteLength").get<U32>();

    // Optional target
    if (desc.contains("target")) {
        const U32 target = desc["target"].get<U32>();
        switch (target) {
        case BUFFER_VIEW_INDICES: // ELEMENT_ARRAY_BUFFER
            _type = BUFFER_VIEW_INDICES;
            break;
        case BUFFER_VIEW_VERTICES: // ARRAY_BUFFER
            _type = BUFFER_VIEW_VERTICES;
            break;
        default:
            _type = BUFFER_VIEW_UNKNOWN;
            break;
        }
    } else {
        _type = BUFFER_VIEW_UNKNOWN;
    }
};

auto GLTFBufferView::write() const -> Json {
    // Required:
    NVCHK(_buffer != nullptr, "Invalid buffer in bufferview.");
    Json desc;

    desc["buffer"] = _buffer->index();
    desc["byteLength"] = _size;

    // Optional
    if (!_name.empty()) {
        desc["name"] = _name;
    }

    if (_offset != 0) {
        desc["byteOffset"] = _offset;
    }

    if (_stride != 0) {
        desc["byteStride"] = _stride;
    }

    // Optional target
    if (_type != BUFFER_VIEW_UNKNOWN) {
        desc["target"] = _type;
    }

    return desc;
}

void GLTFBufferView::set_buffer(GLTFBuffer& buf) { _buffer = &buf; }
GLTFBufferView::GLTFBufferView(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}
auto GLTFBufferView::stride() const -> U32 { return _stride; };
void GLTFBufferView::set_stride(U32 stride) { _stride = stride; };
auto GLTFBufferView::add_accessor(GLTFElementType etype,
                                  GLTFComponentType ctype, U32 count,
                                  U32 offset) -> GLTFAccessor& {
    return _parent.add_accessor(*this, etype, ctype, count, offset);
};
} // namespace nv