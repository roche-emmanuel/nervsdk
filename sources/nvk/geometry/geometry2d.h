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

template <typename T>
auto seg2_point_distance(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& pt)
    -> T {
    Vec2<T> ab = b - a;
    T ab_len_sq = ab.dot(ab);

    // Handle degenerate case: segment is a point
    if (ab_len_sq < T(1e-20)) {
        return (pt - a).length();
    }

    T t = (pt - a).dot(ab) / ab_len_sq;
    t = std::clamp(t, T(0.0), T(1.0));
    Vec2<T> proj = a + ab * t;
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

    auto point_distance(const Vec2<T>& pt) const -> T {
        return seg2_point_distance(a, b, pt);
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
