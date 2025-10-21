#ifndef _NV_MAT2_H_
#define _NV_MAT2_H_

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <limits.h>
#include <nvk/math/Quat.h>
#include <nvk/math/Vec2.h>

namespace nv {

#define MAT_RC(row, col) _mat[(col)][(row)]

#define SET_ROW(row, v1, v2)                                                   \
    MAT_RC((row), 0) = (v1);                                                   \
    MAT_RC((row), 1) = (v2);

#define INNER_PRODUCT(a, b, r, c)                                              \
    ((a).MAT_RC(r, 0) * (b).MAT_RC(0, c)) +                                    \
        ((a).MAT_RC(r, 1) * (b).MAT_RC(1, c))

template <typename T> class Mat2 {
  public:
    using value_t = T;

    enum { num_elements = 4 };

    Mat2() { make_identity(); }

    Mat2(const Mat2& mat) { set(mat.ptr()); }

    Mat2(Mat2&& mat) noexcept { set(mat.ptr()); }

    auto operator=(const Mat2& rhs) -> Mat2& {
        if (&rhs == this)
            return *this;
        set(rhs.ptr());
        return *this;
    }

    auto operator=(Mat2&& rhs) noexcept -> Mat2& {
        if (&rhs == this)
            return *this;
        set(rhs.ptr());
        return *this;
    }

    // inline explicit Mat2( float const * const ptr ) { set(ptr); }
    // inline explicit Mat2( double const * const ptr ) { set(ptr); }
    explicit Mat2(const Quaternion<T>& quat) { make_rotate(quat); }

    Mat2(value_t a00, value_t a01, value_t a10, value_t a11) {
        SET_ROW(0, a00, a01);
        SET_ROW(1, a10, a11);
    }

    ~Mat2() = default;

    [[nodiscard]] auto compare(const Mat2& m) const -> int {
        const auto* lhs = (const value_t*)(_mat);
        const value_t* end_lhs = lhs + num_elements;
        const auto* rhs = (const value_t*)(m._mat);
        for (; lhs != end_lhs; ++lhs, ++rhs) {
            if (*lhs < *rhs)
                return -1;
            if (*rhs < *lhs)
                return 1;
        }
        return 0;
    }

    auto operator<(const Mat2& m) const -> bool { return compare(m) < 0; }
    auto operator==(const Mat2& m) const -> bool { return compare(m) == 0; }
    auto operator!=(const Mat2& m) const -> bool { return compare(m) != 0; }

    inline auto operator()(int row, int col) -> value_t& {
        return MAT_RC(row, col);
    }
    inline auto operator()(int row, int col) const -> value_t {
        return MAT_RC(row, col);
    }

    [[nodiscard]] inline auto valid() const -> bool { return !is_nan(); }
    [[nodiscard]] inline auto is_nan() const -> bool {
        const auto* ptr = (const value_t*)_mat;

        for (int i = 0; i < num_elements; ++i, ++ptr) {
            if (std::isnan(*ptr)) {
                return true;
            }
        }
        return false;
    }

    // inline void set(const Mat2& rhs, bool transpose = false) {
    inline void set(const Mat2& rhs) { set(rhs.ptr()); }

    template <typename U> inline void set(const Mat2<U>& rhs) {
        T* dst = ptr();
        const U* src = rhs.ptr();
        for (int i = 0; i < num_elements; ++i) {
            (*dst++) = (T)(*src++);
        }
    }

    inline void set(value_t const* const ptr) {

        // auto* local_ptr = (value_t*)_mat;
        // for (int i = 0; i < num_elements; ++i)
        //     local_ptr[i] = (value_t)ptr[i];
        memcpy((void*)_mat, (void*)ptr, num_elements * sizeof(value_t));
    }

    void set(value_t a00, value_t a01, value_t a10, value_t a11) {
        SET_ROW(0, a00, a01);
        SET_ROW(1, a10, a11);
    }

    auto ptr() -> value_t* { return (value_t*)_mat; }
    auto ptr() const -> const value_t* { return (const value_t*)_mat; }

    /** Build a rotation matrix with a given rotation angle (in radians)*/
    void set_rotate(value_t angle) {
        auto ct = (value_t)cos(angle);
        auto st = (value_t)cos(angle);

        SET_ROW(0, ct, -st);
        SET_ROW(1, st, ct);
    }

#if 0
    /** Get the matrix rotation as a Quat. Note that this function
     * assumes a non-scaled matrix and will return incorrect results
     * for scaled matrixces. Consider decompose() instead.
     */
    auto get_rotate() const -> Quaternion<T> {

    }
#endif

    [[nodiscard]] auto is_identity() const -> bool {
        return MAT_RC(0, 0) == 1 && MAT_RC(0, 1) == 0 && MAT_RC(1, 0) == 0 &&
               MAT_RC(1, 1) == 1;
    }

    void make_identity() {
        SET_ROW(0, 1, 0)
        SET_ROW(1, 0, 1)
    }

    void transpose() {
        value_t tmp;
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < i; ++j) {
                tmp = _mat[i][j];
                _mat[i][j] = _mat[j][i];
                _mat[j][i] = tmp;
            }
        }
    }

    void make_scale(value_t x, value_t y) {
        SET_ROW(0, x, 0)
        SET_ROW(1, 0, y)
    }

    void make_scale(const Vec2<T>& vec) { make_scale(vec[0], vec[1]); }

