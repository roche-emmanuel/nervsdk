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

// ---------------------------------------------------------------------------
// polygon2_offset
//
// Offsets a single closed polygon by `offset` (positive grows outward,
// negative shrinks inward), using Clipper2's polygon end type so the ring is
// treated as closed rather than as an open, capped polyline (cf.
// Polygon2::inflated(), which defaults to round end caps and is meant for
// open lines, not for eroding/growing a closed ring). A negative offset
// larger than the polygon can sustain collapses it down to nothing, in which
// case an empty vector is returned; a non-convex input can also split into
// several parts, one per output entry.
// ---------------------------------------------------------------------------
auto polygon2_offset(const Polygon2d& poly, F64 offset,
                     I32 joinType = PATH_JOIN_ROUND) -> Vector<Polygon2d>;

// ---------------------------------------------------------------------------
// polygon2_smooth_chaikin
//
// Smooths a closed polygon's silhouette via Chaikin's corner-cutting
// subdivision: each edge (p_i, p_i+1) is replaced by 2 points cut in from
// either end by `cutRatio` (clamped to [0, 0.5]; 0.25 is Chaikin's original
// value), run for `iterations` rounds. Every cut point lies strictly between
// its 2 source vertices, so the result never expands past the input's own
// edges -- it only ever cuts corners inward -- which keeps it safely inside
// whatever the input itself was already inside (eg. the junction bag polygon
// it was offset from). iterations == 0 returns the input unchanged.
// ---------------------------------------------------------------------------
auto polygon2_smooth_chaikin(const Polygon2d& poly, U32 iterations,
                             F64 cutRatio = 0.25) -> Polygon2d;

// ---------------------------------------------------------------------------
// polygon2_resample
//
// Resamples a closed polygon at a fixed arc-length step around its full
// perimeter (the last vertex connects back to the first -- there is no
// pinned "first"/"last" endpoint the way an open polyline resample has).
// Degenerate input (fewer than 2 points, or a perimeter shorter than
// stepCm) is returned unchanged.
// ---------------------------------------------------------------------------
auto polygon2_resample(const Polygon2d& poly, F64 stepCm) -> Polygon2d;

// ---------------------------------------------------------------------------
// polygon2_smooth_window_mean
//
// Smooths a closed polygon's silhouette by first resampling it at `stepCm`
// (a dense, evenly-spaced point set is what makes the windowed mean below
// behave consistently regardless of the input's original vertex spacing),
// then running `iterations` rounds of a windowed mean: each round replaces
// every point with the mean of the `windowRadius` closest points on either
// side of it along the ring (2*windowRadius + 1 points total, wrapping
// across the closed loop). Every point in a round is read from the
// *previous* round's list, so a round is a consistent snapshot rather than
// a smear that depends on iteration order.
//
// Unlike Chaikin corner-cutting, this has no built-in guarantee of staying
// inside the source polygon -- a tight concave pinch can bulge slightly
// outward under enough smoothing. Worth checking against the source (eg.
// via polygon2_difference) if strict containment matters for the caller.
// ---------------------------------------------------------------------------
auto polygon2_smooth_window_mean(const Polygon2d& poly, F64 stepCm,
                                 U32 windowRadius, U32 iterations) -> Polygon2d;

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
