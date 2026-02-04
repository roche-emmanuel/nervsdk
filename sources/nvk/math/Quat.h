#ifndef _NV_QUATERNION_H_
#define _NV_QUATERNION_H_

/// Good introductions to Quaternions at:
/// http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm
/// http://mathworld.wolfram.com/Quaternion.html

#include <nvk/math/Vec3.h>
#include <nvk/math/Vec4.h>

namespace nv {

/** A quaternion class. It can be used to represent an orientation in 3D
 * space.*/
template <typename T> class Quaternion {

  public:
    using value_t = T;

    std::array<value_t, 4> _v{}; // a four-vector

    inline Quaternion() {
        _v[0] = 0.0;
        _v[1] = 0.0;
        _v[2] = 0.0;
        _v[3] = 1.0;
    }

    inline Quaternion(const Quaternion& rhs) {
        _v[0] = rhs._v[0];
        _v[1] = rhs._v[1];
        _v[2] = rhs._v[2];
        _v[3] = rhs._v[3];
    }

    inline Quaternion(Quaternion&& rhs) noexcept {
        _v[0] = rhs._v[0];
        _v[1] = rhs._v[1];
        _v[2] = rhs._v[2];
        _v[3] = rhs._v[3];
    }

    inline Quaternion(value_t x, value_t y, value_t z, value_t w) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
        _v[3] = w;
    }

    inline explicit Quaternion(const Vec4<T>& v) {
        _v[0] = v.x();
        _v[1] = v.y();
        _v[2] = v.z();
        _v[3] = v.w();
    }

    inline Quaternion(value_t angle, const Vec3<T>& axis) {
        make_rotate(angle, axis);
    }

    inline Quaternion(value_t angle1, const Vec3<T>& axis1, value_t angle2,
                      const Vec3<T>& axis2, value_t angle3,
                      const Vec3<T>& axis3) {
        make_rotate(angle1, axis1, angle2, axis2, angle3, axis3);
    }

    Quaternion(const Vec3<T>& vec1, const Vec3<T>& vec2) {
        make_rotate(vec1, vec2);
    }

    ~Quaternion() = default;

    inline auto operator=(const Quaternion& v) -> Quaternion& {
        _v[0] = v._v[0];
        _v[1] = v._v[1];
        _v[2] = v._v[2];
        _v[3] = v._v[3];
        return *this;
    }

    inline auto operator=(Quaternion&& v) noexcept -> Quaternion& {
        _v[0] = v._v[0];
        _v[1] = v._v[1];
        _v[2] = v._v[2];
        _v[3] = v._v[3];
        return *this;
    }

    inline auto operator==(const Quaternion& v) const -> bool {
        return _v[0] == v._v[0] && _v[1] == v._v[1] && _v[2] == v._v[2] &&
               _v[3] == v._v[3];
    }

    inline auto operator!=(const Quaternion& v) const -> bool {
        return _v[0] != v._v[0] || _v[1] != v._v[1] || _v[2] != v._v[2] ||
               _v[3] != v._v[3];
    }