#if 0
    void make_rotate(const Vec3<T>& from, const Vec3<T>& to) {
        make_identity();

        Quaternion<T> quat;
        quat.make_rotate(from, to);
        set_rotate(quat);
    }

    void make_rotate(value_t angle, const Vec3<T>& axis) {
        make_identity();

        Quaternion<T> quat;
        quat.make_rotate(angle, axis);
        set_rotate(quat);
    }

    void make_rotate(value_t angle, value_t x, value_t y, value_t z) {
        make_identity();

        Quaternion<T> quat;
        quat.make_rotate(angle, x, y, z);
        set_rotate(quat);
    }

    void make_rotate(const Quaternion<T>& quat) {
        make_identity();
        set_rotate(quat);
    }

    void make_rotate(value_t angle1, const Vec3<T>& axis1, value_t angle2,
                     const Vec3<T>& axis2, value_t angle3,
                     const Vec3<T>& axis3) {
        make_identity();

        Quaternion<T> quat;
        quat.make_rotate(angle1, axis1, angle2, axis2, angle3, axis3);
        set_rotate(quat);
    }
#endif

    [[nodiscard]] auto determinant() const -> value_t {
        return MAT_RC(0, 0) * MAT_RC(1, 1) - MAT_RC(1, 0) * MAT_RC(0, 1);
    }

    /** invert the matrix rhs, automatically select invert_4x3 or invert_4x4. */
    inline auto invert(const Mat2& rhs, value_t tolerance = 1e-6) -> Bool {
        value_t det = rhs.determinant();

        if (fabs(det) <= tolerance) {
            SET_ROW(0, 0, 0);
            SET_ROW(1, 0, 0);
            return false;
        }

        value_t invDet = 1 / det;

        MAT_RC(0, 0) = rhs.MAT_RC(1, 1) * invDet;
        MAT_RC(0, 1) = -rhs.MAT_RC(0, 1) * invDet;
        MAT_RC(1, 0) = -rhs.MAT_RC(1, 0) * invDet;
        MAT_RC(1, 1) = rhs.MAT_RC(0, 0) * invDet;
        return true;
    }

    [[nodiscard]] inline auto inverse(value_t tolerance = 1e-6) const -> Mat2 {
        Mat2 m;
        m.invert(*this, tolerance);
        return m;
    }

