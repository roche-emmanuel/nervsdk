#include <nvk_gltf.h>

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
        THROW_MSG("Unsupported GLTF element type: {}", (int)type);
        return "";
    }
}

// Convert string to enum
auto to_element_type(std::string_view str) -> GLTFElementType {
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

auto get_data_type(GLTFElementType type, GLTFComponentType ctype) -> DataType {
    if (ctype == GLTF_COMP_F32) {
        switch (type) {
        case GLTF_ELEM_SCALAR:
            return DTYPE_F32;
        case GLTF_ELEM_VEC2:
            return DTYPE_VEC2F;
        case GLTF_ELEM_VEC3:
            return DTYPE_VEC3F;
        case GLTF_ELEM_VEC4:
            return DTYPE_VEC4F;
        case GLTF_ELEM_MAT2:
            return DTYPE_MAT2F;
        case GLTF_ELEM_MAT3:
            return DTYPE_MAT3F;
        case GLTF_ELEM_MAT4:
            return DTYPE_MAT4F;
        default:
            return DTYPE_UNKNOWN;
        }
    }
    if (ctype == GLTF_COMP_U32) {
        switch (type) {
        case GLTF_ELEM_SCALAR:
            return DTYPE_U32;
        case GLTF_ELEM_VEC2:
            return DTYPE_VEC2U;
        case GLTF_ELEM_VEC3:
            return DTYPE_VEC3U;
        case GLTF_ELEM_VEC4:
            return DTYPE_VEC4U;
        default:
            return DTYPE_UNKNOWN;
        }
    }

    return DTYPE_UNKNOWN;
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

void GLTFAsset::load(const char* path, bool load_buffers,
                     bool forceAllowSystem) {
    String path_str(path);

    // Check file extension to determine format
    if (path_str.size() >= 4 &&
        path_str.substr(path_str.size() - 4) == ".glb") {
        load_glb(path, forceAllowSystem);
    } else {
        load_gltf(path, load_buffers, forceAllowSystem);
    }
}

void GLTFAsset::load_from_json(const Json& data, U8Vector* glb_bin_chunk) {
    auto asset = data["asset"];
    _version = asset["version"];

    if (asset.contains("generator"))
        _generator = asset["generator"];

    if (asset.contains("copyright"))
        _copyright = asset["copyright"];

    // Buffers
    if (data.contains("buffers")) {
        bool first_buffer = true;

        for (const auto& desc : data["buffers"]) {
            auto& buffer = add_buffer();
            buffer.read(desc);

            // If this is a GLB and we have BIN data
            if (first_buffer) {
                if (glb_bin_chunk != nullptr && !glb_bin_chunk->empty()) {
                    buffer.set_data(std::move(*glb_bin_chunk));
                }
                first_buffer = false;
            }
        }
    }

    if (data.contains("bufferViews"))
        for (const auto& desc : data["bufferViews"])
            add_bufferview().read(desc);

    if (data.contains("accessors"))
        for (const auto& desc : data["accessors"])
            add_accessor().read(desc);

    if (data.contains("images"))
        for (const auto& desc : data["images"])
            add_image().read(desc);

    if (data.contains("textures"))
        for (const auto& desc : data["textures"])
            add_texture().read(desc);

    if (data.contains("materials"))
        for (const auto& desc : data["materials"])
            add_material().read(desc);

    if (data.contains("meshes"))
        for (const auto& desc : data["meshes"])
            add_mesh().read(desc);

    if (data.contains("nodes"))
        for (const auto& desc : data["nodes"])
            add_node().read(desc);

    if (data.contains("scenes")) {
        for (const auto& desc : data["scenes"])
            add_scene().read(desc);

        if (data.contains("scene")) {
            U32 idx = data["scene"].get<U32>();
            _defaultScene = &get_scene(idx);
        }
    }
}

void GLTFAsset::load_gltf(const char* path, bool load_buffers,
                          bool forceAllowSystem) {
    clear();

    auto data = read_json_file(path, forceAllowSystem);
    load_from_json(data);
}

void GLTFAsset::load_glb_from_memory(const U8Vector& content) {
    clear();
    NVCHK(content.size() >= sizeof(GLBHeader),
          "File too small to be valid GLB");

    const uint8_t* ptr = content.data();
    const uint8_t* end = ptr + content.size();

    // --- Read header ---
    const GLBHeader* header = reinterpret_cast<const GLBHeader*>(ptr);

    NVCHK(header->magic == GLB_MAGIC, "Invalid GLB magic number");

    NVCHK(header->version == GLB_VERSION, "Unsupported GLB version");

    ptr += sizeof(GLBHeader);

    // --- Read JSON chunk header ---
    NVCHK(ptr + sizeof(GLBChunkHeader) <= end,
          "Unexpected end of file (JSON chunk header)");

    const GLBChunkHeader* json_chunk =
        reinterpret_cast<const GLBChunkHeader*>(ptr);

    NVCHK(json_chunk->type == GLB_CHUNK_JSON, "Expected JSON chunk");

    ptr += sizeof(GLBChunkHeader);

    // --- Read JSON chunk data ---
    NVCHK(ptr + json_chunk->length <= end,
          "Unexpected end of file (JSON chunk data)");

    const char* json_begin = reinterpret_cast<const char*>(ptr);
    const char* json_end = json_begin + json_chunk->length;

    Json data = Json::parse(json_begin, json_end);

    ptr += json_chunk->length;

    // --- Read BIN chunk (optional) ---
    U8Vector bin_data;

    if (ptr + sizeof(GLBChunkHeader) <= end) {
        const GLBChunkHeader* bin_chunk =
            reinterpret_cast<const GLBChunkHeader*>(ptr);

        ptr += sizeof(GLBChunkHeader);

        if (bin_chunk->type == GLB_CHUNK_BIN) {
            NVCHK((ptr + bin_chunk->length) <= end,
                  "Unexpected end of file (BIN chunk)");

            bin_data.resize(bin_chunk->length);
            std::memcpy(bin_data.data(), ptr, bin_chunk->length);

            ptr += bin_chunk->length;
        }
    }

    // --- Unified JSON loading ---
    load_from_json(data, bin_data.empty() ? nullptr : &bin_data);
}

void GLTFAsset::load_glb(const char* path, bool forceAllowSystem) {
    U8Vector content = nv::read_virtual_binary_file(path, forceAllowSystem);
    load_glb_from_memory(content);
}

void GLTFAsset::update_all_position_bounds() const {
    for (const auto& mesh : _meshes) {
        mesh->update_position_bounds();
    }
}

auto GLTFAsset::write_json() const -> Json {
    // Compute the position bounds:
    update_all_position_bounds();

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

    // Write the images:
    if (!_images.empty()) {
        Json imgs;
        for (const auto& img : _images) {
            imgs.push_back(img->write());
        }
        data["images"] = std::move(imgs);
    }

    // Write the textures:
    if (!_textures.empty()) {
        Json texs;
        for (const auto& tex : _textures) {
            texs.push_back(tex->write());
        }
        data["textures"] = std::move(texs);
    }

    // Write the materials:
    if (!_materials.empty()) {
        Json mats;
        for (const auto& mat : _materials) {
            mats.push_back(mat->write());
        }
        data["materials"] = std::move(mats);
    }

    // Write the meshes:
    if (!_meshes.empty()) {
        Json meshes;
        for (const auto& mesh : _meshes) {
            meshes.push_back(mesh->write());
        }
        data["meshes"] = std::move(meshes);
    }

    // Write the nodes:
    if (!_nodes.empty()) {
        Json nodes;
        for (const auto& node : _nodes) {
            nodes.push_back(node->write());
        }
        data["nodes"] = std::move(nodes);
    }

    // Write the scenes:
    if (!_scenes.empty()) {
        Json scenes;
        for (const auto& scene : _scenes) {
            scenes.push_back(scene->write());
        }
        data["scenes"] = std::move(scenes);

        if (_defaultScene != nullptr) {
            data["scene"] = _defaultScene->index();
        }
    }

    return data;
}

void GLTFAsset::save(const char* path) const {
    String path_str(path);

    // Check file extension to determine format
    if (path_str.size() >= 4 &&
        path_str.substr(path_str.size() - 4) == ".glb") {
        save_glb(path);
    } else {
        save_gltf(path);
    }
}

void GLTFAsset::save_gltf(const char* path) const {
    auto data = write_json();
    nv::write_json_file(path, data);
}

void GLTFAsset::save_glb(const char* path) const {
    // Before writing the data we should disable base64 data uri writing in the
    // first buffer:
    if (!_buffers.empty()) {
        _buffers[0]->set_write_base64(false);
    }

    auto json_data = write_json();
    String json_str = json_data.dump();

    // Pad JSON to 4-byte alignment with spaces
    while (json_str.size() % 4 != 0) {
        json_str += ' ';
    }

    // Get binary buffer data (first buffer only for GLB)
    U8Vector bin_data;
    if (!_buffers.empty()) {
        const auto& buffer = _buffers[0];
        bin_data.assign(buffer->data(), buffer->data() + buffer->size());

        // Pad BIN to 4-byte alignment with zeros
        while (bin_data.size() % 4 != 0) {
            bin_data.push_back(0);
        }
    }

    // Calculate sizes
    uint32_t json_length = static_cast<uint32_t>(json_str.size());
    uint32_t bin_length = static_cast<uint32_t>(bin_data.size());
    uint32_t total_length =
        sizeof(GLBHeader) + sizeof(GLBChunkHeader) + json_length;

    if (bin_length > 0) {
        total_length += sizeof(GLBChunkHeader) + bin_length;
    }

    // Write file
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to create GLB file");
    }

    // Write GLB header
    GLBHeader header{
        .magic = GLB_MAGIC, .version = GLB_VERSION, .length = total_length};
    file.write(reinterpret_cast<const char*>(&header), sizeof(GLBHeader));

    // Write JSON chunk
    GLBChunkHeader json_chunk{.length = json_length, .type = GLB_CHUNK_JSON};
    file.write(reinterpret_cast<const char*>(&json_chunk),
               sizeof(GLBChunkHeader));
    file.write(json_str.data(), json_length);

    // Write BIN chunk if we have buffer data
    if (bin_length > 0) {
        GLBChunkHeader bin_chunk{.length = bin_length, .type = GLB_CHUNK_BIN};
        file.write(reinterpret_cast<const char*>(&bin_chunk),
                   sizeof(GLBChunkHeader));
        file.write(reinterpret_cast<const char*>(bin_data.data()), bin_length);
    }
}

auto GLTFAsset::save_to_memory() const -> String {
    auto data = write_json();
    return data.dump();
};

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
    _generator = "NervSDK GLTF Asset";
    _version = "2.0";
    _copyright = "";

    _numElements = 0;

    _defaultScene.reset();

    _images.clear();
    _samplers.clear();
    _textures.clear();
    _materials.clear();
    _scenes.clear();
    _nodes.clear();
    _meshes.clear();
    _accessors.clear();
    _bufferViews.clear();
    _buffers.clear();
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

auto GLTFAsset::add_node(String name) -> GLTFNode& {
    auto obj = nv::create<GLTFNode>(*this, _nodes.size());
    obj->set_name(std::move(name));
    _nodes.emplace_back(obj);
    return *obj;
};

auto GLTFAsset::get_node(U32 idx) -> GLTFNode& {
    NVCHK(idx < _nodes.size(), "Out of range node index {}", idx);
    return *_nodes[idx];
};

auto GLTFAsset::get_node(U32 idx) const -> const GLTFNode& {
    NVCHK(idx < _nodes.size(), "Out of range node index {}", idx);
    return *_nodes[idx];
};

auto GLTFAsset::add_scene(String name) -> GLTFScene& {
    auto obj = nv::create<GLTFScene>(*this, _scenes.size());
    obj->set_name(std::move(name));
    _scenes.emplace_back(obj);
    return *obj;
};

auto GLTFAsset::get_scene(U32 idx) -> GLTFScene& {
    NVCHK(idx < _scenes.size(), "Out of range scene index {}", idx);
    return *_scenes[idx];
};

auto GLTFAsset::get_scene(U32 idx) const -> const GLTFScene& {
    NVCHK(idx < _scenes.size(), "Out of range scene index {}", idx);
    return *_scenes[idx];
};

auto GLTFAsset::default_scene() const -> RefPtr<GLTFScene> {
    return _defaultScene;
}
void GLTFAsset::set_default_scene(GLTFScene* scene) { _defaultScene = scene; }

auto GLTFAsset::add_material(String name) -> GLTFMaterial& {
    auto obj = nv::create<GLTFMaterial>(*this, _materials.size());
    obj->set_name(std::move(name));
    _materials.emplace_back(obj);
    return *obj;
};

auto GLTFAsset::get_material(U32 idx) -> GLTFMaterial& {
    NVCHK(idx < _materials.size(), "Out of range material index {}", idx);
    return *_materials[idx];
};

auto GLTFAsset::get_material(U32 idx) const -> const GLTFMaterial& {
    NVCHK(idx < _materials.size(), "Out of range material index {}", idx);
    return *_materials[idx];
};

auto GLTFAsset::add_texture(String name) -> GLTFTexture& {
    auto obj = nv::create<GLTFTexture>(*this, _textures.size());
    obj->set_name(std::move(name));
    _textures.emplace_back(obj);
    return *obj;
};

auto GLTFAsset::get_texture(U32 idx) -> GLTFTexture& {
    NVCHK(idx < _textures.size(), "Out of range texture index {}", idx);
    return *_textures[idx];
};

auto GLTFAsset::get_texture(U32 idx) const -> const GLTFTexture& {
    NVCHK(idx < _textures.size(), "Out of range texture index {}", idx);
    return *_textures[idx];
};

auto GLTFAsset::add_sampler(String name) -> GLTFSampler& {
    auto obj = nv::create<GLTFSampler>(*this, _samplers.size());
    obj->set_name(std::move(name));
    _samplers.emplace_back(obj);
    return *obj;
};
auto GLTFAsset::get_sampler(U32 idx) -> GLTFSampler& {
    NVCHK(idx < _samplers.size(), "Out of range sampler index {}", idx);
    return *_samplers[idx];
};
auto GLTFAsset::get_sampler(U32 idx) const -> const GLTFSampler& {
    NVCHK(idx < _samplers.size(), "Out of range sampler index {}", idx);
    return *_samplers[idx];
};
auto GLTFAsset::add_image(String name) -> GLTFImage& {
    auto obj = nv::create<GLTFImage>(*this, _images.size());
    obj->set_name(std::move(name));
    _images.emplace_back(obj);
    return *obj;
};

auto GLTFAsset::get_image(U32 idx) -> GLTFImage& {
    NVCHK(idx < _images.size(), "Out of range image index {}", idx);
    return *_images[idx];
};

auto GLTFAsset::get_image(U32 idx) const -> const GLTFImage& {
    NVCHK(idx < _images.size(), "Out of range image index {}", idx);
    return *_images[idx];
};

} // namespace nv