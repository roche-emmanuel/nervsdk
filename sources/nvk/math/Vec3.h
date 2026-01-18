#ifndef _NV_VEC3_H_
#define _NV_VEC3_H_

#include <nvk/math/Vec2.h>
#include <nvk_math.h>

namespace nv {

template <typename T> struct Vec3 {
    /** Data type of vector components.*/
    using value_t = T;

    /** Number of vector components. */
    enum { num_components = 3 };

    std::array<value_t, 3> _v{};

    /** Constructor that sets all components of the vector to zero */
    constexpr Vec3() {
        _v[0] = 0.0F;
        _v[1] = 0.0F;
        _v[2] = 0.0F;
    }

    constexpr explicit Vec3(value_t x) {
        _v[0] = x;
        _v[1] = x;
        _v[2] = x;
    }

    constexpr Vec3(value_t x, value_t y, value_t z) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
    }

    Vec3(const Vec2<T>& v2, value_t zz) {
        _v[0] = v2[0];
        _v[1] = v2[1];
        _v[2] = zz;
    }

    ~Vec3() = default;

    template <typename U> explicit Vec3(const Vec3<U>& rhs) {
        _v[0] = (value_t)rhs._v[0];
        _v[1] = (value_t)rhs._v[1];
        _v[2] = (value_t)rhs._v[2];
    }

    inline auto operator==(const Vec3& v) const -> bool {
        return _v[0] == v._v[0] && _v[1] == v._v[1] && _v[2] == v._v[2];
    }

    inline auto operator!=(const Vec3& v) const -> bool {
        return _v[0] != v._v[0] || _v[1] != v._v[1] || _v[2] != v._v[2];
    }

    inline auto operator<(const Vec3& v) const -> bool {
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
        return (_v[2] < v._v[2]);
    }

    inline auto ptr() -> value_t* { return _v.data(); }
    [[nodiscard]] inline auto ptr() const -> const value_t* {
        return _v.data();
    }

