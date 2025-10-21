#ifndef _NV_VEC4_H_
#define _NV_VEC4_H_

#include <nvk/math/Vec3.h>

namespace nv {

template <typename T> struct Vec4 {
    /** Data type of vector components.*/
    using value_t = T;

    /** Number of vector components. */
    enum { num_components = 4 };

    std::array<value_t, 4> _v{};

    /** Constructor that sets all components of the vector to zero */
    constexpr Vec4() {
        _v[0] = 0.0;
        _v[1] = 0.0;
        _v[2] = 0.0;
        _v[3] = 0.0;
    }

    constexpr explicit Vec4(value_t x) {
        _v[0] = x;
        _v[1] = x;
        _v[2] = x;
        _v[3] = x;
    }

    constexpr Vec4(value_t x, value_t y, value_t z, value_t w) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
        _v[3] = w;
    }

    constexpr Vec4(const Vec3<T>& v3, value_t w) {
        _v[0] = v3[0];
        _v[1] = v3[1];
        _v[2] = v3[2];
        _v[3] = w;
    }

    constexpr Vec4(const Vec2<T>& v0, const Vec2<T>& v1) {
        _v[0] = v0[0];
        _v[1] = v0[1];
        _v[2] = v1[0];
        _v[3] = v1[1];
    }

    constexpr Vec4(const Vec2<T>& v0, value_t z, value_t w) {
        _v[0] = v0[0];
        _v[1] = v0[1];
        _v[2] = z;
        _v[3] = w;
    }

    ~Vec4() = default;

    template <typename U> inline explicit Vec4(const Vec4<U>& vec) {
        _v[0] = (value_t)vec._v[0];
        _v[1] = (value_t)vec._v[1];
        _v[2] = (value_t)vec._v[2];
        _v[3] = (value_t)vec._v[3];
    }

    inline auto operator==(const Vec4& v) const -> bool {
        return _v[0] == v._v[0] && _v[1] == v._v[1] && _v[2] == v._v[2] &&
               _v[3] == v._v[3];
    }

    inline auto operator!=(const Vec4& v) const -> bool {
        return _v[0] != v._v[0] || _v[1] != v._v[1] || _v[2] != v._v[2] ||
               _v[3] != v._v[3];
    }

    inline auto operator<(const Vec4& v) const -> bool {
        if (_v[0] < v._v[0]) {
            return true;
        }
        if (_v[0] > v._v[0]) {
            return false;
        }
        if (_v[1] < v._v[1]) {
            return true;
        }
        if (_v[1] > v._v[1]) {
            return false;
        }
        if (_v[2] < v._v[2]) {
            return true;
        }
        if (_v[2] > v._v[2]) {
            return false;
        }
        return (_v[3] < v._v[3]);
    }

    inline auto ptr() -> value_t* { return _v.data(); }
    [[nodiscard]] inline auto ptr() const -> const value_t* {
        return _v.data();
    }

    template <typename U> inline void set(const Vec4<U>& rhs) {
        _v[0] = rhs._v[0];
        _v[1] = rhs._v[1];
        _v[2] = rhs._v[2];
        _v[3] = rhs._v[3];
    }

    template <typename U> inline void set(const Vec3<U>& rhs, value_t w) {
        _v[0] = rhs._v[0];
        _v[1] = rhs._v[1];
        _v[2] = rhs._v[2];
        _v[3] = w;
    }

    template <typename U, typename V>
    inline void set(const Vec2<U>& lhs, const Vec2<V>& rhs) {
        _v[0] = lhs._v[0];
        _v[1] = lhs._v[1];
        _v[2] = rhs._v[0];
        _v[3] = rhs._v[1];
    }

