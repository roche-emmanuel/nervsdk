// gltf_asset.h
#ifndef _GLTF_ASSET_H_
#define _GLTF_ASSET_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

// Forward declarations
class GLTFMesh;
class GLTFNode;
class GLTFScene;
class GLTFBuffer;
class GLTFBufferView;
class GLTFAccessor;
class GLTFMaterial;
class GLTFTexture;
class GLTFImage;
class GLTFAnimation;
class GLTFSkin;
class GLTFCamera;
class GLTFPrimitive;

// Exception types
class GLTFException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class GLTFLoadException : public GLTFException {
    using GLTFException::GLTFException;
};

class GLTFValidationException : public GLTFException {
    using GLTFException::GLTFException;
};

// Main asset class
class GLTFAsset {
  public:
    // Construction
    GLTFAsset();
    explicit GLTFAsset(const char* path, bool load_buffers = true);
    explicit GLTFAsset(const std::string& path, bool load_buffers = true)
        : GLTFAsset(path.c_str(), load_buffers) {}

    // Move-only semantics
    ~GLTFAsset();
    GLTFAsset(const GLTFAsset&) = delete;
    auto operator=(const GLTFAsset&) -> GLTFAsset& = delete;
    GLTFAsset(GLTFAsset&& other) noexcept;
    auto operator=(GLTFAsset&& other) noexcept -> GLTFAsset&;

    // File I/O
    void load(const char* path, bool load_buffers = true);
    void save(const char* path) const;
    void validate() const;

    // Read access to existing elements
    [[nodiscard]] auto meshes() const -> std::span<const GLTFMesh>;
    [[nodiscard]] auto nodes() const -> std::span<const GLTFNode>;
    [[nodiscard]] auto scenes() const -> std::span<const GLTFScene>;
    [[nodiscard]] auto buffers() const -> std::span<const GLTFBuffer>;
    [[nodiscard]] auto buffer_views() const -> std::span<const GLTFBufferView>;
    [[nodiscard]] auto accessors() const -> std::span<const GLTFAccessor>;
    [[nodiscard]] auto materials() const -> std::span<const GLTFMaterial>;
    [[nodiscard]] auto textures() const -> std::span<const GLTFTexture>;
    [[nodiscard]] auto images() const -> std::span<const GLTFImage>;
    [[nodiscard]] auto animations() const -> std::span<const GLTFAnimation>;
    [[nodiscard]] auto skins() const -> std::span<const GLTFSkin>;
    [[nodiscard]] auto cameras() const -> std::span<const GLTFCamera>;

    // Mutable access
    auto meshes() -> std::span<GLTFMesh>;
    auto nodes() -> std::span<GLTFNode>;
    auto scenes() -> std::span<GLTFScene>;
    auto buffers() -> std::span<GLTFBuffer>;
    auto buffer_views() -> std::span<GLTFBufferView>;
    auto accessors() -> std::span<GLTFAccessor>;
    auto materials() -> std::span<GLTFMaterial>;
    auto textures() -> std::span<GLTFTexture>;
    auto images() -> std::span<GLTFImage>;
    auto animations() -> std::span<GLTFAnimation>;
    auto skins() -> std::span<GLTFSkin>;
    auto cameras() -> std::span<GLTFCamera>;

    // Add new elements
    auto add_mesh(std::string_view name = "") -> GLTFMesh&;
    auto add_node(std::string_view name = "") -> GLTFNode&;
    auto add_scene(std::string_view name = "") -> GLTFScene&;
    auto add_buffer(size_t size = 0) -> GLTFBuffer&;
    auto add_buffer_view(size_t buffer_index, size_t offset, size_t length)
        -> GLTFBufferView&;
    auto add_accessor(cgltf_type type, cgltf_component_type component_type,
                      size_t count) -> GLTFAccessor&;
    auto add_material(std::string_view name = "") -> GLTFMaterial&;
    auto add_texture() -> GLTFTexture&;
    auto add_image(std::string_view uri = "") -> GLTFImage&;
    auto add_animation(std::string_view name = "") -> GLTFAnimation&;
    auto add_skin(std::string_view name = "") -> GLTFSkin&;
    auto add_camera(std::string_view name = "") -> GLTFCamera&;

