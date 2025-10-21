#ifndef _NV_MAT3_H_
#define _NV_MAT3_H_

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <limits.h>
#include <nvk/math/Quat.h>
#include <nvk/math/Vec3.h>
#include <nvk/math/Vec4.h>

namespace nv {

#define MAT_RC(row, col) _mat[(col)][(row)]

#define SET_ROW(row, v1, v2, v3)                                               \
    MAT_RC((row), 0) = (v1);                                                   \
    MAT_RC((row), 1) = (v2);                                                   \
    MAT_RC((row), 2) = (v3);

#define INNER_PRODUCT(a, b, r, c)                                              \
    ((a).MAT_RC(r, 0) * (b).MAT_RC(0, c)) +                                    \
        ((a).MAT_RC(r, 1) * (b).MAT_RC(1, c)) +                                \
        ((a).MAT_RC(r, 2) * (b).MAT_RC(2, c))

template <typename T> class Mat3 {
  public:
    using value_t = T;

    enum { num_elements = 9 };

    Mat3() { make_identity(); }

    Mat3(const Mat3& mat) { set(mat.ptr()); }

    Mat3(Mat3&& mat) noexcept { set(mat.ptr()); }

    auto operator=(const Mat3& rhs) -> Mat3& {
        if (&rhs == this)
            return *this;
        set(rhs.ptr());
        return *this;
    }

    auto operator=(Mat3&& rhs) noexcept -> Mat3& {
        if (&rhs == this)
            return *this;
        set(rhs.ptr());
        return *this;
    }

    // inline explicit Mat3( float const * const ptr ) { set(ptr); }
    // inline explicit Mat3( double const * const ptr ) { set(ptr); }
    explicit Mat3(const Quaternion<T>& quat) { make_rotate(quat); }

    Mat3(value_t a00, value_t a01, value_t a02, value_t a10, value_t a11,
         value_t a12, value_t a20, value_t a21, value_t a22) {
        SET_ROW(0, a00, a01, a02)
        SET_ROW(1, a10, a11, a12)
        SET_ROW(2, a20, a21, a22)
    }

    ~Mat3() = default;

    [[nodiscard]] auto compare(const Mat3& m) const -> int {
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

    auto operator<(const Mat3& m) const -> bool { return compare(m) < 0; }
    auto operator==(const Mat3& m) const -> bool { return compare(m) == 0; }
    auto operator!=(const Mat3& m) const -> bool { return compare(m) != 0; }

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

    // inline void set(const Mat3& rhs, bool transpose = false) {
    inline void set(const Mat3& rhs) { set(rhs.ptr()); }

    template <typename U> inline void set(const Mat3<U>& rhs) {
        T* dst = ptr();
        const U* src = rhs.ptr();
        for (int i = 0; i < num_elements; ++i) {
            (*dst++) = (T)(*src++);
        }
    }

    inline void set(value_t const* const ptr) {
        memcpy((void*)_mat, (void*)ptr, num_elements * sizeof(value_t));
    }

    void set(value_t a00, value_t a01, value_t a02, value_t a10, value_t a11,
             value_t a12, value_t a20, value_t a21, value_t a22) {
        SET_ROW(0, a00, a01, a02);
        SET_ROW(1, a10, a11, a12);
        SET_ROW(2, a20, a21, a22);
    }

    auto ptr() -> value_t* { return (value_t*)_mat; }
    auto ptr() const -> const value_t* { return (const value_t*)_mat; }

#define QX q._v[0]
#define QY q._v[1]
#define QZ q._v[2]
#define QW q._v[3]

    void set_rotate(const Quaternion<T>& q) {

        double length2 = q.length2();
        if (fabs(length2) <= std::numeric_limits<value_t>::min()) {
            MAT_RC(0, 0) = 0.0;
            MAT_RC(1, 0) = 0.0;
            MAT_RC(2, 0) = 0.0;
            MAT_RC(0, 1) = 0.0;
            MAT_RC(1, 1) = 0.0;
            MAT_RC(2, 1) = 0.0;
            MAT_RC(0, 2) = 0.0;
            MAT_RC(1, 2) = 0.0;
            MAT_RC(2, 2) = 0.0;
        } else {
            double rlength2;
            // normalize quat if required.
            // We can avoid the expensive sqrt in this case since all
            // 'coefficients' below are products of two q components. That is a
            // square of a square root, so it is possible to avoid that
            if (length2 != 1.0) {
                rlength2 = 2.0 / length2;
            } else {
                rlength2 = 2.0;
            }

            // Source: Gamasutra, Rotating Objects Using Quaternions
            //
            // http://www.gamasutra.com/features/19980703/quaternions_01.htm

            double wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

            // calculate coefficients
            x2 = rlength2 * QX;
            y2 = rlength2 * QY;
            z2 = rlength2 * QZ;

            xx = QX * x2;
            xy = QX * y2;
            xz = QX * z2;

            yy = QY * y2;
            yz = QY * z2;
            zz = QZ * z2;

            wx = QW * x2;
            wy = QW * y2;
            wz = QW * z2;

            // Note.  Gamasutra gets the matrix assignments inverted, resulting
            // in left-handed rotations, which is contrary to OpenGL and OSG's
            // methodology.  The matrix assignment has been altered in the next
            // few lines of code to do the right thing.
            // Don Burns - Oct 13, 2001
            MAT_RC(0, 0) = 1.0 - (yy + zz);
            MAT_RC(0, 1) = xy - wz;
            MAT_RC(0, 2) = xz + wy;

            MAT_RC(1, 0) = xy + wz;
            MAT_RC(1, 1) = 1.0 - (xx + zz);
            MAT_RC(1, 2) = yz - wx;

            MAT_RC(2, 0) = xz - wy;
            MAT_RC(2, 1) = yz + wx;
            MAT_RC(2, 2) = 1.0 - (xx + yy);
        }
    }

    /** Get the matrix rotation as a Quat. Note that this function
     * assumes a non-scaled matrix and will return incorrect results
     * for scaled matrixces. Consider decompose() instead.
     */
    auto get_rotate() const -> Quaternion<T> {
        Quaternion<T> q;

        value_t s;
        value_t tq[4];
        int i, j;

        // Use tq to store the largest trace
        tq[0] = 1 + MAT_RC(0, 0) + MAT_RC(1, 1) + MAT_RC(2, 2);
        tq[1] = 1 + MAT_RC(0, 0) - MAT_RC(1, 1) - MAT_RC(2, 2);
        tq[2] = 1 - MAT_RC(0, 0) + MAT_RC(1, 1) - MAT_RC(2, 2);
        tq[3] = 1 - MAT_RC(0, 0) - MAT_RC(1, 1) + MAT_RC(2, 2);

        // Find the maximum (could also use stacked if's later)
        j = 0;
        for (i = 1; i < 4; i++)
            j = (tq[i] > tq[j]) ? i : j;

        // check the diagonal
        if (j == 0) {
            /* perform instant calculation */
            QW = tq[0];
            QX = MAT_RC(2, 1) - MAT_RC(1, 2);
            QY = MAT_RC(0, 2) - MAT_RC(2, 0);
            QZ = MAT_RC(1, 0) - MAT_RC(0, 1);
        } else if (j == 1) {
            QW = MAT_RC(2, 1) - MAT_RC(1, 2);
            QX = tq[1];
            QY = MAT_RC(1, 0) + MAT_RC(0, 1);
            QZ = MAT_RC(0, 2) + MAT_RC(2, 0);
        } else if (j == 2) {
            QW = MAT_RC(0, 2) - MAT_RC(2, 0);
            QX = MAT_RC(1, 0) + MAT_RC(0, 1);
            QY = tq[2];
            QZ = MAT_RC(2, 1) + MAT_RC(1, 2);
        } else /* if (j==3) */
        {
            QW = MAT_RC(1, 0) - MAT_RC(0, 1);
            QX = MAT_RC(0, 2) + MAT_RC(2, 0);
            QY = MAT_RC(2, 1) + MAT_RC(1, 2);
            QZ = tq[3];
        }

        s = sqrt(0.25 / tq[j]);
        QW *= s;
        QX *= s;
        QY *= s;
        QZ *= s;

        return q;
    }

#undef QX
#undef QY
#undef QZ
#undef QW

    [[nodiscard]] auto is_identity() const -> bool {
        return MAT_RC(0, 0) == 1 && MAT_RC(0, 1) == 0 && MAT_RC(0, 2) == 0 &&
               MAT_RC(1, 0) == 0 && MAT_RC(1, 1) == 1 && MAT_RC(1, 2) == 0 &&
               MAT_RC(2, 0) == 0 && MAT_RC(2, 1) == 0 && MAT_RC(2, 2) == 1;
    }

    void make_identity() {
        SET_ROW(0, 1, 0, 0)
        SET_ROW(1, 0, 1, 0)
        SET_ROW(2, 0, 0, 1)
    }

    void transpose() {
        value_t tmp;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < i; ++j) {
                tmp = _mat[i][j];
                _mat[i][j] = _mat[j][i];
                _mat[j][i] = tmp;
            }
        }
    }

