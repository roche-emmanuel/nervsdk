#ifndef _NV_BUOYANCYMODEL_H_
#define _NV_BUOYANCYMODEL_H_

#include <nvk/math/Vec2.h>
#include <nvk/math/Vec3.h>
#include <nvk/sim/WaterSurface.h>
#include <nvk_common.h>
#include <nvk_math.h>
#include <nvk_types.h>

namespace nv {

/// One hull sampling probe, expressed in the boat body frame.
///
/// Body frame: x = forward (toward the bow), y = starboard, z = up.
struct HullProbe {
    /// Horizontal offset from the hull reference point (m).
    Vec2d offsetM{};

    /// Relative weight in the least-squares surface fit. Probes near the ends
    /// of the hull can be weighted up to make the vessel follow long swell
    /// more eagerly.
    F64 weight{1.0};
};

/// Static description of a vessel hull, as consumed by BuoyancyModel.
///
/// This is the tier-1 *kinematic* parameterisation: the vessel is characterised
/// by the natural period and damping of each free-surface degree of freedom
/// rather than by mass and inertia. Mass never appears — for a force-based
/// solver it would come out of Archimedes (displaced volume × ρ) anyway, not
/// from an authored number.
struct HullDesc {
    /// Overall dimensions (m). Used to build the default probe layout and to
    /// sanity-check against the mesh bounding box at bake time.
    F64 lengthM{10.0};
    F64 beamM{3.0};
    F64 draftM{1.0};

    /// Signed distance from the mesh pivot **up** to the design waterline (m).
    ///   0        → pivot authored at the waterline
    ///   = draftM → pivot authored at the keel
    /// The rendered pivot sits at waterElevation − pivotToWaterlineM.
    F64 pivotToWaterlineM{0.0};

    /// Natural periods of the three free-surface degrees of freedom (s).
    /// Heave and pitch are typically close and shorter than roll.
    F64 heavePeriodS{2.0};
    F64 pitchPeriodS{2.5};
    F64 rollPeriodS{3.5};

    /// Damping ratio ζ shared by the three DOFs. 1 = critically damped;
    /// 0.3–0.5 gives the slight overshoot that reads as a real hull.
    F64 dampingRatio{0.45};

    /// Fraction of the local surface slope actually followed. A hull is long
    /// relative to short chop and averages it out, so gains below 1 look far
    /// more convincing than a rigid slope match.
    F64 pitchGain{0.8};
    F64 rollGain{0.9};

    /// Hard clamps on the attitude response (rad).
    F64 maxPitchRad{0.26};
    F64 maxRollRad{0.44};

    /// Hull probe layout. Empty is invalid — BuoyancyModel asserts on it.
    Vector<HullProbe> probes;
};

/// Instantaneous free-surface state of a vessel.
struct BuoyancyState {
    /// Elevation of the hull reference point above mean sea level (m).
    F64 heaveM{0.0};
    F64 heaveVelMs{0.0};

    /// Pitch angle, bow-up positive (rad).
    F64 pitchRad{0.0};
    F64 pitchVelRs{0.0};

    /// Roll angle, starboard-down positive (rad).
    F64 rollRad{0.0};
    F64 rollVelRs{0.0};
};

/// World pose to hand to the renderer.
struct BoatPose {
    /// World position of the mesh pivot (m), z included.
    Vec3d posM{};

    /// Heading, radians CCW from +x (East).
    F64 headingRad{0.0};

    /// Pitch, bow-up positive (rad).
    F64 pitchRad{0.0};

    /// Roll, starboard-down positive (rad).
    F64 rollRad{0.0};
};

/// Kinematic buoyancy solver.
///
/// Each update the model samples the water surface at the hull probes, fits a
/// weighted least-squares plane through the samples in the body frame, and
/// drives heave / pitch / roll toward that plane through three independent
/// damped harmonic oscillators. The oscillators are what give the vessel
/// apparent mass: without them the hull snaps rigidly to the surface and reads
/// as a decal rather than a boat.
///
/// Horizontal motion is *not* owned here — posM and headingRad are supplied by
/// the caller (a route follower, or a fixed mooring position). This model only
/// answers "given where the boat is, how does it sit on the water".
///
/// The interface is deliberately identical to what a force-based 6-DOF solver
/// would need, so tier 2 can replace the internals without touching the end
/// engine.
class BuoyancyModel {
  public:
    explicit BuoyancyModel(HullDesc hull);

