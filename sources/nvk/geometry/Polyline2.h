#ifndef _POLYLINE2_H_
#define _POLYLINE2_H_

#include <nvk/geometry/Segment2.h>

namespace nv {

template <typename T> struct Polyline2 {
    I32 id{-1};
    Vector<Vec2<T>> points;
    bool closedLoop{false};

    void reverse() { polyline2_reverse(points); }
    void append(const Polyline2<T>& rhs) {
        points.reserve(points.size() + rhs.points.size());
        points.insert(points.end(), rhs.points.begin(), rhs.points.end());
    }

    // Returns the point at the given arc-length along the polyline, clamped to
    // [0, total length].
    auto sample_at(T dist) const -> Vec2<T> {
        return polyline2_sample_at(points, dist);
    }

    auto length() const -> T { return polyline2_length(points); }

    auto dedupe_points() -> U32 {
        return polyline2_dedupe_points(points, closedLoop);
    }

    // Appends to out the polyline vertices whose arc-length lies strictly
    // inside (min(d0,d1)+eps, max(d0,d1)-eps), ordered from d0 towards d1 (i.e.
    // the output is reversed when d0 > d1). The 1 cm eps guard band avoids
    // emitting near-duplicates of the boundary points the caller inserts itself
    // (seam points, corner apexes).
    void append_slice(const Vector<Vec2<T>>& pts, T d0, T d1, T eps = 0.01) {
        polyline2_append_slice(points, pts, d0, d1, eps);
    }

    void append_polyline2_slice(const Polyline2<T>& line, T d0, T d1) {
        append_polyline2_slice(line.points, d0, d1);
    }

    // Appends to out the sampled points of an arc around center going from the
    // angle of `from` to the angle of `to` (endpoints excluded), rotating CCW
    // when ccw is true, CW otherwise. The radius is linearly interpolated
    // between |from-center| and |to-center| so slightly off-circle endpoints
    // still produce a smooth blend. The angular step is derived from
    // sampleDistCm at the largest radius. No-op when either endpoint coincides
    // with center.
    void append_arc_points(const Vec2<T>& center, const Vec2<T>& from,
                           const Vec2<T>& to, bool ccw, T sampleDistCm) {

        auto va = from - center;
        auto vb = to - center;
        T ra = va.length();
        T rb = vb.length();
        if (ra < 1e-6 || rb < 1e-6) {
            return;
        }

        T a0 = std::atan2(va.y(), va.x());
        T a1 = std::atan2(vb.y(), vb.x());

        T sweep = a1 - a0;
        if (ccw) {
            while (sweep <= 0.0) {
                sweep += 2.0 * PI;
            }
        } else {
            while (sweep >= 0.0) {
                sweep -= 2.0 * PI;
            }
        }

        NVCHK(sampleDistCm > 0.0,
              "append_arc_points: invalid sample distance.");
        F64 rmax = std::max(ra, rb);
        U32 nsegs =
            std::max(2u, U32(std::ceil(std::abs(sweep) * rmax / sampleDistCm)));

        for (U32 k = 1; k < nsegs; ++k) {
            F64 x = F64(k) / F64(nsegs);
            F64 ang = a0 + sweep * x;
            F64 r = ra + (rb - ra) * x;
            points.emplace_back(center +
                                Vec2d{std::cos(ang), std::sin(ang)} * r);
        }
    }

    auto find_first_intersection(const Polyline2<T>& pb, Polyline2Hit<T>& hit)
        -> bool {
        return polyline2_find_first_intersection(points, pb.points, hit);
    }
};

template <typename T> using Polyline2Vector = Vector<Polyline2<T>>;

using Polyline2f = Polyline2<F32>;
using Polyline2d = Polyline2<F64>;

template <typename T>
auto polyline_ray_intersections(const Vec2<T>& origin, const Vec2<T>& dir,
                                const Polyline2<T>& polyline) {
    return polyline_ray_intersections(origin, dir, polyline.points,
                                      polyline.closedLoop);
}

auto compute_polyline2_intersections(const Polyline2Vector<F32>& paths,
                                     F32 endpointDistance)
    -> Polyline2IntersectionResults<F32>;

auto compute_polyline2_intersections(const Polyline2Vector<F64>& paths,
                                     F64 endpointDistance)
    -> Polyline2IntersectionResults<F64>;

} // namespace nv

#endif
