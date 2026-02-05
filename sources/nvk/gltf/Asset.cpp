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

GLTFAsset::GLTFAsset() : _data(nullptr), _loaded_from_file(false) {
    _initialize_empty();
}

GLTFAsset::GLTFAsset(const char* path, bool load_buffers)
    : _data(nullptr), _loaded_from_file(false) {
    load(path, load_buffers);
}

GLTFAsset::~GLTFAsset() { clear(); }

GLTFAsset::GLTFAsset(GLTFAsset&& other) noexcept
    : _data(other._data), _owned(std::move(other._owned)),
      _loaded_from_file(other._loaded_from_file) {
    other._data = nullptr;
}

auto GLTFAsset::operator=(GLTFAsset&& other) noexcept -> GLTFAsset& {
    if (this != &other) {
        clear();
        _data = other._data;
        _owned = std::move(other._owned);
        _loaded_from_file = other._loaded_from_file;
        other._data = nullptr;
    }
    return *this;
}

void GLTFAsset::load(const char* path, bool load_buffers) {
    clear();

    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, path, &_data);

    if (result != cgltf_result_success) {
        throw GLTFLoadException("Failed to parse GLTF file: " +
                                std::string(path));
    }

    if (load_buffers) {
        result = cgltf_load_buffers(&options, _data, path);
        if (result != cgltf_result_success) {
            cgltf_free(_data);
            _data = nullptr;
            throw GLTFLoadException("Failed to load buffers for GLTF file: " +
                                    std::string(path));
        }
    }

    _loaded_from_file = true;
}

void GLTFAsset::save(const char* path) const {
    if (_data == nullptr) {
        throw GLTFException("Cannot save empty GLTF asset");
    }

    cgltf_options options = {};
    cgltf_result result = cgltf_write_file(&options, path, _data);

    if (result != cgltf_result_success) {
        throw GLTFException("Failed to write GLTF file: " + std::string(path));
    }
}

void GLTFAsset::validate() const {
    if (_data == nullptr) {
        throw GLTFValidationException("Cannot validate empty GLTF asset");
    }

    cgltf_result result = cgltf_validate(_data);
    if (result != cgltf_result_success) {
        throw GLTFValidationException("GLTF validation failed");
    }
}

void GLTFAsset::clear() {
    if (_data) {
        if (_loaded_from_file) {
            cgltf_free(_data);
        } else {
            // For constructed assets, we need to free our owned data
            delete _data;
        }
        _data = nullptr;
    }

    _owned = OwnedElements();
    _loaded_from_file = false;
}

void GLTFAsset::_initialize_empty() {
    _data = new cgltf_data();
    std::memset(_data, 0, sizeof(cgltf_data));

    // Set required asset info
    _data->asset.version = _intern_string("2.0");
    _data->asset.generator = _intern_string("GLTFAsset");

    _loaded_from_file = false;
}

auto GLTFAsset::_intern_string(std::string_view str) -> char* {
    if (str.empty())
        return nullptr;
    _owned.strings.emplace_back(str);
    return (char*)_owned.strings.back().c_str();
}