    inline void set(value_t x, value_t y, value_t z, value_t w) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
        _v[3] = w;
    }

    inline auto operator[](unsigned int i) -> value_t& { return _v[i]; }
    inline auto operator[](unsigned int i) const -> value_t { return _v[i]; }

    inline auto x() -> value_t& { return _v[0]; }
    inline auto y() -> value_t& { return _v[1]; }
    inline auto z() -> value_t& { return _v[2]; }
    inline auto w() -> value_t& { return _v[3]; }

    [[nodiscard]] inline auto x() const -> value_t { return _v[0]; }
    [[nodiscard]] inline auto y() const -> value_t { return _v[1]; }
    [[nodiscard]] inline auto z() const -> value_t { return _v[2]; }
    [[nodiscard]] inline auto w() const -> value_t { return _v[3]; }

    inline auto r() -> value_t& { return _v[0]; }
    inline auto g() -> value_t& { return _v[1]; }
    inline auto b() -> value_t& { return _v[2]; }
    inline auto a() -> value_t& { return _v[3]; }

    [[nodiscard]] inline auto r() const -> value_t { return _v[0]; }
    [[nodiscard]] inline auto g() const -> value_t { return _v[1]; }
    [[nodiscard]] inline auto b() const -> value_t { return _v[2]; }
    [[nodiscard]] inline auto a() const -> value_t { return _v[3]; }

    [[nodiscard]] inline auto asABGR() const -> unsigned int {
        return (unsigned int)nv::clamp((_v[0] * 255.0), 0.0, 255.0) << 24 |
               (unsigned int)nv::clamp((_v[1] * 255.0), 0.0, 255.0) << 16 |
               (unsigned int)nv::clamp((_v[2] * 255.0), 0.0, 255.0) << 8 |
               (unsigned int)nv::clamp((_v[3] * 255.0), 0.0, 255.0);
    }

    [[nodiscard]] inline auto asRGBA() const -> unsigned int {
        return (unsigned int)nv::clamp((_v[3] * 255.0), 0.0, 255.0) << 24 |
               (unsigned int)nv::clamp((_v[2] * 255.0), 0.0, 255.0) << 16 |
               (unsigned int)nv::clamp((_v[1] * 255.0), 0.0, 255.0) << 8 |
               (unsigned int)nv::clamp((_v[0] * 255.0), 0.0, 255.0);
    }

    /** Returns true if all components have values that are not NaN. */
    [[nodiscard]] inline auto valid() const -> bool { return !isNaN(); }
    /** Returns true if at least one component has value NaN. */
    [[nodiscard]] inline auto isNaN() const -> bool {
        return nv::isNaN(_v[0]) || nv::isNaN(_v[1]) || nv::isNaN(_v[2]) ||
               nv::isNaN(_v[3]);
    }

    /** Component multiplication */
    inline auto operator*(const Vec4& rhs) const -> Vec4 {
        return {_v[0] * rhs._v[0], _v[1] * rhs._v[1], _v[2] * rhs._v[2],
                _v[3] * rhs._v[3]};
    }

    /** Multiply by scalar. */
    inline auto operator*(value_t rhs) const -> Vec4 {
        return {_v[0] * rhs, _v[1] * rhs, _v[2] * rhs, _v[3] * rhs};
    }

    /** Unary multiply by scalar. */
    inline auto operator*=(value_t rhs) -> Vec4& {
        _v[0] *= rhs;
        _v[1] *= rhs;
        _v[2] *= rhs;
        _v[3] *= rhs;
        return *this;
    }

    /** Divide by scalar. */
    inline auto operator/(value_t rhs) const -> Vec4 {
        return {_v[0] / rhs, _v[1] / rhs, _v[2] / rhs, _v[3] / rhs};
    }

    /** Unary divide by scalar. */
    inline auto operator/=(value_t rhs) -> Vec4& {
        _v[0] /= rhs;
        _v[1] /= rhs;
        _v[2] /= rhs;
        _v[3] /= rhs;
        return *this;
    }

    /** Divide by Vec4 */
    inline auto operator/(Vec4 rhs) const -> Vec4 {
        return {_v[0] / rhs._v[0], _v[1] / rhs._v[1], _v[2] / rhs._v[2],
                _v[3] / rhs._v[3]};
    }

    /** Binary vector add. */
    inline auto operator+(const Vec4& rhs) const -> Vec4 {
        return {_v[0] + rhs._v[0], _v[1] + rhs._v[1], _v[2] + rhs._v[2],
                _v[3] + rhs._v[3]};
    }

    inline auto operator+(value_t val) const -> Vec4 {
        return {_v[0] + val, _v[1] + val, _v[2] + val, _v[3] + val};
    }

    /** Unary vector add. Slightly more efficient because no temporary
     * intermediate object.
     */
    inline auto operator+=(const Vec4& rhs) -> Vec4& {
        _v[0] += rhs._v[0];
        _v[1] += rhs._v[1];
        _v[2] += rhs._v[2];
        _v[3] += rhs._v[3];
        return *this;
    }

    inline auto operator+=(value_t val) -> Vec4& {
        _v[0] += val;
        _v[1] += val;
        _v[2] += val;
        _v[3] += val;
        return *this;
    }

    /** Binary vector subtract. */
    inline auto operator-(const Vec4& rhs) const -> Vec4 {
        return {_v[0] - rhs._v[0], _v[1] - rhs._v[1], _v[2] - rhs._v[2],
                _v[3] - rhs._v[3]};
    }

    /** Unary vector subtract. */
    inline auto operator-=(const Vec4& rhs) -> Vec4& {
        _v[0] -= rhs._v[0];
        _v[1] -= rhs._v[1];
        _v[2] -= rhs._v[2];
        _v[3] -= rhs._v[3];
        return *this;
    }

    /** Negation operator. Returns the negative of the Vec4. */
    inline auto operator-() const -> Vec4 {
        return {-_v[0], -_v[1], -_v[2], -_v[3]};
    }

    /** Length of the vector = sqrt( vec . vec ) */
    [[nodiscard]] inline auto length() const -> value_t {
        return sqrt(_v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2] +
                    _v[3] * _v[3]);
    }

    /** Length squared of the vector = vec . vec */
    [[nodiscard]] inline auto length2() const -> value_t {
        return _v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2] + _v[3] * _v[3];
    }

    /** Normalize the vector so that it has length unity.
     * Returns the previous length of the vector.
     */
    inline auto normalize() -> value_t {
        value_t norm = Vec4::length();
        if (norm > 0.0F) {
            value_t inv = 1.0 / norm;
            _v[0] *= inv;
            _v[1] *= inv;
            _v[2] *= inv;
            _v[3] *= inv;
        }
        return (norm);
    }

    inline auto normalized() -> Vec4 {
        Vec4 res{_v[0], _v[1], _v[2], _v[3]};
        res.normalize();
        return res;
    }

    /** Get the abs of this vector: */
    [[nodiscard]] inline auto abs() const -> Vec4 {
        return {fabs(_v[0]), fabs(_v[1]), fabs(_v[2]), fabs(_v[3])};
    }

    /** Get the maximum of the components */
    [[nodiscard]] inline auto maximum() const -> value_t {
        return std::max(std::max(_v[0], _v[1]), std::max(_v[2], _v[3]));
    }

    /** Get the minimum of the components */
    [[nodiscard]] inline auto minimum() const -> value_t {
        return std::min(std::min(_v[0], _v[1]), std::min(_v[2], _v[3]));
    }

    /** Get the range of data */
    [[nodiscard]] inline auto range() const -> Range<value_t> {
        return {minimum(), maximum()};
    }

    /** Component wise max computation */
    [[nodiscard]] inline auto max(const Vec4& rhs) const -> Vec4 {
        return {std::max(_v[0], rhs._v[0]), std::max(_v[1], rhs._v[1]),
                std::max(_v[2], rhs._v[2]), std::max(_v[3], rhs._v[3])};
    }

    /** Component wise min computation */
    [[nodiscard]] inline auto min(const Vec4& rhs) const -> Vec4 {
        return {std::min(_v[0], rhs._v[0]), std::min(_v[1], rhs._v[1]),
                std::min(_v[2], rhs._v[2]), std::min(_v[3], rhs._v[3])};
    }

    [[nodiscard]] inline auto inverse() const -> Vec4 {
        return {(value_t)(_v[0] == 0 ? 0.0 : 1.0 / _v[0]),
                (value_t)(_v[1] == 0 ? 0.0 : 1.0 / _v[1]),
                (value_t)(_v[2] == 0 ? 0.0 : 1.0 / _v[2]),
                (value_t)(_v[3] == 0 ? 0.0 : 1.0 / _v[3])};
    }

    /** Quick accessors */
    [[nodiscard]] auto xyz() const -> Vec3<T> { return {_v[0], _v[1], _v[2]}; }
    [[nodiscard]] auto xy() const -> Vec2<T> { return {_v[0], _v[1]}; }
    [[nodiscard]] auto xz() const -> Vec2<T> { return {_v[0], _v[2]}; }
    [[nodiscard]] auto zw() const -> Vec2<T> { return {_v[2], _v[3]}; }
    [[nodiscard]] auto yw() const -> Vec2<T> { return {_v[1], _v[3]}; }

    template <class Archive> void serialize(Archive& ar) { ar(_v); }
}; // end of class Vec4

