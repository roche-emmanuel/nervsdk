// File: nvk/sim/FlightModel.cpp

#include <nvk/sim/FlightModel.h>

namespace nv {

namespace {

/// Wrap to (-pi, pi].
auto wrap_pi(F64 angleRad) -> F64 {
    F64 res = std::fmod(angleRad + PI, 2.0 * PI);
    if (res <= 0.0) {
        res += 2.0 * PI;
    }
    return res - PI;
}

/// First-order lag with an exponential kernel rather than an Euler step, so
/// the response is identical whatever sub-step the caller ends up with and a
/// long step cannot overshoot the target.
auto lag_toward(F64 value, F64 target, F64 tauS, F64 dtS) -> F64 {
    if (tauS <= 1e-6) {
        return target;
    }
    const F64 alpha = 1.0 - std::exp(-dtS / tauS);
    return value + (target - value) * alpha;
}

auto move_toward(F64 value, F64 target, F64 maxDelta) -> F64 {
    const F64 diff = target - value;
    if (std::abs(diff) <= maxDelta) {
        return target;
    }
    return value + std::copysign(maxDelta, diff);
}

} // namespace

// ---------------------------------------------------------------------------
// FlightEnvelope
// ---------------------------------------------------------------------------

auto FlightEnvelope::usable_bank_rad() const -> F64 {
    F64 bank = maxBankRad;

    if (maxLoadFactor > 1.0) {
        // n = 1 / cos(bank) in a level turn, so the load factor caps the
        // bank at acos(1/n).
        const F64 loadBank = std::acos(std::min(1.0, 1.0 / maxLoadFactor));
        bank = std::min(bank, loadBank);
    }

    // A hair under 90 degrees, or the tangent below goes to infinity.
    return std::clamp(bank, 0.0, 1.5);
}

auto FlightEnvelope::turn_radius_m(F64 speedMs) const -> F64 {
    const F64 speed = std::max(speedMs, minTurnSpeedMs);
    const F64 bank = usable_bank_rad();
    const F64 tanBank = std::tan(bank);
    if (tanBank < 1e-6) {
        return std::numeric_limits<F64>::max();
    }
    return speed * speed / (kGravityMs2 * tanBank);
}

auto make_fixed_wing_envelope(F64 cruiseSpeedMs, F64 minSpeedMs, F64 maxBankDeg)
    -> FlightEnvelope {
    NVCHK(cruiseSpeedMs > 0.0,
          "make_fixed_wing_envelope: cruise speed must be positive, got {}",
          cruiseSpeedMs);

    FlightEnvelope env;
    env.cruiseSpeedMs = cruiseSpeedMs;
    env.minSpeedMs = minSpeedMs > 0.0 ? minSpeedMs : cruiseSpeedMs * 0.45;
    env.maxSpeedMs = cruiseSpeedMs * 1.25;

    env.maxBankRad = maxBankDeg * PI / 180.0;
    env.maxBankRateRadS = 0.15;
    env.maxLoadFactor = 1.0 / std::cos(std::min(env.maxBankRad, 1.4)) + 0.05;

    // Bigger and faster means slower to respond, in every axis. Scaling the
    // time constants off the cruise speed keeps one set of numbers usable
    // from a Cessna to a widebody.
    const F64 sizeFactor = std::clamp(cruiseSpeedMs / 120.0, 0.4, 2.5);
    env.accelTauS = 12.0 * sizeFactor;
    env.decelTauS = 8.0 * sizeFactor;
    env.trackTauS = 5.0 * sizeFactor;
    env.altTauS = 10.0 * sizeFactor;

    env.maxClimbRateMs = 12.0;
    env.maxDescentRateMs = 9.0;
    env.maxClimbAngleRad = 0.14;
    env.maxDescentAngleRad = 0.09;
    env.maxVerticalAccelMs2 = 0.6;

    env.maxCrabRad = 0.0;
    env.trimPitchRad = 0.035;

    return env;
}

auto make_rotorcraft_envelope(F64 cruiseSpeedMs, F64 maxBankDeg)
    -> FlightEnvelope {
    NVCHK(cruiseSpeedMs > 0.0,
          "make_rotorcraft_envelope: cruise speed must be positive, got {}",
          cruiseSpeedMs);

    FlightEnvelope env;
    env.minSpeedMs = 0.0; // the flag that makes this a rotorcraft
    env.cruiseSpeedMs = cruiseSpeedMs;
    env.maxSpeedMs = cruiseSpeedMs * 1.3;

    env.maxBankRad = maxBankDeg * PI / 180.0;
    env.maxBankRateRadS = 0.35;
    env.maxLoadFactor = 1.6;

    env.accelTauS = 5.0;
    env.decelTauS = 4.0;
    env.trackTauS = 3.0;
    env.altTauS = 5.0;
    env.minTurnSpeedMs = 3.0;

    env.maxClimbRateMs = 8.0;
    env.maxDescentRateMs = 6.0;
    env.maxClimbAngleRad = 0.7; // near enough vertical; a helicopter can
    env.maxDescentAngleRad = 0.6;
    env.maxVerticalAccelMs2 = 1.2;

    env.maxCrabRad = 0.35;
    env.maxYawRateRadS = 0.5;
    env.pitchPerAccelSRad = 0.06;
    env.trimPitchRad = -0.05; // nose slightly down in the cruise

    return env;
}

// ---------------------------------------------------------------------------
// FlightModel
// ---------------------------------------------------------------------------

FlightModel::FlightModel(FlightEnvelope envelope) : _env(envelope) {
    NVCHK(_env.cruiseSpeedMs > 0.0,
          "FlightModel: cruise speed must be positive, got {}",
          _env.cruiseSpeedMs);
    NVCHK(_env.maxSpeedMs >= _env.minSpeedMs,
          "FlightModel: max speed {} is below min speed {}", _env.maxSpeedMs,
          _env.minSpeedMs);
}

void FlightModel::set_envelope(const FlightEnvelope& envelope) {
    _env = envelope;
}

void FlightModel::set_max_sub_step(F64 dtS) {
    NVCHK(dtS > 0.0, "FlightModel: sub-step must be positive, got {}", dtS);
    _maxSubStepS = dtS;
}

void FlightModel::reset(const Vec3d& posM, F64 trackRad, F64 speedMs) {
    _state = FlightState{};
    _state.posM = posM;
    _state.trackRad = wrap_pi(trackRad);
    _state.headingRad = _state.trackRad;
    _state.speedMs = std::clamp(speedMs, _env.minSpeedMs, _env.maxSpeedMs);
    _state.pitchRad = _env.trimPitchRad;
    _lastAccelMs2 = 0.0;
}

void FlightModel::update(const FlightControls& controls, F64 dtS) {
    if (dtS <= 0.0) {
        return;
    }

    // A hitch is split rather than integrated in one go: the roll rate limit
    // is the only thing keeping the bank bounded, and a 0.5 s step would let
    // a full-scale track error roll straight past it.
    const auto steps = U32(std::max(1.0, std::ceil(dtS / _maxSubStepS)));
    const F64 sub = dtS / F64(steps);

    for (U32 i = 0; i < steps; ++i) {
        step(controls, sub);
    }
}

auto FlightModel::bank_for_rate(F64 rateRadS, F64 speedMs) const -> F64 {
    const F64 speed = std::max(speedMs, _env.minTurnSpeedMs);
    return std::atan2(speed * rateRadS, kGravityMs2);
}

auto FlightModel::rate_for_bank(F64 bankRad, F64 speedMs) const -> F64 {
    const F64 speed = std::max(speedMs, _env.minTurnSpeedMs);
    return kGravityMs2 * std::tan(bankRad) / speed;
}

void FlightModel::step(const FlightControls& controls, F64 dtS) {
    // ── Speed ───────────────────────────────────────────────────────────
    const F64 targetSpeed =
        std::clamp(controls.targetSpeedMs, _env.minSpeedMs, _env.maxSpeedMs);
    const F64 tauS =
        targetSpeed > _state.speedMs ? _env.accelTauS : _env.decelTauS;

    const F64 prevSpeed = _state.speedMs;
    _state.speedMs = lag_toward(_state.speedMs, targetSpeed, tauS, dtS);
    _lastAccelMs2 = (_state.speedMs - prevSpeed) / dtS;

    // ── Roll and turn ───────────────────────────────────────────────────
    //
    // Demand a turn rate from the track error, convert it to the bank that
    // rate would need, rate-limit the roll, then derive the turn actually
    // achieved from the bank now held. Ordering it this way is the whole
    // trick: the aircraft rolls first and turns second, which is what the
    // eye reads as flight.
    const F64 trackErr = wrap_pi(controls.targetTrackRad - _state.trackRad);
    const F64 demandRate = trackErr / std::max(_env.trackTauS, 1e-3);

    const F64 usableBank = _env.usable_bank_rad();
    F64 bankTarget = bank_for_rate(demandRate, _state.speedMs);
    bankTarget = std::clamp(bankTarget, -usableBank, usableBank);

    _state.bankRad =
        move_toward(_state.bankRad, bankTarget, _env.maxBankRateRadS * dtS);

    _state.turnRateRadS = rate_for_bank(_state.bankRad, _state.speedMs);

    // A rotorcraft below its turn-speed floor pivots on the spot instead:
    // banking a hovering helicopter would translate it, not turn it.
    if (_env.is_rotorcraft() && _state.speedMs < _env.minTurnSpeedMs) {
        _state.turnRateRadS =
            std::clamp(demandRate, -_env.maxYawRateRadS, _env.maxYawRateRadS);
        _state.bankRad =
            move_toward(_state.bankRad, 0.0, _env.maxBankRateRadS * dtS);
    }

    _state.trackRad = wrap_pi(_state.trackRad + _state.turnRateRadS * dtS);

    // ── Vertical ────────────────────────────────────────────────────────
    const F64 altErr = controls.targetAltM - _state.posM.z();

    F64 vsDemand = altErr / std::max(_env.altTauS, 1e-3);
    vsDemand =
        std::clamp(vsDemand, -_env.maxDescentRateMs, _env.maxClimbRateMs);

    // The flight path angle is the binding limit at low speed. Applying it
    // after the rate limit rather than instead of it means a slow aircraft
    // climbs shallowly and a fast one is held by the rate.
    const F64 climbCap = _state.speedMs * std::tan(_env.maxClimbAngleRad);
    const F64 descentCap = _state.speedMs * std::tan(_env.maxDescentAngleRad);
    vsDemand = std::clamp(vsDemand, -descentCap, climbCap);

    _state.verticalSpeedMs = move_toward(_state.verticalSpeedMs, vsDemand,
                                         _env.maxVerticalAccelMs2 * dtS);

    // ── Integrate position ──────────────────────────────────────────────
    const F64 dx = std::cos(_state.trackRad) * _state.speedMs * dtS;
    const F64 dy = std::sin(_state.trackRad) * _state.speedMs * dtS;
    const F64 dz = _state.verticalSpeedMs * dtS;

    _state.posM.set(_state.posM.x() + dx, _state.posM.y() + dy,
                    _state.posM.z() + dz);

    // ── Attitude ────────────────────────────────────────────────────────
    //
    // Pitch is the flight path angle plus trim. It is a *display* quantity
    // derived from the state, never integrated, so it cannot drift away from
    // the trajectory the way an independently filtered pitch would.
    const F64 gammaRad = std::asin(std::clamp(
        _state.verticalSpeedMs / std::max(_state.speedMs, 1e-3), -1.0, 1.0));
    F64 pitch = gammaRad + _env.trimPitchRad;

    if (_env.is_rotorcraft()) {
        // Tilt the disc to accelerate: nose down to speed up, nose up to
        // slow down. Without this a helicopter reads as a hovering prop.
        pitch -= _env.pitchPerAccelSRad * _lastAccelMs2;
    }

    _state.pitchRad = pitch;

    // ── Heading ─────────────────────────────────────────────────────────
    if (_env.is_rotorcraft() && controls.wants_heading_hold()) {
        // Point the nose where asked, within the crab limit once there is
        // meaningful airspeed — a helicopter can look sideways at 20 kn but
        // not at 120.
        F64 target = controls.targetHeadingRad;
        if (_state.speedMs > _env.minTurnSpeedMs && _env.maxCrabRad > 0.0) {
            const F64 crab = wrap_pi(target - _state.trackRad);
            target = _state.trackRad +
                     std::clamp(crab, -_env.maxCrabRad, _env.maxCrabRad);
        }
        const F64 err = wrap_pi(target - _state.headingRad);
        _state.headingRad = wrap_pi(_state.headingRad +
                                    std::clamp(err, -_env.maxYawRateRadS * dtS,
                                               _env.maxYawRateRadS * dtS));
    } else {
        _state.headingRad = _state.trackRad;
    }
}

auto FlightModel::pose() const -> AircraftPose {
    AircraftPose out;
    out.posM = _state.posM;
    out.headingRad = _state.headingRad;
    out.pitchRad = _state.pitchRad;
    out.bankRad = _state.bankRad;
    return out;
}

auto FlightModel::project_ahead_m(F64 seconds) const -> Vec3d {
    if (seconds <= 0.0) {
        return _state.posM;
    }

    const F64 dist = _state.speedMs * seconds;
    const F64 rate = _state.turnRateRadS;
    const F64 dz = _state.verticalSpeedMs * seconds;

    // Straight line when the turn is negligible; the arc form below is
    // ill-conditioned there.
    if (std::abs(rate) < 1e-5) {
        return {_state.posM.x() + std::cos(_state.trackRad) * dist,
                _state.posM.y() + std::sin(_state.trackRad) * dist,
                _state.posM.z() + dz};
    }

    // Constant-rate arc: the centre sits one radius abeam, and the aircraft
    // sweeps rate*seconds around it.
    const F64 radius = _state.speedMs / rate;
    const F64 endTrack = _state.trackRad + rate * seconds;

    const F64 cx = _state.posM.x() - radius * std::sin(_state.trackRad);
    const F64 cy = _state.posM.y() + radius * std::cos(_state.trackRad);

    return {cx + radius * std::sin(endTrack), cy - radius * std::cos(endTrack),
            _state.posM.z() + dz};
}

} // namespace nv
