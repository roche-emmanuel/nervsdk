#include <nvk/sim/BuoyancyModel.h>

namespace nv {

// ─────────────────────────────────────────────────────────────────────────────
// Free helpers
// ─────────────────────────────────────────────────────────────────────────────

auto make_box_hull_probes(F64 lengthM, F64 beamM, F64 inset)
    -> Vector<HullProbe> {
    const F64 halfLength =
        0.5 * maximum(lengthM, 0.01) * clamp(inset, 0.1, 1.0);
    const F64 halfBeam = 0.5 * maximum(beamM, 0.01) * clamp(inset, 0.1, 1.0);

    Vector<HullProbe> probes;
    probes.reserve(5);

    // Bow and stern carry the pitch response, port and starboard the roll
    // response, and the centre probe anchors the heave fit.
    probes.push_back({.offsetM = Vec2d(halfLength, 0.0), .weight = 1.0});
    probes.push_back({.offsetM = Vec2d(-halfLength, 0.0), .weight = 1.0});
    probes.push_back({.offsetM = Vec2d(0.0, halfBeam), .weight = 1.0});
    probes.push_back({.offsetM = Vec2d(0.0, -halfBeam), .weight = 1.0});
    probes.push_back({.offsetM = Vec2d(0.0, 0.0), .weight = 2.0});

    return probes;
}

auto make_hull_desc(F64 lengthM, F64 beamM, F64 draftM, F64 rollPeriodCoeff,
                    F64 pitchPeriodCoeff, F64 heavePeriodCoeff) -> HullDesc {
    HullDesc hull;
    hull.lengthM = lengthM;
    hull.beamM = beamM;
    hull.draftM = draftM;
    hull.rollPeriodS = froude_period(lengthM, rollPeriodCoeff);
    hull.pitchPeriodS = froude_period(lengthM, pitchPeriodCoeff);
    hull.heavePeriodS = froude_period(lengthM, heavePeriodCoeff);
    hull.probes = make_box_hull_probes(lengthM, beamM);
    return hull;
}

// ─────────────────────────────────────────────────────────────────────────────
// BuoyancyModel
// ─────────────────────────────────────────────────────────────────────────────

BuoyancyModel::BuoyancyModel(HullDesc hull) : _hull(std::move(hull)) {
    NVCHK(!_hull.probes.empty(),
          "BuoyancyModel: hull has no probes — call make_box_hull_probes().");
    NVCHK(_hull.heavePeriodS > 0.0 && _hull.pitchPeriodS > 0.0 &&
              _hull.rollPeriodS > 0.0,
          "BuoyancyModel: all natural periods must be positive "
          "(heave={}, pitch={}, roll={}).",
          _hull.heavePeriodS, _hull.pitchPeriodS, _hull.rollPeriodS);

    refresh_frequencies();
}

void BuoyancyModel::refresh_frequencies() {
    _heaveOmega = 2.0 * PI / _hull.heavePeriodS;
    _pitchOmega = 2.0 * PI / _hull.pitchPeriodS;
    _rollOmega = 2.0 * PI / _hull.rollPeriodS;

    // Keep the default sub-step comfortably inside the stability limit of the
    // fastest oscillator: ω·dt ≲ 0.2 keeps semi-implicit Euler well behaved.
    const F64 fastestOmega =
        maximum(_heaveOmega, maximum(_pitchOmega, _rollOmega));
    _maxSubStepS = clamp(0.2 / fastestOmega, 0.002, 0.02);
}

void BuoyancyModel::set_max_sub_step(F64 dtS) {
    NVCHK(dtS > 0.0, "BuoyancyModel: sub-step must be positive, got {}", dtS);
    _maxSubStepS = dtS;
}

auto BuoyancyModel::fit_surface(const Vec2d& posM, F64 headingRad,
                                F64 timeS) const -> SurfaceFit {
    SurfaceFit fit;

    if (_surface == nullptr) {
        return fit;
    }

    const Vec2d forward(std::cos(headingRad), std::sin(headingRad));
    const Vec2d starboard = forward.cw90();

    // Weighted least-squares fit of z = a·x + b·y + c over the probes, with
    // (x, y) in the body frame. The normal equations are the symmetric 3×3
    // system below.
    F64 sumW = 0.0;
    F64 sumWx = 0.0;
    F64 sumWy = 0.0;
    F64 sumWxx = 0.0;
    F64 sumWyy = 0.0;
    F64 sumWxy = 0.0;
    F64 sumWz = 0.0;
    F64 sumWxz = 0.0;
    F64 sumWyz = 0.0;

    for (const auto& probe : _hull.probes) {
        const F64 bodyX = probe.offsetM.x();
        const F64 bodyY = probe.offsetM.y();
        const Vec2d worldPos = posM + forward * bodyX + starboard * bodyY;
        const F64 height = _surface->height_at(worldPos, timeS);
        const F64 weight = probe.weight;

        sumW += weight;
        sumWx += weight * bodyX;
        sumWy += weight * bodyY;
        sumWxx += weight * bodyX * bodyX;
        sumWyy += weight * bodyY * bodyY;
        sumWxy += weight * bodyX * bodyY;
        sumWz += weight * height;
        sumWxz += weight * bodyX * height;
        sumWyz += weight * bodyY * height;
    }

    NVCHK(sumW > 0.0, "BuoyancyModel: probe weights sum to zero.");

    // Solve
    //   | sumWxx sumWxy sumWx | |a|   |sumWxz|
    //   | sumWxy sumWyy sumWy | |b| = |sumWyz|
    //   | sumWx  sumWy  sumW  | |c|   |sumWz |
    const F64 m00 = sumWxx;
    const F64 m01 = sumWxy;
    const F64 m02 = sumWx;
    const F64 m11 = sumWyy;
    const F64 m12 = sumWy;
    const F64 m22 = sumW;

    const F64 cof00 = m11 * m22 - m12 * m12;
    const F64 cof01 = m02 * m12 - m01 * m22;
    const F64 cof02 = m01 * m12 - m02 * m11;

    const F64 det = m00 * cof00 + m01 * cof01 + m02 * cof02;

    // A degenerate layout (all probes collinear, or a single probe) still gives
    // a usable heave from the weighted mean; the slopes simply stay zero.
    if (std::abs(det) < 1e-12) {
        fit.heightM = sumWz / sumW;
        return fit;
    }

    const F64 cof11 = m00 * m22 - m02 * m02;
    const F64 cof12 = m01 * m02 - m00 * m12;
    const F64 cof22 = m00 * m11 - m01 * m01;

    const F64 invDet = 1.0 / det;

    fit.slopeFwd = invDet * (cof00 * sumWxz + cof01 * sumWyz + cof02 * sumWz);
    fit.slopeStbd = invDet * (cof01 * sumWxz + cof11 * sumWyz + cof12 * sumWz);
    fit.heightM = invDet * (cof02 * sumWxz + cof12 * sumWyz + cof22 * sumWz);

    return fit;
}

auto BuoyancyModel::pitch_target(const SurfaceFit& fit) const -> F64 {
    // Water rising toward the bow (slopeFwd > 0) lifts the bow: pitch positive.
    const F64 raw = std::atan(fit.slopeFwd) * _hull.pitchGain;
    return clamp(raw, -_hull.maxPitchRad, _hull.maxPitchRad);
}

auto BuoyancyModel::roll_target(const SurfaceFit& fit) const -> F64 {
    // Water rising to starboard lifts the starboard side, and roll is positive
    // starboard-**down**, hence the sign flip.
    const F64 raw = -std::atan(fit.slopeStbd) * _hull.rollGain;
    return clamp(raw, -_hull.maxRollRad, _hull.maxRollRad);
}

void BuoyancyModel::integrate_dof(F64& value, F64& velocity, F64 target,
                                  F64 omega, F64 dtS) const {
    // Damped harmonic oscillator, semi-implicit (symplectic) Euler:
    //   a = ω²(target − x) − 2ζω·v
    // Velocity is updated first so the position uses the new velocity, which
    // is what keeps the scheme stable at the sub-step sizes we use.
    const F64 accel = omega * omega * (target - value) -
                      2.0 * _hull.dampingRatio * omega * velocity;
    velocity += accel * dtS;
    value += velocity * dtS;
}

void BuoyancyModel::reset(const Vec2d& posM, F64 headingRad, F64 timeS) {
    const SurfaceFit fit = fit_surface(posM, headingRad, timeS);

    _state.heaveM = fit.heightM;
    _state.pitchRad = pitch_target(fit);
    _state.rollRad = roll_target(fit);

    _state.heaveVelMs = 0.0;
    _state.pitchVelRs = 0.0;
    _state.rollVelRs = 0.0;
}

void BuoyancyModel::update(const Vec2d& posM, F64 headingRad, F64 dtS,
                           F64 timeS) {
    if (dtS <= 0.0 || _surface == nullptr) {
        return;
    }

    // The targets move on the wave timescale (seconds), so one fit per update
    // is ample even when the step is subdivided; only the integration needs
    // the smaller steps.
    const SurfaceFit fit = fit_surface(posM, headingRad, timeS);

    const F64 heaveTarget = fit.heightM;
    const F64 pitchTarget = pitch_target(fit);
    const F64 rollTarget = roll_target(fit);

    // Subdivide long frames. The cap keeps a pathological hitch (an editor
    // breakpoint, a streaming stall) from turning into hundreds of sub-steps.
    constexpr U32 maxSubSteps = 16;
    U32 numSteps = U32(std::ceil(dtS / _maxSubStepS));
    numSteps = clamp(numSteps, 1U, maxSubSteps);

    const F64 subDt = dtS / F64(numSteps);

    for (U32 step = 0; step < numSteps; ++step) {
        integrate_dof(_state.heaveM, _state.heaveVelMs, heaveTarget,
                      _heaveOmega, subDt);
        integrate_dof(_state.pitchRad, _state.pitchVelRs, pitchTarget,
                      _pitchOmega, subDt);
        integrate_dof(_state.rollRad, _state.rollVelRs, rollTarget, _rollOmega,
                      subDt);
    }

    // The oscillators can overshoot past the clamped targets; keep the final
    // attitude inside the declared limits and kill the velocity that drove it
    // there, so the hull settles rather than grinding against the clamp.
    if (std::abs(_state.pitchRad) > _hull.maxPitchRad) {
        _state.pitchRad =
            clamp(_state.pitchRad, -_hull.maxPitchRad, _hull.maxPitchRad);
        _state.pitchVelRs = 0.0;
    }
    if (std::abs(_state.rollRad) > _hull.maxRollRad) {
        _state.rollRad =
            clamp(_state.rollRad, -_hull.maxRollRad, _hull.maxRollRad);
        _state.rollVelRs = 0.0;
    }
}

auto BuoyancyModel::pose(const Vec2d& posM, F64 headingRad) const -> BoatPose {
    BoatPose result;
    result.posM.set(posM.x(), posM.y(),
                    _state.heaveM - _hull.pivotToWaterlineM);
    result.headingRad = headingRad;
    result.pitchRad = _state.pitchRad;
    result.rollRad = _state.rollRad;
    return result;
}

} // namespace nv
