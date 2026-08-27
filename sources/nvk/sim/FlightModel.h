// File: nvk/sim/FlightModel.h
//
// Kinematic flight dynamics — the aircraft counterpart of BuoyancyModel.
//
// Same philosophy as the buoyancy solver: tier 1 is parameterised by envelope
// limits and time constants rather than by mass, thrust and aerodynamic
// coefficients. Nobody authors a lift curve slope for a background aircraft,
// and a solver that needed one would be tuned by guesswork anyway. What does
// read on screen is the *coupling*: the aeroplane banks before it turns, the
// nose rises when it climbs, speed lags the throttle, and none of the three
// happens instantly.
//
// The one relation that makes it look like flight rather than a dolly move is
// the coordinated turn:
//
//     tan(bank) = v * omega / g
//
// The model therefore does not steer and then bank to match. It rolls toward
// the bank the demanded turn would need, rate-limited, and then derives the
// turn actually achieved from the bank it currently holds. An aircraft that
// is wings-level does not turn, however hard the autopilot is asking. That is
// the aviation equivalent of "the turn radius is what makes a ship a ship".
//
// Frames and signs
// ----------------
//   * Positions are metres; z is altitude above mean sea level.
//   * Heading and track are radians, measured from +x toward +y. In the UE
//     raster frame (x East, y South) that is a compass turn to the right, so
//     a positive heading rate is a right turn.
//   * Positive bank is *into* a positive heading rate — i.e. right wing down
//     for a right turn. In the UE frame that maps straight onto actor Roll
//     with no sign flip. A caller working in a right-handed y-North frame
//     must negate it.
//   * Positive pitch is nose up, which is UE Pitch directly.
//
// Horizontal *routing* is not owned here: FlightControls is supplied by the
// caller, normally by FlightPathFollower. This model only answers "given what
// the autopilot is asking for, what attitude and position result".

#ifndef _NV_FLIGHTMODEL_H_
#define _NV_FLIGHTMODEL_H_

#include <nvk/math/Vec2.h>
#include <nvk/math/Vec3.h>
#include <nvk_common.h>
#include <nvk_math.h>
#include <nvk_types.h>

namespace nv {

/// Standard gravity (m/s^2).
inline constexpr F64 kGravityMs2 = 9.80665;

/// One knot in metres per second.
inline constexpr F64 kKnotToMs = 0.514444444;

/// One foot per minute in metres per second.
inline constexpr F64 kFpmToMs = 0.00508;

// ---------------------------------------------------------------------------
// FlightEnvelope
// ---------------------------------------------------------------------------

/// What a given airframe can and will do. Everything is a limit or a time
/// constant; nothing here is a force.
struct FlightEnvelope {
    // ── Speed ───────────────────────────────────────────────────────────

    /// Zero marks a rotorcraft: it may hover, and its heading decouples from
    /// its track. Anything positive is a fixed-wing aircraft, which cannot
    /// fly slower than this and whose nose always points along its track.
    F64 minSpeedMs{60.0};
    F64 cruiseSpeedMs{120.0};
    F64 maxSpeedMs{160.0};

    /// First-order speed lag. Separate constants because an airframe adds
    /// energy far more slowly than it sheds it.
    F64 accelTauS{14.0};
    F64 decelTauS{9.0};

    // ── Turning ─────────────────────────────────────────────────────────

    /// Largest bank the autopilot will command. 25 deg is a transport in
    /// normal law, 45 deg a light aircraft being flown with intent.
    F64 maxBankRad{0.436};

    /// How fast the aircraft rolls between bank angles. This is what makes
    /// the roll-in visible instead of instantaneous.
    F64 maxBankRateRadS{0.15};

    /// Structural / comfort load factor. Caps the bank jointly with
    /// maxBankRad through n = 1 / cos(bank).
    F64 maxLoadFactor{1.4};

    /// Time constant of the track-capture law: the demanded turn rate is the
    /// track error divided by this. Longer looks more deliberate.
    F64 trackTauS{6.0};

    /// Speed floor used when converting bank to turn rate, so a hovering
    /// rotorcraft does not compute an infinite rate.
    F64 minTurnSpeedMs{5.0};

    // ── Vertical ────────────────────────────────────────────────────────

    F64 maxClimbRateMs{12.0};
    F64 maxDescentRateMs{8.0}; ///< magnitude, positive

    /// Flight path angle limits. The binding constraint at low speed: an
    /// aircraft at 60 m/s cannot make 12 m/s up without standing on its
    /// tail, and the angle limit is what stops it trying.
    F64 maxClimbAngleRad{0.14};
    F64 maxDescentAngleRad{0.10};

    /// Vertical acceleration limit. Rounds every level-off into a parabola
    /// instead of a corner.
    F64 maxVerticalAccelMs2{0.6};

    /// Time constant of the altitude-capture law.
    F64 altTauS{12.0};

    // ── Rotorcraft only ─────────────────────────────────────────────────

    /// Largest angle between the nose and the track. Zero locks the nose to
    /// the track, which is what a fixed-wing does.
    F64 maxCrabRad{0.35};

    /// Yaw rate available when pointing the nose independently of the track
    /// (pedal turns, hovering onto a heading).
    F64 maxYawRateRadS{0.5};

