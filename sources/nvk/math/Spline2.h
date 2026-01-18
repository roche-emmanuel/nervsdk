#ifndef _NV_SPLINE2_H_
#define _NV_SPLINE2_H_

#include <nvk/math/Vec2.h>
#include <nvk_math.h>
#include <optional>
#include <vector>

namespace nv {

template <typename T> struct ControlPoint2 {
    using value_t = T;

    Vec2<T> position;
    Vec2<T> tangent_in;  // Arriving tangent (from previous point)
    Vec2<T> tangent_out; // Departing tangent (to next point)

    constexpr ControlPoint2()
        : position(0, 0), tangent_in(0, 0), tangent_out(0, 0) {}

    constexpr ControlPoint2(const Vec2<T>& pos)
        : position(pos), tangent_in(0, 0), tangent_out(0, 0) {}

    constexpr ControlPoint2(const Vec2<T>& pos, const Vec2<T>& tan_in,
                            const Vec2<T>& tan_out)
        : position(pos), tangent_in(tan_in), tangent_out(tan_out) {}

    // Auto-tangent: smooth tangent based on neighboring points
    void autoTangent(const Vec2<T>& prev, const Vec2<T>& next,
                     value_t tension = 0.5) {
        Vec2<T> dir = (next - prev) * tension;
        tangent_in = -dir;
        tangent_out = dir;
    }
};

template <typename T> class Spline2 {
  public:
    using value_t = T;
    using point_t = Vec2<T>;
    using control_point_t = ControlPoint2<T>;

  private:
    Vector<control_point_t> _control_points;
    bool _closed = false;
    mutable Vector<value_t> _segment_lengths; // Cached segment lengths
    mutable bool _lengths_dirty = true;

    // Hermite basis functions
    static value_t h00(value_t t) { return (1 + 2 * t) * (1 - t) * (1 - t); }
    static value_t h10(value_t t) { return t * (1 - t) * (1 - t); }
    static value_t h01(value_t t) { return t * t * (3 - 2 * t); }
    static value_t h11(value_t t) { return t * t * (t - 1); }

    // Hermite derivatives
    static value_t dh00(value_t t) { return 6 * t * t - 6 * t; }
    static value_t dh10(value_t t) { return 3 * t * t - 4 * t + 1; }
    static value_t dh01(value_t t) { return -6 * t * t + 6 * t; }
    static value_t dh11(value_t t) { return 3 * t * t - 2 * t; }

    void invalidateLengths() const { _lengths_dirty = true; }

    void computeSegmentLengths() const {
        if (!_lengths_dirty)
            return;

        _segment_lengths.clear();
        size_t numSegs = numSegments();
        _segment_lengths.reserve(numSegs);

        for (size_t i = 0; i < numSegs; ++i) {
            _segment_lengths.push_back(computeSegmentLength(i));
        }

        _lengths_dirty = false;
    }

    value_t computeSegmentLength(size_t segIdx, int samples = 32) const {
        if (segIdx >= numSegments())
            return 0;

        value_t length = 0;
        point_t prev = getSegmentPoint(segIdx, 0);

        for (int i = 1; i <= samples; ++i) {
            value_t t = static_cast<value_t>(i) / samples;
            point_t curr = getSegmentPoint(segIdx, t);
            length += (curr - prev).length();
            prev = curr;
        }

        return length;
    }

  public:
    Spline2() = default;

    explicit Spline2(const Vector<point_t>& points, bool closed = false)
        : _closed(closed) {
        for (const auto& pt : points) {
            _control_points.emplace_back(pt);
        }
        autoTangents();
    }

    explicit Spline2(const Vector<control_point_t>& controlPoints,
                     bool closed = false)
        : _control_points(controlPoints), _closed(closed) {
        invalidateLengths();
    }

    // Control point management
    void addPoint(const point_t& pos) {
        _control_points.emplace_back(pos);
        invalidateLengths();
    }

    void addPoint(const point_t& pos, const point_t& tanIn,
                  const point_t& tanOut) {
        _control_points.emplace_back(pos, tanIn, tanOut);
        invalidateLengths();
    }

    void addControlPoint(const control_point_t& cp) {
        _control_points.push_back(cp);
        invalidateLengths();
    }

    void insertPoint(size_t index, const point_t& pos) {
        if (index > _control_points.size())
            return;
        _control_points.insert(_control_points.begin() + index,
                               control_point_t(pos));
        invalidateLengths();
    }

    void removePoint(size_t index) {
        if (index >= _control_points.size())
            return;
        _control_points.erase(_control_points.begin() + index);
        invalidateLengths();
    }

    void setPoint(size_t index, const point_t& pos) {
        if (index >= _control_points.size())
            return;
        _control_points[index].position = pos;
        invalidateLengths();
    }

    void setTangents(size_t index, const point_t& tanIn,
                     const point_t& tanOut) {
        if (index >= _control_points.size())
            return;
        _control_points[index].tangent_in = tanIn;
        _control_points[index].tangent_out = tanOut;
        invalidateLengths();
    }

    void clear() {
        _control_points.clear();
        invalidateLengths();
    }

    // Accessors
    [[nodiscard]] size_t size() const { return _control_points.size(); }
    [[nodiscard]] bool empty() const { return _control_points.empty(); }
    [[nodiscard]] bool isClosed() const { return _closed; }
    void setClosed(bool closed) {
        _closed = closed;
        invalidateLengths();
    }

    [[nodiscard]] size_t numSegments() const {
        if (_control_points.size() < 2)
            return 0;
        return _closed ? _control_points.size() : _control_points.size() - 1;
    }

    control_point_t& operator[](size_t i) {
        invalidateLengths();
        return _control_points[i];
    }

    const control_point_t& operator[](size_t i) const {
        return _control_points[i];
    }

    [[nodiscard]] const Vector<control_point_t>& controlPoints() const {
        return _control_points;
    }

    // Auto-compute smooth tangents
    void autoTangents(value_t tension = 0.5) {
        if (_control_points.size() < 2)
            return;

        size_t n = _control_points.size();

        for (size_t i = 0; i < n; ++i) {
            size_t prev = (i == 0) ? (_closed ? n - 1 : i) : i - 1;
            size_t next = (i == n - 1) ? (_closed ? 0 : i) : i + 1;

            if (!_closed && (i == 0 || i == n - 1)) {
                // End points: use forward/backward difference
                if (i == 0) {
                    _control_points[i].tangent_out =
                        (_control_points[next].position -
                         _control_points[i].position) *
                        tension;
                    _control_points[i].tangent_in =
                        -_control_points[i].tangent_out;
                } else {
                    _control_points[i].tangent_in =
                        -(_control_points[i].position -
                          _control_points[prev].position) *
                        tension;
                    _control_points[i].tangent_out =
                        -_control_points[i].tangent_in;
                }
            } else {
                _control_points[i].autoTangent(_control_points[prev].position,
                                               _control_points[next].position,
                                               tension);
            }
        }

        invalidateLengths();
    }

    // Evaluate point on segment using local t [0,1]
    [[nodiscard]] point_t getSegmentPoint(size_t segIdx, value_t t) const {
        if (segIdx >= numSegments())
            return point_t(0, 0);

        size_t i0 = segIdx;
        size_t i1 = (segIdx + 1) % _control_points.size();

        const auto& p0 = _control_points[i0];
        const auto& p1 = _control_points[i1];

        return p0.position * h00(t) + p0.tangent_out * h10(t) +
               p1.position * h01(t) + p1.tangent_in * h11(t);
    }

    // Evaluate tangent (derivative) on segment
    [[nodiscard]] point_t getSegmentTangent(size_t segIdx, value_t t) const {
        if (segIdx >= numSegments())
            return point_t(0, 0);

        size_t i0 = segIdx;
        size_t i1 = (segIdx + 1) % _control_points.size();

        const auto& p0 = _control_points[i0];
        const auto& p1 = _control_points[i1];

        return p0.position * dh00(t) + p0.tangent_out * dh10(t) +
               p1.position * dh01(t) + p1.tangent_in * dh11(t);
    }

    // Evaluate point using global t [0,1] across entire spline
    [[nodiscard]] auto evaluate(value_t t) const -> point_t {
        if (_control_points.size() < 2) {
            return _control_points.empty() ? point_t(0, 0)
                                           : _control_points[0].position;
        }

        t = nv::clamp(t, static_cast<value_t>(0), static_cast<value_t>(1));

        size_t numSegs = numSegments();
        value_t segFloat = t * numSegs;
        size_t segIdx = static_cast<size_t>(segFloat);

        if (segIdx >= numSegs)
            segIdx = numSegs - 1;

        value_t localT = segFloat - segIdx;
        return getSegmentPoint(segIdx, localT);
    }

    // Evaluate tangent using global t [0,1]
    [[nodiscard]] point_t evaluateTangent(value_t t) const {
        if (_control_points.size() < 2)
            return point_t(0, 0);

        t = nv::clamp(t, static_cast<value_t>(0), static_cast<value_t>(1));

        size_t numSegs = numSegments();
        value_t segFloat = t * numSegs;
        size_t segIdx = static_cast<size_t>(segFloat);

        if (segIdx >= numSegs)
            segIdx = numSegs - 1;

        value_t localT = segFloat - segIdx;
        return getSegmentTangent(segIdx, localT);
    }

    // Get normalized direction at t
    [[nodiscard]] point_t evaluateDirection(value_t t) const {
        point_t tangent = evaluateTangent(t);
        value_t len = tangent.length();
        return len > 0 ? tangent / len : point_t(1, 0);
    }

    // Get normal (perpendicular) at t
    [[nodiscard]] point_t evaluateNormal(value_t t) const {
        point_t dir = evaluateDirection(t);
        return point_t(-dir.y(), dir.x());
    }

    // Length computation
    [[nodiscard]] value_t totalLength(int samplesPerSegment = 32) const {
        computeSegmentLengths();

        value_t total = 0;
        for (value_t len : _segment_lengths) {
            total += len;
        }
        return total;
    }

    [[nodiscard]] value_t segmentLength(size_t segIdx) const {
        computeSegmentLengths();
        return segIdx < _segment_lengths.size() ? _segment_lengths[segIdx] : 0;
    }

    // Convert arc length to t parameter
    [[nodiscard]] value_t arcLengthToT(value_t arcLength) const {
        computeSegmentLengths();

        if (_segment_lengths.empty())
            return 0;

        value_t total = totalLength();
        arcLength = nv::clamp(arcLength, static_cast<value_t>(0), total);

        value_t accumulated = 0;
        for (size_t i = 0; i < _segment_lengths.size(); ++i) {
            if (accumulated + _segment_lengths[i] >= arcLength) {
                value_t segT = (arcLength - accumulated) / _segment_lengths[i];
                return (i + segT) / numSegments();
            }
            accumulated += _segment_lengths[i];
        }

        return 1.0;
    }

    // Sample points uniformly by arc length
    [[nodiscard]] Vector<point_t> sampleUniform(size_t numSamples) const {
        Vector<point_t> samples;
        samples.reserve(numSamples);

        value_t totalLen = totalLength();

        for (size_t i = 0; i < numSamples; ++i) {
            value_t arcLen =
                (static_cast<value_t>(i) / (numSamples - 1)) * totalLen;
            value_t t = arcLengthToT(arcLen);
            samples.push_back(evaluate(t));
        }

        return samples;
    }

    // Extract sub-spline between t0 and t1
    [[nodiscard]] Spline2 subSpline(value_t t0, value_t t1) const {
        if (t0 > t1)
            std::swap(t0, t1);
        t0 = nv::clamp(t0, static_cast<value_t>(0), static_cast<value_t>(1));
        t1 = nv::clamp(t1, static_cast<value_t>(0), static_cast<value_t>(1));

        Spline2 result;

        size_t numSegs = numSegments();
        size_t seg0 = static_cast<size_t>(t0 * numSegs);
        size_t seg1 = static_cast<size_t>(t1 * numSegs);

        if (seg0 >= numSegs)
            seg0 = numSegs - 1;
        if (seg1 >= numSegs)
            seg1 = numSegs - 1;

        // Add start point
        value_t localT0 = t0 * numSegs - seg0;
        point_t startPos = getSegmentPoint(seg0, localT0);
        point_t startTan = getSegmentTangent(seg0, localT0);
        result.addPoint(startPos, -startTan, startTan);

        // Add intermediate control points
        for (size_t i = seg0 + 1; i <= seg1; ++i) {
            if (i < _control_points.size()) {
                result.addControlPoint(_control_points[i]);
            }
        }

        // Add end point
        value_t localT1 = t1 * numSegs - seg1;
        point_t endPos = getSegmentPoint(seg1, localT1);
        point_t endTan = getSegmentTangent(seg1, localT1);
        result.addPoint(endPos, -endTan, endTan);

        return result;
    }

    // Find closest point on spline to given point
    [[nodiscard]] std::optional<value_t>
    closestT(const point_t& target, int samplesPerSegment = 16) const {
        if (empty())
            return std::nullopt;

        value_t minDist = std::numeric_limits<value_t>::max();
        value_t bestT = 0;

        size_t numSegs = numSegments();

        for (size_t seg = 0; seg < numSegs; ++seg) {
            for (int i = 0; i <= samplesPerSegment; ++i) {
                value_t localT = static_cast<value_t>(i) / samplesPerSegment;
                point_t pt = getSegmentPoint(seg, localT);
                value_t dist = (pt - target).length2();

                if (dist < minDist) {
                    minDist = dist;
                    bestT = (seg + localT) / numSegs;
                }
            }
        }

        return bestT;
    }

    // Reverse the spline direction
    void reverse() {
        std::reverse(_control_points.begin(), _control_points.end());
        for (auto& cp : _control_points) {
            std::swap(cp.tangent_in, cp.tangent_out);
            cp.tangent_in = -cp.tangent_in;
            cp.tangent_out = -cp.tangent_out;
        }
        invalidateLengths();
    }

    // Transform all points
    template <typename TransformFunc> void transform(TransformFunc func) {
        for (auto& cp : _control_points) {
            cp.position = func(cp.position);
        }
        invalidateLengths();
    }

    // Serialization
    template <class Archive> void serialize(Archive& ar) {
        ar(_control_points, _closed);
        invalidateLengths();
    }
};

using Spline2f = Spline2<F32>;
using Spline2d = Spline2<F64>;

} // namespace nv

#endif