    // Scene management
    [[nodiscard]] auto default_scene() const -> std::optional<GLTFScene>;
    void set_default_scene(size_t scene_index);

    // Asset metadata
    [[nodiscard]] auto generator() const -> std::string_view;
    [[nodiscard]] auto version() const -> std::string_view;
    [[nodiscard]] auto copyright() const -> std::string_view;
    void set_generator(std::string_view gen);
    void set_copyright(std::string_view copyright);

    // Direct access (use with caution)
    auto data() -> cgltf_data* { return _data; }
    [[nodiscard]] auto data() const -> const cgltf_data* { return _data; }

    // Utility
    [[nodiscard]] auto empty() const -> bool { return _data == nullptr; }
    void clear();

  private:
    cgltf_data* _data;

    // Storage for dynamically allocated elements
    struct OwnedElements {
        std::vector<cgltf_buffer> buffers;
        std::vector<cgltf_buffer_view> buffer_views;
        std::vector<cgltf_accessor> accessors;
        std::vector<cgltf_mesh> meshes;
        std::vector<cgltf_node> nodes;
        std::vector<cgltf_scene> scenes;
        std::vector<cgltf_material> materials;
        std::vector<cgltf_texture> textures;
        std::vector<cgltf_image> images;
        std::vector<cgltf_sampler> samplers;
        std::vector<cgltf_animation> animations;
        std::vector<cgltf_skin> skins;
        std::vector<cgltf_camera> cameras;
        std::vector<cgltf_primitive> primitives;
        std::vector<cgltf_attribute> attributes;
        std::vector<cgltf_morph_target> morph_targets;
        std::vector<cgltf_animation_sampler> animation_samplers;
        std::vector<cgltf_animation_channel> animation_channels;

        // String storage (for names, URIs, etc.)
        std::vector<std::string> strings;

        // Binary data storage for buffers
        std::vector<std::vector<uint8_t>> buffer_data;
    } _owned;

    // Track which elements came from file vs. were added
    bool _loaded_from_file;

    void _rebuild_pointers();
    void _initialize_empty();
    auto _intern_string(std::string_view str) -> char*;
    void _reserve_capacity(size_t estimate);
};

// Wrapper classes for type-safe access

class GLTFMesh {
    friend class GLTFAsset;
    cgltf_mesh* _mesh;

    explicit GLTFMesh(cgltf_mesh* m) : _mesh(m) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _mesh->name ? std::string_view(_mesh->name) : std::string_view();
    }

    void set_name(std::string_view name);

    [[nodiscard]] auto primitives() const -> std::span<const GLTFPrimitive>;
    auto primitives() -> std::span<GLTFPrimitive>;

    auto add_primitive() -> GLTFPrimitive&;

    [[nodiscard]] auto primitive_count() const -> size_t {
        return _mesh->primitives_count;
    }

    // Weights for morph targets
    [[nodiscard]] auto weights() const -> std::span<const float> {
        return {_mesh->weights, _mesh->weights_count};
    }

    auto handle() -> cgltf_mesh* { return _mesh; }
    [[nodiscard]] auto handle() const -> const cgltf_mesh* { return _mesh; }
};

class GLTFPrimitive {
    friend class GLTFMesh;
    cgltf_primitive* _prim;

    explicit GLTFPrimitive(cgltf_primitive* p) : _prim(p) {}

  public:
    [[nodiscard]] auto type() const -> cgltf_primitive_type {
        return _prim->type;
    }
    void set_type(cgltf_primitive_type type) { _prim->type = type; }

    // Indices
    [[nodiscard]] auto indices() const -> std::optional<GLTFAccessor>;
    void set_indices(size_t accessor_index);

    // Attributes
    struct Attribute {
        std::string_view name;
        size_t accessor_index;
        cgltf_attribute_type type;
    };

    [[nodiscard]] auto attributes() const -> std::vector<Attribute>;
    void add_attribute(std::string_view name, size_t accessor_index,
                       cgltf_attribute_type type);
    void add_attribute(cgltf_attribute_type type, size_t accessor_index);

    // Material
    [[nodiscard]] auto material_index() const -> std::optional<size_t>;
    void set_material(size_t material_index);

