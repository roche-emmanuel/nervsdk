#include <nvk/gltf/Camera.h>

namespace nv {
auto GLTFCamera::name() const -> std::string_view {
    return _camera->name ? std::string_view(_camera->name) : std::string_view();
}
auto GLTFCamera::type() const -> cgltf_camera_type { return _camera->type; }
auto GLTFCamera::perspective_yfov() const -> float {
    return _camera->type == cgltf_camera_type_perspective
               ? _camera->data.perspective.yfov
               : 0.0f;
}
auto GLTFCamera::perspective_aspect_ratio() const -> float {
    return _camera->type == cgltf_camera_type_perspective
               ? _camera->data.perspective.aspect_ratio
               : 0.0f;
}
auto GLTFCamera::perspective_znear() const -> float {
    return _camera->type == cgltf_camera_type_perspective
               ? _camera->data.perspective.znear
               : 0.0f;
}
auto GLTFCamera::perspective_zfar() const -> float {
    return _camera->type == cgltf_camera_type_perspective
               ? _camera->data.perspective.zfar
               : 0.0f;
}
void GLTFCamera::set_perspective(float yfov, float aspect_ratio, float znear,
                                 float zfar) {
    _camera->type = cgltf_camera_type_perspective;
    _camera->data.perspective.yfov = yfov;
    _camera->data.perspective.aspect_ratio = aspect_ratio;
    _camera->data.perspective.znear = znear;
    _camera->data.perspective.zfar = zfar;
    _camera->data.perspective.has_aspect_ratio = aspect_ratio > 0;
    _camera->data.perspective.has_zfar = zfar > 0;
}
auto GLTFCamera::orthographic_xmag() const -> float {
    return _camera->type == cgltf_camera_type_orthographic
               ? _camera->data.orthographic.xmag
               : 0.0f;
}
auto GLTFCamera::orthographic_ymag() const -> float {
    return _camera->type == cgltf_camera_type_orthographic
               ? _camera->data.orthographic.ymag
               : 0.0f;
}
auto GLTFCamera::orthographic_znear() const -> float {
    return _camera->type == cgltf_camera_type_orthographic
               ? _camera->data.orthographic.znear
               : 0.0f;
}
auto GLTFCamera::orthographic_zfar() const -> float {
    return _camera->type == cgltf_camera_type_orthographic
               ? _camera->data.orthographic.zfar
               : 0.0f;
}
void GLTFCamera::set_orthographic(float xmag, float ymag, float znear,
                                  float zfar) {
    _camera->type = cgltf_camera_type_orthographic;
    _camera->data.orthographic.xmag = xmag;
    _camera->data.orthographic.ymag = ymag;
    _camera->data.orthographic.znear = znear;
    _camera->data.orthographic.zfar = zfar;
}
auto GLTFCamera::handle() -> cgltf_camera* { return _camera; }
auto GLTFCamera::handle() const -> const cgltf_camera* { return _camera; }
} // namespace nv