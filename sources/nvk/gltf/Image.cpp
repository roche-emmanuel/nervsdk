#include <nvk_gltf.h>

namespace nv {

GLTFImage::GLTFImage(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}

auto GLTFImage::name() const -> const String& { return _name; }

void GLTFImage::set_name(String name) { _name = std::move(name); }

auto GLTFImage::has_uri() const -> bool { return _hasUri; }

auto GLTFImage::uri() const -> const String& { return _uri; }

void GLTFImage::set_uri(String uri) {
    _uri = std::move(uri);
    _hasUri = true;
}

void GLTFImage::clear_uri() {
    _uri.clear();
    _hasUri = false;
}

auto GLTFImage::has_mime_type() const -> bool { return _hasMimeType; }

auto GLTFImage::mime_type() const -> const String& { return _mimeType; }

void GLTFImage::set_mime_type(String mimeType) {
    _mimeType = std::move(mimeType);
    _hasMimeType = true;
}

void GLTFImage::clear_mime_type() {
    _mimeType.clear();
    _hasMimeType = false;
}

auto GLTFImage::has_bufferview() const -> bool {
    return _bufferView != nullptr;
}

auto GLTFImage::bufferview() const -> const GLTFBufferView& {
    NVCHK(_bufferView != nullptr, "Image has no buffer view");
    return *_bufferView;
}

auto GLTFImage::bufferview() -> GLTFBufferView& {
    NVCHK(_bufferView != nullptr, "Image has no buffer view");
    return *_bufferView;
}

void GLTFImage::set_bufferview(GLTFBufferView& view) { _bufferView = &view; }

void GLTFImage::clear_bufferview() { _bufferView = nullptr; }

void GLTFImage::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("uri") && desc["uri"].is_string()) {
        _uri = desc["uri"].get<String>();
        _hasUri = true;
    }

    if (desc.contains("mimeType") && desc["mimeType"].is_string()) {
        _mimeType = desc["mimeType"].get<String>();
        _hasMimeType = true;
    }

    if (desc.contains("bufferView")) {
        const U32 bufferViewIndex = desc["bufferView"].get<U32>();
        _bufferView = &_parent.get_bufferview(bufferViewIndex);
    }
}

auto GLTFImage::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (_hasUri) {
        json["uri"] = _uri;
    }

    if (_hasMimeType) {
        json["mimeType"] = _mimeType;
    }

    if (_bufferView != nullptr) {
        json["bufferView"] = _bufferView->index();
    }

    return json;
}

} // namespace nv