    auto handle() -> cgltf_primitive* { return _prim; }
    [[nodiscard]] auto handle() const -> const cgltf_primitive* {
        return _prim;
    }
};

class GLTFNode {
    friend class GLTFAsset;
    cgltf_node* _node;

    explicit GLTFNode(cgltf_node* n) : _node(n) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _node->name ? std::string_view(_node->name) : std::string_view();
    }
    void set_name(std::string_view name);

    // Transform
    [[nodiscard]] auto has_matrix() const -> bool { return _node->has_matrix; }
    [[nodiscard]] auto has_trs() const -> bool {
        return _node->has_translation || _node->has_rotation ||
               _node->has_scale;
    }

    [[nodiscard]] auto matrix() const -> std::span<const float, 16> {
        return std::span<const float, 16>(_node->matrix, 16);
    }

    void set_matrix(const float matrix[16]) {
        std::copy(matrix, matrix + 16, _node->matrix);
        _node->has_matrix = 1;
        _node->has_translation = _node->has_rotation = _node->has_scale = 0;
    }

    [[nodiscard]] auto translation() const -> std::span<const float, 3> {
        return std::span<const float, 3>(_node->translation, 3);
    }

    [[nodiscard]] auto rotation() const -> std::span<const float, 4> {
        return std::span<const float, 4>(_node->rotation, 4);
    }

    [[nodiscard]] auto scale() const -> std::span<const float, 3> {
        return std::span<const float, 3>(_node->scale, 3);
    }

    void set_translation(float x, float y, float z) {
        _node->translation[0] = x;
        _node->translation[1] = y;
        _node->translation[2] = z;
        _node->has_translation = 1;
        _node->has_matrix = 0;
    }

    void set_rotation(float x, float y, float z, float w) {
        _node->rotation[0] = x;
        _node->rotation[1] = y;
        _node->rotation[2] = z;
        _node->rotation[3] = w;
        _node->has_rotation = 1;
        _node->has_matrix = 0;
    }

    void set_scale(float x, float y, float z) {
        _node->scale[0] = x;
        _node->scale[1] = y;
        _node->scale[2] = z;
        _node->has_scale = 1;
        _node->has_matrix = 0;
    }

    // Hierarchy
    [[nodiscard]] auto child_indices() const -> std::vector<size_t>;
    void add_child(size_t node_index);
    [[nodiscard]] auto parent_index() const -> std::optional<size_t>;

    // Attachments
    [[nodiscard]] auto mesh_index() const -> std::optional<size_t>;
    void set_mesh(size_t mesh_index);

    [[nodiscard]] auto skin_index() const -> std::optional<size_t>;
    void set_skin(size_t skin_index);

    [[nodiscard]] auto camera_index() const -> std::optional<size_t>;
    void set_camera(size_t camera_index);

    auto handle() -> cgltf_node* { return _node; }
    [[nodiscard]] auto handle() const -> const cgltf_node* { return _node; }
};

class GLTFScene {
    friend class GLTFAsset;
    cgltf_scene* _scene;

    explicit GLTFScene(cgltf_scene* s) : _scene(s) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _scene->name ? std::string_view(_scene->name)
                            : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto node_indices() const -> std::vector<size_t>;
    void add_node(size_t node_index);

    auto handle() -> cgltf_scene* { return _scene; }
    [[nodiscard]] auto handle() const -> const cgltf_scene* { return _scene; }
};

class GLTFBuffer {
    friend class GLTFAsset;
    cgltf_buffer* _buffer;
    std::vector<uint8_t>* _data_storage;

    explicit GLTFBuffer(cgltf_buffer* b,
                        std::vector<uint8_t>* storage = nullptr)
        : _buffer(b), _data_storage(storage) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _buffer->name ? std::string_view(_buffer->name)
                             : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto uri() const -> std::string_view {
        return _buffer->uri ? std::string_view(_buffer->uri)
                            : std::string_view();
    }
    void set_uri(std::string_view uri);

    [[nodiscard]] auto size() const -> size_t { return _buffer->size; }

    [[nodiscard]] auto data() const -> std::span<const uint8_t> {
        if (_buffer->data) {
            return {static_cast<const uint8_t*>(_buffer->data), _buffer->size};
        }
        return {};
    }