void GLTFAsset::_rebuild_pointers() {
    if (_data == nullptr)
        return;

    // Update all array pointers to point to owned vectors
    _data->meshes = _owned.meshes.empty() ? nullptr : _owned.meshes.data();
    _data->meshes_count = _owned.meshes.size();

    _data->nodes = _owned.nodes.empty() ? nullptr : _owned.nodes.data();
    _data->nodes_count = _owned.nodes.size();

    _data->scenes = _owned.scenes.empty() ? nullptr : _owned.scenes.data();
    _data->scenes_count = _owned.scenes.size();

    _data->buffers = _owned.buffers.empty() ? nullptr : _owned.buffers.data();
    _data->buffers_count = _owned.buffers.size();

    _data->buffer_views =
        _owned.buffer_views.empty() ? nullptr : _owned.buffer_views.data();
    _data->buffer_views_count = _owned.buffer_views.size();

    _data->accessors =
        _owned.accessors.empty() ? nullptr : _owned.accessors.data();
    _data->accessors_count = _owned.accessors.size();

    _data->materials =
        _owned.materials.empty() ? nullptr : _owned.materials.data();
    _data->materials_count = _owned.materials.size();

    _data->textures =
        _owned.textures.empty() ? nullptr : _owned.textures.data();
    _data->textures_count = _owned.textures.size();

    _data->images = _owned.images.empty() ? nullptr : _owned.images.data();
    _data->images_count = _owned.images.size();

    _data->samplers =
        _owned.samplers.empty() ? nullptr : _owned.samplers.data();
    _data->samplers_count = _owned.samplers.size();

    _data->animations =
        _owned.animations.empty() ? nullptr : _owned.animations.data();
    _data->animations_count = _owned.animations.size();

    _data->skins = _owned.skins.empty() ? nullptr : _owned.skins.data();
    _data->skins_count = _owned.skins.size();

    _data->cameras = _owned.cameras.empty() ? nullptr : _owned.cameras.data();
    _data->cameras_count = _owned.cameras.size();
}

void GLTFAsset::_reserve_capacity(size_t estimate) {
    _owned.meshes.reserve(estimate);
    _owned.nodes.reserve(estimate * 2);
    _owned.scenes.reserve(std::max(size_t(1), estimate / 10));
    _owned.buffers.reserve(estimate);
    _owned.buffer_views.reserve(estimate * 2);
    _owned.accessors.reserve(estimate * 3);
    _owned.materials.reserve(estimate);
    _owned.strings.reserve(estimate * 5);
}

//==============================================================================
// Add element methods
//==============================================================================

auto GLTFAsset::add_mesh(std::string_view name) -> GLTFMesh& {
    if (!_data)
        _initialize_empty();

    _owned.meshes.emplace_back();
    cgltf_mesh& mesh = _owned.meshes.back();
    std::memset(&mesh, 0, sizeof(cgltf_mesh));

    if (!name.empty()) {
        mesh.name = _intern_string(name);
    }

    _rebuild_pointers();
    return *reinterpret_cast<GLTFMesh*>(&mesh);
}

auto GLTFAsset::add_node(std::string_view name) -> GLTFNode& {
    if (!_data)
        _initialize_empty();

    _owned.nodes.emplace_back();
    cgltf_node& node = _owned.nodes.back();
    std::memset(&node, 0, sizeof(cgltf_node));

    if (!name.empty()) {
        node.name = _intern_string(name);
    }

    // Initialize default transform (identity)
    node.rotation[3] = 1.0f; // w component of quaternion
    node.scale[0] = node.scale[1] = node.scale[2] = 1.0f;

    _rebuild_pointers();
    return *reinterpret_cast<GLTFNode*>(&node);
}

auto GLTFAsset::add_scene(std::string_view name) -> GLTFScene& {
    if (!_data)
        _initialize_empty();

    _owned.scenes.emplace_back();
    cgltf_scene& scene = _owned.scenes.back();
    std::memset(&scene, 0, sizeof(cgltf_scene));

    if (!name.empty()) {
        scene.name = _intern_string(name);
    }

    // If this is the first scene, make it default
    if (_owned.scenes.size() == 1) {
        _data->scene = &scene;
    }

    _rebuild_pointers();
    return *reinterpret_cast<GLTFScene*>(&scene);
}

auto GLTFAsset::add_buffer(size_t size) -> GLTFBuffer& {
    if (!_data)
        _initialize_empty();

    _owned.buffers.emplace_back();
    cgltf_buffer& buffer = _owned.buffers.back();
    std::memset(&buffer, 0, sizeof(cgltf_buffer));

    buffer.size = size;

    // Allocate buffer data storage
    _owned.buffer_data.emplace_back(size, 0);
    buffer.data = _owned.buffer_data.back().data();

    _rebuild_pointers();
    return *reinterpret_cast<GLTFBuffer*>(&buffer);
}

