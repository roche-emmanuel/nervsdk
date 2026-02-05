#ifndef _GLTF_ACCESSOR_H_
#define _GLTF_ACCESSOR_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFAccessor {
    friend class GLTFAsset;
    cgltf_accessor* _accessor;

    explicit GLTFAccessor(cgltf_accessor* a) : _accessor(a) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto type() const -> cgltf_type;
    [[nodiscard]] auto component_type() const -> cgltf_component_type;
    [[nodiscard]] auto count() const -> size_t;
    [[nodiscard]] auto offset() const -> size_t;
    [[nodiscard]] auto normalized() const -> bool;

    [[nodiscard]] auto buffer_view_index() const -> std::optional<size_t>;
    void set_buffer_view(size_t buffer_view_index);
    void set_offset(size_t offset);
    void set_normalized(bool normalized);

    // Type-safe data access
    template <typename T>
    [[nodiscard]] auto data_as() const -> std::span<const T> {
        if (!_accessor->buffer_view)
            return {};

        const auto* base =
            static_cast<const uint8_t*>(_accessor->buffer_view->buffer->data);
        if (!base)
            return {};

        base += _accessor->buffer_view->offset + _accessor->offset;

        size_t stride = _accessor->stride ? _accessor->stride : sizeof(T);

        // Note: This assumes packed data when stride == sizeof(T)
        // For strided data, need a different approach
        if (stride == sizeof(T)) {
            return {reinterpret_cast<const T*>(base), _accessor->count};
        }

        return {}; // Strided access requires special handling
    }

    // Min/max bounds
    [[nodiscard]] auto has_min() const -> bool;
    [[nodiscard]] auto has_max() const -> bool;

    [[nodiscard]] auto min() const -> std::span<const float>;

    [[nodiscard]] auto max() const -> std::span<const float>;

    void set_min(std::span<const float> values);
    void set_max(std::span<const float> values);

    auto handle() -> cgltf_accessor*;
    [[nodiscard]] auto handle() const -> const cgltf_accessor*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_