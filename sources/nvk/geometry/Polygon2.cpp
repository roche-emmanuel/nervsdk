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

} // namespace nv