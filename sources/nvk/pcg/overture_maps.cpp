#include <nvk/pcg/overture_maps.h>

namespace nv {

static const std::unordered_map<std::string, LandUseClass> kLandUseMap = {
    {"aboriginal_land", LandUseClass::aboriginal_land},
    {"airfield", LandUseClass::airfield},
    {"allotments", LandUseClass::allotments},
    {"animal_keeping", LandUseClass::animal_keeping},
    {"aquaculture", LandUseClass::aquaculture},
    {"barracks", LandUseClass::barracks},
    {"base", LandUseClass::base},
    {"beach_resort", LandUseClass::beach_resort},
    {"brownfield", LandUseClass::brownfield},
    {"bunker", LandUseClass::bunker},
    {"camp_site", LandUseClass::camp_site},
    {"cemetery", LandUseClass::cemetery},
    {"clinic", LandUseClass::clinic},
    {"college", LandUseClass::college},
    {"commercial", LandUseClass::commercial},
    {"connection", LandUseClass::connection},
    {"construction", LandUseClass::construction},
    {"danger_area", LandUseClass::danger_area},
    {"doctors", LandUseClass::doctors},
    {"dog_park", LandUseClass::dog_park},
    {"downhill", LandUseClass::downhill},
    {"driving_range", LandUseClass::driving_range},
    {"driving_school", LandUseClass::driving_school},
    {"education", LandUseClass::education},
    {"environmental", LandUseClass::environmental},
    {"fairway", LandUseClass::fairway},
    {"farmland", LandUseClass::farmland},
    {"farmyard", LandUseClass::farmyard},
    {"fatbike", LandUseClass::fatbike},
    {"flowerbed", LandUseClass::flowerbed},
    {"forest", LandUseClass::forest},
    {"garages", LandUseClass::garages},
    {"garden", LandUseClass::garden},
    {"golf_course", LandUseClass::golf_course},
    {"grass", LandUseClass::grass},
    {"grave_yard", LandUseClass::grave_yard},
    {"green", LandUseClass::green},
    {"greenfield", LandUseClass::greenfield},
    {"greenhouse_horticulture", LandUseClass::greenhouse_horticulture},
    {"highway", LandUseClass::highway},
    {"hike", LandUseClass::hike},
    {"hospital", LandUseClass::hospital},
    {"ice_skate", LandUseClass::ice_skate},
    {"industrial", LandUseClass::industrial},
    {"institutional", LandUseClass::institutional},
    {"kindergarten", LandUseClass::kindergarten},
    {"landfill", LandUseClass::landfill},
    {"lateral_water_hazard", LandUseClass::lateral_water_hazard},
    {"logging", LandUseClass::logging},
    {"marina", LandUseClass::marina},
    {"meadow", LandUseClass::meadow},
    {"military", LandUseClass::military},
    {"military_hospital", LandUseClass::military_hospital},
    {"military_school", LandUseClass::military_school},
    {"music_school", LandUseClass::music_school},
    {"national_park", LandUseClass::national_park},
    {"natural_monument", LandUseClass::natural_monument},
    {"nature_reserve", LandUseClass::nature_reserve},
    {"naval_base", LandUseClass::naval_base},
    {"nordic", LandUseClass::nordic},
    {"nuclear_explosion_site", LandUseClass::nuclear_explosion_site},
    {"obstacle_course", LandUseClass::obstacle_course},
    {"orchard", LandUseClass::orchard},
    {"park", LandUseClass::park},
    {"peat_cutting", LandUseClass::peat_cutting},
    {"pedestrian", LandUseClass::pedestrian},
    {"pitch", LandUseClass::pitch},
    {"plant_nursery", LandUseClass::plant_nursery},
    {"playground", LandUseClass::playground},
    {"plaza", LandUseClass::plaza},
    {"protected", LandUseClass::protected_area},
    {"protected_landscape_seascape",
     LandUseClass::protected_landscape_seascape},
    {"quarry", LandUseClass::quarry},
    {"railway", LandUseClass::railway},
    {"range", LandUseClass::range},
    {"recreation_ground", LandUseClass::recreation_ground},
    {"religious", LandUseClass::religious},
    {"residential", LandUseClass::residential},
    {"resort", LandUseClass::resort},
    {"retail", LandUseClass::retail},
    {"rough", LandUseClass::rough},
    {"salt_pond", LandUseClass::salt_pond},
    {"school", LandUseClass::school},
    {"schoolyard", LandUseClass::schoolyard},
    {"ski_jump", LandUseClass::ski_jump},
    {"skitour", LandUseClass::skitour},
    {"sled", LandUseClass::sled},
    {"sleigh", LandUseClass::sleigh},
    {"snow_park", LandUseClass::snow_park},
    {"species_management_area", LandUseClass::species_management_area},
    {"stadium", LandUseClass::stadium},
    {"state_park", LandUseClass::state_park},
    {"static_caravan", LandUseClass::static_caravan},
    {"strict_nature_reserve", LandUseClass::strict_nature_reserve},
    {"tee", LandUseClass::tee},
    {"theme_park", LandUseClass::theme_park},
    {"track", LandUseClass::track},
    {"traffic_island", LandUseClass::traffic_island},
    {"training_area", LandUseClass::training_area},
    {"trench", LandUseClass::trench},
    {"university", LandUseClass::university},
    {"village_green", LandUseClass::village_green},
    {"vineyard", LandUseClass::vineyard},
    {"water_hazard", LandUseClass::water_hazard},
    {"water_park", LandUseClass::water_park},
    {"wilderness_area", LandUseClass::wilderness_area},
    {"winter_sports", LandUseClass::winter_sports},
    {"works", LandUseClass::works},
    {"zoo", LandUseClass::zoo},
};

auto overture_land_use_class_from_string(const String& name) -> LandUseClass {
    // clang-format off

    // clang-format on
    const auto it = kLandUseMap.find(name);
    return (it != kLandUseMap.end()) ? it->second : LandUseClass::unknown;
}

auto overture_land_class_to_string(LandUseClass luClass) -> String {
    for (const auto& it : kLandUseMap) {
        if (it.second == luClass) {
            return it.first;
        }
    }
    THROW_MSG("Unsupport land use class {}", (I32)luClass);
    return {};
}

// ---------------------------------------------------------------------------
// Overture base/land — class name ↔ LandClass enum
// ---------------------------------------------------------------------------

static const std::unordered_map<std::string, LandClass> kLandClassMap = {
    {"archipelago", LandClass::archipelago},
    {"bare_rock", LandClass::bare_rock},
    {"beach", LandClass::beach},
    {"cave_entrance", LandClass::cave_entrance},
    {"cliff", LandClass::cliff},
    {"desert", LandClass::desert},
    {"dune", LandClass::dune},
    {"fell", LandClass::fell},
    {"forest", LandClass::forest},
    {"glacier", LandClass::glacier},
    {"grass", LandClass::grass},
    {"grassland", LandClass::grassland},
    {"heath", LandClass::heath},
    {"hill", LandClass::hill},
    {"island", LandClass::island},
    {"islet", LandClass::islet},
    {"land", LandClass::land},
    {"meadow", LandClass::meadow},
    {"meteor_crater", LandClass::meteor_crater},
    {"mountain_range", LandClass::mountain_range},
    {"peak", LandClass::peak},
    {"peninsula", LandClass::peninsula},
    {"plateau", LandClass::plateau},
    {"reef", LandClass::reef},
    {"ridge", LandClass::ridge},
    {"rock", LandClass::rock},
    {"saddle", LandClass::saddle},
    {"sand", LandClass::sand},
    {"scree", LandClass::scree},
    {"scrub", LandClass::scrub},
    {"shingle", LandClass::shingle},
    {"shrub", LandClass::shrub},
    {"shrubbery", LandClass::shrubbery},
    {"stone", LandClass::stone},
    {"tree", LandClass::tree},
    {"tree_row", LandClass::tree_row},
    {"tundra", LandClass::tundra},
    {"valley", LandClass::valley},
    {"volcanic_caldera_rim", LandClass::volcanic_caldera_rim},
    {"volcano", LandClass::volcano},
    {"wetland", LandClass::wetland},
    {"wood", LandClass::wood},
};

auto overture_land_class_from_string(const String& name) -> LandClass {
    const auto it = kLandClassMap.find(name);
    return (it != kLandClassMap.end()) ? it->second : LandClass::unknown;
}

auto overture_land_class_to_string(LandClass cls) -> String {
    for (const auto& it : kLandClassMap) {
        if (it.second == cls)
            return it.first;
    }
    THROW_MSG("Unsupported land class {}", (I32)cls);
    return {};
}

auto road_default_max_slope_deg(const String& roadClass) -> F64 {
    static const UnorderedMap<String, F64> kTable = {
        {"motorway", 4.0},      {"trunk", 4.0},        {"primary", 5.0},
        {"secondary", 6.0},     {"tertiary", 6.0},     {"residential", 8.0},
        {"living_street", 8.0}, {"unclassified", 8.0}, {"service", 12.0},
        {"track", 15.0},        {"path", 15.0},        {"bridleway", 15.0},
        {"footway", 15.0},      {"cycleway", 12.0},    {"pedestrian", 12.0},
        {"steps", 45.0},
    };
    const auto it = kTable.find(roadClass);
    return it != kTable.end() ? it->second : kRoadDefaultMaxSlopeDeg;
}

auto slope_ratio_from_deg(F64 deg) -> F64 { return std::tan(deg * kDegToRad); }

void clamp_profile_slope_raise_only(const Vector<F64>& u, Vector<F64>& z,
                                    F64 maxSlope) {
    const U32 n = U32(z.size());
    if (n < 2 || maxSlope <= 0.0)
        return;

    // Forward sweep: a peak behind us caps how fast the road may descend, so
    // each sample is pulled up to at least (previous − maxSlope·Δu).
    for (U32 i = 1; i < n; ++i) {
        const F64 du = std::max(0.0, u[i] - u[i - 1]);
        const F64 floorFromPrev = z[i - 1] - maxSlope * du;
        if (z[i] < floorFromPrev)
            z[i] = floorFromPrev;
    }
    // Backward sweep: symmetrically, a peak ahead caps the descent looking
    // back. After both sweeps z[i] == max_j (z_in[j] − maxSlope·|u[i]−u[j]|),
    // the minimal raise-only slope-limited majorant.
    for (U32 i = n - 1; i-- > 0;) {
        const F64 du = std::max(0.0, u[i + 1] - u[i]);
        const F64 floorFromNext = z[i + 1] - maxSlope * du;
        if (z[i] < floorFromNext)
            z[i] = floorFromNext;
    }
}

void adjust_rib_elevations(Vector<RoadRib>& ribs, F64 maxSlope) {
    const U32 n = U32(ribs.size());
    if (n < 2 || maxSlope <= 0.0)
        return;

    Vector<F64> u(n);
    Vector<F64> z(n);
    for (U32 i = 0; i < n; ++i) {
        u[i] = ribs[i].u;
        z[i] = ribs[i].z;
    }
    clamp_profile_slope_raise_only(u, z, maxSlope);
    for (U32 i = 0; i < n; ++i)
        ribs[i].z = z[i]; // both left/right of a rib still share this single z
}

auto nearest_rib_index(const Vector<RoadRib>& ribs, const Vec2d& pos) -> U32 {
    U32 best = 0;
    F64 bestD2 = std::numeric_limits<F64>::max();
    for (U32 i = 0; i < U32(ribs.size()); ++i) {
        const Vec2d centre = (ribs[i].left + ribs[i].right) * 0.5;
        const Vec2d d = centre - pos;
        const F64 d2 = d.dot(d);
        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best;
}

void pin_segment_profile(Vector<RoadRib>& ribs,
                         Vector<std::pair<U32, F64>> pins, F64 maxSlope,
                         const String& segId) {
    const U32 n = U32(ribs.size());
    if (n < 2 || pins.size() < 2)
        return;

    std::sort(pins.begin(), pins.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    Vector<F64> u(n), ground(n), z(n);
    for (U32 i = 0; i < n; ++i) {
        u[i] = ribs[i].u;
        ground[i] = ribs[i].z; // build_ribs left the raw ground cross-max here
        z[i] = ribs[i].z;
    }

    for (U32 k = 0; k + 1 < U32(pins.size()); ++k) {
        const U32 ia = pins[k].first, ib = pins[k + 1].first;
        if (ib <= ia)
            continue; // duplicate pin (two connectors on one rib): skip
        if (pin_and_fill_span(u, ground, z, ia, ib, pins[k].second,
                              pins[k + 1].second, maxSlope)) {
            const F64 grade = std::abs(pins[k + 1].second - pins[k].second) /
                              std::max(1e-9, u[ib] - u[ia]);
            logWARN("Road {}: connector-to-connector grade {:.1f}% exceeds "
                    "limit {:.1f}% — building anyway",
                    segId, 100.0 * grade, 100.0 * maxSlope);
        }
    }

    for (U32 i = 0; i < n; ++i)
        ribs[i].z = z[i];
}

auto pin_and_fill_span(const Vector<F64>& u, const Vector<F64>& ground,
                       Vector<F64>& z, U32 ia, U32 ib, F64 za, F64 zb,
                       F64 maxSlope) -> bool {
    z[ia] = za;
    z[ib] = zb;
    for (U32 i = ia + 1; i < ib; ++i)
        z[i] = ground[i]; // interior floor

    // Forward cone from the start pin (za seeds z[ia]); fills valleys.
    for (U32 i = ia + 1; i < ib; ++i) {
        const F64 du = std::max(0.0, u[i] - u[i - 1]);
        const F64 cone = z[i - 1] - maxSlope * du;
        if (z[i] < cone)
            z[i] = cone;
    }
    // Backward cone from the end pin (zb seeds z[ib]). Endpoints stay
    // exact.
    for (U32 i = ib; i-- > ia + 1;) {
        const F64 du = std::max(0.0, u[i + 1] - u[i]);
        const F64 cone = z[i + 1] - maxSlope * du;
        if (z[i] < cone)
            z[i] = cone;
    }

    // Did the pins force a grade break? Check overall span + pin-adjacent
    // edges.
    constexpr F64 eps = 1e-6;
    const F64 spanLen = u[ib] - u[ia];
    bool violated =
        (spanLen > 0.0 && std::abs(zb - za) / spanLen > maxSlope + eps);
    if (ib > ia + 1) {
        const F64 d0 = std::max(1e-9, u[ia + 1] - u[ia]);
        const F64 d1 = std::max(1e-9, u[ib] - u[ib - 1]);
        if (std::abs(z[ia + 1] - za) / d0 > maxSlope + eps)
            violated = true;
        if (std::abs(zb - z[ib - 1]) / d1 > maxSlope + eps)
            violated = true;
    }
    return violated;
}

} // namespace nv