auto GLTFAsset::add_buffer_view(size_t buffer_index, size_t offset,
                                size_t length) -> GLTFBufferView& {
    if (!_data)
        _initialize_empty();

    if (buffer_index >= _owned.buffers.size()) {
        throw GLTFException("Buffer index out of range");
    }

    _owned.buffer_views.emplace_back();
    cgltf_buffer_view& view = _owned.buffer_views.back();
    std::memset(&view, 0, sizeof(cgltf_buffer_view));

    view.buffer = &_owned.buffers[buffer_index];
    view.offset = offset;
    view.size = length;

    _rebuild_pointers();
    return *reinterpret_cast<GLTFBufferView*>(&view);
}

auto GLTFAsset::add_accessor(cgltf_type type,
                             cgltf_component_type component_type, size_t count)
    -> GLTFAccessor& {
    if (!_data)
        _initialize_empty();

    _owned.accessors.emplace_back();
    cgltf_accessor& accessor = _owned.accessors.back();
    std::memset(&accessor, 0, sizeof(cgltf_accessor));

    accessor.type = type;
    accessor.component_type = component_type;
    accessor.count = count;

    _rebuild_pointers();
    return *reinterpret_cast<GLTFAccessor*>(&accessor);
}

auto GLTFAsset::add_material(std::string_view name) -> GLTFMaterial& {
    if (!_data)
        _initialize_empty();

    _owned.materials.emplace_back();
    cgltf_material& material = _owned.materials.back();
    std::memset(&material, 0, sizeof(cgltf_material));

    if (!name.empty()) {
        material.name = _intern_string(name);
    }

    // Initialize with defaults
    material.has_pbr_metallic_roughness = 1;
    material.pbr_metallic_roughness.base_color_factor[0] = 1.0f;
    material.pbr_metallic_roughness.base_color_factor[1] = 1.0f;
    material.pbr_metallic_roughness.base_color_factor[2] = 1.0f;
    material.pbr_metallic_roughness.base_color_factor[3] = 1.0f;
    material.pbr_metallic_roughness.metallic_factor = 1.0f;
    material.pbr_metallic_roughness.roughness_factor = 1.0f;
    material.alpha_mode = cgltf_alpha_mode_opaque;
    material.alpha_cutoff = 0.5f;

    _rebuild_pointers();
    return *reinterpret_cast<GLTFMaterial*>(&material);
}

auto GLTFAsset::add_texture() -> GLTFTexture& {
    if (!_data)
        _initialize_empty();

    _owned.textures.emplace_back();
    cgltf_texture& texture = _owned.textures.back();
    std::memset(&texture, 0, sizeof(cgltf_texture));

    _rebuild_pointers();
    return *reinterpret_cast<GLTFTexture*>(&texture);
}

auto GLTFAsset::add_image(std::string_view uri) -> GLTFImage& {
    if (!_data)
        _initialize_empty();

    _owned.images.emplace_back();
    cgltf_image& image = _owned.images.back();
    std::memset(&image, 0, sizeof(cgltf_image));

    if (!uri.empty()) {
        image.uri = _intern_string(uri);
    }

    _rebuild_pointers();
    return *reinterpret_cast<GLTFImage*>(&image);
}

auto GLTFAsset::add_animation(std::string_view name) -> GLTFAnimation& {
    if (!_data)
        _initialize_empty();

    _owned.animations.emplace_back();
    cgltf_animation& animation = _owned.animations.back();
    std::memset(&animation, 0, sizeof(cgltf_animation));

    if (!name.empty()) {
        animation.name = _intern_string(name);
    }

    _rebuild_pointers();
    return *reinterpret_cast<GLTFAnimation*>(&animation);
}

