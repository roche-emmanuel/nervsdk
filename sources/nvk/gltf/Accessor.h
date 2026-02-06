#ifndef _GLTF_ACCESSOR_H_
#define _GLTF_ACCESSOR_H_

#include <nvk/gltf/Element.h>

namespace nv {

class GLTFAccessor : public GLTFElement {
  public:
    explicit GLTFAccessor(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // Component type accessors
    [[nodiscard]] auto component_type() const -> GLTFComponentType;
    void set_component_type(GLTFComponentType type);

    // Normalized flag accessors
    [[nodiscard]] auto normalized() const -> bool;
    void set_normalized(bool normalized);

    // Element type accessors
    [[nodiscard]] auto element_type() const -> GLTFElementType;
    void set_element_type(GLTFElementType type);

    // Offset accessors
    [[nodiscard]] auto offset() const -> U32;
    void set_offset(U32 offset);

    // Count accessors
    [[nodiscard]] auto count() const -> U32;
    void set_count(U32 count);

    void set_buffer_view(GLTFBufferView& view);

    // Min/Max accessors
    [[nodiscard]] auto has_min() const -> bool;
    [[nodiscard]] auto min() const -> const F32Vector&;
    void set_min(const F32Vector& min);
    void set_min(const Vec3f& vec);
    void clear_min();

    [[nodiscard]] auto has_max() const -> bool;
    [[nodiscard]] auto max() const -> const F32Vector&;
    void set_max(const F32Vector& max);
    void set_max(const Vec3f& vec);
    void clear_max();

    // Serialization
    void read(const Json& desc);

    [[nodiscard]] auto write() const -> Json;

    auto get_data_type() const -> DataType;

    void update_bounds();
    auto data() -> U8*;
    auto data() const -> const U8*;

  protected:
    String _name;
    GLTFComponentType _componentType{GLTF_COMP_UNKNOWN};
    bool _normalized{false};
    GLTFElementType _elementType{GLTF_ELEM_UNKNOWN};
    U32 _offset{0};
    U32 _count{0};
    U32 _stride{0};
    RefPtr<GLTFBufferView> _bufferView;
    bool _hasMin{false};
    F32Vector _min;
    bool _hasMax{false};
    F32Vector _max;
};

} // namespace nv

#endif // _GLTF_ACCESSOR_H_