#if 0
    /** ortho-normalize the 3x3 rotation & scale matrix */
    void ortho_normalize(const Mat2& rhs) {
        value_t x_colMag = (rhs.MAT_RC(0, 0) * rhs.MAT_RC(0, 0)) +
                           (rhs.MAT_RC(1, 0) * rhs.MAT_RC(1, 0)) +
                           (rhs.MAT_RC(2, 0) * rhs.MAT_RC(2, 0));
        value_t y_colMag = (rhs.MAT_RC(0, 1) * rhs.MAT_RC(0, 1)) +
                           (rhs.MAT_RC(1, 1) * rhs.MAT_RC(1, 1)) +
                           (rhs.MAT_RC(2, 1) * rhs.MAT_RC(2, 1));
        value_t z_colMag = (rhs.MAT_RC(0, 2) * rhs.MAT_RC(0, 2)) +
                           (rhs.MAT_RC(1, 2) * rhs.MAT_RC(1, 2)) +
                           (rhs.MAT_RC(2, 2) * rhs.MAT_RC(2, 2));

        if (!equivalent((double)x_colMag, 1.0) &&
            !equivalent((double)x_colMag, 0.0)) {
            x_colMag = sqrt(x_colMag);
            MAT_RC(0, 0) = rhs.MAT_RC(0, 0) / x_colMag;
            MAT_RC(1, 0) = rhs.MAT_RC(1, 0) / x_colMag;
            MAT_RC(2, 0) = rhs.MAT_RC(2, 0) / x_colMag;
        } else {
            MAT_RC(0, 0) = rhs.MAT_RC(0, 0);
            MAT_RC(1, 0) = rhs.MAT_RC(1, 0);
            MAT_RC(2, 0) = rhs.MAT_RC(2, 0);
        }

        if (!equivalent((double)y_colMag, 1.0) &&
            !equivalent((double)y_colMag, 0.0)) {
            y_colMag = sqrt(y_colMag);
            MAT_RC(0, 1) = rhs.MAT_RC(0, 1) / y_colMag;
            MAT_RC(1, 1) = rhs.MAT_RC(1, 1) / y_colMag;
            MAT_RC(2, 1) = rhs.MAT_RC(2, 1) / y_colMag;
        } else {
            MAT_RC(0, 1) = rhs.MAT_RC(0, 1);
            MAT_RC(1, 1) = rhs.MAT_RC(1, 1);
            MAT_RC(2, 1) = rhs.MAT_RC(2, 1);
        }

        if (!equivalent((double)z_colMag, 1.0) &&
            !equivalent((double)z_colMag, 0.0)) {
            z_colMag = sqrt(z_colMag);
            MAT_RC(0, 2) = rhs.MAT_RC(0, 2) / z_colMag;
            MAT_RC(1, 2) = rhs.MAT_RC(1, 2) / z_colMag;
            MAT_RC(2, 2) = rhs.MAT_RC(2, 2) / z_colMag;
        } else {
            MAT_RC(0, 2) = rhs.MAT_RC(0, 2);
            MAT_RC(1, 2) = rhs.MAT_RC(1, 2);
            MAT_RC(2, 2) = rhs.MAT_RC(2, 2);
        }

        MAT_RC(3, 0) = rhs.MAT_RC(3, 0);
        MAT_RC(3, 1) = rhs.MAT_RC(3, 1);
        MAT_RC(3, 2) = rhs.MAT_RC(3, 2);

        MAT_RC(0, 3) = rhs.MAT_RC(0, 3);
        MAT_RC(1, 3) = rhs.MAT_RC(1, 3);
        MAT_RC(2, 3) = rhs.MAT_RC(2, 3);
        MAT_RC(3, 3) = rhs.MAT_RC(3, 3);
    }
#endif

    // basic utility functions to create new matrices
    inline static auto identity() -> Mat2 {
        Mat2 m;
        m.make_identity();
        return m;
    }

    inline static auto scale(const Vec2<T>& sv) -> Mat2 {
        Mat2 m;
        m.make_scale(sv);
        return m;
    }

    inline static auto scale(value_t sx, value_t sy) -> Mat2 {
        Mat2 m;
        m.make_scale(sx, sy);
        return m;
    }

