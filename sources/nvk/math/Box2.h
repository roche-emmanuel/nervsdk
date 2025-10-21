#ifndef _NV_BOX2_H_
#define _NV_BOX2_H_

#include <nvk_math.h>

#include <nvk/math/Vec2.h>
#include <nvk/math/Vec4.h>
#include <nvk/math/sdf.h>

namespace nv {

template <typename T> struct Box2 {
    /** Data type of vector components.*/
    using value_t = T;

    enum EdgeType {
        EDGE_LEFT,
        EDGE_TOP,
        EDGE_RIGHT,
        EDGE_BOTTOM,
    };

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
     * Creates a new, empty bounding box.
     */
    Box2() : xmin(INFINITY), xmax(-INFINITY), ymin(INFINITY), ymax(-INFINITY) {}

    /**
     * Creates a new bounding box with the given coordinates.
     */
    Box2(value_t xmin, value_t xmax, value_t ymin, value_t ymax)
        : xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax) {}

    /** Create Box2 from a single point */
    explicit Box2(const Vec2<value_t>& p)
        : xmin(p.x()), xmax(p.x()), ymin(p.y()), ymax(p.y()) {}

    /**
     * Creates a new bounding box enclosing the two given points.
     */
    Box2(const Vec2<value_t>& p, const Vec2<value_t>& q)
        : xmin(std::min(p.x(), q.x())), xmax(std::max(p.x(), q.x())),
          ymin(std::min(p.y(), q.y())), ymax(std::max(p.y(), q.y())) {}

    inline auto operator==(const Box2& b) const -> bool {
        return xmin == b.xmin && xmax == b.xmax && ymin == b.ymin &&
               ymax == b.ymax;
    }

    inline auto operator!=(const Box2& b) const -> bool {
        return xmin != b.xmin || xmax != b.xmax || ymin != b.ymin ||
               ymax != b.ymax;
    }

    /** set this box2 */
    void set(value_t x0, value_t x1, value_t y0, value_t y1) {
        xmin = std::min(x0, x1);
        xmax = std::max(x0, x1);

        ymin = std::min(y0, y1);
        ymax = std::max(y0, y1);
    }

    /**
     * Returns the center of this bounding box.
     */
    [[nodiscard]] auto center() const -> Vec2<value_t> {
        return Vec2<value_t>((xmin + xmax) * 0.5, (ymin + ymax) * 0.5);
    }

    /** Return width of this box */
    [[nodiscard]] auto width() const -> value_t { return (xmax - xmin); }

    /** Return height of this box */
    [[nodiscard]] auto height() const -> value_t { return (ymax - ymin); }

    /** Return the width/height of this box */
    [[nodiscard]] auto size() const -> Vec2<value_t> {
        return {xmax - xmin, ymax - ymin};
    }

    /** Check validity of this box */
    [[nodiscard]] auto valid() const -> bool {
        return xmax >= xmin && ymax >= ymin;
    }

    /** Get an edge start/end point. */
    [[nodiscard]] auto edge(I32 i) const -> Vec4<value_t> {
        switch (i) {
        case EDGE_LEFT: // left edge
            return {xmin, ymax, xmin, ymin};
        case EDGE_TOP: // top edge
            return {xmax, ymax, xmin, ymax};
        case EDGE_RIGHT: // right edge
            return {xmax, ymin, xmax, ymax};
        case EDGE_BOTTOM: // bottom edge
            return {xmin, ymin, xmax, ymin};
        default:
            THROW_MSG("Unsupported box edge index {}", i);
        }
    }

    [[nodiscard]] auto edgeNormal(I32 i) const -> Vec2<value_t> {
        switch (i) {
        case EDGE_LEFT: // left edge
            return {-1, 0};
        case EDGE_TOP: // top edge
            return {0, 1};
        case EDGE_RIGHT: // right edge
            return {1, 0};
        case EDGE_BOTTOM: // bottom edge
            return {0, -1};
        default:
            THROW_MSG("Invalid box edge index {}", i);
        }
    }

    /** Extend a bounding box to a given point */
    void extendTo(const Vec2<value_t>& p) {
        xmin = std::min(xmin, p.x());
        xmax = std::max(xmax, p.x());
        ymin = std::min(ymin, p.y());
        ymax = std::max(ymax, p.y());
    }