    auto data() -> std::span<uint8_t> {
        if (_data_storage) {
            return *_data_storage;
        }
        if (_buffer->data) {
            return {static_cast<uint8_t*>(_buffer->data), _buffer->size};
        }
        return {};
    }

    void set_data(std::span<const uint8_t> data);
    void resize(size_t new_size);

    auto handle() -> cgltf_buffer* { return _buffer; }
    [[nodiscard]] auto handle() const -> const cgltf_buffer* { return _buffer; }
};

class GLTFBufferView {
    friend class GLTFAsset;
    cgltf_buffer_view* _view;

    explicit GLTFBufferView(cgltf_buffer_view* v) : _view(v) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _view->name ? std::string_view(_view->name) : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto buffer_index() const -> size_t;
    [[nodiscard]] auto offset() const -> size_t { return _view->offset; }
    [[nodiscard]] auto size() const -> size_t { return _view->size; }
    [[nodiscard]] auto stride() const -> size_t { return _view->stride; }

    [[nodiscard]] auto type() const -> cgltf_buffer_view_type {
        return _view->type;
    }
    void set_type(cgltf_buffer_view_type type) { _view->type = type; }

    void set_buffer(size_t buffer_index);
    void set_offset(size_t offset) { _view->offset = offset; }
    void set_size(size_t size) { _view->size = size; }
    void set_stride(size_t stride) { _view->stride = stride; }

    auto handle() -> cgltf_buffer_view* { return _view; }
    [[nodiscard]] auto handle() const -> const cgltf_buffer_view* {
        return _view;
    }
};

class GLTFAccessor {
    friend class GLTFAsset;
    cgltf_accessor* _accessor;

    explicit GLTFAccessor(cgltf_accessor* a) : _accessor(a) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _accessor->name ? std::string_view(_accessor->name)
                               : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto type() const -> cgltf_type { return _accessor->type; }
    [[nodiscard]] auto component_type() const -> cgltf_component_type {
        return _accessor->component_type;
    }
    [[nodiscard]] auto count() const -> size_t { return _accessor->count; }
    [[nodiscard]] auto offset() const -> size_t { return _accessor->offset; }
    [[nodiscard]] auto normalized() const -> bool {
        return _accessor->normalized;
    }

    [[nodiscard]] auto buffer_view_index() const -> std::optional<size_t>;
    void set_buffer_view(size_t buffer_view_index);
    void set_offset(size_t offset) { _accessor->offset = offset; }
    void set_normalized(bool normalized) { _accessor->normalized = normalized; }

    // Type-safe data access
    template <typename T>
    [[nodiscard]] auto data_as() const -> std::span<const T> {
        if (!_accessor->buffer_view)
            return {};

        const auto* base =
            static_cast<const uint8_t*>(_accessor->buffer_view->buffer->data);
        if (!base)
            return {};

        base += _accessor->buffer_view->offset + _accessor->offset;

        size_t stride = _accessor->stride ? _accessor->stride : sizeof(T);

        // Note: This assumes packed data when stride == sizeof(T)
        // For strided data, need a different approach
        if (stride == sizeof(T)) {
            return {reinterpret_cast<const T*>(base), _accessor->count};
        }

        return {}; // Strided access requires special handling
    }

    // Min/max bounds
    [[nodiscard]] auto has_min() const -> bool { return _accessor->has_min; }
    [[nodiscard]] auto has_max() const -> bool { return _accessor->has_max; }

    [[nodiscard]] auto min() const -> std::span<const float> {
        return _accessor->has_min ? std::span<const float>(_accessor->min, 16)
                                  : std::span<const float>();
    }

    [[nodiscard]] auto max() const -> std::span<const float> {
        return _accessor->has_max ? std::span<const float>(_accessor->max, 16)
                                  : std::span<const float>();
    }

    void set_min(std::span<const float> values);
    void set_max(std::span<const float> values);

    auto handle() -> cgltf_accessor* { return _accessor; }
    [[nodiscard]] auto handle() const -> const cgltf_accessor* {
        return _accessor;
    }
};

class GLTFMaterial {
    friend class GLTFAsset;
    cgltf_material* _material;

