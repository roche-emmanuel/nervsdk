// File: nvk/sim/TerrainField.h
//
// The obstacle ceiling every airborne agent plans against.
//
// This is to flight what WaterSurface is to buoyancy: an abstract "what is
// under me" query with a couple of concrete implementations, borrowed rather
// than owned by the models that consume it. FlightPlanner never knows whether
// it is looking at a full-resolution heightmap in an offline tool or at an
// eight-megabyte pyramid baked into a UE data asset.
//
// The one query that matters is max_height_in(): the *highest* obstacle
// within a radius, not the height directly underneath. An aircraft is
// protected by a corridor, not by a point sample, and a point sample of a
// ridge line is exactly the thing that reads fine in the planner and puts a
// wing into a hillside at runtime.
//
// Frame
// -----
// x, y and the returned heights are metres. The XY frame is the *raster*
// frame used by everything downstream of PCGManager::geo_to_world_xy:
// x grows East, y grows South, origin at the north-west corner of the
// terrain. That is deliberately not the right-handed nvk sim frame: every
// consumer of this class (route points, aircraft positions, the UE actor
// transform) already lives in the raster frame, and reflecting twice to end
// up back where we started would only create somewhere for a sign to go
// missing. Heights are metres above mean sea level, matching the heightmap
// encoding.

#ifndef _NV_TERRAINFIELD_H_
#define _NV_TERRAINFIELD_H_

#include <nvk/math/Vec2.h>
#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {

/// Abstract obstacle ceiling.
class TerrainField {
  public:
    TerrainField() = default;
    TerrainField(const TerrainField&) = default;
    auto operator=(const TerrainField&) -> TerrainField& = default;
    TerrainField(TerrainField&&) = default;
    auto operator=(TerrainField&&) -> TerrainField& = default;
    virtual ~TerrainField() = default;

    /// Height at a point (m MSL). Off-map reads as the map edge, clamped,
    /// rather than as zero: an aircraft leaving the world should not think
    /// the ground fell away.
    [[nodiscard]] virtual auto height_at(const Vec2d& posM) const -> F64 = 0;

    /// Highest obstacle within radiusM of posM (m MSL). Implementations may
    /// over-estimate — that is safe — but must never under-estimate.
    [[nodiscard]] virtual auto max_height_in(const Vec2d& posM,
                                             F64 radiusM) const -> F64 = 0;

    /// World extent in metres. Square, matching the terrain raster.
    [[nodiscard]] virtual auto size_m() const -> F64 = 0;

    [[nodiscard]] virtual auto is_valid() const -> bool = 0;
};

// ---------------------------------------------------------------------------
// FlatTerrain
// ---------------------------------------------------------------------------

/// Constant-height field. The baseline used by the unit tests and by any
/// world baked without a ceiling raster.
class FlatTerrain : public TerrainField {
  public:
    explicit FlatTerrain(F64 heightM = 0.0, F64 sizeM = 1.0e7)
        : _heightM(heightM), _sizeM(sizeM) {}

    [[nodiscard]] auto height_at(const Vec2d& /*posM*/) const -> F64 override {
        return _heightM;
    }

    [[nodiscard]] auto max_height_in(const Vec2d& /*posM*/,
                                     F64 /*radiusM*/) const -> F64 override {
        return _heightM;
    }

    [[nodiscard]] auto size_m() const -> F64 override { return _sizeM; }
    [[nodiscard]] auto is_valid() const -> bool override { return true; }

  private:
    F64 _heightM{0.0};
    F64 _sizeM{1.0e7};
};

// ---------------------------------------------------------------------------
// MaxPyramidTerrain
// ---------------------------------------------------------------------------

/// Elevation raster plus a max-reduction pyramid over it.
///
/// Level 0 is a square raster of `res` cells covering the world; level k is
/// the 2x2 max-reduction of level k-1, so a texel at level k holds the
/// maximum over a 2^k x 2^k block of level 0. That makes max_height_in() an
/// O(1) query for *any* radius: pick the coarsest level whose texel is still
/// no larger than the radius, then take the maximum over the handful of
/// texels the query disc's bounding box touches. The answer is conservative
/// by construction — a texel only ever reports something at least as high as
/// anything inside it — which is the direction an obstacle ceiling is
/// allowed to be wrong in.
///
/// Storage is F32 per texel across all levels, i.e. 4/3 x res^2 x 4 bytes.
/// At res = 1024 that is 5.6 MB, small enough to carry inline in a data
/// asset and to sample on the game thread without touching a render
/// resource — the same reasoning that keeps the sea clearance field inline.
///
/// Raster convention: *area*. Texel (row, col) of level 0 covers the square
/// [col*pixelM, (col+1)*pixelM) x [row*pixelM, (row+1)*pixelM), with
/// pixelM = sizeM / res. Row 0 is world y = 0.
class MaxPyramidTerrain : public TerrainField {
  public:
    MaxPyramidTerrain() = default;

