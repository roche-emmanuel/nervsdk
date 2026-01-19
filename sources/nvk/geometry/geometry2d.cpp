#include <external/RTree.h>
#include <nvk/geometry/geometry2d.h>

namespace nv {

// Internal helper elements:
namespace {

template <typename T> struct Seg2TreeData {
    Segment2Vector<T> segments;
    using SegmentTree = RTree<const Segment2<T>*, T, 2>;
    SegmentTree tree;
};

template <typename T>
auto build_seg2_tree_data(const Polyline2Vector<T>& paths) -> Seg2TreeData<T> {
    Seg2TreeData<T> result;

    for (const auto& path : paths) {
        int n = (int)path.points.size();
        if (n < 2)
            continue;

        for (int i = 0; i < n - 1; ++i) {
            Segment2<T> s;
            s.a = path.points[i];
            s.b = path.points[i + 1];
            s.lineId = path.id;
            s.index = i;

            result.segments.push_back(s);
        }

        if (path.closedLoop) {
            Segment2<T> s;
            s.a = path.points[n - 1];
            s.b = path.points[0];
            s.lineId = path.id;
            s.index = n - 1;
            s.isLastLoopSeg = true;
            result.segments.push_back(s);
        }
    }

    // Insert into R-tree
    for (const auto& s : result.segments) {
        auto bb = s.bounds();
        result.tree.Insert(bb.minimum().ptr(), bb.maximum().ptr(), &s);
    }

    return result;
}

template <typename T>
auto findSegmentIntersections(const Seg2TreeData<T>& tdata)
    -> Segment2IntersectionVector<T> {
    Segment2IntersectionVector<T> result;

    for (const auto& s : tdata.segments) {

        auto bb = s.bounds();

        tdata.tree.Search(bb.minimum().ptr(), bb.maximum().ptr(),
                          [&](const Segment2<T>* other) {
                              if (&s >= other)
                                  return true; // avoid duplicates

                              // Skip neighbors in same line
                              if (s.lineId == other->lineId &&
                                  (std::abs(s.index - other->index) <= 1 ||
                                   (other->isLastLoopSeg && s.index == 0) ||
                                   (s.isLastLoopSeg && other->index == 0)))
                                  return true;

                              Vec2<T> ip;
                              if (s.intersect(*other, ip)) {
                                  result.push_back(
                                      {.position = ip, .s0 = s, .s1 = *other});
                              }

                              return true;
                          });
    }

    return result;
}

#if 0
template <typename T>
auto findEndpointNearSegments(const Polyline2Vector<T>& paths,
                              const Seg2TreeData<T>& tdata, T distance)
    -> EndpointNearSegment2Vector<T> {
    EndpointNearSegment2Vector<T> result;

    auto checkPoint = [&](const Vec2<T>& p, int lineId, bool isStart) {
        T min[2] = {p.x() - distance, p.y() - distance};
        T max[2] = {p.x() + distance, p.y() + distance};

        tdata.tree.Search(min, max, [&](const Segment2<T>* s) {
            // Skip same path endpoint segment
            if (s->lineId == lineId)
                return true;

            T t = 0.0;
            T d = s->point_distance(p, false, &t);
            if (d <= distance && 0 <= t && t <= 1.0) {
                result.push_back({.endpoint = p,
                                  .pathId = lineId,
                                  .isStart = isStart,
                                  .segment = *s,
                                  .distance = d});
            }
            return true;
        });
    };

    for (const auto& path : paths) {
        if (path.points.empty())
            continue;

        checkPoint(path.points.front(), path.id, true);

        if (!path.closedLoop)
            checkPoint(path.points.back(), path.id, false);
    }

    return result;
}
#else
template <typename T>
auto findEndpointNearSegments(const Polyline2Vector<T>& paths,
                              const Seg2TreeData<T>& tdata, T maxDistance)
    -> EndpointNearSegment2Vector<T> {
    EndpointNearSegment2Vector<T> result;

    auto checkPoint = [&](const Vec2<T>& p, int lineId, bool isStart) {
        auto& path = paths[lineId];
        auto npoints = path.points.size();
        if (npoints <= 1)
            return;

        Vec2<T> p0 = path.points[isStart ? 0 : npoints - 1];
        Vec2<T> p1 = path.points[isStart ? 1 : npoints - 2];

        Vec2<T> dir = (p0 - p1).normalized(); // direction away from path
        Vec2<T> rayEnd = p0 + dir * maxDistance;

        auto small_epsilon = 0.01;

        auto bb = Box2<T>(p0, rayEnd);
        bb.expand(small_epsilon);

        tdata.tree.Search(
            bb.minimum().ptr(), bb.maximum().ptr(),
            [&](const Segment2<T>* other) {
                Vec2<T> ip;
                if (seg2_intersect(p0, rayEnd, other->a, other->b, ip)) {
                    T dist = (ip - p0).length();
                    if (dist <= maxDistance && dist > small_epsilon) {
                        result.push_back({.endpoint = p0,
                                          .intersection = ip,
                                          .pathId = path.id,
                                          .isStart = isStart,
                                          .segment = *other,
                                          .distance = dist});
                    }
                }

                return true;
            });
    };

    for (const auto& path : paths) {
        if (path.points.empty())
            continue;

        checkPoint(path.points.front(), path.id, true);

        if (!path.closedLoop)
            checkPoint(path.points.back(), path.id, false);
    }

    return result;
}
#endif

// template <typename T>
// auto findEndpointExtensionIntersections(const Polyline2Vector<T>& paths,
//                                         const Seg2TreeData<T>& tdata,
//                                         T maxDistance)
//     -> EndpointExtensionIntersectionVector<T> {

//     EndpointExtensionIntersectionVector<T> result;

//     for (const auto& path : paths) {
//         if (path.closedLoop || path.points.size() < 2)
//             continue;

//         // Check start endpoint
//         {
//             Vec2<T> p0 = path.points[0];
//             Vec2<T> p1 = path.points[1];
//             Vec2<T> dir = (p0 - p1).normalized(); // direction away from path
//             Vec2<T> rayEnd = p0 + dir * maxDistance;

//             // Search area around the ray
//             auto bb = Box2<T>(p0, rayEnd).expanded(small_epsilon);

//             tdata.tree.Search(
//                 bb.minimum().ptr(), bb.maximum().ptr(),
//                 [&](const Segment2<T>* s) {
//                     if (s->lineId == path.id)
//                         return true; // skip same path

//                     Vec2<T> ip;
//                     if (seg2_intersect(p0, rayEnd, s->a, s->b, ip)) {
//                         T dist = (ip - p0).length();
//                         if (dist <= maxDistance && dist > small_epsilon) {
//                             result.push_back({.position = ip,
//                                               .pathId = path.id,
//                                               .isStart = true,
//                                               .segment = *s,
//                                               .distance = dist});
//                         }
//                     }
//                     return true;
//                 });
//         }

//         // Check end endpoint (similar logic)
//         {
//             int n = path.points.size();
//             Vec2<T> pn = path.points[n - 1];
//             Vec2<T> pn1 = path.points[n - 2];
//             Vec2<T> dir = (pn - pn1).normalized();
//             Vec2<T> rayEnd = pn + dir * maxDistance;

//             // ... similar search logic
//         }
//     }

//     return result;
// }

} // namespace

auto compute_polyline2_intersections(const Polyline2Vector<F32>& paths,
                                     float endpointDistance)
    -> Polyline2IntersectionResults<F32> {
    auto tdata = build_seg2_tree_data<F32>(paths);

    Polyline2IntersectionResults<F32> out;
    out.intersections = findSegmentIntersections(tdata);

    if (endpointDistance > 0.0) {
        out.endpointNearSegments =
            findEndpointNearSegments(paths, tdata, endpointDistance);
    }

    return out;
}

auto compute_polyline2_intersections(const Polyline2Vector<F64>& paths,
                                     F64 endpointDistance)
    -> Polyline2IntersectionResults<F64> {
    auto tdata = build_seg2_tree_data<F64>(paths);

    Polyline2IntersectionResults<F64> out;
    out.intersections = findSegmentIntersections(tdata);

    if (endpointDistance > 0.0) {
        out.endpointNearSegments =
            findEndpointNearSegments(paths, tdata, endpointDistance);
    }

    return out;
}

}; // namespace nv