    [[nodiscard]] auto transposed() const -> Mat3 {
        Mat3 res;
        value_t tmp;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                res._mat[i][j] = _mat[j][i];
            }
        }
        return res;
    }

    void make_scale(value_t x, value_t y, value_t z) {
        SET_ROW(0, x, 0, 0)
        SET_ROW(1, 0, y, 0)
        SET_ROW(2, 0, 0, z)
    }

    void make_scale(const Vec3<T>& vec) { make_scale(vec[0], vec[1], vec[2]); }

    void make_rotate(const Vec3<T>& from, const Vec3<T>& to) {
        Quaternion<T> quat;
        quat.make_rotate(from, to);
        set_rotate(quat);
    }

    void make_rotate(value_t angle, const Vec3<T>& axis) {
        Quaternion<T> quat;
        quat.make_rotate(angle, axis);
        set_rotate(quat);
    }

    void make_rotate(value_t angle, value_t x, value_t y, value_t z) {
        Quaternion<T> quat;
        quat.make_rotate(angle, x, y, z);
        set_rotate(quat);
    }

    void make_rotate(const Quaternion<T>& quat) { set_rotate(quat); }

    void make_rotate(value_t angle1, const Vec3<T>& axis1, value_t angle2,
                     const Vec3<T>& axis2, value_t angle3,
                     const Vec3<T>& axis3) {
        Quaternion<T> quat;
        quat.make_rotate(angle1, axis1, angle2, axis2, angle3, axis3);
        set_rotate(quat);
    }

    /** Compute determinant */
    [[nodiscard]] auto determinant() const -> value_t {
        return MAT_RC(0, 0) *
                   (MAT_RC(1, 1) * MAT_RC(2, 2) - MAT_RC(1, 2) * MAT_RC(2, 1)) -
               MAT_RC(0, 1) *
                   (MAT_RC(1, 0) * MAT_RC(2, 2) - MAT_RC(1, 2) * MAT_RC(2, 0)) +
               MAT_RC(0, 2) *
                   (MAT_RC(1, 0) * MAT_RC(2, 1) - MAT_RC(1, 1) * MAT_RC(2, 0));
    }

    /** invert the matrix rhs */
    inline auto invert(const Mat3& rhs, bool throwOnFail = true) -> bool {
        value_t det = rhs.determinant();

        if (std::abs(det) < 1e-6) {
            // Matrix is singular, and thus not invertible
            NVCHK(!throwOnFail, "Mat3 is not invertible!");
            return false;
        }

        value_t invDet = 1.0 / det;

        MAT_RC(0, 0) = (rhs.MAT_RC(1, 1) * rhs.MAT_RC(2, 2) -
                        rhs.MAT_RC(1, 2) * rhs.MAT_RC(2, 1)) *
                       invDet;
        MAT_RC(0, 1) = (rhs.MAT_RC(0, 2) * rhs.MAT_RC(2, 1) -
                        rhs.MAT_RC(0, 1) * rhs.MAT_RC(2, 2)) *
                       invDet;
        MAT_RC(0, 2) = (rhs.MAT_RC(0, 1) * rhs.MAT_RC(1, 2) -
                        rhs.MAT_RC(0, 2) * rhs.MAT_RC(1, 1)) *
                       invDet;

        MAT_RC(1, 0) = (rhs.MAT_RC(1, 2) * rhs.MAT_RC(2, 0) -
                        rhs.MAT_RC(1, 0) * rhs.MAT_RC(2, 2)) *
                       invDet;
        MAT_RC(1, 1) = (rhs.MAT_RC(0, 0) * rhs.MAT_RC(2, 2) -
                        rhs.MAT_RC(0, 2) * rhs.MAT_RC(2, 0)) *
                       invDet;
        MAT_RC(1, 2) = (rhs.MAT_RC(0, 2) * rhs.MAT_RC(1, 0) -
                        rhs.MAT_RC(0, 0) * rhs.MAT_RC(1, 2)) *
                       invDet;

        MAT_RC(2, 0) = (rhs.MAT_RC(1, 0) * rhs.MAT_RC(2, 1) -
                        rhs.MAT_RC(1, 1) * rhs.MAT_RC(2, 0)) *
                       invDet;
        MAT_RC(2, 1) = (rhs.MAT_RC(0, 1) * rhs.MAT_RC(2, 0) -
                        rhs.MAT_RC(0, 0) * rhs.MAT_RC(2, 1)) *
                       invDet;
        MAT_RC(2, 2) = (rhs.MAT_RC(0, 0) * rhs.MAT_RC(1, 1) -
                        rhs.MAT_RC(0, 1) * rhs.MAT_RC(1, 0)) *
                       invDet;

        return true;
    }

    [[nodiscard]] inline auto inverse(bool throwOnFail = true) const -> Mat3 {
        Mat3 m;
        m.invert(*this, throwOnFail);
        return m;
    }

    /** ortho-normalize the 3x3 rotation & scale matrix */
    void ortho_normalize(const Mat3& rhs) {
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
    }

    // basic utility functions to create new matrices
    inline static auto identity() -> Mat3 {
        Mat3 m;
        m.make_identity();
        return m;
    }

    inline static auto scale(const Vec3<T>& sv) -> Mat3 {
        Mat3 m;
        m.make_scale(sv);
        return m;
    }

    inline static auto scale(value_t sx, value_t sy, value_t sz) -> Mat3 {
        Mat3 m;
        m.make_scale(sx, sy, sz);
        return m;
    }

    inline static auto rotate(const Vec3<T>& from, const Vec3<T>& to) -> Mat3 {
        Mat3 m;
        m.make_rotate(from, to);
        return m;
    }

    inline static auto rotate(value_t angle, value_t x, value_t y, value_t z)
        -> Mat3 {
        Mat3 m;
        m.make_rotate(angle, x, y, z);
        return m;
    }

    inline static auto rotate(value_t angle, const Vec3<T>& axis) -> Mat3 {
        Mat3 m;
        m.make_rotate(angle, axis);
        return m;
    }

    inline static auto rotate(value_t angle1, const Vec3<T>& axis1,
                              value_t angle2, const Vec3<T>& axis2,
                              value_t angle3, const Vec3<T>& axis3) -> Mat3 {
        Mat3 m;
        m.make_rotate(angle1, axis1, angle2, axis2, angle3, axis3);
        return m;
    }

    inline static auto rotate(const Quaternion<T>& quat) -> Mat3 {
        Mat3 m;
        m.make_rotate(quat);
        return m;
    }

    inline static auto inverse(const Mat3& matrix) -> Mat3 {
        Mat3 m;
        m.invert(matrix);
        return m;
    }

    inline static auto ortho_normal(const Mat3& matrix) -> Mat3 {
        Mat3 m;
        m.ortho_normalize(matrix);
        return m;
    }

    [[nodiscard]] inline auto pre_mult(const Vec3<T>& v) const -> Vec3<T> {
        return {
            MAT_RC(0, 0) * v.x() + MAT_RC(1, 0) * v.y() + MAT_RC(2, 0) * v.z(),
            MAT_RC(0, 1) * v.x() + MAT_RC(1, 1) * v.y() + MAT_RC(2, 1) * v.z(),
            MAT_RC(0, 2) * v.x() + MAT_RC(1, 2) * v.y() + MAT_RC(2, 2) * v.z()};
    }

    [[nodiscard]] inline auto post_mult(const Vec3<T>& v) const -> Vec3<T> {
        return {
            MAT_RC(0, 0) * v.x() + MAT_RC(0, 1) * v.y() + MAT_RC(0, 2) * v.z(),
            MAT_RC(1, 0) * v.x() + MAT_RC(1, 1) * v.y() + MAT_RC(1, 2) * v.z(),
            MAT_RC(2, 0) * v.x() + MAT_RC(2, 1) * v.y() + MAT_RC(2, 2) * v.z()};
    }

    inline auto operator*(const Vec3<T>& v) const -> Vec3<T> {
        return post_mult(v);
    }

    inline auto operator*(const Vec4<T>& v) const -> Vec4<T> {
        return post_mult(v);
    }

    [[nodiscard]] inline auto get_scale() const -> Vec3<T> {
        Vec3<T> x_vec(MAT_RC(0, 0), MAT_RC(1, 0), MAT_RC(2, 0));
        Vec3<T> y_vec(MAT_RC(0, 1), MAT_RC(1, 1), MAT_RC(2, 1));
        Vec3<T> z_vec(MAT_RC(0, 2), MAT_RC(1, 2), MAT_RC(2, 2));
        return {x_vec.length(), y_vec.length(), z_vec.length()};
    }

    // basic Mat3 multiplication, our workhorse methods.
    void mult(const Mat3& lhs, const Mat3& rhs) {
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
        MAT_RC(0, 2) = INNER_PRODUCT(lhs, rhs, 0, 2);
        MAT_RC(1, 0) = INNER_PRODUCT(lhs, rhs, 1, 0);
        MAT_RC(1, 1) = INNER_PRODUCT(lhs, rhs, 1, 1);
        MAT_RC(1, 2) = INNER_PRODUCT(lhs, rhs, 1, 2);
        MAT_RC(2, 0) = INNER_PRODUCT(lhs, rhs, 2, 0);
        MAT_RC(2, 1) = INNER_PRODUCT(lhs, rhs, 2, 1);
        MAT_RC(2, 2) = INNER_PRODUCT(lhs, rhs, 2, 2);
    }

    void pre_mult(const Mat3& other) {
        // brute force method requiring a copy
        // Matrix_implementation tmp(other* *this);
        // *this = tmp;

        // more efficient method just use a value_t[4] for temporary storage.
        value_t t[3];
        for (int col = 0; col < 3; ++col) {
            t[0] = INNER_PRODUCT(other, *this, 0, col);
            t[1] = INNER_PRODUCT(other, *this, 1, col);
            t[2] = INNER_PRODUCT(other, *this, 2, col);
            MAT_RC(0, col) = t[0];
            MAT_RC(1, col) = t[1];
            MAT_RC(2, col) = t[2];
        }
    }

    void post_mult(const Mat3& other) {
        // brute force method requiring a copy
        // Matrix_implementation tmp(*this * other);
        // *this = tmp;

        // more efficient method just use a value_t[4] for temporary storage.
        value_t t[3];
        for (int row = 0; row < 4; ++row) {
            t[0] = INNER_PRODUCT(*this, other, row, 0);
            t[1] = INNER_PRODUCT(*this, other, row, 1);
            t[2] = INNER_PRODUCT(*this, other, row, 2);
            SET_ROW(row, t[0], t[1], t[2])
        }
    }

    /** Optimized version of pre_mult(scale(v)); */
    inline void pre_mult_scale(const Vec3<T>& v) {
        MAT_RC(0, 0) *= v[0];
        MAT_RC(0, 1) *= v[0];
        MAT_RC(0, 2) *= v[0];
        MAT_RC(1, 0) *= v[1];
        MAT_RC(1, 1) *= v[1];
        MAT_RC(1, 2) *= v[1];
        MAT_RC(2, 0) *= v[2];
        MAT_RC(2, 1) *= v[2];
        MAT_RC(2, 2) *= v[2];
    }

    /** Optimized version of post_mult(scale(v)); */
    inline void post_mult_scale(const Vec3<T>& v) {
        MAT_RC(0, 0) *= v[0];
        MAT_RC(1, 0) *= v[0];
        MAT_RC(2, 0) *= v[0];
        MAT_RC(0, 1) *= v[1];
        MAT_RC(1, 1) *= v[1];
        MAT_RC(2, 1) *= v[1];
        MAT_RC(0, 2) *= v[2];
        MAT_RC(1, 2) *= v[2];
        MAT_RC(2, 2) *= v[2];
    }

    /** Optimized version of pre_mult(rotate(q)); */
    inline void pre_mult_rotate(const Quaternion<T>& q) {
        if (q.is_zero_rotation())
            return;
        Mat3 r;
        r.set_rotate(q);
        pre_mult(r);
    }

    /** Optimized version of post_mult(rotate(q)); */
    inline void post_mult_rotate(const Quaternion<T>& q) {
        if (q.is_zero_rotation())
            return;
        Mat3 r;
        r.set_rotate(q);
        post_mult(r);
    }

    inline void operator*=(const Mat3& other) {
        if (this == &other) {
            Mat3 temp(other);
            post_mult(temp);
        } else
            post_mult(other);
    }

    inline auto operator*(const Mat3& m) const -> Mat3 {
        Mat3 r;
        r.mult(*this, m);
        return r;
    }

    /** Multiply by scalar. */
    inline auto operator*(value_t rhs) const -> Mat3 {
        return Mat3(MAT_RC(0, 0) * rhs, MAT_RC(0, 1) * rhs, MAT_RC(0, 2) * rhs,
                    MAT_RC(1, 0) * rhs, MAT_RC(1, 1) * rhs, MAT_RC(1, 2) * rhs,
                    MAT_RC(2, 0) * rhs, MAT_RC(2, 1) * rhs, MAT_RC(2, 2) * rhs);
    }

    /** Unary multiply by scalar. */
    inline auto operator*=(value_t rhs) -> Mat3& {
        MAT_RC(0, 0) *= rhs;
        MAT_RC(0, 1) *= rhs;
        MAT_RC(0, 2) *= rhs;
        MAT_RC(1, 0) *= rhs;
        MAT_RC(1, 1) *= rhs;
        MAT_RC(1, 2) *= rhs;
        MAT_RC(2, 0) *= rhs;
        MAT_RC(2, 1) *= rhs;
        MAT_RC(2, 2) *= rhs;
        return *this;
    }

    /** Divide by scalar. */
    inline auto operator/(value_t rhs) const -> Mat3 {
        return Mat3(MAT_RC(0, 0) / rhs, MAT_RC(0, 1) / rhs, MAT_RC(0, 2) / rhs,
                    MAT_RC(1, 0) / rhs, MAT_RC(1, 1) / rhs, MAT_RC(1, 2) / rhs,
                    MAT_RC(2, 0) / rhs, MAT_RC(2, 1) / rhs, MAT_RC(2, 2) / rhs);
    }

    /** Unary divide by scalar. */
    inline auto operator/=(value_t rhs) -> Mat3& {
        MAT_RC(0, 0) /= rhs;
        MAT_RC(0, 1) /= rhs;
        MAT_RC(0, 2) /= rhs;
        MAT_RC(1, 0) /= rhs;
        MAT_RC(1, 1) /= rhs;
        MAT_RC(1, 2) /= rhs;
        MAT_RC(2, 0) /= rhs;
        MAT_RC(2, 1) /= rhs;
        MAT_RC(2, 2) /= rhs;
        return *this;
    }

    /** Binary mat add. */
    inline auto operator+(const Mat3& rhs) const -> Mat3 {
        return Mat3(
            MAT_RC(0, 0) + rhs.MAT_RC(0, 0), MAT_RC(0, 1) + rhs.MAT_RC(0, 1),
            MAT_RC(0, 2) + rhs.MAT_RC(0, 2), MAT_RC(1, 0) + rhs.MAT_RC(1, 0),
            MAT_RC(1, 1) + rhs.MAT_RC(1, 1), MAT_RC(1, 2) + rhs.MAT_RC(1, 2),
            MAT_RC(2, 0) + rhs.MAT_RC(2, 0), MAT_RC(2, 1) + rhs.MAT_RC(2, 1),
            MAT_RC(2, 2) + rhs.MAT_RC(2, 2));
    }

    /** Unary mat add
     */
    inline auto operator+=(const Mat3& rhs) -> Mat3& {
        MAT_RC(0, 0) += rhs.MAT_RC(0, 0);
        MAT_RC(0, 1) += rhs.MAT_RC(0, 1);
        MAT_RC(0, 2) += rhs.MAT_RC(0, 2);
        MAT_RC(1, 0) += rhs.MAT_RC(1, 0);
        MAT_RC(1, 1) += rhs.MAT_RC(1, 1);
        MAT_RC(1, 2) += rhs.MAT_RC(1, 2);
        MAT_RC(2, 0) += rhs.MAT_RC(2, 0);
        MAT_RC(2, 1) += rhs.MAT_RC(2, 1);
        MAT_RC(2, 2) += rhs.MAT_RC(2, 2);
        return *this;
    }

    /** Binary mat sub. */
    inline auto operator-(const Mat3& rhs) const -> Mat3 {
        return Mat3(
            MAT_RC(0, 0) - rhs.MAT_RC(0, 0), MAT_RC(0, 1) - rhs.MAT_RC(0, 1),
            MAT_RC(0, 2) - rhs.MAT_RC(0, 2), MAT_RC(1, 0) - rhs.MAT_RC(1, 0),
            MAT_RC(1, 1) - rhs.MAT_RC(1, 1), MAT_RC(1, 2) - rhs.MAT_RC(1, 2),
            MAT_RC(2, 0) - rhs.MAT_RC(2, 0), MAT_RC(2, 1) - rhs.MAT_RC(2, 1),
            MAT_RC(2, 2) - rhs.MAT_RC(2, 2));
    }

    /** Unary mat sub
     */
    inline auto operator-=(const Mat3& rhs) -> Mat3& {
        MAT_RC(0, 0) -= rhs.MAT_RC(0, 0);
        MAT_RC(0, 1) -= rhs.MAT_RC(0, 1);
        MAT_RC(0, 2) -= rhs.MAT_RC(0, 2);
        MAT_RC(1, 0) -= rhs.MAT_RC(1, 0);
        MAT_RC(1, 1) -= rhs.MAT_RC(1, 1);
        MAT_RC(1, 2) -= rhs.MAT_RC(1, 2);
        MAT_RC(2, 0) -= rhs.MAT_RC(2, 0);
        MAT_RC(2, 1) -= rhs.MAT_RC(2, 1);
        MAT_RC(2, 2) -= rhs.MAT_RC(2, 2);
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
    [[nodiscard]] auto col(U32 i) const -> Vec3<value_t> {
        return {MAT_RC(0, i), MAT_RC(1, i), MAT_RC(2, i)};
    }

    /** Get a given row */
    [[nodiscard]] auto row(U32 i) const -> Vec3<value_t> {
        return {MAT_RC(i, 0), MAT_RC(i, 1), MAT_RC(i, 2)};
    }

    template <class Archive> void serialize(Archive& ar) { ar(_mat); }

  protected:
    value_t _mat[3][3];
};

