#ifndef _NV_MAT4_H_
#define _NV_MAT4_H_

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

template <typename T> constexpr auto default_epsilon() -> T {
    if constexpr (std::is_same_v<T, F64>) {
        return static_cast<T>(1e-12);
    } else {
        return static_cast<T>(1e-6); // fallback
    }
}

// Need to declare this here to use NVCHK below (since we have not included the
// LogManager yet)
template <typename... Args>
void check(bool cond, fmt::format_string<Args...> fmt, Args&&... args);

#define MAT_RC(row, col) _mat[(col)][(row)]

#define SET_ROW(row, v1, v2, v3, v4)                                           \
    MAT_RC((row), 0) = (v1);                                                   \
    MAT_RC((row), 1) = (v2);                                                   \
    MAT_RC((row), 2) = (v3);                                                   \
    MAT_RC((row), 3) = (v4);

#define INNER_PRODUCT(a, b, r, c)                                              \
    ((a).MAT_RC(r, 0) * (b).MAT_RC(0, c)) +                                    \
        ((a).MAT_RC(r, 1) * (b).MAT_RC(1, c)) +                                \
        ((a).MAT_RC(r, 2) * (b).MAT_RC(2, c)) +                                \
        ((a).MAT_RC(r, 3) * (b).MAT_RC(3, c))

