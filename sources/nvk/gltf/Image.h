#ifndef _GLTF_IMAGE_H_
#define _GLTF_IMAGE_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFImage {
    friend class GLTFAsset;
    cgltf_image* _image;

    explicit GLTFImage(cgltf_image* i) : _image(i) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto uri() const -> std::string_view;
    void set_uri(std::string_view uri);

    [[nodiscard]] auto mime_type() const -> std::string_view;
    void set_mime_type(std::string_view mime_type);

    [[nodiscard]] auto buffer_view_index() const -> std::optional<size_t>;
    void set_buffer_view(size_t buffer_view_index);

    auto handle() -> cgltf_image*;
    [[nodiscard]] auto handle() const -> const cgltf_image*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_