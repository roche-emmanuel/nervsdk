#ifndef _NV_GEOMETRY2_H_
#define _NV_GEOMETRY2_H_

#include <nvk_common.h>

namespace nv {

template <typename T>
auto seg2_intersect(const Vec2<T>& seg0_a, const Vec2<T>& seg0_b,
                    const Vec2<T>& seg1_a, const Vec2<T>& seg1_b,
                    Vec2<T>& intersecPt) -> bool {
    // Direction vectors
    const T d0x = seg0_b.x() - seg0_a.x();
    const T d0y = seg0_b.y() - seg0_a.y();
    const T d1x = seg1_b.x() - seg1_a.x();
    const T d1y = seg1_b.y() - seg1_a.y();

    // Cross product of direction vectors (determinant)
    const T det = d0x * d1y - d0y * d1x;

    // Parallel or collinear segments (no unique intersection)
    constexpr T epsilon = 1e-10;
    if (std::abs(det) < epsilon) {
        return false;
    }

    // Vector from seg0_a to seg1_a
    const T dx = seg1_a.x() - seg0_a.x();
    const T dy = seg1_a.y() - seg0_a.y();

    // Compute parametric coefficients
    const T inv_det = 1.0 / det;
    const T t = (dx * d1y - dy * d1x) * inv_det;
    const T u = (dx * d0y - dy * d0x) * inv_det;

    // Check if intersection is within both segments [0, 1]
    if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0) {
        return false;
    }

    // Compute intersection point
    intersecPt.x() = seg0_a.x() + t * d0x;
    intersecPt.y() = seg0_a.y() + t * d0y;

    return true;
}

/** General segment/circle intersection function: */
template <typename T>
auto seg2_circle_intersect(const Vec2<T>& seg_a, const Vec2<T>& seg_b,
                           const Vec2<T>& center, T radius, T& t0, T& t1)
    -> I32 {
    // Direction vector from a to b
    Vec2<T> d = seg_b - seg_a;

    // Vector from center to segment start
    Vec2<T> f = seg_a - center;

    // Solve quadratic equation: ||a + t*d - center||^2 = radius^2
    // Expands to: (d·d)t^2 + 2(f·d)t + (f·f - r^2) = 0
    T a = d.dot(d);
    T b = T(2) * f.dot(d);
    T c = f.dot(f) - radius * radius;

    // Check if segment is degenerate (a and b are the same point)
    if (a < std::numeric_limits<T>::epsilon()) {
        // Check if the point is on the circle
        if (std::abs(c) < std::numeric_limits<T>::epsilon()) {
            t0 = T(0);
            return 1;
        }
        return 0;
    }

    // Calculate discriminant
    T discriminant = b * b - T(4) * a * c;

    // No intersection
    if (discriminant < T(0)) {
        return 0;
    }

    // One intersection (tangent)
    if (discriminant < std::numeric_limits<T>::epsilon()) {
        T t = -b / (T(2) * a);

        // Check if intersection is within segment bounds [0, 1]
        if (t >= T(0) && t <= T(1)) {
            t0 = t;
            return 1;
        }
        return 0;
    }

    // Two intersections
    T sqrt_discriminant = std::sqrt(discriminant);
    T inv_2a = T(1) / (T(2) * a);

    T t_minus = (-b - sqrt_discriminant) * inv_2a;
    T t_plus = (-b + sqrt_discriminant) * inv_2a;

    // Count how many intersections are within [0, 1]
    I32 count = 0;

    if (t_minus >= T(0) && t_minus <= T(1)) {
        t0 = t_minus;
        count++;
    }

    if (t_plus >= T(0) && t_plus <= T(1)) {
        if (count == 0) {
            t0 = t_plus;
        } else {
            t1 = t_plus;
        }
        count++;
    }

    // If only one intersection is in bounds but the other exists outside,
    // we might need to handle the case where we found t_plus but not t_minus
    if (count == 1 && t_minus < T(0) && t_plus >= T(0) && t_plus <= T(1)) {
        t0 = t_plus;
    }

    return count;
}

