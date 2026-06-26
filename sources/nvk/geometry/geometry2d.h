#ifndef _NV_GEOMETRY2_H_
#define _NV_GEOMETRY2_H_

#include <nvk_common.h>

#include <external/earcut.hpp>

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
    I32 id{-1};
    Vector<Vec2<T>> points;
    bool closedLoop{false};
};

template <typename T> using Polyline2Vector = Vector<Polyline2<T>>;

using Polyline2f = Polyline2<F32>;
using Polyline2d = Polyline2<F64>;

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

template <typename T>
auto polyline_ray_intersections(const Vec2<T>& origin, const Vec2<T>& dir,
                                const Polyline2<T>& polyline) {
    return polyline_ray_intersections(origin, dir, polyline.points,
                                      polyline.closedLoop);
}

// ---------------------------------------------------------------------------
// Polygon2 / Polygon2f / Polygon2d
// ---------------------------------------------------------------------------

template <typename T> struct Polygon2 {
    Vector<Vec2<T>> coords;

    [[nodiscard]] auto size() const -> size_t { return coords.size(); }
};

using Polygon2f = Polygon2<F32>;
using Polygon2d = Polygon2<F64>;

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

auto inflate_polyline2(const Polyline2f& centerLine, F32 offset,
                       I32 joinType = PATH_JOIN_ROUND,
                       I32 endType = PATH_END_ROUND) -> Vector<Polygon2f>;

auto inflate_polyline2(const Polyline2d& centerLine, F64 offset,
                       I32 joinType = PATH_JOIN_ROUND,
                       I32 endType = PATH_END_ROUND) -> Vector<Polygon2d>;

