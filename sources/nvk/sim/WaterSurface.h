#ifndef _NV_WATERSURFACE_H_
#define _NV_WATERSURFACE_H_

#include <nvk/math/Vec2.h>
#include <nvk/math/Vec3.h>
#include <nvk_common.h>
#include <nvk_math.h>
#include <nvk_types.h>

namespace nv {

/// Standard gravity used by every water-surface model (m/s²).
constexpr F64 WATER_GRAVITY = 9.80665;

/// Density of sea water at 15 °C (kg/m³). Not used by the kinematic buoyancy
/// model, but needed by any future force-based solver.
constexpr F64 SEA_WATER_DENSITY = 1025.0;

/// One sample of the water surface at a single horizontal position and time.
///
/// Coordinate convention — matches UE world axes:
///   x = East, y = North, z = Up, all in **metres**.
///   z = 0 is mean sea level (the UE heightmap encodes sea level at raw 32768,
///   which maps to world Z = 0).
struct WaterSample {
    /// Surface elevation above mean sea level (m).
    F64 height{0.0};

    /// Unit surface normal, pointing up out of the water.
    Vec3d normal{0.0, 0.0, 1.0};

    /// Orbital velocity of the water particle at the surface (m/s).
    ///
    /// Gerstner particle orbits are closed, so a particle followed over a
    /// whole period has exactly zero net drift. Averaging this field at a
    /// *fixed world position* is a different quantity and does not vanish:
    /// uniform sampling in world space weights source points by the Jacobian
    /// (1 − q k A sin θ), leaving a small Eulerian mean of −Σ q² ω k A² / 2
    /// directed against the wave. On a 1 m sea that is a few millimetres per
    /// second. Anything driving vessel drift from this field should use the
    /// Lagrangian interpretation, not the Eulerian average.
    Vec3d velocity{};
};

/// Abstract water-surface query interface.
///
/// This is the seam that lets buoyancy stay engine-agnostic: nvk ships the
/// Gerstner implementation below (option A — YAML owns the wave spec), while
/// other layers can supply an alternative implementation backed by the UE Water
/// plugin (option B) without any change to BuoyancyModel.
///
/// Implementations must be thread-safe for concurrent const access: sample()
/// is called from the game thread today, but the offline placement tools may
/// call it from worker threads.
class WaterSurface {
  public:
    WaterSurface() = default;
    WaterSurface(const WaterSurface&) = delete;
    auto operator=(const WaterSurface&) -> WaterSurface& = delete;
    WaterSurface(WaterSurface&&) = delete;
    auto operator=(WaterSurface&&) -> WaterSurface& = delete;
    virtual ~WaterSurface() = default;

    /// Full sample: elevation, normal and orbital velocity.
    /// @param posM   horizontal world position (m)
    /// @param timeS  simulation time (s)
    [[nodiscard]] virtual auto sample(const Vec2d& posM, F64 timeS) const
        -> WaterSample = 0;

    /// Elevation-only fast path. The default forwards to sample(); override
    /// when the implementation can skip the normal / velocity work.
    [[nodiscard]] virtual auto height_at(const Vec2d& posM, F64 timeS) const
        -> F64 {
        return sample(posM, timeS).height;
    }

    /// Significant wave height Hs (m) — the mean height of the highest third
    /// of the waves, and the standard way to describe a sea state.
    [[nodiscard]] virtual auto significant_wave_height() const -> F64 = 0;
};

/// Perfectly flat water at mean sea level. Useful as a baseline in tests and
/// as the degenerate case when waves are disabled entirely.
class FlatWaterSurface : public WaterSurface {
  public:
    [[nodiscard]] auto sample(const Vec2d& posM, F64 timeS) const
        -> WaterSample override;

    [[nodiscard]] auto height_at(const Vec2d& posM, F64 timeS) const
        -> F64 override;

    [[nodiscard]] auto significant_wave_height() const -> F64 override {
        return 0.0;
    }
};

/// One Gerstner (trochoidal) wave component.
///
/// The surface is the parametric map of a horizontal source position `s`:
///   xy(s, t) = s + Σ dir_i · (q · A_i · cos θ_i)
///   z (s, t) =     Σ         A_i · sin θ_i
/// with θ_i = k_i (dir_i · s) − ω_i t + φ_i.
///
/// `q` is the trochoid fraction: the horizontal orbital amplitude of a real
/// deep-water wave equals its vertical amplitude, so q = 1 is the physical
/// wave and q = 0 collapses the model to a plain sum of sines with equally
/// rounded crests and troughs. Values above 1 put more horizontal than
/// vertical motion into the water, which no real wave does and which inflates
/// the Stokes drift quadratically — build_waves() clamps to [0, 1].
///
/// Σ q k_i A_i must additionally stay ≤ 1 or the trochoid folds over on
/// itself; build_waves() enforces that too.
struct GerstnerWave {
    /// Unit horizontal propagation direction.
    Vec2d dir{1.0, 0.0};

    /// Amplitude (m) — half the crest-to-trough height of this component.
    F64 amplitude{0.0};

    /// Wavelength (m).
    F64 wavelength{1.0};

    /// Phase offset (rad).
    F64 phase{0.0};

    /// Trochoid fraction q (see above). Dimensionless, in [0, 1].
    F64 steepness{0.0};

    /// Derived: wave number k = 2π / wavelength (rad/m).
    F64 waveNumber{0.0};