    /// Nose-down pitch per unit of longitudinal acceleration. A helicopter
    /// accelerates by tilting its disc, and this is the only reason it looks
    /// like one rather than like a floating box.
    F64 pitchPerAccelSRad{0.06};

    /// Body pitch held in level unaccelerated flight. Small nose-up for a
    /// transport, small nose-down for a helicopter in the cruise.
    F64 trimPitchRad{0.0};

    [[nodiscard]] auto is_rotorcraft() const -> bool {
        return minSpeedMs <= 1e-6;
    }

    /// Bank actually usable, after the load-factor cap.
    [[nodiscard]] auto usable_bank_rad() const -> F64;

    /// Radius of a level turn at `speedMs` and full bank (m).
    [[nodiscard]] auto turn_radius_m(F64 speedMs) const -> F64;
};

/// Sensible defaults for the two families. Both take the numbers the config
/// actually carries and derive the rest, so a YAML block stays short.
[[nodiscard]] auto make_fixed_wing_envelope(F64 cruiseSpeedMs, F64 minSpeedMs,
                                            F64 maxBankDeg) -> FlightEnvelope;

[[nodiscard]] auto make_rotorcraft_envelope(F64 cruiseSpeedMs,
                                            F64 maxBankDeg) -> FlightEnvelope;

// ---------------------------------------------------------------------------
// FlightControls
// ---------------------------------------------------------------------------

/// What the autopilot is asking for this step. Produced by
/// FlightPathFollower, or by anything else that wants to fly the aircraft.
struct FlightControls {
    /// Commanded airspeed (m/s). Clamped into the envelope by the model.
    F64 targetSpeedMs{0.0};

    /// Commanded ground track (rad).
    F64 targetTrackRad{0.0};

    /// Commanded altitude (m MSL).
    F64 targetAltM{0.0};

    /// Rotorcraft only: commanded nose heading (rad). Leave as NaN — the
    /// default — to let the nose follow the track.
    F64 targetHeadingRad{std::numeric_limits<F64>::quiet_NaN()};

    [[nodiscard]] auto wants_heading_hold() const -> bool {
        return std::isfinite(targetHeadingRad);
    }
};

// ---------------------------------------------------------------------------
// FlightState / AircraftPose
// ---------------------------------------------------------------------------

struct FlightState {
    /// World position (m); z is altitude MSL.
    Vec3d posM{};

    /// Direction of travel over the ground (rad).
    F64 trackRad{0.0};

    /// Direction the nose points (rad). Equal to trackRad for fixed wing.
    F64 headingRad{0.0};

    /// Airspeed, which with no wind model is also ground speed (m/s).
    F64 speedMs{0.0};

    /// Positive up (m/s).
    F64 verticalSpeedMs{0.0};

    /// Positive right-wing-down (rad).
    F64 bankRad{0.0};

    /// Positive nose-up (rad).
    F64 pitchRad{0.0};

    /// Achieved turn rate (rad/s), diagnostics and traffic projection.
    F64 turnRateRadS{0.0};

    [[nodiscard]] auto pos2_m() const -> Vec2d {
        return {posM.x(), posM.y()};
    }
};

/// What the renderer needs. Deliberately the same shape as BoatPose.
struct AircraftPose {
    Vec3d posM{};
    F64 headingRad{0.0};
    F64 pitchRad{0.0};
    F64 bankRad{0.0};
};

// ---------------------------------------------------------------------------
// FlightModel
// ---------------------------------------------------------------------------

class FlightModel {
  public:
    explicit FlightModel(FlightEnvelope envelope);

    [[nodiscard]] auto envelope() const -> const FlightEnvelope& {
        return _env;
    }

    void set_envelope(const FlightEnvelope& envelope);

    [[nodiscard]] auto state() const -> const FlightState& { return _state; }

    /// Place the aircraft with zero rates. Call once at spawn so it does not
    /// visibly snap into a bank on the first tick.
    void reset(const Vec3d& posM, F64 trackRad, F64 speedMs);

    /// Advance by dtS seconds, subdividing internally so a frame hitch
    /// cannot let the roll integrator overshoot.
    void update(const FlightControls& controls, F64 dtS);

    [[nodiscard]] auto pose() const -> AircraftPose;

    void set_max_sub_step(F64 dtS);

    /// Where the aircraft will be in `seconds` if it holds its current turn
    /// rate and speed. Used by the traffic layer's separation test, which is
    /// why it is an arc rather than a straight line: two aircraft in a
    /// holding pattern are permanently turning.
    [[nodiscard]] auto project_ahead_m(F64 seconds) const -> Vec3d;

  private:
    void step(const FlightControls& controls, F64 dtS);

    /// Bank the aircraft would need to hold to fly the demanded turn rate.
    [[nodiscard]] auto bank_for_rate(F64 rateRadS, F64 speedMs) const -> F64;

    /// Turn rate implied by the bank currently held.
    [[nodiscard]] auto rate_for_bank(F64 bankRad, F64 speedMs) const -> F64;

    FlightEnvelope _env;
    FlightState _state{};

    /// Longitudinal acceleration over the last sub-step, for the rotorcraft
    /// pitch response.
    F64 _lastAccelMs2{0.0};

    F64 _maxSubStepS{0.05};
};

} // namespace nv

#endif
