#ifndef _REGION2_H_
#define _REGION2_H_

#include <nvk/geometry/Polygon2.h>

namespace nv {
// ---------------------------------------------------------------------------
// Region2 — a planar region: one outer boundary ring + zero or more holes.
//
// Convention (matches Clipper2 / GeoJSON / earcut expectations):
//   outer : CCW winding  (positive area)
//   holes : CW  winding  (each hole cuts out of the outer)
//
// inflate_polyline2() returns Vector<Polygon2f>; use Region2f::from_rings()
// to interpret the first ring as outer and the rest as holes.
// ---------------------------------------------------------------------------
template <typename T> struct Region2 {
    Polygon2<T> outer;
    Vector<Polygon2<T>> holes;

    [[nodiscard]] auto empty() const -> bool { return outer.coords.size() < 3; }

    [[nodiscard]] auto get_num_points() const -> U32 {
        U32 count = outer.size();
        for (const auto& h : holes) {
            count += h.size();
        }
        return count;
    }

    // Interpret the Vector<Polygon2> returned by inflate_polyline2 as a
    // Region2: first ring = outer, subsequent rings = holes.
    static auto from_rings(const Vector<Polygon2<T>>& rings) -> Region2<T> {
        Region2<T> result;
        if (rings.empty())
            return result;
        result.outer = rings[0];
        for (U32 i = 1; i < U32(rings.size()); ++i)
            result.holes.push_back(rings[i]);
        return result;
    }

    // Triangulate using mapbox/earcut.
    //
    // Returns a flat index buffer into the region's concatenated vertex array:
    //   [ outer.coords ... hole[0].coords ... hole[1].coords ... ]
    // i.e. the same layout you'd build when constructing 3-D mesh vertices,
    // so indices are usable directly with no offset arithmetic.
    //
    // Returns empty if the outer ring has fewer than 3 vertices.
    [[nodiscard]] auto triangulate() const -> Vector<U32> {
        if (outer.coords.size() < 3)
            return {};

        using EarcutPoint = std::array<T, 2>;
        std::vector<std::vector<EarcutPoint>> polygon;

        auto& outerRing = polygon.emplace_back();
        outerRing.reserve(outer.coords.size());
        for (const auto& pt : outer.coords)
            outerRing.push_back({pt.x(), pt.y()});

        for (const auto& hole : holes) {
            if (hole.coords.size() < 3)
                continue;
            auto& holeRing = polygon.emplace_back();
            holeRing.reserve(hole.coords.size());
            for (const auto& pt : hole.coords)
                holeRing.push_back({pt.x(), pt.y()});
        }

        return mapbox::earcut<U32>(polygon);
    }

    [[nodiscard]] auto get_point_position(U32 globalIdx) const -> Vec2<T> {
        // Resolve global index → (ring coords, local index)
        const Vector<Vec2<T>>* ring = nullptr;
        U32 localIdx = globalIdx;

        const U32 outerN = U32(outer.coords.size());
        if (localIdx < outerN) {
            ring = &outer.coords;
        } else {
            localIdx -= outerN;
            for (const auto& hole : holes) {
                const U32 holeN = U32(hole.coords.size());
                if (localIdx < holeN) {
                    ring = &hole.coords;
                    break;
                }
                localIdx -= holeN;
            }
        }

        NVCHK(ring != nullptr,
              "Region2::get_point: global index {} out of range "
              "(get_num_points={})",
              globalIdx, get_num_points());

        return (*ring)[localIdx];
    }

    // Returns the polygon boundary tangent at global vertex index i.
    //
    // Global index layout (same as triangulate()):
    //   [0 .. outerN-1]                          outer ring
    //   [outerN .. outerN+hole0N-1]               hole 0
    //   [outerN+hole0N .. outerN+hole0N+hole1N-1] hole 1
    //   ...
    //
    // The tangent is the mean of the two edge directions meeting at vertex i,
    // then normalised:
    //   prev_edge = normalise(pos[i]   - pos[i-1])
    //   next_edge = normalise(pos[i+1] - pos[i])
    //   tangent   = normalise(prev_edge + next_edge)
    //
    // Rings are treated as implicitly closed (open storage: coords[n-1] is
    // followed by coords[0]). No duplicate closing vertex is assumed.
    //
    // If the two edges are anti-parallel (180° reversal — degenerate spike),
    // the sum is near-zero; in that case the next-edge direction is returned
    // as a fallback.
    [[nodiscard]] auto get_point_tangent(U32 globalIdx) const -> Vec2<T> {
        // Resolve global index → (ring coords, local index)
        const Vector<Vec2<T>>* ring = nullptr;
        U32 localIdx = globalIdx;

        const U32 outerN = U32(outer.coords.size());
        if (localIdx < outerN) {
            ring = &outer.coords;
        } else {
            localIdx -= outerN;
            for (const auto& hole : holes) {
                const U32 holeN = U32(hole.coords.size());
                if (localIdx < holeN) {
                    ring = &hole.coords;
                    break;
                }
                localIdx -= holeN;
            }
        }

        NVCHK(ring != nullptr,
              "Region2::get_point_tangent: global index {} out of range "
              "(get_num_points={})",
              globalIdx, get_num_points());

        const U32 n = U32(ring->size());
        NVCHK(n >= 2,
              "Region2::get_point_tangent: ring has fewer than 2 vertices");

        // Wrap-around neighbours (ring is implicitly closed, no dup vertex)
        const U32 iPrev = (localIdx + n - 1) % n;
        const U32 iNext = (localIdx + 1) % n;

        const Vec2<T>& pPrev = (*ring)[iPrev];
        const Vec2<T>& pCurr = (*ring)[localIdx];
        const Vec2<T>& pNext = (*ring)[iNext];

        // Normalise each edge direction individually before averaging so that
        // very short edges don't dominate long ones.
        auto safeNorm = [](Vec2<T> v) -> Vec2<T> {
            const T len = v.length();
            return (len > T(1e-12)) ? v / len : Vec2<T>(T(0), T(0));
        };

        const Vec2<T> prevEdge = safeNorm(pCurr - pPrev);
        const Vec2<T> nextEdge = safeNorm(pNext - pCurr);

        Vec2<T> sum = prevEdge + nextEdge;
        const T sumLen = sum.length();

        // Anti-parallel edges (spike / 180° reversal): fall back to next edge
        if (sumLen < T(1e-9))
            return nextEdge.normalized();

        return sum.normalized();
    }

    [[nodiscard]] auto get_point_normal(U32 globalIdx) const -> Vec2<T> {
        auto tang = get_point_tangent(globalIdx);
        return tang.ccw90();
    }
};

using Region2f = Region2<F32>;
using Region2d = Region2<F64>;

} // namespace nv

#endif