template <typename T> class Mat4 {
  public:
    using value_t = T;

    enum { num_elements = 16 };

    Mat4() { make_identity(); }

    Mat4(const Mat4& mat) { set(mat.ptr()); }

    Mat4(Mat4&& mat) noexcept { set(mat.ptr()); }

    template <typename U> explicit Mat4(const Mat4<U>& rhs) {
        T* dst = ptr();
        const U* src = rhs.ptr();
        for (int i = 0; i < num_elements; ++i) {
            (*dst++) = (T)(*src++);
        }
    }

    auto operator=(const Mat4& rhs) -> Mat4& {
        if (&rhs == this)
            return *this;
        set(rhs.ptr());
        return *this;
    }

    auto operator=(Mat4&& rhs) noexcept -> Mat4& {
        if (&rhs == this)
            return *this;
        set(rhs.ptr());
        return *this;
    }

    // inline explicit Mat4( float const * const ptr ) { set(ptr); }
    // inline explicit Mat4( double const * const ptr ) { set(ptr); }
    explicit Mat4(const Quaternion<T>& quat) { make_rotate(quat); }

    Mat4(value_t a00, value_t a01, value_t a02, value_t a03, value_t a10,
         value_t a11, value_t a12, value_t a13, value_t a20, value_t a21,
         value_t a22, value_t a23, value_t a30, value_t a31, value_t a32,
         value_t a33) {
        SET_ROW(0, a00, a01, a02, a03)
        SET_ROW(1, a10, a11, a12, a13)
        SET_ROW(2, a20, a21, a22, a23)
        SET_ROW(3, a30, a31, a32, a33)
    }

    ~Mat4() = default;

    template <typename U>
    static auto from_columns(const Vec4<U>& c1, const Vec4<U>& c2,
                             const Vec4<U>& c3, const Vec4<U>& c4) -> Mat4 {
        return {c1.x(), c2.x(), c3.x(), c4.x(), c1.y(), c2.y(), c3.y(), c4.y(),
                c1.z(), c2.z(), c3.z(), c4.z(), c1.w(), c2.w(), c3.w(), c4.w()};
    }

    template <typename U>
    static auto from_rows(const Vec4<U>& r1, const Vec4<U>& r2,
                          const Vec4<U>& r3, const Vec4<U>& r4) -> Mat4 {
        return {r1.x(), r1.y(), r1.z(), r1.w(), r2.x(), r2.y(), r2.z(), r2.w(),
                r3.x(), r3.y(), r3.z(), r3.w(), r4.x(), r4.y(), r4.z(), r4.w()};
    }

    [[nodiscard]] auto compare(const Mat4& m) const -> int {
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

    auto operator<(const Mat4& m) const -> bool { return compare(m) < 0; }
    auto operator==(const Mat4& m) const -> bool { return compare(m) == 0; }
    auto operator!=(const Mat4& m) const -> bool { return compare(m) != 0; }

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

    // inline void set(const Mat4& rhs, bool transpose = false) {
    inline void set(const Mat4& rhs) { set(rhs.ptr()); }

    template <typename U> inline void set(const Mat4<U>& rhs) {
        T* dst = ptr();
        const U* src = rhs.ptr();
        for (int i = 0; i < num_elements; ++i) {
            (*dst++) = (T)(*src++);
        }
    }

    auto almost_equals(const Mat4& rhs,
                       value_t epsilon = default_epsilon<value_t>()) -> bool {
        const value_t* p0 = ptr();
        const value_t* p1 = rhs.ptr();
        for (int i = 0; i < num_elements; ++i) {
            if (std::abs(p1[i] - p0[i]) > epsilon) {
                return false;
            }
        }

        return true;
    }

    inline void set(value_t const* const ptr) {

        // auto* local_ptr = (value_t*)_mat;
        // for (int i = 0; i < num_elements; ++i)
        //     local_ptr[i] = (value_t)ptr[i];
        memcpy((void*)_mat, (void*)ptr, num_elements * sizeof(value_t));
    }

    void set(value_t a00, value_t a01, value_t a02, value_t a03, value_t a10,
             value_t a11, value_t a12, value_t a13, value_t a20, value_t a21,
             value_t a22, value_t a23, value_t a30, value_t a31, value_t a32,
             value_t a33) {
        SET_ROW(0, a00, a01, a02, a03);
        SET_ROW(1, a10, a11, a12, a13);
        SET_ROW(2, a20, a21, a22, a23);
        SET_ROW(3, a30, a31, a32, a33);
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
            // First, get the current scale to preserve it
            Vec3<T> cur_scale = get_scale();

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

            // Now apply the previous scale
            MAT_RC(0, 0) *= cur_scale[0];
            MAT_RC(1, 0) *= cur_scale[0];
            MAT_RC(2, 0) *= cur_scale[0];
            MAT_RC(0, 1) *= cur_scale[1];
            MAT_RC(1, 1) *= cur_scale[1];
            MAT_RC(2, 1) *= cur_scale[1];
            MAT_RC(0, 2) *= cur_scale[2];
            MAT_RC(1, 2) *= cur_scale[2];
            MAT_RC(2, 2) *= cur_scale[2];
        }
    }

    /** Get the matrix rotation as a normalized Quat. Scale components
     * are ignored. Use decompose() if you need both rotation and scale.
     */
    [[nodiscard]] auto get_rotate() const -> Quaternion<T> {
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
               MAT_RC(0, 3) == 0 && MAT_RC(1, 0) == 0 && MAT_RC(1, 1) == 1 &&
               MAT_RC(1, 2) == 0 && MAT_RC(1, 3) == 0 && MAT_RC(2, 0) == 0 &&
               MAT_RC(2, 1) == 0 && MAT_RC(2, 2) == 1 && MAT_RC(2, 3) == 0 &&
               MAT_RC(3, 0) == 0 && MAT_RC(3, 1) == 0 && MAT_RC(3, 2) == 0 &&
               MAT_RC(3, 3) == 1;
    }

    void make_identity() {
        SET_ROW(0, 1, 0, 0, 0)
        SET_ROW(1, 0, 1, 0, 0)
        SET_ROW(2, 0, 0, 1, 0)
        SET_ROW(3, 0, 0, 0, 1)
    }

    void make_zero() {
        SET_ROW(0, 0, 0, 0, 0)
        SET_ROW(1, 0, 0, 0, 0)
        SET_ROW(2, 0, 0, 0, 0)
        SET_ROW(3, 0, 0, 0, 0)
    }

    void transpose() {
        value_t tmp;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < i; ++j) {
                tmp = _mat[i][j];
                _mat[i][j] = _mat[j][i];
                _mat[j][i] = tmp;
            }
        }
    }

    [[nodiscard]] auto transposed() const -> Mat4 {
        Mat4 res;
        value_t tmp;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                res._mat[i][j] = _mat[j][i];
            }
        }
        return res;
    }

    void make_scale(value_t x, value_t y, value_t z) {
        SET_ROW(0, x, 0, 0, 0)
        SET_ROW(1, 0, y, 0, 0)
        SET_ROW(2, 0, 0, z, 0)
        SET_ROW(3, 0, 0, 0, 1)
    }

    void make_scale(const Vec3<T>& vec) { make_scale(vec[0], vec[1], vec[2]); }

    void make_translate(value_t x, value_t y, value_t z) {
        SET_ROW(0, 1, 0, 0, x)
        SET_ROW(1, 0, 1, 0, y)
        SET_ROW(2, 0, 0, 1, z)
        SET_ROW(3, 0, 0, 0, 1)
    }

    void make_translate(const Vec3<T>& vec) {
        make_translate(vec[0], vec[1], vec[2]);
    }

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

    /** decompose the matrix into translation, rotation, scale and scale
     * orientation.*/
    // void decompose(Vec3<T>& translation, Quaternion<T>& rotation,
    //                Vec3<T>& scale, Quaternion<T>& so) const;

    /** Set to an orthographic projection. */
    void make_ortho(value_t left, value_t right, value_t top, value_t bottom,
                    value_t zNear, value_t zFar) {
        value_t tx = -(right + left) / (right - left);
        // value_t ty = -(top + bottom) / (top - bottom);
        value_t ty = -(top + bottom) / (bottom - top);
        // value_t tz = -(zFar + zNear) / (zFar - zNear);
        value_t tz = -zNear / (zFar - zNear);
        SET_ROW(0, 2.0 / (right - left), 0.0, 0.0, tx)
        // SET_ROW(1, 0.0, 2.0 / (top - bottom), 0.0, ty)
        SET_ROW(1, 0.0, 2.0 / (bottom - top), 0.0, ty)
        SET_ROW(2, 0.0, 0.0, 1.0 / (zFar - zNear), tz)
        SET_ROW(3, 0.0, 0.0, 0.0, 1.0)
    }

    /** Get the orthographic settings of the orthographic projection matrix.
     * Note, if matrix is not an orthographic matrix then invalid values
     * will be returned.
     */
    auto get_ortho(value_t& left, value_t& right, value_t& top, value_t& bottom,
                   value_t& zNear, value_t& zFar) const -> bool {
        if (MAT_RC(3, 0) != 0.0 || MAT_RC(3, 1) != 0.0 || MAT_RC(3, 2) != 0.0 ||
            MAT_RC(3, 3) != 1.0)
            return false;

        zNear = -MAT_RC(2, 3) / MAT_RC(2, 2);
        zFar = 1.0 / MAT_RC(2, 2) + zNear;

        // - right -left = 2.0 * m[0,3] / m[0,0]
        // right - left = 2.0 / m[0,0]
        // -2 * left = 2/m[0,0] * (1+m[0,3])
        // left = -(1.0 + m[0,3]) / m[0,0]
        // 2 * right = 2/m[0,0]* (1 - m[0,3])
        left = -(1.0 + MAT_RC(0, 3)) / MAT_RC(0, 0);
        right = (1.0 - MAT_RC(0, 3)) / MAT_RC(0, 0);

        // -top-bottom = 2* m[1,3] / m[1,1]
        // bottom-top = 2.0 / m[1,1]
        bottom = (1.0 - MAT_RC(1, 3)) / MAT_RC(1, 1);
        top = -(1.0 + MAT_RC(1, 3)) / MAT_RC(1, 1);

        return true;
    }

    /** Set to a 2D orthographic projection.
     * See glOrtho2D for further details.
     */
    inline void make_ortho_2d(value_t left, value_t right, value_t top,
                              value_t bottom) {
        make_ortho(left, right, top, bottom, 0.0, 1.0);
    }

    /** Set to a perspective projection.
     * See glFrustum for further details.
     */
    void make_frustum(value_t left, value_t right, value_t top, value_t bottom,
                      value_t zNear, value_t zFar) {

        // Build the frustum matrix for vulkan (from LVE tutorial videos):
        value_t A = 2.0 * zNear / (right - left);
        value_t B = -(right + left) / (right - left);
        value_t C = 2.0 * zNear / (bottom - top);
        value_t D = -(bottom + top) / (bottom - top);
        value_t E = zFar / (zFar - zNear);
        value_t F = -zFar * zNear / (zFar - zNear);

        SET_ROW(0, A, 0.0, B, 0.0)
        SET_ROW(1, 0.0, C, D, 0.0)
        SET_ROW(2, 0.0, 0.0, E, F)
        SET_ROW(3, 0.0, 0.0, 1.0, 0.0)
    }

    /** Get the frustum settings of a perspective projection matrix.
     * Note, if matrix is not a perspective matrix then invalid values
     * will be returned.
     */
    auto get_frustum(value_t& left, value_t& right, value_t& top,
                     value_t& bottom, value_t& zNear, value_t& zFar) const
        -> bool {
        if (MAT_RC(3, 0) != 0.0 || MAT_RC(3, 1) != 0.0 || MAT_RC(3, 2) != 1.0 ||
            MAT_RC(3, 3) != 0.0)
            return false;

        // note: near and far must be used inside this method instead of zNear
        // and zFar because zNear and zFar are references and they may point to
        // the same variable.
        value_t temp_near = -MAT_RC(2, 3) / MAT_RC(2, 2);
        value_t temp_far = temp_near * MAT_RC(2, 2) / (MAT_RC(2, 2) - 1.0);

        left = -temp_near * (1.0 + MAT_RC(0, 2)) / MAT_RC(0, 0);
        right = temp_near * (1.0 - MAT_RC(0, 2)) / MAT_RC(0, 0);

        top = -temp_near * (1.0 + MAT_RC(1, 2)) / MAT_RC(1, 1);
        bottom = temp_near * (1.0 - MAT_RC(1, 2)) / MAT_RC(1, 1);

        zNear = temp_near;
        zFar = temp_far;

        return true;
    }

    /** Set to a symmetrical perspective projection.
     * See gluPerspective for further details.
     * Aspect ratio is defined as width/height.
     */
    void make_perspective(value_t fovy, value_t aspectRatio, value_t zNear,
                          value_t zFar) {
        // The fovy value is the verticall FOV in radians
        // aspectRatio is width/height ratio
        NVCHK(zNear != 0.0 && (zFar - zNear) != 0.0,
              "Detected invalid projection zNear={},  zFar={}", zNear, zFar);

        //         if (zNear == 0.0 || (zFar - zNear) == 0.0) {
        // #ifdef WIN32
        //             logWARN("Detected invalid projection zNear={},  zFar={}",
        //             zNear,
        //                     zFar);
        // #endif
        //         }

        value_t B = 1.0 / tan(fovy / 2.0);
        value_t A = B / aspectRatio;
        value_t C = zFar / (zFar - zNear);
        value_t D = -zFar * zNear / (zFar - zNear);

        SET_ROW(0, A, 0.0, 0.0, 0.0)
        SET_ROW(1, 0.0, B, 0.0, 0.0)
        SET_ROW(2, 0.0, 0.0, C, D)
        SET_ROW(3, 0.0, 0.0, 1.0, 0.0)
    }

    /** Get the frustum settings of a symmetric perspective projection
     * matrix.
     * Return false if matrix is not a perspective matrix,
     * where parameter values are undefined.
     * Note, if matrix is not a symmetric perspective matrix then the
     * shear will be lost.
     * Asymmetric matrices occur when stereo, power walls, caves and
     * reality center display are used.
     * In these configuration one should use the AsFrustum method instead.
     */
    auto get_perspective(value_t& fovy, value_t& aspectRatio, value_t& zNear,
                         value_t& zFar) const -> bool {
        if (MAT_RC(3, 0) != 0.0 || MAT_RC(3, 1) != 0.0 || MAT_RC(3, 2) != 1.0 ||
            MAT_RC(3, 3) != 0.0)
            return false;

        // tan (fovY/2) = 1.0 / m11
        fovy = 2.0 * atan(1.0 / MAT_RC(1, 1));

        aspectRatio = MAT_RC(1, 1) / MAT_RC(0, 0);

        zNear = -MAT_RC(2, 3) / MAT_RC(2, 2);
        zFar = zNear * MAT_RC(2, 2) / (MAT_RC(2, 2) - 1.0);

        return true;
    }

    /** Set the position and orientation to be a view matrix,
     * using the same convention as gluLook_at.
     */
    void make_look_at(const Vec3<T>& eye, const Vec3<T>& center,
                      const Vec3<T>& up) {
        // forward vector:
        Vec3<T> f(center - eye);
        f.normalize();
        // right vector:
        Vec3<T> r(f ^ up);
        r.normalize();
        // down vector:
        Vec3<T> d(f ^ r);
        d.normalize();

        // Here we use the convention of:
        // X: right axis,
        // Y: down axis (ie. -up)
        // Z: forward axis (from camera to target)

        SET_ROW(0, r[0], d[0], f[0], eye[0])
        SET_ROW(1, r[1], d[1], f[1], eye[1])
        SET_ROW(2, r[2], d[2], f[2], eye[2])
        SET_ROW(3, 0.0, 0.0, 0.0, 1.0)

        // post_mult_translate(-eye);
    }

    /** invert the matrix rhs, automatically select invert_4x3 or invert_4x4. */
    inline auto invert(const Mat4& rhs) -> bool {
        bool is_4x3 = (rhs.MAT_RC(0, 3) == 0.0 && rhs.MAT_RC(1, 3) == 0.0 &&
                       rhs.MAT_RC(2, 3) == 0.0 && rhs.MAT_RC(3, 3) == 1.0);
        return is_4x3 ? invert_4x3(rhs) : invert_4x4(rhs);
    }

    [[nodiscard]] inline auto inverse() const -> Mat4 {
        Mat4 m;
        m.invert(*this);
        return m;
    }

    /** 4x3 matrix invert, not right hand column is assumed to be 0,0,0,1. */
    /******************************************
      Matrix inversion technique:
    Given a matrix mat, we want to invert it.
    mat = [ r00 r01 r02 a
            r10 r11 r12 b
            r20 r21 r22 c
            tx  ty  tz  d ]
    We note that this matrix can be split into three matrices.
    mat = rot * trans * corr, where rot is rotation part, trans is translation
    part, and corr is the correction due to perspective (if any). rot = [ r00
    r01 r02 0 r10 r11 r12 0 r20 r21 r22 0 0   0   0   1 ] trans = [ 1  0  0  0
              0  1  0  0
              0  0  1  0
              tx ty tz 1 ]
    corr = [ 1 0 0 px
             0 1 0 py
             0 0 1 pz
             0 0 0 s ]
    where the elements of corr are obtained from linear combinations of the
    elements of rot, trans, and mat. So the inverse is mat' = (trans * corr)' *
    rot', where rot' must be computed the traditional way, which is easy since
    it is only a 3x3 matrix. This problem is simplified if [px py pz s] = [0 0 0
    1], which will happen if mat was composed only of rotations, scales, and
    translations (which is common).  In this case, we can ignore corr entirely
    which saves on a lot of computations.
    ******************************************/

    auto invert_4x3(const Mat4& mat) -> bool {
        if (&mat == this) {
            Mat4 tm(mat);
            return invert_4x3(tm);
        }

        value_t r00, r01, r02, r10, r11, r12, r20, r21, r22;
        // Copy rotation components directly into registers for speed
        r00 = mat.MAT_RC(0, 0);
        r01 = mat.MAT_RC(0, 1);
        r02 = mat.MAT_RC(0, 2);
        r10 = mat.MAT_RC(1, 0);
        r11 = mat.MAT_RC(1, 1);
        r12 = mat.MAT_RC(1, 2);
        r20 = mat.MAT_RC(2, 0);
        r21 = mat.MAT_RC(2, 1);
        r22 = mat.MAT_RC(2, 2);

        // Partially compute inverse of rot
        MAT_RC(0, 0) = r11 * r22 - r12 * r21;
        MAT_RC(0, 1) = r02 * r21 - r01 * r22;
        MAT_RC(0, 2) = r01 * r12 - r02 * r11;

        // Compute determinant of rot from 3 elements just computed
        value_t one_over_det = 1.0 / (r00 * MAT_RC(0, 0) + r10 * MAT_RC(0, 1) +
                                      r20 * MAT_RC(0, 2));
        r00 *= one_over_det;
        r10 *= one_over_det;
        r20 *= one_over_det; // Saves on later computations

        // Finish computing inverse of rot
        MAT_RC(0, 0) *= one_over_det;
        MAT_RC(0, 1) *= one_over_det;
        MAT_RC(0, 2) *= one_over_det;
        MAT_RC(0, 3) = 0.0;
        MAT_RC(1, 0) =
            r12 * r20 - r10 * r22; // Have already been divided by det
        MAT_RC(1, 1) = r00 * r22 - r02 * r20; // same
        MAT_RC(1, 2) = r02 * r10 - r00 * r12; // same
        MAT_RC(1, 3) = 0.0;
        MAT_RC(2, 0) =
            r10 * r21 - r11 * r20; // Have already been divided by det
        MAT_RC(2, 1) = r01 * r20 - r00 * r21; // same
        MAT_RC(2, 2) = r00 * r11 - r01 * r10; // same
        MAT_RC(2, 3) = 0.0;
        MAT_RC(3, 3) = 1.0;

        // We no longer need the rxx or det variables anymore, so we can reuse
        // them for whatever we want.  But we will still rename them for the
        // sake of clarity.

#define d r22
        d = mat.MAT_RC(3, 3);

        if (square(d - 1.0) > 1.0e-6) // Involves perspective, so we must
        {                             // compute the full inverse

            Mat4 TPinv;
            MAT_RC(3, 0) = MAT_RC(3, 1) = MAT_RC(3, 2) = 0.0;

#define px r00
#define py r01
#define pz r02
#define one_over_s one_over_det
#define a r10
#define b r11
#define c r12

            a = mat.MAT_RC(0, 3);
            b = mat.MAT_RC(1, 3);
            c = mat.MAT_RC(2, 3);
            px = MAT_RC(0, 0) * a + MAT_RC(0, 1) * b + MAT_RC(0, 2) * c;
            py = MAT_RC(1, 0) * a + MAT_RC(1, 1) * b + MAT_RC(1, 2) * c;
            pz = MAT_RC(2, 0) * a + MAT_RC(2, 1) * b + MAT_RC(2, 2) * c;

#undef a
#undef b
#undef c
#define tx r10
#define ty r11
#define tz r12

            tx = mat.MAT_RC(3, 0);
            ty = mat.MAT_RC(3, 1);
            tz = mat.MAT_RC(3, 2);
            one_over_s = 1.0 / (d - (tx * px + ty * py + tz * pz));

            tx *= one_over_s;
            ty *= one_over_s;
            tz *= one_over_s; // Reduces number of calculations later on

            // Compute inverse of trans*corr
            TPinv.MAT_RC(0, 0) = tx * px + 1.0;
            TPinv.MAT_RC(0, 1) = ty * px;
            TPinv.MAT_RC(0, 2) = tz * px;
            TPinv.MAT_RC(0, 3) = -px * one_over_s;
            TPinv.MAT_RC(1, 0) = tx * py;
            TPinv.MAT_RC(1, 1) = ty * py + 1.0;
            TPinv.MAT_RC(1, 2) = tz * py;
            TPinv.MAT_RC(1, 3) = -py * one_over_s;
            TPinv.MAT_RC(2, 0) = tx * pz;
            TPinv.MAT_RC(2, 1) = ty * pz;
            TPinv.MAT_RC(2, 2) = tz * pz + 1.0;
            TPinv.MAT_RC(2, 3) = -pz * one_over_s;
            TPinv.MAT_RC(3, 0) = -tx;
            TPinv.MAT_RC(3, 1) = -ty;
            TPinv.MAT_RC(3, 2) = -tz;
            TPinv.MAT_RC(3, 3) = one_over_s;

            pre_mult(TPinv); // Finish computing full inverse of mat

#undef px
#undef py
#undef pz
#undef one_over_s
#undef d
        } else // Rightmost column is [0; 0; 0; 1] so it can be ignored
        {
            tx = mat.MAT_RC(3, 0);
            ty = mat.MAT_RC(3, 1);
            tz = mat.MAT_RC(3, 2);

            // Compute translation components of mat'
            MAT_RC(3, 0) =
                -(tx * MAT_RC(0, 0) + ty * MAT_RC(1, 0) + tz * MAT_RC(2, 0));
            MAT_RC(3, 1) =
                -(tx * MAT_RC(0, 1) + ty * MAT_RC(1, 1) + tz * MAT_RC(2, 1));
            MAT_RC(3, 2) =
                -(tx * MAT_RC(0, 2) + ty * MAT_RC(1, 2) + tz * MAT_RC(2, 2));

#undef tx
#undef ty
#undef tz
        }

        return true;
    }

