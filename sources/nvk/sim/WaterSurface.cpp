#include <nvk/base/RandGen.h>
#include <nvk/sim/WaterSurface.h>

namespace nv {

// ─────────────────────────────────────────────────────────────────────────────
// FlatWaterSurface
// ─────────────────────────────────────────────────────────────────────────────

auto FlatWaterSurface::sample(const Vec2d& posM, F64 timeS) const
    -> WaterSample {
    (void)posM;
    (void)timeS;
    return {};
}

auto FlatWaterSurface::height_at(const Vec2d& posM, F64 timeS) const -> F64 {
    (void)posM;
    (void)timeS;
    return 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Free helpers
// ─────────────────────────────────────────────────────────────────────────────

void GerstnerWave::update_derived() {
    NVCHK(wavelength > 0.0, "GerstnerWave: wavelength must be positive, got {}",
          wavelength);
    waveNumber = 2.0 * PI / wavelength;
    omega = std::sqrt(WATER_GRAVITY * waveNumber);
}

auto sea_state_wave_height(I32 seaState) -> F64 {
    // Douglas scale band mid-points (m). Band 0 is glassy calm.
    static const F64 heights[] = {0.0,  0.05, 0.3, 0.875, 1.875,
                                  3.25, 5.0,  7.5, 11.5,  16.0};
    constexpr I32 maxState = I32(sizeof(heights) / sizeof(heights[0])) - 1;
    return heights[clamp(seaState, 0, maxState)];
}

auto fully_developed_wavelength(F64 significantHeightM) -> F64 {
    if (significantHeightM <= 0.0) {
        return 1.0;
    }
    // Pierson-Moskowitz: Hs ≈ 0.0246 U²  →  U = sqrt(Hs / 0.0246)
    const F64 windSpeed = std::sqrt(significantHeightM / 0.0246);
    // Peak period Tp ≈ 0.73 U, then deep-water λ = g Tp² / 2π.
    const F64 peakPeriod = 0.73 * windSpeed;
    const F64 lambda = WATER_GRAVITY * peakPeriod * peakPeriod / (2.0 * PI);
    return maximum(lambda, 0.5);
}

// ─────────────────────────────────────────────────────────────────────────────
// GerstnerWaterSurface
// ─────────────────────────────────────────────────────────────────────────────

GerstnerWaterSurface::GerstnerWaterSurface(const GerstnerWaterConfig& cfg)
    : _cfg(cfg) {
    build_waves();
}

GerstnerWaterSurface::GerstnerWaterSurface(Vector<GerstnerWave> waves)
    : _waves(std::move(waves)) {
    for (auto& wave : _waves) {
        wave.update_derived();
    }
    refresh_derived_state();
}

void GerstnerWaterSurface::refresh_derived_state() {
    // Hs = 4σ with σ² = Σ A_i² / 2.
    F64 sumSquares = 0.0;
    _hasSteepness = false;
    for (const auto& wave : _waves) {
        sumSquares += wave.amplitude * wave.amplitude;
        if (wave.steepness > 0.0 && wave.amplitude > 0.0) {
            _hasSteepness = true;
        }
    }
    _hs = 4.0 * std::sqrt(0.5 * sumSquares);
}

void GerstnerWaterSurface::build_waves() {
    const F64 targetHs = _cfg.significantWaveHeightM >= 0.0
                             ? _cfg.significantWaveHeightM
                             : sea_state_wave_height(_cfg.seaState);

    const F64 peakLambda = _cfg.peakWavelengthM > 0.0
                               ? _cfg.peakWavelengthM
                               : fully_developed_wavelength(targetHs);

    const U32 numWaves = maximum(1U, _cfg.numWaves);
    const F64 spread = maximum(1.0, _cfg.wavelengthSpread);

    _waves.clear();
    _waves.reserve(numWaves);

    if (targetHs <= 0.0) {
        // Glassy calm: keep one zero-amplitude component so that the sampling
        // code path stays uniform, and report Hs = 0.
        GerstnerWave wave;
        wave.wavelength = peakLambda;
        wave.update_derived();
        _waves.push_back(wave);
        _hs = 0.0;
        _hasSteepness = false;
        return;
    }

    RandGen rng(_cfg.seed);

    // Wavelengths ride a geometric ladder centred on the peak, running from
    // peak/sqrt(spread) up to peak*sqrt(spread).
    const F64 logHalfRange = 0.5 * std::log(spread);

    Vector<F64> weights;
    weights.reserve(numWaves);

    for (U32 i = 0; i < numWaves; ++i) {
        GerstnerWave wave;

        const F64 t =
            numWaves == 1 ? 0.0 : (2.0 * F64(i) / F64(numWaves - 1) - 1.0);

        wave.wavelength = peakLambda * std::exp(t * logHalfRange);
        wave.update_derived();

        // Direction: fan the components across the spread, with a little
        // jitter so the set never looks like a regular comb.
        const F64 jitter =
            rng.uniform_real<F64>(-0.15, 0.15) * _cfg.directionSpreadRad;
        const F64 angle = _cfg.directionRad + t * _cfg.directionSpreadRad +
                          jitter;
        wave.dir.set(std::cos(angle), std::sin(angle));

        wave.phase = rng.uniform_real<F64>(0.0, 2.0 * PI);

        // Energy weight: Gaussian falloff in log-wavelength about the peak, so
        // most of the energy sits near the peak component.
        const F64 logRatio = t * logHalfRange;
        const F64 sigma = 0.5;
        weights.push_back(
            std::exp(-0.5 * (logRatio * logRatio) / (sigma * sigma)));

        _waves.push_back(wave);
    }

    // Normalise amplitudes so that Σ A_i² = Hs² / 8, which makes Hs exact.
    F64 sumWeightSquares = 0.0;
    for (F64 weight : weights) {
        sumWeightSquares += weight * weight;
    }
    NVCHK(sumWeightSquares > 0.0,
          "GerstnerWaterSurface: degenerate amplitude weights.");

    const F64 amplitudeScale =
        std::sqrt((targetHs * targetHs / 8.0) / sumWeightSquares);

    for (U32 i = 0; i < numWaves; ++i) {
        _waves[i].amplitude = weights[i] * amplitudeScale;
    }

    // Distribute the requested steepness across the set so that
    // Σ q_i k_i A_i == cfg.steepness ≤ 1, which is the self-intersection bound.
    // Steepness q is the fraction of the *physical* trochoid horizontal
    // displacement applied, shared by every component: q = 1 reproduces a real
    // deep-water wave, whose horizontal orbital amplitude equals its vertical
    // amplitude. Anything above 1 would put more horizontal than vertical
    // motion into the wave, which inflates the Stokes drift quadratically for
    // no physical reason, so the knob is clamped to [0, 1].
    //
    // Self-intersection additionally requires Σ q kᵢ Aᵢ ≤ 1, and the
    // fixed-point inversion in invert_position() contracts by exactly that
    // factor — so a spectrum sitting right on the folding bound would also be
    // one the inversion cannot solve. Cap well below it, which keeps both
    // comfortable. On a realistic sea Σ kᵢ Aᵢ is around 0.15 and the guard
    // never fires; only a hand-authored short-and-tall spectrum trips it.
    constexpr F64 maxContraction = 0.6;

    F64 effectiveSteepness = clamp(_cfg.steepness, 0.0, 1.0);

    F64 sumKA = 0.0;
    for (const auto& wave : _waves) {
        sumKA += wave.waveNumber * wave.amplitude;
    }

    if (sumKA > 0.0 && effectiveSteepness * sumKA > maxContraction) {
        const F64 capped = maxContraction / sumKA;
        logWARN("GerstnerWaterSurface: steepness {:.3f} on a spectrum with "
                "sum k·A = {:.3f} would approach the trochoid folding bound; "
                "reducing to {:.3f}.",
                effectiveSteepness, sumKA, capped);
        effectiveSteepness = capped;
    }

    for (auto& wave : _waves) {
        wave.steepness = effectiveSteepness;
    }

    // The fixed-point inversion in invert_position() contracts by exactly
    // Σ q kᵢ Aᵢ per iteration. Pick the iteration count from that so the
    // sampled elevation error stays in the millimetre range whatever the
    // spectrum, without paying for iterations a gentle sea does not need.
    const F64 contraction = effectiveSteepness * sumKA;
    if (contraction <= 0.0) {
        _invIters = 1;
    } else {
        const F64 needed = std::log(1e-3) / std::log(minimum(contraction, 0.9));
        _invIters = clamp(U32(std::ceil(needed)), 2U, 8U);
    }

    refresh_derived_state();
}

auto GerstnerWaterSurface::max_period() const -> F64 {
    F64 longest = 0.0;
    for (const auto& wave : _waves) {
        longest = maximum(longest, wave.period());
    }
    return longest;
}

auto GerstnerWaterSurface::displacement(const Vec2d& srcM, F64 timeS) const
    -> Vec2d {
    Vec2d result(0.0, 0.0);
    for (const auto& wave : _waves) {
        const F64 theta = wave.waveNumber * wave.dir.dot(srcM) -
                          wave.omega * timeS + wave.phase;
        result += wave.dir * (wave.steepness * wave.amplitude * std::cos(theta));
    }
    return result;
}

auto GerstnerWaterSurface::invert_position(const Vec2d& posM, F64 timeS) const
    -> Vec2d {
    // Solve src + displacement(src) = posM by fixed-point iteration. The map
    // is a contraction whenever Σ q_i k_i A_i < 1, which build_waves()
    // guarantees, so a handful of iterations converges to well below the
    // amplitude scale.
    Vec2d src = posM;
    for (U32 iter = 0; iter < _invIters; ++iter) {
        src = posM - displacement(src, timeS);
    }
    return src;
}

auto GerstnerWaterSurface::sample(const Vec2d& posM, F64 timeS) const
    -> WaterSample {
    const Vec2d src = _hasSteepness ? invert_position(posM, timeS) : posM;

    WaterSample result;

    F64 height = 0.0;
    F64 normalX = 0.0;
    F64 normalY = 0.0;
    F64 slopeSum = 0.0;
    F64 velX = 0.0;
    F64 velY = 0.0;
    F64 velZ = 0.0;

    for (const auto& wave : _waves) {
        const F64 theta = wave.waveNumber * wave.dir.dot(src) -
                          wave.omega * timeS + wave.phase;
        const F64 sinTheta = std::sin(theta);
        const F64 cosTheta = std::cos(theta);

        const F64 ka = wave.waveNumber * wave.amplitude;

        height += wave.amplitude * sinTheta;

        // Analytic normal of the Gerstner surface (GPU Gems 1, ch. 1).
        normalX -= wave.dir.x() * ka * cosTheta;
        normalY -= wave.dir.y() * ka * cosTheta;
        slopeSum += wave.steepness * ka * sinTheta;

        // Orbital velocity = ∂P/∂t, with ∂θ/∂t = −ω.
        velX += wave.dir.x() * wave.steepness * wave.amplitude * wave.omega *
                sinTheta;
        velY += wave.dir.y() * wave.steepness * wave.amplitude * wave.omega *
                sinTheta;
        velZ -= wave.amplitude * wave.omega * cosTheta;
    }

    result.height = height;
    result.normal = Vec3d(normalX, normalY, 1.0 - slopeSum).normalized();
    result.velocity.set(velX, velY, velZ);

    return result;
}

auto GerstnerWaterSurface::height_at(const Vec2d& posM, F64 timeS) const
    -> F64 {
    const Vec2d src = _hasSteepness ? invert_position(posM, timeS) : posM;

    F64 height = 0.0;
    for (const auto& wave : _waves) {
        const F64 theta = wave.waveNumber * wave.dir.dot(src) -
                          wave.omega * timeS + wave.phase;
        height += wave.amplitude * std::sin(theta);
    }
    return height;
}

} // namespace nv