    inline auto operator<(const Quaternion& v) const -> bool {
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

    /* ----------------------------------
       Methods to access data members
    ---------------------------------- */

    [[nodiscard]] inline auto as_vec4() const -> Vec4<T> {
        return {_v[0], _v[1], _v[2], _v[3]};
    }

    [[nodiscard]] inline auto as_vec3() const -> Vec3<T> {
        return {_v[0], _v[1], _v[2]};
    }

    inline void set(value_t x, value_t y, value_t z, value_t w) {
        _v[0] = x;
        _v[1] = y;
        _v[2] = z;
        _v[3] = w;
    }

    inline void set(const Vec4<T>& v) {
        _v[0] = v.x();
        _v[1] = v.y();
        _v[2] = v.z();
        _v[3] = v.w();
    }

    inline auto operator[](int i) -> value_t& { return _v[i]; }
    inline auto operator[](int i) const -> value_t { return _v[i]; }

    inline auto x() -> value_t& { return _v[0]; }
    inline auto y() -> value_t& { return _v[1]; }
    inline auto z() -> value_t& { return _v[2]; }
    inline auto w() -> value_t& { return _v[3]; }

    [[nodiscard]] inline auto x() const -> value_t { return _v[0]; }
    [[nodiscard]] inline auto y() const -> value_t { return _v[1]; }
    [[nodiscard]] inline auto z() const -> value_t { return _v[2]; }
    [[nodiscard]] inline auto w() const -> value_t { return _v[3]; }

    /** return true if the Quat represents a zero rotation, and therefore can be
     * ignored in computations.*/
    [[nodiscard]] auto is_zero_rotation() const -> bool {
        return _v[0] == 0.0 && _v[1] == 0.0 && _v[2] == 0.0 && _v[3] == 1.0;
    }

    /* -------------------------------------------------------------
              BASIC ARITHMETIC METHODS
   Implemented in terms of Vec4s.  Some Vec4 operators, e.g.
   operator* are not appropriate for quaternions (as
   mathematical objects) so they are implemented differently.
   Also define methods for conjugate and the multiplicative inverse.
   ------------------------------------------------------------- */
    /// Multiply by scalar
    inline auto operator*(value_t rhs) const -> Quaternion {
        return {_v[0] * rhs, _v[1] * rhs, _v[2] * rhs, _v[3] * rhs};
    }

    /// Unary multiply by scalar
    inline auto operator*=(value_t rhs) -> Quaternion& {
        _v[0] *= rhs;
        _v[1] *= rhs;
        _v[2] *= rhs;
        _v[3] *= rhs;
        return *this; // enable nesting
    }

    // Adding a mult operation to re-order the quaternion multiplication:
    [[nodiscard]] inline auto mult(const Quaternion& rhs) const -> Quaternion {
        return {_v[3] * rhs._v[0] + _v[0] * rhs._v[3] + _v[1] * rhs._v[2] -
                    _v[2] * rhs._v[1],
                _v[3] * rhs._v[1] - _v[0] * rhs._v[2] + _v[1] * rhs._v[3] +
                    _v[2] * rhs._v[0],
                _v[3] * rhs._v[2] + _v[0] * rhs._v[1] - _v[1] * rhs._v[0] +
                    _v[2] * rhs._v[3],
                _v[3] * rhs._v[3] - _v[0] * rhs._v[0] - _v[1] * rhs._v[1] -
                    _v[2] * rhs._v[2]};
    }

    inline void post_mult(const Quaternion& rhs) {
        value_t x = _v[3] * rhs._v[0] + _v[0] * rhs._v[3] + _v[1] * rhs._v[2] -
                    _v[2] * rhs._v[1];
        value_t y = _v[3] * rhs._v[1] - _v[0] * rhs._v[2] + _v[1] * rhs._v[3] +
                    _v[2] * rhs._v[0];
        value_t z = _v[3] * rhs._v[2] + _v[0] * rhs._v[1] - _v[1] * rhs._v[0] +
                    _v[2] * rhs._v[3];
        _v[3] = _v[3] * rhs._v[3] - _v[0] * rhs._v[0] - _v[1] * rhs._v[1] -
                _v[2] * rhs._v[2];

        _v[2] = z;
        _v[1] = y;
        _v[0] = x;
    }

    /// Binary multiply
    inline auto operator*(const Quaternion& rhs) const -> Quaternion {
        return mult(rhs);
    }

    /// Unary multiply
    inline auto operator*=(const Quaternion& rhs) -> Quaternion& {
        post_mult(rhs);
        return *this;
    }

    /// Divide by scalar
    inline auto operator/(value_t rhs) const -> Quaternion {
        value_t div = 1.0 / rhs;
        return {_v[0] * div, _v[1] * div, _v[2] * div, _v[3] * div};
    }

    /// Unary divide by scalar
    inline auto operator/=(value_t rhs) -> Quaternion& {
        value_t div = 1.0 / rhs;
        _v[0] *= div;
        _v[1] *= div;
        _v[2] *= div;
        _v[3] *= div;
        return *this;
    }

    /// Binary divide
    inline auto operator/(const Quaternion& denom) const -> Quaternion {
        return ((*this).mult(denom.inverse()));
    }

    /// Unary divide
    inline auto operator/=(const Quaternion& denom) -> Quaternion& {
        (*this) = (*this).mult(denom.inverse());
        return (*this); // enable nesting
    }

    /// Binary addition
    inline auto operator+(const Quaternion& rhs) const -> Quaternion {
        return {_v[0] + rhs._v[0], _v[1] + rhs._v[1], _v[2] + rhs._v[2],
                _v[3] + rhs._v[3]};
    }

    /// Unary addition
    inline auto operator+=(const Quaternion& rhs) -> Quaternion& {
        _v[0] += rhs._v[0];
        _v[1] += rhs._v[1];
        _v[2] += rhs._v[2];
        _v[3] += rhs._v[3];
        return *this; // enable nesting
    }

    /// Binary subtraction
    inline auto operator-(const Quaternion& rhs) const -> Quaternion {
        return {_v[0] - rhs._v[0], _v[1] - rhs._v[1], _v[2] - rhs._v[2],
                _v[3] - rhs._v[3]};
    }

    /// Unary subtraction
    inline auto operator-=(const Quaternion& rhs) -> Quaternion& {
        _v[0] -= rhs._v[0];
        _v[1] -= rhs._v[1];
        _v[2] -= rhs._v[2];
        _v[3] -= rhs._v[3];
        return *this; // enable nesting
    }

    /** Negation operator - returns the negative of the quaternion.
    Basically just calls operator - () on the Vec4 */
    inline auto operator-() const -> Quaternion {
        return {-_v[0], -_v[1], -_v[2], -_v[3]};
    }

    /// Length of the quaternion = sqrt( vec . vec )
    [[nodiscard]] auto length() const -> value_t {
        return sqrt(_v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2] +
                    _v[3] * _v[3]);
    }