template <typename T>
auto seg2_circle_cross(const Vec2<T>& seg_a, const Vec2<T>& seg_b,
                       const Vec2<T>& center, T radius, T& t0, bool a_outside)
    -> bool {
    Vec2<T> d = seg_b - seg_a;
    Vec2<T> f = seg_a - center;

    T a = d.dot(d);
    T b = T(2) * f.dot(d);
    T c = f.dot(f) - radius * radius;

    T discriminant = b * b - T(4) * a * c;
    T sqrt_disc = std::sqrt(discriminant);
    T inv_2a = T(1) / (T(2) * a);

    // outside->inside: use smaller t (entry point)
    // inside->outside: use larger t (exit point)
    t0 = a_outside ? (-b - sqrt_disc) * inv_2a : (-b + sqrt_disc) * inv_2a;

    return true;
}

template <typename T>
auto seg2_circle_entry(const Vec2<T>& seg_a, const Vec2<T>& seg_b,
                       const Vec2<T>& center, T radius, T& t0) -> bool {
    Vec2<T> d = seg_b - seg_a;
    Vec2<T> f = seg_a - center;

    T a = d.dot(d);
    T b = T(2) * f.dot(d);
    T c = f.dot(f) - radius * radius;

    T discriminant = b * b - T(4) * a * c;
    T sqrt_disc = std::sqrt(discriminant);
    T inv_2a = T(1) / (T(2) * a);

    // outside->inside: use smaller t (entry point)
    // inside->outside: use larger t (exit point)
    t0 = (-b - sqrt_disc) * inv_2a;

    return true;
}

template <typename T>
auto seg2_circle_exit(const Vec2<T>& seg_a, const Vec2<T>& seg_b,
                      const Vec2<T>& center, T radius, T& t0) -> bool {
    Vec2<T> d = seg_b - seg_a;
    Vec2<T> f = seg_a - center;

    T a = d.dot(d);
    T b = T(2) * f.dot(d);
    T c = f.dot(f) - radius * radius;

    T discriminant = b * b - T(4) * a * c;
    T sqrt_disc = std::sqrt(discriminant);
    T inv_2a = T(1) / (T(2) * a);

    // outside->inside: use smaller t (entry point)
    // inside->outside: use larger t (exit point)
    t0 = (-b + sqrt_disc) * inv_2a;

    return true;
}

// template <typename T>
// auto seg2_point_distance(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>&
// pt)
//     -> T {
//     Vec2<T> ab = b - a;
//     T ab_len_sq = ab.dot(ab);

//     // Handle degenerate case: segment is a point
//     if (ab_len_sq < T(1e-20)) {
//         return (pt - a).length();
//     }

//     T t = (pt - a).dot(ab) / ab_len_sq;
//     t = std::clamp(t, T(0.0), T(1.0));
//     Vec2<T> proj = a + ab * t;
//     return (pt - proj).length();
// }

template <typename T>
auto seg2_point_distance(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& pt,
                         bool clampProj = true, T* t = nullptr) -> T {
    Vec2<T> ab = b - a;
    T ab_len_sq = ab.dot(ab);

    // Handle degenerate case: segment is a point
    if (ab_len_sq < T(1e-20)) {
        if (t)
            *t = T(0);
        return (pt - a).length();
    }

    T t_val = (pt - a).dot(ab) / ab_len_sq;

    if (clampProj)
        t_val = std::clamp(t_val, T(0.0), T(1.0));

    if (t)
        *t = t_val;

    Vec2<T> proj = a + ab * t_val;
    return (pt - proj).length();
}

template <typename T>
auto seg2_project_point(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& pt,
                        T* t = nullptr) -> Vec2<T> {
    Vec2<T> ab = b - a;
    T ab_len_sq = ab.dot(ab);

    // Handle degenerate case: segment is a point
    if (ab_len_sq < T(1e-20)) {
        if (t)
            *t = T(0);
        return a;
    }

    T t_val = (pt - a).dot(ab) / ab_len_sq;
    if (t)
        *t = t_val;

    return a + ab * t_val;
}

template <typename T>
auto polygon_signed_area_2d(const Vec2<T>* polygon, U32 size) -> T {
    if (size < 3)
        return T(0.0);
    NVCHK(polygon != nullptr, "Invalid polygon");

    T sum = 0.0;
    for (size_t i = 1; i < size - 1; ++i) {
        auto v1 = polygon[i] - polygon[0];
        auto v2 = polygon[i + 1] - polygon[0];
        sum += v1.x() * v2.y() - v1.y() * v2.x();
    }

    return sum * T(0.5);
}