/** Compute the dot product of a (Vec3,1.0) and a Vec4. */
template <typename T>
inline auto operator*(const Vec3<T>& lhs, const Vec4<T>& rhs) -> T {
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2] + rhs[3];
}

/** Compute the dot product of a Vec4 and a (Vec3,1.0). */
template <typename T>
inline auto operator*(const Vec4<T>& lhs, const Vec3<T>& rhs) -> T {
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3];
}

/** multiply by vector components. */
template <typename T>
inline auto componentMultiply(const Vec4<T>& lhs, const Vec4<T>& rhs)
    -> Vec4<T> {
    return {lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2], lhs[3] * rhs[3]};
}

/** divide rhs components by rhs vector components. */
template <typename T>
inline auto componentDivide(const Vec4<T>& lhs, const Vec4<T>& rhs) -> Vec4<T> {
    return {lhs[0] / rhs[0], lhs[1] / rhs[1], lhs[2] / rhs[2], lhs[3] / rhs[3]};
}

template <typename T>
inline auto mix(const Vec4<T>& lhs, const Vec4<T>& rhs, T alpha) -> Vec4<T> {
    return lhs * (1.0 - alpha) + rhs * alpha;
}

using Vec4f = Vec4<F32>;
using Vec4d = Vec4<F64>;
using Vec4i = Vec4<I32>;
using Vec4u = Vec4<U32>;

} // namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Vec4d& vec)
    -> std::ostream& {
    os << "nv::Vec4d(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", "
       << vec.w() << ")";
    return os;
}

inline auto operator<<(std::ostream& os, const nv::Vec4f& vec)
    -> std::ostream& {
    os << "nv::Vec4f(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", "
       << vec.w() << ")";
    return os;
}

inline auto operator<<(std::ostream& os, const nv::Vec4i& vec)
    -> std::ostream& {
    os << "nv::Vec4i(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", "
       << vec.w() << ")";
    return os;
}

inline auto operator<<(std::ostream& os, const nv::Vec4u& vec)
    -> std::ostream& {
    os << "nv::Vec4u(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", "
       << vec.w() << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Vec4f> {
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
    auto format(const nv::Vec4f vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec4f({:6g}, {:6g}, {:6g}, {:6g})",
                              vec[0], vec[1], vec[2], vec[3]);
    }
};

template <> struct fmt::formatter<nv::Vec4d> {
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
    auto format(const nv::Vec4d vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec4d({:6g}, {:6g}, {:6g}, {:6g})",
                              vec[0], vec[1], vec[2], vec[3]);
    }
};

#endif
