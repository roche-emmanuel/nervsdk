#ifndef _NV_GEOMETRY2_H_
#define _NV_GEOMETRY2_H_

#include <nvk_common.h>

namespace nv {

template <typename T> struct Polyline2 {
    I32 id;
    Vector<Vec2<T>> points;
    bool closedLoop;
};

template <typename T> using Polyline2Vector = Vector<Polyline2<T>>;

auto seg2_intersect(const Vec2f& seg0_a, const Vec2f& seg0_b,
                    const Vec2f& seg1_a, const Vec2f& seg1_b, Vec2f& intersecPt)
    -> bool;

auto seg2_point_distance(const Vec2f& a, const Vec2f& b, const Vec2f& pt)
    -> F32;

template <typename T> struct Segment2 {
    Vec2<T> a;
    Vec2<T> b;

    int lineId;
    int index; // index in the polyline

    auto bounds() const -> Box2<T> { return {a, b}; }

    auto intersect(const Segment2& seg, Vec2<T>& intersectPt) const -> bool {
        return seg2_intersect(a, b, seg.a, seg.b, intersectPt);
    };

    auto point_distance(const Vec2<T>& pt) const -> T {
        return seg2_point_distance(a, b, pt);
    }
};

using Segment2f = Segment2<F32>;
using Segment2d = Segment2<F64>;

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
                                     float endpointDistance)
    -> Polyline2IntersectionResults<F32>;

}; // namespace nv

#endif
