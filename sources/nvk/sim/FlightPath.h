// File: nvk/sim/FlightPath.h
//
// A flown route and the autopilot that follows it.
//
// FlightPath is the planner's output: a 3D polyline with a target speed at
// every point, produced by FlightPlanner and consumed by nothing else.
// FlightPathFollower turns it into the FlightControls that FlightModel eats,
// which is the whole of the "make it fly the route" problem.
//
// The follower is deliberately the same shape as the marine agent's steering
// law — pure pursuit with a time-based lookahead, clamped into a band derived
// from the turn radius — because the two failure modes are identical: too
// short a lookahead weaves, too long cuts corners. The differences are that
// the lookahead band scales with v^2 rather than with hull length, and that
// altitude and speed come off the path rather than from a behaviour layer.
//
// Frame: the same raster frame TerrainField uses. Metres, radians, seconds.

#ifndef _NV_FLIGHTPATH_H_
#define _NV_FLIGHTPATH_H_

#include <nvk/math/Vec2.h>
#include <nvk/math/Vec3.h>
#include <nvk/sim/FlightModel.h>
#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {

// ---------------------------------------------------------------------------
// FlightPath
// ---------------------------------------------------------------------------

struct FlightWaypoint {
    /// Horizontal position (m).
    Vec2d posM{};

    /// Altitude above mean sea level (m).
    F64 altM{0.0};

    /// Target speed at this point (m/s). The follower interpolates between
    /// consecutive waypoints, so a deceleration is authored by dropping the
    /// value at the far end of the leg rather than by a separate command.
    F64 speedMs{0.0};

    /// Terrain-relative margin the planner guaranteed here (m). Carried for
    /// diagnostics and for the runtime's "is this route still safe" check
    /// after the ceiling field is replaced.
    F64 marginM{0.0};
};

/// A planned trajectory. `closed` marks a route that loops back on itself —
/// an orbit or a holding pattern — which the follower flies indefinitely
/// rather than arriving at.
struct FlightPath {
    Vector<FlightWaypoint> points;

    /// Cumulative arclength at each point; size == points.size().
    Vector<F64> cumLenM;

    bool closed{false};
    bool valid{false};

    /// Worst terrain margin anywhere on the path (m).
    F64 minMarginM{0.0};

    void reset() {
        points.clear();
        cumLenM.clear();
        closed = false;
        valid = false;
        minMarginM = 0.0;
    }

    /// Recompute cumLenM and validity from `points`. Call after any edit.
    void rebuild();

    [[nodiscard]] auto num_segments() const -> U32 {
        return points.size() >= 2 ? U32(points.size()) - 1 : 0;
    }

    [[nodiscard]] auto length_m() const -> F64 {
        return cumLenM.empty() ? 0.0 : cumLenM.back();
    }

    /// Interpolate the path at arclength `s`, clamped (or wrapped, when
    /// closed) into range.
    [[nodiscard]] auto sample(F64 sM) const -> FlightWaypoint;

    /// Index of the segment containing arclength `s`.
    [[nodiscard]] auto segment_at(F64 sM) const -> U32;

    /// Append a point without rebuilding. Cheap bulk construction.
    void push(const Vec2d& posM, F64 altM, F64 speedMs, F64 marginM = 0.0) {
        points.push_back({posM, altM, speedMs, marginM});
    }
};

// ---------------------------------------------------------------------------
// FlightPathFollower
// ---------------------------------------------------------------------------

/// What the follower did this step.
enum class FlightFollowState : U8 {
    no_path = 0,  ///< nothing to fly; the caller should hold the last command
    following = 1,
    arrived = 2,   ///< within the capture radius of the last point
    off_track = 3, ///< cross-track error blew up; the caller should re-plan
};

struct FlightFollowerTuning {
    /// Lookahead as a time at the current speed, then clamped to the band
    /// below, expressed as multiples of the turn radius. Time-based is what
    /// stops a fast aircraft from weaving and a slow one from cutting.
    F64 lookaheadTimeS{9.0};
    F64 minLookaheadFactor{0.6};  ///< x turn radius
    F64 maxLookaheadFactor{4.0};  ///< x turn radius

