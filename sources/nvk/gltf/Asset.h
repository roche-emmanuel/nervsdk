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

    [[nodiscard]] auto decode_data_uri(const String& uri,
                                       size_t expected_size) const -> U8Vector;
    [[nodiscard]] auto load_external_buffer(const String& uri,
                                            size_t expected_size) const
        -> U8Vector;
    [[nodiscard]] auto resolve_path(const String& uri) const -> String;

    // File I/O
    void load(const char* path, bool load_buffers = true);
    void save(const char* path) const;
    void validate() const;

    auto get_gltf_buffer(U32 idx) -> cgltf_buffer&;
    [[nodiscard]] auto get_gltf_buffer(U32 idx) const -> const cgltf_buffer&;

    auto get_buffer(U32 idx) -> GLTFBuffer&;
    [[nodiscard]] auto get_buffer(U32 idx) const -> const GLTFBuffer&;

    auto add_buffer(size_t size = 0) -> GLTFBuffer&;

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
    cgltf_data* _data{nullptr};

    Vector<cgltf_buffer> _rawBuffers;
    Vector<RefPtr<GLTFBuffer>> _buffers;

    // Storage for dynamically allocated elements
    struct OwnedElements {
        Vector<cgltf_buffer_view> buffer_views;
        Vector<cgltf_accessor> accessors;
        Vector<cgltf_mesh> meshes;
        Vector<cgltf_node> nodes;
        Vector<cgltf_scene> scenes;
        Vector<cgltf_material> materials;
        Vector<cgltf_texture> textures;
        Vector<cgltf_image> images;
        Vector<cgltf_sampler> samplers;
        Vector<cgltf_animation> animations;
        Vector<cgltf_skin> skins;
        Vector<cgltf_camera> cameras;
        Vector<cgltf_primitive> primitives;
        Vector<cgltf_attribute> attributes;
        Vector<cgltf_morph_target> morph_targets;
        Vector<cgltf_animation_sampler> animation_samplers;
        Vector<cgltf_animation_channel> animation_channels;

        // String storage (for names, URIs, etc.)
        Vector<std::string> strings;

        // Binary data storage for buffers
        Vector<Vector<uint8_t>> buffer_data;
    } _owned;

    // Track which elements came from file vs. were added
    bool _loaded_from_file;

    void _rebuild_pointers();
    void initialize_empty();
    auto intern_string(std::string_view str) -> char*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_