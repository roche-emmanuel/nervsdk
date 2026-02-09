#include <nvk_gltf.h>

namespace nv {

// GLTFTextureInfo implementation
void GLTFTextureInfo::read(const Json& desc, GLTFAsset& parent) {
    if (desc.contains("index")) {
        const U32 textureIndex = desc["index"].get<U32>();
        texture = &parent.get_texture(textureIndex);
    }
    texCoord = desc.value("texCoord", 0U);
}

auto GLTFTextureInfo::write() const -> Json {
    Json json;
    if (texture != nullptr) {
        json["index"] = texture->index();
    }
    if (texCoord != 0) {
        json["texCoord"] = texCoord;
    }
    return json;
}

// GLTFNormalTextureInfo implementation
void GLTFNormalTextureInfo::read(const Json& desc, GLTFAsset& parent) {
    GLTFTextureInfo::read(desc, parent);
    scale = desc.value("scale", 1.0F);
}

auto GLTFNormalTextureInfo::write() const -> Json {
    Json json = GLTFTextureInfo::write();
    if (scale != 1.0F) {
        json["scale"] = scale;
    }
    return json;
}

// GLTFOcclusionTextureInfo implementation
void GLTFOcclusionTextureInfo::read(const Json& desc, GLTFAsset& parent) {
    GLTFTextureInfo::read(desc, parent);
    strength = desc.value("strength", 1.0F);
}

auto GLTFOcclusionTextureInfo::write() const -> Json {
    Json json = GLTFTextureInfo::write();
    if (strength != 1.0F) {
        json["strength"] = strength;
    }
    return json;
}

// GLTFPBRMetallicRoughness implementation
void GLTFPBRMetallicRoughness::read(const Json& desc, GLTFAsset& parent) {
    if (desc.contains("baseColorFactor")) {
        const auto& factor = desc["baseColorFactor"];
        if (factor.is_array() && factor.size() == 4) {
            baseColorFactor = Vec4f(factor[0].get<F32>(), factor[1].get<F32>(),
                                    factor[2].get<F32>(), factor[3].get<F32>());
        }
    }

    if (desc.contains("baseColorTexture")) {
        baseColorTexture.read(desc["baseColorTexture"], parent);
    }

    metallicFactor = desc.value("metallicFactor", 1.0F);
    roughnessFactor = desc.value("roughnessFactor", 1.0F);

    if (desc.contains("metallicRoughnessTexture")) {
        metallicRoughnessTexture.read(desc["metallicRoughnessTexture"], parent);
    }
}

auto GLTFPBRMetallicRoughness::write() const -> Json {
    Json json;

    // Only write non-default values
    if (baseColorFactor != Vec4f(1.0F, 1.0F, 1.0F, 1.0F)) {
        json["baseColorFactor"] =
            Json::array({baseColorFactor.x(), baseColorFactor.y(),
                         baseColorFactor.z(), baseColorFactor.w()});
    }

    if (baseColorTexture.texture != nullptr) {
        json["baseColorTexture"] = baseColorTexture.write();
    }

    if (metallicFactor != 1.0F) {
        json["metallicFactor"] = metallicFactor;
    }

    if (roughnessFactor != 1.0F) {
        json["roughnessFactor"] = roughnessFactor;
    }

    if (metallicRoughnessTexture.texture != nullptr) {
        json["metallicRoughnessTexture"] = metallicRoughnessTexture.write();
    }

    return json;
}

// GLTFMaterial implementation
GLTFMaterial::GLTFMaterial(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}

auto GLTFMaterial::name() const -> const String& { return _name; }
void GLTFMaterial::set_name(String name) { _name = std::move(name); }

auto GLTFMaterial::has_pbr_metallic_roughness() const -> bool {
    return _hasPbrMetallicRoughness;
}

auto GLTFMaterial::pbr_metallic_roughness() const
    -> const GLTFPBRMetallicRoughness& {
    return _pbrMetallicRoughness;
}

auto GLTFMaterial::pbr_metallic_roughness() -> GLTFPBRMetallicRoughness& {
    _hasPbrMetallicRoughness = true;
    return _pbrMetallicRoughness;
}

void GLTFMaterial::set_pbr_metallic_roughness(
    const GLTFPBRMetallicRoughness& pbr) {
    _pbrMetallicRoughness = pbr;
    _hasPbrMetallicRoughness = true;
}

void GLTFMaterial::clear_pbr_metallic_roughness() {
    _hasPbrMetallicRoughness = false;
}

auto GLTFMaterial::has_normal_texture() const -> bool {
    return _hasNormalTexture;
}

auto GLTFMaterial::normal_texture() const -> const GLTFNormalTextureInfo& {
    return _normalTexture;
}

auto GLTFMaterial::normal_texture() -> GLTFNormalTextureInfo& {
    _hasNormalTexture = true;
    return _normalTexture;
}

void GLTFMaterial::set_normal_texture(const GLTFNormalTextureInfo& texture) {
    _normalTexture = texture;
    _hasNormalTexture = true;
}

void GLTFMaterial::clear_normal_texture() { _hasNormalTexture = false; }

auto GLTFMaterial::has_occlusion_texture() const -> bool {
    return _hasOcclusionTexture;
}

auto GLTFMaterial::occlusion_texture() const
    -> const GLTFOcclusionTextureInfo& {
    return _occlusionTexture;
}

auto GLTFMaterial::occlusion_texture() -> GLTFOcclusionTextureInfo& {
    _hasOcclusionTexture = true;
    return _occlusionTexture;
}

void GLTFMaterial::set_occlusion_texture(
    const GLTFOcclusionTextureInfo& texture) {
    _occlusionTexture = texture;
    _hasOcclusionTexture = true;
}

void GLTFMaterial::clear_occlusion_texture() { _hasOcclusionTexture = false; }

auto GLTFMaterial::has_emissive_texture() const -> bool {
    return _hasEmissiveTexture;
}

auto GLTFMaterial::emissive_texture() const -> const GLTFTextureInfo& {
    return _emissiveTexture;
}

auto GLTFMaterial::emissive_texture() -> GLTFTextureInfo& {
    _hasEmissiveTexture = true;
    return _emissiveTexture;
}

void GLTFMaterial::set_emissive_texture(const GLTFTextureInfo& texture) {
    _emissiveTexture = texture;
    _hasEmissiveTexture = true;
}

void GLTFMaterial::clear_emissive_texture() { _hasEmissiveTexture = false; }

auto GLTFMaterial::emissive_factor() const -> const Vec3f& {
    return _emissiveFactor;
}

void GLTFMaterial::set_emissive_factor(const Vec3f& factor) {
    _emissiveFactor = factor;
}

auto GLTFMaterial::alpha_mode() const -> GLTFAlphaMode { return _alphaMode; }

void GLTFMaterial::set_alpha_mode(GLTFAlphaMode mode) { _alphaMode = mode; }

auto GLTFMaterial::alpha_cutoff() const -> F32 { return _alphaCutoff; }

void GLTFMaterial::set_alpha_cutoff(F32 cutoff) { _alphaCutoff = cutoff; }

auto GLTFMaterial::double_sided() const -> bool { return _doubleSided; }

void GLTFMaterial::set_double_sided(bool doubleSided) {
    _doubleSided = doubleSided;
}

void GLTFMaterial::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("pbrMetallicRoughness")) {
        _pbrMetallicRoughness.read(desc["pbrMetallicRoughness"], _parent);
        _hasPbrMetallicRoughness = true;
    }

    if (desc.contains("normalTexture")) {
        _normalTexture.read(desc["normalTexture"], _parent);
        _hasNormalTexture = true;
    }

    if (desc.contains("occlusionTexture")) {
        _occlusionTexture.read(desc["occlusionTexture"], _parent);
        _hasOcclusionTexture = true;
    }

    if (desc.contains("emissiveTexture")) {
        _emissiveTexture.read(desc["emissiveTexture"], _parent);
        _hasEmissiveTexture = true;
    }

    if (desc.contains("emissiveFactor")) {
        const auto& factor = desc["emissiveFactor"];
        if (factor.is_array() && factor.size() == 3) {
            _emissiveFactor = Vec3f(factor[0].get<F32>(), factor[1].get<F32>(),
                                    factor[2].get<F32>());
        }
    }

    if (desc.contains("alphaMode")) {
        const String modeStr = desc["alphaMode"].get<String>();
        if (modeStr == "OPAQUE") {
            _alphaMode = GLTF_ALPHA_OPAQUE;
        } else if (modeStr == "MASK") {
            _alphaMode = GLTF_ALPHA_MASK;
        } else if (modeStr == "BLEND") {
            _alphaMode = GLTF_ALPHA_BLEND;
        }
    }

    _alphaCutoff = desc.value("alphaCutoff", 0.5F);
    _doubleSided = desc.value("doubleSided", false);
}

