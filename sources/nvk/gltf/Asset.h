// gltf_asset.h
#ifndef _GLTF_ASSET_H_
#define _GLTF_ASSET_H_

#include <nvk_common.h>

namespace nv {

namespace gltf {
auto to_string(GLTFElementType type) -> std::string_view;
auto to_element_type(std::string_view str) -> GLTFElementType;
// Helper to determine number of components based on element type
auto get_element_component_count(GLTFElementType type) -> size_t;

auto get_attribute_size(GLTFElementType type, GLTFComponentType ctype)
    -> size_t;

auto to_string(GLTFAttributeType type) -> std::string_view;
auto to_attribute_type(std::string_view str) -> GLTFAttributeType;

auto get_data_type(GLTFElementType type, GLTFComponentType ctype) -> DataType;
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
    auto write_json() const -> Json;
    void save(const char* path) const;
    auto save_to_memory() const -> String;
    void validate() const;

    static auto create() -> RefPtr<GLTFAsset>;

    auto add_buffer(size_t size = 0, String name = {}) -> GLTFBuffer&;
    auto get_buffer(U32 idx) -> GLTFBuffer&;
    [[nodiscard]] auto get_buffer(U32 idx) const -> const GLTFBuffer&;

    auto add_bufferview(String name = {}) -> GLTFBufferView&;
    auto add_bufferview(GLTFBuffer& buf, U32 offset = 0, U32 size = 0)
        -> GLTFBufferView&;
    auto get_bufferview(U32 idx) -> GLTFBufferView&;
    [[nodiscard]] auto get_bufferview(U32 idx) const -> const GLTFBufferView&;

    auto add_accessor(String name = {}) -> GLTFAccessor&;
    auto add_accessor(GLTFBufferView& view, GLTFElementType etype,
                      GLTFComponentType ctype, U32 count, U32 offset = 0)
        -> GLTFAccessor&;
    auto get_accessor(U32 idx) -> GLTFAccessor&;
    [[nodiscard]] auto get_accessor(U32 idx) const -> const GLTFAccessor&;

    auto add_mesh(String name = {}) -> GLTFMesh&;
    auto get_mesh(U32 idx) -> GLTFMesh&;
    [[nodiscard]] auto get_mesh(U32 idx) const -> const GLTFMesh&;

    auto add_node(String name = {}) -> GLTFNode&;
    auto get_node(U32 idx) -> GLTFNode&;
    [[nodiscard]] auto get_node(U32 idx) const -> const GLTFNode&;

    auto add_scene(String name = {}) -> GLTFScene&;
    auto get_scene(U32 idx) -> GLTFScene&;
    [[nodiscard]] auto get_scene(U32 idx) const -> const GLTFScene&;

    auto add_material(String name = {}) -> GLTFMaterial&;
    auto get_material(U32 idx) -> GLTFMaterial&;
    [[nodiscard]] auto get_material(U32 idx) const -> const GLTFMaterial&;

    auto add_sampler(String name = {}) -> GLTFSampler&;
    auto get_sampler(U32 idx) -> GLTFSampler&;
    [[nodiscard]] auto get_sampler(U32 idx) const -> const GLTFSampler&;

    auto add_texture(String name = {}) -> GLTFTexture&;
    auto get_texture(U32 idx) -> GLTFTexture&;
    [[nodiscard]] auto get_texture(U32 idx) const -> const GLTFTexture&;

    auto add_image(String name = {}) -> GLTFImage&;
    auto get_image(U32 idx) -> GLTFImage&;
    [[nodiscard]] auto get_image(U32 idx) const -> const GLTFImage&;

    // Scene management
    [[nodiscard]] auto default_scene() const -> RefPtr<GLTFScene>;
    void set_default_scene(GLTFScene* scene);

    // Asset metadata
    [[nodiscard]] auto generator() const -> const String&;
    [[nodiscard]] auto version() const -> const String&;
    [[nodiscard]] auto copyright() const -> const String&;
    void set_generator(String gen);
    void set_copyright(String copyright);

    // Utility
    [[nodiscard]] auto empty() const -> bool { return _numElements == 0; }
    void clear();

    void update_all_position_bounds() const;

  private:
    String _generator{"NervSDK GLTF Asset"};
    String _version{"2.0"};
    String _copyright;

    I32 _numElements{0};

    Vector<RefPtr<GLTFBuffer>> _buffers;
    Vector<RefPtr<GLTFBufferView>> _bufferViews;
    Vector<RefPtr<GLTFAccessor>> _accessors;
    Vector<RefPtr<GLTFMesh>> _meshes;
    Vector<RefPtr<GLTFNode>> _nodes;
    Vector<RefPtr<GLTFScene>> _scenes;
    Vector<RefPtr<GLTFMaterial>> _materials;
    Vector<RefPtr<GLTFTexture>> _textures;
    Vector<RefPtr<GLTFSampler>> _samplers;
    Vector<RefPtr<GLTFImage>> _images;

    RefPtr<GLTFScene> _defaultScene;
};

} // namespace nv

#endif // _GLTF_ASSET_H_