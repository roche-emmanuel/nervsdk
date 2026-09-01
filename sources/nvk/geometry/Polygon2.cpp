#include <nvk/geometry/geometry2d.h>

#include <clipper2/clipper.h>

namespace nv {

namespace {
auto polygon2_to_path(const Polygon2d& poly) -> Clipper2Lib::PathD {
    Clipper2Lib::PathD path;
    path.reserve(poly.coords.size());
    for (const auto& pt : poly.coords) {
        path.emplace_back(pt.x(), pt.y());
    }
    return path;
}

auto polygon2_vector_to_paths(const Vector<Polygon2d>& polys)
    -> Clipper2Lib::PathsD {
    Clipper2Lib::PathsD paths;
    paths.reserve(polys.size());
    for (const auto& poly : polys) {
        paths.push_back(polygon2_to_path(poly));
    }
    return paths;
}

auto paths_to_polygon2_vector(const Clipper2Lib::PathsD& paths)
    -> Vector<Polygon2d> {
    Vector<Polygon2d> result;
    result.reserve(paths.size());
    for (const auto& ring : paths) {
        if (ring.size() < 3) {
            continue;
        }
        Polygon2d poly;
        poly.coords.reserve(ring.size());
        for (const auto& pt : ring) {
            poly.coords.emplace_back(pt.x, pt.y);
        }
        result.push_back(std::move(poly));
    }
    return result;
}
} // namespace

auto polygon2_union(const Vector<Polygon2d>& inputs, I32 fillRule)
    -> Vector<Polygon2d> {
    Clipper2Lib::PathsD subjectPaths = polygon2_vector_to_paths(inputs);
    Clipper2Lib::PathsD solution =
        Clipper2Lib::Union(subjectPaths, (Clipper2Lib::FillRule)fillRule);
    return paths_to_polygon2_vector(solution);
}

auto polygon2_union_inflated(const Vector<Polygon2d>& inputs, F64 offset,
                             I32 fillRule, I32 joinType) -> Vector<Polygon2d> {
    Clipper2Lib::PathsD subjectPaths = polygon2_vector_to_paths(inputs);
    auto jType = (Clipper2Lib::JoinType)joinType;
    auto eType = Clipper2Lib::EndType::Polygon;
    F64 miterLimit = 2.0;

    // Grow, merge, shrink. All 3 stages run on the *whole* set: shrinking ring
    // by ring would offset a hole as if it were an island, and would leave the
    // self intersections the erosion creates unresolved.
    auto grown = Clipper2Lib::InflatePaths(subjectPaths, offset, jType, eType,
                                           miterLimit);

    auto merged = Clipper2Lib::Union(grown, (Clipper2Lib::FillRule)fillRule);

    auto shrunk =
        Clipper2Lib::InflatePaths(merged, -offset, jType, eType, miterLimit);

    return paths_to_polygon2_vector(shrunk);
}

auto polygon2_offset(const Polygon2d& poly, F64 offset, I32 joinType)
    -> Vector<Polygon2d> {
    if (poly.coords.size() < 3) {
        return {};
    }

    Clipper2Lib::PathsD subjectPaths{polygon2_to_path(poly)};
    auto jType = (Clipper2Lib::JoinType)joinType;
    F64 miterLimit = 2.0;

    Clipper2Lib::PathsD solution = Clipper2Lib::InflatePaths(
        subjectPaths, offset, jType, Clipper2Lib::EndType::Polygon, miterLimit);

    return paths_to_polygon2_vector(solution);
}

auto polygon2_smooth_chaikin(const Polygon2d& poly, U32 iterations,
                             F64 cutRatio) -> Polygon2d {
    if (iterations == 0 || poly.coords.size() < 3) {
        return poly;
    }

    F64 ratio = std::clamp(cutRatio, 0.0, 0.5);

    Polygon2d current = poly;

    for (U32 it = 0; it < iterations; ++it) {
        const U32 n = U32(current.coords.size());
        if (n < 3) {
            break;
        }

        Polygon2d next;
        next.coords.reserve(size_t(n) * 2);

        for (U32 i = 0; i < n; ++i) {
            const Vec2d& p0 = current.coords[i];
            const Vec2d& p1 = current.coords[(i + 1) % n];

            next.coords.push_back(p0 + (p1 - p0) * ratio);
            next.coords.push_back(p0 + (p1 - p0) * (1.0 - ratio));
        }

        current = std::move(next);
    }

    return current;
}

auto polygon2_difference(const Vector<Polygon2d>& subjects,
                         const Vector<Polygon2d>& clips, I32 fillRule)
    -> Vector<Polygon2d> {
    Clipper2Lib::PathsD subjectPaths = polygon2_vector_to_paths(subjects);
    Clipper2Lib::PathsD clipPaths = polygon2_vector_to_paths(clips);
    Clipper2Lib::PathsD solution = Clipper2Lib::Difference(
        subjectPaths, clipPaths, (Clipper2Lib::FillRule)fillRule);
    return paths_to_polygon2_vector(solution);
}
auto polygon2_intersection(const Vector<Polygon2d>& subjects,
                           const Vector<Polygon2d>& clips, I32 fillRule)
    -> Vector<Polygon2d> {
    Clipper2Lib::PathsD subjectPaths = polygon2_vector_to_paths(subjects);
    Clipper2Lib::PathsD clipPaths = polygon2_vector_to_paths(clips);
    Clipper2Lib::PathsD solution = Clipper2Lib::Intersect(
        subjectPaths, clipPaths, (Clipper2Lib::FillRule)fillRule);
    return paths_to_polygon2_vector(solution);
}
auto polygon2_xor(const Vector<Polygon2d>& subjects,
                  const Vector<Polygon2d>& clips, I32 fillRule)
    -> Vector<Polygon2d> {
    Clipper2Lib::PathsD subjectPaths = polygon2_vector_to_paths(subjects);
    Clipper2Lib::PathsD clipPaths = polygon2_vector_to_paths(clips);
    Clipper2Lib::PathsD solution = Clipper2Lib::Xor(
        subjectPaths, clipPaths, (Clipper2Lib::FillRule)fillRule);
    return paths_to_polygon2_vector(solution);
}
auto polygon2_area(const Polygon2d& poly) -> F64 {
    const auto& coords = poly.coords;
    if (coords.size() < 3) {
        return 0.0;
    }
    F64 area = 0.0;
    const size_t n = coords.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& p0 = coords[i];
        const auto& p1 = coords[(i + 1) % n];
        area += p0.x() * p1.y() - p1.x() * p0.y();
    }
    return area * 0.5;
}
auto polygon2_is_positive_orientation(const Polygon2d& poly) -> bool {
    return polygon2_area(poly) > 0.0;
}

