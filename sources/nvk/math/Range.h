#ifndef _NV_RANGE_H_
#define _NV_RANGE_H_

#include <nvk/math/Vec2.h>
#include <nvk/math/Vec3.h>
#include <nvk/math/Vec4.h>
#include <nvk_math.h>

namespace nv {

template <typename T> constexpr auto range_max() -> T {
    if constexpr (std::is_same_v<T, U64>) {
        return static_cast<T>(UINT64_MAX);
    } else if (std::is_same_v<T, U32>) {
        return static_cast<T>(UINT32_MAX);
    } else if (std::is_same_v<T, U16>) {
        return static_cast<T>(UINT16_MAX);
    } else if (std::is_same_v<T, I64>) {
        return static_cast<T>(INT64_MAX);
    } else if (std::is_same_v<T, I32>) {
        return static_cast<T>(INT32_MAX);
    } else if (std::is_same_v<T, I16>) {
        return static_cast<T>(INT16_MAX);
    } else {
        return static_cast<T>(INFINITY); // fallback
    }
}

template <typename T> constexpr auto range_min() -> T {
    if constexpr (std::is_same_v<T, U64>) {
        return static_cast<T>(0);
    } else if (std::is_same_v<T, U32>) {
        return static_cast<T>(0);
    } else if (std::is_same_v<T, U16>) {
        return static_cast<T>(0);
    } else if (std::is_same_v<T, I64>) {
        return static_cast<T>(INT64_MIN);
    } else if (std::is_same_v<T, I32>) {
        return static_cast<T>(INT32_MIN);
    } else if (std::is_same_v<T, I16>) {
        return static_cast<T>(INT16_MIN);
    } else {
        return static_cast<T>(-INFINITY); // fallback
    }
}

template <typename T> struct Range {
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
     * Creates a new, empty bounding box.
     */
    Range() : xmin(range_max<value_t>()), xmax(range_min<value_t>()) {}

    /**
     * Creates a new bounding box with the given coordinates.
     */
    Range(value_t xmin, value_t xmax) : xmin(xmin), xmax(xmax) {}

    inline auto operator==(const Range& b) const -> bool {
        return xmin == b.xmin && xmax == b.xmax;
    }

    inline auto operator!=(const Range& b) const -> bool {
        return xmin != b.xmin || xmax != b.xmax;
    }

    void set(value_t x0) {
        xmin = x0;
        xmax = x0;
    }
    void set(value_t x0, value_t x1) {
        set(x0);
        extendTo(x1);
    }
    void set(value_t x0, value_t x1, value_t x2) {
        set(x0);
        extendTo(x1);
        extendTo(x2);
    }
    void set(value_t x0, value_t x1, value_t x2, value_t x3) {
        set(x0);
        extendTo(x1);
        extendTo(x2);
        extendTo(x3);
    }

    template <typename U> void set(const Vec2<U>& p) { set(p.x(), p.y()); }

    template <typename U> void set(const Vec3<U>& p) {
        set(p.x(), p.y(), p.z());
    }

    template <typename U> void set(const Vec4<U>& p) {
        set(p.x(), p.y(), p.z(), p.w());
    }

    /**
     * Returns the center of this bounding box.
     */
    [[nodiscard]] auto center() const -> value_t {
        return (value_t)((xmin + xmax) * 0.5);
    }

    [[nodiscard]] auto min() const -> value_t { return xmin; }
    [[nodiscard]] auto max() const -> value_t { return xmax; }

    /** Return width of this box */
    [[nodiscard]] auto width() const -> value_t { return (xmax - xmin); }

    /** Check validity of this box */
    [[nodiscard]] auto valid() const -> bool { return xmax >= xmin; }

    /** Extend range to a given point */
    void extendTo(value_t p) {
        xmin = std::min(xmin, p);
        xmax = std::max(xmax, p);
    }

    void extendTo(const Vec2<value_t>& p) {
        extendTo(p.x());
        extendTo(p.y());
    }
    void extendTo(const Vec3<value_t>& p) {
        extendTo(p.x());
        extendTo(p.y());
        extendTo(p.z());
    }
    void extendTo(const Vec4<value_t>& p) {
        extendTo(p.x());
        extendTo(p.y());
        extendTo(p.z());
        extendTo(p.w());
    }

    /** Extend to other bounding box */
    void extendTo(const Range<value_t>& p) {
        xmin = std::min(xmin, p.xmin);
        xmax = std::max(xmax, p.xmax);
    }

    void resize(value_t newWidth) {
        newWidth = std::max(newWidth, (value_t)0.0);
        auto c = (xmax + xmin) * 0.5;
        xmin = c - newWidth * 0.5;
        xmax = c + newWidth * 0.5;
    }

    /**
     * Returns true if this bounding box contains the given point.
     *
     * @param p an arbitrary point.
     */
    [[nodiscard]] auto contains(value_t p) const -> bool {
        return p >= xmin && p <= xmax;
    }

    /** Reset this bounding box */
    void reset() {
        xmin = range_max<value_t>();
        xmax = range_min<value_t>();
    }

    /**
     * Expands the box by the specified amounts on each side.
     *
     * @param left Amount to expand on the left side
     * @param right Amount to expand on the right side
     */
    auto expand(value_t left, value_t right) -> Range& {
        xmin -= left;
        xmax += right;

        // Ensure the box remains valid
        if (xmin > xmax) {
            // If the box becomes inverted, set it to zero width
            value_t center = (xmin + xmax) / 2;
            xmin = xmax = center;
        }
        return *this;
    }

    auto expand(value_t size) -> Range& { return expand(size, size); }

    auto expand(const Vec2f& ltrb) -> Range& {
        return expand(ltrb.x(), ltrb.y());
    }

    /**
     * Shrinks the box by the specified amounts on each side.
     *
     * @param left Amount to shrink from the left side
     * @param right Amount to shrink from the right side
     */
    auto shrink(value_t left, value_t right) -> Range& {
        return expand(-left, -right);
    }

    auto shrink(value_t size) -> Range& { return expand(-size); }

    auto shrink(const Vec2f& ltrb) -> Range& {
        return shrink(ltrb.x(), ltrb.y());
    }

    [[nodiscard]] auto shrinked(const Vec2f& ltrb) const -> Range {
        return shrinked(ltrb.x(), ltrb.y());
    }

    [[nodiscard]] auto shrinked(value_t size) const -> Range {
        return shrinked(size, size);
    }

    [[nodiscard]] auto shrinked(value_t left, value_t right) const -> Range {
        Range box = *this;
        box.shrink(left, right);
        return box;
    }

    void translate(value_t pos) {
        xmin += pos;
        xmax += pos;
    }

    /**
     * Casts this bounding box to another base value_t.
     */
    template <class U> auto cast() const -> Range<U> {
        return Range<U>(xmin, xmax);
    }

    /** Convert this Rangef into a Vec4f */
    [[nodiscard]] auto asVec2() const -> Vec2<T> { return {xmin, xmax}; }

    /** Get the signed distance of a point to this box: */
    auto get_point_distance(value_t p) -> F32 {
        if (p < xmin) {
            return xmin - p;
        }
        if (p > xmax) {
            return p - xmax;
        }
        // Point is inside - return negative distance to nearest boundary
        return -std::min(p - xmin, xmax - p);
    }

}; // end of class Range

using Rangef = Range<F32>;
using Ranged = Range<F64>;
using Rangei = Range<I32>;
using Rangeu = Range<U32>;

template <typename T>
inline void to_json(nlohmann::json& j, const Range<T>& range) {
    j = nlohmann::json{
        {"xmin", range.xmin},
        {"xmax", range.xmax},
    };
}

template <typename T>
inline void from_json(const nlohmann::json& j, Range<T>& range) {
    j.at("xmin").get_to(range.xmin);
    j.at("xmax").get_to(range.xmax);
}

} // end of namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Rangef& box)
    -> std::ostream& {
    os << "nv::Rangef(" << box.xmin << ", " << box.xmax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Ranged& box)
    -> std::ostream& {
    os << "nv::Ranged(" << box.xmin << ", " << box.xmax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Rangei& box)
    -> std::ostream& {
    os << "nv::Rangei(" << box.xmin << ", " << box.xmax << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Rangeu& box)
    -> std::ostream& {
    os << "nv::Rangeu(" << box.xmin << ", " << box.xmax << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Rangef> {
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
    auto format(const nv::Rangef box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Rangef({:6g}, {:6g})", box.xmin,
                              box.xmax);
    }
};

template <> struct fmt::formatter<nv::Ranged> {
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
    auto format(const nv::Ranged box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Ranged({:6g}, {:6g})", box.xmin,
                              box.xmax);
    }
};

template <> struct fmt::formatter<nv::Rangei> {
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
    auto format(const nv::Rangei box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Rangei({}, {})", box.xmin, box.xmax);
    }
};

template <> struct fmt::formatter<nv::Rangeu> {
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
    auto format(const nv::Rangeu box, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Rangeu({}, {})", box.xmin, box.xmax);
    }
};

#endif
