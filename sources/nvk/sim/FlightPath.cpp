// File: nvk/sim/FlightPath.cpp

#include <nvk/sim/FlightPath.h>

namespace nv {

namespace {

auto wrap_pi(F64 angleRad) -> F64 {
    F64 res = std::fmod(angleRad + PI, 2.0 * PI);
    if (res <= 0.0) {
        res += 2.0 * PI;
    }
    return res - PI;
}

/// Closest point to `p` on segment [a, b], returned as the parameter t in
/// [0, 1] along the segment.
auto project_on_segment(const Vec2d& pnt, const Vec2d& sega, const Vec2d& segb)
    -> F64 {
    const Vec2d dir = segb - sega;
    const F64 lenSq = dir.dot(dir);
    if (lenSq < 1e-9) {
        return 0.0;
    }
    return std::clamp((pnt - sega).dot(dir) / lenSq, 0.0, 1.0);
}

} // namespace

// ---------------------------------------------------------------------------
// FlightPath
// ---------------------------------------------------------------------------

void FlightPath::rebuild() {
    cumLenM.clear();
    minMarginM = 0.0;
    valid = false;

    if (points.size() < 2) {
        return;
    }

    cumLenM.resize(points.size());
    cumLenM[0] = 0.0;

    for (size_t i = 1; i < points.size(); ++i) {
        const F64 seg = (points[i].posM - points[i - 1].posM).length();
        cumLenM[i] = cumLenM[i - 1] + seg;
    }

    if (cumLenM.back() <= 0.0) {
        return;
    }

    minMarginM = std::numeric_limits<F64>::max();
    for (const auto& pnt : points) {
        minMarginM = std::min(minMarginM, pnt.marginM);
    }

    valid = true;
}

auto FlightPath::segment_at(F64 sM) const -> U32 {
    if (points.size() < 2) {
        return 0;
    }

    const F64 total = length_m();
    F64 sClamped = sM;

    if (closed && total > 0.0) {
        sClamped = std::fmod(sM, total);
        if (sClamped < 0.0) {
            sClamped += total;
        }
    } else {
        sClamped = std::clamp(sM, 0.0, total);
    }

    // upper_bound gives the first cumulative length strictly greater than s,
    // whose index is one past the segment start.
    const auto it = std::upper_bound(cumLenM.begin(), cumLenM.end(), sClamped);
    const auto idx = U32(std::distance(cumLenM.begin(), it));

    return std::min(idx == 0 ? 0U : idx - 1U, num_segments() - 1U);
}

auto FlightPath::sample(F64 sM) const -> FlightWaypoint {
    if (points.empty()) {
        return {};
    }
    if (points.size() == 1) {
        return points[0];
    }

    const F64 total = length_m();
    F64 sClamped = sM;

    if (closed && total > 0.0) {
        sClamped = std::fmod(sM, total);
        if (sClamped < 0.0) {
            sClamped += total;
        }
    } else {
        sClamped = std::clamp(sM, 0.0, total);
    }

    const U32 seg = segment_at(sClamped);
    const F64 segLen = cumLenM[seg + 1] - cumLenM[seg];
    const F64 tpar = segLen > 1e-9 ? (sClamped - cumLenM[seg]) / segLen : 0.0;

    const FlightWaypoint& wpa = points[seg];
    const FlightWaypoint& wpb = points[seg + 1];

    FlightWaypoint out;
    out.posM = wpa.posM + (wpb.posM - wpa.posM) * tpar;
    out.altM = wpa.altM + (wpb.altM - wpa.altM) * tpar;
    out.speedMs = wpa.speedMs + (wpb.speedMs - wpa.speedMs) * tpar;
    out.marginM = std::min(wpa.marginM, wpb.marginM);

    return out;
}

// ---------------------------------------------------------------------------
// FlightPathFollower
// ---------------------------------------------------------------------------

void FlightPathFollower::set_path(const FlightPath& path) {
    _path = path;
    if (_path.cumLenM.size() != _path.points.size()) {
        _path.rebuild();
    }
    _sM = 0.0;
    _segment = 0;
    _crossTrackM = 0.0;
    _laps = 0;
}

void FlightPathFollower::clear_path() {
    _path.reset();
    _sM = 0.0;
    _segment = 0;
    _crossTrackM = 0.0;
    _laps = 0;
}

auto FlightPathFollower::remaining_m() const -> F64 {
    if (!has_path()) {
        return 0.0;
    }
    if (_path.closed) {
        return std::numeric_limits<F64>::max();
    }
    return std::max(0.0, _path.length_m() - _sM);
}

void FlightPathFollower::snap_to_nearest(const Vec2d& posM) {
    if (!has_path()) {
        return;
    }

    F64 bestDistSq = std::numeric_limits<F64>::max();
    F64 bestS = 0.0;
    U32 bestSeg = 0;

    for (U32 seg = 0; seg < _path.num_segments(); ++seg) {
        const Vec2d& sega = _path.points[seg].posM;
        const Vec2d& segb = _path.points[seg + 1].posM;

        const F64 tpar = project_on_segment(posM, sega, segb);
        const Vec2d closest = sega + (segb - sega) * tpar;
        const F64 distSq = (posM - closest).length2();

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestSeg = seg;
            bestS = _path.cumLenM[seg] +
                    tpar * (_path.cumLenM[seg + 1] - _path.cumLenM[seg]);
        }
    }