    // Extend on a segment:
    void extendTo(const Vec2<value_t>& p0, const Vec2<value_t>& p1) {
        xmin = std::min(xmin, std::min(p0.x(), p1.x()));
        xmax = std::max(xmax, std::max(p0.x(), p1.x()));
        ymin = std::min(ymin, std::min(p0.y(), p1.y()));
        ymax = std::max(ymax, std::max(p0.y(), p1.y()));
    }

    /** Extend to other bounding box */
    void extendTo(const Box2<value_t>& p) {
        xmin = std::min(xmin, p.xmin);
        xmax = std::max(xmax, p.xmax);
        ymin = std::min(ymin, p.ymin);
        ymax = std::max(ymax, p.ymax);
    }

    void resize_width(value_t newWidth) {
        newWidth = std::max(newWidth, (value_t)0.0);
        auto c = (xmax + xmin) * 0.5;
        xmin = c - newWidth * 0.5;
        xmax = c + newWidth * 0.5;
    }

    void resize_height(value_t newHeight) {
        newHeight = std::max(newHeight, (value_t)0.0);
        auto c = (ymax + ymin) * 0.5;
        ymin = c - newHeight * 0.5;
        ymax = c + newHeight * 0.5;
    }

    void resize(value_t newWidth, value_t newHeight) {
        resize_width(newWidth);
        resize_height(newHeight);
    }

    /**
     * Returns the bounding box containing this box and the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto enlarge(const Vec2<value_t>& p) const -> Box2<value_t> {
        return Box2<value_t>(minimum(xmin, p.x), maximum(xmax, p.x),
                             minimum(ymin, p.y), maximum(ymax, p.y));
    }

    /**
     * Returns the bounding box containing this box and the given box.
     *
     * @param r an arbitrary bounding box.
     */
    [[nodiscard]] auto enlarge(const Box2<value_t>& r) const -> Box2<value_t> {
        return Box2<value_t>(minimum(xmin, r.xmin), maximum(xmax, r.xmax),
                             minimum(ymin, r.ymin), maximum(ymax, r.ymax));
    }

    /**
     * Returns true if this bounding box contains the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto contains(const Vec2<value_t>& p) const -> bool {
        return p.x() >= xmin && p.x() <= xmax && p.y() >= ymin && p.y() <= ymax;
    }

    /** Min vector */
    [[nodiscard]] auto minimum() const -> Vec2<T> { return {xmin, ymin}; }

    /** Max vector */
    [[nodiscard]] auto maximum() const -> Vec2<T> { return {xmax, ymax}; }

    /** Reset this bounding box */
    void reset() {
        xmin = INFINITY;
        xmax = -INFINITY;
        ymin = INFINITY;
        ymax = -INFINITY;
    }

    /**
     * Expands the box by the specified amounts on each side.
     *
     * @param left Amount to expand on the left side
     * @param top Amount to expand on the top side
     * @param right Amount to expand on the right side
     * @param bottom Amount to expand on the bottom side
     */
    auto expand(value_t left, value_t top, value_t right, value_t bottom)
        -> Box2& {
        xmin -= left;
        ymax += top;
        xmax += right;
        ymin -= bottom;

        // Ensure the box remains valid
        if (xmin > xmax) {
            // If the box becomes inverted, set it to zero width
            value_t center = (xmin + xmax) / 2;
            xmin = xmax = center;
        }
        if (ymin > ymax) {
            // If the box becomes inverted, set it to zero height
            value_t center = (ymin + ymax) / 2;
            ymin = ymax = center;
        }
        return *this;
    }

    auto expand(value_t size) -> Box2& {
        return expand(size, size, size, size);
    }

    auto expand(const Vec4f& ltrb) -> Box2& {
        return expand(ltrb.x(), ltrb.y(), ltrb.z(), ltrb.w());
    }

    /**
     * Shrinks the box by the specified amounts on each side.
     *
     * @param left Amount to shrink from the left side
     * @param top Amount to shrink from the top side
     * @param right Amount to shrink from the right side
     * @param bottom Amount to shrink from the bottom side
     */
    auto shrink(value_t left, value_t top, value_t right, value_t bottom)
        -> Box2& {
        return expand(-left, -top, -right, -bottom);
    }

    auto shrink(value_t size) -> Box2& { return expand(-size); }

    auto shrink(const Vec4f& ltrb) -> Box2& {
        return shrink(ltrb.x(), ltrb.y(), ltrb.z(), ltrb.w());
    }

    [[nodiscard]] auto shrinked(const Vec4f& ltrb) const -> Box2 {
        return shrinked(ltrb.x(), ltrb.y(), ltrb.z(), ltrb.w());
    }