#if 0
    inline static auto rotate(const Vec3<T>& from, const Vec3<T>& to) -> Mat2 {
        Mat2 m;
        m.make_rotate(from, to);
        return m;
    }

    inline static auto rotate(value_t angle, value_t x, value_t y, value_t z)
        -> Mat2 {
        Mat2 m;
        m.make_rotate(angle, x, y, z);
        return m;
    }

    inline static auto rotate(value_t angle, const Vec3<T>& axis) -> Mat2 {
        Mat2 m;
        m.make_rotate(angle, axis);
        return m;
    }

    inline static auto rotate(value_t angle1, const Vec3<T>& axis1,
                              value_t angle2, const Vec3<T>& axis2,
                              value_t angle3, const Vec3<T>& axis3) -> Mat2 {
        Mat2 m;
        m.make_rotate(angle1, axis1, angle2, axis2, angle3, axis3);
        return m;
    }

    inline static auto rotate(const Quaternion<T>& quat) -> Mat2 {
        Mat2 m;
        m.make_rotate(quat);
        return m;
    }
#endif

    inline static auto inverse(const Mat2& matrix) -> Mat2 {
        Mat2 m;
        m.invert(matrix);
        return m;
    }

#if 0
    inline static auto ortho_normal(const Mat2& matrix) -> Mat2 {
        Mat2 m;
        m.ortho_normalize(matrix);
        return m;
    }
