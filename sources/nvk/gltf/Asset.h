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

namespace gltf {
auto to_string(GLTFElementType type) -> std::string_view;
auto to_element_type(std::string_view str) -> GLTFElementType;
// Helper to determine number of components based on element type
auto get_element_component_count(GLTFElementType type) -> size_t;
} // namespace gltf

// Main asset class
class GLTFAsset : public RefObject {
  public:
    // Construction
    GLTFAsset();
    explicit GLTFAsset(const char* path, bool load_buffers = true);
    explicit GLTFAsset(const std::string& path, bool load_buffers = true)
        : GLTFAsset(path.c_str(), load_buffers) {}

    // Move-only semantics
    ~GLTFAsset() override;
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

    static auto create() -> RefPtr<GLTFAsset>;

    auto add_buffer(size_t size = 0) -> GLTFBuffer&;
    auto get_buffer(U32 idx) -> GLTFBuffer&;
    [[nodiscard]] auto get_buffer(U32 idx) const -> const GLTFBuffer&;

    auto add_bufferview() -> GLTFBufferView&;
    auto add_bufferview(GLTFBuffer& buf, U32 offset = 0, U32 size = 0)
        -> GLTFBufferView&;
    auto get_bufferview(U32 idx) -> GLTFBufferView&;
    [[nodiscard]] auto get_bufferview(U32 idx) const -> const GLTFBufferView&;

    // Scene management
    [[nodiscard]] auto default_scene() const -> std::optional<GLTFScene>;
    void set_default_scene(size_t scene_index);

    // Asset metadata
    [[nodiscard]] auto generator() const -> const String&;
    [[nodiscard]] auto version() const -> const String&;
    [[nodiscard]] auto copyright() const -> const String&;
    void set_generator(String gen);
    void set_copyright(String copyright);

    // Utility
    [[nodiscard]] auto empty() const -> bool { return _numElements == 0; }
    void clear();

  private:
    String _generator{"NervSDK GLTF Asset"};
    String _version{"2.0"};
    String _copyright;

    I32 _numElements{0};

    Vector<RefPtr<GLTFBuffer>> _buffers;
    Vector<RefPtr<GLTFBufferView>> _bufferViews;

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

    void _rebuild_pointers();
    auto intern_string(std::string_view str) -> char*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_