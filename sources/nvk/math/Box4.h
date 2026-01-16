#ifndef _NV_BOX4_H_
#define _NV_BOX4_H_

#include <nvk/math/Vec4.h>
#include <nvk_math.h>

namespace nv {

template <typename T> struct Box4 {
    /** Data type of vector components.*/
    using value_t = T;

    /**
     * Minimum x coordinate.
     */
    value_t xmin;

    /**
     * Maximum x coordinate.
     */
    value_t xmax;

    /**
     * Minimum y coordinate.
     */
    value_t ymin;

    /**
     * Maximum y coordinate.
     */
    value_t ymax;

    /**
     * Minimum z coordinate.
     */
    value_t zmin;

    /**
     * Maximum z coordinate.
     */
    value_t zmax;

    /**
     * Minimum w coordinate.
     */
    value_t wmin;

    /**
     * Maximum w coordinate.
     */
    value_t wmax;

    /**
     * Creates a new, empty bounding box.
     */
    Box4()
        : xmin(INFINITY), xmax(-INFINITY), ymin(INFINITY), ymax(-INFINITY),
          zmin(INFINITY), zmax(-INFINITY), wmin(INFINITY), wmax(-INFINITY) {}

    /**
     * Creates a new bounding box with the given coordinates.
     */
    Box4(value_t xmin, value_t xmax, value_t ymin, value_t ymax, value_t zmin,
         value_t zmax, value_t wmin, value_t wmax)
        : xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax), zmin(zmin),
          zmax(zmax), wmin(wmin), wmax(wmax) {}

    /** Create Box4 from a single point */
    explicit Box4(const Vec4<value_t>& p)
        : xmin(p.x()), xmax(p.x()), ymin(p.y()), ymax(p.y()), zmin(p.z()),
          zmax(p.z()), wmin(p.w()), wmax(p.w()) {}

    /**
     * Creates a new bounding box enclosing the two given points.
     */
    Box4(const Vec4<value_t>& p, const Vec4<value_t>& q)
        : xmin(minimum(p.x, q.x)), xmax(maximum(p.x, q.x)),
          ymin(minimum(p.y, q.y)), ymax(maximum(p.y, q.y)),
          zmin(minimum(p.z, q.z)), zmax(maximum(p.z, q.z)),
          wmin(minimum(p.w, q.w)), wmax(maximum(p.w, q.w)) {}

    /**
     * Returns the center of this bounding box.
     */
    [[nodiscard]] auto center() const -> Vec4<value_t> {
        return Vec4<value_t>((xmin + xmax) / 2, (ymin + ymax) / 2,
                             (zmin + zmax) / 2, (wmin + wmax) / 2);
    }

    /** Extend a bounding box to a given point */
    void extendTo(const Vec4<value_t>& p) {
        xmin = std::min(xmin, p.x());
        xmax = std::max(xmax, p.x());
        ymin = std::min(ymin, p.y());
        ymax = std::max(ymax, p.y());
        zmin = std::min(zmin, p.z());
        zmax = std::max(zmax, p.z());
        wmin = std::min(wmin, p.w());
        wmax = std::max(wmax, p.w());
    }

    /**
     * Returns the bounding box containing this box and the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto enlarge(const Vec4<value_t>& p) const -> Box4<value_t> {
        return Box4<value_t>(minimum(xmin, p.x), maximum(xmax, p.x),
                             minimum(ymin, p.y), maximum(ymax, p.y),
                             minimum(zmin, p.z), maximum(zmax, p.z),
                             minimum(wmin, p.w), maximum(wmax, p.w));
    }

    /**
     * Returns the bounding box containing this box and the given box.
     *
     * @param r an arbitrary bounding box.
     */
    [[nodiscard]] auto enlarge(const Box4<value_t>& r) const -> Box4<value_t> {
        return Box4<value_t>(minimum(xmin, r.xmin), maximum(xmax, r.xmax),
                             minimum(ymin, r.ymin), maximum(ymax, r.ymax),
                             minimum(zmin, r.zmin), maximum(zmax, r.zmax),
                             minimum(wmin, r.wmin), maximum(wmax, r.wmax));
    }

    /**
     * Returns true if this bounding box contains the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto contains(const Vec4<value_t>& p) const -> bool {
        return p.x >= xmin && p.x <= xmax && p.y >= ymin && p.y <= ymax &&
               p.z >= zmin && p.z <= zmax && p.w >= wmin && p.w <= wmax;
    }

    /** Min vector */
    [[nodiscard]] auto minimum() const -> Vec4<T> {
        return {xmin, ymin, zmin, wmin};
    }

    /** Max vector */
    [[nodiscard]] auto maximum() const -> Vec4<T> {
        return {xmax, ymax, zmax, wmax};
    }

    [[nodiscard]] auto xyz() const -> Box3<T> {
        return {xmin, xmax, ymin, ymax, zmin, zmax};
    }

    [[nodiscard]] auto xy() const -> Box2<T> {
        return {xmin, xmax, ymin, ymax};
    }

    /**
     * Casts this bounding box to another base value_t.
     */
    template <class U> auto cast() -> Box4<U> {
        return Box4<U>(xmin, xmax, ymin, ymax, zmin, zmax, wmin, wmax);
    }

}; // end of class Box4

using Box4f = Box4<F32>;
using Box4d = Box4<F64>;

} // end of namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Box4f& box)
    -> std::ostream& {
    os << "nv::Box4f(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ", " << box.zmin << ", " << box.zmax << ", "
       << box.wmin << ", " << box.wmax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Box4d& box)
    -> std::ostream& {
    os << "nv::Box4d(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ", " << box.zmin << ", " << box.zmax << ", "
       << box.wmin << ", " << box.wmax << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Box4f> {
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Box4f box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(),
            "Box4f({:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g})",
            box.xmin, box.xmax, box.ymin, box.ymax, box.zmin, box.zmax,
            box.wmin, box.wmax);
    }
};

template <> struct fmt::formatter<nv::Box4d> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Box4d box, fmt::format_context& ctx)
        -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(),
            "Box4d({:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g})",
            box.xmin, box.xmax, box.ymin, box.ymax, box.zmin, box.zmax,
            box.wmin, box.wmax);
    }
};

#endif