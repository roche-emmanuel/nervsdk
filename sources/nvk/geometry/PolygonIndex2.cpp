#include <nvk/geometry/PolygonIndex2.h>

#include <algorithm>
#include <limits>

namespace nv {

namespace polygon_index2_detail {

// First crossing of the ray with a polygon boundary, < 0 when none. A hit at
// the ray origin itself does not count.
auto first_crossing(const Vector<Vec2d>& poly, const Vec2d& pos,
                    const Vec2d& dir) -> F64 {
    constexpr F64 kMinT = 1e-6;
    F64 best = -1.0;
    const U32 count = U32(poly.size());
    for (U32 i = 0, j = count - 1; i < count; j = i++) {
        F64 t = 0.0;
        if (ray_intersect_segment2d(pos, dir, poly[j], poly[i], t) &&
            t > kMinT && (best < 0.0 || t < best)) {
            best = t;
        }
    }
    return best;
}

} // namespace polygon_index2_detail

using namespace polygon_index2_detail;

PolygonIndex2::PolygonIndex2(F64 polygonCellCm, F64 edgeCellCm)
    : _boxGrid(polygonCellCm), _edgeGrid(edgeCellCm) {
}

auto PolygonIndex2::add(Vector<Vec2d> coords, U32 layer) -> U32 {
    if (coords.size() < 3) {
        return invalidId;
    }
    Entry entry;
    entry.layer = layer;
    entry.lo = coords[0];
    entry.hi = coords[0];
    for (const auto& pt : coords) {
        entry.lo = {std::min(entry.lo.x(), pt.x()), std::min(entry.lo.y(), pt.y())};
        entry.hi = {std::max(entry.hi.x(), pt.x()), std::max(entry.hi.y(), pt.y())};
    }
    entry.coords = std::move(coords);

    const U32 id = U32(_polys.size());
    _boxGrid.insert(id, Box2d(entry.lo.x(), entry.hi.x(), entry.lo.y(),
                              entry.hi.y()));
    _polys.push_back(std::move(entry));
    return id;
}

void PolygonIndex2::build_edge_index() {
    _edgeGrid.clear();
    _numEdges = 0;
    for (U32 id = 0; id < U32(_polys.size()); ++id) {
        const auto& pts = _polys[id].coords;
        const U32 count = U32(pts.size());
        for (U32 edge = 0; edge < count; ++edge) {
            const Vec2d& a = pts[edge];
            const Vec2d& b = pts[(edge + 1) % count];
            _edgeGrid.insert({id, edge},
                             Box2d(std::min(a.x(), b.x()), std::max(a.x(), b.x()),
                                   std::min(a.y(), b.y()), std::max(a.y(), b.y())));
            ++_numEdges;
        }
    }
}

auto PolygonIndex2::candidates(const Vec2d& lo, const Vec2d& hi) const
    -> Vector<U32> {
    Vector<U32> ids = _boxGrid.query(Box2d(lo.x(), hi.x(), lo.y(), hi.y()));
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

auto PolygonIndex2::polygons_at(const Vec2d& pos, U32 layer) const
    -> Vector<U32> {
    Vector<U32> ids;
    for (U32 id : candidates(pos, pos)) {
        const auto& entry = _polys[id];
        if (entry.layer == layer && point_in_polygon(entry.coords, pos)) {
            ids.push_back(id);
        }
    }
    return ids;
}

auto PolygonIndex2::polygons_in_box(const Vec2d& lo, const Vec2d& hi,
                                    U32 layer) const -> Vector<U32> {
    Vector<U32> ids;
    for (U32 id : candidates(lo, hi)) {
        const auto& entry = _polys[id];
        if (entry.layer == layer && entry.hi.x() >= lo.x() &&
            entry.lo.x() <= hi.x() && entry.hi.y() >= lo.y() &&
            entry.lo.y() <= hi.y()) {
            ids.push_back(id);
        }
    }
    return ids;
}

auto PolygonIndex2::cast(const Vec2d& pos, const Vec2d& dir, F64 maxDistCm,
                         U32 layer, const Vector<U32>& excluded) const -> F64 {
    const Vec2d end = pos + dir * maxDistCm;
    const Vec2d lo{std::min(pos.x(), end.x()), std::min(pos.y(), end.y())};
    const Vec2d hi{std::max(pos.x(), end.x()), std::max(pos.y(), end.y())};

    F64 best = -1.0;
    for (U32 id : candidates(lo, hi)) {
        const auto& entry = _polys[id];
        if (entry.layer != layer ||
            std::find(excluded.begin(), excluded.end(), id) != excluded.end()) {
            continue;
        }
        const F64 t = first_crossing(entry.coords, pos, dir);
        if (t > 0.0 && t <= maxDistCm && (best < 0.0 || t < best)) {
            best = t;
        }
    }
    return best;
}

auto PolygonIndex2::exit_distance(U32 id, const Vec2d& pos,
                                  const Vec2d& dir) const -> F64 {
    return first_crossing(_polys[id].coords, pos, dir);
}

auto PolygonIndex2::boundary_distance(U32 id, const Vec2d& pos) const -> F64 {
    const auto& pts = _polys[id].coords;
    F64 best = std::numeric_limits<F64>::max();
    const U32 count = U32(pts.size());
    for (U32 i = 0, j = count - 1; i < count; j = i++) {
        best = std::min(best, seg2_point_distance(pts[j], pts[i], pos, true));
    }
    return best;
}

void PolygonIndex2::boundary_hits(const Vec2d& pos, F64 reachCm, U32 layer,
                                  Vector<std::pair<U32, F64>>& out) const {
    out.clear();
    Vector<EdgeRef> refs;
    _edgeGrid.query_radius(pos, reachCm, refs);
    for (const auto& ref : refs) {
        const auto& entry = _polys[ref.polygon];
        if (entry.layer != layer) {
            continue;
        }
        const U32 count = U32(entry.coords.size());
        const F64 dist =
            seg2_point_distance(entry.coords[ref.edge],
                                entry.coords[(ref.edge + 1) % count], pos, true);
        if (dist >= reachCm) {
            continue;
        }
        bool found = false;
        for (auto& hit : out) {
            if (hit.first == ref.polygon) {
                hit.second = std::min(hit.second, dist);
                found = true;
                break;
            }
        }
        if (!found) {
            out.emplace_back(ref.polygon, dist);
        }
    }
    std::sort(out.begin(), out.end(),
              [](const auto& l, const auto& r) { return l.first < r.first; });
}

} // namespace nv
