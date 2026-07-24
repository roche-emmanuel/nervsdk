#ifndef _SEGMENT2_H_
#define _SEGMENT2_H_

#include <nvk/geometry/geom_utils.h>

namespace nv {

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

} // namespace nv

#endif
