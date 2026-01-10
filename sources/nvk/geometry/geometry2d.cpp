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
auto build_seg2_tree_data(const Polyline2Vector<F32>& paths)
    -> Seg2TreeData<T> {
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
            result.segments.push_back(s);
        }
    }

    // Insert into R-tree
    for (const auto& s : result.segments) {
        auto bb = s.bounds();
        result.tree.Insert(bb.bottom_left().ptr(), bb.top_right().ptr(), &s);
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
                                  std::abs(s.index - other->index) <= 1)
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

            F32 d = s->point_distance(p);
            if (d <= distance) {
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

}; // namespace nv
