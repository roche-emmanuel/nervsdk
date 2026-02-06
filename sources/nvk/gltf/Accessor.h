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

    // Stride accessors
    [[nodiscard]] auto stride() const -> U32;
    void set_stride(U32 stride);

    void set_buffer_view(GLTFBufferView& view);

    // Min/Max accessors
    [[nodiscard]] auto has_min() const -> bool;
    [[nodiscard]] auto min() const -> const std::array<F32, 16>&;
    void set_min(const std::array<F32, 16>& min);
    void clear_min();

    [[nodiscard]] auto has_max() const -> bool;
    [[nodiscard]] auto max() const -> const std::array<F32, 16>&;
    void set_max(const std::array<F32, 16>& max);
    void clear_max();

    // Serialization
    void read(const Json& desc);

    [[nodiscard]] auto write() const -> Json;

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
    std::array<F32, 16> _min{};
    bool _hasMax{false};
    std::array<F32, 16> _max{};
};

} // namespace nv

#endif // _GLTF_ACCESSOR_H_