    /// Capture radius at the end of an open path, as a multiple of the turn
    /// radius.
    F64 arrivalFactor{0.5};

    /// Cross-track error, as a multiple of the turn radius, past which the
    /// follower gives up and asks for a re-plan. Generous, because a legal
    /// turn can put an aircraft a full radius off a corner and that is not
    /// an error.
    F64 maxCrossTrackFactor{6.0};

    /// Fraction of the commanded speed retained while the bank is saturated.
    /// Aircraft do slow in a hard turn, and it is also what stops pure
    /// pursuit oscillating on a tight corner.
    F64 turnSpeedFactor{0.85};

    /// How far ahead the vertical channel looks, as a multiple of the
    /// horizontal lookahead. Larger, because an aircraft must start its
    /// climb well before the terrain that needs it — the planner already
    /// built the ramps, and this makes the follower fly them rather than
    /// chase them.
    F64 verticalLeadFactor{2.0};
};

class FlightPathFollower {
  public:
    FlightPathFollower() = default;

    void set_tuning(const FlightFollowerTuning& tuning) { _tuning = tuning; }
    [[nodiscard]] auto tuning() const -> const FlightFollowerTuning& {
        return _tuning;
    }

    /// Adopt a path and reset the cursor. The path is copied: a follower
    /// outliving its planner is normal, and the alternative is a dangling
    /// pointer waiting to happen.
    void set_path(const FlightPath& path);

    void clear_path();

    [[nodiscard]] auto has_path() const -> bool {
        return _path.valid && _path.points.size() >= 2;
    }

    [[nodiscard]] auto path() const -> const FlightPath& { return _path; }

    /// Arclength of the cursor (m), and the current cross-track error.
    [[nodiscard]] auto arclength_m() const -> F64 { return _sM; }
    [[nodiscard]] auto cross_track_m() const -> F64 { return _crossTrackM; }
    [[nodiscard]] auto remaining_m() const -> F64;

    /// Snap the cursor to the nearest point on the path. Call once after
    /// set_path() when the aircraft is already airborne somewhere along it —
    /// which is exactly the baked-instance case.
    void snap_to_nearest(const Vec2d& posM);

    /// Advance the cursor and produce this step's controls.
    auto update(const FlightState& state, const FlightEnvelope& env, F64 dtS,
                FlightControls& outControls) -> FlightFollowState;

  private:
    /// Forward-only window search. A global nearest-point search would
    /// teleport the cursor a lap backwards the moment an orbit closed on
    /// itself, which is precisely the case this exists to survive.
    void advance_cursor(const Vec2d& posM, F64 maxAdvanceM);

    [[nodiscard]] auto lookahead_m(const FlightState& state,
                                   const FlightEnvelope& env) const -> F64;

    FlightPath _path;
    FlightFollowerTuning _tuning{};

    F64 _sM{0.0};
    F64 _crossTrackM{0.0};
    U32 _segment{0};
    U32 _laps{0};
};

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

/// Resample a polyline at a fixed arclength step, always keeping both
/// endpoints. Degenerate input (fewer than two distinct points) comes back
/// unchanged.
[[nodiscard]] auto resample_polyline(const Vector<Vec2d>& pointsM, F64 stepM)
    -> Vector<Vec2d>;

/// A closed circle of `turns` laps around centreM, starting at the point of
/// the circle nearest entryM so the aircraft rolls straight into it.
/// `segmentsPerTurn` controls the polygonal approximation; 24 gives a
/// 15-degree step, which is finer than the follower's lookahead resolves.
[[nodiscard]] auto make_orbit_polyline(const Vec2d& centreM, F64 radiusM,
                                       const Vec2d& entryM, U32 turns,
                                       bool clockwise,
                                       U32 segmentsPerTurn = 24)
    -> Vector<Vec2d>;

/// Total planar length of a polyline (m).
[[nodiscard]] auto polyline_length_m(const Vector<Vec2d>& pointsM) -> F64;

} // namespace nv

#endif
