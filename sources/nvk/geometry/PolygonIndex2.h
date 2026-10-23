#ifndef _NV_POLYGONINDEX2_H_
#define _NV_POLYGONINDEX2_H_

#include <nvk/geometry/SpatialGrid2.h>
#include <nvk/geometry/geom_utils.h>
#include <nvk/math/Vec2.h>
#include <nvk_common.h>

namespace nv {

/**
PolygonIndex2: a set of simple 2D polygons (one ring each) with the spatial
queries a "what surface is under / around this point" lookup needs, without
scanning the whole set: point containment, box overlap, ray casting, and
distance to the boundaries.

Every polygon belongs to a `layer` (any small integer the caller chooses: an
elevated / ground flag, a floor number, ...). Each query names the layer it is
about, and polygons of other layers are ignored, so stacked surfaces never
interact.

Two grids are kept: one over the polygon bounding boxes (containment, box and
ray queries) and, once build_edge_index() has been called, one over the
individual boundary edges, which boundary_hits() uses so that a polygon with
thousands of edges (a long road strip) is not walked edge by edge for a nearby
point.

Polygons are added first, ids are the order of insertion. Not thread-safe while
building; queries are const and can be shared afterwards.
*/
class PolygonIndex2 {
  public:
    static constexpr U32 invalidId = U32(-1);

    /// `polygonCellCm`: cell of the bounding box grid; `edgeCellCm`: cell of
    /// the boundary edge grid.
    explicit PolygonIndex2(F64 polygonCellCm = 2000.0,
                           F64 edgeCellCm = 1000.0);

    /// Adds a polygon (implicitly closed) and returns its id, or invalidId
    /// when it has fewer than 3 points (nothing is stored then).
    auto add(Vector<Vec2d> coords, U32 layer = 0) -> U32;

    /// Indexes the boundary edges of all the polygons added so far. Call once
    /// after the last add() and before boundary_hits().
    void build_edge_index();

    [[nodiscard]] auto size() const -> U32 { return U32(_polys.size()); }
    [[nodiscard]] auto coords(U32 id) const -> const Vector<Vec2d>& {
        return _polys[id].coords;
    }
    [[nodiscard]] auto layer(U32 id) const -> U32 { return _polys[id].layer; }
    [[nodiscard]] auto box_min(U32 id) const -> const Vec2d& {
        return _polys[id].lo;
    }
    [[nodiscard]] auto box_max(U32 id) const -> const Vec2d& {
        return _polys[id].hi;
    }
    /// Number of boundary edges indexed by build_edge_index().
    [[nodiscard]] auto num_edges() const -> U64 { return _numEdges; }

    /// Ids (increasing) of the polygons of that layer containing pos.
    [[nodiscard]] auto polygons_at(const Vec2d& pos, U32 layer) const
        -> Vector<U32>;

    /// Ids (increasing) of the polygons of that layer whose bounding box
    /// overlaps [lo, hi].
    [[nodiscard]] auto polygons_in_box(const Vec2d& lo, const Vec2d& hi,
                                       U32 layer) const -> Vector<U32>;

    /// Distance to the first boundary crossed by the ray pos + dir * t,
    /// t in ]0, maxDistCm], among the polygons of that layer that are not
    /// excluded; < 0 when nothing is crossed. dir is expected to be unit.
    [[nodiscard]] auto cast(const Vec2d& pos, const Vec2d& dir, F64 maxDistCm,
                            U32 layer, const Vector<U32>& excluded) const
        -> F64;

    /// Distance along dir from pos to the boundary of one polygon, < 0 when
    /// the ray never crosses it.
    [[nodiscard]] auto exit_distance(U32 id, const Vec2d& pos,
                                     const Vec2d& dir) const -> F64;

    /// Distance from pos to the boundary of one polygon.
    [[nodiscard]] auto boundary_distance(U32 id, const Vec2d& pos) const
        -> F64;

    /// The polygons of that layer whose boundary is closer than reachCm to
    /// pos, with that distance, by increasing id. Same result as testing
    /// boundary_distance() on polygons_in_box(), but only the edges around
    /// pos are looked at. Needs build_edge_index().
    void boundary_hits(const Vec2d& pos, F64 reachCm, U32 layer,
                       Vector<std::pair<U32, F64>>& out) const;

  private:
    struct Entry {
        Vector<Vec2d> coords;
        U32 layer{0};
        Vec2d lo;
        Vec2d hi;
    };
    struct EdgeRef {
        U32 polygon;
        U32 edge; // from coords[edge] to coords[(edge + 1) % size]
    };

    [[nodiscard]] auto candidates(const Vec2d& lo, const Vec2d& hi) const
        -> Vector<U32>;

    Vector<Entry> _polys;
    SpatialGrid2<U32> _boxGrid;
    SpatialGrid2<EdgeRef> _edgeGrid;
    U64 _numEdges{0};
};

} // namespace nv

#endif
