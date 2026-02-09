#ifndef _GLTF_SAMPLER_H_
#define _GLTF_SAMPLER_H_

#include <nvk/gltf/Element.h>

namespace nv {

enum GLTFMagFilter { GLTF_MAG_NEAREST = 9728, GLTF_MAG_LINEAR = 9729 };

enum GLTFMinFilter {
    GLTF_MIN_NEAREST = 9728,
    GLTF_MIN_LINEAR = 9729,
    GLTF_MIN_NEAREST_MIPMAP_NEAREST = 9984,
    GLTF_MIN_LINEAR_MIPMAP_NEAREST = 9985,
    GLTF_MIN_NEAREST_MIPMAP_LINEAR = 9986,
    GLTF_MIN_LINEAR_MIPMAP_LINEAR = 9987
};

enum GLTFWrapMode {
    GLTF_WRAP_CLAMP_TO_EDGE = 33071,
    GLTF_WRAP_MIRRORED_REPEAT = 33648,
    GLTF_WRAP_REPEAT = 10497
};

class GLTFSampler : public GLTFElement {
  public:
    explicit GLTFSampler(GLTFAsset& parent, U32 index);

    // Name accessors
    [[nodiscard]] auto name() const -> const String&;
    void set_name(String name);

    // Mag filter accessors
    [[nodiscard]] auto has_mag_filter() const -> bool;
    [[nodiscard]] auto mag_filter() const -> GLTFMagFilter;
    void set_mag_filter(GLTFMagFilter filter);
    void clear_mag_filter();

    // Min filter accessors
    [[nodiscard]] auto has_min_filter() const -> bool;
    [[nodiscard]] auto min_filter() const -> GLTFMinFilter;
    void set_min_filter(GLTFMinFilter filter);
    void clear_min_filter();

    // Wrap S accessors
    [[nodiscard]] auto wrap_s() const -> GLTFWrapMode;
    void set_wrap_s(GLTFWrapMode mode);

    // Wrap T accessors
    [[nodiscard]] auto wrap_t() const -> GLTFWrapMode;
    void set_wrap_t(GLTFWrapMode mode);

    // Serialization
    void read(const Json& desc);
    [[nodiscard]] auto write() const -> Json;

  protected:
    String _name;
    GLTFMagFilter _magFilter = GLTF_MAG_LINEAR;
    bool _hasMagFilter = false;
    GLTFMinFilter _minFilter = GLTF_MIN_LINEAR;
    bool _hasMinFilter = false;
    GLTFWrapMode _wrapS = GLTF_WRAP_REPEAT;
    GLTFWrapMode _wrapT = GLTF_WRAP_REPEAT;
};

} // namespace nv

#endif // _GLTF_SAMPLER_H_