    _segment = bestSeg;
    _sM = bestS;
    _crossTrackM = std::sqrt(bestDistSq);
}

void FlightPathFollower::advance_cursor(const Vec2d& posM, F64 maxAdvanceM) {
    const U32 numSegs = _path.num_segments();
    if (numSegs == 0) {
        return;
    }

    // Search forward from the current segment only, over a window bounded by
    // how far the aircraft could possibly have moved. This is what keeps an
    // orbit from snapping a lap backwards when its start and end points come
    // within metres of each other.
    F64 bestDistSq = std::numeric_limits<F64>::max();
    F64 bestS = _sM;
    U32 bestSeg = _segment;

    const F64 sLimit = _sM + std::max(maxAdvanceM, 1.0);

    for (U32 step = 0; step < numSegs; ++step) {
        const U32 seg = _path.closed ? (_segment + step) % numSegs
                                     : std::min(_segment + step, numSegs - 1);

        const Vec2d& sega = _path.points[seg].posM;
        const Vec2d& segb = _path.points[seg + 1].posM;

        const F64 tpar = project_on_segment(posM, sega, segb);
        const Vec2d closest = sega + (segb - sega) * tpar;
        const F64 distSq = (posM - closest).length2();

        F64 segS = _path.cumLenM[seg] +
                   tpar * (_path.cumLenM[seg + 1] - _path.cumLenM[seg]);

        // Unwrap onto the current lap so the comparison against sLimit and
        // the monotonic cursor both stay meaningful.
        if (_path.closed) {
            segS += F64(_laps) * _path.length_m();
            while (segS < _sM - 0.5 * _path.length_m()) {
                segS += _path.length_m();
            }
        }

        if (segS < _sM - 1.0 || segS > sLimit) {
            // Behind the cursor, or further ahead than the aircraft could
            // have travelled. Keep walking: the next segment may still be a
            // legal candidate.
            if (segS > sLimit) {
                break;
            }
            continue;
        }

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestSeg = seg;
            bestS = segS;
        }
    }

    if (bestDistSq < std::numeric_limits<F64>::max()) {
        _sM = bestS;
        _segment = bestSeg;
        _crossTrackM = std::sqrt(bestDistSq);

        if (_path.closed && _path.length_m() > 0.0) {
            const auto lap = U32(_sM / _path.length_m());
            _laps = lap;
        }
    }
}

auto FlightPathFollower::lookahead_m(const FlightState& state,
                                     const FlightEnvelope& env) const -> F64 {
    const F64 radiusM = env.turn_radius_m(state.speedMs);
    const F64 timeBased = state.speedMs * _tuning.lookaheadTimeS;

    return std::clamp(timeBased, _tuning.minLookaheadFactor * radiusM,
                      _tuning.maxLookaheadFactor * radiusM);
}

