#ifndef _NV_BOX3_H_
#define _NV_BOX3_H_

#include <nvk/math/Vec3.h>
#include <nvk_math.h>

namespace nv {

template <typename T> struct Box3 {
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
     * Creates a new, empty bounding box.
     */
    Box3()
        : xmin(INFINITY), xmax(-INFINITY), ymin(INFINITY), ymax(-INFINITY),
          zmin(INFINITY), zmax(-INFINITY) {}

    /**
     * Creates a new bounding box with the given coordinates.
     */
    Box3(value_t xmin, value_t xmax, value_t ymin, value_t ymax, value_t zmin,
         value_t zmax)
        : xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax), zmin(zmin),
          zmax(zmax) {}

    /** Create Box3 from a single point */
    explicit Box3(const Vec3<value_t>& p)
        : xmin(p.x()), xmax(p.x()), ymin(p.y()), ymax(p.y()), zmin(p.z()),
          zmax(p.z()) {}

    /**
     * Creates a new bounding box enclosing the two given points.
     */
    Box3(const Vec3<value_t>& p, const Vec3<value_t>& q)
        : xmin(minimum(p.x, q.x)), xmax(maximum(p.x, q.x)),
          ymin(minimum(p.y, q.y)), ymax(maximum(p.y, q.y)),
          zmin(minimum(p.z, q.z)), zmax(maximum(p.z, q.z)) {}

    /**
     * Returns the center of this bounding box.
     */
    [[nodiscard]] auto center() const -> Vec3<value_t> {
        return Vec3<value_t>((xmin + xmax) / 2, (ymin + ymax) / 2,
                             (zmin + zmax) / 2);
    }

    /** Extend a bounding box to a given point */
    void extendTo(const Vec3<value_t>& p) {
        xmin = std::min(xmin, p.x());
        xmax = std::max(xmax, p.x());
        ymin = std::min(ymin, p.y());
        ymax = std::max(ymax, p.y());
        zmin = std::min(zmin, p.z());
        zmax = std::max(zmax, p.z());
    }

    /**
     * Returns the bounding box containing this box and the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto enlarge(const Vec3<value_t>& p) const -> Box3<value_t> {
        return Box3<value_t>(minimum(xmin, p.x), maximum(xmax, p.x),
                             minimum(ymin, p.y), maximum(ymax, p.y),
                             minimum(zmin, p.z), maximum(zmax, p.z));
    }

    /**
     * Returns the bounding box containing this box and the given box.
     *
     * @param r an arbitrary bounding box.
     */
    [[nodiscard]] auto enlarge(const Box3<value_t>& r) const -> Box3<value_t> {
        return Box3<value_t>(minimum(xmin, r.xmin), maximum(xmax, r.xmax),
                             minimum(ymin, r.ymin), maximum(ymax, r.ymax),
                             minimum(zmin, r.zmin), maximum(zmax, r.zmax));
    }

    /**
     * Returns true if this bounding box contains the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto contains(const Vec3<value_t>& p) const -> bool {
        return p.x >= xmin && p.x <= xmax && p.y >= ymin && p.y <= ymax &&
               p.z >= zmin && p.z <= zmax;
    }

    /** Min vector */
    [[nodiscard]] auto minimum() const -> Vec3<T> { return {xmin, ymin, zmin}; }

    /** Max vector */
    [[nodiscard]] auto maximum() const -> Vec3<T> { return {xmax, ymax, zmax}; }

    /**
     * Casts this bounding box to another base value_t.
     */
    template <class U> auto cast() -> Box3<U> {
        return Box3<U>(xmin, xmax, ymin, ymax, zmin, zmax);
    }

}; // end of class Box3

using Box3f = Box3<F32>;
using Box3d = Box3<F64>;

} // end of namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Box3f& box)
    -> std::ostream& {
    os << "nv::Box3f(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ", " << box.zmin << ", " << box.zmax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Box3d& box)
    -> std::ostream& {
    os << "nv::Box3d(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ", " << box.zmin << ", " << box.zmax << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Box3f> {
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
    auto format(const nv::Box3f box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(), "Box3f({:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g})",
            box.xmin, box.xmax, box.ymin, box.ymax, box.zmin, box.zmax);
    }
};

template <> struct fmt::formatter<nv::Box3d> {
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
    auto format(const nv::Box3d box, fmt::format_context& ctx)
        -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(), "Box3d({:6g}, {:6g}, {:6g}, {:6g}, {:6g}, {:6g})",
            box.xmin, box.xmax, box.ymin, box.ymax, box.zmin, box.zmax);
    }
};

#endif
