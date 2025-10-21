#ifndef SDF_UTILS_H_
#define SDF_UTILS_H_

#include <nvk/math/Vec2.h>

namespace nv {

// cf. https://iquilezles.org/articles/distfunctions2d/

// Rounded Box - exact   (https://www.shadertoy.com/view/4llXD7 and
// https://www.youtube.com/watch?v=s5NGeUV2EyU)
// p: 2d position of the point.
// b: half width/height of the box.
// r: border radius: x=top-right, y=bottom-right, z=top-left, w=bottom-left
template <typename T>
auto sd_rounded_box(const Vec2<T>& p, const Vec2<T>& b, const Vec4<T>& r) -> T {
    auto rxy = (p.x() > 0.0) ? r.xy() : r.zw();
    auto radius = (p.y() > 0.0) ? rxy.x() : rxy.y();
    auto q = p.abs() - b + Vec2<T>(radius);
    return std::min(std::max(q.x(), q.y()), (T)0.0) +
           q.max({0.0, 0.0}).length() - radius;
}

template <typename T> auto sd_box(const Vec2<T>& p, const Vec2<T>& b) -> T {
    auto d = p.abs() - b;
    return d.max({0.0, 0.0}).length() +
           std::min(std::max(d.x(), d.y()), (T)0.0);
}

template <typename T> auto sd_circle(const Vec2<T>& p, T r) -> T {
    return p.length() - r;
}

} // namespace nv

#endif