auto GLTFMaterial::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (_hasPbrMetallicRoughness) {
        Json pbrJson = _pbrMetallicRoughness.write();
        if (!pbrJson.empty()) {
            json["pbrMetallicRoughness"] = pbrJson;
        }
    }

    if (_hasNormalTexture) {
        json["normalTexture"] = _normalTexture.write();
    }

    if (_hasOcclusionTexture) {
        json["occlusionTexture"] = _occlusionTexture.write();
    }

    if (_hasEmissiveTexture) {
        json["emissiveTexture"] = _emissiveTexture.write();
    }

    if (_emissiveFactor != Vec3f(0.0F, 0.0F, 0.0F)) {
        json["emissiveFactor"] = Json::array(
            {_emissiveFactor.x(), _emissiveFactor.y(), _emissiveFactor.z()});
    }

    if (_alphaMode != GLTF_ALPHA_OPAQUE) {
        if (_alphaMode == GLTF_ALPHA_MASK) {
            json["alphaMode"] = "MASK";
        } else if (_alphaMode == GLTF_ALPHA_BLEND) {
            json["alphaMode"] = "BLEND";
        }
    }

    if (_alphaMode == GLTF_ALPHA_MASK && _alphaCutoff != 0.5f) {
        json["alphaCutoff"] = _alphaCutoff;
    }

    if (_doubleSided) {
        json["doubleSided"] = _doubleSided;
    }

    return json;
}

auto GLTFMaterial::add_base_color_texture() -> GLTFTexture& {
    _hasPbrMetallicRoughness = true;
    auto& tex = _parent.add_texture();
    _pbrMetallicRoughness.baseColorTexture.texture = &tex;
    return tex;
};

auto GLTFMaterial::add_metal_roughness_texture() -> GLTFTexture& {
    _hasPbrMetallicRoughness = true;
    auto& tex = _parent.add_texture();
    _pbrMetallicRoughness.metallicRoughnessTexture.texture = &tex;
    return tex;
};
} // namespace nv