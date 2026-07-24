#ifndef _PROFILE_H_
#define _PROFILE_H_

#include <nvk/geometry/geom_utils.h>

namespace nv {
// Generic lerp: works for scalar T==V (F32/F64) and for Vec2/Vec3/Vec4,
// since both cases support operator+, operator-, and operator*(value_t).
template <typename T, typename V>
[[nodiscard]] inline auto lerp_sample(const V& v0, const V& v1, T alpha) -> V {
    return v0 + (v1 - v0) * alpha;
}

template <typename T, typename V> struct Sample {
    T t{0.0};
    V v{0.0};
};

using SampleValf = Sample<F32, F32>;
using SampleVald = Sample<F64, F64>;

using SampleVec2f = Sample<F32, Vec2f>;
using SampleVec2d = Sample<F64, Vec2d>;

using SampleVec3f = Sample<F32, Vec3f>;
using SampleVec3d = Sample<F64, Vec3d>;

using SampleVec4f = Sample<F32, Vec4f>;
using SampleVec4d = Sample<F64, Vec4d>;

template <typename T, typename V> struct Profile {
    Vector<Sample<T, V>> samples;

    Profile() = default;
    explicit Profile(Vector<Sample<T, V>> inputs) : samples(std::move(inputs)) {
        std::sort(samples.begin(), samples.end(),
                  [](const Sample<T, V>& a, const Sample<T, V>& b) {
                      return a.t < b.t;
                  });
    }

    auto get_segment_index(T t, bool clamp = true) const -> I32 {
        if (samples.empty()) {
            return -1;
        }

        if (clamp) {
            t = std::clamp(t, samples.front().t, samples.back().t);
        }

        if (t < samples.front().t || t > samples.back().t)
            return -1;

        for (I32 i = 1; i < samples.size(); ++i) {
            if (samples[i - 1].t <= t && t < samples[i].t) {
                return i - 1;
            }
        }

        THROW_MSG("Should not be reached.");
        return -1;
    }

    auto get_closest_sample_index(T t) const -> I32 {
        if (samples.empty()) {
            return -1;
        }
        auto idx = get_segment_index(t, true);
        NVCHK(idx >= 0, "Invalid segment index.");
        return (t - samples[idx].t) > (samples[idx + 1].t - t) ? idx + 1 : idx;
    }

    void add_sample(T t, V v) {
        const auto pos = std::upper_bound(
            samples.begin(), samples.end(), t,
            [](T lhs, const Sample<T, V>& rhs) { return lhs < rhs.t; });
        samples.insert(pos, Sample<T, V>{t, v});
    }

    auto operator[](U32 i) const -> const Sample<T, V>& { return samples[i]; }
    auto operator[](U32 i) -> Sample<T, V>& { return samples[i]; }

    void clear() { return samples.clear(); }
    [[nodiscard]] auto empty() const -> bool { return samples.empty(); }
    [[nodiscard]] auto size() const -> U32 { return U32(samples.size()); }
    [[nodiscard]] auto front() const -> const Sample<T, V>& {
        return samples.front();
    }
    [[nodiscard]] auto back() const -> const Sample<T, V>& {
        return samples.back();
    }
    [[nodiscard]] auto begin() const { return samples.begin(); }
    [[nodiscard]] auto end() const { return samples.end(); }
    [[nodiscard]] auto begin() { return samples.begin(); }
    [[nodiscard]] auto end() { return samples.end(); }

    void set_v_values(const Vector<V>& vals) {
        NVCHK(vals.size() == samples.size(),
              "Mismatch in Profile::set_v_values()");
        for (U32 i = 0; i < vals.size(); ++i) {
            samples[i].v = vals[i];
        }
    }

    void set_t_values(const Vector<T>& vals) {
        NVCHK(vals.size() == samples.size(),
              "Mismatch in Profile::set_t_values()");
        for (U32 i = 0; i < vals.size(); ++i) {
            samples[i].t = vals[i];
        }
    }

    // Extracts just the t values from `samples`, in order.
    [[nodiscard]] auto extract_t_values() const -> Vector<T> {
        Vector<T> out;
        out.reserve(samples.size());
        for (const auto& s : samples)
            out.push_back(s.t);
        return out;
    }

    // Extracts just the v values from `samples`, in order.
    [[nodiscard]] auto extract_v_values() const -> Vector<V> {
        Vector<V> out;
        out.reserve(samples.size());
        for (const auto& s : samples)
            out.push_back(s.v);
        return out;
    }

    auto sub_range(T startT, T endT, bool normalizeOutputT = false,
                   T minPointDist = 1e-3) const -> Profile<T, V> {

        NVCHK(startT <= endT, "Invalid start/end T values.");
        I32 num = (I32)samples.size();
        NVCHK(num >= 2, "Not enough points in cline.");

        T clineMinT = samples.front().t;
        T clineMaxT = samples.back().t;

        if (endT < clineMinT || startT > clineMaxT) {
            return {};
        }

        T clampedStartT = std::clamp(startT, clineMinT, clineMaxT);
        T clampedEndT = std::clamp(endT, clineMinT, clineMaxT);
        NVCHK(clampedStartT <= clampedEndT, "Invalid start/end T values.");

        I32 startIdx = -1;
        I32 endIdx = -1;
        V startPt;
        V endPt;
        bool useStartPt = false;
        bool useEndPt = false;

        for (I32 i = 1; i < num; ++i) {
            if (startIdx == -1 && samples[i - 1].t <= clampedStartT &&
                clampedStartT <= samples[i].t) {
                auto denom = samples[i].t - samples[i - 1].t;
                NVCHK(denom > 0.0, "Invalid consecutive T values.");
                auto r = (clampedStartT - samples[i - 1].t) / denom;
                // startPt = samples[i - 1].v * (1.0 - r) + samples[i].v * r;
                startPt = lerp_sample(samples[i - 1].v, samples[i].v, r);
                useStartPt = (samples[i].t - clampedStartT) > minPointDist;
                startIdx = i;
            }
            if (endIdx == -1 && samples[i - 1].t <= clampedEndT &&
                clampedEndT <= samples[i].t) {
                auto denom = samples[i].t - samples[i - 1].t;
                NVCHK(denom > 0.0, "Invalid consecutive T values.");
                auto r = (clampedEndT - samples[i - 1].t) / denom;
                // endPt = samples[i - 1].v * (1.0 - r) + samples[i].v * r;
                endPt = lerp_sample(samples[i - 1].v, samples[i].v, r);
                useEndPt = (clampedEndT - samples[i - 1].t) > minPointDist;
                endIdx = i - 1;
                break;
            }
        }
        NVCHK(startIdx > -1 && endIdx > -1,
              "Cannot extract sub range from samples.");

        Profile<T, V> out;
        U32 ncopy = (endIdx - startIdx + 1);
        if (ncopy == 0 && useStartPt && useEndPt &&
            (clampedEndT - clampedStartT) <= minPointDist) {
            useEndPt = false; // keep only the start point
        }

        U32 n = ncopy + (useStartPt ? 1 : 0) + (useEndPt ? 1 : 0);

        if (n == 0) {
            // segment shorter than minPointDist: just emit a single
            // representative point
            out.samples.resize(1);
            out[0].t = clampedStartT;
            out[0].v = startPt; // or (startPt + endPt) * 0.5
        } else {
            out.samples.resize(n);
            if (useStartPt) {
                out[0].v = startPt;
                out[0].t = clampedStartT;
            }
            if (useEndPt) {
                out[n - 1].v = endPt;
                out[n - 1].t = clampedEndT;
            }

            if (ncopy > 0) {
                Sample<T, V>* ptr = out.samples.data() + (useStartPt ? 1 : 0);
                memcpy(ptr, &samples[startIdx], ncopy * sizeof(Sample<T, V>));
            }
        }

        if (normalizeOutputT) {
            T trange = clampedEndT - clampedStartT;
            for (auto& s : out) {
                s.t = trange > 0.0 ? (s.t - clampedStartT) / trange : 0.0;
            }
        }

        return out;
    }

    [[nodiscard]] auto sample(F64 t) const -> V {
        const U32 n = U32(samples.size());
        if (n == 0)
            return V{};
        if (n == 1)
            return samples[0].v;

        const F64 tMin = F64(samples[0].t);
        const F64 tMax = F64(samples[n - 1].t);
        const F64 tClamped = std::clamp(t, tMin, tMax);

        // Binary search for the first sample whose t exceeds tClamped.
        const auto it = std::upper_bound(
            samples.begin(), samples.end(), tClamped,
            [](F64 lhs, const Sample<T, V>& rhs) { return lhs < F64(rhs.t); });

        const U32 i1 = std::min(U32(std::distance(samples.begin(), it)), n - 1);
        const U32 i0 = (i1 == 0) ? 0 : i1 - 1;

        if (i0 == i1)
            return samples[i0].v;

        const F64 t0 = F64(samples[i0].t);
        const F64 t1 = F64(samples[i1].t);
        const F64 span = t1 - t0;

        // Guard against duplicate/degenerate t stamps.
        const F64 alpha = (span > 0.0) ? (tClamped - t0) / span : 0.0;

        return lerp_sample(samples[i0].v, samples[i1].v, T(alpha));
    }

    // Resamples this profile at uniform steps of `stepSize` along t, returning
    // a new Profile. Endpoints match exactly (no extrapolation). Coincident-t
    // pairs (e.g. corner-fan pivots) resolve to the higher v, so a resampled
    // elevation profile never dips below a pinched-together pair.
    [[nodiscard]] auto resampled(T stepSize) const -> Profile<T, V> {
        Profile<T, V> out;

        const U32 n = U32(samples.size());
        if (n == 0 || stepSize <= T(0))
            return out;
        if (n == 1) {
            out.samples.push_back(samples[0]);
            return out;
        }

        const T tMin = samples.front().t;
        const T tMax = samples.back().t;
        const T span = tMax - tMin;

        const U32 approxCount = U32(std::ceil(span / stepSize)) + 2;
        out.samples.reserve(approxCount);

        // Two-pointer walk: `seg` is the LEFT sample of the current
        // interpolation bracket [samples[seg].t, samples[seg + 1].t).
        U32 seg = 0;

        const U32 nSteps = U32(std::ceil(span / stepSize));
        for (U32 i = 0; i <= nSteps; ++i) {
            const T t = (i < nSteps) ? tMin + T(i) * stepSize : tMax;

            // Advance bracket so samples[seg].t <= t < samples[seg + 1].t.
            // The guard (seg + 2 < n) keeps seg + 1 a valid index.
            while (seg + 2 < n && samples[seg + 1].t <= t)
                ++seg;

            const T t0 = samples[seg].t;
            const T t1 = samples[seg + 1].t;
            const V v0 = samples[seg].v;
            const V v1 = samples[seg + 1].v;

            V v;
            const T dt = t1 - t0;
            if (dt < T(1e-12)) {
                // Coincident-t pair: take the higher value to stay above
                // ground.
                v = std::max(v0, v1);
            } else {
                const T frac = (t - t0) / dt;
                v = v0 + frac * (v1 - v0);
            }

            out.samples.push_back(Sample<T, V>{t, v});
        }

        return out;
    }
};

using ProfileValf = Profile<F32, F32>;
using ProfileVald = Profile<F64, F64>;

using ProfileVec2f = Profile<F32, Vec2f>;
using ProfileVec2d = Profile<F64, Vec2d>;

using ProfileVec3f = Profile<F32, Vec3f>;
using ProfileVec3d = Profile<F64, Vec3d>;

using ProfileVec4f = Profile<F32, Vec4f>;
using ProfileVec4d = Profile<F64, Vec4d>;

auto samples_apply_normal_offset(const ProfileVec2d& cline, F64 offset)
    -> ProfileVec2d;

} // namespace nv

#endif
