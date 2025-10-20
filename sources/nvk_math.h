#ifndef NV_NVK_MATH_
#define NV_NVK_MATH_

#include <cmath>
namespace nv {

// define the standard trig values
#ifdef PI
#undef PI
#undef PI_2
#undef PI_4
#endif
const double PI = 3.14159265358979323846;
const double PI_2 = 1.57079632679489661923;
const double PI_4 = 0.78539816339744830962;
const double LN_2 = 0.69314718055994530942;
const double INVLN_2 = 1.0 / LN_2;
const double EXP_1 = 2.718281828459045235360;

const float PI_f = 3.14159265358979323846;
const float PI_2_f = 1.57079632679489661923;
const float PI_4_f = 0.78539816339744830962;
const float EXP_1_f = 2.718281828459045235360;

/**
 * Returns the arccosinus of x clamped to [-1,1].
 */
template <typename T> inline auto safeAcos(T x) -> T {
    if (x <= -1) {
        x = -1;
    } else if (x >= 1) {
        x = 1;
    }
    return acos(x);
}

/**
 * Returns the arcsinus of x clamped to [-1,1].
 */
template <typename T> inline auto safeAsin(T x) -> T {
    if (x <= -1) {
        x = -1;
    } else if (x >= 1) {
        x = 1;
    }
    return asin(x);
}

/** Convert from radians to degrees.*/
template <typename T> inline auto toRad(const T& val) -> T {
    return val * (nv::PI / 180.0);
}

/** Convert from degrees to radians*/
template <typename T> inline auto toDeg(const T& val) -> T {
    return val * (180.0 / nv::PI);
}

template <typename T> inline auto minimum(const T& a, const T& b) -> T {
    return a < b ? a : b;
}

template <typename T> inline auto maximum(const T& a, const T& b) -> T {
    return a > b ? a : b;
}

template <typename T>
constexpr inline auto clamp(const T& x, const T& mini, const T& maxi) -> T {
    return x < mini ? mini : (x > maxi ? maxi : x);
}

template <typename T> inline auto clampAbove(T v, T minimum) -> T {
    return v < minimum ? minimum : v;
}

template <typename T> inline auto clampBelow(T v, T maximum) -> T {
    return v > maximum ? maximum : v;
}

/** return the minimum of two values, equivalent to std::min.
 * std::min not used because of STL implementation under IRIX not
 * containing std::min.
 */
template <typename T> inline auto absolute(T v) -> T {
    return v < (T)0 ? -v : v;
}

/** return true if float lhs and rhs are equivalent,
 * meaning that the difference between them is less than an epsilon value
 * which defaults to 1e-6.
 */
inline auto equivalent(float lhs, float rhs, float epsilon = 1e-6) -> bool {
    float delta = rhs - lhs;
    return delta < 0.0F ? delta >= -epsilon : delta <= epsilon;
}

/** return true if double lhs and rhs are equivalent,
 * meaning that the difference between them is less than an epsilon value
 * which defaults to 1e-6.
 */
inline auto equivalent(double lhs, double rhs, double epsilon = 1e-6) -> bool {
    double delta = rhs - lhs;
    return delta < 0.0 ? delta >= -epsilon : delta <= epsilon;
}

template <typename T> inline auto sign(T v) -> T {
    return v < (T)0 ? (T)-1 : (T)1;
}

template <typename T> inline auto signOrZero(T v) -> T {
    return v < (T)0 ? (T)-1 : (v > (T)0 ? (T)1 : 0);
}

template <typename T> inline auto square(T v) -> T { return v * v; }

template <typename T> inline auto signedSquare(T v) -> T {
    return v < (T)0 ? -v * v : v * v;
    ;
}

inline auto round(float v) -> float {
    return v >= 0.0F ? floorf(v + 0.5F) : ceilf(v - 0.5F);
}
inline auto round(double v) -> double {
    return v >= 0.0 ? floor(v + 0.5) : ceil(v - 0.5);
}

template <typename T> inline auto isNaN(T v) -> bool { return std::isnan(v); }
template <> inline auto isNaN(int32_t v) -> bool { return false; }
template <> inline auto isNaN(uint32_t v) -> bool { return false; }

template <typename T> struct Range;

} // namespace nv

#endif
