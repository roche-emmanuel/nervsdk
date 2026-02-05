#ifndef _GLTF_BUFFER_H_
#define _GLTF_BUFFER_H_

#include <nvk/gltf/Element.h>
namespace nv {

class GLTFBuffer : public GLTFElement {
    Vector<U8> _data;
    String _name;
    String _uri;

  public:
    GLTFBuffer(GLTFAsset& parent, U32 index);
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    [[nodiscard]] auto uri() const -> const String&;
    void set_uri(String uri);

    [[nodiscard]] auto size() const -> size_t;

    [[nodiscard]] auto data() const -> const U8*;

    auto data() -> U8*;

    void set_data(Vector<U8> data);
    void resize(size_t new_size);

    void read(const Json& desc);
    void write(Json& desc) const;
};

} // namespace nv

#endif // _GLTF_ASSET_H_