    [[nodiscard]] auto shrinked(value_t size) const -> Box2 {
        return shrinked(size, size, size, size);
    }

    [[nodiscard]] auto shrinked(value_t left, value_t top, value_t right,
                                value_t bottom) const -> Box2 {
        Box2 box = *this;
        box.shrink(left, top, right, bottom);
        return box;
    }

    void translate(const Vec2<value_t>& pos) {
        xmin += pos.x();
        xmax += pos.x();
        ymin += pos.y();
        ymax += pos.y();
    }

    /** Get the area of this box. */
    auto area() -> F32 { return width() * height(); }

    [[nodiscard]] auto top_left() const -> Vec2<value_t> {
        return {xmin, ymax};
    }
    [[nodiscard]] auto bottom_left() const -> Vec2<value_t> {
        return {xmin, ymin};
    }
    [[nodiscard]] auto center_left() const -> Vec2<value_t> {
        return {xmin, (ymax + ymin) * value_t(0.5)};
    }
    [[nodiscard]] auto top_right() const -> Vec2<value_t> {
        return {xmax, ymax};
    }
    [[nodiscard]] auto bottom_right() const -> Vec2<value_t> {
        return {xmax, ymin};
    }
    [[nodiscard]] auto center_right() const -> Vec2<value_t> {
        return {xmax, (ymax + ymin) * value_t(0.5)};
    }

    /**
     * Casts this bounding box to another base value_t.
     */
    template <class U> auto cast() const -> Box2<U> {
        return Box2<U>(xmin, xmax, ymin, ymax);
    }

    /** Convert this box2f into a Vec4f */
    [[nodiscard]] auto asVec4() const -> Vec4<T> {
        return {xmin, xmax, ymin, ymax};
    }

    /** Get the signed distance of a point to this box: */
    auto get_point_distance(const Vec2<value_t>& pos) -> F32 {
        return sd_box(pos - center(), size() * 0.5F);
    }

    auto get_alignment_anchor(I32 align) -> Vec2<value_t> {
        value_t xpos = 0;
        value_t ypos = 0;

        // Horizontal position:
        if ((align & ALIGN_LEFT) != 0) {
            xpos = xmin;
        } else if ((align & ALIGN_RIGHT) != 0) {
            xpos = xmax;
        } else {
            xpos = (xmax + xmin) * 0.5;
        }

        // Vertical position:
        if ((align & ALIGN_BOTTOM) != 0) {
            ypos = ymin;
        } else if ((align & ALIGN_TOP) != 0) {
            ypos = ymax;
        } else if ((align & ALIGN_BASELINE) != 0) {
            ypos = 0;
        } else {
            ypos = (ymax + ymin) * 0.5;
        }

        return {xpos, ypos};
    }

    void align_to(const Vec2<value_t>& anchor, I32 align) {
        auto tgtPoint = get_alignment_anchor(align);
        // We should ensure that the anchor point location becomes the
        // location of our "tgtPoint"
        // se we should translate our box by vec = anchor - tgtPoint:
        translate(anchor - tgtPoint);
    }

    auto aligned_to(const Vec2<value_t>& anchor, I32 align) -> Box2 {
        Box2 res{*this};
        res.align_to(anchor, align);
        return res;
    }

}; // end of class Box2

using Box2f = Box2<F32>;
using Box2d = Box2<F64>;
using Box2i = Box2<I32>;

} // end of namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Box2f& box)
    -> std::ostream& {
    os << "nv::Box2f(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Box2d& box)
    -> std::ostream& {
    os << "nv::Box2d(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Box2i& box)
    -> std::ostream& {
    os << "nv::Box2i(" << box.xmin << ", " << box.xmax << ", " << box.ymin
       << ", " << box.ymax << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Box2f> {
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
    auto format(const nv::Box2f box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Box2f({:6g}, {:6g}, {:6g}, {:6g})",
                              box.xmin, box.xmax, box.ymin, box.ymax);
    }
};

template <> struct fmt::formatter<nv::Box2d> {
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
    auto format(const nv::Box2d box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Box2d({:6g}, {:6g}, {:6g}, {:6g})",
                              box.xmin, box.xmax, box.ymin, box.ymax);
    }
};

template <> struct fmt::formatter<nv::Box2i> {
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
    auto format(const nv::Box2i box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Box2i({}, {}, {}, {})", box.xmin,
                              box.xmax, box.ymin, box.ymax);
    }
};

#endif