    inline void set(value_t x, value_t y, value_t z) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
    }

    inline void set(const Vec3& rhs) {
        _v[0] = rhs._v[0];
        _v[1] = rhs._v[1];
        _v[2] = rhs._v[2];
    }

    inline auto operator[](int i) -> value_t& { return _v[i]; }
    inline auto operator[](int i) const -> value_t { return _v[i]; }

    inline auto x() -> value_t& { return _v[0]; }
    inline auto y() -> value_t& { return _v[1]; }
    inline auto z() -> value_t& { return _v[2]; }

    [[nodiscard]] inline auto x() const -> value_t { return _v[0]; }
    [[nodiscard]] inline auto y() const -> value_t { return _v[1]; }
    [[nodiscard]] inline auto z() const -> value_t { return _v[2]; }

    /** Returns true if all components have values that are not NaN. */
    [[nodiscard]] inline auto valid() const -> bool { return !isNaN(); }

    /** Returns true if at least one component has value NaN. */
    [[nodiscard]] inline auto isNaN() const -> bool {
        return nv::isNaN(_v[0]) || nv::isNaN(_v[1]) || nv::isNaN(_v[2]);
    }

    /** component multiplication. */
    inline auto operator*(const Vec3& rhs) const -> Vec3 {
        return {_v[0] * rhs._v[0], _v[1] * rhs._v[1], _v[2] * rhs._v[2]};
    }

    [[nodiscard]] inline auto dot(const Vec3& rhs) const -> value_t {
        return _v[0] * rhs._v[0] + _v[1] * rhs._v[1] + _v[2] * rhs._v[2];
    }

    /** Cross product. */
    inline auto operator^(const Vec3& rhs) const -> Vec3 {
        return {_v[1] * rhs._v[2] - _v[2] * rhs._v[1],
                _v[2] * rhs._v[0] - _v[0] * rhs._v[2],
                _v[0] * rhs._v[1] - _v[1] * rhs._v[0]};
    }

    [[nodiscard]] inline auto cross(const Vec3& rhs) const -> Vec3 {
        return {_v[1] * rhs._v[2] - _v[2] * rhs._v[1],
                _v[2] * rhs._v[0] - _v[0] * rhs._v[2],
                _v[0] * rhs._v[1] - _v[1] * rhs._v[0]};
    }

    /** Multiply by scalar. */
    inline auto operator*(value_t rhs) const -> Vec3 {
        return {_v[0] * rhs, _v[1] * rhs, _v[2] * rhs};
    }

    /** Unary multiply by scalar. */
    inline auto operator*=(value_t rhs) -> Vec3& {
        _v[0] *= rhs;
        _v[1] *= rhs;
        _v[2] *= rhs;
        return *this;
    }

    /** Divide by scalar. */
    inline auto operator/(value_t rhs) const -> Vec3 {
        return {_v[0] / rhs, _v[1] / rhs, _v[2] / rhs};
    }

    /** Unary divide by scalar. */
    inline auto operator/=(value_t rhs) -> Vec3& {
        _v[0] /= rhs;
        _v[1] /= rhs;
        _v[2] /= rhs;
        return *this;
    }

    /** Divide by Vec3 */
    inline auto operator/(Vec3 rhs) const -> Vec3 {
        return {_v[0] / rhs._v[0], _v[1] / rhs._v[1], _v[2] / rhs._v[2]};
    }

    /** Binary vector add. */
    inline auto operator+(const Vec3& rhs) const -> Vec3 {
        return {_v[0] + rhs._v[0], _v[1] + rhs._v[1], _v[2] + rhs._v[2]};
    }

    inline auto operator+(value_t val) const -> Vec3 {
        return {_v[0] + val, _v[1] + val, _v[2] + val};
    }

    /** Unary vector add. Slightly more efficient because no temporary
     * intermediate object.
     */
    inline auto operator+=(const Vec3& rhs) -> Vec3& {
        _v[0] += rhs._v[0];
        _v[1] += rhs._v[1];
        _v[2] += rhs._v[2];
        return *this;
    }

    inline auto operator+=(value_t val) -> Vec3& {
        _v[0] += val;
        _v[1] += val;
        _v[2] += val;
        return *this;
    }

    /** Binary vector subtract. */
    inline auto operator-(const Vec3& rhs) const -> Vec3 {
        return {_v[0] - rhs._v[0], _v[1] - rhs._v[1], _v[2] - rhs._v[2]};
    }

    /** Unary vector subtract. */
    inline auto operator-=(const Vec3& rhs) -> Vec3& {
        _v[0] -= rhs._v[0];
        _v[1] -= rhs._v[1];
        _v[2] -= rhs._v[2];
        return *this;
    }

    /** Negation operator. Returns the negative of the Vec3. */
    inline auto operator-() const -> Vec3 { return {-_v[0], -_v[1], -_v[2]}; }

    /** Length of the vector = sqrt( vec . vec ) */
    [[nodiscard]] inline auto length() const -> value_t {
        return sqrt(_v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2]);
    }

    /** Length squared of the vector = vec . vec */
    [[nodiscard]] inline auto length2() const -> value_t {
        return _v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2];
    }

    /** Normalize the vector so that it has length unity.
     * Returns the previous length of the vector.
     */
    inline auto normalize() -> value_t {
        value_t norm = Vec3::length();
        if (norm > 0.0) {
            value_t inv = 1.0F / norm;
            _v[0] *= inv;
            _v[1] *= inv;
            _v[2] *= inv;
        }
        return (norm);
    }

    inline auto normalize(value_t newLen) -> value_t {
        value_t norm = Vec3::length();
        if (norm > 0.0) {
            value_t inv = newLen / norm;
            _v[0] *= inv;
            _v[1] *= inv;
            _v[2] *= inv;
        }
        return (norm);
    }

    inline auto normalized() const -> Vec3 {
        Vec3 res{_v[0], _v[1], _v[2]};
        res.normalize();
        return res;
    }

    inline auto normalized(value_t* prevLength) const -> Vec3 {
        Vec3 res{_v[0], _v[1], _v[2]};
        value_t norm = res.normalize();
        if (prevLength != nullptr) {
            *prevLength = norm;
        }
        return res;
    }

    inline auto mix(Vec3 rhs, value_t ratio) -> Vec3& {
        _v[0] += (rhs._v[0] - _v[0]) * ratio;
        _v[1] += (rhs._v[1] - _v[1]) * ratio;
        _v[2] += (rhs._v[2] - _v[2]) * ratio;
        return *this;
    }

    [[nodiscard]] inline auto mixed(Vec3 rhs, value_t ratio) -> Vec3 {
        return {_v[0] + (rhs._v[0] - _v[0]) * ratio,
                _v[1] + (rhs._v[1] - _v[1]) * ratio,
                _v[2] + (rhs._v[2] - _v[2]) * ratio};
    }

    /** Get the abs of this vector: */
    [[nodiscard]] inline auto abs() const -> Vec3 {
        return {fabs(_v[0]), fabs(_v[1]), fabs(_v[2])};
    }

    /** Get the maximum of the components */
    [[nodiscard]] inline auto maximum() const -> value_t {
        return nv::maximum(_v[0], nv::maximum(_v[1], _v[2]));
    }

    /** Get the minimum of the components */
    [[nodiscard]] inline auto minimum() const -> value_t {
        return nv::minimum(_v[0], nv::minimum(_v[1], _v[2]));
    }

    /** Get the range of data */
    [[nodiscard]] inline auto range() const -> Range<value_t> {
        return {minimum(), maximum()};
    }

    /** Component wise max computation */
    [[nodiscard]] inline auto max(const Vec3& rhs) const -> Vec3 {
        return {std::max(_v[0], rhs._v[0]), std::max(_v[1], rhs._v[1]),
                std::max(_v[2], rhs._v[2])};
    }

    /** Component wise min computation */
    [[nodiscard]] inline auto min(const Vec3& rhs) const -> Vec3 {
        return {std::min(_v[0], rhs._v[0]), std::min(_v[1], rhs._v[1]),
                std::min(_v[2], rhs._v[2])};
    }

    [[nodiscard]] inline auto inverse() const -> Vec3 {
        return {(value_t)(_v[0] == 0 ? 0.0 : 1.0 / _v[0]),
                (value_t)(_v[1] == 0 ? 0.0 : 1.0 / _v[1]),
                (value_t)(_v[2] == 0 ? 0.0 : 1.0 / _v[2])};
    }

    /** Quick accessors */
    [[nodiscard]] auto xy() const -> Vec2<T> { return {_v[0], _v[1]}; }
    [[nodiscard]] auto yx() const -> Vec2<T> { return {_v[1], _v[0]}; }
    [[nodiscard]] auto yz() const -> Vec2<T> { return {_v[1], _v[2]}; }
    [[nodiscard]] auto xz() const -> Vec2<T> { return {_v[0], _v[2]}; }

    /** Component multiply */
    [[nodiscard]] auto mult(const Vec3 rhs) const -> Vec3 {
        return {_v[0] * rhs._v[0], _v[1] * rhs._v[1], _v[2] * rhs._v[2]};
    }

    /** Compute the angle between this vector and another vector in radians.
     * Returns the angle in the range [0, pi].
     */
    [[nodiscard]] auto angleTo(const Vec3& rhs) const -> value_t {
        value_t dot_product = dot(rhs);
        value_t len_product = length() * rhs.length();

        if (len_product == 0.0) {
            return 0.0;
        }

        // Clamp to avoid numerical issues with acos
        value_t cos_angle = dot_product / len_product;
        cos_angle = nv::clamp(cos_angle, static_cast<value_t>(-1.0),
                              static_cast<value_t>(1.0));

        return std::acos(cos_angle);
    }

    /** Compute the signed angle from this vector to another vector in radians,
     * relative to a reference axis (normal vector).
     * Returns the angle in the range [-pi, pi].
     * Positive angles indicate counter-clockwise rotation when viewing along
     * the normal.
     *
     * @param rhs The target vector
     * @param normal The reference axis (should be normalized and perpendicular
     * to the plane)
     */
    [[nodiscard]] auto signedAngleTo(const Vec3& rhs, const Vec3& normal) const
        -> value_t {
        Vec3 cross_product = cross(rhs);
        value_t sin_angle = cross_product.length();
        value_t cos_angle = dot(rhs);

        // Determine sign based on the normal
        if (cross_product.dot(normal) < 0.0) {
            sin_angle = -sin_angle;
        }

        return std::atan2(sin_angle, cos_angle);
    }

    template <class Archive> void serialize(Archive& ar) { ar(_v); }
}; // end of class Vec3

