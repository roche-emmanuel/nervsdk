#ifndef _NV_VEC2_H_
#define _NV_VEC2_H_

#include <core_exports.h>
#include <nvk_math.h>

namespace nv {

template <typename T> struct Vec2 {
    /** Data type of vector components.*/
    using value_t = T;

    /** Number of vector components. */
    enum { num_components = 2 };

    // #ifndef __NERVBIND__
    /** Vec member variable. */
    std::array<value_t, 2> _v{0};
    // #endif

    /** Constructor that sets all components of the vector to zero */
    Vec2() = default;

    constexpr explicit Vec2(value_t x) {
        _v[0] = x;
        _v[1] = x;
    }

    Vec2(value_t x, value_t y) {
        _v[0] = x;
        _v[1] = y;
    }

    ~Vec2() = default;

    template <typename U> explicit Vec2(const Vec2<U>& rhs) {
        _v[0] = (value_t)rhs._v[0];
        _v[1] = (value_t)rhs._v[1];
    }

    inline auto operator==(const Vec2& v) const -> bool {
        return _v[0] == v._v[0] && _v[1] == v._v[1];
    }

    inline auto operator!=(const Vec2& v) const -> bool {
        return _v[0] != v._v[0] || _v[1] != v._v[1];
    }

    inline auto operator<(const Vec2& v) const -> bool {
        if (_v[0] < v._v[0]) {
            return true;
        }
        if (_v[0] > v._v[0]) {
            return false;
        }
        return (_v[1] < v._v[1]);
    }

    inline auto ptr() -> value_t* { return _v.data(); }
    [[nodiscard]] inline auto ptr() const -> const value_t* {
        return _v.data();
    }

    inline void set(value_t x, value_t y) {
        _v[0] = x;
        _v[1] = y;
    }

    inline auto operator[](int i) -> value_t& { return _v[i]; }
    inline auto operator[](int i) const -> value_t { return _v[i]; }

    inline auto x() -> value_t& { return _v[0]; }
    inline auto y() -> value_t& { return _v[1]; }

    [[nodiscard]] inline auto x() const -> value_t { return _v[0]; }
    [[nodiscard]] inline auto y() const -> value_t { return _v[1]; }

    /** Returns true if all components have values that are not NaN. */
    [[nodiscard]] inline auto valid() const -> bool { return !isNaN(); }
    /** Returns true if at least one component has value NaN. */
    [[nodiscard]] inline auto isNaN() const -> bool {
        return nv::isNaN(_v[0]) || nv::isNaN(_v[1]);
    }

    /** component multiplication. */
    inline auto operator*(const Vec2& rhs) const -> Vec2 {
        return {_v[0] * rhs._v[0], _v[1] * rhs._v[1]};
    }

    /** Multiply by scalar. */
    inline auto operator*(value_t rhs) const -> Vec2 {
        return {_v[0] * rhs, _v[1] * rhs};
    }

    /** Unary multiply by scalar. */
    inline auto operator*=(value_t rhs) -> Vec2& {
        _v[0] *= rhs;
        _v[1] *= rhs;
        return *this;
    }

    [[nodiscard]] inline auto dot(const Vec2& rhs) const -> value_t {
        return _v[0] * rhs._v[0] + _v[1] * rhs._v[1];
    }

    // Note: the 2D cross product is actually a determinant.
    [[nodiscard]] inline auto cross(const Vec2& rhs) const -> value_t {
        return _v[0] * rhs._v[1] - _v[1] * rhs._v[0];
    }

    // Compute the ccw 90deg rotated vector:
    [[nodiscard]] inline auto ccw90() const -> Vec2 { return {-_v[1], _v[0]}; }

    // Compute the cw 90deg rotated vector:
    [[nodiscard]] inline auto cw90() const -> Vec2 { return {_v[1], -_v[0]}; }

    [[nodiscard]] inline auto rotated(value_t angle) const -> Vec2 {
        auto c = std::cos(angle);
        auto s = std::sin(angle);
        return {_v[0] * c - _v[1] * s, _v[0] * s + _v[1] * c};
    }

    /** Divide by scalar. */
    inline auto operator/(value_t rhs) const -> Vec2 {
        return {_v[0] / rhs, _v[1] / rhs};
    }

    /** Unary divide by scalar. */
    inline auto operator/=(value_t rhs) -> Vec2& {
        _v[0] /= rhs;
        _v[1] /= rhs;
        return *this;
    }

    /** Divide by Vec2 */
    inline auto operator/(Vec2 rhs) const -> Vec2 {
        return {_v[0] / rhs._v[0], _v[1] / rhs._v[1]};
    }

    /** Unary divide by Vec2. */
    inline auto operator/=(Vec2 rhs) -> Vec2& {
        _v[0] /= rhs._v[0];
        _v[1] /= rhs._v[1];
        return *this;
    }

    /** Binary vector add. */
    inline auto operator+(const Vec2& rhs) const -> Vec2 {
        return {_v[0] + rhs._v[0], _v[1] + rhs._v[1]};
    }

    inline auto operator+(value_t val) const -> Vec2 {
        return {_v[0] + val, _v[1] + val};
    }

    /** Unary vector add. Slightly more efficient because no temporary
     * intermediate object.
     */
    inline auto operator+=(const Vec2& rhs) -> Vec2& {
        _v[0] += rhs._v[0];
        _v[1] += rhs._v[1];
        return *this;
    }

    inline auto operator+=(value_t val) -> Vec2& {
        _v[0] += val;
        _v[1] += val;
        return *this;
    }

    /** Binary vector subtract. */
    inline auto operator-(const Vec2& rhs) const -> Vec2 {
        return {_v[0] - rhs._v[0], _v[1] - rhs._v[1]};
    }

    /** Unary vector subtract. */
    inline auto operator-=(const Vec2& rhs) -> Vec2& {
        _v[0] -= rhs._v[0];
        _v[1] -= rhs._v[1];
        return *this;
    }

    /** Negation operator. Returns the negative of the Vec2. */
    inline auto operator-() const -> Vec2 { return {-_v[0], -_v[1]}; }

    /** Length of the vector = sqrt( vec . vec ) */
    [[nodiscard]] inline auto length() const -> value_t {
        return sqrt(_v[0] * _v[0] + _v[1] * _v[1]);
    }

    /** Length squared of the vector = vec . vec */
    [[nodiscard]] inline auto length2() const -> value_t {
        return _v[0] * _v[0] + _v[1] * _v[1];
    }

    /** Normalize the vector so that it has length unity.
     * Returns the previous length of the vector.
     */
    inline auto normalize() -> value_t {
        value_t norm = length();
        if (norm > 0.0) {
            value_t inv = 1.0 / norm;
            _v[0] *= inv;
            _v[1] *= inv;
        }
        return (norm);
    }

    [[nodiscard]] inline auto normalized() const -> Vec2 {
        Vec2 res{_v[0], _v[1]};
        res.normalize();
        return res;
    }

    /** Get the abs of this vector: */
    [[nodiscard]] inline auto abs() const -> Vec2 {
        return {fabs(_v[0]), fabs(_v[1])};
    }

    /** Get the maximum of the components */
    [[nodiscard]] inline auto maximum() const -> value_t {
        return nv::maximum(_v[0], _v[1]);
    }

    /** Get the minimum of the components */
    [[nodiscard]] inline auto minimum() const -> value_t {
        return nv::minimum(_v[0], _v[1]);
    }

    /** Get the range of data */
    [[nodiscard]] inline auto range() const -> Range<value_t> {
        return {minimum(), maximum()};
    }

    /** Component wise max computation */
    [[nodiscard]] inline auto max(const Vec2& rhs) const -> Vec2 {
        return {std::max(_v[0], rhs._v[0]), std::max(_v[1], rhs._v[1])};
    }

    /** Component wise min computation */
    [[nodiscard]] inline auto min(const Vec2& rhs) const -> Vec2 {
        return {std::min(_v[0], rhs._v[0]), std::min(_v[1], rhs._v[1])};
    }

    /** Return the floor of each component */
    [[nodiscard]] inline auto floor() const -> Vec2 {
        return {std::floor(_v[0]), std::floor(_v[1])};
    }

    /** Return the ceil of each component */
    [[nodiscard]] inline auto ceil() const -> Vec2 {
        return {std::ceil(_v[0]), std::ceil(_v[1])};
    }

    template <class Archive> void serialize(Archive& ar) { ar(_v); }
}; // end of class Vec2

/** multiply by vector components. */
template <typename T>
inline auto componentMultiply(const Vec2<T>& lhs, const Vec2<T>& rhs)
    -> Vec2<T> {
    return {lhs[0] * rhs[0], lhs[1] * rhs[1]};
}

/** divide rhs components by rhs vector components. */
template <typename T>
inline auto componentDivide(const Vec2<T>& lhs, const Vec2<T>& rhs) -> Vec2<T> {
    return {lhs[0] / rhs[0], lhs[1] / rhs[1]};
}

using Vec2f = Vec2<F32>;
using Vec2d = Vec2<F64>;
using Vec2i = Vec2<I32>;
using Vec2u = Vec2<U32>;

// Specialized version of normalize() for I32
template <> inline auto Vec2<I32>::normalize() -> value_t {
    // Your specialized implementation for I32
    value_t norm = length();
    // For example, maybe round the results:
    if (norm > 0) {
        F64 inv = 1.0 / norm;
        _v[0] = std::lround((double)(_v[0]) * inv);
        _v[1] = std::lround((double)(_v[1]) * inv);
    }
    return norm;
}

template <> inline auto Vec2<U32>::normalize() -> value_t {
    // Your specialized implementation for U32
    value_t norm = length();
    // For example, maybe round the results:
    if (norm > 0) {
        F64 inv = 1.0 / norm;
        _v[0] = static_cast<U32>(_v[0] * inv + 0.5);
        _v[1] = static_cast<U32>(_v[1] * inv + 0.5);
    }
    return norm;
}

} // end of namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Vec2f& vec)
    -> std::ostream& {
    os << "nv::Vec2f(" << vec.x() << ", " << vec.y() << ")";
    return os;
}

inline auto operator<<(std::ostream& os, const nv::Vec2d& vec)
    -> std::ostream& {
    os << "nv::Vec2d(" << vec.x() << ", " << vec.y() << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Vec2f> {
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Vec2f vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec2f({:6g}, {:6g})", vec[0], vec[1]);
    }
};

template <> struct fmt::formatter<nv::Vec2d> {
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Vec2d vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec2d({:6g}, {:6g})", vec[0], vec[1]);
    }
};

template <> struct fmt::formatter<nv::Vec2i> {
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
    auto format(const nv::Vec2i vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec2i({}, {})", vec[0], vec[1]);
    }
};

template <> struct fmt::formatter<nv::Vec2u> {
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
    auto format(const nv::Vec2u vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec2u({}, {})", vec[0], vec[1]);
    }
};

#endif