auto FlightPathFollower::update(const FlightState& state,
                                const FlightEnvelope& env, F64 dtS,
                                FlightControls& outControls)
    -> FlightFollowState {
    if (!has_path()) {
        return FlightFollowState::no_path;
    }

    const Vec2d posM = state.pos2_m();

    // Bound the cursor advance by the distance actually flown, plus a slack
    // term so a slow tick cannot strand the cursor behind the aircraft.
    advance_cursor(posM, state.speedMs * std::max(dtS, 0.5) * 3.0 + 50.0);

    const F64 lookM = lookahead_m(state, env);
    const F64 radiusM = env.turn_radius_m(state.speedMs);

    // ── Arrival ─────────────────────────────────────────────────────────
    if (!_path.closed) {
        const F64 arriveM = std::max(_tuning.arrivalFactor * radiusM, 100.0);
        if (remaining_m() <= arriveM) {
            const FlightWaypoint last = _path.points.back();
            outControls.targetTrackRad = state.trackRad;
            outControls.targetAltM = last.altM;
            outControls.targetSpeedMs = last.speedMs;
            return FlightFollowState::arrived;
        }
    }

    // ── Horizontal: pure pursuit ────────────────────────────────────────
    const FlightWaypoint aim = _path.sample(_sM + lookM);
    const Vec2d toAim = aim.posM - posM;

    if (toAim.length2() > 1e-6) {
        outControls.targetTrackRad = std::atan2(toAim.y(), toAim.x());
    } else {
        outControls.targetTrackRad = state.trackRad;
    }

    // ── Vertical: look further ahead than the lateral channel ───────────
    //
    // The planner already put the climb ramps where they belong; aiming the
    // altitude channel at the same point as the track channel would make the
    // aircraft fly the ramp a lookahead late and clip the top of it.
    const FlightWaypoint vertAim =
        _path.sample(_sM + lookM * _tuning.verticalLeadFactor);
    outControls.targetAltM = vertAim.altM;

    // ── Speed ───────────────────────────────────────────────────────────
    const FlightWaypoint here = _path.sample(_sM);
    F64 speedMs = here.speedMs > 0.0 ? here.speedMs : env.cruiseSpeedMs;

    // Slow into a saturated turn. Using the bank actually held rather than
    // the demand means the aircraft is already committed before it slows,
    // which is the right order.
    const F64 bankFrac =
        std::abs(state.bankRad) / std::max(env.usable_bank_rad(), 1e-3);
    if (bankFrac > 0.8) {
        const F64 excess = std::min(1.0, (bankFrac - 0.8) / 0.2);
        speedMs *= 1.0 - excess * (1.0 - _tuning.turnSpeedFactor);
    }

    outControls.targetSpeedMs = speedMs;

    // ── Rotorcraft heading ──────────────────────────────────────────────
    //
    // Left as NaN: the fleet layer overrides it when a helicopter should be
    // pointing at something (a feature it is observing) rather than where it
    // is going.
    outControls.targetHeadingRad = std::numeric_limits<F64>::quiet_NaN();

    // ── Off track ───────────────────────────────────────────────────────
    if (_crossTrackM > _tuning.maxCrossTrackFactor * radiusM) {
        return FlightFollowState::off_track;
    }

    return FlightFollowState::following;
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

auto polyline_length_m(const Vector<Vec2d>& pointsM) -> F64 {
    F64 total = 0.0;
    for (size_t i = 1; i < pointsM.size(); ++i) {
        total += (pointsM[i] - pointsM[i - 1]).length();
    }
    return total;
}

auto resample_polyline(const Vector<Vec2d>& pointsM, F64 stepM)
    -> Vector<Vec2d> {
    if (pointsM.size() < 2 || stepM <= 0.0) {
        return pointsM;
    }

    const F64 total = polyline_length_m(pointsM);
    if (total < stepM) {
        return {pointsM.front(), pointsM.back()};
    }

    // Round the step so the last sample lands exactly on the end point
    // rather than a sliver short of it — the planner indexes the profile by
    // sample count and a ragged tail costs an extra special case there.
    const auto numSteps = U32(std::max(1.0, std::round(total / stepM)));
    const F64 exactStep = total / F64(numSteps);

    Vector<Vec2d> out;
    out.reserve(numSteps + 1);
    out.push_back(pointsM.front());

    F64 walked = 0.0;
    size_t seg = 0;
    F64 segStart = 0.0;

    for (U32 i = 1; i < numSteps; ++i) {
        const F64 target = F64(i) * exactStep;

        while (seg + 1 < pointsM.size()) {
            const F64 segLen = (pointsM[seg + 1] - pointsM[seg]).length();
            if (segStart + segLen >= target || seg + 2 == pointsM.size()) {
                const F64 tpar =
                    segLen > 1e-9 ? (target - segStart) / segLen : 0.0;
                out.push_back(pointsM[seg] + (pointsM[seg + 1] - pointsM[seg]) *
                                                 std::clamp(tpar, 0.0, 1.0));
                break;
            }
            segStart += segLen;
            ++seg;
        }
        walked = target;
    }
    (void)walked;

    out.push_back(pointsM.back());
    return out;
}

auto make_orbit_polyline(const Vec2d& centreM, F64 radiusM, const Vec2d& entryM,
                         U32 turns, bool clockwise, U32 segmentsPerTurn)
    -> Vector<Vec2d> {
    NVCHK(radiusM > 0.0, "make_orbit_polyline: radius must be positive, got {}",
          radiusM);

    const U32 nTurns = std::max(1U, turns);
    const U32 nSegs = std::max(8U, segmentsPerTurn);

    // Enter at the point of the circle nearest where the aircraft is coming
    // from, so it rolls into the orbit instead of crossing it.
    Vec2d toEntry = entryM - centreM;
    if (toEntry.length2() < 1e-6) {
        toEntry = {1.0, 0.0};
    }
    const F64 startAngle = std::atan2(toEntry.y(), toEntry.x());

    const F64 dir = clockwise ? -1.0 : 1.0;
    const F64 stepRad = dir * 2.0 * PI / F64(nSegs);

    Vector<Vec2d> out;
    out.reserve(size_t(nTurns) * nSegs + 1);

    for (U32 i = 0; i <= nTurns * nSegs; ++i) {
        const F64 angle = startAngle + F64(i) * stepRad;
        out.push_back({centreM.x() + radiusM * std::cos(angle),
                       centreM.y() + radiusM * std::sin(angle)});
    }

    return out;
}

} // namespace nv