#endif

    [[nodiscard]] inline auto pre_mult(const Vec2<T>& v) const -> Vec2<T> {
        return {MAT_RC(0, 0) * v.x() + MAT_RC(1, 0) * v.y(),
                MAT_RC(0, 1) * v.x() + MAT_RC(1, 1) * v.y()};
    }

    [[nodiscard]] inline auto post_mult(const Vec3<T>& v) const -> Vec3<T> {
        return {MAT_RC(0, 0) * v.x() + MAT_RC(0, 1) * v.y(),
                MAT_RC(1, 0) * v.x() + MAT_RC(1, 1) * v.y()};
    }

    inline auto operator*(const Vec2<T>& v) const -> Vec2<T> {
        return post_mult(v);
    }

    [[nodiscard]] inline auto get_scale() const -> Vec3<T> {
        Vec2<T> x_vec(MAT_RC(0, 0), MAT_RC(1, 0));
        Vec2<T> y_vec(MAT_RC(0, 1), MAT_RC(1, 1));
        return {x_vec.length(), y_vec.length()};
    }

    // basic Mat2 multiplication, our workhorse methods.
    void mult(const Mat2& lhs, const Mat2& rhs) {
        if (&lhs == this) {
            post_mult(rhs);
            return;
        }
        if (&rhs == this) {
            pre_mult(lhs);
            return;
        }

        // PRECONDITION: We assume neither &lhs nor &rhs == this
        // if it did, use pre_mult or post_mult instead
        MAT_RC(0, 0) = INNER_PRODUCT(lhs, rhs, 0, 0);
        MAT_RC(0, 1) = INNER_PRODUCT(lhs, rhs, 0, 1);
        MAT_RC(1, 0) = INNER_PRODUCT(lhs, rhs, 1, 0);
        MAT_RC(1, 1) = INNER_PRODUCT(lhs, rhs, 1, 1);
    }

    void pre_mult(const Mat2& other) {
        // brute force method requiring a copy
        // Matrix_implementation tmp(other* *this);
        // *this = tmp;

        // more efficient method just use a value_t[4] for temporary storage.
        value_t t[2];
        for (int col = 0; col < 2; ++col) {
            t[0] = INNER_PRODUCT(other, *this, 0, col);
            t[1] = INNER_PRODUCT(other, *this, 1, col);
            MAT_RC(0, col) = t[0];
            MAT_RC(1, col) = t[1];
        }
    }

    void post_mult(const Mat2& other) {
        // brute force method requiring a copy
        // Matrix_implementation tmp(*this * other);
        // *this = tmp;

        // more efficient method just use a value_t[4] for temporary storage.
        value_t t[2];
        for (int row = 0; row < 4; ++row) {
            t[0] = INNER_PRODUCT(*this, other, row, 0);
            t[1] = INNER_PRODUCT(*this, other, row, 1);
            SET_ROW(row, t[0], t[1])
        }
    }

    inline void operator*=(const Mat2& other) {
        if (this == &other) {
            Mat2 temp(other);
            post_mult(temp);
        } else
            post_mult(other);
    }

    inline auto operator*(const Mat2& m) const -> Mat2 {
        Mat2 r;
        r.mult(*this, m);
        return r;
    }

    /** Multiply by scalar. */
    inline auto operator*(value_t rhs) const -> Mat2 {
        return Mat2(MAT_RC(0, 0) * rhs, MAT_RC(0, 1) * rhs, MAT_RC(1, 0) * rhs,
                    MAT_RC(1, 1) * rhs);
    }

    /** Unary multiply by scalar. */
    inline auto operator*=(value_t rhs) -> Mat2& {
        MAT_RC(0, 0) *= rhs;
        MAT_RC(0, 1) *= rhs;
        MAT_RC(1, 0) *= rhs;
        MAT_RC(1, 1) *= rhs;
        return *this;
    }

    /** Divide by scalar. */
    inline auto operator/(value_t rhs) const -> Mat2 {
        return Mat2(MAT_RC(0, 0) / rhs, MAT_RC(0, 1) / rhs, MAT_RC(1, 0) / rhs,
                    MAT_RC(1, 1) / rhs);
    }

    /** Unary divide by scalar. */
    inline auto operator/=(value_t rhs) -> Mat2& {
        MAT_RC(0, 0) /= rhs;
        MAT_RC(0, 1) /= rhs;
        MAT_RC(1, 0) /= rhs;
        MAT_RC(1, 1) /= rhs;
        return *this;
    }

    /** Binary mat add. */
    inline auto operator+(const Mat2& rhs) const -> Mat2 {
        return Mat2(
            MAT_RC(0, 0) + rhs.MAT_RC(0, 0), MAT_RC(0, 1) + rhs.MAT_RC(0, 1),
            MAT_RC(1, 0) + rhs.MAT_RC(1, 0), MAT_RC(1, 1) + rhs.MAT_RC(1, 1));
    }

    /** Unary mat add
     */
    inline auto operator+=(const Mat2& rhs) -> Mat2& {
        MAT_RC(0, 0) += rhs.MAT_RC(0, 0);
        MAT_RC(0, 1) += rhs.MAT_RC(0, 1);
        MAT_RC(1, 0) += rhs.MAT_RC(1, 0);
        MAT_RC(1, 1) += rhs.MAT_RC(1, 1);
        return *this;
    }

    /** Binary mat sub. */
    inline auto operator-(const Mat2& rhs) const -> Mat2 {
        return Mat2(
            MAT_RC(0, 0) - rhs.MAT_RC(0, 0), MAT_RC(0, 1) - rhs.MAT_RC(0, 1),
            MAT_RC(1, 0) - rhs.MAT_RC(1, 0), MAT_RC(1, 1) - rhs.MAT_RC(1, 1));
    }

    /** Unary mat sub
     */
    inline auto operator-=(const Mat2& rhs) -> Mat2& {
        MAT_RC(0, 0) -= rhs.MAT_RC(0, 0);
        MAT_RC(0, 1) -= rhs.MAT_RC(0, 1);
        MAT_RC(1, 0) -= rhs.MAT_RC(1, 0);
        MAT_RC(1, 1) -= rhs.MAT_RC(1, 1);
        return *this;
    }

    /** Get the maximum of the components */
    inline auto maximum() -> value_t {
        const auto* ptr = (const value_t*)_mat;
        value_t maxi = std::numeric_limits<value_t>::lowest();

        for (int i = 0; i < num_elements; ++i) {
            if (std::isnan(*ptr)) {
                return NAN;
            }
            maxi = nv::maximum(maxi, *ptr++);
        }

        return maxi;
    }

    /** Get the minimum of the components */
    inline auto minimum() -> value_t {
        const auto* ptr = (const value_t*)_mat;
        value_t mini = std::numeric_limits<value_t>::max();

        for (int i = 0; i < num_elements; ++i) {
            if (std::isnan(*ptr)) {
                return NAN;
            }
            mini = nv::minimum(mini, *ptr++);
        }

        return mini;
    }

    [[nodiscard]] inline auto range() const -> Range<value_t> {
        Range<value_t> r;
        const auto* ptr = (const value_t*)_mat;
        for (int i = 0; i < num_elements; ++i) {
            if (std::isnan(*ptr)) {
                return {NAN};
            }
            r.extendTo(*ptr++);
        }

        return r;
    }

    /** Get a given column */
    [[nodiscard]] auto col(U32 i) const -> Vec2<value_t> {
        return {MAT_RC(0, i), MAT_RC(1, i)};
    }

    /** Get a given row */
    [[nodiscard]] auto row(U32 i) const -> Vec2<value_t> {
        return {MAT_RC(i, 0), MAT_RC(i, 1)};
    }

    template <class Archive> void serialize(Archive& ar) { ar(_mat); }

  protected:
    value_t _mat[2][2];
};