auto inflate_polyline2(const Polyline2f& centerLine, F32 offset, I32 joinType,
                       I32 endType) -> Vector<Polygon2f> {

    Clipper2Lib::PathD cPath;
    cPath.reserve(centerLine.points.size());
    for (const auto& pt : centerLine.points) {
        cPath.emplace_back(pt.x(), pt.y());
    }

    Clipper2Lib::PathsD solution = Clipper2Lib::InflatePaths(
        {cPath}, offset, (Clipper2Lib::JoinType)joinType,
        (Clipper2Lib::EndType)endType);

    Vector<Polygon2f> result;
    result.reserve(solution.size());
    for (const auto& ring : solution) {
        if (ring.size() < 3) {
            continue;
        }
        Polygon2f poly;
        poly.coords.reserve(ring.size());
        for (const auto& pt : ring) {
            poly.coords.emplace_back(F32(pt.x), F32(pt.y));
        }
        result.push_back(std::move(poly));
    }
    return result;
};

auto inflate_polyline2(const Polyline2d& centerLine, F64 offset, I32 joinType,
                       I32 endType) -> Vector<Polygon2d> {

    Clipper2Lib::PathD cPath;
    cPath.reserve(centerLine.points.size());
    for (const auto& pt : centerLine.points) {
        cPath.emplace_back(pt.x(), pt.y());
    }

    Clipper2Lib::PathsD solution = Clipper2Lib::InflatePaths(
        {cPath}, offset, (Clipper2Lib::JoinType)joinType,
        (Clipper2Lib::EndType)endType);

    Vector<Polygon2d> result;
    result.reserve(solution.size());
    for (const auto& ring : solution) {
        if (ring.size() < 3) {
            continue;
        }
        Polygon2d poly;
        poly.coords.reserve(ring.size());
        for (const auto& pt : ring) {
            poly.coords.emplace_back(pt.x, pt.y);
        }
        result.push_back(std::move(poly));
    }
    return result;
};

auto build_convex_hull(Vector<Vec2d> pts) -> Polygon2d {
    std::sort(pts.begin(), pts.end()); // Vec2d::operator< is (x, then y)
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());

    const U32 n = U32(pts.size());
    if (n < 3)
        return {};

    // (a-o) x (b-o) > 0  ⇔  o→a→b turns left (CCW).
    auto turn = [](const Vec2d& o, const Vec2d& a, const Vec2d& b) -> F64 {
        return (a - o).cross(b - o);
    };

    Vector<Vec2d> hull(2 * n);
    U32 k = 0;

    // Lower chain (left→right).
    for (U32 i = 0; i < n; ++i) {
        while (k >= 2 && turn(hull[k - 2], hull[k - 1], pts[i]) <= 0.0)
            --k;
        hull[k++] = pts[i];
    }
    // Upper chain (right→left); starts one past the lower chain's last point.
    const U32 lower = k + 1;
    for (I32 i = I32(n) - 2; i >= 0; --i) {
        while (k >= lower && turn(hull[k - 2], hull[k - 1], pts[i]) <= 0.0)
            --k;
        hull[k++] = pts[i];
    }

    hull.resize(k - 1); // last == first, drop the duplicate
    if (hull.size() < 3)
        return {}; // everything was collinear
    return Polygon2d{std::move(hull)};
}

auto polygon2_triangulate(const Vector<Vec2d>& poly, U32 indexOffset, bool ccw)
    -> Vector<U32> {
    using Point = std::array<F64, 2>;
    std::vector<std::vector<Point>> polygon;
    auto& ring = polygon.emplace_back();
    ring.reserve(poly.size());
    for (const auto& p : poly)
        ring.push_back({p.x(), p.y()});

    auto raw = mapbox::earcut<U32>(polygon);
    for (auto& idx : raw)
        idx += indexOffset;

    if (!ccw)
        std::ranges::reverse(raw);

    return raw;
}
} // namespace nv