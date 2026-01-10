#include <external/RTree.h>
#include <nvk/geometry/geometry2d.h>

namespace nv {

auto seg2d_point_distance(const Vec2f& a, const Vec2f& b, const Vec2f& pt)
    -> F32 {
    Vec2f ab = b - a;
    F32 t = (pt - a).dot(ab) / ab.dot(ab);
    t = std::clamp(t, 0.0F, 1.0F);
    Vec2f proj = a + ab * t;
    return (pt - proj).length();
}

using SegmentTreef = RTree<const Segment2f*, F32, 2>;

struct IndexedSegments {
    Vector<Segment2f> segments;
    SegmentTreef tree;
};

static auto build_index(const Polyline2Vector<F32>& paths) -> IndexedSegments {
    IndexedSegments result;

    for (const auto& path : paths) {
        int n = (int)path.points.size();
        if (n < 2)
            continue;

        for (int i = 0; i < n - 1; ++i) {
            Segment2f s;
            s.a = path.points[i];
            s.b = path.points[i + 1];
            s.lineId = path.id;
            s.index = i;

            result.segments.push_back(s);
        }

        if (path.closedLoop) {
            Segment2f s;
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

static auto findSegmentIntersections(const IndexedSegments& index)
    -> Segment2IntersectionVector<F32> {
    Segment2IntersectionVector<F32> result;

    for (const Segment2f& s : index.segments) {

        auto bb = s.bounds();

        index.tree.Search(bb.minimum().ptr(), bb.maximum().ptr(),
                          [&](const Segment2f* other) {
                              if (&s >= other)
                                  return true; // avoid duplicates

                              // Skip neighbors in same line
                              if (s.lineId == other->lineId &&
                                  std::abs(s.index - other->index) <= 1)
                                  return true;

                              Vec2f ip;
                              if (s.intersect(*other, ip)) {
                                  result.push_back(
                                      {.position = ip, .s0 = s, .s1 = *other});
                              }

                              return true;
                          });
    }

    return result;
}

auto findEndpointNearSegments(const Polyline2Vector<F32>& paths,
                              const IndexedSegments& index, F32 distance)
    -> EndpointNearSegment2Vector<F32> {
    EndpointNearSegment2Vector<F32> result;

    auto checkPoint = [&](const Vec2f& p, int lineId, bool isStart) {
        F32 min[2] = {p.x() - distance, p.y() - distance};
        F32 max[2] = {p.x() + distance, p.y() + distance};

        index.tree.Search(min, max, [&](const Segment2f* s) {
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

auto compute_polyline2_intersections(const Polyline2Vector<F32>& paths,
                                     float endpointDistance)
    -> Polyline2IntersectionResults<F32> {
    auto index = build_index(paths);

    Polyline2IntersectionResults<F32> out;
    out.intersections = findSegmentIntersections(index);

    if (endpointDistance > 0.0) {
        out.endpointNearSegments =
            findEndpointNearSegments(paths, index, endpointDistance);
    }

    return out;
}

}; // namespace nv