template <typename T>
inline auto operator*(const Vec2<T>& v, const Mat2<T>& m) -> Vec2<T> {
    return m.pre_mult(v);
}

using Mat2f = Mat2<F32>;
using Mat2d = Mat2<F64>;

inline auto to_mat2f(const Mat2d& mat) -> Mat2f {
    Mat2f res;
    F32* dst = res.ptr();
    const F64* src = mat.ptr();
    for (I32 i = 0; i < Mat2f::num_elements; ++i) {
        (*dst++) = (F32)(*src++);
    }

    return res;
}

inline auto to_mat2d(const Mat2f& mat) -> Mat2d {
    Mat2d res;
    F64* dst = res.ptr();
    const F32* src = mat.ptr();
    for (I32 i = 0; i < Mat2f::num_elements; ++i) {
        (*dst++) = (F64)(*src++);
    }

    return res;
}

} // namespace nv

// Custom formatter for the Matrix4x4 class
template <> struct fmt::formatter<nv::Mat2d> {

    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        // [ctx.begin(), ctx.end()) is a character range that contains a part of
        // the format string starting from the format specifications to be
        // parsed, e.g. in
        //
        //   fmt::format("{:f} - point of interest", point{1, 2});
        //
        // the range will contain "f} - point of interest". The formatter should
        // parse specifiers until '}' or the end of the range. In this example
        // the formatter should parse the 'f' specifier and return an iterator
        // pointing to '}'.

        // Please also note that this character range may be empty, in case of
        // the "{}" format string, so therefore you should check ctx.begin()
        // for equality with ctx.end().

        // Parse the presentation format and store it in the formatter:
        const auto* it = ctx.begin();
        const auto* end = ctx.end();
        // if (it != end && (*it == 'f' || *it == 'e'))
        //     presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Mat2d& mat, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        // Format the matrix as a string and return it
        return fmt::format_to(ctx.out(),
                              "\nMat2d[{:6g}, {:6g},\n"
                              "      {:6g}, {:6g}]",
                              mat(0, 0), mat(0, 1), mat(1, 0), mat(1, 1));
    }
};

template <> struct fmt::formatter<nv::Mat2f> {
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        // [ctx.begin(), ctx.end()) is a character range that contains a part of
        // the format string starting from the format specifications to be
        // parsed, e.g. in
        //
        //   fmt::format("{:f} - point of interest", point{1, 2});
        //
        // the range will contain "f} - point of interest". The formatter should
        // parse specifiers until '}' or the end of the range. In this example
        // the formatter should parse the 'f' specifier and return an iterator
        // pointing to '}'.

        // Please also note that this character range may be empty, in case of
        // the "{}" format string, so therefore you should check ctx.begin()
        // for equality with ctx.end().

        // Parse the presentation format and store it in the formatter:
        const auto* it = ctx.begin();
        const auto* end = ctx.end();
        // if (it != end && (*it == 'f' || *it == 'e'))
        //     presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Mat2f& mat, fmt::format_context& ctx)
        -> decltype(ctx.out()) {
        // Format the matrix as a string and return it
        return fmt::format_to(ctx.out(),
                              "\nMat2f[{:6g}, {:6g},\n"
                              "      {:6g}, {:6g}]",
                              mat(0, 0), mat(0, 1), mat(1, 0), mat(1, 1));
    }
};

#undef INNER_PRODUCT
#undef SET_ROW
#undef MAT_RC

#endif
