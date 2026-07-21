#ifndef _OVERTURE_MAPS_H_
#define _OVERTURE_MAPS_H_

#include <nvk/geometry/geometry2d.h>

namespace nv {
// land_use_mask — pixel value is the LandUseClass enum cast to U8.
// 0 = background (no Overture polygon covers this pixel; WorldCover drives
// placement). Values 1-109 correspond to the 109 known Overture land-use
// classes. 109 classes fit in U8 (max value 109 < 255).
//
// Consumers look up the per-class vegetation density factor from the YAML
// vegetation.land_use_density map keyed by class name string.

enum class LandUseClass : U8 {
    unknown = 0, // background / unrecognised
    aboriginal_land = 1,
    airfield = 2,
    allotments = 3,
    animal_keeping = 4,
    aquaculture = 5,
    barracks = 6,
    base = 7,
    beach_resort = 8,
    brownfield = 9,
    bunker = 10,
    camp_site = 11,
    cemetery = 12,
    clinic = 13,
    college = 14,
    commercial = 15,
    connection = 16,
    construction = 17,
    danger_area = 18,
    doctors = 19,
    dog_park = 20,
    downhill = 21,
    driving_range = 22,
    driving_school = 23,
    education = 24,
    environmental = 25,
    fairway = 26,
    farmland = 27,
    farmyard = 28,
    fatbike = 29,
    flowerbed = 30,
    forest = 31,
    garages = 32,
    garden = 33,
    golf_course = 34,
    grass = 35,
    grave_yard = 36,
    green = 37,
    greenfield = 38,
    greenhouse_horticulture = 39,
    highway = 40,
    hike = 41,
    hospital = 42,
    ice_skate = 43,
    industrial = 44,
    institutional = 45,
    kindergarten = 46,
    landfill = 47,
    lateral_water_hazard = 48,
    logging = 49,
    marina = 50,
    meadow = 51,
    military = 52,
    military_hospital = 53,
    military_school = 54,
    music_school = 55,
    national_park = 56,
    natural_monument = 57,
    nature_reserve = 58,
    naval_base = 59,
    nordic = 60,
    nuclear_explosion_site = 61,
    obstacle_course = 62,
    orchard = 63,
    park = 64,
    peat_cutting = 65,
    pedestrian = 66,
    pitch = 67,
    plant_nursery = 68,
    playground = 69,
    plaza = 70,
    protected_area = 71, // "protected" is a C++ keyword
    protected_landscape_seascape = 72,
    quarry = 73,
    railway = 74,
    range = 75,
    recreation_ground = 76,
    religious = 77,
    residential = 78,
    resort = 79,
    retail = 80,
    rough = 81,
    salt_pond = 82,
    school = 83,
    schoolyard = 84,
    ski_jump = 85,
    skitour = 86,
    sled = 87,
    sleigh = 88,
    snow_park = 89,
    species_management_area = 90,
    stadium = 91,
    state_park = 92,
    static_caravan = 93,
    strict_nature_reserve = 94,
    tee = 95,
    theme_park = 96,
    track = 97,
    traffic_island = 98,
    training_area = 99,
    trench = 100,
    university = 101,
    village_green = 102,
    vineyard = 103,
    water_hazard = 104,
    water_park = 105,
    wilderness_area = 106,
    winter_sports = 107,
    works = 108,
    zoo = 109,
};

// Convert an Overture class name string to its enum value.
// Returns LandUseClass::unknown (0) for any unrecognised name.
// Note: "protected" maps to LandUseClass::protected_area to avoid
// the C++ keyword conflict.
auto overture_land_use_class_from_string(const String& name) -> LandUseClass;
auto overture_land_use_class_to_string(LandUseClass luClass) -> String;

// ---------------------------------------------------------------------------
// Overture base/land physical classes
// ---------------------------------------------------------------------------
// Pixel encoding in land_mask.png:
//   0                              background — no Overture land polygon
//   class_value + LAND_CLASS_PIXEL_OFFSET   actual class (range 214–255)
//
// LAND_CLASS_PIXEL_OFFSET = 213  (255 - 42 classes = 213; first class → 214)
// This pushes all real values into the bright upper quarter of the U8 range,
// making the mask visually meaningful when viewed as a greyscale image.

static constexpr U8 LAND_CLASS_PIXEL_OFFSET = 213;

enum class LandClass : U8 {
    unknown = 0, // background / unrecognised (not added to offset)
    archipelago = 1,
    bare_rock = 2,
    beach = 3,
    cave_entrance = 4,
    cliff = 5,
    desert = 6,
    dune = 7,
    fell = 8,
    forest = 9,
    glacier = 10,
    grass = 11,
    grassland = 12,
    heath = 13,
    hill = 14,
    island = 15,
    islet = 16,
    land = 17,
    meadow = 18,
    meteor_crater = 19,
    mountain_range = 20,
    peak = 21,
    peninsula = 22,
    plateau = 23,
    reef = 24,
    ridge = 25,
    rock = 26,
    saddle = 27,
    sand = 28,
    scree = 29,
    scrub = 30,
    shingle = 31,
    shrub = 32,
    shrubbery = 33,
    stone = 34,
    tree = 35,
    tree_row = 36,
    tundra = 37,
    valley = 38,
    volcanic_caldera_rim = 39,
    volcano = 40,
    wetland = 41,
    wood = 42,
};

// Convert an Overture land class name string to its enum value.
// Returns LandClass::unknown (0) for any unrecognised name.
auto overture_land_class_from_string(const String& name) -> LandClass;
auto overture_land_class_to_string(LandClass cls) -> String;

struct CellVertex {
    F32 px, py, pz;
    F32 nx, ny, nz;
    F32 u0, v0;
    F32 texIdx;
};

struct TileGeom {
    Vector<CellVertex> verts;
    Vector<U32> indices;
};

// One road cross-section. left/right are world-XY (cm). z is the shared
// surface elevation (cm, terrain max under the cross-section, WITHOUT the road
// Z bias — tessellation adds that). u is cumulative centreline distance (cm).
//
// Invariant produced by build_ribs(): |left - right| == 2 * halfWidth for every
// rib, and both sides share z, so the surface is always level across its width.
struct RoadRib {
    Vec2d left;
    Vec2d right;
    F64 z{0.0};
    F64 u{0.0};
};

struct RoadConnectorInfos {
    F64 elev{0.0};
    Vec2d pos;
    U32 count{0};
    F64 groundElev{-1e6};
    F32 maxHalfWidth{0.0};
};

// Default along-road grade caps, in degrees. tan(deg) ≈ grade %:
//   4° ≈ 7%, 6° ≈ 10.5%, 8° ≈ 14%, 12° ≈ 21%, 15° ≈ 27%.
constexpr F64 kRoadDefaultMaxSlopeDeg = 8.0; // fallback for unknown classes
constexpr F64 kRoadLinkSlopeBonusDeg = 2.0;  // ramps tolerate a steeper grade
constexpr F64 kDegToRad = 3.14159265358979323846 / 180.0;

// Default maximum along-road grade (degrees) for an Overture road class.
// Unknown classes fall back to kRoadDefaultMaxSlopeDeg.
[[nodiscard]] auto road_default_max_slope_deg(const String& roadClass) -> F64;

// tan() of a grade given in degrees → dimensionless rise/run ratio.
// u and z are both in cm, so this ratio applies directly to the profile.
[[nodiscard]] auto slope_ratio_from_deg(F64 deg) -> F64;

// Raise-only, slope-limited conditioning of an elevation profile.
// Postconditions (for all i): z[i] >= z_in[i]  (never sinks below ground), and
// |z[i+1]-z[i]| <= maxSlope*(u[i+1]-u[i])  (grade cap honoured). The result is
// the LOWEST profile satisfying both — i.e. minimal skirt height. u must be
// non-decreasing; coincident-u samples are levelled to a shared elevation.
void clamp_profile_slope_raise_only(const Vector<F64>& u, Vector<F64>& z,
                                    F64 maxSlope);

// Convenience wrapper: extracts (u, z) from ribs, conditions them, writes the
// shared z back. No-op for < 2 ribs or maxSlope <= 0.
void adjust_rib_elevations(Vector<RoadRib>& ribs, F64 maxSlope);

// Index of the rib whose centre is closest (XY) to pos. ribs must be
// non-empty.
[[nodiscard]] auto nearest_rib_index(const Vector<RoadRib>& ribs,
                                     const Vec2d& pos) -> U32;

void pin_segment_profile(Vector<RoadRib>& ribs,
                         Vector<std::pair<U32, F64>> pins, F64 maxSlope,
                         const String& segId);

[[nodiscard]] auto pin_and_fill_span(const Vector<F64>& u,
                                     const Vector<F64>& ground, Vector<F64>& z,
                                     U32 ia, U32 ib, F64 za, F64 zb,
                                     F64 maxSlope) -> bool;

// Phase-1 "free" profile: raise-only slope-limited majorant of the segment's
// ground, with both endpoints tacked back down to ground level. Result goes in
// `adj`; ribs are NOT modified, so ribs[].z stays the raw ground floor that
// phase 3 needs. Endpoints sit at ground (so endpoint-only connectors suggest
// ground); interior stations carry the lifted, smoothed value.
void free_profile_ground_ends(const Vector<RoadRib>& ribs, F64 maxSlope,
                              Vector<F64>& adj);

// ---------------------------------------------------------------------------
// resample_profile
//
// Resamples a piecewise-linear elevation profile at uniform arc-length steps.
//
// Input:  ribs   — rib array (u is cumulative centreline distance in cm,
//                  non-decreasing; z is the raw ground elevation in cm).
//         stepCm — desired uniform sampling interval (cm).  Must be > 0.
//
// Output: outU   — uniformly-spaced u values: 0, stepCm, 2*stepCm, …, uMax.
//                  The last sample is always clamped to ribs.back().u so the
//                  endpoint is always included exactly.
//         outZ   — linearly interpolated z at each outU sample.
//
// The first and last samples correspond exactly to ribs.front() and
// ribs.back() (no extrapolation).  Coincident-u ribs (corner fan pivots) are
// handled by the bracketing search: the interpolant just returns the shared z.
//
// Complexity: O(n + m) where n = ribs.size(), m = number of output samples.
// ---------------------------------------------------------------------------
void resample_profile(const Vector<RoadRib>& ribs, F64 stepCm,
                      Vector<F64>& outU, Vector<F64>& outZ);

// ---------------------------------------------------------------------------
// Elevation profile smoothing — road-concept-agnostic utilities.
//
// All functions operate on a uniformly-sampled signal z[0..n-1] with a
// uniform sample spacing (implied; not needed by the algorithms themselves).
// They write their result into `out`, which is resized to match `z`.
//
// Boundary convention (all variants):
//   The signal is extended as a constant beyond both ends:
//     z[-k]  == z[0]       for k > 0
//     z[n+k] == z[n-1]     for k > 0
//   This keeps the smoothed endpoints anchored near the raw ground values at
//   the road's start and end, avoiding the "slope-pull" artifact that
//   occurred with free_profile_ground_ends.
//
// All functions are safe to call with out == z (in-place).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// smooth_box
//
// Simple unweighted moving average over a window of (2*halfRad + 1) samples.
// Fast O(n) via a running sum.  Maximally smooths but introduces a flat-top
// response — peaks and valleys are rounded symmetrically.
//
// halfRad == 0  →  identity (no-op copy).
// ---------------------------------------------------------------------------
void smooth_box(const Vector<F64>& z, U32 halfRad, Vector<F64>& out);

// ---------------------------------------------------------------------------
// smooth_gaussian
//
// Weighted moving average with a Gaussian kernel truncated at ±halfRad
// samples (sigma = halfRad / 2.0 by default, giving ~95 % of the kernel
// weight inside the window).  Better frequency response than box — no flat-
// top ringing — at the cost of a slightly heavier inner loop.
//
// The kernel is recomputed once and normalised so boundary clamping does not
// reduce the total weight near the ends (renormalised per-sample is NOT done
// here; the constant kernel is intentional — endpoint anchoring comes from
// the clamped reads, not weight renormalisation).
//
// halfRad == 0  →  identity.
// ---------------------------------------------------------------------------
void smooth_gaussian(const Vector<F64>& z, U32 halfRad, Vector<F64>& out,
                     F64 sigma = 0.0);

// ---------------------------------------------------------------------------
// smooth_median
//
// Sliding median over a window of (2*halfRad + 1) samples.  Resistant to
// spikes and terrain noise — does not blur sharp legitimate transitions as
// much as box/Gaussian.  O(n * halfRad * log(halfRad)) via std::nth_element.
//
// halfRad == 0  →  identity.
// ---------------------------------------------------------------------------
void smooth_median(const Vector<F64>& z, U32 halfRad, Vector<F64>& out);

// ---------------------------------------------------------------------------
// smooth_savitzky_golay_cubic
//
// Savitzky-Golay smoothing with a cubic polynomial fit over a window of
// (2*halfRad + 1) samples.  Preserves peaks and inflection points better
// than box/Gaussian while still suppressing noise.  Good choice for
// elevation profiles where you care about grade continuity.
//
// Coefficients are computed analytically for cubic (degree-3) S-G filters.
// Only odd window sizes 5..25 are supported (halfRad 2..12); outside that
// range the function falls back to smooth_gaussian.
//
// halfRad == 0  →  identity.
// halfRad == 1  →  falls back to smooth_gaussian (window too small for cubic).
// ---------------------------------------------------------------------------
void smooth_savitzky_golay_cubic(const Vector<F64>& z, U32 halfRad,
                                 Vector<F64>& out);

// ---------------------------------------------------------------------------
// sample_profile
//
// Samples a uniformly-resampled elevation profile at an arbitrary u value
// using linear interpolation.
//
// z       — uniformly-spaced elevation samples (produced by resample_profile
//           then any smooth_* pass).
// stepCm  — the uniform spacing used when the profile was resampled.  Must
//           match exactly what was passed to resample_profile.
// u       — query arc-length position (cm).  Clamped to [0, (n-1)*stepCm]
//           so out-of-range queries return the nearest endpoint value rather
//           than extrapolating.
//
// Returns the interpolated elevation (cm).
//
// Complexity: O(1) — no search, no loop.
// ---------------------------------------------------------------------------
[[nodiscard]] auto sample_profile(const Vector<F64>& z, F64 stepCm, F64 u)
    -> F64;

// ---------------------------------------------------------------------------
// build_anticipatory_profile
//
// Generates a smooth, ground-hugging elevation profile for a road segment by
// running a forward and backward anticipatory pass over a uniformly-resampled
// smoothed elevation profile, then combining the results and enforcing the
// grade cap.
//
// Algorithm per pass (described for forward; backward is symmetric):
//   - Start anchored to the raw ground at the first rib.
//   - At each rib, look ahead by `lookaheadCm` in the smoothed profile to
//     get a target elevation.  The current rib should be at a fraction
//     (deltaU / lookaheadCm) of the way between the previous rib elevation
//     and that target.
//   - Clamp to raw ground (road never sinks below terrain).
//
// After both passes, the per-rib elevation is max(forward, backward), then
// a final clamp_profile_slope_raise_only enforces the grade cap.
//
// Parameters:
//   ribs          — rib array from build_ribs() (u non-decreasing, z raw
//                   ground elevation in cm).
//   maxSlope      — grade cap (dimensionless rise/run, from
//                   slope_ratio_from_deg).
//   stepCm        — resampling interval used internally (cm).  Typically the
//                   road sampling distance, e.g. 100 cm.
//   lookaheadCm   — arc-length lookahead distance (cm).  Controls how far
//                   ahead the road "sees" when anticipating elevation changes.
//                   A value of 10 * stepCm is a reasonable starting point.
//   adj           — output: per-rib adjusted elevation (cm), parallel to
//                   ribs[].  Caller writes adj[i] back into ribs[i].z.
// ---------------------------------------------------------------------------
void build_anticipatory_profile(const Vector<RoadRib>& ribs, F64 maxSlope,
                                F64 stepCm, F64 lookaheadCm, Vector<F64>& adj);

auto build_anticipatory_profile(const ProfileVald& rawElevs, F64 stepCm,
                                F64 lookaheadCm) -> ProfileVald;

// ---------------------------------------------------------------------------
// build_anticipatory_profile  (pinned variant — Phase 3)
//
// Same anticipatory smoothing as the unpinned variant, but connector pins
// are honoured as hard constraints: each pin is an exact elevation anchor,
// and the anticipatory passes run independently within each inter-pin span.
//
// This replaces the Phase-3 call to pin_segment_profile for segments that
// already have a smoothed profile: instead of linearly interpolating
// between pins and then cone-raising to ground, we run the full
// anticipatory logic within each span so the interior still gets smooth
// grade anticipation rather than a flat ramp.
//
// pins     — sorted (rib index, target elevation) pairs, raise-only guarded
//            by the caller (i.e. pin elev >= ribs[ridx].z already
//            enforced). Must include the endpoint pins (ridx 0 and last) —
//            the caller in Phase 3 already guarantees this.
// adj      — output: per-rib adjusted elevation (cm), parallel to ribs[].
// ---------------------------------------------------------------------------
void build_anticipatory_profile(
    const Vector<RoadRib>& ribs, F64 maxSlope, F64 stepCm, F64 lookaheadCm,
    F64 connectPlateauFactor,
    const Vector<std::pair<U32, RoadConnectorInfos>>& pins, Vector<F64>& adj);

} // namespace nv

#endif