#ifndef SGL_SWAP
#define SGL_SWAP(a, b, temp) ((temp) = (a), (a) = (b), (b) = (temp))
#endif

    /** full 4x4 matrix invert. */

    auto invert_4x4(const Mat4& mat) -> bool {
        if (&mat == this) {
            Mat4 tm(mat);
            return invert_4x4(tm);
        }

        unsigned int indxc[4], indxr[4], ipiv[4];
        unsigned int i, j, k, l, ll;
        unsigned int icol = 0;
        unsigned int irow = 0;
        double temp, pivinv, dum, big;

        // copy in place this may be unnecessary
        *this = mat;

        for (j = 0; j < 4; j++)
            ipiv[j] = 0;

        for (i = 0; i < 4; i++) {
            big = 0.0;
            for (j = 0; j < 4; j++)
                if (ipiv[j] != 1)
                    for (k = 0; k < 4; k++) {
                        if (ipiv[k] == 0) {
                            if (absolute(operator()(j, k)) >= big) {
                                big = absolute(operator()(j, k));
                                irow = j;
                                icol = k;
                            }
                        } else if (ipiv[k] > 1)
                            return false;
                    }
            ++(ipiv[icol]);
            if (irow != icol)
                for (l = 0; l < 4; l++)
                    SGL_SWAP(operator()(irow, l), operator()(icol, l), temp);

            indxr[i] = irow;
            indxc[i] = icol;
            if (operator()(icol, icol) == 0)
                return false;

            pivinv = 1.0 / operator()(icol, icol);
            operator()(icol, icol) = 1;
            for (l = 0; l < 4; l++)
                operator()(icol, l) *= pivinv;
            for (ll = 0; ll < 4; ll++)
                if (ll != icol) {
                    dum = operator()(ll, icol);
                    operator()(ll, icol) = 0;
                    for (l = 0; l < 4; l++)
                        operator()(ll, l) -= operator()(icol, l) * dum;
                }
        }
        for (int lx = 4; lx > 0; --lx) {
            if (indxr[lx - 1] != indxc[lx - 1])
                for (k = 0; k < 4; k++)
                    SGL_SWAP(operator()(k, indxr[lx - 1]),
                             operator()(k, indxc[lx - 1]), temp);
        }

        return true;
    }

    /** ortho-normalize the 3x3 rotation & scale matrix */
    void ortho_normalize(const Mat4& rhs) {
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

    // basic utility functions to create new matrices
    inline static auto identity() -> Mat4 {
        Mat4 m;
        m.make_identity();
        return m;
    }

    inline static auto scale(const Vec3<T>& sv) -> Mat4 {
        Mat4 m;
        m.make_scale(sv);
        return m;
    }

    inline static auto scale(value_t sx, value_t sy, value_t sz) -> Mat4 {
        Mat4 m;
        m.make_scale(sx, sy, sz);
        return m;
    }

    inline static auto translate(const Vec3<T>& dv) -> Mat4 {
        Mat4 m;
        m.make_translate(dv);
        return m;
    }

    inline static auto translate(value_t x, value_t y, value_t z) -> Mat4 {
        Mat4 m;
        m.make_translate(x, y, z);
        return m;
    }

    inline static auto rotate(const Vec3<T>& from, const Vec3<T>& to) -> Mat4 {
        Mat4 m;
        m.make_rotate(from, to);
        return m;
    }

    inline static auto rotate(value_t angle, value_t x, value_t y, value_t z)
        -> Mat4 {
        Mat4 m;
        m.make_rotate(angle, x, y, z);
        return m;
    }

    inline static auto rotate(value_t angle, const Vec3<T>& axis) -> Mat4 {
        Mat4 m;
        m.make_rotate(angle, axis);
        return m;
    }

    inline static auto rotate(value_t angle1, const Vec3<T>& axis1,
                              value_t angle2, const Vec3<T>& axis2,
                              value_t angle3, const Vec3<T>& axis3) -> Mat4 {
        Mat4 m;
        m.make_rotate(angle1, axis1, angle2, axis2, angle3, axis3);
        return m;
    }

    inline static auto rotate(const Quaternion<T>& quat) -> Mat4 {
        Mat4 m;
        m.make_rotate(quat);
        return m;
    }

    inline static auto inverse(const Mat4& matrix) -> Mat4 {
        Mat4 m;
        m.invert(matrix);
        return m;
    }

    inline static auto ortho_normal(const Mat4& matrix) -> Mat4 {
        Mat4 m;
        m.ortho_normalize(matrix);
        return m;
    }

    /** Create an orthographic projection matrix.
     * See glOrtho for further details.
     */
    inline static auto ortho(double left, double right, double top,
                             double bottom, double zNear, double zFar) -> Mat4 {
        Mat4 m;
        m.make_ortho(left, right, top, bottom, zNear, zFar);
        return m;
    }

    /** Create a perspective projection.
     * See glFrustum for further details.
     */
    inline static auto frustum(double left, double right, double top,
                               double bottom, double zNear, double zFar)
        -> Mat4 {
        Mat4 m;
        m.make_frustum(left, right, top, bottom, zNear, zFar);
        return m;
    }

    /** Create a symmetrical perspective projection.
     * See gluPerspective for further details.
     * Aspect ratio is defined as width/height.
     */
    inline static auto perspective(double fovy, double aspectRatio,
                                   double zNear, double zFar) -> Mat4 {
        Mat4 m;
        m.make_perspective(fovy, aspectRatio, zNear, zFar);
        return m;
    }

    /** Create the position and orientation as per a camera,
     * using the same convention as gluLook_at.
     */
    inline static auto look_at(const Vec3<T>& eye, const Vec3<T>& center,
                               const Vec3<T>& up) -> Mat4 {
        Mat4 m;
        m.make_look_at(eye, center, up);
        return m;
    }

    [[nodiscard]] inline auto pre_mult(const Vec3<T>& v) const -> Vec3<T> {
        value_t d = 1.0 / (MAT_RC(0, 3) * v.x() + MAT_RC(1, 3) * v.y() +
                           MAT_RC(2, 3) * v.z() + MAT_RC(3, 3));
        return {(MAT_RC(0, 0) * v.x() + MAT_RC(1, 0) * v.y() +
                 MAT_RC(2, 0) * v.z() + MAT_RC(3, 0)) *
                    d,
                (MAT_RC(0, 1) * v.x() + MAT_RC(1, 1) * v.y() +
                 MAT_RC(2, 1) * v.z() + MAT_RC(3, 1)) *
                    d,
                (MAT_RC(0, 2) * v.x() + MAT_RC(1, 2) * v.y() +
                 MAT_RC(2, 2) * v.z() + MAT_RC(3, 2)) *
                    d};
    }

    [[nodiscard]] inline auto post_mult(const Vec3<T>& v) const -> Vec3<T> {
        value_t d = 1.0 / (MAT_RC(3, 0) * v.x() + MAT_RC(3, 1) * v.y() +
                           MAT_RC(3, 2) * v.z() + MAT_RC(3, 3));
        return {(MAT_RC(0, 0) * v.x() + MAT_RC(0, 1) * v.y() +
                 MAT_RC(0, 2) * v.z() + MAT_RC(0, 3)) *
                    d,
                (MAT_RC(1, 0) * v.x() + MAT_RC(1, 1) * v.y() +
                 MAT_RC(1, 2) * v.z() + MAT_RC(1, 3)) *
                    d,
                (MAT_RC(2, 0) * v.x() + MAT_RC(2, 1) * v.y() +
                 MAT_RC(2, 2) * v.z() + MAT_RC(2, 3)) *
                    d};
    }

    [[nodiscard]] inline auto pre_mult_dir(const Vec3<T>& v) const -> Vec3<T> {
        return {
            MAT_RC(0, 0) * v.x() + MAT_RC(1, 0) * v.y() + MAT_RC(2, 0) * v.z(),
            MAT_RC(0, 1) * v.x() + MAT_RC(1, 1) * v.y() + MAT_RC(2, 1) * v.z(),
            MAT_RC(0, 2) * v.x() + MAT_RC(1, 2) * v.y() + MAT_RC(2, 2) * v.z()};
    }

    [[nodiscard]] inline auto post_mult_dir(const Vec3<T>& v) const -> Vec3<T> {
        return {
            MAT_RC(0, 0) * v.x() + MAT_RC(0, 1) * v.y() + MAT_RC(0, 2) * v.z(),
            MAT_RC(1, 0) * v.x() + MAT_RC(1, 1) * v.y() + MAT_RC(1, 2) * v.z(),
            MAT_RC(2, 0) * v.x() + MAT_RC(2, 1) * v.y() + MAT_RC(2, 2) * v.z()};
    }

    [[nodiscard]] inline auto mult_dir(const Vec3<T>& v) const -> Vec3<T> {
        return post_mult_dir(v);
    }

    inline auto operator*(const Vec3<T>& v) const -> Vec3<T> {
        return post_mult(v);
    }

    [[nodiscard]] inline auto pre_mult(const Vec4<T>& v) const -> Vec4<T> {
        return {(MAT_RC(0, 0) * v.x() + MAT_RC(1, 0) * v.y() +
                 MAT_RC(2, 0) * v.z() + MAT_RC(3, 0) * v.w()),
                (MAT_RC(0, 1) * v.x() + MAT_RC(1, 1) * v.y() +
                 MAT_RC(2, 1) * v.z() + MAT_RC(3, 1) * v.w()),
                (MAT_RC(0, 2) * v.x() + MAT_RC(1, 2) * v.y() +
                 MAT_RC(2, 2) * v.z() + MAT_RC(3, 2) * v.w()),
                (MAT_RC(0, 3) * v.x() + MAT_RC(1, 3) * v.y() +
                 MAT_RC(2, 3) * v.z() + MAT_RC(3, 3) * v.w())};
    }

    [[nodiscard]] inline auto post_mult(const Vec4<T>& v) const -> Vec4<T> {
        return {(MAT_RC(0, 0) * v.x() + MAT_RC(0, 1) * v.y() +
                 MAT_RC(0, 2) * v.z() + MAT_RC(0, 3) * v.w()),
                (MAT_RC(1, 0) * v.x() + MAT_RC(1, 1) * v.y() +
                 MAT_RC(1, 2) * v.z() + MAT_RC(1, 3) * v.w()),
                (MAT_RC(2, 0) * v.x() + MAT_RC(2, 1) * v.y() +
                 MAT_RC(2, 2) * v.z() + MAT_RC(2, 3) * v.w()),
                (MAT_RC(3, 0) * v.x() + MAT_RC(3, 1) * v.y() +
                 MAT_RC(3, 2) * v.z() + MAT_RC(3, 3) * v.w())};
    }

    inline auto operator*(const Vec4<T>& v) const -> Vec4<T> {
        return post_mult(v);
    }

    void set_trans(value_t tx, value_t ty, value_t tz) {
        MAT_RC(0, 3) = tx;
        MAT_RC(1, 3) = ty;
        MAT_RC(2, 3) = tz;
    }

    void set_trans(const Vec3<T>& v) { set_trans(v[0], v[1], v[2]); }

    [[nodiscard]] inline auto get_trans() const -> Vec3<T> {
        return {MAT_RC(0, 3), MAT_RC(1, 3), MAT_RC(2, 3)};
    }

    [[nodiscard]] inline auto get_scale() const -> Vec3<T> {
        Vec3<T> x_vec(MAT_RC(0, 0), MAT_RC(1, 0), MAT_RC(2, 0));
        Vec3<T> y_vec(MAT_RC(0, 1), MAT_RC(1, 1), MAT_RC(2, 1));
        Vec3<T> z_vec(MAT_RC(0, 2), MAT_RC(1, 2), MAT_RC(2, 2));
        return {x_vec.length(), y_vec.length(), z_vec.length()};
    }

    void set_scale(value_t sx, value_t sy, value_t sz) {
        // Get current column vectors
        Vec3<T> x_vec(MAT_RC(0, 0), MAT_RC(1, 0), MAT_RC(2, 0));
        Vec3<T> y_vec(MAT_RC(0, 1), MAT_RC(1, 1), MAT_RC(2, 1));
        Vec3<T> z_vec(MAT_RC(0, 2), MAT_RC(1, 2), MAT_RC(2, 2));

        // Normalize and scale
        x_vec = x_vec.normalized() * sx;
        y_vec = y_vec.normalized() * sy;
        z_vec = z_vec.normalized() * sz;

        // Set back to matrix
        MAT_RC(0, 0) = x_vec[0];
        MAT_RC(1, 0) = x_vec[1];
        MAT_RC(2, 0) = x_vec[2];
        MAT_RC(0, 1) = y_vec[0];
        MAT_RC(1, 1) = y_vec[1];
        MAT_RC(2, 1) = y_vec[2];
        MAT_RC(0, 2) = z_vec[0];
        MAT_RC(1, 2) = z_vec[1];
        MAT_RC(2, 2) = z_vec[2];
    }

    void set_scale(const Vec3<T>& v) { set_scale(v[0], v[1], v[2]); }

    /** apply a 3x3 transform of v*M[0..2,0..2]. */
    [[nodiscard]] static auto transform3x3(const Vec3<T>& v, const Mat4& m)
        -> Vec3<T> {
        return {(m.MAT_RC(0, 0) * v.x() + m.MAT_RC(1, 0) * v.y() +
                 m.MAT_RC(2, 0) * v.z()),
                (m.MAT_RC(0, 1) * v.x() + m.MAT_RC(1, 1) * v.y() +
                 m.MAT_RC(2, 1) * v.z()),
                (m.MAT_RC(0, 2) * v.x() + m.MAT_RC(1, 2) * v.y() +
                 m.MAT_RC(2, 2) * v.z())};
    }

    /** apply a 3x3 transform of M[0..2,0..2]*v. */
    [[nodiscard]] static auto transform3x3(const Mat4& m, const Vec3<T>& v)
        -> Vec3<T> {
        return {(m.MAT_RC(0, 0) * v.x() + m.MAT_RC(0, 1) * v.y() +
                 m.MAT_RC(0, 2) * v.z()),
                (m.MAT_RC(1, 0) * v.x() + m.MAT_RC(1, 1) * v.y() +
                 m.MAT_RC(1, 2) * v.z()),
                (m.MAT_RC(2, 0) * v.x() + m.MAT_RC(2, 1) * v.y() +
                 m.MAT_RC(2, 2) * v.z())};
    }

    /** Get to the position and orientation of a modelview matrix,
     * using the same convention as gluLook_at.
     */
    void get_look_at(Vec3<T>& eye, Vec3<T>& center, Vec3<T>& up,
                     value_t lookDistance = 1.0) const {

        // Mat4 inv;
        // inv.invert(*this);

        // Vec3<T> e = inv * Vec3<T>(0.0, 0.0, 0.0);
        eye.set(MAT_RC(0, 3), MAT_RC(1, 3), MAT_RC(2, 3));
        up.set(-MAT_RC(0, 1), -MAT_RC(1, 1), -MAT_RC(2, 1));
        Vec3<T> fwd(MAT_RC(0, 2), MAT_RC(1, 2), MAT_RC(2, 2));

        center = eye + fwd * lookDistance;
        // eye = e;
    }

    // basic Mat4 multiplication, our workhorse methods.
    void mult(const Mat4& lhs, const Mat4& rhs) {
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
        MAT_RC(0, 3) = INNER_PRODUCT(lhs, rhs, 0, 3);
        MAT_RC(1, 0) = INNER_PRODUCT(lhs, rhs, 1, 0);
        MAT_RC(1, 1) = INNER_PRODUCT(lhs, rhs, 1, 1);
        MAT_RC(1, 2) = INNER_PRODUCT(lhs, rhs, 1, 2);
        MAT_RC(1, 3) = INNER_PRODUCT(lhs, rhs, 1, 3);
        MAT_RC(2, 0) = INNER_PRODUCT(lhs, rhs, 2, 0);
        MAT_RC(2, 1) = INNER_PRODUCT(lhs, rhs, 2, 1);
        MAT_RC(2, 2) = INNER_PRODUCT(lhs, rhs, 2, 2);
        MAT_RC(2, 3) = INNER_PRODUCT(lhs, rhs, 2, 3);
        MAT_RC(3, 0) = INNER_PRODUCT(lhs, rhs, 3, 0);
        MAT_RC(3, 1) = INNER_PRODUCT(lhs, rhs, 3, 1);
        MAT_RC(3, 2) = INNER_PRODUCT(lhs, rhs, 3, 2);
        MAT_RC(3, 3) = INNER_PRODUCT(lhs, rhs, 3, 3);
    }

    void pre_mult(const Mat4& other) {
        // brute force method requiring a copy
        // Matrix_implementation tmp(other* *this);
        // *this = tmp;

        // more efficient method just use a value_t[4] for temporary storage.
        value_t t[4];
        for (int col = 0; col < 4; ++col) {
            t[0] = INNER_PRODUCT(other, *this, 0, col);
            t[1] = INNER_PRODUCT(other, *this, 1, col);
            t[2] = INNER_PRODUCT(other, *this, 2, col);
            t[3] = INNER_PRODUCT(other, *this, 3, col);
            MAT_RC(0, col) = t[0];
            MAT_RC(1, col) = t[1];
            MAT_RC(2, col) = t[2];
            MAT_RC(3, col) = t[3];
        }
    }

    void post_mult(const Mat4& other) {
        // brute force method requiring a copy
        // Matrix_implementation tmp(*this * other);
        // *this = tmp;

        // more efficient method just use a value_t[4] for temporary storage.
        value_t t[4];
        for (int row = 0; row < 4; ++row) {
            t[0] = INNER_PRODUCT(*this, other, row, 0);
            t[1] = INNER_PRODUCT(*this, other, row, 1);
            t[2] = INNER_PRODUCT(*this, other, row, 2);
            t[3] = INNER_PRODUCT(*this, other, row, 3);
            SET_ROW(row, t[0], t[1], t[2], t[3])
        }
    }

    /** Optimized version of pre_mult(translate(v)); */
    inline void pre_mult_translate(const Vec3<T>& v) {
        for (unsigned i = 0; i < 3; ++i) {
            double tmp = v[i];
            if (tmp == 0)
                continue;
            MAT_RC(3, 0) += tmp * MAT_RC(i, 0);
            MAT_RC(3, 1) += tmp * MAT_RC(i, 1);
            MAT_RC(3, 2) += tmp * MAT_RC(i, 2);
            MAT_RC(3, 3) += tmp * MAT_RC(i, 3);
        }
    }

    /** Optimized version of post_mult(translate(v)); */
    inline void post_mult_translate(const Vec3<T>& v) {
        for (unsigned i = 0; i < 3; ++i) {
            double tmp = v[i];
            if (tmp == 0)
                continue;
            MAT_RC(0, 3) += tmp * MAT_RC(0, i);
            MAT_RC(1, 3) += tmp * MAT_RC(1, i);
            MAT_RC(2, 3) += tmp * MAT_RC(2, i);
            MAT_RC(3, 3) += tmp * MAT_RC(3, i);
        }
    }

    /** Optimized version of pre_mult(scale(v)); */
    inline void pre_mult_scale(const Vec3<T>& v) {
        MAT_RC(0, 0) *= v[0];
        MAT_RC(0, 1) *= v[0];
        MAT_RC(0, 2) *= v[0];
        MAT_RC(0, 3) *= v[0];
        MAT_RC(1, 0) *= v[1];
        MAT_RC(1, 1) *= v[1];
        MAT_RC(1, 2) *= v[1];
        MAT_RC(1, 3) *= v[1];
        MAT_RC(2, 0) *= v[2];
        MAT_RC(2, 1) *= v[2];
        MAT_RC(2, 2) *= v[2];
        MAT_RC(2, 3) *= v[2];
    }

    /** Optimized version of post_mult(scale(v)); */
    inline void post_mult_scale(const Vec3<T>& v) {
        MAT_RC(0, 0) *= v[0];
        MAT_RC(1, 0) *= v[0];
        MAT_RC(2, 0) *= v[0];
        MAT_RC(3, 0) *= v[0];
        MAT_RC(0, 1) *= v[1];
        MAT_RC(1, 1) *= v[1];
        MAT_RC(2, 1) *= v[1];
        MAT_RC(3, 1) *= v[1];
        MAT_RC(0, 2) *= v[2];
        MAT_RC(1, 2) *= v[2];
        MAT_RC(2, 2) *= v[2];
        MAT_RC(3, 2) *= v[2];
    }

    /** Optimized version of pre_mult(rotate(q)); */
    inline void pre_mult_rotate(const Quaternion<T>& q) {
        if (q.is_zero_rotation())
            return;
        Mat4 r;
        r.set_rotate(q);
        pre_mult(r);
    }

    /** Optimized version of post_mult(rotate(q)); */
    inline void post_mult_rotate(const Quaternion<T>& q) {
        if (q.is_zero_rotation())
            return;
        Mat4 r;
        r.set_rotate(q);
        post_mult(r);
    }

    inline void operator*=(const Mat4& other) {
        if (this == &other) {
            Mat4 temp(other);
            post_mult(temp);
        } else
            post_mult(other);
    }

    inline auto operator*(const Mat4& m) const -> Mat4 {
        Mat4 r;
        r.mult(*this, m);
        return r;
    }

    /** Multiply by scalar. */
    inline auto operator*(value_t rhs) const -> Mat4 {
        return Mat4(MAT_RC(0, 0) * rhs, MAT_RC(0, 1) * rhs, MAT_RC(0, 2) * rhs,
                    MAT_RC(0, 3) * rhs, MAT_RC(1, 0) * rhs, MAT_RC(1, 1) * rhs,
                    MAT_RC(1, 2) * rhs, MAT_RC(1, 3) * rhs, MAT_RC(2, 0) * rhs,
                    MAT_RC(2, 1) * rhs, MAT_RC(2, 2) * rhs, MAT_RC(2, 3) * rhs,
                    MAT_RC(3, 0) * rhs, MAT_RC(3, 1) * rhs, MAT_RC(3, 2) * rhs,
                    MAT_RC(3, 3) * rhs);
    }

    /** Unary multiply by scalar. */
    inline auto operator*=(value_t rhs) -> Mat4& {
        MAT_RC(0, 0) *= rhs;
        MAT_RC(0, 1) *= rhs;
        MAT_RC(0, 2) *= rhs;
        MAT_RC(0, 3) *= rhs;
        MAT_RC(1, 0) *= rhs;
        MAT_RC(1, 1) *= rhs;
        MAT_RC(1, 2) *= rhs;
        MAT_RC(1, 3) *= rhs;
        MAT_RC(2, 0) *= rhs;
        MAT_RC(2, 1) *= rhs;
        MAT_RC(2, 2) *= rhs;
        MAT_RC(2, 3) *= rhs;
        MAT_RC(3, 0) *= rhs;
        MAT_RC(3, 1) *= rhs;
        MAT_RC(3, 2) *= rhs;
        MAT_RC(3, 3) *= rhs;
        return *this;
    }

    /** Divide by scalar. */
    inline auto operator/(value_t rhs) const -> Mat4 {
        return Mat4(MAT_RC(0, 0) / rhs, MAT_RC(0, 1) / rhs, MAT_RC(0, 2) / rhs,
                    MAT_RC(0, 3) / rhs, MAT_RC(1, 0) / rhs, MAT_RC(1, 1) / rhs,
                    MAT_RC(1, 2) / rhs, MAT_RC(1, 3) / rhs, MAT_RC(2, 0) / rhs,
                    MAT_RC(2, 1) / rhs, MAT_RC(2, 2) / rhs, MAT_RC(2, 3) / rhs,
                    MAT_RC(3, 0) / rhs, MAT_RC(3, 1) / rhs, MAT_RC(3, 2) / rhs,
                    MAT_RC(3, 3) / rhs);
    }

    /** Unary divide by scalar. */
    inline auto operator/=(value_t rhs) -> Mat4& {
        MAT_RC(0, 0) /= rhs;
        MAT_RC(0, 1) /= rhs;
        MAT_RC(0, 2) /= rhs;
        MAT_RC(0, 3) /= rhs;
        MAT_RC(1, 0) /= rhs;
        MAT_RC(1, 1) /= rhs;
        MAT_RC(1, 2) /= rhs;
        MAT_RC(1, 3) /= rhs;
        MAT_RC(2, 0) /= rhs;
        MAT_RC(2, 1) /= rhs;
        MAT_RC(2, 2) /= rhs;
        MAT_RC(2, 3) /= rhs;
        MAT_RC(3, 0) /= rhs;
        MAT_RC(3, 1) /= rhs;
        MAT_RC(3, 2) /= rhs;
        MAT_RC(3, 3) /= rhs;
        return *this;
    }

    /** Binary mat add. */
    inline auto operator+(const Mat4& rhs) const -> Mat4 {
        return Mat4(
            MAT_RC(0, 0) + rhs.MAT_RC(0, 0), MAT_RC(0, 1) + rhs.MAT_RC(0, 1),
            MAT_RC(0, 2) + rhs.MAT_RC(0, 2), MAT_RC(0, 3) + rhs.MAT_RC(0, 3),
            MAT_RC(1, 0) + rhs.MAT_RC(1, 0), MAT_RC(1, 1) + rhs.MAT_RC(1, 1),
            MAT_RC(1, 2) + rhs.MAT_RC(1, 2), MAT_RC(1, 3) + rhs.MAT_RC(1, 3),
            MAT_RC(2, 0) + rhs.MAT_RC(2, 0), MAT_RC(2, 1) + rhs.MAT_RC(2, 1),
            MAT_RC(2, 2) + rhs.MAT_RC(2, 2), MAT_RC(2, 3) + rhs.MAT_RC(2, 3),
            MAT_RC(3, 0) + rhs.MAT_RC(3, 0), MAT_RC(3, 1) + rhs.MAT_RC(3, 1),
            MAT_RC(3, 2) + rhs.MAT_RC(3, 2), MAT_RC(3, 3) + rhs.MAT_RC(3, 3));
    }

    /** Unary mat add
     */
    inline auto operator+=(const Mat4& rhs) -> Mat4& {
        MAT_RC(0, 0) += rhs.MAT_RC(0, 0);
        MAT_RC(0, 1) += rhs.MAT_RC(0, 1);
        MAT_RC(0, 2) += rhs.MAT_RC(0, 2);
        MAT_RC(0, 3) += rhs.MAT_RC(0, 3);
        MAT_RC(1, 0) += rhs.MAT_RC(1, 0);
        MAT_RC(1, 1) += rhs.MAT_RC(1, 1);
        MAT_RC(1, 2) += rhs.MAT_RC(1, 2);
        MAT_RC(1, 3) += rhs.MAT_RC(1, 3);
        MAT_RC(2, 0) += rhs.MAT_RC(2, 0);
        MAT_RC(2, 1) += rhs.MAT_RC(2, 1);
        MAT_RC(2, 2) += rhs.MAT_RC(2, 2);
        MAT_RC(2, 3) += rhs.MAT_RC(2, 3);
        MAT_RC(3, 0) += rhs.MAT_RC(3, 0);
        MAT_RC(3, 1) += rhs.MAT_RC(3, 1);
        MAT_RC(3, 2) += rhs.MAT_RC(3, 2);
        MAT_RC(3, 3) += rhs.MAT_RC(3, 3);
        return *this;
    }

    /** Binary mat sub. */
    inline auto operator-(const Mat4& rhs) const -> Mat4 {
        return Mat4(
            MAT_RC(0, 0) - rhs.MAT_RC(0, 0), MAT_RC(0, 1) - rhs.MAT_RC(0, 1),
            MAT_RC(0, 2) - rhs.MAT_RC(0, 2), MAT_RC(0, 3) - rhs.MAT_RC(0, 3),
            MAT_RC(1, 0) - rhs.MAT_RC(1, 0), MAT_RC(1, 1) - rhs.MAT_RC(1, 1),
            MAT_RC(1, 2) - rhs.MAT_RC(1, 2), MAT_RC(1, 3) - rhs.MAT_RC(1, 3),
            MAT_RC(2, 0) - rhs.MAT_RC(2, 0), MAT_RC(2, 1) - rhs.MAT_RC(2, 1),
            MAT_RC(2, 2) - rhs.MAT_RC(2, 2), MAT_RC(2, 3) - rhs.MAT_RC(2, 3),
            MAT_RC(3, 0) - rhs.MAT_RC(3, 0), MAT_RC(3, 1) - rhs.MAT_RC(3, 1),
            MAT_RC(3, 2) - rhs.MAT_RC(3, 2), MAT_RC(3, 3) - rhs.MAT_RC(3, 3));
    }

    /** Unary mat sub
     */
    inline auto operator-=(const Mat4& rhs) -> Mat4& {
        MAT_RC(0, 0) -= rhs.MAT_RC(0, 0);
        MAT_RC(0, 1) -= rhs.MAT_RC(0, 1);
        MAT_RC(0, 2) -= rhs.MAT_RC(0, 2);
        MAT_RC(0, 3) -= rhs.MAT_RC(0, 3);
        MAT_RC(1, 0) -= rhs.MAT_RC(1, 0);
        MAT_RC(1, 1) -= rhs.MAT_RC(1, 1);
        MAT_RC(1, 2) -= rhs.MAT_RC(1, 2);
        MAT_RC(1, 3) -= rhs.MAT_RC(1, 3);
        MAT_RC(2, 0) -= rhs.MAT_RC(2, 0);
        MAT_RC(2, 1) -= rhs.MAT_RC(2, 1);
        MAT_RC(2, 2) -= rhs.MAT_RC(2, 2);
        MAT_RC(2, 3) -= rhs.MAT_RC(2, 3);
        MAT_RC(3, 0) -= rhs.MAT_RC(3, 0);
        MAT_RC(3, 1) -= rhs.MAT_RC(3, 1);
        MAT_RC(3, 2) -= rhs.MAT_RC(3, 2);
        MAT_RC(3, 3) -= rhs.MAT_RC(3, 3);
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

    /** Get the range of data */
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
    [[nodiscard]] auto col(U32 i) const -> Vec4<value_t> {
        return {MAT_RC(0, i), MAT_RC(1, i), MAT_RC(2, i), MAT_RC(3, i)};
    }

    /** Get a given row */
    [[nodiscard]] auto row(U32 i) const -> Vec4<value_t> {
        return {MAT_RC(i, 0), MAT_RC(i, 1), MAT_RC(i, 2), MAT_RC(i, 3)};
    }

    void set_col(U32 i, const Vec4<value_t>& vec) {
        MAT_RC(0, i) = vec.x();
        MAT_RC(1, i) = vec.y();
        MAT_RC(2, i) = vec.z();
        MAT_RC(3, i) = vec.w();
    }

    void set_col(U32 i, const Vec3<value_t>& vec, value_t w = 0) {
        MAT_RC(0, i) = vec.x();
        MAT_RC(1, i) = vec.y();
        MAT_RC(2, i) = vec.z();
        MAT_RC(3, i) = w;
    }

    void set_row(U32 i, const Vec4<value_t>& vec) {
        MAT_RC(i, 0) = vec.x();
        MAT_RC(i, 1) = vec.y();
        MAT_RC(i, 2) = vec.z();
        MAT_RC(i, 3) = vec.w();
    }

    void set_row(U32 i, const Vec3<value_t>& vec, value_t w = 0) {
        MAT_RC(i, 0) = vec.x();
        MAT_RC(i, 1) = vec.y();
        MAT_RC(i, 2) = vec.z();
        MAT_RC(i, 3) = w;
    }

    template <class Archive> void serialize(Archive& ar) { ar(_mat); }

  protected:
    value_t _mat[4][4];
};

template <typename T>
inline auto operator*(const Vec3<T>& v, const Mat4<T>& m) -> Vec3<T> {
    return m.pre_mult(v);
}

template <typename T>
inline auto operator*(const Vec4<T>& v, const Mat4<T>& m) -> Vec4<T> {
    return m.pre_mult(v);
}

using Mat4f = Mat4<F32>;
using Mat4d = Mat4<F64>;

inline auto to_mat4f(const Mat4d& mat) -> Mat4f {
    Mat4f res;
    F32* dst = res.ptr();
    const F64* src = mat.ptr();
    for (I32 i = 0; i < Mat4d::num_elements; ++i) {
        (*dst++) = (F32)(*src++);
    }

    return res;
}

inline auto to_mat4d(const Mat4f& mat) -> Mat4d {
    Mat4d res;
    F64* dst = res.ptr();
    const F32* src = mat.ptr();
    for (I32 i = 0; i < Mat4f::num_elements; ++i) {
        (*dst++) = (F64)(*src++);
    }

    return res;
}

} // namespace nv