template <typename T>
auto polygon_signed_area_xy(const Vec3<T>* polygon, U32 size) -> T {
    if (size < 3)
        return T(0.0);

    NVCHK(polygon != nullptr, "Invalid polygon");

    T sum = 0.0;
    for (size_t i = 1; i < size - 1; ++i) {
        auto v1 = polygon[i] - polygon[0];
        auto v2 = polygon[i + 1] - polygon[0];
        sum += v1.x() * v2.y() - v1.y() * v2.x();
    }

    return sum * T(0.5);
}

/**
Note: the formula below assume that all the polygon points are in the same
plane!
*/
template <typename T>
auto polygon_planar_area(const Vec3<T>* polygon, U32 size) -> T {
    if (size < 3)
        return T(0);

    NVCHK(polygon != nullptr, "Invalid polygon");

    Vec3<T> normal(0, 0, 0);
    for (size_t i = 1; i < size - 1; ++i) {
        auto v1 = polygon[i] - polygon[0];
        auto v2 = polygon[i + 1] - polygon[0];
        normal += v1.cross(v2); // Accumulate cross products
    }

    return normal.length() * T(0.5); // Magnitude gives area
}

template <typename T>
auto polygon_signed_area_2d(const Vec3<T>* polygon, U32 size,
                            const Vec3<T>& plane_normal) -> T {
    if (size < 3)
        return T(0);

    NVCHK(polygon != nullptr, "Invalid polygon");

    Vec3<T> accumulated_normal(0, 0, 0);
    for (size_t i = 1; i < size - 1; ++i) {
        auto v1 = polygon[i] - polygon[0];
        auto v2 = polygon[i + 1] - polygon[0];
        accumulated_normal += v1.cross(v2);
    }

    // Project onto plane normal to get signed twice-area
    T signed_twice_area = accumulated_normal.dot(plane_normal);

    return signed_twice_area * T(0.5);
}

template <typename T> struct Polyline2 {
    I32 id;
    Vector<Vec2<T>> points;
    bool closedLoop;
};

template <typename T> using Polyline2Vector = Vector<Polyline2<T>>;

template <typename T> struct Segment2 {
    Vec2<T> a;
    Vec2<T> b;

    I32 lineId;
    I32 index; // index in the polyline
    bool isLastLoopSeg{false};

    auto bounds() const -> Box2<T> { return {a, b}; }

    auto intersect(const Segment2& seg, Vec2<T>& intersectPt) const -> bool {
        return seg2_intersect(a, b, seg.a, seg.b, intersectPt);
    };

    auto point_distance(const Vec2<T>& pt, bool clampProj, T* t = nullptr) const
        -> T {
        return seg2_point_distance(a, b, pt, clampProj, t);
    }
};

using Seg2f = Segment2<F32>;
using Seg2d = Segment2<F64>;

template <typename T> using Segment2Vector = Vector<Segment2<T>>;

template <typename T> struct Segment2Intersection {
    Vec2<T> position;

    Segment2<T> s0;
    Segment2<T> s1;
};

template <typename T>
using Segment2IntersectionVector = Vector<Segment2Intersection<T>>;

template <typename T> struct EndpointNearSegment2 {
    Vec2<T> endpoint;
    Vec2<T> intersection;
    int pathId;
    bool isStart; // true = start, false = end

    Segment2<T> segment;
    T distance;
};

template <typename T>
using EndpointNearSegment2Vector = Vector<EndpointNearSegment2<T>>;

template <typename T> struct Polyline2IntersectionResults {
    Segment2IntersectionVector<T> intersections;
    EndpointNearSegment2Vector<T> endpointNearSegments;
};

auto compute_polyline2_intersections(const Polyline2Vector<F32>& paths,
                                     F32 endpointDistance)
    -> Polyline2IntersectionResults<F32>;

auto compute_polyline2_intersections(const Polyline2Vector<F64>& paths,
                                     F64 endpointDistance)
    -> Polyline2IntersectionResults<F64>;

}; // namespace nv

#endif