/** multiply by vector components. */
template <typename T>
inline auto componentMultiply(const Vec3<T>& lhs, const Vec3<T>& rhs)
    -> Vec3<T> {
    return {lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]};
}

/** divide rhs components by rhs vector components. */
template <typename T>
inline auto componentDivide(const Vec3<T>& lhs, const Vec3<T>& rhs) -> Vec3<T> {
    return {lhs[0] / rhs[0], lhs[1] / rhs[1], lhs[2] / rhs[2]};
}

template <typename T> inline auto exp(const Vec3<T>& lhs) -> Vec3<T> {
    return {std::exp(lhs[0]), std::exp(lhs[1]), std::exp(lhs[2])};
}

template <typename T>
inline auto componentMaximum(const Vec3<T>& lhs, const Vec3<T>& rhs)
    -> Vec3<T> {
    return {std::max(lhs[0], rhs[0]), std::max(lhs[1], rhs[1]),
            std::max(lhs[2], rhs[2])};
}

template <typename T>
inline auto componentMinimum(const Vec3<T>& lhs, const Vec3<T>& rhs)
    -> Vec3<T> {
    return {std::min(lhs[0], rhs[0]), std::min(lhs[1], rhs[1]),
            std::min(lhs[2], rhs[2])};
}

// const Vec3 X_AXIS(1.0,0.0,0.0);
// const Vec3 Y_AXIS(0.0,1.0,0.0);
// const Vec3 Z_AXIS(0.0,0.0,1.0);