auto GLTFAsset::add_skin(std::string_view name) -> GLTFSkin& {
    if (!_data)
        _initialize_empty();

    _owned.skins.emplace_back();
    cgltf_skin& skin = _owned.skins.back();
    std::memset(&skin, 0, sizeof(cgltf_skin));

    if (!name.empty()) {
        skin.name = _intern_string(name);
    }

    _rebuild_pointers();
    return *reinterpret_cast<GLTFSkin*>(&skin);
}

auto GLTFAsset::add_camera(std::string_view name) -> GLTFCamera& {
    if (!_data)
        _initialize_empty();

    _owned.cameras.emplace_back();
    cgltf_camera& camera = _owned.cameras.back();
    std::memset(&camera, 0, sizeof(cgltf_camera));

    if (!name.empty()) {
        camera.name = _intern_string(name);
    }

    _rebuild_pointers();
    return *reinterpret_cast<GLTFCamera*>(&camera);
}

//==============================================================================
// Accessors for const spans
//==============================================================================

auto GLTFAsset::meshes() const -> std::span<const GLTFMesh> {
    return std::span<const GLTFMesh>(
        reinterpret_cast<const GLTFMesh*>(_data->meshes), _data->meshes_count);
}

auto GLTFAsset::nodes() const -> std::span<const GLTFNode> {
    return std::span<const GLTFNode>(
        reinterpret_cast<const GLTFNode*>(_data->nodes), _data->nodes_count);
}

auto GLTFAsset::scenes() const -> std::span<const GLTFScene> {
    return std::span<const GLTFScene>(
        reinterpret_cast<const GLTFScene*>(_data->scenes), _data->scenes_count);
}

auto GLTFAsset::buffers() const -> std::span<const GLTFBuffer> {
    return std::span<const GLTFBuffer>(
        reinterpret_cast<const GLTFBuffer*>(_data->buffers),
        _data->buffers_count);
}

auto GLTFAsset::buffer_views() const -> std::span<const GLTFBufferView> {
    return std::span<const GLTFBufferView>(
        reinterpret_cast<const GLTFBufferView*>(_data->buffer_views),
        _data->buffer_views_count);
}

auto GLTFAsset::accessors() const -> std::span<const GLTFAccessor> {
    return std::span<const GLTFAccessor>(
        reinterpret_cast<const GLTFAccessor*>(_data->accessors),
        _data->accessors_count);
}

auto GLTFAsset::materials() const -> std::span<const GLTFMaterial> {
    return std::span<const GLTFMaterial>(
        reinterpret_cast<const GLTFMaterial*>(_data->materials),
        _data->materials_count);
}

auto GLTFAsset::textures() const -> std::span<const GLTFTexture> {
    return std::span<const GLTFTexture>(
        reinterpret_cast<const GLTFTexture*>(_data->textures),
        _data->textures_count);
}

auto GLTFAsset::images() const -> std::span<const GLTFImage> {
    return std::span<const GLTFImage>(
        reinterpret_cast<const GLTFImage*>(_data->images), _data->images_count);
}

auto GLTFAsset::animations() const -> std::span<const GLTFAnimation> {
    return std::span<const GLTFAnimation>(
        reinterpret_cast<const GLTFAnimation*>(_data->animations),
        _data->animations_count);
}

auto GLTFAsset::skins() const -> std::span<const GLTFSkin> {
    return std::span<const GLTFSkin>(
        reinterpret_cast<const GLTFSkin*>(_data->skins), _data->skins_count);
}

auto GLTFAsset::cameras() const -> std::span<const GLTFCamera> {
    return std::span<const GLTFCamera>(
        reinterpret_cast<const GLTFCamera*>(_data->cameras),
        _data->cameras_count);
}

//==============================================================================
// Accessors for mutable spans
//==============================================================================

auto GLTFAsset::meshes() -> std::span<GLTFMesh> {
    return std::span<GLTFMesh>(reinterpret_cast<GLTFMesh*>(_data->meshes),
                               _data->meshes_count);
}

