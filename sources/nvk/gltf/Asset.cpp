#include <nvk/gltf/Accessor.h>
#include <nvk/gltf/Animation.h>
#include <nvk/gltf/Asset.h>
#include <nvk/gltf/Buffer.h>
#include <nvk/gltf/BufferView.h>
#include <nvk/gltf/Camera.h>
#include <nvk/gltf/Image.h>
#include <nvk/gltf/Material.h>
#include <nvk/gltf/Mesh.h>
#include <nvk/gltf/Node.h>
#include <nvk/gltf/Primitive.h>
#include <nvk/gltf/Scene.h>
#include <nvk/gltf/Skin.h>
#include <nvk/gltf/Texture.h>

#define CGLTF_IMPLEMENTATION
// #include <external/cgltf.h>

#define CGLTF_WRITE_IMPLEMENTATION
#include <external/cgltf_write.h>

namespace nv {

//==============================================================================
// GLTFAsset Implementation
//==============================================================================
auto GLTFAsset::decode_data_uri(const String& uri, size_t expected_size) const
    -> U8Vector {
    // Find the base64 data after "data:application/octet-stream;base64,"
    size_t comma_pos = uri.find(',');
    if (comma_pos == String::npos) {
        throw std::runtime_error("Invalid data URI format");
    }

    String base64_data = uri.substr(comma_pos + 1);
    U8Vector decoded = base64_decode(base64_data);

    if (decoded.size() != expected_size) {
        throw std::runtime_error("Decoded data size mismatch");
    }

    return decoded;
}

auto GLTFAsset::load_external_buffer(const String& uri,
                                     size_t expected_size) const -> U8Vector {
    // Resolve path relative to glTF file location
    String full_path = resolve_path(uri);

    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open buffer file: " + full_path);
    }

    U8Vector data(expected_size);
    file.read(reinterpret_cast<char*>(data.data()), expected_size);

    if (file.gcount() != static_cast<std::streamsize>(expected_size)) {
        throw std::runtime_error("Buffer file size mismatch");
    }

    return data;
}

GLTFAsset::GLTFAsset() {}

GLTFAsset::GLTFAsset(const char* path, bool load_buffers) {
    load(path, load_buffers);
}

GLTFAsset::~GLTFAsset() { clear(); }

void GLTFAsset::load(const char* path, bool load_buffers) {
    clear();

    auto data = read_json_file(path);
    auto asset = data["asset"];
    _version = asset["version"];

    if (asset.contains("generator")) {
        _generator = asset["generator"];
    }
    if (asset.contains("copyright")) {
        _copyright = asset["copyright"];
    }

    if (asset.contains("buffers")) {
        // Create the buffers:
        for (const auto& desc : asset["buffers"]) {
            add_buffer().read(desc);
        }
    }

    if (asset.contains("bufferViews")) {
        // Create the bufferViews:
        for (const auto& desc : asset["bufferViews"]) {
            add_bufferview().read(desc);
        }
    }
}

void GLTFAsset::save(const char* path) const {
    // Asset component:
    Json asset{{"version", _version}};
    if (!_generator.empty()) {
        asset["generator"] = _generator;
    }
    if (!_copyright.empty()) {
        asset["copyright"] = _copyright;
    }

    // Prepare a json object
    Json data{{"asset", std::move(asset)}};

    // Write the buffers:
    if (!_buffers.empty()) {
        Json buffers;
        for (const auto& buf : _buffers) {
            buffers.push_back(buf->write());
        }
        data["buffers"] = std::move(buffers);
    }

    // Write the bufferViews:
    if (!_bufferViews.empty()) {
        Json views;
        for (const auto& view : _bufferViews) {
            views.push_back(view->write());
        }
        data["bufferViews"] = std::move(views);
    }

    nv::write_json_file(path, data);
}

auto GLTFAsset::intern_string(std::string_view str) -> char* {
    if (str.empty())
        return nullptr;
    _owned.strings.emplace_back(str);
    return (char*)_owned.strings.back().c_str();
}

auto GLTFAsset::add_buffer(size_t size) -> GLTFBuffer& {
    auto buf = nv::create<GLTFBuffer>(*this, _buffers.size());
    _buffers.emplace_back(buf);
    buf->resize(size);
    return *buf;
}

auto GLTFAsset::get_buffer(U32 idx) -> GLTFBuffer& {
    NVCHK(idx < _buffers.size(), "Out of range buffer index {}", idx);
    return *_buffers[idx];
}

auto GLTFAsset::get_buffer(U32 idx) const -> const GLTFBuffer& {
    NVCHK(idx < _buffers.size(), "Out of range buffer index {}", idx);
    return *_buffers[idx];
}
auto GLTFAsset::resolve_path(const String& uri) const -> String { return uri; };

auto GLTFAsset::create() -> RefPtr<GLTFAsset> {
    return nv::create<GLTFAsset>();
}
void GLTFAsset::set_copyright(String copyright) {
    _copyright = std::move(copyright);
};
void GLTFAsset::set_generator(String gen) { _generator = std::move(gen); };
auto GLTFAsset::copyright() const -> const String& { return _copyright; };
auto GLTFAsset::version() const -> const String& { return _version; };
auto GLTFAsset::generator() const -> const String& { return _generator; };

void GLTFAsset::clear() {
    logDEBUG("GLTFAsset: Should clear everything here.");
};

auto GLTFAsset::add_bufferview() -> GLTFBufferView& {
    auto view = nv::create<GLTFBufferView>(*this, _bufferViews.size());
    _bufferViews.emplace_back(view);
    return *view;
};

auto GLTFAsset::add_bufferview(GLTFBuffer& buf, U32 offset, U32 size)
    -> GLTFBufferView& {
    auto& view = add_bufferview();
    view.set_buffer(buf);
    view.set_offset(offset);
    if (size == 0) {
        size = buf.size() - offset;
    }
    view.set_size(size);
    return view;
};

auto GLTFAsset::get_bufferview(U32 idx) -> GLTFBufferView& {
    NVCHK(idx < _bufferViews.size(), "Out of range buffer index {}", idx);
    return *_bufferViews[idx];
};

auto GLTFAsset::get_bufferview(U32 idx) const -> const GLTFBufferView& {
    NVCHK(idx < _bufferViews.size(), "Out of range buffer index {}", idx);
    return *_bufferViews[idx];
};

} // namespace nv