using Vec3f = Vec3<F32>;
using Vec3d = Vec3<F64>;
using Vec3i = Vec3<I32>;
using Vec3u = Vec3<U32>;

constexpr Vec3f VEC3F_UP{0.0, -1.0, 0.0};
constexpr Vec3f VEC3F_RIGHT{1.0, 0.0, 0.0};
constexpr Vec3f VEC3F_FWD{0.0, 0.0, 1.0};
constexpr Vec3f VEC3F_ZERO{0.0, 0.0, 0.0};
constexpr Vec3f VEC3F_ONE{1.0, 1.0, 1.0};
constexpr Vec3f VEC3F_XAXIS{1.0, 0.0, 0.0};
constexpr Vec3f VEC3F_YAXIS{0.0, 1.0, 0.0};
constexpr Vec3f VEC3F_ZAXIS{0.0, 0.0, 1.0};

constexpr Vec3d VEC3D_UP{0.0, -1.0, 0.0};
constexpr Vec3d VEC3D_RIGHT{1.0, 0.0, 0.0};
constexpr Vec3d VEC3D_FWD{0.0, 0.0, 1.0};
constexpr Vec3d VEC3D_ZERO{0.0, 0.0, 0.0};
constexpr Vec3d VEC3D_ONE{1.0, 1.0, 1.0};
constexpr Vec3d VEC3D_XAXIS{1.0, 0.0, 0.0};
constexpr Vec3d VEC3D_YAXIS{0.0, 1.0, 0.0};
constexpr Vec3d VEC3D_ZAXIS{0.0, 0.0, 1.0};

inline void to_json(nlohmann::json& j, const Vec3d& vec) {
    j = nlohmann::json::array({vec.x(), vec.y(), vec.z()});
}

inline void from_json(const nlohmann::json& j, Vec3d& vec) {
    j.at(0).get_to(vec.x());
    j.at(1).get_to(vec.y());
    j.at(2).get_to(vec.z());
}

} // end of namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Vec3f& vec)
    -> std::ostream& {
    os << "nv::Vec3f(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Vec3d& vec)
    -> std::ostream& {
    os << "nv::Vec3d(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Vec3i& vec)
    -> std::ostream& {
    os << "nv::Vec3i(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Vec3u& vec)
    -> std::ostream& {
    os << "nv::Vec3u(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Vec3f> {
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
    auto format(const nv::Vec3f vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec3f({:6g}, {:6g}, {:6g})", vec[0],
                              vec[1], vec[2]);
    }
};

template <> struct fmt::formatter<nv::Vec3d> {
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
    auto format(const nv::Vec3d vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec3d({:6g}, {:6g}, {:6g})", vec[0],
                              vec[1], vec[2]);
    }
};

template <> struct fmt::formatter<nv::Vec3i> {
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
    auto format(const nv::Vec3i vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec3i({}, {}, {})", vec[0], vec[1],
                              vec[2]);
    }
};

template <> struct fmt::formatter<nv::Vec3u> {
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
    auto format(const nv::Vec3u vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vec3u({}, {}, {})", vec[0], vec[1],
                              vec[2]);
    }
};

#endif
