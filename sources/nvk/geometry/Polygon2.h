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

    [[nodiscard]] auto empty() const -> bool { return coords.empty(); }

    auto dedupe_points() -> U32 {
        return polyline2_dedupe_points(coords, true);
    }
};

using Polygon2f = Polygon2<F32>;
using Polygon2d = Polygon2<F64>;

auto polygon2_union(const Vector<Polygon2d>& inputs,
                    I32 fillRule = FILL_NONZERO) -> Vector<Polygon2d>;

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