    /// The surface is borrowed, not owned; it must outlive the model. Passing
    /// nullptr is legal and freezes the model at its current state.
    void set_water_surface(const WaterSurface* surface) { _surface = surface; }

    [[nodiscard]] auto water_surface() const -> const WaterSurface* {
        return _surface;
    }

    [[nodiscard]] auto hull() const -> const HullDesc& { return _hull; }

    [[nodiscard]] auto state() const -> const BuoyancyState& { return _state; }

    /// Snap directly onto the water surface with zero velocities. Call once at
    /// spawn so the vessel does not visibly drop into place on the first tick.
    void reset(const Vec2d& posM, F64 headingRad, F64 timeS);

    /// Advance the model by dtS seconds. timeS is the simulation time at the
    /// **end** of the step.
    void update(const Vec2d& posM, F64 headingRad, F64 dtS, F64 timeS);

    /// Compose the current state with a horizontal position and heading.
    [[nodiscard]] auto pose(const Vec2d& posM, F64 headingRad) const
        -> BoatPose;

    /// Largest integration sub-step (s). update() subdivides longer steps so
    /// that a frame hitch cannot destabilise the oscillators.
    void set_max_sub_step(F64 dtS);

  private:
    /// Local water plane in the body frame: height at the reference point plus
    /// the two slopes.
    struct SurfaceFit {
        F64 heightM{0.0};
        F64 slopeFwd{0.0};
        F64 slopeStbd{0.0};
    };

    [[nodiscard]] auto fit_surface(const Vec2d& posM, F64 headingRad,
                                   F64 timeS) const -> SurfaceFit;

    [[nodiscard]] auto pitch_target(const SurfaceFit& fit) const -> F64;
    [[nodiscard]] auto roll_target(const SurfaceFit& fit) const -> F64;

    void integrate_dof(F64& value, F64& velocity, F64 target, F64 omega,
                       F64 dtS) const;

    void refresh_frequencies();

    HullDesc _hull;
    const WaterSurface* _surface{nullptr};
    BuoyancyState _state{};

    F64 _heaveOmega{0.0};
    F64 _pitchOmega{0.0};
    F64 _rollOmega{0.0};
    F64 _maxSubStepS{0.02};
};

/// Build the default probe layout: bow, stern, port, starboard and centre.
/// @param inset  fraction of the half-length / half-beam at which the end
///               probes sit; below 1 so they stay on the wetted hull rather
///               than on the extreme bounding-box corners.
[[nodiscard]] auto make_box_hull_probes(F64 lengthM, F64 beamM,
                                        F64 inset = 0.85) -> Vector<HullProbe>;

/// Natural period (s) of a hull degree of freedom under Froude similitude:
/// T = coeff · sqrt(L). Typical coefficients — fishing craft: roll 0.95,
/// pitch 0.60, heave 0.55; cargo / tanker: roll 1.60, pitch 0.75, heave 0.70.
[[nodiscard]] inline auto froude_period(F64 lengthM, F64 coeff) -> F64 {
    return coeff * std::sqrt(maximum(lengthM, 0.01));
}

/// Assemble a HullDesc from dimensions plus Froude coefficients, with the
/// default probe layout already installed.
[[nodiscard]] auto make_hull_desc(F64 lengthM, F64 beamM, F64 draftM,
                                  F64 rollPeriodCoeff = 0.95,
                                  F64 pitchPeriodCoeff = 0.60,
                                  F64 heavePeriodCoeff = 0.55) -> HullDesc;

} // namespace nv

#endif
