#include <nvk/gltf/Asset.h>
#include <nvk/gltf/Buffer.h>

namespace nv {

auto GLTFBuffer::name() const -> const String& { return _name; }

auto GLTFBuffer::uri() const -> const String& { return _uri; }

auto GLTFBuffer::size() const -> size_t { return _data.size(); }

auto GLTFBuffer::data() const -> const U8* { return _data.data(); }

auto GLTFBuffer::data() -> U8* { return _data.data(); }

void GLTFBuffer::set_data(Vector<U8> data) { _data = std::move(data); };

void GLTFBuffer::resize(size_t new_size) { _data.resize(new_size); };

void GLTFBuffer::set_name(String name) { _name = std::move(name); };

void GLTFBuffer::set_uri(String uri) { _uri = std::move(uri); };

GLTFBuffer::GLTFBuffer(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {};

#if 0
  // Reference:
typedef struct cgltf_buffer {
    char* name;
    cgltf_size size;
    char* uri;
    void* data; /* loaded by cgltf_load_buffers */
    cgltf_data_free_method data_free_method;
    cgltf_extras extras;
    cgltf_size extensions_count;
    cgltf_extension* extensions;
} cgltf_buffer;
#endif

void GLTFBuffer::read(const Json& desc) {
    // Required: byteLength
    if (!desc.contains("byteLength")) {
        throw std::runtime_error(
            "GLTFBuffer: missing required 'byteLength' property");
    }
    size_t byteLength = desc["byteLength"].get<size_t>();

    // Optional: name
    if (desc.contains("name")) {
        _name = desc["name"].get<String>();
    }

    // Optional: uri
    if (desc.contains("uri")) {
        _uri = desc["uri"].get<String>();

        // Load the actual buffer data from URI
        // You'll need to implement this based on your needs:
        // - Data URI (base64 encoded): starts with "data:"
        // - External file: relative path
        // - For GLB files: uri may be absent (buffer is in binary chunk)

        if (_uri.starts_with("data:")) {
            // Handle data URI (base64 decoding)
            _data = _parent.decode_data_uri(_uri, byteLength);
        } else {
            // Handle external file
            _data = _parent.load_external_buffer(_uri, byteLength);
        }
    } else {
        // No URI means this buffer is in the GLB binary chunk
        // You'll need to handle this separately when parsing GLB
        _data.resize(byteLength);
    }
}

auto GLTFBuffer::write() const -> Json {
    Json desc;

    // Required: byteLength
    desc["byteLength"] = _data.size();

    // Optional: name
    if (!_name.empty()) {
        desc["name"] = _name;
    }

    // Optional: uri
    if (!_uri.empty()) {
        desc["uri"] = _uri;
    } else {
        // Encode the data as base64:
        String data = "data:application/octet-stream;base64,";
        data += base64_encode(_data);
        desc["uri"] = data;
    }

    // Note: The actual binary data is NOT written to JSON
    // It goes either to:
    // - A separate .bin file (referenced by uri)
    // - GLB binary chunk (no uri)
    // - Data URI (base64 encoded in uri)
    return desc;
}

auto GLTFBuffer::add_bufferview(U32 offset, U32 size) -> GLTFBufferView& {
    return _parent.add_bufferview(*this, offset, size);
}

} // namespace nv