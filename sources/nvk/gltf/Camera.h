#ifndef _GLTF_CAMERA_H_
#define _GLTF_CAMERA_H_

#include <external/cgltf.h>
#include <nvk_common.h>

namespace nv {

class GLTFCamera {
    friend class GLTFAsset;
    cgltf_camera* _camera;

    explicit GLTFCamera(cgltf_camera* c) : _camera(c) {}

  public:
    [[nodiscard]] auto name() const -> std::string_view;
    void set_name(std::string_view name);

    [[nodiscard]] auto type() const -> cgltf_camera_type;

    // Perspective camera
    [[nodiscard]] auto perspective_yfov() const -> float;

    [[nodiscard]] auto perspective_aspect_ratio() const -> float;

    [[nodiscard]] auto perspective_znear() const -> float;

    [[nodiscard]] auto perspective_zfar() const -> float;

    void set_perspective(float yfov, float aspect_ratio, float znear,
                         float zfar);

    // Orthographic camera
    [[nodiscard]] auto orthographic_xmag() const -> float;

    [[nodiscard]] auto orthographic_ymag() const -> float;

    [[nodiscard]] auto orthographic_znear() const -> float;

    [[nodiscard]] auto orthographic_zfar() const -> float;

    void set_orthographic(float xmag, float ymag, float znear, float zfar);

    auto handle() -> cgltf_camera*;
    [[nodiscard]] auto handle() const -> const cgltf_camera*;
};

} // namespace nv

#endif // _GLTF_ASSET_H_