    explicit GLTFMaterial(cgltf_material* m) : _material(m) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _material->name ? std::string_view(_material->name)
                               : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto has_metallic_roughness() const -> bool {
        return _material->has_pbr_metallic_roughness;
    }
    [[nodiscard]] auto has_specular_glossiness() const -> bool {
        return _material->has_pbr_specular_glossiness;
    }

    // PBR Metallic Roughness
    [[nodiscard]] auto base_color_factor() const -> std::span<const float, 4> {
        return std::span<const float, 4>(
            _material->pbr_metallic_roughness.base_color_factor, 4);
    }

    void set_base_color_factor(float r, float g, float b, float a) {
        _material->pbr_metallic_roughness.base_color_factor[0] = r;
        _material->pbr_metallic_roughness.base_color_factor[1] = g;
        _material->pbr_metallic_roughness.base_color_factor[2] = b;
        _material->pbr_metallic_roughness.base_color_factor[3] = a;
        _material->has_pbr_metallic_roughness = 1;
    }

    [[nodiscard]] auto metallic_factor() const -> float {
        return _material->pbr_metallic_roughness.metallic_factor;
    }

    [[nodiscard]] auto roughness_factor() const -> float {
        return _material->pbr_metallic_roughness.roughness_factor;
    }

    void set_metallic_factor(float metallic) {
        _material->pbr_metallic_roughness.metallic_factor = metallic;
        _material->has_pbr_metallic_roughness = 1;
    }

    void set_roughness_factor(float roughness) {
        _material->pbr_metallic_roughness.roughness_factor = roughness;
        _material->has_pbr_metallic_roughness = 1;
    }

    // Alpha mode
    [[nodiscard]] auto alpha_mode() const -> cgltf_alpha_mode {
        return _material->alpha_mode;
    }
    [[nodiscard]] auto alpha_cutoff() const -> float {
        return _material->alpha_cutoff;
    }

    void set_alpha_mode(cgltf_alpha_mode mode) { _material->alpha_mode = mode; }
    void set_alpha_cutoff(float cutoff) { _material->alpha_cutoff = cutoff; }

    // Double sided
    [[nodiscard]] auto double_sided() const -> bool {
        return _material->double_sided;
    }
    void set_double_sided(bool double_sided) {
        _material->double_sided = double_sided;
    }

    auto handle() -> cgltf_material* { return _material; }
    [[nodiscard]] auto handle() const -> const cgltf_material* {
        return _material;
    }
};

class GLTFTexture {
    friend class GLTFAsset;
    cgltf_texture* _texture;

    explicit GLTFTexture(cgltf_texture* t) : _texture(t) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _texture->name ? std::string_view(_texture->name)
                              : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto image_index() const -> std::optional<size_t>;
    void set_image(size_t image_index);

    [[nodiscard]] auto sampler_index() const -> std::optional<size_t>;
    void set_sampler(size_t sampler_index);

    auto handle() -> cgltf_texture* { return _texture; }
    [[nodiscard]] auto handle() const -> const cgltf_texture* {
        return _texture;
    }
};

class GLTFImage {
    friend class GLTFAsset;
    cgltf_image* _image;

    explicit GLTFImage(cgltf_image* i) : _image(i) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _image->name ? std::string_view(_image->name)
                            : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto uri() const -> std::string_view {
        return _image->uri ? std::string_view(_image->uri) : std::string_view();
    }
    void set_uri(std::string_view uri);

    [[nodiscard]] auto mime_type() const -> std::string_view {
        return _image->mime_type ? std::string_view(_image->mime_type)
                                 : std::string_view();
    }
    void set_mime_type(std::string_view mime_type);

    [[nodiscard]] auto buffer_view_index() const -> std::optional<size_t>;
    void set_buffer_view(size_t buffer_view_index);

    auto handle() -> cgltf_image* { return _image; }
    [[nodiscard]] auto handle() const -> const cgltf_image* { return _image; }
};

class GLTFAnimation {
    friend class GLTFAsset;
    cgltf_animation* _animation;

    explicit GLTFAnimation(cgltf_animation* a) : _animation(a) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _animation->name ? std::string_view(_animation->name)
                                : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto sampler_count() const -> size_t {
        return _animation->samplers_count;
    }
    [[nodiscard]] auto channel_count() const -> size_t {
        return _animation->channels_count;
    }

