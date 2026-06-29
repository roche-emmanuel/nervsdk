#ifndef _OVERTURE_MAPS_H_
#define _OVERTURE_MAPS_H_

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

} // namespace nv

#endif