    /// Derived: angular frequency ω = sqrt(g k) (rad/s), deep-water dispersion.
    F64 omega{0.0};

    /// Recompute waveNumber and omega from wavelength. Call after changing
    /// wavelength directly.
    void update_derived();

    /// Period of this component (s).
    [[nodiscard]] auto period() const -> F64 {
        return omega > 0.0 ? (2.0 * PI / omega) : 0.0;
    }
};

/// Parameters used to synthesise a Gerstner wave set.
///
/// The intent is that this maps one-to-one onto a YAML block, so the same
/// numbers can later drive a UE `UGerstnerWaterWaves` asset at bake time.
struct GerstnerWaterConfig {
    /// Douglas sea state (0 = glassy … 9 = phenomenal). Used only when
    /// significantWaveHeightM is negative.
    I32 seaState{2};

    /// Explicit significant wave height (m). Negative → derive from seaState.
    F64 significantWaveHeightM{-1.0};

    /// Explicit peak wavelength (m). Negative → derive from Hs assuming a
    /// fully developed sea.
    F64 peakWavelengthM{-1.0};

    /// Number of wave components summed. 4–6 is plenty for a calm sea.
    U32 numWaves{5};

    /// Mean propagation direction, radians CCW from +x (East).
    F64 directionRad{0.0};

    /// Half-angle of the directional spread around directionRad (rad).
    /// 0 gives a perfectly regular swell; ~0.5 rad looks like a natural sea.
    F64 directionSpreadRad{0.5};

    /// Trochoid fraction in [0, 1]: 0 = plain sum of sines, 1 = a true
    /// deep-water trochoid. Reduced automatically if the requested value would
    /// let the surface fold over on itself.
    F64 steepness{0.4};

    /// Ratio between the longest and the shortest wavelength in the set.
    F64 wavelengthSpread{3.0};

    /// Seed for the component phases and direction jitter. Fixed by default so
    /// that a given world config always produces the same sea.
    U32 seed{1234};
};

/// Sum-of-Gerstner-waves surface, synthesised from a GerstnerWaterConfig.
///
/// The component amplitudes are normalised so that the resulting surface has
/// exactly the requested significant wave height: for a sum of sinusoids with
/// independent phases the elevation variance is σ² = Σ A_i²/2, and Hs = 4σ,
/// hence Σ A_i² = Hs²/8. That relation is directly asserted in the spec.
class GerstnerWaterSurface : public WaterSurface {
  public:
    explicit GerstnerWaterSurface(const GerstnerWaterConfig& cfg = {});

    /// Build directly from an explicit component set — used by tests that need
    /// a single, exactly known wave.
    explicit GerstnerWaterSurface(Vector<GerstnerWave> waves);

    [[nodiscard]] auto sample(const Vec2d& posM, F64 timeS) const
        -> WaterSample override;

    [[nodiscard]] auto height_at(const Vec2d& posM, F64 timeS) const
        -> F64 override;

    [[nodiscard]] auto significant_wave_height() const -> F64 override {
        return _hs;
    }

    [[nodiscard]] auto waves() const -> const Vector<GerstnerWave>& {
        return _waves;
    }

    [[nodiscard]] auto config() const -> const GerstnerWaterConfig& {
        return _cfg;
    }

    /// Trochoid fraction actually applied, after clamping to [0, 1] and after
    /// the anti-folding guard. Equals config().steepness unless a guard fired.
    [[nodiscard]] auto effective_steepness() const -> F64 {
        return _waves.empty() ? 0.0 : _waves[0].steepness;
    }

    /// Fixed-point iteration count currently used to invert the horizontal
    /// displacement. Derived from the spectrum; see set_inversion_iterations().
    [[nodiscard]] auto inversion_iterations() const -> U32 { return _invIters; }

    /// Longest period in the component set (s) — handy for sizing the settling
    /// time of tests and for logging.
    [[nodiscard]] auto max_period() const -> F64;

    /// Number of fixed-point iterations used to invert the horizontal
    /// displacement when steepness > 0. 3 is ample at realistic steepness.
    void set_inversion_iterations(U32 count) { _invIters = maximum(1U, count); }

  private:
    void build_waves();

    /// Horizontal displacement applied to a source position (m).
    [[nodiscard]] auto displacement(const Vec2d& srcM, F64 timeS) const
        -> Vec2d;

    /// Recover the source position whose displaced position is posM.
    [[nodiscard]] auto invert_position(const Vec2d& posM, F64 timeS) const
        -> Vec2d;

    /// Recompute _hs and _hasSteepness from the current component set.
    void refresh_derived_state();

    GerstnerWaterConfig _cfg{};
    Vector<GerstnerWave> _waves;
    F64 _hs{0.0};

    /// True when at least one component has non-zero steepness, i.e. when the
    /// horizontal displacement has to be inverted before sampling.
    bool _hasSteepness{false};

    U32 _invIters{3};
};

/// Representative significant wave height (m) for a Douglas sea state.
/// Values are the mid-points of the standard bands; the scale is clamped to
/// [0, 9].
[[nodiscard]] auto sea_state_wave_height(I32 seaState) -> F64;

/// Peak wavelength (m) of a fully developed sea with the given significant
/// wave height, via the Pierson-Moskowitz relations
/// Hs ≈ 0.0246 U² and Tp ≈ 0.73 U, then λ = g Tp² / 2π.
[[nodiscard]] auto fully_developed_wavelength(F64 significantHeightM) -> F64;

} // namespace nv

#endif