    auto handle() -> cgltf_animation* { return _animation; }
    [[nodiscard]] auto handle() const -> const cgltf_animation* {
        return _animation;
    }
};

class GLTFSkin {
    friend class GLTFAsset;
    cgltf_skin* _skin;

    explicit GLTFSkin(cgltf_skin* s) : _skin(s) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _skin->name ? std::string_view(_skin->name) : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto joint_indices() const -> std::vector<size_t>;
    void add_joint(size_t node_index);

    [[nodiscard]] auto skeleton_index() const -> std::optional<size_t>;
    void set_skeleton(size_t node_index);

    [[nodiscard]] auto inverse_bind_matrices_index() const
        -> std::optional<size_t>;
    void set_inverse_bind_matrices(size_t accessor_index);

    auto handle() -> cgltf_skin* { return _skin; }
    [[nodiscard]] auto handle() const -> const cgltf_skin* { return _skin; }
};

class GLTFCamera {
    friend class GLTFAsset;
    cgltf_camera* _camera;

    explicit GLTFCamera(cgltf_camera* c) : _camera(c) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view {
        return _camera->name ? std::string_view(_camera->name)
                             : std::string_view();
    }
    void set_name(std::string_view name);

    [[nodiscard]] auto type() const -> cgltf_camera_type {
        return _camera->type;
    }

    // Perspective camera
    [[nodiscard]] auto perspective_yfov() const -> float {
        return _camera->type == cgltf_camera_type_perspective
                   ? _camera->data.perspective.yfov
                   : 0.0f;
    }

    [[nodiscard]] auto perspective_aspect_ratio() const -> float {
        return _camera->type == cgltf_camera_type_perspective
                   ? _camera->data.perspective.aspect_ratio
                   : 0.0f;
    }

    [[nodiscard]] auto perspective_znear() const -> float {
        return _camera->type == cgltf_camera_type_perspective
                   ? _camera->data.perspective.znear
                   : 0.0f;
    }

    [[nodiscard]] auto perspective_zfar() const -> float {
        return _camera->type == cgltf_camera_type_perspective
                   ? _camera->data.perspective.zfar
                   : 0.0f;
    }

    void set_perspective(float yfov, float aspect_ratio, float znear,
                         float zfar) {
        _camera->type = cgltf_camera_type_perspective;
        _camera->data.perspective.yfov = yfov;
        _camera->data.perspective.aspect_ratio = aspect_ratio;
        _camera->data.perspective.znear = znear;
        _camera->data.perspective.zfar = zfar;
        _camera->data.perspective.has_aspect_ratio = aspect_ratio > 0;
        _camera->data.perspective.has_zfar = zfar > 0;
    }

    // Orthographic camera
    [[nodiscard]] auto orthographic_xmag() const -> float {
        return _camera->type == cgltf_camera_type_orthographic
                   ? _camera->data.orthographic.xmag
                   : 0.0f;
    }

    [[nodiscard]] auto orthographic_ymag() const -> float {
        return _camera->type == cgltf_camera_type_orthographic
                   ? _camera->data.orthographic.ymag
                   : 0.0f;
    }

    [[nodiscard]] auto orthographic_znear() const -> float {
        return _camera->type == cgltf_camera_type_orthographic
                   ? _camera->data.orthographic.znear
                   : 0.0f;
    }

    [[nodiscard]] auto orthographic_zfar() const -> float {
        return _camera->type == cgltf_camera_type_orthographic
                   ? _camera->data.orthographic.zfar
                   : 0.0f;
    }

    void set_orthographic(float xmag, float ymag, float znear, float zfar) {
        _camera->type = cgltf_camera_type_orthographic;
        _camera->data.orthographic.xmag = xmag;
        _camera->data.orthographic.ymag = ymag;
        _camera->data.orthographic.znear = znear;
        _camera->data.orthographic.zfar = zfar;
    }

    auto handle() -> cgltf_camera* { return _camera; }
    [[nodiscard]] auto handle() const -> const cgltf_camera* { return _camera; }
};

} // namespace nv

#endif // _GLTF_ASSET_H_