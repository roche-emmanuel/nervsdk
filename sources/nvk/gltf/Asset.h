// gltf_asset.h
#ifndef _GLTF_ASSET_H_
#define _GLTF_ASSET_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

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