    /// Length of the quaternion = vec . vec
    [[nodiscard]] auto length2() const -> value_t {
        return _v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2] + _v[3] * _v[3];
    }

    /// Conjugate
    [[nodiscard]] inline auto conj() const -> Quaternion {
        return {-_v[0], -_v[1], -_v[2], _v[3]};
    }

    /// Multiplicative inverse method: q^(-1) = q^*/(q.q^*)
    [[nodiscard]] inline auto inverse() const -> Quaternion {
        return conj() / length2();
    }

    // Static constructor from yaw-pitch-roll for WebGPU convention
    // Coordinate system: X=right, Y=up, Z=forward (right-handed)
    // Convention: yaw around Y (up), pitch around X (right), roll around Z
    // (forward) Applied in order: Y (yaw) -> X (pitch) -> Z (roll)
    [[nodiscard]] static auto from_ypr(value_t yaw, value_t pitch, value_t roll)
        -> Quaternion {
        // Convert degrees → radians
        value_t y = toRad(-yaw);   // NEGATE yaw
        value_t p = toRad(-pitch); // NEGATE pitch
        value_t r = toRad(roll);   // roll unchanged

        value_t hy = y * value_t(0.5);
        value_t hp = p * value_t(0.5);
        value_t hr = r * value_t(0.5);

        value_t cy = cos(hy);
        value_t sy = sin(hy);
        value_t cp = cos(hp);
        value_t sp = sin(hp);
        value_t cr = cos(hr);
        value_t sr = sin(hr);

        Quaternion q;
        // Y (yaw) → X (pitch) → Z (roll)  (YXZ)
        q._v[0] = cy * sp * cr + sy * cp * sr; // x
        q._v[1] = sy * cp * cr - cy * sp * sr; // y
        q._v[2] = cy * cp * sr - sy * sp * cr; // z
        q._v[3] = cy * cp * cr + sy * sp * sr; // w

        return q;
    }

    template <typename U>
    [[nodiscard]] static auto from_ypr(Vec3<U> ypr) -> Quaternion {
        return from_ypr(ypr.x(), ypr.y(), ypr.z());
    }

    // Convert quaternion to yaw-pitch-roll for WebGPU convention
    // Coordinate system: X=right, Y=up, Z=forward (right-handed)
    // Convention: yaw around Y (up), pitch around X (right), roll around Z
    // (forward) YXZ Euler order
    [[nodiscard]] auto to_ypr() const -> Vec3<value_t> {
        const value_t x = _v[0];
        const value_t y = _v[1];
        const value_t z = _v[2];
        const value_t w = _v[3];

        value_t yawRad, pitchRad, rollRad;

        // Pitch (X axis)
        value_t sinp = value_t(2) * (w * x - y * z);

        if (fabs(sinp) >= value_t(1)) {
            // ===== GIMBAL LOCK =====
            pitchRad = copysign(value_t(PI) / value_t(2), sinp);
            rollRad = value_t(0);

            // At gimbal lock, we need to find the effective yaw that represents
            // the combined yaw-roll rotation. We use atan2 on the appropriate
            // matrix elements.

            if (sinp > 0) {
                // Pitch = +90° (looking straight up)
                // In this case, yaw - roll is recoverable
                yawRad = atan2(value_t(2) * (x * y - w * z),
                               value_t(1) - value_t(2) * (y * y + z * z));
            } else {
                // Pitch = -90° (looking straight down)
                // In this case, yaw + roll is recoverable
                yawRad = atan2(value_t(2) * (x * y + w * z),
                               value_t(1) - value_t(2) * (y * y + z * z));
            }
        } else {
            // ===== NORMAL CASE =====
            pitchRad = asin(sinp);

            yawRad = atan2(value_t(2) * (w * y + x * z),
                           value_t(1) - value_t(2) * (x * x + y * y));

            rollRad = atan2(value_t(2) * (w * z + x * y),
                            value_t(1) - value_t(2) * (x * x + z * z));
        }

        // Convert to degrees and apply camera conventions
        return {toDeg(-yawRad), toDeg(-pitchRad), toDeg(rollRad)};
    }

    /* --------------------------------------------------------
             METHODS RELATED TO ROTATIONS
      Set a quaternion which will perform a rotation of an
      angle around the axis given by the vector (x,y,z).
      Should be written to also accept an angle and a Vec3?

      Define Spherical Linear interpolation method also

      Not inlined - see the Quat.cpp file for implementation
      -------------------------------------------------------- */
    /// Set the elements of the Quat to represent a rotation of angle
    /// (radians) around the axis (x,y,z)
    void make_rotate(value_t angle, value_t x, value_t y, value_t z) {
        const value_t epsilon = 0.0000001;

        value_t length = sqrt(x * x + y * y + z * z);
        if (length < epsilon) {
            // ~zero length axis, so reset rotation to zero.
            *this = Quaternion<T>();
            return;
        }

        value_t inversenorm = 1.0 / length;
        value_t coshalfangle = cos(0.5 * angle);
        value_t sinhalfangle = sin(0.5 * angle);

        _v[0] = x * sinhalfangle * inversenorm;
        _v[1] = y * sinhalfangle * inversenorm;
        _v[2] = z * sinhalfangle * inversenorm;
        _v[3] = coshalfangle;
    }

    void make_rotate(value_t angle, const Vec3<T>& vec) {
        make_rotate(angle, vec[0], vec[1], vec[2]);
    }

    void make_rotate(value_t angle1, const Vec3<T>& axis1, value_t angle2,
                     const Vec3<T>& axis2, value_t angle3,
                     const Vec3<T>& axis3) {
        Quaternion q1;
        q1.make_rotate(angle1, axis1);
        Quaternion q2;
        q2.make_rotate(angle2, axis2);
        Quaternion q3;
        q3.make_rotate(angle3, axis3);

        *this = q3.mult(q2).mult(q1);
    }

    /** Make a rotation Quat which will rotate vec1 to vec2.
        Generally take a dot product to get the angle between these
        and then use a cross product to get the rotation axis
        Watch out for the two special cases of when the vectors
        are co-incident or opposite in direction.*/
    /** Make a rotation Quat which will rotate vec1 to vec2

    This routine uses only fast geometric transforms, without costly acos/sin
    computations. It's exact, fast, and with less degenerate cases than the
    acos/sin method.

    For an explanation of the math used, you may see for example:
    http://logiciels.cnes.fr/MARMOTTES/marmottes-mathematique.pdf

    @note This is the rotation with shortest angle, which is the one equivalent
    to the acos/sin transform method. Other rotations exists, for example to
    additionally keep a local horizontal attitude.

    @author Nicolas Brodu
    */

    void make_rotate(const Vec3<T>& vec1, const Vec3<T>& vec2) {

        // This routine takes any vector as argument but normalized
        // vectors are necessary, if only for computing the dot product.
        // Too bad the API is that generic, it leads to performance loss.
        // Even in the case the 2 vectors are not normalized but same length,
        // the sqrt could be shared, but we have no way to know beforehand
        // at this point, while the caller may know.
        // So, we have to test... in the hope of saving at least a sqrt
        Vec3<T> sourceVector = vec1;
        Vec3<T> targetVector = vec2;

        value_t fromLen2 = vec1.length2();
        value_t fromLen = NAN;
        // normalize only when necessary, epsilon test
        if ((fromLen2 < 1.0 - 1e-7) || (fromLen2 > 1.0 + 1e-7)) {
            fromLen = sqrt(fromLen2);
            sourceVector /= fromLen;
        } else
            fromLen = 1.0;

        value_t toLen2 = vec2.length2();
        // normalize only when necessary, epsilon test
        if ((toLen2 < 1.0 - 1e-7) || (toLen2 > 1.0 + 1e-7)) {
            value_t toLen = NAN;
            // re-use fromLen for case of mapping 2 vectors of the same length
            if ((toLen2 > fromLen2 - 1e-7) && (toLen2 < fromLen2 + 1e-7)) {
                toLen = fromLen;
            } else
                toLen = sqrt(toLen2);
            targetVector /= toLen;
        }

        // Now let's get into the real stuff
        // Use "dot product plus one" as test as it can be re-used later on
        double dotProdPlus1 = 1.0 + sourceVector.dot(targetVector);

        // Check for degenerate case of full u-turn. Use epsilon for detection
        if (dotProdPlus1 < 1e-7) {

            // Get an orthogonal vector of the given vector
            // in a plane with maximum vector coordinates.
            // Then use it as quaternion axis with pi angle
            // Trick is to realize one value at least is >0.6 for a normalized
            // vector.
            if (fabs(sourceVector.x()) < 0.6) {
                const double norm =
                    sqrt(1.0 - sourceVector.x() * sourceVector.x());
                _v[0] = 0.0;
                _v[1] = sourceVector.z() / norm;
                _v[2] = -sourceVector.y() / norm;
                _v[3] = 0.0;
            } else if (fabs(sourceVector.y()) < 0.6) {
                const double norm =
                    sqrt(1.0 - sourceVector.y() * sourceVector.y());
                _v[0] = -sourceVector.z() / norm;
                _v[1] = 0.0;
                _v[2] = sourceVector.x() / norm;
                _v[3] = 0.0;
            } else {
                const double norm =
                    sqrt(1.0 - sourceVector.z() * sourceVector.z());
                _v[0] = sourceVector.y() / norm;
                _v[1] = -sourceVector.x() / norm;
                _v[2] = 0.0;
                _v[3] = 0.0;
            }
        }

        else {
            // Find the shortest angle quaternion that transforms normalized
            // vectors into one other. Formula is still valid when vectors are
            // colinear
            const double s = sqrt(0.5 * dotProdPlus1);
            const Vec3<T> tmp = sourceVector ^ targetVector / (2.0 * s);
            _v[0] = tmp.x();
            _v[1] = tmp.y();
            _v[2] = tmp.z();
            _v[3] = s;
        }
    }

    /** Return the angle and vector components represented by the quaternion.*/
    // Get the angle of rotation and axis of this Quat object.
    // Won't give very meaningful results if the Quat is not associated
    // with a rotation!
    void get_rotate(value_t& angle, value_t& x, value_t& y, value_t& z) const {
        value_t sinhalfangle =
            sqrt(_v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2]);

        angle = 2.0 * atan2(sinhalfangle, _v[3]);
        if (sinhalfangle != 0.0) {
            x = _v[0] / sinhalfangle;
            y = _v[1] / sinhalfangle;
            z = _v[2] / sinhalfangle;
        } else {
            x = 0.0;
            y = 0.0;
            z = 1.0;
        }
    }

    /** Return the angle and vector represented by the quaternion.*/
    void get_rotate(value_t& angle, Vec3<T>& vec) const {
        get_rotate(angle, vec.x(), vec.y(), vec.z());
    }

    auto normalize() -> value_t {
        value_t len =
            sqrt(_v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2] + _v[3] * _v[3]);

        if (len > value_t(0.0)) {
            value_t inv_len = value_t(1.0) / len;
            _v[0] *= inv_len;
            _v[1] *= inv_len;
            _v[2] *= inv_len;
            _v[3] *= inv_len;
        }

        return len;
    }

    // Return normalized copy without modifying original
    [[nodiscard]] auto normalized() const -> Quaternion {
        Quaternion result(*this);
        result.normalize();
        return result;
    }

    /** Spherical Linear Interpolation.
    As t goes from 0 to 1, the Quat object goes from "from" to "to". */
    /// Spherical Linear Interpolation
    /// As t goes from 0 to 1, the Quat object goes from "from" to "to"
    /// Reference: Shoemake at SIGGRAPH 89
    /// See also
    /// http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm
    static auto slerp(const Quaternion& from, const Quaternion& to, value_t t)
        -> Quaternion {
        const value_t epsilon =
            std::numeric_limits<value_t>::epsilon() * value_t(10.0);

        value_t cosomega = from.as_vec4().dot(to.as_vec4());

        Quaternion quatTo = to;
        if (cosomega < value_t(0.0)) {
            cosomega = -cosomega;
            quatTo = -to;
        }

        value_t scale_from;
        value_t scale_to;

        if ((value_t(1.0) - cosomega) > epsilon) {
            value_t omega = acos(cosomega);
            value_t sinomega = sin(omega);
            scale_from = sin((value_t(1.0) - t) * omega) / sinomega;
            scale_to = sin(t * omega) / sinomega;
        } else {
            scale_from = value_t(1.0) - t;
            scale_to = t;
        }

        Quaternion result = (from * scale_from) + (quatTo * scale_to);
        result.normalize();
        return result;
    }

    static auto slerp(const Vec3<value_t>& from, const Vec3<value_t>& to,
                      value_t t) -> Quaternion {
        return slerp(Quaternion{}, Quaternion(from, to), t);
    }

    /** Rotate a vector by this quaternion.*/
    auto operator*(const Vec3f& v) const -> Vec3f {
        // nVidia SDK implementation
        Vec3f uv;
        Vec3f uuv;
        Vec3f qvec(_v[0], _v[1], _v[2]);
        uv = qvec ^ v;
        uuv = qvec ^ uv;
        uv *= (2.0F * _v[3]);
        uuv *= 2.0F;
        return v + uv + uuv;
    }

    /** Rotate a vector by this quaternion.*/
    auto operator*(const Vec3d& v) const -> Vec3d {
        // nVidia SDK implementation
        Vec3d uv;
        Vec3d uuv;
        Vec3d qvec(_v[0], _v[1], _v[2]);
        uv = qvec ^ v;
        uuv = qvec ^ uv;
        uv *= (2.0 * _v[3]);
        uuv *= 2.0;
        return v + uv + uuv;
    }

    template <class Archive> void serialize(Archive& ar) { ar(_v); }
}; // end of class prototype

using Quatf = Quaternion<F32>;
using Quatd = Quaternion<F64>;

} // namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::Quatd& q) -> std::ostream& {
    os << "Quatd(" << q.x() << ", " << q.y() << ", " << q.z() << ", " << q.w()
       << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::Quatf& q) -> std::ostream& {
    os << "Quatf(" << q.x() << ", " << q.y() << ", " << q.z() << ", " << q.w()
       << ")";
    return os;
}
} // namespace std

template <> struct fmt::formatter<nv::Quatf> {
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
    auto format(const nv::Quatf vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Quatf({:6g}, {:6g}, {:6g}, {:6g})",
                              vec[0], vec[1], vec[2], vec[3]);
    }
};

template <> struct fmt::formatter<nv::Quatd> {
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
    auto format(const nv::Quatd vec, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Quatd({:6g}, {:6g}, {:6g}, {:6g})",
                              vec[0], vec[1], vec[2], vec[3]);
    }
};

#endif
