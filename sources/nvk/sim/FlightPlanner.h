// File: nvk/sim/FlightPlanner.h
//
// Turns a horizontal route into a flyable trajectory.
//
// The horizontal part of an air route is easy — airspace is mostly empty, so
// a waypoint list is a route. All the difficulty is vertical, and it comes
// from one observation: aircraft do not follow the ground. They fly levels,
// join them with ramps, and round the knees between the two. A profile that
// tracks the terrain reads as a camera drone however good the flight model
// underneath it is.
//
// The profile is built in five passes:
//
//   1. Floor.   floor[i] = terrain.max_height_in(p[i], protectionRadius)
//               + minAgl. The radius is what makes this a corridor rather
//               than a point sample, and it is the single most important
//               number in the whole file.
//
//   2. Closure. Two sweeps enforce the climb and descent gradient limits:
//               forward, the profile may not fall faster than tanDescent;
//               backward, it must already be high enough to reach what comes
//               next at no more than tanClimb. Because each constraint is a
//               one-sided Lipschitz bound and both passes only ever raise
//               the profile, two sweeps are provably enough — the same
//               structure as a two-pass distance transform. The result is
//               the *minimum feasible* altitude everywhere.
//
//   3. Plateaus. Aircraft fly levels, not minima. A sliding-window maximum
//               of width minPlateauLen is rounded up to the next flight
//               level, with half a level of hysteresis so the profile does
//               not chatter between two adjacent bands.
//
//   4. Re-closure. Run pass 2 again with the staircase as the floor. This
//               inserts the ramps for free and in the right place: a step up
//               makes the backward sweep raise the samples before it, so the
//               climb starts early, and a step down makes the forward sweep
//               raise the samples after it, so the descent is gradual.
//
//   5. Rounding. A moving average whose width is the vertical-acceleration
//               limit converts every knee into a parabola, which is exactly
//               the level-off an aircraft flies.
//
// The floor is padded before pass 2 by however much pass 5 can lower a knee,
// so the smoothing has room to work without ever eating into terrain
// clearance. The pad is checked afterwards, and a violation is logged rather
// than asserted: a route through a cliff face should degrade to a steep
// profile with a warning, not take the bake down.

#ifndef _NV_FLIGHTPLANNER_H_
#define _NV_FLIGHTPLANNER_H_

#include <nvk/sim/FlightModel.h>
#include <nvk/sim/FlightPath.h>
#include <nvk/sim/TerrainField.h>
#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {

// ---------------------------------------------------------------------------
// FlightProfileDesc
// ---------------------------------------------------------------------------

/// Everything the vertical planner needs about one aircraft category. All of
/// it comes off the aircraft type's YAML block.
struct FlightProfileDesc {
    /// Arclength between profile samples (m). 250 for a transport, 100 for a
    /// helicopter working close to the ground.
    F64 sampleStepM{250.0};

    /// Radius of the terrain corridor the aircraft is protected in (m).
    /// 2000 for an airliner, 150 for a helicopter.
    F64 protectionRadiusM{2000.0};

    /// Height demanded above the corridor's highest obstacle (m).
    F64 minAglM{900.0};

    /// Flight level quantisation (m). 300 is close enough to 1000 ft.
    F64 levelStepM{300.0};

    /// Per-aircraft offset applied to the level grid (m). This is the whole
    /// of the deconfliction scheme: two aircraft assigned different offsets
    /// cruise at different altitudes and physically cannot collide, however
    /// their ground tracks cross.
    F64 levelOffsetM{0.0};

    /// Shortest run the planner will hold a level for (m). Below this it
    /// simply climbs to the higher of the two neighbours instead of stepping
    /// down and straight back up.
    F64 minPlateauLenM{15000.0};

    /// Gradient limits. Taken from the envelope by make_profile_desc().
    F64 maxClimbAngleRad{0.14};
    F64 maxDescentAngleRad{0.09};

    /// Ceiling on the whole profile (m MSL). A route over a volcano still
    /// has to stay inside the aircraft's service ceiling.
    F64 maxAltM{12000.0};

