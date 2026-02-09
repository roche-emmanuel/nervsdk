#include <nvk_gltf.h>

namespace nv {

GLTFSampler::GLTFSampler(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}

auto GLTFSampler::name() const -> const String& { return _name; }

void GLTFSampler::set_name(String name) { _name = std::move(name); }

auto GLTFSampler::has_mag_filter() const -> bool { return _hasMagFilter; }

auto GLTFSampler::mag_filter() const -> GLTFMagFilter { return _magFilter; }

void GLTFSampler::set_mag_filter(GLTFMagFilter filter) {
    _magFilter = filter;
    _hasMagFilter = true;
}

void GLTFSampler::clear_mag_filter() { _hasMagFilter = false; }

auto GLTFSampler::has_min_filter() const -> bool { return _hasMinFilter; }

auto GLTFSampler::min_filter() const -> GLTFMinFilter { return _minFilter; }

void GLTFSampler::set_min_filter(GLTFMinFilter filter) {
    _minFilter = filter;
    _hasMinFilter = true;
}

void GLTFSampler::clear_min_filter() { _hasMinFilter = false; }

auto GLTFSampler::wrap_s() const -> GLTFWrapMode { return _wrapS; }

void GLTFSampler::set_wrap_s(GLTFWrapMode mode) { _wrapS = mode; }

auto GLTFSampler::wrap_t() const -> GLTFWrapMode { return _wrapT; }

void GLTFSampler::set_wrap_t(GLTFWrapMode mode) { _wrapT = mode; }

void GLTFSampler::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("magFilter")) {
        _magFilter = static_cast<GLTFMagFilter>(desc["magFilter"].get<U32>());
        _hasMagFilter = true;
    }

    if (desc.contains("minFilter")) {
        _minFilter = static_cast<GLTFMinFilter>(desc["minFilter"].get<U32>());
        _hasMinFilter = true;
    }

    _wrapS = static_cast<GLTFWrapMode>(desc.value("wrapS", 10497U)); // REPEAT
    _wrapT = static_cast<GLTFWrapMode>(desc.value("wrapT", 10497U)); // REPEAT
}

auto GLTFSampler::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (_hasMagFilter) {
        json["magFilter"] = static_cast<U32>(_magFilter);
    }

    if (_hasMinFilter) {
        json["minFilter"] = static_cast<U32>(_minFilter);
    }

    if (_wrapS != GLTF_WRAP_REPEAT) {
        json["wrapS"] = static_cast<U32>(_wrapS);
    }

    if (_wrapT != GLTF_WRAP_REPEAT) {
        json["wrapT"] = static_cast<U32>(_wrapT);
    }

    return json;
}

} // namespace nv