// ---------------------------------------------------------------------------
// Region2 — a planar region: one outer boundary ring + zero or more holes.
//
// Convention (matches Clipper2 / GeoJSON / earcut expectations):
//   outer : CCW winding  (positive area)
//   holes : CW  winding  (each hole cuts out of the outer)
//
// inflate_polyline2() returns Vector<Polygon2f>; use Region2f::from_rings()
// to interpret the first ring as outer and the rest as holes.
// ---------------------------------------------------------------------------
template <typename T> struct Region2 {
    Polygon2<T> outer;
    Vector<Polygon2<T>> holes;

    [[nodiscard]] auto empty() const -> bool { return outer.coords.size() < 3; }

    [[nodiscard]] auto get_num_points() const -> U32 {
        U32 count = outer.size();
        for (const auto& h : holes) {
            count += h.size();
        }
        return count;
    }

    // Interpret the Vector<Polygon2> returned by inflate_polyline2 as a
    // Region2: first ring = outer, subsequent rings = holes.
    static auto from_rings(const Vector<Polygon2<T>>& rings) -> Region2<T> {
        Region2<T> result;
        if (rings.empty())
            return result;
        result.outer = rings[0];
        for (U32 i = 1; i < U32(rings.size()); ++i)
            result.holes.push_back(rings[i]);
        return result;
    }

    // Triangulate using mapbox/earcut.
    //
    // Returns a flat index buffer into the region's concatenated vertex array:
    //   [ outer.coords ... hole[0].coords ... hole[1].coords ... ]
    // i.e. the same layout you'd build when constructing 3-D mesh vertices,
    // so indices are usable directly with no offset arithmetic.
    //
    // Returns empty if the outer ring has fewer than 3 vertices.
    [[nodiscard]] auto triangulate() const -> Vector<U32> {
        if (outer.coords.size() < 3)
            return {};

        using EarcutPoint = std::array<T, 2>;
        std::vector<std::vector<EarcutPoint>> polygon;

        auto& outerRing = polygon.emplace_back();
        outerRing.reserve(outer.coords.size());
        for (const auto& pt : outer.coords)
            outerRing.push_back({pt.x(), pt.y()});

        for (const auto& hole : holes) {
            if (hole.coords.size() < 3)
                continue;
            auto& holeRing = polygon.emplace_back();
            holeRing.reserve(hole.coords.size());
            for (const auto& pt : hole.coords)
                holeRing.push_back({pt.x(), pt.y()});
        }

        return mapbox::earcut<U32>(polygon);
    }

    [[nodiscard]] auto get_point_position(U32 globalIdx) const -> Vec2<T> {
        // Resolve global index → (ring coords, local index)
        const Vector<Vec2<T>>* ring = nullptr;
        U32 localIdx = globalIdx;

        const U32 outerN = U32(outer.coords.size());
        if (localIdx < outerN) {
            ring = &outer.coords;
        } else {
            localIdx -= outerN;
            for (const auto& hole : holes) {
                const U32 holeN = U32(hole.coords.size());
                if (localIdx < holeN) {
                    ring = &hole.coords;
                    break;
                }
                localIdx -= holeN;
            }
        }

        NVCHK(ring != nullptr,
              "Region2::get_point: global index {} out of range "
              "(get_num_points={})",
              globalIdx, get_num_points());

        return (*ring)[localIdx];
    }

    // Returns the polygon boundary tangent at global vertex index i.
    //
    // Global index layout (same as triangulate()):
    //   [0 .. outerN-1]                          outer ring
    //   [outerN .. outerN+hole0N-1]               hole 0
    //   [outerN+hole0N .. outerN+hole0N+hole1N-1] hole 1
    //   ...
    //
    // The tangent is the mean of the two edge directions meeting at vertex i,
    // then normalised:
    //   prev_edge = normalise(pos[i]   - pos[i-1])
    //   next_edge = normalise(pos[i+1] - pos[i])
    //   tangent   = normalise(prev_edge + next_edge)
    //
    // Rings are treated as implicitly closed (open storage: coords[n-1] is
    // followed by coords[0]). No duplicate closing vertex is assumed.
    //
    // If the two edges are anti-parallel (180° reversal — degenerate spike),
    // the sum is near-zero; in that case the next-edge direction is returned
    // as a fallback.
    [[nodiscard]] auto get_point_tangent(U32 globalIdx) const -> Vec2<T> {
        // Resolve global index → (ring coords, local index)
        const Vector<Vec2<T>>* ring = nullptr;
        U32 localIdx = globalIdx;

        const U32 outerN = U32(outer.coords.size());
        if (localIdx < outerN) {
            ring = &outer.coords;
        } else {
            localIdx -= outerN;
            for (const auto& hole : holes) {
                const U32 holeN = U32(hole.coords.size());
                if (localIdx < holeN) {
                    ring = &hole.coords;
                    break;
                }
                localIdx -= holeN;
            }
        }

        NVCHK(ring != nullptr,
              "Region2::get_point_tangent: global index {} out of range "
              "(get_num_points={})",
              globalIdx, get_num_points());

        const U32 n = U32(ring->size());
        NVCHK(n >= 2,
              "Region2::get_point_tangent: ring has fewer than 2 vertices");

        // Wrap-around neighbours (ring is implicitly closed, no dup vertex)
        const U32 iPrev = (localIdx + n - 1) % n;
        const U32 iNext = (localIdx + 1) % n;

        const Vec2<T>& pPrev = (*ring)[iPrev];
        const Vec2<T>& pCurr = (*ring)[localIdx];
        const Vec2<T>& pNext = (*ring)[iNext];

        // Normalise each edge direction individually before averaging so that
        // very short edges don't dominate long ones.
        auto safeNorm = [](Vec2<T> v) -> Vec2<T> {
            const T len = v.length();
            return (len > T(1e-12)) ? v / len : Vec2<T>(T(0), T(0));
        };

        const Vec2<T> prevEdge = safeNorm(pCurr - pPrev);
        const Vec2<T> nextEdge = safeNorm(pNext - pCurr);

        Vec2<T> sum = prevEdge + nextEdge;
        const T sumLen = sum.length();

        // Anti-parallel edges (spike / 180° reversal): fall back to next edge
        if (sumLen < T(1e-9))
            return nextEdge.normalized();

        return sum.normalized();
    }

    [[nodiscard]] auto get_point_normal(U32 globalIdx) const -> Vec2<T> {
        auto tang = get_point_tangent(globalIdx);
        return tang.ccw90();
    }
};

using Region2f = Region2<F32>;
using Region2d = Region2<F64>;

}; // namespace nv

#endif
