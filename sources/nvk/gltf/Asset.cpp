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

namespace gltf {

auto to_string(GLTFElementType type) -> std::string_view {
    switch (type) {
    case GLTF_ELEM_SCALAR:
        return "SCALAR";
    case GLTF_ELEM_VEC2:
        return "VEC2";
    case GLTF_ELEM_VEC3:
        return "VEC3";
    case GLTF_ELEM_VEC4:
        return "VEC4";
    case GLTF_ELEM_MAT2:
        return "MAT2";
    case GLTF_ELEM_MAT3:
        return "MAT3";
    case GLTF_ELEM_MAT4:
        return "MAT4";
    case GLTF_ELEM_UNKNOWN:
    default:
        return ""; // or throw if you prefer
    }
}

// Convert string to enum
auto to_element_type(std::string_view str) -> GLTFElementType {
    // if (str == "SCALAR")
    //     return GLTF_ELEM_SCALAR;
    // if (str == "VEC2")
    //     return GLTF_ELEM_VEC2;
    // if (str == "VEC3")
    //     return GLTF_ELEM_VEC3;
    // if (str == "VEC4")
    //     return GLTF_ELEM_VEC4;
    // if (str == "MAT2")
    //     return GLTF_ELEM_MAT2;
    // if (str == "MAT3")
    //     return GLTF_ELEM_MAT3;
    // if (str == "MAT4")
    //     return GLTF_ELEM_MAT4;

    // THROW_MSG("Invalid GLTF element string: {}", str);
    // return GLTF_ELEM_UNKNOWN;
    static const std::unordered_map<std::string_view, GLTFElementType> map = {
        {"SCALAR", GLTF_ELEM_SCALAR}, {"VEC2", GLTF_ELEM_VEC2},
        {"VEC3", GLTF_ELEM_VEC3},     {"VEC4", GLTF_ELEM_VEC4},
        {"MAT2", GLTF_ELEM_MAT2},     {"MAT3", GLTF_ELEM_MAT3},
        {"MAT4", GLTF_ELEM_MAT4},
    };

    auto it = map.find(str);
    NVCHK(it != map.end(), "Invalid GLTF element string: {}", str);
    // return (it != map.end()) ? it->second : GLTF_ELEM_UNKNOWN;
    return it->second;
}

auto to_string(GLTFAttributeType type) -> std::string_view {
    switch (type) {
    case GLTF_ATTR_POSITION:
        return "POSITION";
    case GLTF_ATTR_NORMAL:
        return "NORMAL";
    case GLTF_ATTR_TANGENT:
        return "TANGENT";
    case GLTF_ATTR_TEXCOORD0:
        return "TEXCOORD_0";
    case GLTF_ATTR_TEXCOORD1:
        return "TEXCOORD_1";
    case GLTF_ATTR_TEXCOORD2:
        return "TEXCOORD_2";
    case GLTF_ATTR_TEXCOORD3:
        return "TEXCOORD_3";
    case GLTF_ATTR_COLOR0:
        return "COLOR_0";
    case GLTF_ATTR_COLOR1:
        return "COLOR_1";
    case GLTF_ATTR_COLOR2:
        return "COLOR_2";
    case GLTF_ATTR_COLOR3:
        return "COLOR_3";
    case GLTF_ATTR_JOINTS0:
        return "JOINTS_0";
    case GLTF_ATTR_JOINTS1:
        return "JOINTS_1";
    case GLTF_ATTR_JOINTS2:
        return "JOINTS_2";
    case GLTF_ATTR_JOINTS3:
        return "JOINTS_3";
    case GLTF_ATTR_WEIGHTS0:
        return "WEIGHTS_0";
    case GLTF_ATTR_WEIGHTS1:
        return "WEIGHTS_1";
    case GLTF_ATTR_WEIGHTS2:
        return "WEIGHTS_2";
    case GLTF_ATTR_WEIGHTS3:
        return "WEIGHTS_3";
    case GLTF_ATTR_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

auto to_attribute_type(std::string_view str) -> GLTFAttributeType {
    static const std::unordered_map<std::string_view, GLTFAttributeType> map = {
        {"POSITION", GLTF_ATTR_POSITION},
        {"NORMAL", GLTF_ATTR_NORMAL},
        {"TANGENT", GLTF_ATTR_TANGENT},
        {"TEXCOORD_0", GLTF_ATTR_TEXCOORD0},
        {"TEXCOORD_1", GLTF_ATTR_TEXCOORD1},
        {"TEXCOORD_2", GLTF_ATTR_TEXCOORD2},
        {"TEXCOORD_3", GLTF_ATTR_TEXCOORD3},
        {"COLOR_0", GLTF_ATTR_COLOR0},
        {"COLOR_1", GLTF_ATTR_COLOR1},
        {"COLOR_2", GLTF_ATTR_COLOR2},
        {"COLOR_3", GLTF_ATTR_COLOR3},
        {"JOINTS_0", GLTF_ATTR_JOINTS0},
        {"JOINTS_1", GLTF_ATTR_JOINTS1},
        {"JOINTS_2", GLTF_ATTR_JOINTS2},
        {"JOINTS_3", GLTF_ATTR_JOINTS3},
        {"WEIGHTS_0", GLTF_ATTR_WEIGHTS0},
        {"WEIGHTS_1", GLTF_ATTR_WEIGHTS1},
        {"WEIGHTS_2", GLTF_ATTR_WEIGHTS2},
        {"WEIGHTS_3", GLTF_ATTR_WEIGHTS3},
    };

    auto it = map.find(str);
    NVCHK(it != map.end(), "Invalid GLTF attribute string: {}", str);
    // return (it != map.end()) ? it->second : GLTF_ATTR_UNKNOWN;
    return it->second;
}

auto get_element_component_count(GLTFElementType type) -> size_t {
    switch (type) {
    case GLTF_ELEM_SCALAR:
        return 1;
    case GLTF_ELEM_VEC2:
        return 2;
    case GLTF_ELEM_VEC3:
        return 3;
    case GLTF_ELEM_VEC4:
    case GLTF_ELEM_MAT2:
        return 4;
    case GLTF_ELEM_MAT3:
        return 9;
    case GLTF_ELEM_MAT4:
        return 16;
    default:
        return 0;
    }
}

auto get_attribute_size(GLTFElementType type, GLTFComponentType ctype)
    -> size_t {
    // Get component size in bytes
    size_t component_size = 0;
    switch (ctype) {
    case GLTF_COMP_I8:
    case GLTF_COMP_U8:
        component_size = 1;
        break;
    case GLTF_COMP_I16:
    case GLTF_COMP_U16:
        component_size = 2;
        break;
    case GLTF_COMP_U32:
    case GLTF_COMP_F32:
        component_size = 4;
        break;
    case GLTF_COMP_UNKNOWN:
    default:
        return 0;
    }

    // Get number of components
    size_t num_components = 0;
    switch (type) {
    case GLTF_ELEM_SCALAR:
        num_components = 1;
        break;
    case GLTF_ELEM_VEC2:
        num_components = 2;
        break;
    case GLTF_ELEM_VEC3:
        num_components = 3;
        break;
    case GLTF_ELEM_VEC4:
    case GLTF_ELEM_MAT2:
        num_components = 4;
        break;
    case GLTF_ELEM_MAT3:
        num_components = 9; // 3x3 = 9
        break;
    case GLTF_ELEM_MAT4:
        num_components = 16; // 4x4 = 16
        break;
    case GLTF_ELEM_UNKNOWN:
    default:
        return 0;
    }

    return component_size * num_components;
}

} // namespace gltf

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

    if (asset.contains("accessors")) {
        // Create the accessors:
        for (const auto& desc : asset["accessors"]) {
            add_accessor().read(desc);
        }
    }

    if (asset.contains("meshes")) {
        // Create the meshes:
        for (const auto& desc : asset["meshes"]) {
            add_mesh().read(desc);
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

    // Write the accessors:
    if (!_accessors.empty()) {
        Json accs;
        for (const auto& acc : _accessors) {
            accs.push_back(acc->write());
        }
        data["accessors"] = std::move(accs);
    }

    // Write the meshes:
    if (!_meshes.empty()) {
        Json meshes;
        for (const auto& mesh : _meshes) {
            meshes.push_back(mesh->write());
        }
        data["meshes"] = std::move(meshes);
    }

    nv::write_json_file(path, data);
}

auto GLTFAsset::intern_string(std::string_view str) -> char* {
    if (str.empty())
        return nullptr;
    _owned.strings.emplace_back(str);
    return (char*)_owned.strings.back().c_str();
}

auto GLTFAsset::add_buffer(size_t size, String name) -> GLTFBuffer& {
    auto buf = nv::create<GLTFBuffer>(*this, _buffers.size());
    _buffers.emplace_back(buf);
    buf->set_name(std::move(name));
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

auto GLTFAsset::add_bufferview(String name) -> GLTFBufferView& {
    auto view = nv::create<GLTFBufferView>(*this, _bufferViews.size());
    view->set_name(std::move(name));
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
    NVCHK(idx < _bufferViews.size(), "Out of range bufferview index {}", idx);
    return *_bufferViews[idx];
};

auto GLTFAsset::get_bufferview(U32 idx) const -> const GLTFBufferView& {
    NVCHK(idx < _bufferViews.size(), "Out of range bufferview index {}", idx);
    return *_bufferViews[idx];
};

auto GLTFAsset::add_accessor(String name) -> GLTFAccessor& {
    auto obj = nv::create<GLTFAccessor>(*this, _accessors.size());
    obj->set_name(std::move(name));
    _accessors.emplace_back(obj);
    return *obj;
};
auto GLTFAsset::get_accessor(U32 idx) -> GLTFAccessor& {
    NVCHK(idx < _accessors.size(), "Out of range accessor index {}", idx);
    return *_accessors[idx];
};
auto GLTFAsset::get_accessor(U32 idx) const -> const GLTFAccessor& {
    NVCHK(idx < _accessors.size(), "Out of range accessor index {}", idx);
    return *_accessors[idx];
};

auto GLTFAsset::add_accessor(GLTFBufferView& view, GLTFElementType etype,
                             GLTFComponentType ctype, U32 count, U32 offset)
    -> GLTFAccessor& {
    auto& acc = add_accessor();
    acc.set_buffer_view(view);
    acc.set_element_type(etype);
    acc.set_component_type(ctype);
    acc.set_count(count);
    acc.set_offset(offset);
    return acc;
};

auto GLTFAsset::add_mesh(String name) -> GLTFMesh& {
    auto obj = nv::create<GLTFMesh>(*this, _meshes.size());
    obj->set_name(std::move(name));
    _meshes.emplace_back(obj);
    return *obj;
};

auto GLTFAsset::get_mesh(U32 idx) -> GLTFMesh& {
    NVCHK(idx < _meshes.size(), "Out of range mesh index {}", idx);
    return *_meshes[idx];
};

auto GLTFAsset::get_mesh(U32 idx) const -> const GLTFMesh& {
    NVCHK(idx < _meshes.size(), "Out of range mesh index {}", idx);
    return *_meshes[idx];
};

} // namespace nv