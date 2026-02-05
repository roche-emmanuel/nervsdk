#include <nvk/gltf/Image.h>

namespace nv {
auto GLTFImage::name() const -> std::string_view {
    return _image->name ? std::string_view(_image->name) : std::string_view();
}
auto GLTFImage::uri() const -> std::string_view {
    return _image->uri ? std::string_view(_image->uri) : std::string_view();
}
auto GLTFImage::mime_type() const -> std::string_view {
    return _image->mime_type ? std::string_view(_image->mime_type)
                             : std::string_view();
}
auto GLTFImage::handle() -> cgltf_image* { return _image; }
auto GLTFImage::handle() const -> const cgltf_image* { return _image; }
} // namespace nv