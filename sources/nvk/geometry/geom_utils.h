#ifndef _GEOM_UTILS_H_
#define _GEOM_UTILS_H_

#include <external/earcut.hpp>
#include <nvk_common.h>

namespace nv {

enum PolygonFillRule {
    FILL_EVENODD,
    FILL_NONZERO,
    FILL_POSITIVE,
    FILL_NEGATIVE,
};

enum PathJoinType {
    PATH_JOIN_SQUARE,
    PATH_JOIN_BEVEL,
    PATH_JOIN_ROUND,
    PATH_JOIN_MITER,
};

enum PathEndType {
    PATH_END_POLYGON,
    PATH_END_JOINED,
    PATH_END_BUTT,
    PATH_END_SQUARE,
    PATH_END_ROUND,
};

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

// ---------------------------------------------------------------------------
// ray_intersect_segment2d
//
// Tests a ray (origin, dir) against a finite line segment (a, b).
// Unlike seg2_intersect, the ray is unbounded in the forward direction:
//   only requires  tSeg ∈ [0, 1]  (point lies on segment)
//   and            tRay > -kEps   (point is in front of origin).
//
// Returns true and writes tRay (>= 0) if an intersection exists.
// tRay is the scalar such that  origin + dir * tRay == intersection point.
// ---------------------------------------------------------------------------
template <typename T>
auto ray_intersect_segment2d(const Vec2<T>& origin, const Vec2<T>& dir,
                             const Vec2<T>& a, const Vec2<T>& b, T& tRay)
    -> bool {
    // Solve:  origin + tRay * dir  ==  a + tSeg * (b - a)
    // Rearranged as a 2x2 linear system:
    //   [ dir | -(b-a) ] * [ tRay; tSeg ] = a - origin
    const Vec2<T> seg = b - a;
    const T det = dir.x() * (-seg.y()) - dir.y() * (-seg.x());
    // det == cross(dir, seg)  (2-D cross product)

    constexpr T kEps = T(100) * std::numeric_limits<T>::epsilon();
    if (std::abs(det) < kEps)
        return false; // ray and segment are parallel

    const Vec2<T> rhs = a - origin;
    const T invDet = 1.0 / det;

    tRay = (rhs.x() * (-seg.y()) - rhs.y() * (-seg.x())) * invDet;
    const T tSeg = (dir.x() * rhs.y() - dir.y() * rhs.x()) * invDet;

    // Ray must go forward; segment must be hit within [0, 1].
    if (tRay < -kEps || tSeg < -kEps || tSeg > 1.0 + kEps)
        return false;

    tRay = std::max(tRay, 0.0); // clamp tiny negatives to zero
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

// ---------------------------------------------------------------------------
// point_in_convex
//
// Winding-agnostic: p is inside (or on the boundary of) the convex polygon iff
// it never sits strictly on both sides of the polygon's edges. A small epsilon
// keeps points on an edge classified as inside.
// ---------------------------------------------------------------------------
template <typename T>
auto point_in_convex(const Vector<Vec2<T>>& hull, const Vec2<T>& p) -> bool {
    const U32 n = U32(hull.size());
    if (n < 3)
        return false;

    constexpr F64 kEps = 1e-6; // cm^2 scale — negligible at world units
    bool pos = false;
    bool neg = false;
    for (U32 i = 0; i < n; ++i) {
        const Vec2<T>& a = hull[i];
        const Vec2<T>& b = hull[(i + 1) % n];
        const F64 s = (b - a).cross(p - a);
        if (s > kEps)
            pos = true;
        else if (s < -kEps)
            neg = true;
        if (pos && neg)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// segment_intersects_convex
//
// True iff segment a→b touches the convex polygon `hull` at all: either
// endpoint inside (boundary counts), or the segment crosses any hull edge. For
// a convex hull these cases are exhaustive — a chord passing clean through with
// both endpoints outside must cross the boundary, so the edge test catches it.
// ---------------------------------------------------------------------------
template <typename T>
auto segment_intersects_convex(const Vector<Vec2<T>>& hull, const Vec2<T>& a,
                               const Vec2<T>& b) -> bool {
    const U32 n = U32(hull.size());
    if (n < 3)
        return false;

    if (point_in_convex(hull, a) || point_in_convex(hull, b))
        return true;

    Vec2<T> ip;
    for (U32 i = 0; i < n; ++i) {
        if (seg2_intersect(a, b, hull[i], hull[(i + 1) % n], ip))
            return true;
    }
    return false;
}

template <typename T> void polyline2_reverse(Vector<Vec2<T>>& line) {
    std::reverse(line.begin(), line.end());
}

template <typename T> auto polyline2_length(const Vector<Vec2<T>>& pts) -> T {
    T len = 0;
    if (pts.size() < 2) {
        return len;
    }
    for (I32 i = 1; i < pts.size(); ++i) {
        len += (pts[i] - pts[i - 1]).length();
    }
    return len;
}

template <typename T> struct Polyline2Hit {
    Vec2<T> pos;
    T distA{0.0}; // arc-length along polyline A from its first point
    T distB{0.0}; // arc-length along polyline B from its first point
};

// Finds the first intersection between polylines a and b, walking a from its
// start outward. "First" means the hit with the smallest arc-length along a
// (and, within a single a-segment, the smallest local distance). Returns true
// and fills hit when an intersection exists, false otherwise (parallel or
// diverging polylines).
//
// Cost is O(numSegsA * numSegsB) worst case but exits on the first hit, which
// for junction corner borders is typically found within the first few
// segments.
template <typename T>
auto polyline2_find_first_intersection(const Vector<Vec2<T>>& pa,
                                       const Vector<Vec2<T>>& pb,
                                       Polyline2Hit<T>& hit) -> bool {
    if (pa.size() < 2 || pb.size() < 2) {
        return false;
    }

    // Cumulative arc-lengths along b (a's arc-length is accumulated on the
    // fly in the outer loop):
    Vector<F64> cumB(pb.size(), 0.0);
    for (U32 k = 1; k < pb.size(); ++k) {
        cumB[k] = cumB[k - 1] + (pb[k] - pb[k - 1]).length();
    }

    F64 cumA = 0.0;
    for (U32 i = 0; i + 1 < pa.size(); ++i) {
        const Vec2d& a0 = pa[i];
        const Vec2d& a1 = pa[i + 1];

        // Keep the hit with the smallest local distance along this a-segment
        // so "first along a" is well defined even with multiple b crossings:
        bool found = false;
        F64 bestDistA = 0.0;

        for (U32 k = 0; k + 1 < pb.size(); ++k) {
            Vec2d ip;
            if (!seg2_intersect(a0, a1, pb[k], pb[k + 1], ip)) {
                continue;
            }
            F64 dA = (ip - a0).length();
            if (!found || dA < bestDistA) {
                found = true;
                bestDistA = dA;
                hit.pos = ip;
                hit.distA = cumA + dA;
                hit.distB = cumB[k] + (ip - pb[k]).length();
            }
        }

        if (found) {
            return true;
        }
        cumA += (a1 - a0).length();
    }

    return false;
}

template <typename T>
auto polyline2_sample_at(const Vector<Vec2<T>>& pts, T dist) -> Vec2<T> {
    NVCHK(pts.size() >= 2, "Polyline2::sample_at: need >= 2 points.");

    if (dist <= 0.0) {
        return pts.front();
    }

    F64 cum = 0.0;
    for (U32 i = 1; i < pts.size(); ++i) {
        F64 len = (pts[i] - pts[i - 1]).length();
        if (dist <= (cum + len)) {
            F64 x = len > 0.0 ? (dist - cum) / len : 0.0;
            return pts[i - 1] * (1.0 - x) + pts[i] * x;
        }
        cum += len;
    }

    return pts.back();
}

template <typename T>
auto polyline2_append_slice(Vector<Vec2<T>>& out, const Vector<Vec2<T>>& pts,
                            T d0, T d1, T eps = 0.01) {
    // 0.01 cm guard band around the bounds (see header comment):
    const bool reversed = d0 > d1;
    T lo = std::min(d0, d1) + eps;
    T hi = std::max(d0, d1) - eps;
    if (hi <= lo) {
        return;
    }

    auto mark = I64(out.size());

    T cum = 0.0;
    for (U32 i = 0; i < pts.size(); ++i) {
        if (i > 0) {
            cum += (pts[i] - pts[i - 1]).length();
        }
        if (cum > hi) {
            break;
        }
        if (cum >= lo) {
            out.push_back(pts[i]);
        }
    }

    if (reversed) {
        std::reverse(out.begin() + mark, out.end());
    }
}

// Removes consecutive near-duplicate points (including the wrap-around pair)
// from a closed polygon point loop.
template <typename T>
auto polyline2_dedupe_points(Vector<Vec2<T>>& pts, bool closedLoop = false)
    -> U32 {
    constexpr F64 kMinSpacingCm = 0.01;

    U32 count = 0;
    Vector<Vec2<T>> out;
    out.reserve(pts.size());
    for (const auto& pt : pts) {
        if (!out.empty() && (pt - out.back()).length() < kMinSpacingCm) {
            count++;
            continue;
        }
        out.push_back(pt);
    }

    while (closedLoop && out.size() >= 2 &&
           (out.back() - out.front()).length() < kMinSpacingCm) {
        count++;
        out.pop_back();
    }
    pts = std::move(out);
    return count;
}

template <typename T> inline auto distance(const T& a, const T& b) -> T {
    return std::abs(a - b);
}
template <typename T>
inline auto distance(const Vec2<T>& a, const Vec2<T>& b) -> T {
    return (a - b).length();
}
template <typename T>
inline auto distance(const Vec3<T>& a, const Vec3<T>& b) -> T {
    return (a - b).length();
}
template <typename T>
inline auto distance(const Vec4<T>& a, const Vec4<T>& b) -> T {
    return (a - b).length();
}

template <typename T> struct PolylineRayHit {
    T tRay{0.0};   // distance parameter along the ray
    U32 segIdx{0}; // index of the polyline segment that was hit
    Vec2<T> point; // world position of the intersection
};

template <typename T>
auto polyline_ray_intersections(const Vec2<T>& origin, const Vec2<T>& dir,
                                const Vector<Vec2<T>>& points,
                                bool closedLoop = false)
    -> Vector<PolylineRayHit<T>> {
    Vector<PolylineRayHit<T>> hits;

    const U32 n = U32(points.size());
    if (n < 2)
        return hits;

    // Number of segments: n-1 for open polylines, n for closed loops
    // (the extra segment wraps points[n-1] → points[0]).
    const U32 numSegs = closedLoop ? n : n - 1;

    for (U32 i = 0; i < numSegs; ++i) {
        const Vec2<T>& a = points[i];
        const Vec2<T>& b = points[(i + 1) % n]; // wrap on the closing segment

        T tRay = T(0);
        if (ray_intersect_segment2d(origin, dir, a, b, tRay)) {
            if (tRay >= T(0)) {
                hits.push_back({
                    .tRay = tRay,
                    .segIdx = i,
                    .point = origin + dir * tRay,
                });
            }
        }
    }

    // Sort closest first so callers can just take hits[0].
    std::sort(hits.begin(), hits.end(),
              [](const PolylineRayHit<T>& x, const PolylineRayHit<T>& y) {
                  return x.tRay < y.tRay;
              });

    return hits;
}

} // namespace nv

#endif
