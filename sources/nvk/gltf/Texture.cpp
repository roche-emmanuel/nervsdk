#include <nvk_gltf.h>

namespace nv {

GLTFTexture::GLTFTexture(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}

auto GLTFTexture::name() const -> const String& { return _name; }

void GLTFTexture::set_name(String name) { _name = std::move(name); }

auto GLTFTexture::has_sampler() const -> bool { return _sampler != nullptr; }

auto GLTFTexture::sampler() const -> const GLTFSampler& {
    NVCHK(_sampler != nullptr, "Texture has no sampler");
    return *_sampler;
}

auto GLTFTexture::sampler() -> GLTFSampler& {
    NVCHK(_sampler != nullptr, "Texture has no sampler");
    return *_sampler;
}

void GLTFTexture::set_sampler(GLTFSampler& sampler) { _sampler = &sampler; }

void GLTFTexture::clear_sampler() { _sampler = nullptr; }

auto GLTFTexture::has_source() const -> bool { return _source != nullptr; }

auto GLTFTexture::source() const -> const GLTFImage& {
    NVCHK(_source != nullptr, "Texture has no source image");
    return *_source;
}

auto GLTFTexture::source() -> GLTFImage& {
    NVCHK(_source != nullptr, "Texture has no source image");
    return *_source;
}

void GLTFTexture::set_source(GLTFImage& image) { _source = &image; }

void GLTFTexture::clear_source() { _source = nullptr; }

void GLTFTexture::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("sampler")) {
        const U32 samplerIndex = desc["sampler"].get<U32>();
        _sampler = &_parent.get_sampler(samplerIndex);
    }

    if (desc.contains("source")) {
        const U32 sourceIndex = desc["source"].get<U32>();
        _source = &_parent.get_image(sourceIndex);
    }
}

auto GLTFTexture::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (_sampler != nullptr) {
        json["sampler"] = _sampler->index();
    }

    if (_source != nullptr) {
        json["source"] = _source->index();
    }

    return json;
}

auto GLTFTexture::add_source() -> GLTFImage& {
    auto& img = _parent.add_image();
    set_source(img);
    return img;
};

} // namespace nv