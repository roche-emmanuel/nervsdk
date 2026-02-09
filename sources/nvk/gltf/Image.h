#ifndef _GLTF_IMAGE_H_
#define _GLTF_IMAGE_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFImage : public GLTFElement {
  public:
    explicit GLTFImage(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // URI accessors
    [[nodiscard]] auto has_uri() const -> bool;
    [[nodiscard]] auto uri() const -> const String&;
    void set_uri(String uri);
    void clear_uri();

    // MIME type accessors
    [[nodiscard]] auto has_mime_type() const -> bool;
    [[nodiscard]] auto mime_type() const -> const String&;
    void set_mime_type(String mimeType);
    void clear_mime_type();

    // Buffer view accessors (for embedded images)
    [[nodiscard]] auto has_buffer_view() const -> bool;
    [[nodiscard]] auto buffer_view() const -> const GLTFBufferView&;
    [[nodiscard]] auto buffer_view() -> GLTFBufferView&;
    void set_buffer_view(GLTFBufferView& view);
    void clear_buffer_view();

    // Serialization
    void read(const Json& desc);
    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    String _uri;
    bool _hasUri = false;
    String _mimeType;
    bool _hasMimeType = false;
    RefPtr<GLTFBufferView> _bufferView = nullptr;
};

} // namespace nv

#endif // _GLTF_IMAGE_H_