// Custom formatter for the Matrix4x4 class
template <> struct fmt::formatter<nv::Mat4d> {

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
        auto it = ctx.begin(), end = ctx.end();
        // if (it != end && (*it == 'f' || *it == 'e'))
        //     presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Mat4d& mat, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        // Format the matrix as a string and return it
        return fmt::format_to(ctx.out(),
                              "\nMat4d[{:6g}, {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}, {:6g}]",
                              mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3),
                              mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3),
                              mat(2, 0), mat(2, 1), mat(2, 2), mat(2, 3),
                              mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3));
    }
};

template <> struct fmt::formatter<nv::Mat4f> {
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
        auto it = ctx.begin(), end = ctx.end();
        // if (it != end && (*it == 'f' || *it == 'e'))
        //     presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Mat4f& mat, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        // Format the matrix as a string and return it
        return fmt::format_to(ctx.out(),
                              "\nMat4f[{:6g}, {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}, {:6g},\n"
                              "      {:6g}, {:6g}, {:6g}, {:6g}]",
                              mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3),
                              mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3),
                              mat(2, 0), mat(2, 1), mat(2, 2), mat(2, 3),
                              mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3));
    }
};

#undef INNER_PRODUCT
#undef SET_ROW
#undef MAT_RC

#endif
