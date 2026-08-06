#ifndef _POLYGON2_H_
#define _POLYGON2_H_

#include <nvk/geometry/Polyline2.h>

namespace nv {
template <typename T> struct Polygon2 {
    Vector<Vec2<T>> coords;

    Polygon2() = default;
    // Polygon2(Polygon2&& rhs) = default;
    explicit Polygon2(Vector<Vec2<T>> input) : coords(std::move(input)) {};

    [[nodiscard]] auto size() const -> size_t { return coords.size(); }
    [[nodiscard]] auto area() const -> T {
        return polygon_signed_area_2d(coords.data(), coords.size());
    }
    [[nodiscard]] auto centroid() const -> Vec2<T> {
        return polygon_centroid_2d(coords.data(), coords.size());
    }

    [[nodiscard]] auto empty() const -> bool { return coords.empty(); }

    auto dedupe_points() -> U32 {
        return polyline2_dedupe_points(coords, true);
    }
    auto compute_bounds() const -> Box2<T> {
        Box2<T> box;
        for (const auto& pt : coords) {
            box.extendTo(pt);
        }
        return box;
    }
    // Quantizes the coords onto a regular grid of step `gridStep` and drops
    // the points that the snapping collapsed onto their neighbour. Call this
    // on every input of a boolean op that is meant to share edges with the
    // others, using the *same* step on all of them.
    //
    // The step must stay coarser than the precision the clipping backend
    // works at (0.01 in the coord unit by default), otherwise the snapping is
    // a no-op. Returns the number of points removed by the dedupe pass.
    auto snap_coords(T gridStep) -> U32 {
        snap_points_to_grid(coords, gridStep);
        return dedupe_points();
    }

    auto inflated(T offset, I32 joinType = PATH_JOIN_ROUND) const
        -> Vector<Polygon2<T>> {
        return inflate_polyline2(coords, offset, joinType);
    }

    void translate(const Vec2<T>& trans) {
        for (auto& pt : coords) {
            pt += trans;
        }
    }
    auto translated(const Vec2<T>& trans) const -> Polygon2<T> {
        Polygon2<T> res(coords);
        res.translate(trans);
        return res;
    }
};

using Polygon2f = Polygon2<F32>;
using Polygon2d = Polygon2<F64>;

auto polygon2_union(const Vector<Polygon2d>& inputs,
                    I32 fillRule = FILL_NONZERO) -> Vector<Polygon2d>;

auto polygon2_union_inflated(const Vector<Polygon2d>& inputs, F64 offset,
                             I32 fillRule = FILL_NONZERO,
                             I32 joinType = PATH_JOIN_ROUND)
    -> Vector<Polygon2d>;

auto polygon2_difference(const Vector<Polygon2d>& subjects,
                         const Vector<Polygon2d>& clips,
                         I32 fillRule = FILL_NONZERO) -> Vector<Polygon2d>;

auto polygon2_intersection(const Vector<Polygon2d>& subjects,
                           const Vector<Polygon2d>& clips,
                           I32 fillRule = FILL_NONZERO) -> Vector<Polygon2d>;

auto polygon2_xor(const Vector<Polygon2d>& subjects,
                  const Vector<Polygon2d>& clips, I32 fillRule = FILL_NONZERO)
    -> Vector<Polygon2d>;

auto polygon2_area(const Polygon2d& poly) -> F64;

auto polygon2_is_positive_orientation(const Polygon2d& poly) -> bool;

auto inflate_polyline2(const Polyline2f& centerLine, F32 offset,
                       I32 joinType = PATH_JOIN_ROUND,
                       I32 endType = PATH_END_ROUND) -> Vector<Polygon2f>;

auto inflate_polyline2(const Polyline2d& centerLine, F64 offset,
                       I32 joinType = PATH_JOIN_ROUND,
                       I32 endType = PATH_END_ROUND) -> Vector<Polygon2d>;

auto polygon2_triangulate(const Vector<Vec2d>& poly, U32 indexOffset, bool ccw)
    -> Vector<U32>;

// ---------------------------------------------------------------------------
// convex_hull  (Andrew's monotone chain)
//
// Lexicographic sort + lower/upper chain. Collinear points are dropped (strict
// turn test), so the result is a minimal CCW hull with no repeated closing
// vertex. Returns empty when fewer than 3 non-collinear points remain.
// ---------------------------------------------------------------------------
auto build_convex_hull(Vector<Vec2d> pts) -> Polygon2d;

} // namespace nv

#endif