    /// Floor on the whole profile (m MSL), applied after everything else.
    /// Stops a route over open sea from being planned at 150 m just because
    /// the water is flat.
    F64 minAltM{0.0};

    /// Requested cruise altitude (m MSL). When positive the planner will not
    /// step below it except where terrain forces it higher. Zero derives the
    /// cruise from the terrain alone.
    F64 cruiseAltM{0.0};

    /// Vertical acceleration used to round the knees (m/s^2), and the speed
    /// they are rounded at. Together these set the moving-average width.
    F64 verticalAccelMs2{0.4};
    F64 roundingSpeedMs{120.0};

    /// Emit a diagnostic SVG of the profile to this path when non-empty.
    String profileSvgPath;
};

/// Derive the gradient and rounding numbers from an envelope, leaving the
/// terrain-facing ones (radius, AGL, plateau length) to the caller.
[[nodiscard]] auto make_profile_desc(const FlightEnvelope& env)
    -> FlightProfileDesc;

// ---------------------------------------------------------------------------
// FlightPlanner
// ---------------------------------------------------------------------------

class FlightPlanner {
  public:
    /// The terrain is borrowed, not owned; it must outlive the planner.
    /// Passing nullptr is legal and yields flat profiles at cruiseAltM,
    /// which is what a world baked without a ceiling raster gets.
    explicit FlightPlanner(const TerrainField* terrain = nullptr);

    void set_terrain(const TerrainField* terrain) { _terrain = terrain; }
    [[nodiscard]] auto terrain() const -> const TerrainField* {
        return _terrain;
    }

    /// Build a full trajectory from a horizontal waypoint list.
    /// `closed` marks an orbit, which changes nothing here but is carried
    /// through onto the FlightPath for the follower.
    [[nodiscard]] auto build_path(const Vector<Vec2d>& waypointsM,
                                  const FlightProfileDesc& desc,
                                  F64 cruiseSpeedMs, bool closed = false) const
        -> FlightPath;

    /// Build an orbit around a point at a fixed altitude offset above the
    /// local terrain ceiling. Used by the survey missions, where the point
    /// of the leg is to stay at a legible height above one feature rather
    /// than at a flight level.
    [[nodiscard]] auto build_orbit(const Vec2d& centreM, F64 radiusM,
                                   const Vec2d& entryM, U32 turns,
                                   bool clockwise,
                                   const FlightProfileDesc& desc,
                                   F64 speedMs) const -> FlightPath;

    // ── Profile passes, exposed for the unit tests ──────────────────────

    /// Sample the terrain floor along an already-resampled polyline.
    [[nodiscard]] auto sample_floor(const Vector<Vec2d>& samplesM,
                                    const FlightProfileDesc& desc) const
        -> Vector<F64>;

    /// Raise `alt` until it clears `floor` everywhere and satisfies both
    /// gradient limits. Only ever raises, and two sweeps are sufficient.
    static void lipschitz_closure(Vector<F64>& alt, const Vector<F64>& floorM,
                                  F64 stepM, F64 tanClimb, F64 tanDescent);

    /// Replace a minimum-altitude profile with a staircase of flight levels.
    static void quantize_plateaus(Vector<F64>& alt, F64 stepM,
                                  F64 minPlateauLenM, F64 levelStepM,
                                  F64 levelOffsetM);

    /// Moving average of half-width `halfWindow` samples, endpoints held.
    static void smooth_profile(Vector<F64>& alt, U32 halfWindow);

    /// Sliding-window maximum, O(n) via a monotonic deque.
    [[nodiscard]] static auto window_max(const Vector<F64>& src, U32 halfWindow)
        -> Vector<F64>;

    // ── Diagnostics ─────────────────────────────────────────────────────

    /// Distance / altitude dump: terrain, floor, and the final profile. The
    /// fastest way to tune minPlateauLenM without opening the editor.
    void dump_profile_svg(const String& path, const Vector<Vec2d>& samplesM,
                          const Vector<F64>& terrainM,
                          const Vector<F64>& floorM, const Vector<F64>& altM,
                          F64 stepM) const;

  private:
    const TerrainField* _terrain{nullptr};
};

} // namespace nv

#endif
