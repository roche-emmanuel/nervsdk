
#include <nvk_pcg.h>

namespace nv {
namespace {

auto get_segment_dir(PointArrayVector& paths, I32 pId, I32 segId) -> Vec2d {
    auto pt0 = paths[pId]->get_seg_start_point(segId);
    auto pt1 = paths[pId]->get_seg_end_point(segId);
    return (pt1.position().xy() - pt0.position().xy()).normalized();
};

auto get_intersection_min_angle(const Vec2d& dir0, const Vec2d& dir1) -> F64 {
    auto angle = dir0.angleTo(dir1);
    // If angle is larger than PI/2 returh the complement to PI:
    return angle > PI_2 ? (PI - angle) : angle;
}

void sort_ccw(Vector<Vec2d>& points) {
    std::ranges::sort(points, [](const Vec2d& a, const Vec2d& b) {
        double angleA = std::atan2(a.y(), a.x());
        double angleB = std::atan2(b.y(), b.x());

        return angleA < angleB;
    });
}

struct IntersectionConfig {
    Vector<Vec2d> mainPoints;
    Vector<I32> splineSegments; // num points per segment
    F64 spTension;
    F64 spPower;
    F64 radius;
};

auto compute_intersection_config(PCGContext& ctx, const Vec2d& dir0,
                                 const Vec2d& dir1, bool is4Way)
    -> IntersectionConfig {
    auto& in = ctx.inputs();

    auto angle = get_intersection_min_angle(dir0, dir1);
    F64 roadWidth = in.get("RoadWidth", 500.0);
    F64 halfWidth = roadWidth * 0.5;
    F64 minSpacing = in.get("TurnMinSpacing", 200.0);
    F64 halfSpacing = minSpacing * 0.5;
    F64 L = (halfWidth + halfSpacing) / std::tan(angle * 0.5);

    IntersectionConfig config;

    if (is4Way) {
        config.mainPoints = {dir0 * L, dir1 * L, -dir0 * L, -dir1 * L};
        sort_ccw(config.mainPoints);
        config.splineSegments =
            Vector<I32>(4, in.get("TurnSplineResolution", 20));

    } else {
        config.mainPoints = {dir0 * L};
        if (dir0.signedAngleTo(dir1) > 0.0) {
            config.mainPoints.push_back(dir1 * L);
            config.mainPoints.push_back(-dir1 * L);
        } else {
            config.mainPoints.push_back(-dir1 * L);
            config.mainPoints.push_back(dir1 * L);
        }
        I32 spNum = in.get("TurnSplineResolution", 20);
        config.splineSegments = {spNum, 2, spNum}; // middle segment is straight
    }

    config.radius = L;
    config.spTension = in.get("TurnTensionScale", 80.0);
    config.spPower = in.get("TurnTensionPower", 3.2);

    return config;
}

auto get_intersection_directions(PointArrayVector& paths, PCGPointRef& iPt,
                                 bool is4Way) -> std::pair<Vec2d, Vec2d> {
    if (is4Way) {
        auto dir0 = get_segment_dir(paths, iPt.get<I32>("line0_index"),
                                    iPt.get<I32>("seg0_index"));
        auto dir1 = get_segment_dir(paths, iPt.get<I32>("line1_index"),
                                    iPt.get<I32>("seg1_index"));
        return {dir0, dir1};
    }

    auto endPt = paths[iPt.get<I32>("line0_index")]->get_point(
        iPt.get<I32>("seg0_index"));
    auto dir0 = (endPt.position().xy() - iPt.position().xy()).normalized();
    auto dir1 = get_segment_dir(paths, iPt.get<I32>("line1_index"),
                                iPt.get<I32>("seg1_index"));
    return {dir0, dir1};
}

auto build_intersection_path(const Vec2d& center,
                             const IntersectionConfig& config, F64 halfWidth)
    -> RefPtr<PointArray> {
    I32 totalPoints = std::accumulate(config.splineSegments.begin(),
                                      config.splineSegments.end(), 0);
    auto path = PointArray::create(totalPoints);
    path->add_std_attributes();

    I32 idx = 0;
    I32 numSegments = (I32)config.mainPoints.size();

    for (I32 i = 0; i < numSegments; ++i) {
        const auto& pt0 = config.mainPoints[i];
        const auto& pt1 = config.mainPoints[(i + 1) % numSegments];

        auto xdir = pt0.normalized();
        auto ydir = xdir.ccw90();
        auto startPos = pt0 + ydir * halfWidth;

        auto xdir1 = pt1.normalized();
        auto ydir1 = xdir1.ccw90();
        auto endPos = pt1 - ydir1 * halfWidth;

        auto tension =
            config.spTension * std::pow(xdir.angleTo(xdir1), config.spPower);
        Spline2d sp({{startPos, -xdir * tension, -xdir * tension},
                     {endPos, xdir1 * tension, xdir1 * tension}});

        I32 num = config.splineSegments[i];
        for (I32 j = 0; j < num; ++j) {
            F64 t = F64(j) / F64(num - 1);
            auto pos = sp.evaluate(t);
            path->get_point(idx++).set_position(Vec3d(center + pos, 0.0));
        }
    }

    return path;
}

struct IntersectionDisc {
    Vec2d center;
    F64 radius;
    F64 radius2;
    Vector<Vec2d> snapPoints;
};

using IntersectionDiscVector = Vector<IntersectionDisc>;

auto handle_intersection(PCGContext& ctx, PCGPointRef& iPt,
                         PointArrayVector& outPaths, bool is4Way,
                         IntersectionDiscVector& idiscs) {
    auto& in = ctx.inputs();
    PointArrayVector& paths = in.get("In");

    auto [dir0, dir1] = get_intersection_directions(paths, iPt, is4Way);
    auto config = compute_intersection_config(ctx, dir0, dir1, is4Way);

    F64 roadWidth = in.get("RoadWidth", 500.0);
    F64 halfWidth = roadWidth * 0.5;
    auto center = iPt.position().xy();

    auto path = build_intersection_path(center, config, halfWidth);
    outPaths.emplace_back(path);

    idiscs.emplace_back(IntersectionDisc{
        .center = iPt.position().xy(),
        .radius = config.radius,
        .radius2 = config.radius * config.radius,
        .snapPoints = config.mainPoints,
    });
}

auto get_closest_point(const Vec2d& pos, const Vector<Vec2d>& points) -> Vec2d {
    if (points.empty()) {
        THROW_MSG("Empty snap point list.");
    }

    Vec2d best = points[0];
    F64 dist2 = (best - pos).length2();

    for (size_t i = 1; i < points.size(); ++i) {
        F64 L = (points[i] - pos).length2();
        if (L < dist2) {
            best = points[i];
            dist2 = L;
        }
    }

    return best;
}

void cut_road_paths(const RefPtr<PointArray>& path,
                    const IntersectionDiscVector& idiscs,
                    PointArrayVector& roadPaths) {

    RefPtr<PointArray> currentSection = nullptr;

    auto is_cutout = [&idiscs](const PCGPointRef& pt) -> I32 {
        auto pos = pt.position().xy();
        U32 num = idiscs.size();
        for (I32 i = 0; i < num; ++i) {
            if ((pos - idiscs[i].center).length2() < idiscs[i].radius2) {
                return i;
            }
        }
        return -1;
    };

    // Iterate on all the path points and check if we are inside a disc or not.
    U32 num = path->get_num_points();
    Vector<I32> pointDiscIndex(num, -1);

    I32 cIdx = -1;

    for (U32 i = 0; i < num; ++i) {
        auto pt = path->get_point(i);

        I32 curCircleIdx = is_cutout(pt);
        if (curCircleIdx >= 0) {
            // We are entering a circle:
            cIdx = curCircleIdx;

            // We need to remove this point.
            // So we check if we are currently build a section already.
            if (currentSection != nullptr) {
                // We have a section in progress. So we need to find the
                // intersection between the intersection disc and the segment to
                // this point:
                auto lastPt = currentSection->get_point(-1);
                F64 t{};
                auto res = seg2_circle_entry(
                    lastPt.position().xy(), pt.position().xy(),
                    idiscs[cIdx].center, idiscs[cIdx].radius, t);
                NVCHK(res, "Cannot find segment/disc intersection.");

                // Compute the interpolated point:
                if (t > 0.0) {
                    auto endPt = PCGPoint::mix(lastPt, pt, t);

                    auto center = idiscs[cIdx].center;

                    // Snap the end point to the closest snap point we have:
                    auto snapPos =
                        get_closest_point(endPt.position().xy() - center,
                                          idiscs[cIdx].snapPoints);

                    endPt.set_position(Vec3d(center + snapPos, 0.0));

                    // When we add a end point, we should update its Yaw
                    // rotation to match the direction to the center of
                    // the sphere:
                    auto dir = (-snapPos).normalized();
                    auto angle = toDeg(Vec2d(1.0, 0.0).signedAngleTo(dir));
                    endPt.set_rotation({0.0, 0.0, angle});
                    currentSection->add_point(endPt);
                }

                // Add this section to the list:
                roadPaths.emplace_back(currentSection);

                // Now reset the section:
                currentSection = nullptr;
            }
        } else {
            // we need to keep this point:
            if (currentSection == nullptr) {
                // We need to create a new section first:
                currentSection = PointArray::create_like(path, 0);

                // Check if there was a previous point inside the circle
                if (i > 0) {
                    NVCHK(cIdx >= 0, "Unexpected current intersection circle.");

                    auto lastPt = path->get_point(i - 1);
                    F64 t{};
                    auto res = seg2_circle_exit(
                        lastPt.position().xy(), pt.position().xy(),
                        idiscs[cIdx].center, idiscs[cIdx].radius, t);
                    NVCHK(res, "Cannot find segment/disc intersection.");
                    // NVCHK(is_cutout(lastPt) == cIdx,
                    //       "Invalid is_cutout result.");

                    // logDEBUG("Adding road section start point with t={}", t);

                    // Compute the interpolated point:
                    // if (t < 1.0) {
                    auto startPt = PCGPoint::mix(lastPt, pt, t);

                    auto center = idiscs[cIdx].center;

                    // Snap the end point to the closest snap point we have:
                    auto snapPos =
                        get_closest_point(startPt.position().xy() - center,
                                          idiscs[cIdx].snapPoints);

                    startPt.set_position(Vec3d(center + snapPos, 0.0));

                    // When we add a start point, we should update its Yaw
                    // rotation to match the direction from the center of
                    // the sphere:
                    auto dir = (snapPos).normalized();
                    auto angle = toDeg(Vec2d(1.0, 0.0).signedAngleTo(dir));
                    startPt.set_rotation({0.0, 0.0, angle});
                    currentSection->add_point(startPt);

                    cIdx = -1;
                    // }
                }
            }

            currentSection->add_point(pt);
        }
    }

    // Add the last current section if any:
    if (currentSection != nullptr) {
        roadPaths.emplace_back(currentSection);
        currentSection = nullptr;
    }
}

auto cut_all_road_paths(PCGContext& ctx, const IntersectionDiscVector& idiscs)
    -> PointArrayVector {
    auto& in = ctx.inputs();
    PointArrayVector& paths = in.get("In");

    PointArrayVector roadPaths;

    for (const auto& path : paths) {
        cut_road_paths(path, idiscs, roadPaths);
    }

    return roadPaths;
}

} // namespace

/** Find intersections from all the input paths. */
void pcg_build_intersection_contours(PCGContext& ctx) {
    auto& out = ctx.outputs();
    pcg_find_path_2d_intersections(ctx);

    RefPtr<PointArray> rawIntersections = out.get("Out");
    U32 nIntersecs = rawIntersections->get_num_points();
    PointArrayVector outPaths;

    IntersectionDiscVector idiscs;

    for (U32 i = 0; i < nIntersecs; ++i) {
        auto iPoint = rawIntersections->get_point(i);
        auto itype = iPoint.get<I32>("intersect_type");

        switch (itype) {
        case ITYPE_4WAY:
            handle_intersection(ctx, iPoint, outPaths, true, idiscs);
            break;
        case ITYPE_3WAY:
            handle_intersection(ctx, iPoint, outPaths, false, idiscs);
            break;
        default:
            break;
        }
    }

    ctx.outputs().set("Out", outPaths, true);

    // Once we have the intersection discs we need to split the input paths
    // cutting the discs where applicable:
    PointArrayVector roadPaths = cut_all_road_paths(ctx, idiscs);

    // Re-sample the roadPaths:
    auto ctx2 = PCGContext::create();
    auto& in2 = ctx2->inputs();
    in2.set("In", roadPaths);
    in2.set("Distance", 100.0);
    in2.set("FitToCurve", true);
    pcg_resample_paths(*ctx2);

    ctx.outputs().set("RoadSections", roadPaths);
}

} // namespace nv