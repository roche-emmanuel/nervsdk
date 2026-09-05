#ifndef _NV_SPATIALGRID2_H_
#define _NV_SPATIALGRID2_H_

#include <nvk/base/std_containers.h>
#include <nvk/math/Box2.h>
#include <nvk/math/Vec2.h>
#include <nvk_common.h>

#include <cmath>

namespace nv {

/**
 * SpatialGrid2<T>: a uniform 2D grid used to accelerate "what's near this
 * point/region" queries.
 *
 * The same broad-phase idea keeps recurring across this codebase under
 * different names — PCGen's RoadConnectorGrid, Argus's air traffic
 * subsystem hash, a ground vehicle placer's overlap grid — always the same
 * shape: bucket payloads by cell, then a query only has to look at the
 * handful of cells its own footprint spans rather than the whole set. This
 * is that shape, factored out once.
 *
 * T is whatever payload each entry carries — an index into an external
 * array is the usual choice, since the grid does not own or interpret T,
 * only buckets it.
 *
 * Entries are stored per overlapped cell, so a query can return the same
 * value more than once if its footprint spans several cells and more than
 * one of them matches; callers that need a unique set should dedupe (e.g.
 * via an UnorderedSet) if that matters for their use case.
 *
 * Not thread-safe, and not rebuilt automatically — every caller of this
 * grid already knows exactly when its point set changes, so insert() is
 * left explicit rather than the grid re-scanning some external container on
 * every query.
 */
template <typename T> class SpatialGrid2 {
  public:
    explicit SpatialGrid2(F64 cellSizeCm) : _cellSizeCm(cellSizeCm) {
        NVCHK(cellSizeCm > 0.0, "SpatialGrid2: cellSizeCm must be > 0, got {}.",
              cellSizeCm);
    }

    /** Entries stored so far, counting one per (value, cell) pair — the
        same value inserted under a box spanning 4 cells counts 4 times. */
    [[nodiscard]] auto size() const -> U64 { return _size; }

    [[nodiscard]] auto empty() const -> bool { return _size == 0; }

    void clear() {
        _cells.clear();
        _size = 0;
    }

    /** Adds `value` to every cell overlapped by `box`. */
    void insert(const T& value, const Box2d& box) {
        I64 cellXMin = 0;
        I64 cellYMin = 0;
        I64 cellXMax = 0;
        I64 cellYMax = 0;
        cell_of(box.xmin, box.ymin, cellXMin, cellYMin);
        cell_of(box.xmax, box.ymax, cellXMax, cellYMax);

        for (I64 cellX = cellXMin; cellX <= cellXMax; ++cellX) {
            for (I64 cellY = cellYMin; cellY <= cellYMax; ++cellY) {
                _cells[cell_key(cellX, cellY)].push_back(value);
                ++_size;
            }
        }
    }

    /** Adds `value` to the single cell containing `point`. */
    void insert(const T& value, const Vec2d& point) {
        insert(value, Box2d(point));
    }

    /** Appends every value stored under a cell overlapping `box` to `out`
        (which is not cleared first, so several queries can be accumulated
        into one result). */
    void query(const Box2d& box, Vector<T>& out) const {
        I64 cellXMin = 0;
        I64 cellYMin = 0;
        I64 cellXMax = 0;
        I64 cellYMax = 0;
        cell_of(box.xmin, box.ymin, cellXMin, cellYMin);
        cell_of(box.xmax, box.ymax, cellXMax, cellYMax);

        for (I64 cellX = cellXMin; cellX <= cellXMax; ++cellX) {
            for (I64 cellY = cellYMin; cellY <= cellYMax; ++cellY) {
                const auto found = _cells.find(cell_key(cellX, cellY));
                if (found == _cells.end()) {
                    continue;
                }
                out.insert(out.end(), found->second.begin(),
                          found->second.end());
            }
        }
    }

    [[nodiscard]] auto query(const Box2d& box) const -> Vector<T> {
        Vector<T> out;
        query(box, out);
        return out;
    }

    /** Convenience: every value stored under a cell overlapping the square
        bounding `center`/`radiusCm` — still a cell-level (box) test, so
        callers wanting an exact disc still need their own distance check on
        the results. */
    void query_radius(const Vec2d& center, F64 radiusCm, Vector<T>& out) const {
        query(Box2d(center.x() - radiusCm, center.x() + radiusCm,
                    center.y() - radiusCm, center.y() + radiusCm),
              out);
    }

  private:
    void cell_of(F64 x, F64 y, I64& outCellX, I64& outCellY) const {
        outCellX = I64(std::floor(x / _cellSizeCm));
        outCellY = I64(std::floor(y / _cellSizeCm));
    }

    [[nodiscard]] auto cell_key(I64 cellX, I64 cellY) const -> U64 {
        return (U64(U32(cellX)) << 32) | U64(U32(cellY));
    }

    F64 _cellSizeCm;
    UnorderedMap<U64, Vector<T>> _cells;
    U64 _size{0};
};

} // namespace nv

#endif