template <typename T>
inline auto operator*(const Vec3<T>& v, const Mat3<T>& m) -> Vec3<T> {
    return m.pre_mult(v);
}

using Mat3f = Mat3<F32>;
using Mat3d = Mat3<F64>;

inline auto to_mat3f(const Mat3d& mat) -> Mat3f {
    Mat3f res;
    F32* dst = res.ptr();
    const F64* src = mat.ptr();
    for (I32 i = 0; i < Mat3d::num_elements; ++i) {
        (*dst++) = (F32)(*src++);
    }

    return res;
}

inline auto to_mat3d(const Mat3f& mat) -> Mat3d {
    Mat3d res;
    F64* dst = res.ptr();
    const F32* src = mat.ptr();
    for (I32 i = 0; i < Mat3f::num_elements; ++i) {
        (*dst++) = (F64)(*src++);
    }

    return res;
}

} // namespace nv

// Custom formatter for the Matrix4x4 class
template <> struct fmt::formatter<nv::Mat3d> {

    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {

        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Mat3d& mat, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        // Format the matrix as a string and return it
        return fmt::format_to(ctx.out(),
                              "\nMat3d[{:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}]",
                              mat(0, 0), mat(0, 1), mat(0, 2), mat(1, 0),
                              mat(1, 1), mat(1, 2), mat(2, 0), mat(2, 1),
                              mat(2, 2));
    }
};

template <> struct fmt::formatter<nv::Mat3f> {
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Mat3f& mat, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        // Format the matrix as a string and return it
        return fmt::format_to(ctx.out(),
                              "\nMat3f[{:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}]",
                              mat(0, 0), mat(0, 1), mat(0, 2), mat(1, 0),
                              mat(1, 1), mat(1, 2), mat(2, 0), mat(2, 1),
                              mat(2, 2));
    }
};

#undef INNER_PRODUCT
#undef SET_ROW
#undef MAT_RC

#endif