auto GLTFAsset::nodes() -> std::span<GLTFNode> {
    return std::span<GLTFNode>(reinterpret_cast<GLTFNode*>(_data->nodes),
                               _data->nodes_count);
}

auto GLTFAsset::scenes() -> std::span<GLTFScene> {
    return std::span<GLTFScene>(reinterpret_cast<GLTFScene*>(_data->scenes),
                                _data->scenes_count);
}

auto GLTFAsset::buffers() -> std::span<GLTFBuffer> {
    return std::span<GLTFBuffer>(reinterpret_cast<GLTFBuffer*>(_data->buffers),
                                 _data->buffers_count);
}

auto GLTFAsset::buffer_views() -> std::span<GLTFBufferView> {
    return std::span<GLTFBufferView>(
        reinterpret_cast<GLTFBufferView*>(_data->buffer_views),
        _data->buffer_views_count);
}

auto GLTFAsset::accessors() -> std::span<GLTFAccessor> {
    return std::span<GLTFAccessor>(
        reinterpret_cast<GLTFAccessor*>(_data->accessors),
        _data->accessors_count);
}

auto GLTFAsset::materials() -> std::span<GLTFMaterial> {
    return std::span<GLTFMaterial>(
        reinterpret_cast<GLTFMaterial*>(_data->materials),
        _data->materials_count);
}

auto GLTFAsset::textures() -> std::span<GLTFTexture> {
    return std::span<GLTFTexture>(
        reinterpret_cast<GLTFTexture*>(_data->textures), _data->textures_count);
}

auto GLTFAsset::images() -> std::span<GLTFImage> {
    return std::span<GLTFImage>(reinterpret_cast<GLTFImage*>(_data->images),
                                _data->images_count);
}

auto GLTFAsset::animations() -> std::span<GLTFAnimation> {
    return std::span<GLTFAnimation>(
        reinterpret_cast<GLTFAnimation*>(_data->animations),
        _data->animations_count);
}

auto GLTFAsset::skins() -> std::span<GLTFSkin> {
    return std::span<GLTFSkin>(reinterpret_cast<GLTFSkin*>(_data->skins),
                               _data->skins_count);
}

auto GLTFAsset::cameras() -> std::span<GLTFCamera> {
    return std::span<GLTFCamera>(reinterpret_cast<GLTFCamera*>(_data->cameras),
                                 _data->cameras_count);
}

//==============================================================================
// Asset metadata
//==============================================================================

auto GLTFAsset::generator() const -> std::string_view {
    return _data && _data->asset.generator
               ? std::string_view(_data->asset.generator)
               : std::string_view();
}

auto GLTFAsset::version() const -> std::string_view {
    return _data && _data->asset.version
               ? std::string_view(_data->asset.version)
               : std::string_view();
}

auto GLTFAsset::copyright() const -> std::string_view {
    return _data && _data->asset.copyright
               ? std::string_view(_data->asset.copyright)
               : std::string_view();
}

void GLTFAsset::set_generator(std::string_view gen) {
    if (_data) {
        _data->asset.generator = _intern_string(gen);
    }
}

void GLTFAsset::set_copyright(std::string_view copyright) {
    if (_data) {
        _data->asset.copyright = _intern_string(copyright);
    }
}

//==============================================================================
// Scene management
//==============================================================================

auto GLTFAsset::default_scene() const -> std::optional<GLTFScene> {
    if (_data && _data->scene) {
        return *reinterpret_cast<const GLTFScene*>(_data->scene);
    }
    return std::nullopt;
}

void GLTFAsset::set_default_scene(size_t scene_index) {
    if (!_data || scene_index >= _data->scenes_count) {
        throw GLTFException("Scene index out of range");
    }
    _data->scene = &_data->scenes[scene_index];
}

} // namespace nv