    /// Build from a level-0 raster of res x res heights in metres MSL.
    /// `heights` is row-major with row 0 at world y = 0, and is consumed.
    void build(Vector<F32>&& heights, U32 res, F64 sizeM);

    /// Build by sampling a callback over the level-0 grid. The callback
    /// receives the world XY of the texel *centre* and returns metres MSL.
    /// Convenient in PCGen, where the source is a full-resolution heightmap
    /// that must be block-maximum reduced rather than point sampled — see
    /// build_from_block_max() for that case.
    void build_from_fn(U32 res, F64 sizeM,
                       const std::function<F64(const Vec2d&)>& fn);

    /// Build by block-maximum reduction of a higher-resolution source
    /// raster. srcRes must be >= res; the source uses the *node* convention
    /// (srcRes samples spanning [0, sizeM] inclusive), which is what the
    /// terrain heightmap uses, and each output texel takes the maximum over
    /// every source sample landing inside it, plus the samples on its
    /// border so no ridge falls between two texels.
    void build_from_block_max(const Vector<F32>& srcHeights, U32 srcRes,
                              U32 res, F64 sizeM);

    /// Raise a rectangular region by a fixed amount, applied to level 0
    /// before the pyramid is reduced. Used to fold building tops into the
    /// ceiling. Call reduce() once after the last edit.
    void raise_level0(U32 row, U32 col, F32 heightM);

    /// (Re)build levels 1..N from level 0.
    void reduce();

    // ── TerrainField ────────────────────────────────────────────────────

    [[nodiscard]] auto height_at(const Vec2d& posM) const -> F64 override;

    [[nodiscard]] auto max_height_in(const Vec2d& posM, F64 radiusM) const
        -> F64 override;

    [[nodiscard]] auto size_m() const -> F64 override { return _sizeM; }

    [[nodiscard]] auto is_valid() const -> bool override {
        return _res > 0 && !_levels.empty();
    }

    // ── Introspection ───────────────────────────────────────────────────

    [[nodiscard]] auto resolution() const -> U32 { return _res; }
    [[nodiscard]] auto num_levels() const -> U32 { return U32(_levels.size()); }
    [[nodiscard]] auto pixel_m() const -> F64 { return _pixelM; }

    [[nodiscard]] auto level_res(U32 level) const -> U32 {
        return _levelRes[level];
    }

    [[nodiscard]] auto level(U32 idx) const -> const Vector<F32>& {
        return _levels[idx];
    }

    /// Mutable access to level 0, for callers that fill it in place before
    /// calling reduce().
    [[nodiscard]] auto level0_mutable() -> Vector<F32>& { return _levels[0]; }

    [[nodiscard]] auto min_height_m() const -> F64 { return _minM; }
    [[nodiscard]] auto max_height_m() const -> F64 { return _maxM; }

    /// Flatten every level into one row-major buffer, levels stacked
    /// vertically: level k occupies rows [level_row_offset(k),
    /// level_row_offset(k) + level_res(k)) and columns [0, level_res(k)).
    /// The total row count is total_rows(). This is the layout written to
    /// terrain_ceiling.png and read back by the UE bake, so it lives here
    /// rather than in either consumer.
    [[nodiscard]] auto pack_rows() const -> Vector<F32>;

    [[nodiscard]] auto level_row_offset(U32 level) const -> U32;
    [[nodiscard]] auto total_rows() const -> U32;

    /// Inverse of pack_rows(). res/sizeM must match what produced it.
    void unpack_rows(const Vector<F32>& packed, U32 res, F64 sizeM);

  private:
    void refresh_extents();

    [[nodiscard]] auto texel(U32 level, I32 row, I32 col) const -> F32;

    /// Coarsest level whose texel size is at most `spanM`, so that a query
    /// of that span touches a bounded number of texels.
    [[nodiscard]] auto level_for_span(F64 spanM) const -> U32;

    U32 _res{0};
    F64 _sizeM{0.0};
    F64 _pixelM{0.0};

    Vector<Vector<F32>> _levels;
    Vector<U32> _levelRes;

    F64 _minM{0.0};
    F64 _maxM{0.0};
};

} // namespace nv

#endif
