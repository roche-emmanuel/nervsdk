// File: nvk/sim/FlightPlanner.cpp

#include <nvk/sim/FlightPlanner.h>

#include <nvk/io/SvgCanvas.h>

namespace nv {

namespace {

/// Below this the profile is a single point and every pass is a no-op.
constexpr U32 kMinSamples = 2;

/// Hysteresis on the level quantiser, as a fraction of one level step. A
/// window maximum a metre over a boundary should not cost a whole extra
/// level, and without this the staircase chatters on gently rising ground.
constexpr F64 kLevelHysteresis = 0.15;

} // namespace

// ---------------------------------------------------------------------------
// FlightProfileDesc
// ---------------------------------------------------------------------------

auto make_profile_desc(const FlightEnvelope& env) -> FlightProfileDesc {
    FlightProfileDesc desc;

    desc.maxClimbAngleRad = env.maxClimbAngleRad;
    desc.maxDescentAngleRad = env.maxDescentAngleRad;
    desc.verticalAccelMs2 = env.maxVerticalAccelMs2 * 0.65;
    desc.roundingSpeedMs = env.cruiseSpeedMs;

    if (env.is_rotorcraft()) {
        // A helicopter works close in: small corridor, low level grid, short
        // plateaus. Anything else and it flies like an airliner doing a
        // very slow flypast.
        desc.sampleStepM = 100.0;
        desc.protectionRadiusM = 150.0;
        desc.minAglM = 150.0;
        desc.levelStepM = 100.0;
        desc.minPlateauLenM = 800.0;
        desc.maxAltM = 4000.0;
    }

    return desc;
}

// ---------------------------------------------------------------------------
// FlightPlanner
// ---------------------------------------------------------------------------

FlightPlanner::FlightPlanner(const TerrainField* terrain) : _terrain(terrain) {}

auto FlightPlanner::sample_floor(const Vector<Vec2d>& samplesM,
                                 const FlightProfileDesc& desc) const
    -> Vector<F64> {
    Vector<F64> floorM(samplesM.size(), desc.minAltM);

    if (_terrain == nullptr || !_terrain->is_valid()) {
        return floorM;
    }

    for (size_t i = 0; i < samplesM.size(); ++i) {
        const F64 ceilM =
            _terrain->max_height_in(samplesM[i], desc.protectionRadiusM);
        floorM[i] = std::max(desc.minAltM, ceilM + desc.minAglM);
    }

    return floorM;
}

// ---------------------------------------------------------------------------
// Pass 2 / 4 — Lipschitz closure
// ---------------------------------------------------------------------------

void FlightPlanner::lipschitz_closure(Vector<F64>& alt,
                                      const Vector<F64>& floorM, F64 stepM,
                                      F64 tanClimb, F64 tanDescent) {
    NVCHK(alt.size() == floorM.size(),
          "lipschitz_closure: profile has {} samples, floor has {}", alt.size(),
          floorM.size());

    if (alt.size() < kMinSamples) {
        if (!alt.empty()) {
            alt[0] = std::max(alt[0], floorM[0]);
        }
        return;
    }

    for (size_t i = 0; i < alt.size(); ++i) {
        alt[i] = std::max(alt[i], floorM[i]);
    }

    const F64 dropM = stepM * std::max(tanDescent, 1e-6);
    const F64 riseM = stepM * std::max(tanClimb, 1e-6);

    // Forward: going forward the profile may not fall faster than the
    // descent gradient, so sample i must be at least its predecessor minus
    // one step's worth of descent.
    for (size_t i = 1; i < alt.size(); ++i) {
        alt[i] = std::max(alt[i], alt[i - 1] - dropM);
    }

    // Backward: to reach sample i+1 at the climb gradient, sample i must
    // already be within one step of it.
    for (size_t i = alt.size() - 1; i-- > 0;) {
        alt[i] = std::max(alt[i], alt[i + 1] - riseM);
    }

    // Both passes only raise, and raising an earlier sample can never
    // violate the forward constraint (which bounds how fast the profile
    // *falls*), so two sweeps are the fixed point. The unit test asserts it.
}

// ---------------------------------------------------------------------------
// Pass 3 — plateau quantisation
// ---------------------------------------------------------------------------

auto FlightPlanner::window_max(const Vector<F64>& src, U32 halfWindow)
    -> Vector<F64> {
    Vector<F64> out(src.size(), 0.0);
    if (src.empty()) {
        return out;
    }
    if (halfWindow == 0) {
        return src;
    }

    // Monotonic deque of indices whose values are strictly decreasing: the
    // front is always the window maximum. O(n) whatever the window width,
    // which matters because minPlateauLen over a 250 m step is a 60-sample
    // window and the naive form is O(n*w).
    std::deque<size_t> dq;
    const auto num = src.size();

    for (size_t i = 0; i < num + halfWindow; ++i) {
        if (i < num) {
            while (!dq.empty() && src[dq.back()] <= src[i]) {
                dq.pop_back();
            }
            dq.push_back(i);
        }

        if (i >= halfWindow) {
            const size_t centre = i - halfWindow;

            // Evict anything that has fallen off the trailing edge.
            const size_t lowest = centre > halfWindow ? centre - halfWindow : 0;
            while (!dq.empty() && dq.front() < lowest) {
                dq.pop_front();
            }

            if (centre < num && !dq.empty()) {
                out[centre] = src[dq.front()];
            }
        }
    }

    return out;
}

void FlightPlanner::quantize_plateaus(Vector<F64>& alt, F64 stepM,
                                      F64 minPlateauLenM, F64 levelStepM,
                                      F64 levelOffsetM) {
    if (alt.size() < kMinSamples || levelStepM <= 0.0) {
        return;
    }

    const auto halfWindow =
        U32(std::max(0.0, std::round(0.5 * minPlateauLenM / stepM)));

    const Vector<F64> wmax = window_max(alt, halfWindow);

    // Round each window maximum up to the next level of the offset grid.
    // The hysteresis subtracts a sliver before rounding, so a maximum that
    // only just crosses a boundary stays on the level below.
    for (size_t i = 0; i < alt.size(); ++i) {
        const F64 relative = wmax[i] - levelOffsetM;
        const F64 levels = std::ceil(relative / levelStepM - kLevelHysteresis);
        alt[i] = levels * levelStepM + levelOffsetM;
    }
}

// ---------------------------------------------------------------------------
// Pass 5 — knee rounding
// ---------------------------------------------------------------------------

void FlightPlanner::smooth_profile(Vector<F64>& alt, U32 halfWindow) {
    if (halfWindow == 0 || alt.size() < 3) {
        return;
    }

    // A moving average of a piecewise-linear profile is quadratic at every
    // knee and unchanged along every straight run, which is exactly the
    // parabolic level-off an aircraft flies. Nothing fancier is warranted.
    const Vector<F64> src = alt;
    const auto num = I64(src.size());
    const auto win = I64(halfWindow);

    for (I64 i = 0; i < num; ++i) {
        F64 sum = 0.0;
        I64 count = 0;
        for (I64 k = -win; k <= win; ++k) {
            const I64 idx = std::clamp(i + k, I64(0), num - 1);
            sum += src[size_t(idx)];
            ++count;
        }
        alt[size_t(i)] = sum / F64(count);
    }
}

// ---------------------------------------------------------------------------
// build_path
// ---------------------------------------------------------------------------

auto FlightPlanner::build_path(const Vector<Vec2d>& waypointsM,
                               const FlightProfileDesc& desc, F64 cruiseSpeedMs,
                               bool closed) const -> FlightPath {
    FlightPath path;

    if (waypointsM.size() < 2) {
        logWARN("FlightPlanner: a route needs at least two waypoints, got {}",
                waypointsM.size());
        return path;
    }

    NVCHK(desc.sampleStepM > 0.0,
          "FlightPlanner: sample step must be positive, got {}",
          desc.sampleStepM);

    // ── Horizontal ──────────────────────────────────────────────────────
    const Vector<Vec2d> samplesM =
        resample_polyline(waypointsM, desc.sampleStepM);

    if (samplesM.size() < kMinSamples) {
        logWARN("FlightPlanner: route is shorter than one sample step");
        return path;
    }

    const F64 stepM = polyline_length_m(samplesM) / F64(samplesM.size() - 1);

    // ── Pass 1: floor ───────────────────────────────────────────────────
    const Vector<F64> floorM = sample_floor(samplesM, desc);

    Vector<F64> terrainM(samplesM.size(), 0.0);
    if (_terrain != nullptr && _terrain->is_valid()) {
        for (size_t i = 0; i < samplesM.size(); ++i) {
            terrainM[i] = _terrain->height_at(samplesM[i]);
        }
    }

    const F64 tanClimb = std::tan(desc.maxClimbAngleRad);
    const F64 tanDescent = std::tan(desc.maxDescentAngleRad);

    // How many samples the rounding pass spans. The level-off length is the
    // distance covered while the vertical speed bleeds off at the vertical
    // acceleration limit: v * (v * tanClimb / a).
    const F64 roundLenM = desc.roundingSpeedMs * desc.roundingSpeedMs *
                          tanClimb / std::max(desc.verticalAccelMs2, 1e-3);
    const auto halfWindow =
        U32(std::clamp(std::round(0.5 * roundLenM / stepM), 0.0, 64.0));

    // Pad the floor by the most the smoothing can pull a knee down: half a
    // window of ramp, at the steeper of the two gradients. Giving the
    // rounding room up front is far better than clamping afterwards, which
    // would put the kink straight back.
    const F64 smoothPadM =
        F64(halfWindow) * stepM * std::max(tanClimb, tanDescent) * 0.5;

    Vector<F64> paddedFloorM = floorM;
    for (F64& val : paddedFloorM) {
        val += smoothPadM;
        if (desc.cruiseAltM > 0.0) {
            val = std::max(val, desc.cruiseAltM);
        }
    }

    // ── Pass 2: minimum feasible profile ────────────────────────────────
    Vector<F64> altM = paddedFloorM;
    lipschitz_closure(altM, paddedFloorM, stepM, tanClimb, tanDescent);

    // ── Pass 3: plateaus ────────────────────────────────────────────────
    quantize_plateaus(altM, stepM, desc.minPlateauLenM, desc.levelStepM,
                      desc.levelOffsetM);

    // ── Pass 4: ramps ───────────────────────────────────────────────────
    //
    // The staircase is now the floor. The closure inserts the climbs and
    // descents in the only places they can legally go.
    Vector<F64> staircaseM = altM;
    for (size_t i = 0; i < staircaseM.size(); ++i) {
        staircaseM[i] = std::max(staircaseM[i], paddedFloorM[i]);
    }
    lipschitz_closure(altM, staircaseM, stepM, tanClimb, tanDescent);

    // ── Pass 5: knees ───────────────────────────────────────────────────
    smooth_profile(altM, halfWindow);

    // ── Clamp and verify ────────────────────────────────────────────────
    F64 worstMarginM = std::numeric_limits<F64>::max();
    U32 numViolations = 0;

    for (size_t i = 0; i < altM.size(); ++i) {
        altM[i] = std::clamp(altM[i], desc.minAltM, desc.maxAltM);

        // The margin the aircraft actually has over the protected corridor,
        // which is what the caller cares about — not over the point
        // directly below it.
        const F64 ceilM = floorM[i] - desc.minAglM;
        const F64 marginM = altM[i] - ceilM;

        if (marginM < desc.minAglM * 0.95) {
            ++numViolations;
        }
        worstMarginM = std::min(worstMarginM, marginM);
    }

    if (numViolations > 0) {
        logWARN("FlightPlanner: {}/{} sample(s) fall below the requested "
                "{:.0f} m clearance (worst margin {:.0f} m). The route "
                "crosses terrain the climb gradient or the altitude ceiling "
                "cannot clear — it is still flyable, but check maxAltM and "
                "maxClimbAngleRad.",
                numViolations, altM.size(), desc.minAglM, worstMarginM);
    }

    // ── Assemble ────────────────────────────────────────────────────────
    path.points.reserve(samplesM.size());
    for (size_t i = 0; i < samplesM.size(); ++i) {
        const F64 ceilM = floorM[i] - desc.minAglM;
        path.push(samplesM[i], altM[i], cruiseSpeedMs, altM[i] - ceilM);
    }

    path.closed = closed;
    path.rebuild();

    if (!desc.profileSvgPath.empty()) {
        dump_profile_svg(desc.profileSvgPath, samplesM, terrainM, floorM, altM,
                         stepM);
    }

    return path;
}

auto FlightPlanner::build_orbit(const Vec2d& centreM, F64 radiusM,
                                const Vec2d& entryM, U32 turns, bool clockwise,
                                const FlightProfileDesc& desc,
                                F64 speedMs) const -> FlightPath {
    const Vector<Vec2d> ring =
        make_orbit_polyline(centreM, radiusM, entryM, turns, clockwise);

    // An orbit is planned at one altitude: the ceiling over the whole disc
    // plus the AGL margin. Sampling the profile round the circle would make
    // the aircraft porpoise once a lap over sloping ground, which is both
    // unrealistic and immediately obvious.
    FlightProfileDesc flat = desc;

    if (_terrain != nullptr && _terrain->is_valid()) {
        const F64 ceilM =
            _terrain->max_height_in(centreM, radiusM + desc.protectionRadiusM);
        flat.cruiseAltM = std::max(desc.cruiseAltM, ceilM + desc.minAglM);
    } else if (desc.cruiseAltM <= 0.0) {
        flat.cruiseAltM = desc.minAglM;
    }

    // One long plateau, which the quantiser then rounds to a level.
    flat.minPlateauLenM =
        std::max(desc.minPlateauLenM, polyline_length_m(ring) * 2.0);

    return build_path(ring, flat, speedMs, /*closed=*/true);
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

void FlightPlanner::dump_profile_svg(const String& path,
                                     const Vector<Vec2d>& samplesM,
                                     const Vector<F64>& terrainM,
                                     const Vector<F64>& floorM,
                                     const Vector<F64>& altM, F64 stepM) const {
    if (samplesM.size() < 2) {
        return;
    }

    const F64 totalM = stepM * F64(samplesM.size() - 1);

    F64 minZ = std::numeric_limits<F64>::max();
    F64 maxZ = -std::numeric_limits<F64>::max();
    for (size_t i = 0; i < altM.size(); ++i) {
        minZ = std::min({minZ, altM[i], floorM[i], terrainM[i]});
        maxZ = std::max({maxZ, altM[i], floorM[i], terrainM[i]});
    }
    if (!(maxZ > minZ)) {
        maxZ = minZ + 1.0;
    }

    const F64 padZ = (maxZ - minZ) * 0.08;

    SvgCanvas canvas;
    canvas.fit_bounds(0.0, minZ - padZ, totalM, maxZ + padZ, 1600.0, false);

    auto series = [&](const Vector<F64>& vals) {
        Vector<Vec2d> line;
        line.reserve(vals.size());
        for (size_t i = 0; i < vals.size(); ++i) {
            line.push_back({F64(i) * stepM, vals[i]});
        }
        return line;
    };

    canvas.polyline(series(terrainM), "#8c7853", 1.5); // ground
    canvas.polyline(series(floorM), "#d62728", 1.5);   // corridor floor
    canvas.polyline(series(altM), "#1f77b4", 2.5);     // flown profile

    canvas.text_px(12.0, 22.0,
                   "vertical profile — blue: flown, red: floor "
                   "(corridor max + AGL), brown: ground",
                   "#000000", 16.0);
    canvas.text_px(12.0, 42.0,
                   fmt::format("route {:.1f} km, {} samples at {:.0f} m",
                               totalM / 1000.0, samplesM.size(), stepM)
                       .c_str(),
                   "#555555", 14.0);

    canvas.write_file(path);
    logINFO("FlightPlanner: profile dump written to '{}'", path);
}

} // namespace nv
