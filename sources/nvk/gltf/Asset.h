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

} // namespace nv

#endif // _GLTF_ASSET_H_