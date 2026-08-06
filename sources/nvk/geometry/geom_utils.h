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
auto polygon_centroid_2d(const Vec2<T>* polygon, U32 size) -> T {
    Vec2d centroid{0.0, 0.0};
    for (size_t i = 0; i < size; ++i) {
        centroid = centroid + polygon[i];
    }
    centroid = centroid / T(size);
    return centroid;
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
// Winding-agnostic: p is inside (or on the boundary of) the convex polygon
// iff it never sits strictly on both sides of the polygon's edges. A small
// epsilon keeps points on an edge classified as inside.
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
// endpoint inside (boundary counts), or the segment crosses any hull edge.
// For a convex hull these cases are exhaustive — a chord passing clean
// through with both endpoints outside must cross the boundary, so the edge
// test catches it.
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

// Finds the first intersection between polylines a and b, walking a from
// its start outward. "First" means the hit with the smallest arc-length
// along a (and, within a single a-segment, the smallest local distance).
// Returns true and fills hit when an intersection exists, false otherwise
// (parallel or diverging polylines).
//
// Cost is O(numSegsA * numSegsB) worst case but exits on the first hit,
// which for junction corner borders is typically found within the first few
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

        // Keep the hit with the smallest local distance along this
        // a-segment so "first along a" is well defined even with multiple b
        // crossings:
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

// Removes consecutive near-duplicate points (including the wrap-around
// pair) from a closed polygon point loop.
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

// ---------------------------------------------------------------------------
// snap_points_to_grid
//
// Quantizes every coordinate onto a regular grid of step `gridStep`, rounding
// half away from zero so the mapping stays symmetric about the origin. Two
// points falling in the same cell become bit identical afterwards, which is
// what the boolean ops need in order to see 2 polygons sharing an edge as
// really sharing it rather than as 2 hairline separated contours.
//
// `gridStep` is expressed in the unit of the coordinates (cm in the PCGen
// pipeline). A non positive step leaves the points untouched.
//
// Beware that snapping is *not* a weld: 2 points straddling a cell boundary
// land on different cells however close they were. It removes accumulated
// float noise, it does not close a genuine gap, and it does nothing for a
// vertex sitting in the middle of another polygon's edge.
//
// Returns the number of points that were actually moved.
// ---------------------------------------------------------------------------
template <typename T>
auto snap_points_to_grid(Vector<Vec2<T>>& pts, T gridStep) -> U32 {
    if (gridStep <= T(0)) {
        return 0;
    }

    const T invStep = T(1) / gridStep;
    U32 count = 0;

    for (auto& pt : pts) {
        T sx = std::round(pt.x() * invStep) * gridStep;
        T sy = std::round(pt.y() * invStep) * gridStep;

        if (sx != pt.x() || sy != pt.y()) {
            ++count;
        }

        pt.set(sx, sy);
    }

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

// Collapses consecutive points in `coords` that are closer than `minDist`
// apart into their mean point. Comparison is done against the last *kept*
// (possibly already-merged) point, so a run of several close points in a
// row collapses progressively into one. Returns the number of points
// removed from `coords`.
template <typename T>
auto collapse_close_points_2d(Vector<Vec2<T>>& coords, T minDist) -> I32 {
    if (coords.size() < 2) {
        return 0;
    }

    Vector<Vec2<T>> merged;
    merged.reserve(coords.size());
    merged.push_back(coords[0]);

    for (U32 i = 1; i < coords.size(); ++i) {
        const Vec2<T>& prevPt = merged.back();
        const Vec2<T>& currPt = coords[i];

        T dist = (currPt - prevPt).length();
        if (dist < minDist) {
            // Replace the last kept point with the mean of the pair.
            merged.back() = (prevPt + currPt) * 0.5;
        } else {
            merged.push_back(currPt);
        }
    }

    I32 numRemoved = static_cast<I32>(coords.size() - merged.size());
    coords = std::move(merged);
    return numRemoved;
}

// File: nvk/geometry/PlaneFit.h

/** Result of a least-squares plane fit z = a*x + b*y + c. */
template <typename T> struct PlaneFitXY {
    T slopeX{0.0};  // a: dz/dx
    T slopeY{0.0};  // b: dz/dy
    T offsetZ{0.0}; // c: z at (x, y) == (0, 0)

    [[nodiscard]] auto eval(T x, T y) const -> T {
        return slopeX * x + slopeY * y + offsetZ;
    }

    [[nodiscard]] auto eval(const Vec2<T>& posXY) const -> T {
        return eval(posXY.x(), posXY.y());
    }

    /** Unit normal of the fitted plane, pointing "up" (+Z hemisphere). */
    [[nodiscard]] auto normal() const -> Vec3<T> {
        Vec3<T> n(-slopeX, -slopeY, 1.0);
        n.normalize();
        return n;
    }
};

// Computes the least-squares plane z = a*x + b*y + c fitting the elevation
// (Z) of the given 3D points, using their (X, Y) as the horizontal position.
//
// Returns false if the fit is degenerate (fewer than 3 points, or all points
// collinear/coincident in XY — a singular normal-equations matrix). In that
// case, fit.offsetZ is still set to the mean elevation of the input points
// (or 0 if points is empty), with zero slopes, so the result remains usable
// as a flat fallback.
template <typename T>
auto fit_plane_xy(const Vector<Vec3<T>>& points, PlaneFitXY<T>& fit) -> bool {
    fit = PlaneFitXY<T>{};

    if (points.empty()) {
        return false;
    }

    // Normal equations for z = a*x + b*y + c minimizing sum (z_i - (a*x_i +
    // b*y_i + c))^2:
    //   | sumXX sumXY sumX | |a|   |sumXZ|
    //   | sumXY sumYY sumY | |b| = |sumYZ|
    //   | sumX  sumY  n    | |c|   |sumZ |
    T sumX = 0.0;
    T sumY = 0.0;
    T sumXX = 0.0;
    T sumYY = 0.0;
    T sumXY = 0.0;
    T sumZ = 0.0;
    T sumXZ = 0.0;
    T sumYZ = 0.0;

    for (const auto& p : points) {
        const T x = p.x();
        const T y = p.y();
        const T z = p.z();

        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumYY += y * y;
        sumXY += x * y;
        sumZ += z;
        sumXZ += x * z;
        sumYZ += y * z;
    }

    const T n = static_cast<T>(points.size());

    const T m00 = sumXX;
    const T m01 = sumXY;
    const T m02 = sumX;
    const T m11 = sumYY;
    const T m12 = sumY;
    const T m22 = n;

    const T cof00 = m11 * m22 - m12 * m12;
    const T cof01 = m02 * m12 - m01 * m22;
    const T cof02 = m01 * m12 - m02 * m11;

    const T det = m00 * cof00 + m01 * cof01 + m02 * cof02;

    // Degenerate layout (fewer than 3 distinct points, or all points
    // collinear in XY): fall back to the mean elevation, zero slopes.
    if (std::abs(det) < 1e-12) {
        fit.offsetZ = sumZ / n;
        return false;
    }

    const T cof11 = m00 * m22 - m02 * m02;
    const T cof12 = m01 * m02 - m00 * m12;
    const T cof22 = m00 * m11 - m01 * m01;

    const T invDet = 1.0 / det;

    fit.slopeX = invDet * (cof00 * sumXZ + cof01 * sumYZ + cof02 * sumZ);
    fit.slopeY = invDet * (cof01 * sumXZ + cof11 * sumYZ + cof12 * sumZ);
    fit.offsetZ = invDet * (cof02 * sumXZ + cof12 * sumYZ + cof22 * sumZ);

    return true;
}

// ---------------------------------------------------------------------------
// fit_plane_xy_local
//
// Least-squares plane fit performed in a frame local to `origin`, then shifted
// back to world XY.
//
// The shift back is exact enough: z = a*(x-ox) + b*(y-oy) + c becomes
// z = a*x + b*y + (c - a*ox - b*oy), and evaluating that in F64 costs ~1e-11 cm
// of cancellation, which is irrelevant at cm scale.
// ---------------------------------------------------------------------------
template <typename T>
auto fit_plane_xy_local(const Vector<Vec3<T>>& points, const Vec2<T>& origin,
                        PlaneFitXY<T>& fit) -> bool {

    Vector<Vec3<T>> local;
    local.reserve(points.size());
    for (const auto& pt : points) {
        local.emplace_back(pt.x() - origin.x(), pt.y() - origin.y(), pt.z());
    }

    bool res = fit_plane_xy(local, fit);

    fit.offsetZ -= fit.slopeX * origin.x() + fit.slopeY * origin.y();
    return res;
}

// ---------------------------------------------------------------------------
// point_in_polygon
//
// Crossing number test for an arbitrary *simple* polygon (the section strips
// are strongly non convex, so point_in_convex() cannot be used here). The
// polygon is implicitly closed. Candidate for promotion to geom_utils.h.
// ---------------------------------------------------------------------------
template <typename T>
auto point_in_polygon(const Vector<Vec2<T>>& poly, const Vec2<T>& pt) -> bool {
    const U32 num = U32(poly.size());
    if (num < 3) {
        return false;
    }

    bool inside = false;
    for (U32 i = 0, j = num - 1; i < num; j = i++) {
        const auto& a = poly[i];
        const auto& b = poly[j];

        if ((a.y() > pt.y()) != (b.y() > pt.y())) {
            T xcross =
                a.x() + (pt.y() - a.y()) / (b.y() - a.y()) * (b.x() - a.x());
            if (pt.x() < xcross) {
                inside = !inside;
            }
        }
    }

    return inside;
}

// ---------------------------------------------------------------------------
// Span2
//
// Minimal "pair of endpoints" segment carrying no polyline identity. This is
// deliberately distinct from Segment2 (nvk/geometry/Segment2.h), which also
// tracks lineId/index/isLastLoopSeg and lives one layer above this header.
// Used as the output element of the clipping helpers below.
// ---------------------------------------------------------------------------
template <typename T> struct Span2 {
    Vec2<T> a;
    Vec2<T> b;

    [[nodiscard]] auto direction() const -> Vec2<T> { return b - a; }
    [[nodiscard]] auto length() const -> T { return (b - a).length(); }
    [[nodiscard]] auto midpoint() const -> Vec2<T> { return (a + b) * T(0.5); }
    [[nodiscard]] auto bounds() const -> Box2<T> { return {a, b}; }
};

using Span2f = Span2<F32>;
using Span2d = Span2<F64>;

template <typename T> using Span2Vector = Vector<Span2<T>>;

// ---------------------------------------------------------------------------
// seg2_clip_by_polygon
//
// Clips the segment segA->segB against an arbitrary *simple* polygon (given as
// an implicitly-closed ring of coords, any winding) and returns the parts of
// the segment that survive the requested selection, in increasing order along
// the segment. Adjacent kept parts are merged, so a segment that never changes
// classification comes back as a single span.
//
// Every sub-part of the segment falls into exactly one of three classes:
//   - strictly inside the polygon,
//   - strictly outside the polygon,
//   - on the boundary (i.e. running collinear with one of the polygon edges).
//
// keepInside selects between the first two:
//   false -> keep the outside parts  (the usual "punch a hole in the segment")
//   true  -> keep the inside parts
//
// keepBoundaryParts independently decides whether collinear-with-an-edge parts
// are emitted, and is what makes the "strictly inside" distinction explicit:
//   keepInside=false, keepBoundaryParts=true  -> only *strictly interior* parts
//                                                are removed
//   keepInside=false, keepBoundaryParts=false -> boundary counts as inside and
//                                                is removed too
// Calling the function twice with complementary settings (outside+boundary and
// inside without boundary, or the reverse) yields an exact partition of the
// input segment.
//
// eps is a world-space tolerance (same units as the input coordinates): it
// drives the collinearity test, the on-boundary test, degenerate-edge culling
// and the rejection of zero-length output spans.
//
// Complexity is O(numEdges * numCuts): every candidate sub-interval is
// classified by testing its midpoint, which sidesteps all the enter/exit parity
// bookkeeping that breaks down on vertex-grazing and collinear cases.
// ---------------------------------------------------------------------------
template <typename T>
auto seg2_clip_by_polygon(const Vec2<T>& segA, const Vec2<T>& segB,
                          const Vector<Vec2<T>>& poly, bool keepInside,
                          bool keepBoundaryParts = false, T eps = T(1e-6))
    -> Span2Vector<T> {
    Span2Vector<T> result;

    const auto numPts = U32(poly.size());

    // Degenerate polygon: no interior at all, so everything is outside.
    if (numPts < 3) {
        if (!keepInside) {
            result.push_back({segA, segB});
        }
        return result;
    }

    // True when pt sits within eps of the polygon boundary.
    auto on_boundary = [&](const Vec2<T>& pt) -> bool {
        for (U32 i = 0, j = numPts - 1; i < numPts; j = i++) {
            if (seg2_point_distance(poly[j], poly[i], pt, true) <= eps) {
                return true;
            }
        }
        return false;
    };

    // Single classification point for the whole function.
    auto should_keep = [&](const Vec2<T>& pt) -> bool {
        if (on_boundary(pt)) {
            return keepBoundaryParts;
        }
        return point_in_polygon(poly, pt) == keepInside;
    };

    const Vec2<T> segDir = segB - segA;
    const T segLen = segDir.length();

    // Degenerate segment: classify it as a single point.
    if (segLen <= eps) {
        if (should_keep(segA)) {
            result.push_back({segA, segB});
        }
        return result;
    }

    const T invLenSq = T(1) / segDir.dot(segDir);
    const T tEps = eps / segLen;

    // -- 1. Collect every parameter at which the classification can change.
    Vector<T> cuts;
    cuts.reserve(numPts + 2);
    cuts.push_back(T(0));
    cuts.push_back(T(1));

    for (U32 i = 0, j = numPts - 1; i < numPts; j = i++) {
        const Vec2<T>& p0 = poly[j];
        const Vec2<T>& p1 = poly[i];

        const Vec2<T> edgeDir = p1 - p0;
        const T edgeLen = edgeDir.length();
        if (edgeLen <= eps) {
            continue; // degenerate edge (e.g. explicit closing vertex)
        }

        const Vec2<T> rel = p0 - segA;
        const T det = segDir.cross(edgeDir);

        // Relative parallelism test: |det| == |segDir| * |edgeDir| * sin(angle)
        if (std::abs(det) > T(1e-9) * segLen * edgeLen) {
            // Proper crossing: solve segA + t * segDir == p0 + u * edgeDir
            const T t = rel.cross(edgeDir) / det;
            const T u = rel.cross(segDir) / det;
            const T uEps = eps / edgeLen;

            // t == 0 and t == 1 are already in the cut list; u is widened by
            // uEps so a segment passing exactly through a polygon vertex still
            // produces its cut.
            if (t > tEps && t < T(1) - tEps && u > -uEps && u < T(1) + uEps) {
                cuts.push_back(t);
            }
            continue;
        }

        // Parallel edge: only relevant when actually collinear with the segment
        // line. |rel.cross(segDir)| == perpDistance * segLen.
        if (std::abs(rel.cross(segDir)) > eps * segLen) {
            continue;
        }

        // Collinear overlap: cut where the edge endpoints project onto us.
        const T t0 = (p0 - segA).dot(segDir) * invLenSq;
        const T t1 = (p1 - segA).dot(segDir) * invLenSq;
        for (T tv : {t0, t1}) {
            if (tv > tEps && tv < T(1) - tEps) {
                cuts.push_back(tv);
            }
        }
    }

    std::sort(cuts.begin(), cuts.end());

    // -- 2. Classify each sub-interval by its midpoint, keeping the selected
    //       ones and merging contiguous runs.
    bool hasPending = false;
    T pendingStart = T(0);
    T pendingEnd = T(0);

    auto flush = [&]() {
        if (!hasPending) {
            return;
        }
        // Reuse the original endpoints verbatim when the span reaches them, so
        // an unclipped segment round-trips bit-exactly.
        const Vec2<T> pa =
            pendingStart <= tEps ? segA : segA + segDir * pendingStart;
        const Vec2<T> pb =
            pendingEnd >= T(1) - tEps ? segB : segA + segDir * pendingEnd;
        result.push_back({pa, pb});
        hasPending = false;
    };

    for (U32 i = 0; i + 1 < U32(cuts.size()); ++i) {
        const T t0 = cuts[i];
        const T t1 = cuts[i + 1];
        if (t1 - t0 <= tEps) {
            continue; // duplicate cut / zero-length slice
        }

        // A midpoint can only land on the boundary when the whole slice runs
        // along an edge: any transversal touch would itself be a cut and would
        // have split this interval further.
        const Vec2<T> mid = segA + segDir * ((t0 + t1) * T(0.5));

        if (!should_keep(mid)) {
            flush();
            continue;
        }

        if (hasPending && std::abs(t0 - pendingEnd) <= tEps) {
            pendingEnd = t1;
        } else {
            flush();
            pendingStart = t0;
            pendingEnd = t1;
            hasPending = true;
        }
    }

    flush();

    return result;
}

// Convenience wrappers around seg2_clip_by_polygon.

template <typename T>
auto seg2_clip_outside_polygon(const Vec2<T>& segA, const Vec2<T>& segB,
                               const Vector<Vec2<T>>& poly,
                               bool keepBoundaryParts = false, T eps = T(1e-6))
    -> Span2Vector<T> {
    return seg2_clip_by_polygon(segA, segB, poly, /*keepInside=*/false,
                                keepBoundaryParts, eps);
}

template <typename T>
auto seg2_clip_inside_polygon(const Vec2<T>& segA, const Vec2<T>& segB,
                              const Vector<Vec2<T>>& poly,
                              bool keepBoundaryParts = true, T eps = T(1e-6))
    -> Span2Vector<T> {
    return seg2_clip_by_polygon(segA, segB, poly, /*keepInside=*/true,
                                keepBoundaryParts, eps);
}

} // namespace nv

#endif
