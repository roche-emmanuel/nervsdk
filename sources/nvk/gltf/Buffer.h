#ifndef _GLTF_BUFFER_H_
#define _GLTF_BUFFER_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFBuffer {
    friend class GLTFAsset;
    cgltf_buffer* _buffer;
    std::vector<uint8_t>* _data_storage;

    explicit GLTFBuffer(cgltf_buffer* b,
                        std::vector<uint8_t>* storage = nullptr)
        : _buffer(b), _data_storage(storage) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto uri() const -> std::string_view;
    void set_uri(std::string_view uri);

    [[nodiscard]] auto size() const -> size_t;

    [[nodiscard]] auto data() const -> std::span<const uint8_t>;

    auto data() -> std::span<uint8_t>;

    void set_data(std::span<const uint8_t> data);
    void resize(size_t new_size);

    auto handle() -> cgltf_buffer*;
    [[nodiscard]] auto handle() const -> const cgltf_buffer*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_