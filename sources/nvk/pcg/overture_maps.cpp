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

void free_profile_ground_ends(const Vector<RoadRib>& ribs, F64 maxSlope,
                              Vector<F64>& adj) {
    const U32 n = U32(ribs.size());
    adj.resize(n);
    if (n == 0)
        return;
    Vector<F64> u(n);
    for (U32 i = 0; i < n; ++i) {
        u[i] = ribs[i].u;
        adj[i] = ribs[i].z; // seed with raw ground
    }
    clamp_profile_slope_raise_only(u, adj,
                                   maxSlope); // lifts interior (and ends)
    adj[0] = ribs[0].z;                       // tack start to ground
    adj[n - 1] = ribs[n - 1].z;               // tack end to ground
}

void resample_profile(const Vector<RoadRib>& ribs, F64 stepCm,
                      Vector<F64>& outU, Vector<F64>& outZ) {
    outU.clear();
    outZ.clear();

    const U32 n = U32(ribs.size());
    if (n == 0 || stepCm <= 0.0)
        return;
    if (n == 1) {
        outU.push_back(ribs[0].u);
        outZ.push_back(ribs[0].z);
        return;
    }

    const F64 uMax = ribs.back().u;

    // Reserve approximate output count to avoid repeated reallocation.
    const U32 approxCount = U32(std::ceil(uMax / stepCm)) + 2;
    outU.reserve(approxCount);
    outZ.reserve(approxCount);

    // Two-pointer walk: `seg` is the index of the LEFT rib of the current
    // interpolation bracket [ribs[seg].u, ribs[seg+1].u).  We advance it
    // forward as the query u crosses each rib boundary.
    U32 seg = 0;

    const U32 nSamples = U32(std::ceil(uMax / stepCm));
    for (U32 i = 0; i <= nSamples; ++i) {
        const F64 u = (i < nSamples) ? F64(i) * stepCm : uMax;

        // Advance bracket so that ribs[seg].u <= u < ribs[seg+1].u.
        // The guard (seg + 2 < n) keeps seg+1 a valid index.
        while (seg + 2 < n && ribs[seg + 1].u <= u)
            ++seg;

        const F64 u0 = ribs[seg].u;
        const F64 u1 = ribs[seg + 1].u;
        const F64 z0 = ribs[seg].z;
        const F64 z1 = ribs[seg + 1].z;

        F64 z;
        const F64 du = u1 - u0;
        if (du < 1e-12) {
            // Coincident-u pair (inner pivot of a tight corner fan): take the
            // higher z to stay above ground.
            z = std::max(z0, z1);
        } else {
            const F64 t = (u - u0) / du;
            z = z0 + t * (z1 - z0);
        }

        outU.push_back(u);
        outZ.push_back(z);
    }
}
void smooth_savitzky_golay_cubic(const Vector<F64>& z, U32 halfRad,
                                 Vector<F64>& out) {
    const U32 n = U32(z.size());
    out.resize(n);
    if (n == 0)
        return;
    if (halfRad < 2) {
        // Window too small for a cubic fit; fall back gracefully.
        smooth_gaussian(z, halfRad, out);
        return;
    }

    // Analytical S-G coefficients for cubic/quartic (they coincide) are:
    //   c(j) = [ (3m^2 - 1)(3m^2 + 3 - 5*j^2) * (2m+3) ] / [8m(2m-1)(2m+3)]
    // where m = halfRad, j in [-m, m].  We compute them numerically here for
    // generality and normalise so they sum to 1.
    const U32 kLen = 2 * halfRad + 1;
    Vector<F64> kernel(kLen);
    {
        const F64 m = F64(halfRad);
        const F64 m2 = m * m;
        F64 kSum = 0.0;
        for (U32 j = 0; j < kLen; ++j) {
            const F64 jc = F64(j) - m; // centred index
            kernel[j] = (3.0 * m2 - 1.0) * (3.0 * m2 + 3.0 - 5.0 * jc * jc);
            kSum += kernel[j];
        }
        for (F64& w : kernel)
            w /= kSum;
    }

    for (U32 i = 0; i < n; ++i) {
        F64 acc = 0.0;
        for (U32 j = 0; j < kLen; ++j) {
            const U32 idx =
                U32(std::clamp(I32(i) + I32(j) - I32(halfRad), 0, I32(n) - 1));
            acc += kernel[j] * z[idx];
        }
        out[i] = acc;
    }
}
void smooth_median(const Vector<F64>& z, U32 halfRad, Vector<F64>& out) {
    const U32 n = U32(z.size());
    out.resize(n);
    if (n == 0)
        return;
    if (halfRad == 0) {
        out = z;
        return;
    }

    const U32 kLen = 2 * halfRad + 1;
    Vector<F64> window(kLen);
    const U32 medPos = halfRad; // index of median in sorted window

    for (U32 i = 0; i < n; ++i) {
        for (U32 j = 0; j < kLen; ++j) {
            const U32 idx =
                U32(std::clamp(I32(i) + I32(j) - I32(halfRad), 0, I32(n) - 1));
            window[j] = z[idx];
        }
        std::nth_element(window.begin(), window.begin() + medPos, window.end());
        out[i] = window[medPos];
    }
}
void smooth_gaussian(const Vector<F64>& z, U32 halfRad, Vector<F64>& out,
                     F64 sigma) {
    const U32 n = U32(z.size());
    out.resize(n);
    if (n == 0)
        return;
    if (halfRad == 0) {
        out = z;
        return;
    }

    if (sigma <= 0.0)
        sigma = F64(halfRad) / 2.0;

    // Precompute normalised kernel once.
    const U32 kLen = 2 * halfRad + 1;
    Vector<F64> kernel(kLen);
    F64 kSum = 0.0;
    for (U32 j = 0; j < kLen; ++j) {
        const F64 x = F64(j) - F64(halfRad);
        kernel[j] = std::exp(-0.5 * (x / sigma) * (x / sigma));
        kSum += kernel[j];
    }
    for (F64& w : kernel)
        w /= kSum;

    for (U32 i = 0; i < n; ++i) {
        F64 acc = 0.0;
        for (U32 j = 0; j < kLen; ++j) {
            const U32 idx =
                U32(std::clamp(I32(i) + I32(j) - I32(halfRad), 0, I32(n) - 1));
            acc += kernel[j] * z[idx];
        }
        out[i] = acc;
    }
}
void smooth_box(const Vector<F64>& z, U32 halfRad, Vector<F64>& out) {
    const U32 n = U32(z.size());
    out.resize(n);
    if (n == 0)
        return;
    if (halfRad == 0) {
        out = z;
        return;
    }

    // Running sum with clamped boundary reads.
    // Seed the accumulator for i=0: window [-halfRad, +halfRad].
    F64 sum = 0.0;
    for (I32 k = -I32(halfRad); k <= I32(halfRad); ++k) {
        const U32 idx = U32(std::clamp(k, 0, I32(n) - 1));
        sum += z[idx];
    }
    const F64 winWidth = F64(2 * halfRad + 1);
    out[0] = sum / winWidth;

    for (U32 i = 1; i < n; ++i) {
        // Remove sample that left the window on the left.
        const U32 removeIdx =
            U32(std::clamp(I32(i) - I32(halfRad) - 1, 0, I32(n) - 1));
        // Add sample that entered the window on the right.
        const U32 addIdx =
            U32(std::clamp(I32(i) + I32(halfRad), 0, I32(n) - 1));
        sum += z[addIdx] - z[removeIdx];
        out[i] = sum / winWidth;
    }
}
} // namespace nv